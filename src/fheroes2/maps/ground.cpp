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

#include "maps_tiles.h"
#include "direction.h"
#include "world.h"
#include "ground.h"

const char* Maps::Ground::String(int ground)
{
    const char* str_ground[] = { _("Desert"), _("Snow"), _("Swamp"), _("Wasteland"), _("Beach"), 
	_("Lava"), _("Dirt"), _("Grass"), _("Water"), "Unknown" };

    switch(ground)
    {
        case DESERT:	return str_ground[0];
	case SNOW:	return str_ground[1];
	case SWAMP:	return str_ground[2];
	case WASTELAND:	return str_ground[3];
	case BEACH:	return str_ground[4];
	case LAVA:	return str_ground[5];
	case DIRT:	return str_ground[6];
	case GRASS:	return str_ground[7];
	case WATER:	return str_ground[8];
	default: break;
    }

    return str_ground[8];
}

u32 Maps::Ground::GetPenalty(s32 index, int direct, u32 level)
{
    const Maps::Tiles & tile = world.GetTiles(index);

    //            none   basc   advd   expr
    //    Desert  2.00   1.75   1.50   1.00
    //    Snow    1.75   1.50   1.25   1.00
    //    Swamp   1.75   1.50   1.25   1.00
    //    Cracked 1.25   1.00   1.00   1.00
    //    Beach   1.25   1.00   1.00   1.00
    //    Lava    1.00   1.00   1.00   1.00
    //    Dirt    1.00   1.00   1.00   1.00
    //    Grass   1.00   1.00   1.00   1.00
    //    Water   1.00   1.00   1.00   1.00
    //    Road    0.75   0.75   0.75   0.75

    if(tile.isRoad(direct))
	// road priority: need small value
	return 59;

    u32 result = 100;

    switch(tile.GetGround())
    {
        case DESERT:
	    switch(level)
	    {
		case Skill::Level::EXPERT:	break;
		case Skill::Level::ADVANCED:	result += 50; break;
		case Skill::Level::BASIC:	result += 75; break;
		default:			result += 100; break;
	    }
	    break;

        case SNOW:
        case SWAMP:
	    switch(level)
	    {
		case Skill::Level::EXPERT:	break;
		case Skill::Level::ADVANCED:	result += 25; break;
		case Skill::Level::BASIC:	result += 50; break;
		default:			result += 75; break;
	    }
	    break;

        case WASTELAND:
        case BEACH:
            result += (Skill::Level::NONE == level ? 25 : 0);
	    break;

	default: break;
    }

    if(direct & (Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM_LEFT | Direction::TOP_LEFT))
	result += result * 55 / 100;

    return result;
}
