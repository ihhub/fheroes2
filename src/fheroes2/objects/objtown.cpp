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

#include "objtown.h"

#include <bitset>
#include <vector>

#include "direction.h"
#include "mp2.h"
#include "tools.h"

namespace
{
    const std::bitset<256> obTownShadowBitset = fheroes2::makeBitsetFromVector<256>( { 0, 16, 17, 48, 80, 81, 112, 144, 145, 161, 165, 176 } );
}

int ObjTown::GetPassable( const uint8_t index0 )
{
    uint32_t index = index0 % 32;

    // 13, 29, 45, 61, 77, 93, 109, 125, 141, 157, 173, 189
    if ( 13 == index || 29 == index )
        return Direction::CENTER | Direction::BOTTOM;
    else
        // town/castle
        if ( ( 5 < index && index < 13 ) || ( 13 < index && index < 16 ) || ( 21 < index && index < 29 ) || ( 29 < index ) )
        return 0;

    return DIRECTION_ALL;
}

int ObjTwba::GetPassable( const uint8_t index0 )
{
    uint32_t index = index0 % 10;

    // 2, 12, 22, 32, 42, 52, 62, 72
    if ( index == 2 ) {
        return Direction::CENTER | Direction::BOTTOM;
    }
    else if ( index < 5 ) {
        return 0;
    }
    else {
        // 7, 17, 27, 37, 47, 57, 67, 77
        if ( index == 7 )
            return DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW | Direction::TOP;
        else
            return DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW;
    }
}

bool ObjTown::isAction( uint32_t index )
{
    return MP2::OBJ_NONE != GetActionObject( index );
}

bool ObjTwba::isAction( uint32_t index )
{
    return MP2::OBJ_NONE != GetActionObject( index );
}

bool ObjTown::isShadow( const uint8_t index )
{
    return obTownShadowBitset[index];
}

bool ObjTwba::isShadow( const uint8_t /*index*/ )
{
    return false;
}

int ObjTown::GetActionObject( uint32_t index )
{
    switch ( index % 32 ) {
    case 13:
    case 29:
        return MP2::OBJ_CASTLE;
    default:
        break;
    }

    return MP2::OBJ_NONE;
}

int ObjTwba::GetActionObject( uint32_t /* unused */ )
{
    return MP2::OBJ_NONE;
}
