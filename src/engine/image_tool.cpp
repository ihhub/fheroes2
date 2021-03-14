/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "image_tool.h"
#include "palette_h2.h"

#include <SDL_version.h>
#include <SDL_video.h>

#if defined( FHEROES2_IMAGE_SUPPORT )
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#define FHEROES2_ENABLE_PNG 1
#include <SDL_image.h>
#endif
#endif

namespace
{
    std::vector<uint8_t> PALPAlette()
    {
        std::vector<uint8_t> palette( 256 * 3 );
        for ( size_t i = 0; i < palette.size(); ++i ) {
            palette[i] = kb_pal[i] << 2;
        }

        return palette;
    }

    bool SaveImage( const fheroes2::Image & image, const std::string & path )
    {
        const std::vector<uint8_t> & palette = PALPAlette();
        const uint8_t * currentPalette = palette.data();

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        SDL_Surface * surface = SDL_CreateRGBSurface( 0, image.width(), image.height(), 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000 );
#else
        SDL_Surface * surface = SDL_CreateRGBSurface( SDL_SWSURFACE, image.width(), image.height(), 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000 );
#endif
        if ( surface == NULL )
            return false;

        const uint32_t width = image.width();
        const uint32_t height = image.height();

        uint32_t * out = static_cast<uint32_t *>( surface->pixels );
        const uint32_t * outEnd = out + width * height;
        const uint8_t * in = image.image();

        if ( surface->format->Amask > 0 ) {
            const uint8_t * transform = image.transform();

            for ( ; out != outEnd; ++out, ++in, ++transform ) {
                if ( *transform == 1 ) {
                    *out = SDL_MapRGBA( surface->format, 0, 0, 0, 0 );
                }
                else if ( *transform == 2 ) {
                    *out = SDL_MapRGBA( surface->format, 0, 0, 0, 64 );
                }
                else {
                    const uint8_t * value = currentPalette + *in * 3;
                    *out = SDL_MapRGBA( surface->format, *( value ), *( value + 1 ), *( value + 2 ), 255 );
                }
            }
        }
        else {
            for ( ; out != outEnd; ++out, ++in ) {
                const uint8_t * value = currentPalette + *in * 3;
                *out = SDL_MapRGB( surface->format, *( value ), *( value + 1 ), *( value + 2 ) );
            }
        }

#if defined( FHEROES2_ENABLE_PNG )
        int res = 0;
        const std::string pngExtension( ".png" );
        if ( path.size() > pngExtension.size() && path.compare( path.size() - pngExtension.size(), pngExtension.size(), pngExtension ) ) {
            res = IMG_SavePNG( surface, path.c_str() );
        }
        else {
            res = SDL_SaveBMP( surface, path.c_str() );
        }
#else
        const int res = SDL_SaveBMP( surface, path.c_str() );
#endif

        SDL_FreeSurface( surface );

        return res == 0;
    }
}

namespace fheroes2
{
    bool Save( const Image & image, const std::string & path, uint8_t background )
    {
        if ( image.empty() || path.empty() )
            return false;

        Image temp( image.width(), image.height() );
        temp.fill( background );

        Blit( image, temp );

        return SaveImage( temp, path );
    }

    bool Save( const Image & image, const std::string & path )
    {
        if ( image.empty() || path.empty() )
            return false;

        return SaveImage( image, path );
    }

    bool Load( const std::string & path, Image & image )
    {
        SDL_Surface * surface = SDL_LoadBMP( path.c_str() );
        if ( surface == nullptr ) {
            return false;
        }

        if ( surface->format->BytesPerPixel == 3 ) {
            image.resize( surface->w, surface->h );
            memset( image.transform(), 0, surface->w * surface->h );

            const uint8_t * inY = reinterpret_cast<uint8_t *>( surface->pixels );
            uint8_t * outY = image.image();

            const uint8_t * inYEnd = inY + surface->h * surface->pitch;

            for ( ; inY != inYEnd; inY += surface->pitch, outY += surface->w ) {
                const uint8_t * inX = inY;
                uint8_t * outX = outY;
                const uint8_t * inXEnd = inX + surface->w * 3;

                for ( ; inX != inXEnd; inX += 3, ++outX ) {
                    *outX = GetColorId( *( inX + 2 ), *( inX + 1 ), *( inX ) );
                }
            }
        }
        else if ( surface->format->BytesPerPixel == 4 ) {
            image.resize( surface->w, surface->h );
            image.reset();

            const uint8_t * inY = reinterpret_cast<uint8_t *>( surface->pixels );
            uint8_t * outY = image.image();
            uint8_t * transformY = image.transform();

            const uint8_t * inYEnd = inY + surface->h * surface->pitch;

            for ( ; inY != inYEnd; inY += surface->pitch, outY += surface->w, transformY += surface->w ) {
                const uint8_t * inX = inY;
                uint8_t * outX = outY;
                uint8_t * transformX = transformY;
                const uint8_t * inXEnd = inX + surface->w * 4;

                for ( ; inX != inXEnd; inX += 4, ++outX, ++transformX ) {
                    const uint8_t alpha = *( inX + 3 );
                    if ( alpha < 255 ) {
                        if ( alpha == 0 ) {
                            *transformX = 1;
                        }
                        else if ( *( inX ) == 0 && *( inX + 1 ) == 0 && *( inX + 2 ) == 0 ) {
                            *transformX = 2;
                        }
                        else {
                            *outX = GetColorId( *( inX + 2 ), *( inX + 1 ), *( inX ) );
                            *transformX = 0;
                        }
                    }
                    else {
                        *outX = GetColorId( *( inX + 2 ), *( inX + 1 ), *( inX ) );
                        *transformX = 0;
                    }
                }
            }
        }
        else {
            SDL_FreeSurface( surface );
            return false;
        }

        SDL_FreeSurface( surface );

        return true;
    }

    Sprite decodeICNSprite( const uint8_t * data, uint32_t sizeData, const int32_t width, const int32_t height, const int16_t offsetX, const int16_t offsetY )
    {
        Sprite sprite( width, height, offsetX, offsetY );
        sprite.reset();

        uint8_t * imageData = sprite.image();
        uint8_t * imageTransform = sprite.transform();

        uint32_t posX = 0;

        const uint8_t * dataEnd = data + sizeData;

        while ( 1 ) {
            if ( 0 == *data ) { // 0x00 - end line
                imageData += width;
                imageTransform += width;
                posX = 0;
                ++data;
            }
            else if ( 0x80 > *data ) { // 0x7F - count data
                uint32_t c = *data;
                ++data;
                while ( c-- && data != dataEnd ) {
                    imageData[posX] = *data;
                    imageTransform[posX] = 0;
                    ++posX;
                    ++data;
                }
            }
            else if ( 0x80 == *data ) { // 0x80 - end data
                break;
            }
            else if ( 0xC0 > *data ) { // 0xBF - skip data
                posX += *data - 0x80;
                ++data;
            }
            else if ( 0xC0 == *data ) { // 0xC0 - transform layer
                ++data;

                const uint8_t transformValue = *data;
                const uint8_t transformType = static_cast<uint8_t>( ( ( transformValue & 0x3C ) << 6 ) / 256 + 2 ); // 1 is for skipping

                uint32_t c = *data % 4 ? *data % 4 : *( ++data );

                if ( ( transformValue & 0x40 ) && ( transformType <= 15 ) ) {
                    while ( c-- ) {
                        imageTransform[posX] = transformType;
                        ++posX;
                    }
                }
                else {
                    posX += c;
                }

                ++data;
            }
            else if ( 0xC1 == *data ) { // 0xC1
                ++data;
                uint32_t c = *data;
                ++data;
                while ( c-- ) {
                    imageData[posX] = *data;
                    imageTransform[posX] = 0;
                    ++posX;
                }
                ++data;
            }
            else {
                uint32_t c = *data - 0xC0;
                ++data;
                while ( c-- ) {
                    imageData[posX] = *data;
                    imageTransform[posX] = 0;
                    ++posX;
                }
                ++data;
            }

            if ( data >= dataEnd ) {
                break;
            }
        }

        return sprite;
    }
}
