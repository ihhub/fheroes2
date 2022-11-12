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
#include "objswmp.h"
#include "tools.h"

namespace
{
    const std::bitset<256> objSwmpShadowBitset
        = fheroes2::makeBitsetFromVector<256>( { 2,  3,   14,  15,  16,  17,  18,  19,  20,  21,  31,  43,  44,  45,  46,  47,  48,  49, 66,
                                                 83, 125, 127, 130, 132, 136, 141, 163, 170, 175, 178, 195, 197, 202, 204, 207, 211, 215 } );
}

int ObjSwmp::GetPassable( const uint8_t index )
{
    const uint8_t disabled[] = { 88, 89, 90, 91, 94, 95, 96, 97, 98, 108, 109, 110, 112, 113, 115, 116, 118, 119, 122, 123, 143, 144 };
    const uint8_t restricted[] = { 32,  33,  67,  74,  82,  85,  100, 101, 102, 103, 104, 105, 126, 128, 129, 131, 133, 134, 135, 137, 138, 139, 145, 146,
                                   147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 166, 167, 171, 172, 176, 177, 179, 180, 181, 182,
                                   183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 196, 198, 199, 200, 201, 203, 205, 208, 209, 212, 213 };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || std::end( disabled ) != std::find( disabled, std::end( disabled ), index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjSwmp::isAction( uint32_t index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjSwmp::isShadow( const uint8_t index )
{
    return objSwmpShadowBitset[index];
}

int ObjSwmp::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 22:
        return MP2::OBJ_WITCHSHUT;
    case 81:
        return MP2::OBJ_XANADU;
    case 84:
        return MP2::OBJ_FAERIERING;
    case 140:
        return MP2::OBJ_SIGN;
    case 216:
        return MP2::OBJ_OBELISK;
    default:
        break;
    }

    return MP2::OBJ_ZERO;
}
