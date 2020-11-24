/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include "direction.h"
#include "icn.h"
#include "mp2.h"
#include "objmult.h"

int ObjMult::GetPassable( u32 index )
{
    const u8 restricted[] = {2, 4, 58, 63, 64, 65, 70, 72, 73, 89, 104};

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) )
        return 0;

    return ARRAY_COUNT_END( restricted ) != std::find( restricted, ARRAY_COUNT_END( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjMult::isAction( u32 index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjMult::isShadow( u32 index )
{
    const u8 shadows2[] = {1, 3, 15, 25, 45, 54, 57, 61, 67, 68, 75, 77, 79, 81, 83, 97, 98, 105, 113, 115, 121, 122, 124};

    return ARRAY_COUNT_END( shadows2 ) != std::find( shadows2, ARRAY_COUNT_END( shadows2 ), index );
}

int ObjMul2::GetPassable( u32 index )
{
    const u8 disabled[] = {46, 76, 77, 124, 125, 126, 221, 213};
    const u8 restricted[] = {16, 18, 19, 25, 27, 51, 52, 53, 55, 57, 78, 79, 81, 98, 105, 128, 136, 187, 207, 209, 214, 215, 217};

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || ARRAY_COUNT_END( disabled ) != std::find( disabled, ARRAY_COUNT_END( disabled ), index ) )
        return 0;

    return ARRAY_COUNT_END( restricted ) != std::find( restricted, ARRAY_COUNT_END( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjMul2::isAction( u32 index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjMul2::isShadow( u32 index )
{
    const u8 shadows1[] = {14, 17, 20, 24, 34, 36, 42, 43, 49, 50, 60, 71, 72, 113, 115, 118, 121, 123, 127, 161, 164, 180, 181, 189, 199, 200, 202, 206};

    return ARRAY_COUNT_END( shadows1 ) != std::find( shadows1, ARRAY_COUNT_END( shadows1 ), index );
}

int ObjMul2::GetActionObject( u32 index )
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

int ObjMult::GetActionObject( u32 index )
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
