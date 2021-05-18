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

#include "logging.h"
#include "rand.h"

std::mt19937 & Rand::CurrentThreadRandomDevice()
{
    thread_local static std::random_device s_rd;
    thread_local static std::mt19937 s_gen( s_rd() );
    return s_gen;
}

uint32_t Rand::Get( uint32_t from, uint32_t to )
{
    if ( to == 0 || from > to )
        std::swap( from, to );

    std::uniform_int_distribution<uint32_t> distrib( from, to );

    return distrib( CurrentThreadRandomDevice() );
}

uint32_t Rand::GetWithSeed( uint32_t from, uint32_t to, uint32_t seed )
{
    if ( from > to )
        std::swap( from, to );

    std::uniform_int_distribution<uint32_t> distrib( from, to );
    std::mt19937 seededGen( seed );

    return distrib( seededGen );
}

uint32_t Rand::GetWithGen( uint32_t from, uint32_t to, std::mt19937 & gen )
{
    if ( from > to )
        std::swap( from, to );

    std::uniform_int_distribution<uint32_t> distrib( from, to );

    return distrib( gen );
}

Rand::Queue::Queue( u32 size )
{
    reserve( size );
}

void Rand::Queue::Push( s32 value, u32 percent )
{
    if ( percent > 0 )
        emplace_back( value, percent );
}

size_t Rand::Queue::Size( void ) const
{
    return size();
}

int32_t Rand::Queue::Get( const std::function<uint32_t( uint32_t )> & randomFunc )
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

    uint32_t rand = randomFunc( max );
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

int32_t Rand::Queue::Get()
{
    return Rand::Queue::Get( []( uint32_t max ) { return Rand::Get( 0, max ); } );
}

int32_t Rand::Queue::GetWithSeed( uint32_t seed )
{
    return Rand::Queue::Get( [seed]( uint32_t max ) { return Rand::GetWithSeed( 0, max, seed ); } );
}
