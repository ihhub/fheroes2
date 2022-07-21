/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

#include "icn_decode.h"
#include "image.h"

namespace fheroes2
{
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
}
