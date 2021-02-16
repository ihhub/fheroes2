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

#include <cstdlib>
#include <random>

#include "logging.h"
#include "rand.h"

namespace
{
    std::random_device s_rd;
    std::mt19937 s_gen( s_rd() );
}

uint32_t Rand::Get( uint32_t from, uint32_t to )
{
    if ( to == 0 || from > to )
        std::swap( from, to );

    std::uniform_int_distribution<> distrib( from, to );

    return static_cast<uint32_t>( distrib( s_gen ) );
}

Rand::Queue::Queue( u32 size )
{
    reserve( size );
}

void Rand::Queue::Reset( void )
{
    clear();
}

void Rand::Queue::Push( s32 value, u32 percent )
{
    if ( percent )
        emplace_back( value, percent );
}

size_t Rand::Queue::Size( void ) const
{
    return size();
}

s32 Rand::Queue::Get( void )
{
    std::vector<ValuePercent>::iterator it;

    // get max
    it = begin();
    u32 max = 0;
    for ( ; it != end(); ++it )
        max += ( *it ).second;

    // set weight (from 100)
    if ( max > 0 ) {
        it = begin();
        for ( ; it != end(); ++it )
            ( *it ).second = 100 * ( *it ).second / max;
    }

    // get max
    max = 0;
    it = begin();
    for ( ; it != end(); ++it )
        max += ( *it ).second;

    uint32_t rand = Rand::Get( max );
    uint32_t amount = 0;

    it = begin();
    for ( ; it != end(); ++it ) {
        amount += ( *it ).second;
        if ( rand <= amount )
            return ( *it ).first;
    }

    ERROR_LOG( "weight not found, return 0" );
    return 0;
}
