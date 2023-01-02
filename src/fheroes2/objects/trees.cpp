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

#include <bitset>
#include <vector>

#include "direction.h"
#include "mp2.h"
#include "tools.h"
#include "trees.h"

namespace
{
    const std::bitset<256> objTreeShadowBitset = fheroes2::makeBitsetFromVector<256>( { 0, 3, 7, 10, 13, 17, 20, 23, 26, 29, 32, 34 } );
}

int ObjTree::GetPassable( const uint8_t index )
{
    if ( isShadow( index ) )
        return DIRECTION_ALL;

    return ( 5 == index || 15 == index || 22 == index || 27 == index ? 0 : DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW );
}

bool ObjTree::isAction( uint32_t index )
{
    return MP2::OBJ_NONE != GetActionObject( index );
}

bool ObjTree::isShadow( const uint8_t index )
{
    return objTreeShadowBitset[index];
}

int ObjTree::GetActionObject( uint32_t /* unused */ )
{
    return MP2::OBJ_NONE;
}
