/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#include "rand.h"

#include <numeric>
#include <random>

namespace
{
    // Implementation of Fast Random Integer Generation in an Interval (https://arxiv.org/abs/1805.10941)
    // NOTE: we can't use std::uniform_int_distribution here because it behaves differently on different platforms
    uint32_t uniformIntInInterval( const uint32_t range, Rand::PCG32 & gen )
    {
        assert( range > 0 );

        // Our implementation assumes our RNG can use the entire range of uint32_t
        static_assert( Rand::PCG32::min() == 0 && Rand::PCG32::max() == std::numeric_limits<uint32_t>::max() );

        uint32_t generated = gen();
        uint64_t mult = ( static_cast<uint64_t>( generated ) * range );
        uint32_t lowerPart = static_cast<uint32_t>( mult );

        if ( lowerPart >= range ) {
            const uint32_t upperPart = static_cast<uint32_t>( mult >> 32 );
            assert( upperPart <= range );

            return upperPart;
        }

        // This is the same as (2**32 - range) % range in the two's complement representation
        const uint32_t discardBound = static_cast<uint32_t>( ~( range - 1 ) ) % range;

        while ( lowerPart < discardBound ) {
            generated = gen();
            mult = ( static_cast<uint64_t>( generated ) * range );
            lowerPart = static_cast<uint32_t>( mult );
        }

        const uint32_t upperPart = static_cast<uint32_t>( mult >> 32 );
        assert( upperPart <= range );

        return upperPart;
    }
}

uint32_t Rand::uniformIntDistribution( const uint32_t from, const uint32_t to, PCG32 & gen )
{
    if ( from == to ) {
        return from;
    }

    assert( from < to );
    const uint32_t rangeExclusive = to - from;
    // If the range is the entire uint32_t (from 0 to 2**32-1), we can just return a random number
    if ( rangeExclusive == std::numeric_limits<uint32_t>::max() ) {
        static_assert( PCG32::min() == 0 && PCG32::max() == std::numeric_limits<uint32_t>::max() );

        return gen();
    }

    return from + uniformIntInInterval( rangeExclusive + 1, gen );
}

Rand::PCG32 & Rand::CurrentThreadRandomDevice()
{
    thread_local std::random_device rd;
    thread_local PCG32 gen( rd );

    return gen;
}

uint32_t Rand::Get( uint32_t from, uint32_t to /* = 0 */ )
{
    if ( from > to ) {
        std::swap( from, to );
    }

    return uniformIntDistribution( from, to, CurrentThreadRandomDevice() );
}

uint32_t Rand::GetWithSeed( uint32_t from, uint32_t to, uint32_t seed )
{
    if ( from > to ) {
        std::swap( from, to );
    }

    PCG32 seededGen( seed );
    return uniformIntDistribution( from, to, seededGen );
}

uint32_t Rand::GetWithGen( uint32_t from, uint32_t to, PCG32 & gen )
{
    if ( from > to ) {
        std::swap( from, to );
    }

    return uniformIntDistribution( from, to, gen );
}

int32_t Rand::Queue::Get( const std::function<uint32_t( uint32_t )> & randomFunc ) const
{
    if ( empty() ) {
        return 0;
    }

    const uint32_t sum = std::accumulate( begin(), end(), static_cast<uint32_t>( 0 ), []( const uint32_t total, const ValueWeight & vw ) { return total + vw.second; } );
    assert( sum > 0 );

    uint32_t rand = randomFunc( sum - 1 );
    assert( rand < sum );

    for ( const auto & [value, weight] : *this ) {
        if ( rand < weight ) {
            return value;
        }

        rand -= weight;
    }

    assert( 0 );

    return 0;
}
