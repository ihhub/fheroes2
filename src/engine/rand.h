/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
#ifndef H2RAND_H
#define H2RAND_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <list>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

namespace Rand
{
    std::mt19937 & CurrentThreadRandomDevice();

    uint32_t Get( uint32_t from, uint32_t to = 0 );

    uint32_t GetWithSeed( uint32_t from, uint32_t to, uint32_t seed );

    template <typename T, typename std::enable_if<std::is_enum<T>::value>::type * = nullptr>
    T GetWithSeed( const T from, const T to, const uint32_t seed )
    {
        return static_cast<T>( GetWithSeed( static_cast<uint32_t>( from ), static_cast<uint32_t>( to ), seed ) );
    }

    uint32_t GetWithGen( uint32_t from, uint32_t to, std::mt19937 & gen );

    template <typename T>
    void Shuffle( std::vector<T> & vec )
    {
        std::shuffle( vec.begin(), vec.end(), CurrentThreadRandomDevice() );
    }

    template <typename T>
    void ShuffleWithSeed( std::vector<T> & vec, uint32_t seed )
    {
        std::mt19937 seededGen( seed );
        std::shuffle( vec.begin(), vec.end(), seededGen );
    }

    template <typename T>
    void ShuffleWithGen( std::vector<T> & vec, std::mt19937 & gen )
    {
        std::shuffle( vec.begin(), vec.end(), gen );
    }

    template <typename T>
    const T & Get( const std::vector<T> & vec )
    {
        assert( !vec.empty() );

        const uint32_t id = Rand::Get( static_cast<uint32_t>( vec.size() - 1 ) );
        return vec[id];
    }

    template <typename T>
    const T & GetWithGen( const std::vector<T> & vec, std::mt19937 & gen )
    {
        assert( !vec.empty() );

        const uint32_t id = Rand::GetWithGen( 0, static_cast<uint32_t>( vec.size() - 1 ), gen );
        return vec[id];
    }

    template <typename T>
    const T & Get( const std::list<T> & list )
    {
        assert( !list.empty() );

        typename std::list<T>::const_iterator it = list.begin();
        std::advance( it, Rand::Get( static_cast<uint32_t>( list.size() - 1 ) ) );
        return *it;
    }

    using ValuePercent = std::pair<int32_t, uint32_t>;

    class Queue : private std::vector<ValuePercent>
    {
    public:
        explicit Queue( uint32_t size = 0 );

        void Push( int32_t value, uint32_t percent );
        size_t Size() const;
        int32_t Get();
        int32_t GetWithSeed( uint32_t seed );

    private:
        int32_t Get( const std::function<uint32_t( uint32_t )> & randomFunc );
    };

    // Specific random generator that keeps and update its state
    class DeterministicRandomGenerator
    {
    public:
        explicit DeterministicRandomGenerator( const uint32_t initialSeed );

        // prevent accidental copies
        DeterministicRandomGenerator( const DeterministicRandomGenerator & ) = delete;
        DeterministicRandomGenerator & operator=( const DeterministicRandomGenerator & ) = delete;

        uint32_t GetSeed() const;
        void UpdateSeed( const uint32_t seed );

        uint32_t Get( const uint32_t from, const uint32_t to = 0 ) const;

        template <typename T>
        const T & Get( const std::vector<T> & vec ) const
        {
            ++_currentSeed;
            std::mt19937 seededGen( _currentSeed );
            return Rand::GetWithGen( vec, seededGen );
        }

        template <class T>
        void Shuffle( std::vector<T> & vector ) const
        {
            ++_currentSeed;
            Rand::ShuffleWithSeed( vector, _currentSeed );
        }

    private:
        mutable uint32_t _currentSeed; // this is mutable so clients that only call RNG method can receive a const instance
    };
}

#endif
