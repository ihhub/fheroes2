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

#ifndef H2BITMODES_H
#define H2BITMODES_H

#include "gamedefs.h"

#define MODES_ALL 0xFFFFFFFF

class StreamBase;

class BitModes
{
public:
    BitModes()
        : modes( 0 ){};

    u32 operator()( void ) const
    {
        return modes;
    }
    void SetModes( u32 f )
    {
        modes |= f;
    }
    void ResetModes( u32 f )
    {
        modes &= ~f;
    }
    void ToggleModes( u32 f )
    {
        modes ^= f;
    }

    bool Modes( u32 f ) const
    {
        return ( modes & f ) != 0;
    }

protected:
    friend StreamBase & operator<<( StreamBase &, const BitModes & );
    friend StreamBase & operator>>( StreamBase &, BitModes & );

    u32 modes;
};

StreamBase & operator<<( StreamBase &, const BitModes & );
StreamBase & operator>>( StreamBase &, BitModes & );

#endif
