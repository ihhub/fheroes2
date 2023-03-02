/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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
#include "objcrck.h"
#include "tools.h"

namespace
{
    const std::bitset<256> objCrckShadowBitset
        = fheroes2::makeBitsetFromVector<256>( { 2, 9, 13, 15, 20, 23, 28, 33, 36, 39, 45, 48, 51, 54, 56, 73, 75, 79, 200, 201, 207, 237 } );
}

int ObjCrck::GetPassable( const uint8_t index )
{
    const uint8_t disabled[] = { 58, 59, 63, 64, 65, 76, 77, 78, 80, 91, 102, 113, 124, 135, 182, 183, 185, 221, 222, 223, 227, 228, 229, 230, 238, 241, 242, 245 };
    const uint8_t restricted[] = { 5,  6,  10, 11, 14, 16, 17, 18,  21,  22,  24,  25,  29,  30,  31,  32,  34,  35,  37,  38,  40,  41,  42,  43,  46,  49,  52, 55,
                                   57, 62, 67, 68, 69, 71, 72, 136, 148, 159, 170, 181, 186, 187, 188, 202, 224, 225, 226, 231, 232, 233, 234, 235, 243, 244, 246 };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( 184 == index )
        return Direction::CENTER | Direction::BOTTOM_RIGHT | DIRECTION_TOP_ROW;
    else if ( isAction( index ) || std::end( disabled ) != std::find( disabled, std::end( disabled ), index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjCrck::isAction( uint32_t index )
{
    return MP2::OBJ_NONE != GetActionObject( index );
}

bool ObjCrck::isShadow( const uint8_t index )
{
    return objCrckShadowBitset[index];
}

int ObjCrck::GetActionObject( uint32_t index )
{
    /*
    artesian spring: 3, 4
    wagon: 74
    troll bridge: 189
    market: 213
    watering hole: 217, 218, 219, 220
    obelisk: 238
    saw mill: 245
    */

    switch ( index ) {
    case 3:
    case 4:
        return MP2::OBJ_ARTESIAN_SPRING;
    case 74:
        return MP2::OBJ_WAGON;
    case 189:
        return MP2::OBJ_TROLL_BRIDGE;
    case 213:
        return MP2::OBJ_TRADING_POST;
    case 217:
    case 218:
    case 219:
    case 220:
        return MP2::OBJ_WATERING_HOLE;
    case 238:
        return MP2::OBJ_OBELISK;
    case 245:
        return MP2::OBJ_SAWMILL;
    default:
        break;
    }

    return MP2::OBJ_NONE;
}
