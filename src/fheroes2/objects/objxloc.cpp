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
#include "objxloc.h"
int ObjXlc1::GetPassable( u32 index )
{
    const u8 disabled[] = {40, 49, 50};
    const u8 restricted[] = {69, 71, 75, 76, 85, 103, 117, 119, 126, 128, 134, 136};

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || ARRAY_COUNT_END( disabled ) != std::find( disabled, ARRAY_COUNT_END( disabled ), index ) )
        return 0;

    return ARRAY_COUNT_END( restricted ) != std::find( restricted, ARRAY_COUNT_END( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjXlc1::isAction( u32 index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjXlc1::isShadow( u32 index )
{
    const u8 shadows[] = {1, 2, 59, 68, 72, 78, 79, 83, 84, 112, 116, 120, 124, 125, 129, 133};

    return ARRAY_COUNT_END( shadows ) != std::find( shadows, ARRAY_COUNT_END( shadows ), index );
}

int ObjXlc2::GetPassable( u32 index )
{
    const u8 restricted[] = {3, 8, 28, 46, 92, 102};

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || ( 110 < index && index < 136 ) )
        return 0;

    return ARRAY_COUNT_END( restricted ) != std::find( restricted, ARRAY_COUNT_END( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjXlc2::isAction( u32 index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjXlc2::isShadow( u32 index )
{
    const u8 shadows[] = {2, 10, 47, 83};
    return ARRAY_COUNT_END( shadows ) != std::find( shadows, ARRAY_COUNT_END( shadows ), index );
}

int ObjXlc3::GetPassable( u32 index )
{
    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) )
        return 0;

    return DIRECTION_ALL;
}

bool ObjXlc3::isAction( u32 index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjXlc3::isShadow( u32 index )
{
    const u8 shadows[] = {0, 9, 20, 29, 41, 59, 65, 71, 77, 83, 89, 95, 101, 108, 109, 112, 113, 116, 117, 120, 121, 124, 125, 128, 129, 132, 133, 136, 137};

    return ARRAY_COUNT_END( shadows ) != std::find( shadows, ARRAY_COUNT_END( shadows ), index );
}

int ObjXlc1::GetActionObject( u32 index )
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

    return MP2::OBJ_ZERO;
}

int ObjXlc2::GetActionObject( u32 index )
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

    return MP2::OBJ_ZERO;
}

int ObjXlc3::GetActionObject( u32 index )
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

    return MP2::OBJ_ZERO;
}
