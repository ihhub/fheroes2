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

#ifndef H2XMI_H
#define H2XMI_H

namespace XMI
{
    enum
    {
        UNKNOWN,
        MIDI0002,
        MIDI0003,
        MIDI0004,
        MIDI0005,
        MIDI0006,
        MIDI0007,
        MIDI0008,
        MIDI0009,
        MIDI0010,
        MIDI0011,
        MIDI0013,
        MIDI0014,
        MIDI0015,
        MIDI0017,
        MIDI0018,
        MIDI0042,
        MIDI0043,
        MIDI_ORIGINAL_KNIGHT,
        MIDI_ORIGINAL_BARBARIAN,
        MIDI_ORIGINAL_SORCERESS,
        MIDI_ORIGINAL_WARLOCK,
        MIDI_ORIGINAL_WIZARD,
        MIDI_ORIGINAL_NECROMANCER
    };

    const char * GetString( int track );
    int FromMUS( int track, bool expansion );
}

#endif
