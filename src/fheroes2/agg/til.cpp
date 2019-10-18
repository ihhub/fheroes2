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

#include "surface.h"
#include "til.h"

namespace TIL
{
    const struct
    {
	int type;
	const char* string;
    } tilmap[] = {
	{ UNKNOWN,	"UNKNOWN"      },
	{ CLOF32,	"CLOF32.TIL"   },
	{ GROUND32,	"GROUND32.TIL" },
	{ STON,		"STON.TIL"     },
    };
}

const char* TIL::GetString(int til)
{
    return UNKNOWN <= til && LASTTIL > til ? tilmap[til].string : "CUSTOM";
}
