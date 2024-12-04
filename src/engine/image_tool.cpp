/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <ostream>
#include <string_view>
#include <vector>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#include <SDL_error.h>
#include <SDL_pixels.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>
#include <SDL_version.h>

#if defined( WITH_IMAGE )
#include <SDL_image.h>
#endif

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif

#include "agg_file.h"
#include "image_palette.h"
#include "image_tool.h"
#include "logging.h"
#include "system.h"

namespace
{
    bool isPNGFilePath( const std::string_view path )
    {
        const std::string pngExtension( ".png" );
        return path.size() >= pngExtension.size() && ( path.compare( path.size() - pngExtension.size(), pngExtension.size(), pngExtension ) == 0 );
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

#if defined( WITH_IMAGE )
    bool SaveImage( const fheroes2::Image & image, const std::string & path )
#else
    bool SaveImage( const fheroes2::Image & image, std::string path )
#endif
    {
        const std::vector<uint8_t> & palette = PALPalette();
        const uint8_t * currentPalette = palette.data();

        const int32_t width = image.width();
        const int32_t height = image.height();

        const std::unique_ptr<SDL_Surface, void ( * )( SDL_Surface * )> surface( SDL_CreateRGBSurface( 0, width, height, 8, 0, 0, 0, 0 ), SDL_FreeSurface );
        if ( !surface ) {
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
            col.a = 255;
        }

        SDL_SetPaletteColors( surface->format->palette, paletteSDL.data(), 0, 256 );

        if ( surface->pitch != width ) {
            const uint8_t * imageIn = image.image();

            for ( int32_t i = 0; i < height; ++i ) {
                memcpy( static_cast<uint8_t *>( surface->pixels ) + surface->pitch * i, imageIn + width * i, static_cast<size_t>( width ) );
            }
        }
        else {
            memcpy( surface->pixels, image.image(), static_cast<size_t>( width * height ) );
        }

#if defined( WITH_IMAGE )
        int res = 0;

        if ( isPNGFilePath( path ) ) {
            res = IMG_SavePNG( surface.get(), System::encLocalToUTF8( path ).c_str() );
        }
        else {
            res = SDL_SaveBMP( surface.get(), System::encLocalToUTF8( path ).c_str() );
        }
#else
        if ( isPNGFilePath( path ) ) {
            memcpy( path.data() + path.size() - 3, "bmp", 3 );
        }

        const int res = SDL_SaveBMP( surface.get(), System::encLocalToUTF8( path ).c_str() );
#endif

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
        if ( image.singleLayer() ) {
            // Output image should be double-layer!
            assert( 0 );
            return false;
        }

        std::unique_ptr<SDL_Surface, void ( * )( SDL_Surface * )> surface( nullptr, SDL_FreeSurface );
        std::unique_ptr<SDL_Surface, void ( * )( SDL_Surface * )> loadedSurface( nullptr, SDL_FreeSurface );

#if defined( WITH_IMAGE )
        loadedSurface.reset( IMG_Load( System::encLocalToUTF8( path ).c_str() ) );
#else
        loadedSurface.reset( SDL_LoadBMP( System::encLocalToUTF8( path ).c_str() ) );
#endif
        if ( !loadedSurface ) {
            return false;
        }

// SDL_PIXELFORMAT_BGRA32 and other RGBA color variants are only supported starting with SDL 2.0.5
#if !SDL_VERSION_ATLEAST( 2, 0, 5 )
#error Minimal supported SDL version is 2.0.5.
#endif

        // Image loading functions can theoretically return SDL_Surface in any supported color format, so we will convert it to a specific format for subsequent
        // processing
        const std::unique_ptr<SDL_PixelFormat, void ( * )( SDL_PixelFormat * )> pixelFormat( SDL_AllocFormat( SDL_PIXELFORMAT_BGRA32 ), SDL_FreeFormat );
        if ( !pixelFormat ) {
            return false;
        }

        surface.reset( SDL_ConvertSurface( loadedSurface.get(), pixelFormat.get(), 0 ) );
        if ( !surface ) {
            return false;
        }

        assert( SDL_MUSTLOCK( surface.get() ) == SDL_FALSE && surface->format->BytesPerPixel == 4 );

        image.resize( surface->w, surface->h );
        image.reset();

        const uint8_t * inY = static_cast<uint8_t *>( surface->pixels );
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
                        *outX = 0;
                        *transformX = 1;
                    }
                    else if ( *inX == 0 && *( inX + 1 ) == 0 && *( inX + 2 ) == 0 ) {
                        *outX = 0;
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

        return true;
    }

    Sprite decodeICNSprite( const uint8_t * data, const uint8_t * dataEnd, const ICNHeader & icnHeader )
    {
        Sprite sprite( icnHeader.width, icnHeader.height, icnHeader.offsetX, icnHeader.offsetY );
        sprite.reset();

        uint8_t * imageTransform = sprite.transform();

        uint32_t posX = 0;

        // The need for a transform layer can only be determined during ICN decoding.
        bool noTransformLayer = true;

        // When the 6th bit in animationFrames is set then it is Monochromatic ICN image.
        const bool isMonochromatic = ( icnHeader.animationFrames & 0x20 );

        if ( isMonochromatic ) {
            while ( data < dataEnd ) {
                if ( *data == 0 ) {
                    // 0x00 - end of row reached, go to the first pixel of next row.

                    noTransformLayer = noTransformLayer && ( static_cast<int32_t>( posX ) >= icnHeader.width );

                    imageTransform += icnHeader.width;
                    posX = 0;
                    ++data;
                }
                else if ( *data < 0x80 ) {
                    // 0x01-0x7F - number of black pixels.
                    // Image data is all already set to 0. Just set transform layer to 0.

                    const uint8_t pixelCount = *data;

                    memset( imageTransform + posX, static_cast<uint8_t>( 0 ), pixelCount );

                    ++data;
                    posX += pixelCount;
                }
                else if ( *data == 0x80 ) {
                    // 0x80 - end of image.

                    break;
                }
                else {
                    // 0x81 to 0xFF - number of empty (transparent) pixels + 0x80.
                    // The (n - 128) pixels are transparent.

                    noTransformLayer = false;

                    posX += *data - 0x80;
                    ++data;
                }
            }
        }
        else {
            uint8_t * imageData = sprite.image();

            while ( data < dataEnd ) {
                if ( *data == 0 ) {
                    // 0x00 - end of row reached, go to the first pixel of next row.
                    // All of remaining pixels of current line are transparent.

                    noTransformLayer = noTransformLayer && ( static_cast<int32_t>( posX ) >= icnHeader.width );

                    imageData += icnHeader.width;
                    imageTransform += icnHeader.width;
                    posX = 0;
                    ++data;
                }
                else if ( *data < 0x80 ) {
                    // 0x01-0x7F - number N of sprite pixels.
                    // The next N bytes are the colors of the next N pixels.

                    const uint8_t pixelCount = *data;
                    ++data;

                    if ( data + pixelCount > dataEnd ) {
                        // Image data is corrupted - we can not read data beyond dataEnd.
                        break;
                    }

                    memcpy( imageData + posX, data, pixelCount );
                    memset( imageTransform + posX, static_cast<uint8_t>( 0 ), pixelCount );

                    data += pixelCount;
                    posX += pixelCount;
                }
                else if ( *data == 0x80 ) {
                    // 0x80 - end of image

                    noTransformLayer = noTransformLayer && ( static_cast<int32_t>( posX ) >= icnHeader.width );

                    break;
                }
                else if ( *data < 0xC0 ) {
                    // 0x81 to 0xBF - number of empty (transparent) pixels + 0x80. The (n - 128) pixels are transparent.

                    noTransformLayer = false;

                    posX += *data - 0x80;
                    ++data;
                }
                else if ( *data == 0xC0 ) {
                    // 0xC0 - put here N transform layer pixels.
                    // If the next byte modulo 4 is not null, N equals the next byte modulo 4,
                    // otherwise N equals the second next byte.

                    noTransformLayer = false;

                    ++data;

                    const uint8_t transformValue = *data;

                    const uint32_t countValue = transformValue & 0x03;
                    const uint32_t pixelCount = ( countValue != 0 ) ? countValue : *( ++data );

                    if ( transformValue & 0x40 ) {
                        // Transform layer data types:
                        // 0 - no transparency,
                        // 1 - full transparency (to skip image data),
                        // from 5 (light) to 2 (strong) - for darkening,
                        // from 10 (light) to 6 (strong) - for lightening
                        const uint8_t transformType = static_cast<uint8_t>( ( ( transformValue & 0x3C ) >> 2 ) + 2 );

                        if ( transformType < 16 ) {
                            memset( imageTransform + posX, transformType, pixelCount );
                        }
                    }

                    // TODO: Use ( transformValue & 0x80 ) to detect and store shining contour data bit.
                    // It is used for units on the Battlefield and for icons in the View World.

                    posX += pixelCount;

                    ++data;
                }
                else {
                    // 0xC1 - next byte stores the number of next pixels of same color, or
                    // 0xC2 to 0xFF - the number of pixels of same color plus 0xC0.
                    // Next byte is the color of these pixels.

                    const uint32_t pixelCount = ( *data == 0xC1 ) ? *( ++data ) : *data - 0xC0;
                    ++data;

                    memset( imageData + posX, *data, pixelCount );
                    memset( imageTransform + posX, static_cast<uint8_t>( 0 ), pixelCount );

                    posX += pixelCount;

                    ++data;
                }
            }
        }

        if ( noTransformLayer ) {
            sprite._disableTransformLayer();
        }

        return sprite;
    }

    void decodeTILImages( const uint8_t * data, const size_t imageCount, const int32_t width, const int32_t height, std::vector<Image> & output )
    {
        assert( data != nullptr && imageCount > 0 && width > 0 && height > 0 );

        output.resize( imageCount );

        const size_t imageSize = static_cast<size_t>( width ) * height;

        for ( size_t i = 0; i < imageCount; ++i ) {
            Image & tilImage = output[i];
            tilImage._disableTransformLayer();
            tilImage.resize( width, height );
            memcpy( tilImage.image(), data + i * imageSize, imageSize );
        }
    }

    bool isPNGFormatSupported()
    {
#if defined( WITH_IMAGE )
        return true;
#else
        return false;
#endif
    }
}
