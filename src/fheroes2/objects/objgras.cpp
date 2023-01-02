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
#include "objgras.h"
#include "tools.h"

namespace
{
    const std::bitset<256> objGrasShadowBitset = fheroes2::makeBitsetFromVector<256>(
        { 0, 4, 29, 32, 36, 39, 42, 44, 46, 48, 50, 76, 79, 82, 88, 92, 94, 98, 102, 105, 108, 111, 113, 120, 124, 128, 134, 138, 141, 143, 145, 147 } );

    const std::bitset<256> objGra2ShadowBitset = fheroes2::makeBitsetFromVector<256>(
        { 5,  14, 19, 20, 28, 31, 32, 33, 34, 35,  36,  37,  38,  47,  48,  49,  50,  51,  52,  53,  54,  70,  71,  72,  73,  74, 75,
          76, 77, 78, 79, 80, 81, 82, 83, 91, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 121, 124, 128 } );
}

int ObjGras::GetPassable( const uint8_t index )
{
    const uint8_t disabled[] = { 54, 55, 56, 57, 58, 65, 66, 67, 68 };
    const uint8_t restricted[] = {
        5,   7,   31,  33,  34,  37,  38,  40,  41,  43,  45,  47,  49,  59,  60,  61,  62,  63,  69,  70,  71,  72,  73,
        74,  75,  77,  78,  83,  84,  85,  89,  90,  93,  95,  96,  97,  99,  100, 101, 103, 104, 106, 107, 109, 110, 112,
        114, 115, 116, 121, 122, 123, 125, 126, 127, 129, 130, 131, 135, 136, 139, 140, 142, 144, 146, 148, 149, 150,
    };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || std::end( disabled ) != std::find( disabled, std::end( disabled ), index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjGras::isAction( uint32_t index )
{
    return MP2::OBJ_NONE != GetActionObject( index );
}

bool ObjGras::isShadow( const uint8_t index )
{
    return objGrasShadowBitset[index];
}

int ObjGra2::GetPassable( const uint8_t index )
{
    const uint8_t restricted[] = { 2, 3, 6, 8, 22, 59 };
    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjGra2::isAction( uint32_t index )
{
    return MP2::OBJ_NONE != GetActionObject( index );
}

bool ObjGra2::isShadow( const uint8_t index )
{
    return objGra2ShadowBitset[index];
}

int ObjGras::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 6:
        return MP2::OBJ_ABANDONED_MINE;
    case 30:
        return MP2::OBJ_FAERIE_RING;
    default:
        break;
    }

    return MP2::OBJ_NONE;
}

int ObjGra2::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 4:
        return MP2::OBJ_HILL_FORT;
    case 7:
        return MP2::OBJ_HALFLING_HOLE;
    case 21:
        return MP2::OBJ_TREE_CITY;
    case 55:
        return MP2::OBJ_WINDMILL;
    case 84:
        return MP2::OBJ_ARCHER_HOUSE;
    case 92:
        return MP2::OBJ_GOBLIN_HUT;
    case 114:
        return MP2::OBJ_DWARF_COTTAGE;
    case 125:
    case 126:
        return MP2::OBJ_ORACLE;
    case 129:
        return MP2::OBJ_OBELISK;
    default:
        break;
    }

    return MP2::OBJ_NONE;
}
