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

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
// IWYU issue workaround. When <exception> is included IWYU will remove it and require <string>.
// When <string> is included it'll remove it and require <exception>
// IWYU pragma: no_include <exception>
#include <functional>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace Rand
{
    class PCG32
    {
    public:
        static constexpr uint32_t min()
        {
            return std::numeric_limits<uint32_t>::min();
        }

        static constexpr uint32_t max()
        {
            return std::numeric_limits<uint32_t>::max();
        }

        template <typename Random, typename = std::enable_if_t<std::is_invocable_v<Random>>>
        explicit constexpr PCG32( Random & gen )
            : PCG32( generateUInt64( gen ), generateUInt64( gen ) )
        {}

        explicit constexpr PCG32( const uint64_t seed = defaultSeed, const uint64_t stream = defaultStream )
            : _state( 0 )
            , _increment( stream )
        {
            advanceState();
            _state += seed;
            advanceState();
        }

        constexpr uint32_t operator()()
        {
            const uint64_t prevState = _state;
            advanceState();

            const uint32_t xorShifted = static_cast<uint32_t>( ( ( prevState >> 18U ) ^ prevState ) >> 27U );
            const uint32_t rotations = prevState >> 59U;
            return rotateRight( xorShifted, rotations );
        }

        constexpr uint64_t getStream() const
        {
            return _increment;
        }

        constexpr void setStream( const uint64_t stream )
        {
            _increment = stream;
        }

    private:
        static constexpr uint64_t multiplier = 6364136223846793005ULL;
        static constexpr uint64_t defaultStream = 54ULL;
        static constexpr uint64_t defaultSeed = 42ULL;

        static constexpr uint32_t rotateRight( const uint32_t value, const uint32_t rotations )
        {
            return ( value >> rotations ) | ( value << ( ( ~( rotations - 1 ) ) & 31 ) );
        }

        // Defines a new templated false value to be used in static_assert
        // See https://devblogs.microsoft.com/oldnewthing/20200311-00/?p=103553
        template <typename T>
        static constexpr bool alwaysFalseValue = false;

        template <typename Random>
        static uint64_t generateUInt64( Random & gen )
        {
            static_assert( Random::min() == std::numeric_limits<uint32_t>::min() );
            if constexpr ( Random::max() == std::numeric_limits<uint32_t>::max() ) {
                return static_cast<uint64_t>( gen() ) << 32 | static_cast<uint64_t>( gen() );
            }
            else if constexpr ( Random::max() == std::numeric_limits<uint64_t>::max() ) {
                return gen();
            }
            else {
                // Using alwaysFalseValue because static_assert( false ) is ill-formed in c++17
                static_assert( alwaysFalseValue<Random>, "Unsupported random generator type" );
            }
        }

        constexpr void advanceState()
        {
            _state = _state * multiplier + ( _increment | 1 );
        }

        uint64_t _state;
        uint64_t _increment;
    };

    uint32_t uniformIntDistribution( const uint32_t from, const uint32_t to, PCG32 & gen );

    // Fisher-Yates shuffle AKA Knuth shuffle, probably the same as std::shuffle.
    // NOTE: we can't use std::shuffle here because it uses std::uniform_int_distribution which behaves differently on different platforms.
    template <class Iter>
    void shuffle( Iter first, Iter last, PCG32 & gen )
    {
        if ( first == last ) {
            return;
        }

        assert( first < last );

        // Change last from once-past-the-end to last element
        --last;
        const typename std::iterator_traits<Iter>::difference_type interval = last - first;

        // Our implementation doesn't work for intervals bigger than 2**32 - 1
        if constexpr ( sizeof( interval ) > sizeof( uint32_t ) ) {
            assert( interval <= static_cast<typename std::iterator_traits<Iter>::difference_type>( std::numeric_limits<uint32_t>::max() ) );
        }

        uint32_t remainingSwaps = static_cast<uint32_t>( interval );
        while ( remainingSwaps > 0 ) {
            // Allow argument-dependant lookup (ADL) for swap: first try in the namespace of the type, then in the std namespace.
            using std::swap;
            const uint32_t index = uniformIntDistribution( 0, remainingSwaps, gen );
            swap( *last, *( first + index ) );
            --last;
            --remainingSwaps;
        }
    }

    PCG32 & CurrentThreadRandomDevice();

    uint32_t Get( uint32_t from, uint32_t to = 0 );

    template <typename T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
    T Get( const T from, const T to )
    {
        return static_cast<T>( Get( static_cast<uint32_t>( from ), static_cast<uint32_t>( to ) ) );
    }

    uint32_t GetWithSeed( uint32_t from, uint32_t to, uint32_t seed );

    template <typename T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
    T GetWithSeed( const T from, const T to, const uint32_t seed )
    {
        return static_cast<T>( GetWithSeed( static_cast<uint32_t>( from ), static_cast<uint32_t>( to ), seed ) );
    }

    uint32_t GetWithGen( uint32_t from, uint32_t to, PCG32 & gen );

    template <typename T>
    void Shuffle( std::vector<T> & vec )
    {
        Rand::shuffle( vec.begin(), vec.end(), CurrentThreadRandomDevice() );
    }

    template <typename T>
    void ShuffleWithGen( std::vector<T> & vec, PCG32 & gen )
    {
        Rand::shuffle( vec.begin(), vec.end(), gen );
    }

    template <typename T>
    const T & Get( const std::vector<T> & vec )
    {
        assert( !vec.empty() );

        const uint32_t id = Get( static_cast<uint32_t>( vec.size() - 1 ) );
        return vec[id];
    }

    template <typename T>
    const T & GetWithGen( const std::vector<T> & vec, PCG32 & gen )
    {
        assert( !vec.empty() );

        const uint32_t id = GetWithGen( 0, static_cast<uint32_t>( vec.size() - 1 ), gen );
        return vec[id];
    }

    using ValueWeight = std::pair<int32_t, uint32_t>;

    class Queue : private std::vector<ValueWeight>
    {
    public:
        explicit Queue( uint32_t size = 0 )
        {
            reserve( size );
        }

        void Push( const int32_t value, const uint32_t weight )
        {
            if ( weight == 0 ) {
                return;
            }

            emplace_back( value, weight );
        }

        size_t Size() const
        {
            return size();
        }

        int32_t Get() const
        {
            return Get( []( const uint32_t max ) { return Rand::Get( 0, max ); } );
        }

        int32_t GetWithSeed( const uint32_t seed ) const
        {
            return Get( [seed]( const uint32_t max ) { return Rand::GetWithSeed( 0, max, seed ); } );
        }

    private:
        int32_t Get( const std::function<uint32_t( uint32_t )> & randomFunc ) const;
    };

    template <typename Seed, typename Value, std::enable_if_t<std::is_same_v<Seed, uint32_t> || std::is_same_v<Seed, uint64_t>, bool> = true>
    void combineSeedWithValueHash( Seed & seed, const Value & v )
    {
        std::hash<Value> hasher;
        seed ^= hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    }
}
