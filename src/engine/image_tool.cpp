/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2022                                             *
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

#include <cassert>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <vector>

#include <SDL_error.h>
#include <SDL_version.h>

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#include <SDL_pixels.h>
#include <SDL_surface.h>
#else
#include <SDL_video.h>
#endif

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#if defined( WITH_IMAGE )
#define ENABLE_PNG
#include <SDL_image.h>
#endif
#endif

#include "image_palette.h"
#include "image_tool.h"
#include "logging.h"

namespace
{
    bool isPNGFilePath( const std::string & path )
    {
        const std::string pngExtension( ".png" );
        return path.size() > pngExtension.size() && ( path.compare( path.size() - pngExtension.size(), pngExtension.size(), pngExtension ) == 0 );
    }

    std::vector<uint8_t> PALPalette()
    {
        const uint8_t * gamePalette = fheroes2::getGamePalette();

        std::vector<uint8_t> palette( 256 * 3 );
        for ( size_t i = 0; i < palette.size(); ++i ) {
            palette[i] = gamePalette[i] << 2;
        }

        return palette;
    }

#if defined( ENABLE_PNG )
    bool SaveImage( const fheroes2::Image & image, const std::string & path )
#else
    bool SaveImage( const fheroes2::Image & image, std::string path )
#endif
    {
        const std::vector<uint8_t> & palette = PALPalette();
        const uint8_t * currentPalette = palette.data();

        const int32_t width = image.width();
        const int32_t height = image.height();

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        SDL_Surface * surface = SDL_CreateRGBSurface( 0, width, height, 8, 0, 0, 0, 0 );
#else
        SDL_Surface * surface = SDL_CreateRGBSurface( SDL_SWSURFACE, width, height, 8, 0xFF, 0xFF00, 0xFF0000, 0xFF000000 );
#endif
        if ( surface == nullptr ) {
            ERROR_LOG( "Error while creating a SDL surface for an image to be saved under " << path << ". Error " << SDL_GetError() )
            return false;
        }

        assert( surface->format->BitsPerPixel == 8 );

        std::vector<SDL_Color> paletteSDL;
        paletteSDL.resize( 256 );
        for ( int32_t i = 0; i < 256; ++i ) {
            const uint8_t * value = currentPalette + i * 3;
            SDL_Color & col = paletteSDL[i];

            col.r = *value;
            col.g = *( value + 1 );
            col.b = *( value + 2 );
        }

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        SDL_SetPaletteColors( surface->format->palette, paletteSDL.data(), 0, 256 );
#else
        SDL_SetPalette( surface, SDL_LOGPAL | SDL_PHYSPAL, paletteSDL.data(), 0, 256 );
#endif

        if ( surface->pitch != width ) {
            const uint8_t * imageIn = image.image();

            for ( int32_t i = 0; i < height; ++i ) {
                memcpy( static_cast<uint8_t *>( surface->pixels ) + surface->pitch * i, imageIn + width * i, static_cast<size_t>( width ) );
            }
        }
        else {
            memcpy( surface->pixels, image.image(), static_cast<size_t>( width * height ) );
        }

#if defined( ENABLE_PNG )
        int res = 0;
        if ( isPNGFilePath( path ) ) {
            res = IMG_SavePNG( surface, path.c_str() );
        }
        else {
            res = SDL_SaveBMP( surface, path.c_str() );
        }
#else
        if ( isPNGFilePath( path ) ) {
            memcpy( path.data() + path.size() - 3, "bmp", 3 );
        }

        const int res = SDL_SaveBMP( surface, path.c_str() );
#endif

        SDL_FreeSurface( surface );

        return res == 0;
    }
}

namespace fheroes2
{
    bool Save( const Image & image, const std::string & path, const uint8_t background )
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
                    *outX = GetColorId( *( inX + 2 ), *( inX + 1 ), *inX );
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
                        else if ( *inX == 0 && *( inX + 1 ) == 0 && *( inX + 2 ) == 0 ) {
                            *transformX = 2;
                        }
                        else {
                            *outX = GetColorId( *( inX + 2 ), *( inX + 1 ), *inX );
                            *transformX = 0;
                        }
                    }
                    else {
                        *outX = GetColorId( *( inX + 2 ), *( inX + 1 ), *inX );
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

        while ( true ) {
            if ( 0 == *data ) { // 0x00 - end of row
                imageData += width;
                imageTransform += width;
                posX = 0;
                ++data;
            }
            else if ( 0x80 > *data ) { // 0x01-0x7F - repeat a pixel N times
                uint32_t pixelCount = *data;
                ++data;
                while ( pixelCount > 0 && data != dataEnd ) {
                    imageData[posX] = *data;
                    imageTransform[posX] = 0;
                    ++posX;
                    ++data;
                    --pixelCount;
                }
            }
            else if ( 0x80 == *data ) { // 0x80 - end of image
                break;
            }
            else if ( 0xC0 > *data ) { // 0xBF - empty (transparent) pixels
                posX += *data - 0x80;
                ++data;
            }
            else if ( 0xC0 == *data ) { // 0xC0 - transform layer
                ++data;

                const uint8_t transformValue = *data;
                const uint8_t transformType = static_cast<uint8_t>( ( ( transformValue & 0x3C ) << 6 ) / 256 + 2 ); // 1 is for skipping

                uint32_t pixelCount = *data % 4 ? *data % 4 : *( ++data );

                if ( ( transformValue & 0x40 ) && ( transformType <= 15 ) ) {
                    while ( pixelCount > 0 ) {
                        imageTransform[posX] = transformType;
                        ++posX;
                        --pixelCount;
                    }
                }
                else {
                    posX += pixelCount;
                }

                ++data;
            }
            else if ( 0xC1 == *data ) { // 0xC1
                ++data;
                uint32_t pixelCount = *data;
                ++data;
                while ( pixelCount > 0 ) {
                    imageData[posX] = *data;
                    imageTransform[posX] = 0;
                    ++posX;
                    --pixelCount;
                }
                ++data;
            }
            else {
                uint32_t pixelCount = *data - 0xC0;
                ++data;
                while ( pixelCount > 0 ) {
                    imageData[posX] = *data;
                    imageTransform[posX] = 0;
                    ++posX;
                    --pixelCount;
                }
                ++data;
            }

            if ( data >= dataEnd ) {
                break;
            }
        }

        return sprite;
    }

    bool isPNGFormatSupported()
    {
#if defined( ENABLE_PNG )
        return true;
#else
        return false;
#endif
    }
}
