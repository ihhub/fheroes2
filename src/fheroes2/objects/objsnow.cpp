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
#include "objsnow.h"

int ObjSnow::GetPassable(u32 index)
{
    const u8 disabled[] = { 22, 26, 27, 28, 30, 32, 34, 35, 37, 38, 39, 81, 82, 83, 84, 197, 198 };
    const u8 restricted[] = { 2, 12, 41, 42, 43, 44, 45, 49, 50, 55, 56, 57, 60, 64, 65, 68, 71, 74, 77, 80,
	85, 86, 87, 88, 89, 90, 91, 92, 94, 95, 132, 149, 151, 159, 177, 184, 199, 200, 202, 208, 210 };

    if(isShadow(index))
        return DIRECTION_ALL;
    else
    if(isAction(index) ||
        ARRAY_COUNT_END(disabled) != std::find(disabled, ARRAY_COUNT_END(disabled), index))
        return 0;

    return ARRAY_COUNT_END(restricted) != std::find(restricted, ARRAY_COUNT_END(restricted), index) ?
            DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjSnow::isAction(u32 index)
{
    return MP2::OBJ_ZERO != GetActionObject(index);
}

bool ObjSnow::isShadow(u32 index)
{
    const u8 shadows [] = { 21, 25, 29, 31, 33, 36, 40, 48, 54, 59, 63, 67, 70, 73, 76, 79,
	    104, 108, 120, 124, 137, 140, 142, 144, 148, 193, 203, 207 };

    return ARRAY_COUNT_END(shadows) != std::find(shadows, ARRAY_COUNT_END(shadows), index);
}

int ObjSnow::GetActionObject(u32 index)
{
    switch(index)
    {
	case 3:		return MP2::OBJ_CAVE;
	case 13:	return MP2::OBJ_LEANTO;
	case 128:	return MP2::OBJ_WINDMILL;
	case 138:	return MP2::OBJ_WATCHTOWER;
	case 141:	return MP2::OBJ_OBELISK;
	case 143:	return MP2::OBJ_SIGN;
	case 150:	return MP2::OBJ_ALCHEMYTOWER;
	case 160:	return MP2::OBJ_GRAVEYARD;
	case 191:	return MP2::OBJ_WATERWHEEL;
	case 194:	return MP2::OBJ_MAGICWELL;
	case 201:	return MP2::OBJ_SAWMILL;
	case 209:	return MP2::OBJ_GRAVEYARD;
        default: break;
    }

    return MP2::OBJ_ZERO;
}
