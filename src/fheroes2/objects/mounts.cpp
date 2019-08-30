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
#include "mp2.h"
#include "icn.h"
#include "direction.h"
#include "mounts.h"

/*
    Mnts1: MTNDSRT, MTNGRAS, MTNLAVA, MTNMULT, MTNSNOW, MTNSWMP
    Mnts2: MTNCRCK, MTNDIRT
*/

int ObjMnts1::GetPassable(int icn, u32 index)
{
    const u8 disabled2[] = { 6, 7, 8, 9, 14, 15, 16, 28, 29, 30, 31, 33, 34, 35, 47, 48, 56, 57, 64, 67, 68, 69, 82 };

    if(isShadow(index)) return DIRECTION_ALL;
    else
    // fix: disable passable: invalid top sprite
    if(icn == ICN::MTNGRAS &&
        (25 == index || 43 == index || 44 == index || 53 == index || 54 == index || 78 == index))
        return 0;

    return ARRAY_COUNT_END(disabled2) != std::find(disabled2, ARRAY_COUNT_END(disabled2), index) ?
	0 : DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW;
}

int ObjMnts2::GetPassable(int icn, u32 index)
{
    const u8 disabled1[] = { 6, 7, 8, 9, 14, 15, 16, 28, 29, 30, 31, 33, 34, 35, 50, 51, 52, 65, 77, 78, 87, 94, 99, 112 };

    if(isShadow(index)) return DIRECTION_ALL;
    // fix: disable passable: invalid top sprite
    if(icn == ICN::MTNDIRT &&
	(73 == index || 84 == index || 86 == index))
        return 0;

    return ARRAY_COUNT_END(disabled1) != std::find(disabled1, ARRAY_COUNT_END(disabled1), index) ?
	0 : DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW;
}

bool ObjMnts1::isAction(u32 index)
{
    return MP2::OBJ_ZERO != GetActionObject(index);
}

bool ObjMnts2::isAction(u32 index)
{
    return MP2::OBJ_ZERO != GetActionObject(index);
}

int ObjMnts1::GetActionObject(u32 index)
{
    return MP2::OBJ_ZERO;
}

int ObjMnts2::GetActionObject(u32 index)
{
    return MP2::OBJ_ZERO;
}

bool ObjMnts1::isShadow(u32 index)
{
    const u8 shadows1[] = { 0, 5, 11, 17, 21, 26, 32, 38, 42, 45, 49, 52, 55, 59, 62, 65, 68, 71, 74, 75, 79, 80 };
    return ARRAY_COUNT_END(shadows1) != std::find(shadows1, ARRAY_COUNT_END(shadows1), index);
}

bool ObjMnts2::isShadow(u32 index)
{
    const u8 shadows2[] = { 0, 5, 11, 17, 21, 26, 32, 38, 42, 46, 47, 53, 57, 58, 62,
			    68, 72, 75, 79, 82, 85, 89, 92, 95, 98, 101, 104, 105, 109, 110 };
    return ARRAY_COUNT_END(shadows2) != std::find(shadows2, ARRAY_COUNT_END(shadows2), index);
}
