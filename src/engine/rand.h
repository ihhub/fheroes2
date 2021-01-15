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

#include <cstdlib>
#include <list>
#include <utility>
#include <vector>

#include "types.h"

namespace Rand
{
    uint32_t Get( uint32_t from, uint32_t to = 0 );

    template <typename T>
    const T * Get( const std::vector<T> & vec )
    {
        if ( vec.empty() )
            return nullptr;

        const uint32_t id = Rand::Get( static_cast<uint32_t>( vec.size() - 1 ) );
        return &vec[id];
    }

    template <typename T>
    const T * Get( const std::list<T> & list )
    {
        if ( list.empty() )
            return nullptr;

        typename std::list<T>::const_iterator it = list.begin();
        std::advance( it, Rand::Get( static_cast<uint32_t>( list.size() - 1 ) ) );
        return &( *it );
    }

    typedef std::pair<s32, u32> ValuePercent;

    class Queue : private std::vector<ValuePercent>
    {
    public:
        Queue( u32 size = 0 );

        void Reset( void );
        void Push( s32, u32 );
        size_t Size( void ) const;
        s32 Get( void );
    };
}

#endif
