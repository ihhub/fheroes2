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

#include "objswmp.h"

#include <algorithm>
#include <bitset>
#include <vector>

#include "tools.h"

namespace
{
    const std::bitset<256> objSwmpShadowBitset
        = fheroes2::makeBitsetFromVector<256>( { 2,  3,   14,  15,  16,  17,  18,  19,  20,  21,  31,  43,  44,  45,  46,  47,  48,  49, 66,
                                                 83, 125, 127, 130, 132, 136, 141, 163, 170, 175, 178, 195, 197, 202, 204, 207, 211, 215 } );
}

bool ObjSwmp::isShadow( const uint8_t index )
{
    return objSwmpShadowBitset[index];
}
