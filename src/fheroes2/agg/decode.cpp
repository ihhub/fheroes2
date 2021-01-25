/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "decode.h"
#include "agg_file.h"
#include "artifact.h"
#include "audio.h"
#include "audio_cdrom.h"
#include "audio_mixer.h"
#include "audio_music.h"
#include "dir.h"
#include "engine.h"
#include "error.h"
#include "font.h"
#include "game.h"
#include "m82.h"
#include "mus.h"
#include "pal.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "til.h"

#ifdef WITH_ZLIB
#include "embedded_image.h"
#include "zzlib.h"
#endif

StreamBase & operator>>( StreamBase & st, fheroes2::AGG::ICNHeader & icn )
{
    icn.offsetX = st.getLE16();
    icn.offsetY = st.getLE16();
    icn.width = st.getLE16();
    icn.height = st.getLE16();
    icn.animationFrames = st.get();
    icn.offsetData = st.getLE32();

    return st;
}

namespace fheroes2
{
    namespace AGG
    {
        void DecodeICNSprite( Sprite & sprite, const ICNHeader & header, const uint8_t * data, uint32_t sizeData )
        {
            sprite.resize( header.width, header.height );
            sprite.reset();
            sprite.setPosition( static_cast<int16_t>( header.offsetX ), static_cast<int16_t>( header.offsetY ) );

            uint8_t * imageData = sprite.image();
            uint8_t * imageTransform = sprite.transform();

            uint32_t posX = 0;
            const uint32_t width = sprite.width();

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
        }
    }
}
