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
#include "objgras.h"

int ObjGras::GetPassable(u32 index)
{
    const u8 disabled[] = { 54, 55, 56, 57, 58, 65, 66, 67, 68 };
    const u8 restricted[] = { 5, 7, 31, 33, 34, 37, 38, 40, 41, 43, 45, 47, 49, 59,
	60, 61, 62, 63, 69, 70, 71, 72, 73, 74, 75, 77, 78, 83, 84, 85, 89, 90, 93, 95,
	96, 97, 99, 100, 101, 103, 104, 106, 107, 109, 110, 112, 114, 115, 116, 121, 122,
	123, 125, 126, 127, 129, 130, 131, 135, 136, 139, 140, 142, 144, 146, 148, 149, 150, };

    if(isShadow(index))
    	return DIRECTION_ALL;
    else
    if(isAction(index) ||
	ARRAY_COUNT_END(disabled) != std::find(disabled, ARRAY_COUNT_END(disabled), index))
        return 0;

    return ARRAY_COUNT_END(restricted) != std::find(restricted, ARRAY_COUNT_END(restricted), index) ?
            DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjGras::isAction(u32 index)
{
    return MP2::OBJ_ZERO != GetActionObject(index);
}

bool ObjGras::isShadow(u32 index)
{
    const u8 shadows2[] = { 0, 4, 29, 32, 36, 39, 42, 44, 46, 48, 50, 76, 79, 82, 88, 92, 94, 98, 102, 105,
		108, 111, 113, 120, 124, 128, 134, 138, 141, 143, 145, 147 };
    return ARRAY_COUNT_END(shadows2) != std::find(shadows2, ARRAY_COUNT_END(shadows2), index);
}

int ObjGra2::GetPassable(u32 index)
{
    const u8 restricted[] = { 2, 3, 6, 8, 22, 59 };
    if(isShadow(index))
    	return DIRECTION_ALL;
    else
    if(isAction(index))
        return 0;

    return ARRAY_COUNT_END(restricted) != std::find(restricted, ARRAY_COUNT_END(restricted), index) ?
            DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjGra2::isAction(u32 index)
{
    return MP2::OBJ_ZERO != GetActionObject(index);
}

bool ObjGra2::isShadow(u32 index)
{
    const u8 shadows1[] = { 5, 19, 20, 31, 33, 47, 51, 70, 77, 91, 100, 107, 124, 128 };
    return ARRAY_COUNT_END(shadows1) != std::find(shadows1, ARRAY_COUNT_END(shadows1), index);
}

int ObjGras::GetActionObject(u32 index)
{
    switch(index)
    {
	case 6:		return MP2::OBJ_ABANDONEDMINE;
	case 30:	return MP2::OBJ_FAERIERING;
        default: break;
    }

    return MP2::OBJ_ZERO;
}

int ObjGra2::GetActionObject(u32 index)
{
    switch(index)
    {
	case 4:		return MP2::OBJ_HILLFORT;
	case 7:		return MP2::OBJ_HALFLINGHOLE;
	case 21:	return MP2::OBJ_TREECITY;
	case 55:	return MP2::OBJ_WINDMILL;
	case 84:	return MP2::OBJ_ARCHERHOUSE;
	case 92:	return MP2::OBJ_GOBLINHUT;
	case 114:	return MP2::OBJ_DWARFCOTT;
	case 125:
	case 126:	return MP2::OBJ_ORACLE;
	case 129:	return MP2::OBJ_OBELISK;
        default: break;
    }

    return MP2::OBJ_ZERO;
}
