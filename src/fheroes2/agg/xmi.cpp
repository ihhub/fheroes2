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
#include "mus.h"

namespace XMI
{
    const struct
    {
        int type;
        const char * string;
    } xmimap[] = {
        {UNKNOWN, "UNKNOWN"},
        {MIDI0002, "MIDI0002.XMI"},
        {MIDI0003, "MIDI0003.XMI"},
        {MIDI0004, "MIDI0004.XMI"},
        {MIDI0005, "MIDI0005.XMI"},
        {MIDI0006, "MIDI0006.XMI"},
        {MIDI0007, "MIDI0007.XMI"},
        {MIDI0008, "MIDI0008.XMI"},
        {MIDI0009, "MIDI0009.XMI"},
        {MIDI0010, "MIDI0010.XMI"},
        {MIDI0011, "MIDI0011.XMI"},
        {MIDI0013, "MIDI0013.XMI"},
        {MIDI0014, "MIDI0014.XMI"},
        {MIDI0015, "MIDI0015.XMI"},
        {MIDI0017, "MIDI0017.XMI"},
        {MIDI0018, "MIDI0018.XMI"},
        {MIDI0042, "MIDI0042.XMI"},
        {MIDI0043, "MIDI0043.XMI"},
        {MIDI_ORIGINAL_KNIGHT, "MIDI0009.XMI"}, // Knight theme was used by both Barbarian and Wizard castles, so we use either MIDI0009 or MIDI0010
        {MIDI_ORIGINAL_BARBARIAN, "MIDI0007.XMI"}, // Barbarian intended theme is under MIDI0007
        {MIDI_ORIGINAL_SORCERESS, "MIDI0005.XMI"}, // Sorceress doesn't have own track in OG release, Warlock theme was used
        {MIDI_ORIGINAL_WARLOCK, "MIDI0005.XMI"}, // Warlock theme was set to Sorceress so we use MIDI0005
        {MIDI_ORIGINAL_WIZARD, "MIDI0008.XMI"}, // Wizard's and Knight's tracks were switched around, so we use MIDI0008
        {MIDI_ORIGINAL_NECROMANCER, "MIDI0006.XMI"} // Necromancer theme has trickled down to Warlock so we use MIDI0006
    };
}

const char * XMI::GetString( int track )
{
    return UNKNOWN < track && MIDI_ORIGINAL_NECROMANCER >= track ? xmimap[track].string : xmimap[UNKNOWN].string;
}

// Due to a bug in Succession Wars/demo release (HEROES2.AGG) we have to remap original MIDI tracks to intended castles
int XMI::FromMUS( int track, bool expansion )
{
    switch ( track ) {
    case MUS::BATTLE1:
        return MIDI0002;
    case MUS::BATTLE2:
        return MIDI0003;
    case MUS::BATTLE3:
        return MIDI0004;
    case MUS::SORCERESS:
        // Sorceress didn't have own XMI file in original release unfortunately
        return MIDI0005;
    case MUS::WARLOCK:
        return expansion ? MIDI0006 : MIDI_ORIGINAL_WARLOCK;
    case MUS::NECROMANCER:
        return expansion ? MIDI0007 : MIDI_ORIGINAL_NECROMANCER;
    case MUS::KNIGHT:
        return expansion ? MIDI0008 : MIDI_ORIGINAL_KNIGHT;
    case MUS::BARBARIAN:
        return expansion ? MIDI0009 : MIDI_ORIGINAL_BARBARIAN;
    case MUS::WIZARD:
        return expansion ? MIDI0010 : MIDI_ORIGINAL_WIZARD;
    case MUS::LAVA:
        return MIDI0011;
    case MUS::DESERT:
        return MIDI0013;
    case MUS::SNOW:
        return MIDI0014;
    case MUS::SWAMP:
        return MIDI0015;
    case MUS::DIRT:
        return MIDI0017;
    case MUS::GRASS:
        return MIDI0018;
    case MUS::MAINMENU:
        return MIDI0042;
    case MUS::VICTORY:
        return MIDI0043;
    default:
        break;
    }

    return UNKNOWN;
}
