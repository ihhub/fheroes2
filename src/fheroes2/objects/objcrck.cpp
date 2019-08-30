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
#include "objcrck.h"

int ObjCrck::GetPassable(u32 index)
{
    const u8 disabled[] = { 58, 59, 63, 64, 65, 76, 77, 78, 80, 91, 102, 113, 124,
	    135, 182, 183, 185, 221, 222, 223, 227, 228, 229, 230, 238, 241, 242, 245 };
    const u8 restricted[] = { 5, 6, 10, 11, 14, 16, 17, 18, 21, 22, 24, 25, 29, 30, 31, 32, 34,
	    35, 37, 38, 40, 41, 42, 43, 46, 49, 52, 55, 57, 62, 67, 68, 69, 71, 72, 136, 148, 159,
	    170, 181, 186, 187, 188, 202, 224, 225, 226, 231, 232, 233, 234, 235, 243, 244, 246 };

    if(isShadow(index))
	return DIRECTION_ALL;
    else
    if(184 == index)
	return Direction::CENTER | Direction::BOTTOM_RIGHT | DIRECTION_TOP_ROW;
    else
    if(isAction(index) ||
	ARRAY_COUNT_END(disabled) != std::find(disabled, ARRAY_COUNT_END(disabled), index))
	return 0;

   return ARRAY_COUNT_END(restricted) != std::find(restricted, ARRAY_COUNT_END(restricted), index) ?
            DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW : DIRECTION_ALL;
}

bool ObjCrck::isAction(u32 index)
{
    return MP2::OBJ_ZERO != GetActionObject(index);
}

bool ObjCrck::isShadow(u32 index)
{
    const u8 shadows[] = { 0, 2, 9, 12, 13, 15, 20, 23, 28, 33, 36, 39,
				45, 48, 51, 54, 56, 73, 75, 79, 190, 201, 237 };
    return ARRAY_COUNT_END(shadows) != std::find(shadows, ARRAY_COUNT_END(shadows), index);
}

int ObjCrck::GetActionObject(u32 index)
{
    /*
	artesian spring: 3, 4
	wagon: 74
	troll bridge: 189
	market: 213
	watering hole: 217, 218, 219, 220
	obelisk: 238
	saw mill: 245
    */

    switch(index)
    {
	case 3:
	case 4:		return MP2::OBJ_ARTESIANSPRING;
	case 74:	return MP2::OBJ_WAGON;
	case 189:	return MP2::OBJ_TROLLBRIDGE;
	case 213:	return MP2::OBJ_TRADINGPOST;
	case 217: case 218: case 219:
	case 220:	return MP2::OBJ_WATERINGHOLE;
	case 238:	return MP2::OBJ_OBELISK;
	case 245:	return MP2::OBJ_SAWMILL;
    }

    return MP2::OBJ_ZERO;
}
