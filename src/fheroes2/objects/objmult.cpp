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
#include "objmult.h"
#include "tools.h"

namespace
{
    const std::bitset<256> objMultShadowBitset = fheroes2::makeBitsetFromVector<256>(
        { 1,  3,  15, 16, 17, 18, 19, 20, 21, 22, 23, 24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54, 57,
          61, 67, 68, 75, 77, 79, 81, 83, 97, 98, 99, 100, 101, 102, 103, 105, 106, 107, 108, 109, 110, 113, 115, 121, 122, 124, 125, 126, 127, 128, 129, 130 } );

    const std::bitset<256> objMul2ShadowBitset = fheroes2::makeBitsetFromVector<256>(
        { 14,  17,  20,  24,  34,  36,  42,  43,  49,  50,  60,  71,  72,  113, 115, 118, 121, 123, 127, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146,
          147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 164, 180, 181, 182, 183, 184, 185, 186, 189, 199, 200, 202, 206 } );
}

int ObjMult::GetPassable( const uint8_t index )
{
    const uint8_t restricted[] = { 2, 4, 58, 63, 64, 65, 70, 72, 73, 89, 104 };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjMult::isAction( uint32_t index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjMult::isShadow( const uint8_t index )
{
    return objMultShadowBitset[index];
}

int ObjMul2::GetPassable( const uint8_t index )
{
    const uint8_t disabled[] = { 46, 76, 77, 124, 125, 126, 221, 213 };
    const uint8_t restricted[] = { 16, 18, 19, 25, 27, 51, 52, 53, 55, 57, 78, 79, 81, 98, 105, 128, 136, 187, 207, 209, 214, 215, 217 };

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || std::end( disabled ) != std::find( disabled, std::end( disabled ), index ) )
        return 0;

    return std::end( restricted ) != std::find( restricted, std::end( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjMul2::isAction( uint32_t index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjMul2::isShadow( const uint8_t index )
{
    return objMul2ShadowBitset[index];
}

int ObjMul2::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 15:
        return MP2::OBJ_FOUNTAIN;
    case 26:
        return MP2::OBJ_ALCHEMYTOWER;
    case 54:
        return MP2::OBJ_DRAGONCITY;
    case 58:
        return MP2::OBJ_GRAVEYARD;
    case 73:
        return MP2::OBJ_LIGHTHOUSE;
    case 80:
        return MP2::OBJ_SAWMILL;
    case 112:
        return MP2::OBJ_WATERWHEEL;
    case 114:
        return MP2::OBJ_SIGN;
    case 116:
    case 119:
    case 122:
        return MP2::OBJ_STONELITHS;
    case 129:
        return MP2::OBJ_WAGONCAMP;
    case 162:
    case 165:
        return MP2::OBJ_MAGICWELL;
    case 188:
        return MP2::OBJ_FREEMANFOUNDRY;
    case 190:
        return MP2::OBJ_MAGICGARDEN;
    case 201:
        return MP2::OBJ_OBSERVATIONTOWER;
    case 208:
        return MP2::OBJ_GRAVEYARD;
    case 216:
        return MP2::OBJ_SAWMILL;
    default:
        break;
    }

    return MP2::OBJ_ZERO;
}

int ObjMult::GetActionObject( uint32_t index )
{
    switch ( index ) {
    case 35:
        return MP2::OBJ_PEASANTHUT;
    case 59:
        return MP2::OBJ_FORT;
    case 62:
        return MP2::OBJ_GAZEBO;
    case 69:
        return MP2::OBJ_WITCHSHUT;
    case 71:
        return MP2::OBJ_MERCENARYCAMP;
    case 74:
        return MP2::OBJ_RUINS;
    case 76:
        return MP2::OBJ_SHRINE1;
    case 78:
        return MP2::OBJ_SHRINE2;
    case 80:
        return MP2::OBJ_SHRINE3;
    case 82:
        return MP2::OBJ_IDOL;
    case 84:
    case 85:
        return MP2::OBJ_STANDINGSTONES;
    case 88:
        return MP2::OBJ_TEMPLE;
    case 111:
        return MP2::OBJ_TRADINGPOST;
    case 114:
        return MP2::OBJ_TREEHOUSE;
    case 116:
        return MP2::OBJ_WATCHTOWER;
    case 123:
        return MP2::OBJ_TREEKNOWLEDGE;
    default:
        break;
    }

    return MP2::OBJ_ZERO;
}
