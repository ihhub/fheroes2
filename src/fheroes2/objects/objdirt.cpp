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
#include "objdirt.h"
#include "tools.h"

namespace
{
    const std::bitset<256> objDirtShadowBitset = fheroes2::makeBitsetFromVector<256>(
        { 0,   1,   5,   6,   14,  47,  52,  59,  62,  65,  68,  70,  72,  75,  78,  81,  84,  87,  91,  94,  97,  100, 103, 111, 114, 117,
          126, 128, 136, 149, 150, 158, 161, 162, 163, 164, 165, 166, 167, 168, 177, 178, 179, 180, 181, 182, 183, 184, 193, 196, 200 } );
}

int ObjDirt::GetPassable( const uint8_t index )
{
    const uint8_t disabled[] = { 23, 24, 25, 26, 27, 28, 29, 30, 32, 33, 34, 35, 36, 37, 48, 49, 50, 51 };
    const uint8_t restricted[]
        = { 7,  9,  12, 13, 15, 16, 17, 18, 19, 20, 21, 22, 40, 41, 42, 43, 44, 45,  53,  54,  55,  56,  57,  58,  60,  61,  63,  64,  66,  67,  69, 71,
            73, 74, 76, 77, 79, 80, 82, 83, 85, 86, 88, 89, 90, 92, 93, 98, 99, 101, 102, 104, 105, 118, 123, 127, 130, 133, 134, 137, 139, 152, 189 };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || std::end( disabled ) != std::find( disabled, std::end( disabled ), index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjDirt::isAction( uint32_t index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjDirt::isShadow( const uint8_t index )
{
    return objDirtShadowBitset[index];
}

int ObjDirt::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 8:
        return MP2::OBJ_ABANDONEDMINE;
    case 129:
        return MP2::OBJ_FAERIERING;
    case 135:
        return MP2::OBJ_HILLFORT;
    case 138:
        return MP2::OBJ_HALFLINGHOLE;
    case 151:
        return MP2::OBJ_TREECITY;
    case 185:
        return MP2::OBJ_WINDMILL;
    case 197:
    case 198:
        return MP2::OBJ_ORACLE;
    case 201:
        return MP2::OBJ_OBELISK;
    default:
        break;
    }

    return MP2::OBJ_ZERO;
}
