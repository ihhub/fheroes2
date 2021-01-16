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

#ifndef H2PUZZLE_H
#define H2PUZZLE_H

#include <bitset>

#include "gamedefs.h"
#include "serialize.h"

#define PUZZLETILES 48

class Puzzle : public std::bitset<PUZZLETILES>
{
public:
    Puzzle();
    Puzzle & operator=( const char * );

    void Update( u32 open, u32 total );
    void ShowMapsDialog( void ) const;

    u8 zone1_order[24];
    u8 zone2_order[16];
    u8 zone3_order[4];
    u8 zone4_order[4];
};

StreamBase & operator<<( StreamBase &, const Puzzle & );
StreamBase & operator>>( StreamBase &, Puzzle & );

#endif
