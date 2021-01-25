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

#ifndef H2DECODE_H
#define H2DECODE_H

#include <utility>
#include <vector>

#include "types.h"

namespace fheroes2
{
    class Image;
    class Sprite;

    namespace AGG
    {
        struct ICNHeader
        {
            ICNHeader()
                : offsetX( 0 )
                , offsetY( 0 )
                , width( 0 )
                , height( 0 )
                , animationFrames( 0 )
                , offsetData( 0 )
            {}

            u16 offsetX;
            u16 offsetY;
            u16 width;
            u16 height;
            u8 animationFrames; // used for adventure map animations, this can replace ICN::AnimationFrame
            u32 offsetData;
        };

        void DecodeICNSprite( Sprite & sprite, const ICNHeader & header1, const uint8_t * data, uint32_t sizeData );
    }
}

#endif
