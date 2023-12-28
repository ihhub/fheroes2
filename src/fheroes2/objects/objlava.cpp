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
#include "objlava.h"
#include "tools.h"

namespace
{
    const std::bitset<256> objLavaShadowBitset = fheroes2::makeBitsetFromVector<256>( { 10, 11, 45, 49, 79, 80, 81, 82, 109, 113, 116 } );

    const std::bitset<256> objLav2ShadowBitset
        = fheroes2::makeBitsetFromVector<256>( { 7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 29, 34, 38, 39, 43, 44, 45, 46,
                                                 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 72, 77, 78 } );

    const std::bitset<256> objLav3ShadowBitset = fheroes2::makeBitsetFromVector<256>(
        { 1,   2,   3,   4,   16,  17,  18,  19,  31,  32,  33,  34,  38,  46,  47,  48,  49,  50,  57,  58,  59,  61,  62,  63,  64,  76,  77,
          91,  92,  93,  106, 107, 108, 109, 110, 111, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
          134, 136, 137, 138, 139, 142, 143, 144, 145, 146, 147, 148, 149, 166, 167, 168, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186,
          187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
          214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 243 } );
}

bool ObjLav2::isShadow( const uint8_t index )
{
    return objLav2ShadowBitset[index];
}

bool ObjLav3::isShadow( const uint8_t index )
{
    return objLav3ShadowBitset[index];
}

bool ObjLava::isShadow( const uint8_t index )
{
    return objLavaShadowBitset[index];
}
