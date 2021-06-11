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
#ifndef H2RAND_H
#define H2RAND_H

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <list>
#include <random>
#include <utility>
#include <vector>

#include "types.h"

namespace Rand
{
    std::mt19937 & CurrentThreadRandomDevice();

    uint32_t Get( uint32_t from, uint32_t to = 0 );
    uint32_t GetWithSeed( uint32_t from, uint32_t to, uint32_t seed );
    uint32_t GetWithGen( uint32_t from, uint32_t to, std::mt19937 & gen );

    template <typename T>
    void Shuffle( std::vector<T> & vec )
    {
        std::shuffle( vec.begin(), vec.end(), CurrentThreadRandomDevice() );
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
        explicit Queue( u32 size = 0 );

        void Push( s32 value, u32 percent );
        size_t Size( void ) const;
        int32_t Get();
        int32_t GetWithSeed( uint32_t seed );

    private:
        int32_t Get( const std::function<uint32_t( uint32_t )> & randomFunc );
    };
}

#endif
