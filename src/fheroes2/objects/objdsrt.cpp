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
#include "objdsrt.h"
#include "tools.h"

namespace
{
    const std::bitset<256> objDsrtShadowBitset = fheroes2::makeBitsetFromVector<256>(
        { 11, 13, 16, 19, 23, 25, 27, 29, 33, 35, 38, 41, 44, 46, 47, 50, 52, 54, 55, 56, 57, 58, 59, 60, 71, 75, 77, 80, 86, 103, 115, 118 } );
}

int ObjDsrt::GetPassable( const uint8_t index )
{
    const uint8_t disabled[] = { 61, 89, 90, 91, 92, 93, 125, 126 };
    const uint8_t restricted[] = { 3,  6,  9,  12, 14, 15, 17, 18, 20, 21, 22, 24, 26, 28,  30,  31,  32,  34,  36,  39,  40,  42, 45,
                                   48, 49, 51, 53, 72, 76, 81, 83, 94, 95, 97, 98, 99, 100, 110, 111, 112, 116, 121, 127, 128, 130 };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || std::end( disabled ) != std::find( disabled, std::end( disabled ), index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjDsrt::isAction( uint32_t index )
{
    return MP2::OBJ_NONE != GetActionObject( index );
}

bool ObjDsrt::isShadow( const uint8_t index )
{
    return objDsrtShadowBitset[index];
}

int ObjDsrt::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 73:
        return MP2::OBJ_DESERT_TENT;
    case 82:
        return MP2::OBJ_PYRAMID;
    case 84:
        return MP2::OBJ_SKELETON;
    case 87:
        return MP2::OBJ_SPHINX;
    case 96:
        return MP2::OBJ_CITY_OF_DEAD;
    case 101:
        return MP2::OBJ_EXCAVATION;
    case 104:
        return MP2::OBJ_OBELISK;
    case 108:
    case 109:
        return MP2::OBJ_OASIS;
    case 117:
        return MP2::OBJ_DAEMON_CAVE;
    case 119:
        return MP2::OBJ_SIGN;
    case 122:
        return MP2::OBJ_GRAVEYARD;
    case 129:
        return MP2::OBJ_SAWMILL;
    default:
        break;
    }

    return MP2::OBJ_NONE;
}
