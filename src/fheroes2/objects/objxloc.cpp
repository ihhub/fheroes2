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

int ObjXlc1::GetPassable( const uint8_t index )
{
    const uint8_t disabled[] = { 40, 49, 50 };
    const uint8_t restricted[] = { 69, 71, 75, 76, 85, 103, 117, 119, 126, 128, 134, 136 };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || std::end( disabled ) != std::find( disabled, std::end( disabled ), index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjXlc1::isAction( uint32_t index )
{
    return MP2::OBJ_NONE != GetActionObject( index );
}

bool ObjXlc1::isShadow( const uint8_t index )
{
    return objXlc1ShadowBitset[index];
}

int ObjXlc2::GetPassable( const uint8_t index )
{
    const uint8_t restricted[] = { 3, 8, 28, 46, 92, 102 };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || ( 110 < index && index < 136 ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjXlc2::isAction( uint32_t index )
{
    return MP2::OBJ_NONE != GetActionObject( index );
}

bool ObjXlc2::isShadow( const uint8_t index )
{
    return objXlc2ShadowBitset[index];
}

int ObjXlc3::GetPassable( const uint8_t index )
{
    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) )
        return 0;

    return DIRECTION_ALL;
}

bool ObjXlc3::isAction( uint32_t index )
{
    return MP2::OBJ_NONE != GetActionObject( index );
}

bool ObjXlc3::isShadow( const uint8_t index )
{
    return objXlc3ShadowBitset[index];
}

int ObjXlc1::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 3:
        return MP2::OBJ_ALCHEMYTOWER;
    case 70:
        return MP2::OBJ_ARENA;
    case 77:
        return MP2::OBJ_BARROWMOUNDS;
    case 94:
        return MP2::OBJ_EARTHALTAR;
    case 118:
        return MP2::OBJ_AIRALTAR;
    case 127:
        return MP2::OBJ_FIREALTAR;
    case 135:
        return MP2::OBJ_WATERALTAR;
    default:
        break;
    }

    return MP2::OBJ_NONE;
}

int ObjXlc2::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 4:
        return MP2::OBJ_STABLES;
    case 9:
        return MP2::OBJ_JAIL;
    case 37:
        return MP2::OBJ_MERMAID;
    case 101:
        return MP2::OBJ_SIRENS;
    default:
        break;
    }

    return MP2::OBJ_NONE;
}

bool ObjXlc2::isReefs( const uint8_t index )
{
    return index >= 111 && index <= 135;
}

int ObjXlc3::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 30:
        return MP2::OBJ_HUTMAGI;
    case 50:
        return MP2::OBJ_EYEMAGI;
    case 60:
    case 66:
    case 72:
    case 78:
    case 84:
    case 90:
    case 96:
    case 102:
        return MP2::OBJ_BARRIER;
    case 110:
    case 114:
    case 118:
    case 122:
    case 126:
    case 130:
    case 134:
    case 138:
        return MP2::OBJ_TRAVELLERTENT;
    default:
        break;
    }

    return MP2::OBJ_NONE;
}
