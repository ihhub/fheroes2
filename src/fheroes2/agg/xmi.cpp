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

#include "xmi.h"

namespace XMI
{
    const struct
    {
	int type;
	const char* string;
   } xmimap[] = {
        { UNKNOWN,      "UNKNOWN"      },
	{ MIDI0002,	"MIDI0002.XMI" },
	{ MIDI0003,	"MIDI0003.XMI" },
	{ MIDI0004,	"MIDI0004.XMI" },
	{ MIDI0005,	"MIDI0005.XMI" },
	{ MIDI0006,	"MIDI0006.XMI" },
	{ MIDI0007,	"MIDI0007.XMI" },
	{ MIDI0008,	"MIDI0008.XMI" },
	{ MIDI0009,	"MIDI0009.XMI" },
	{ MIDI0010,	"MIDI0010.XMI" },
	{ MIDI0011,	"MIDI0011.XMI" },
	{ MIDI0013,	"MIDI0013.XMI" },
	{ MIDI0014,	"MIDI0014.XMI" },
	{ MIDI0015,	"MIDI0015.XMI" },
	{ MIDI0017,	"MIDI0017.XMI" },
	{ MIDI0018,	"MIDI0018.XMI" },
	{ MIDI0042,	"MIDI0042.XMI" },
	{ MIDI0043,	"MIDI0043.XMI" },
    };
}

const char* XMI::GetString(int xmi)
{
    return UNKNOWN < xmi && MIDI0043 >= xmi ? xmimap[xmi].string : xmimap[UNKNOWN].string;
}

int XMI::FromMUS(int mus)
{
    switch(mus)
    {
        case MUS::BATTLE1:	return MIDI0002;
        case MUS::BATTLE2:	return MIDI0003;
        case MUS::BATTLE3:	return MIDI0004;
        case MUS::SORCERESS:	return MIDI0005;
        case MUS::WARLOCK:	return MIDI0006;
        case MUS::NECROMANCER:	return MIDI0007;
        case MUS::KNIGHT:	return MIDI0008;
        case MUS::BARBARIAN:	return MIDI0009;
        case MUS::WIZARD:	return MIDI0010;
        case MUS::LAVA:		return MIDI0011;
        case MUS::DESERT:	return MIDI0013;
        case MUS::SNOW:		return MIDI0014;
        case MUS::SWAMP:	return MIDI0015;
	case MUS::DIRT:		return MIDI0017;
	case MUS::GRASS:	return MIDI0018;
	case MUS::MAINMENU:	return MIDI0042;
	case MUS::VICTORY:	return MIDI0043;
	default: break;
    }

    return UNKNOWN;
}
