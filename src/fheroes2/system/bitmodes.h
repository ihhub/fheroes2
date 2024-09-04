/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cstdint>

class IStreamBase;
class OStreamBase;

class BitModes
{
public:
    BitModes()
        : modes( 0 )
    {}

    void SetModes( const uint32_t f )
    {
        modes |= f;
    }

    void ResetModes( const uint32_t f )
    {
        modes &= ~f;
    }

    // Returns true if any of the requested modes is set, otherwise returns false
    bool Modes( const uint32_t f ) const
    {
        return ( modes & f ) != 0;
    }

    // Returns true if all the requested modes are set, otherwise returns false
    bool AllModes( const uint32_t f ) const
    {
        return ( modes & f ) == f && f != 0;
    }

protected:
    friend OStreamBase & operator<<( OStreamBase & stream, const BitModes & b );
    friend IStreamBase & operator>>( IStreamBase & stream, BitModes & b );

    uint32_t modes;
};

#endif
