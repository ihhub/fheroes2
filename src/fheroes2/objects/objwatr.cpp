/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <bitset>
#include <iterator>
#include <vector>

#include "direction.h"
#include "mp2.h"
#include "objwatr.h"
#include "tools.h"

namespace
{
    const std::bitset<256> objWatrShadowBitset
        = fheroes2::makeBitsetFromVector<256>( { 12,  13,  14,  15,  16,  17,  18,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,
                                                 42,  43,  44,  52,  55,  118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 166, 167,
                                                 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 184, 188, 189, 190, 191, 192, 193, 194, 240 } );
}

int ObjWat2::GetPassable( const uint8_t index )
{
    const uint8_t disabled[] = { 11, 12, 19, 22 };
    const uint8_t restricted[] = { 2, 20 };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( 10 == index )
        return Direction::CENTER | Direction::TOP | Direction::LEFT | Direction::TOP_LEFT;
    else if ( 22 == index )
        return DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::BOTTOM_LEFT;
    else if ( isAction( index ) || std::end( disabled ) != std::find( disabled, std::end( disabled ), index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

int ObjWatr::GetPassable( const uint8_t index )
{
    const uint8_t disabled[] = { 11, 12, 19, 22 };
    const uint8_t restricted[] = { 69, 182, 183, 185, 186, 187, 248 };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || std::end( disabled ) != std::find( disabled, std::end( disabled ), index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjWat2::isAction( uint32_t index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjWatr::isAction( uint32_t index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjWatr::isShadow( const uint8_t index )
{
    return objWatrShadowBitset[index];
}

bool ObjWat2::isShadow( const uint8_t index )
{
    return index == 1;
}

int ObjWatr::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 62:
        return MP2::OBJ_MAGELLANMAPS;
    case 195:
        return MP2::OBJ_BUOY;
    case 202:
    case 206:
    case 210:
    case 214:
    case 218:
    case 222:
        return MP2::OBJ_WHIRLPOOL;
    case 241:
        return MP2::OBJ_SHIPWRECK;
    default:
        break;
    }

    return MP2::OBJ_ZERO;
}

int ObjWat2::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 21:
        return MP2::OBJ_DERELICTSHIP;
    default:
        break;
    }

    return MP2::OBJ_ZERO;
}
