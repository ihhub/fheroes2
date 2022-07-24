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

#include <array>
#if defined( WITH_IMAGE )
#define ENABLE_PNG
#include <png.h>
#else
#include <cstring>
#endif

#include "image.h"
#include "image_palette.h"
#include "image_tool.h"
#include "serialize.h"

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

    std::vector<uint8_t> getRGBAPixels( const fheroes2::Image & image )
    {
        const std::vector<uint8_t> & palette = PALPalette();
        const uint8_t * currentPalette = palette.data();

        size_t count = image.width() * image.height();
        std::vector<uint8_t> out( count * 4 );
        const uint8_t * in = image.image();
        const uint8_t * transform = image.transform();

        for ( size_t i = 0; i < count; i++ ) {
            if ( transform[i] == 1 ) {
                out[4 * i] = out[4 * i + 1] = out[4 * i + 2] = out[4 * i + 3] = 0;
            }

            else if ( transform[i] == 2 ) {
                out[4 * i] = out[4 * i + 1] = out[4 * i + 2] = 0;
                out[4 * i + 3] = 64;
            }
            else {
                out[4 * i] = *( currentPalette + 3 * in[i] );
                out[4 * i + 1] = *( currentPalette + 3 * in[i] + 1 );
                out[4 * i + 2] = *( currentPalette + 3 * in[i] + 2 );
                out[4 * i + 3] = 255;
            }
        }
        return out;
    }

    bool SaveBMP( const fheroes2::Image & image, const std::string & path )
    {
        const size_t HEADER_SIZE = 124;
        StreamBuf buf( HEADER_SIZE );
        const std::vector<uint8_t> pixels = getRGBAPixels( image );
        // write BITMAPFILEHEADER
        buf.putLE16( 0x4D42 ); // signature "BM"
        buf.putLE32( static_cast<uint32_t>( HEADER_SIZE + pixels.size() ) ); // filesize
        buf.putLE32( 0 ); // two reserved WORDs
        buf.putLE32( HEADER_SIZE ); // TODO: pixels offset
        // write BITMAPV4HEADER
        buf.putLE32( 108 ); // sizeof( BITMAPV4HEADER )
        buf.putLE32( static_cast<int32_t>( image.width() ) );
        buf.putLE32( -static_cast<int32_t>( image.height() ) );
        buf.putLE16( 1 ); // planes, for BMP always 1
        buf.putLE16( 32 ); // bits per pixel
        buf.putLE32( 3 ); // compression BI_BITFIELDS
        buf.putLE32( static_cast<uint32_t>( pixels.size() ) ); // image size
        buf.putLE32( 0 );
        buf.putLE32( 0 );
        buf.putLE32( 0 );
        buf.putLE32( 0 );
        buf.putLE32( 0x000000FF ); //
        buf.putLE32( 0x0000FF00 ); // RGBA Bit masks
        buf.putLE32( 0x00FF0000 ); //
        buf.putLE32( 0xFF000000 ); //
        buf.putLE32( 0x57696E20 ); // Color space LCS_WINDOWS_COLOR_SPACE
        std::array<uint8_t, 50> tail{}; // tail of the BITMAPV4HEADER + 2 bytes for aligment
        buf.putRaw( reinterpret_cast<const char *>( &tail[0] ), 50 );

        StreamFile sf;
        if ( !sf.open( path, "wb" ) )
            return false;
        sf.putRaw( reinterpret_cast<const char *>( buf.getRaw().data() ), HEADER_SIZE );
        sf.putRaw( reinterpret_cast<const char *>( pixels.data() ), pixels.size() );
        sf.close();
        return true;
    }

#if defined( ENABLE_PNG )
    bool SavePNG( const fheroes2::Image & image, const std::string & path )
    {
        StreamFile sf;
        if ( !sf.open( path, "wb" ) ) {
            return false;
        }

        const int width = image.width();
        const int height = image.height();

        png_structp writePtr = png_create_write_struct( PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr );
        png_infop infoPtr = png_create_info_struct( writePtr );
        png_set_IHDR( writePtr, infoPtr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );

        std::vector<uint8_t> pixels = getRGBAPixels( image );
        std::vector<uint8_t *> rowsPtr( height );

        for ( int i = 0; i < height; i++ ) {
            rowsPtr[i] = &pixels[4 * i * width];
        }

        png_set_rows( writePtr, infoPtr, &rowsPtr[0] );
        png_set_write_fn(
            writePtr, &sf,
            []( png_structp png_ptr, png_bytep data, png_size_t length ) {
                static_cast<StreamFile *>( png_get_io_ptr( png_ptr ) )->putRaw( reinterpret_cast<const char *>( data ), length );
            },
            nullptr );
        png_write_png( writePtr, infoPtr, PNG_TRANSFORM_IDENTITY, nullptr );

        sf.close();
        return true;
    }
#endif

#if defined( ENABLE_PNG )
    bool SaveImage( const fheroes2::Image & image, const std::string & path )
#else
    bool SaveImage( const fheroes2::Image & image, std::string path )
#endif
    {
#if defined( ENABLE_PNG )
        return (isPNGFilePath( path ) ? SavePNG : SaveBMP)( image, path );
#else
        if ( isPNGFilePath( path ) ) {
            std::memcpy( path.data() + path.size() - 3, "bmp", 3 );
        }
        return SaveBMP( image, path );
#endif
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
