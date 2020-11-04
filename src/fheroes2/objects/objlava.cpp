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
#include "objlava.h"

int ObjLav2::GetPassable( u32 index )
{
    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) )
        return 0;

    return DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW;
}

bool ObjLav2::isAction( u32 index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjLav2::isShadow( u32 index )
{
    const u8 shadows[] = {0, 7, 14, 29, 33, 44, 55, 78};
    return ARRAY_COUNT_END( shadows ) != std::find( shadows, ARRAY_COUNT_END( shadows ), index );
}

int ObjLav3::GetPassable( u32 index )
{
    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) )
        return 0;

    return DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW;
}

bool ObjLav3::isAction( u32 index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjLav3::isShadow( u32 index )
{
    const u8 shadows[] = {0, 15, 30, 45, 60, 75, 90, 105, 120, 135, 165, 180, 195, 210, 225, 243};
    return ARRAY_COUNT_END( shadows ) != std::find( shadows, ARRAY_COUNT_END( shadows ), index );
}

int ObjLava::GetPassable( u32 index )
{
    const u8 disabled[] = {2, 3, 4, 5, 12, 13, 14, 15, 18, 27, 28, 29, 30, 31, 32, 39, 40, 41, 46, 47, 48, 53, 54, 57, 60, 61, 64, 65, 69, 70, 120, 121};

    const u8 restricted[]
        = {6, 7, 8, 9, 16, 17, 19, 20, 33, 34, 35, 36, 37, 38, 42, 43, 44, 50, 51, 52, 55, 56, 58, 59, 62, 66, 67, 68, 72, 73, 76, 77, 88, 98, 114, 122, 123, 125};

    if ( isAction( index ) || ARRAY_COUNT_END( disabled ) != std::find( disabled, ARRAY_COUNT_END( disabled ), index ) )
        return 0;

    return ARRAY_COUNT_END( restricted ) != std::find( restricted, ARRAY_COUNT_END( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjLava::isAction( u32 index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjLava::isShadow( u32 index )
{
    const u8 shadows[] = {10, 11, 45, 49, 77, 109, 113, 116};
    return ARRAY_COUNT_END( shadows ) != std::find( shadows, ARRAY_COUNT_END( shadows ), index );
}

int ObjLav2::GetActionObject( u32 )
{
    return MP2::OBJ_ZERO;
}

int ObjLav3::GetActionObject( u32 )
{
    return MP2::OBJ_ZERO;
}

int ObjLava::GetActionObject( u32 index )
{
    switch ( index ) {
    case 110:
        return MP2::OBJ_OBELISK;
    case 115:
        return MP2::OBJ_DAEMONCAVE;
    case 117:
        return MP2::OBJ_SIGN;
    case 124:
        return MP2::OBJ_SAWMILL;
    default:
        break;
    }

    return MP2::OBJ_ZERO;
}
