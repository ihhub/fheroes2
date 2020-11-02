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
#include "objdsrt.h"

int ObjDsrt::GetPassable( u32 index )
{
    const u8 disabled[] = {61, 89, 90, 91, 92, 93, 125, 126};
    const u8 restricted[] = {3,  6,  9,  12, 14, 15, 17, 18, 20, 21, 22, 24, 26, 28,  30,  31,  32,  34,  36,  39,  40,  42, 45,
                             48, 49, 51, 53, 72, 76, 81, 83, 94, 95, 97, 98, 99, 100, 110, 111, 112, 116, 121, 127, 128, 130};

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || ARRAY_COUNT_END( disabled ) != std::find( disabled, ARRAY_COUNT_END( disabled ), index ) )
        return 0;

    return ARRAY_COUNT_END( restricted ) != std::find( restricted, ARRAY_COUNT_END( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjDsrt::isAction( u32 index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjDsrt::isShadow( u32 index )
{
    const u8 shadows[] = {11, 13, 16, 19, 23, 25, 27, 29, 33, 35, 38, 41, 44, 46, 47, 50, 52, 54, 71, 75, 77, 80, 86, 103, 115, 118};
    return ARRAY_COUNT_END( shadows ) != std::find( shadows, ARRAY_COUNT_END( shadows ), index );
}

int ObjDsrt::GetActionObject( u32 index )
{
    switch ( index ) {
    case 73:
        return MP2::OBJ_DESERTTENT;
    case 82:
        return MP2::OBJ_PYRAMID;
    case 84:
        return MP2::OBJ_SKELETON;
    case 87:
        return MP2::OBJ_SPHINX;
    case 96:
        return MP2::OBJ_CITYDEAD;
    case 101:
        return MP2::OBJ_EXCAVATION;
    case 104:
        return MP2::OBJ_OBELISK;
    case 108:
    case 109:
        return MP2::OBJ_OASIS;
    case 117:
        return MP2::OBJ_DAEMONCAVE;
    case 119:
        return MP2::OBJ_SIGN;
    case 122:
        return MP2::OBJ_GRAVEYARD;
    case 129:
        return MP2::OBJ_SAWMILL;
    default:
        break;
    }

    return MP2::OBJ_ZERO;
}
