/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
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

#include "map_random_generator_info.h"

#include <cassert>

namespace Maps::Random_Generator
{
    NodeCache::NodeCache( const int32_t width, const int32_t height )
        : _mapSize( width )
        , _outOfBounds( -1 )
        , _data( static_cast<size_t>( width ) * height )
    {
        assert( width > 0 && height > 0 );
        assert( width == height );

        _outOfBounds.type = NodeType::BORDER;

        for ( size_t i = 0; i < _data.size(); ++i ) {
            _data[i].index = static_cast<int>( i );
        }
    }

    void MapEconomy::increaseMineCount( const int resource )
    {
        const auto it = minesCount.find( resource );
        assert( it != minesCount.end() );
        ++it->second;
    }

    int MapEconomy::pickNextMineResource()
    {
        const auto it = std::min_element( secondaryResources.begin(), secondaryResources.end(),
                                          [this]( const auto & a, const auto & b ) { return minesCount.at( a ) < minesCount.at( b ); } );
        assert( it != secondaryResources.end() );

        return *it;
    }
}
