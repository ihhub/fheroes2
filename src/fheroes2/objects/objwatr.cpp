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
#include "objwatr.h"

int ObjWat2::GetPassable( u32 index )
{
    const u8 disabled[] = {11, 12, 19, 22};
    const u8 restricted[] = {2, 20};

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( 10 == index )
        return Direction::CENTER | Direction::TOP | Direction::LEFT | Direction::TOP_LEFT;
    else if ( 22 == index )
        return DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::BOTTOM_LEFT;
    else if ( isAction( index ) || ARRAY_COUNT_END( disabled ) != std::find( disabled, ARRAY_COUNT_END( disabled ), index ) )
        return 0;

    return ARRAY_COUNT_END( restricted ) != std::find( restricted, ARRAY_COUNT_END( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

int ObjWatr::GetPassable( u32 index )
{
    const u8 disabled[] = {11, 12, 19, 22};
    const u8 restricted[] = {69, 182, 183, 185, 186, 187, 248};

    if ( isShadow( index ) )
        return DIRECTION_ALL;
    else if ( isAction( index ) || ARRAY_COUNT_END( disabled ) != std::find( disabled, ARRAY_COUNT_END( disabled ), index ) )
        return 0;

    return ARRAY_COUNT_END( restricted ) != std::find( restricted, ARRAY_COUNT_END( restricted ), index ) ? DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjWat2::isAction( u32 index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjWatr::isAction( u32 index )
{
    return MP2::OBJ_ZERO != GetActionObject( index );
}

bool ObjWatr::isShadow( u32 index )
{
    const u8 shadows[] = {12, 38, 52, 55, 118, 166, 188, 240};
    return ARRAY_COUNT_END( shadows ) != std::find( shadows, ARRAY_COUNT_END( shadows ), index );
}

bool ObjWat2::isShadow( u32 index )
{
    return index == 1;
}

int ObjWatr::GetActionObject( u32 index )
{
    switch ( index ) {
    case 62:
        return MP2::OBJ_MAGELLANMAPS;
    case 195:
        return MP2::OBJ_BUOY;
    case 202:
    case 206:
    case 210:
    case 214:
    case 218:
    case 222:
        return MP2::OBJ_WHIRLPOOL;
    case 241:
        return MP2::OBJ_SHIPWRECK;
    default:
        break;
    }

    return MP2::OBJ_ZERO;
}

int ObjWat2::GetActionObject( u32 index )
{
    switch ( index ) {
    case 21:
        return MP2::OBJ_DERELICTSHIP;
    default:
        break;
    }

    return MP2::OBJ_ZERO;
}
