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

#include "mounts.h"

#include <algorithm>
#include <bitset>
#include <vector>

#include "tools.h"

namespace
{
    const std::bitset<256> objMnts1ShadowBitset
        = fheroes2::makeBitsetFromVector<256>( { 0, 5, 11, 17, 21, 26, 32, 38, 42, 45, 49, 52, 55, 59, 62, 65, 68, 71, 74, 75, 79, 80 } );
    const std::bitset<256> objMnts2ShadowBitset = fheroes2::makeBitsetFromVector<256>(
        { 0, 5, 11, 17, 21, 26, 32, 38, 42, 46, 47, 53, 57, 58, 62, 68, 72, 75, 79, 82, 85, 89, 92, 95, 98, 101, 104, 105, 109, 110 } );
}
/*
    Mnts1: MTNDSRT, MTNGRAS, MTNLAVA, MTNMULT, MTNSNOW, MTNSWMP
    Mnts2: MTNCRCK, MTNDIRT
*/

bool ObjMnts1::isShadow( const uint8_t index )
{
    return objMnts1ShadowBitset[index];
}

bool ObjMnts2::isShadow( const uint8_t index )
{
    return objMnts2ShadowBitset[index];
}
