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
#include "objswmp.h"

int ObjSwmp::GetPassable(u32 index)
{
    const u8 disabled[] = { 88, 89, 90, 91, 94, 95, 96, 97, 98, 108, 109, 110, 112,
	113, 115, 116,118, 119, 122, 123, 143, 144 };
    const u8 restricted[] = { 32, 33, 67, 74, 82, 85, 100, 101, 102, 103, 104, 105,
	126, 128, 129, 131, 133, 134, 135, 137, 138, 139, 145, 146, 147, 148, 149, 150,
	151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 166, 167, 171, 172, 176, 177,
	179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
	196, 198, 199, 200, 201, 203, 205, 208, 209, 212, 213 };

    if(isShadow(index))
        return DIRECTION_ALL;
    else
    if(isAction(index) ||
        ARRAY_COUNT_END(disabled) != std::find(disabled, ARRAY_COUNT_END(disabled), index))
        return 0;

    return ARRAY_COUNT_END(restricted) != std::find(restricted, ARRAY_COUNT_END(restricted), index) ?
            DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjSwmp::isAction(u32 index)
{
    return MP2::OBJ_ZERO != GetActionObject(index);
}

bool ObjSwmp::isShadow(u32 index)
{
    const u8 shadows [] = { 14, 21, 31, 43, 66, 83, 125, 127, 130, 132, 136, 141, 163, 170,
	175, 178, 195, 197, 202, 204, 207, 211, 215 };

    return ARRAY_COUNT_END(shadows) != std::find(shadows, ARRAY_COUNT_END(shadows), index);
}

int ObjSwmp::GetActionObject(u32 index)
{
    switch(index)
    {
	case 22:	return MP2::OBJ_WITCHSHUT;
	case 81:	return MP2::OBJ_XANADU;
	case 84:	return MP2::OBJ_FAERIERING;
	case 140:	return MP2::OBJ_SIGN;
	case 216:	return MP2::OBJ_OBELISK;
        default: break;
    }

    return MP2::OBJ_ZERO;
}
