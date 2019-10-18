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
#ifndef H2MAPSGROUND_H
#define H2MAPSGROUND_H

#include <string>
#include "gamedefs.h"

namespace Maps
{
    class Tiles;

    namespace Ground
    {
        enum
        {
            UNKNOWN	= 0x0000,
    	    DESERT	= 0x0001,
    	    SNOW	= 0x0002,
    	    SWAMP	= 0x0004,
    	    WASTELAND   = 0x0008,
    	    BEACH	= 0x0010,
    	    LAVA	= 0x0020,
    	    DIRT	= 0x0040,
    	    GRASS	= 0x0080,
    	    WATER	= 0x0100,
	    ALL		= DESERT | SNOW | SWAMP | WASTELAND | BEACH | LAVA | DIRT | GRASS
        };

        const char*	String(int);
        u32		GetPenalty(s32, int direction, u32 pathfinding);
    }
}

#endif
