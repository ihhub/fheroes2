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
#include "objxloc.h"
#include "tools.h"

namespace
{
    const std::bitset<256> objXlc1ShadowBitset = fheroes2::makeBitsetFromVector<256>(
        { 1, 2, 32, 33, 34, 35, 36, 37, 38, 39, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 72, 78, 79, 83, 84, 112, 116, 120, 124, 125, 129, 133 } );

    const std::bitset<256> objXlc2ShadowBitset
        = fheroes2::makeBitsetFromVector<256>( { 2, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 47, 48, 49, 50, 51, 52, 53, 54, 55, 83, 84, 85, 86, 87, 88, 89, 90, 91 } );

    const std::bitset<256> objXlc3ShadowBitset = fheroes2::makeBitsetFromVector<256>(
        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,   20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  41,  42,  43,  44,  45,  46, 47,
          48, 49, 59, 65, 71, 77, 83, 89, 95, 101, 108, 109, 112, 113, 116, 117, 120, 121, 124, 125, 128, 129, 132, 133, 136, 137 } );
}

bool ObjXlc1::isShadow( const uint8_t index )
{
    return objXlc1ShadowBitset[index];
}

bool ObjXlc2::isShadow( const uint8_t index )
{
    return objXlc2ShadowBitset[index];
}

bool ObjXlc3::isShadow( const uint8_t index )
{
    return objXlc3ShadowBitset[index];
}

bool ObjXlc2::isReefs( const uint8_t index )
{
    return index >= 111 && index <= 135;
}
