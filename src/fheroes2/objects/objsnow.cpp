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
#include "objsnow.h"
#include "tools.h"

namespace
{
    const std::bitset<256> objSnowShadowBitset
        = fheroes2::makeBitsetFromVector<256>( { 21,  25,  29,  31,  33,  36,  40,  48,  54,  59,  63,  67,  70,  73,  76,  79,  101, 104, 105, 106, 107,
                                                 108, 109, 110, 111, 120, 121, 122, 123, 124, 125, 126, 127, 137, 140, 142, 144, 148, 193, 203, 207 } );
}

int ObjSnow::GetPassable( const uint8_t index )
{
    const uint8_t disabled[] = { 22, 26, 27, 28, 30, 32, 34, 35, 37, 38, 39, 81, 82, 83, 84, 197, 198 };
    const uint8_t restricted[] = { 2,  12, 41, 42, 43, 44, 45, 49, 50, 55,  56,  57,  60,  64,  65,  68,  71,  74,  77,  80, 85,
                                   86, 87, 88, 89, 90, 91, 92, 94, 95, 132, 149, 151, 159, 177, 184, 199, 200, 202, 208, 210 };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || std::end( disabled ) != std::find( disabled, std::end( disabled ), index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjSnow::isAction( uint32_t index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjSnow::isShadow( const uint8_t index )
{
    return objSnowShadowBitset[index];
}

int ObjSnow::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 3:
        return MP2::OBJ_CAVE;
    case 13:
        return MP2::OBJ_LEANTO;
    case 128:
        return MP2::OBJ_WINDMILL;
    case 138:
        return MP2::OBJ_WATCHTOWER;
    case 141:
        return MP2::OBJ_OBELISK;
    case 143:
        return MP2::OBJ_SIGN;
    case 150:
        return MP2::OBJ_ALCHEMYTOWER;
    case 160:
        return MP2::OBJ_GRAVEYARD;
    case 191:
        return MP2::OBJ_WATERWHEEL;
    case 194:
        return MP2::OBJ_MAGICWELL;
    case 201:
        return MP2::OBJ_SAWMILL;
    case 209:
        return MP2::OBJ_GRAVEYARD;
    default:
        break;
    }

    return MP2::OBJ_ZERO;
}
