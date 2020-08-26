/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "world_pathfinding.h"
#include "ground.h"
#include "world.h"

namespace
{
    uint8_t ReflectDirectionIndex( uint8_t direction )
    {
        return ( direction + 4 ) % 8;
    }

    uint16_t GetDirectionBitmask( uint8_t direction, bool reflect = false )
    {
        return 1 << ( reflect ? ( direction + 4 ) % 8 : direction );
    }

    std::vector<int> GetDirectionOffsets( uint32_t mapWidth )
    {
        const int width = static_cast<int>( mapWidth );
        std::vector<int> offsets( 8 );
        offsets[TOP_LEFT] = -width - 1;
        offsets[TOP] = -width;
        offsets[TOP_RIGHT] = -width + 1;
        offsets[RIGHT] = 1;
        offsets[BOTTOM_RIGHT] = width + 1;
        offsets[BOTTOM] = width;
        offsets[BOTTOM_LEFT] = width - 1;
        offsets[LEFT] = -1;
        return offsets;
    }

    int ConvertExtendedIndex( int index, uint32_t width )
    {
        const uint32_t originalWidth = width - 2;
        return ( index / originalWidth + 1 ) * width + ( index % originalWidth ) + 1;
    }
}

void Pathfinder::buildPath( int from, int target, bool ignoreObjects )
{
    std::vector<int> path;
    int currentNode = target;
    while ( currentNode != from && currentNode != -1 ) {
        PathfindingNode & node = _cache[currentNode];
        path.push_back( currentNode );
        currentNode = node._from;
    }
}

void Pathfinder::evaluateMap( int start, uint32_t skillLevel )
{
    const int width = world.w();
    const int height = world.h();

    _cache.clear();
    _cache.resize( width * height );
    _cache[start] = PathfindingNode( start, -1, 0 );

    std::vector<int> processedNodes;
    processedNodes.push_back( start );
    size_t lastProcessedNode = 0;
    while ( lastProcessedNode != processedNodes.size() ) {
        const int currentNodeIdx = processedNodes[lastProcessedNode];
        for ( uint8_t direction = 0; direction < 8; ++direction ) {
            if ( Maps::isValidDirection( currentNodeIdx, GetDirectionBitmask( direction ) ) ) {
                const int newIndex = Maps::GetDirectionIndex( currentNodeIdx, GetDirectionBitmask( direction ) );
                const Maps::Tiles & newTile = world.GetTiles( newIndex );
                uint32_t moveCost = _cache[currentNodeIdx]._cost + Maps::Ground::GetPenalty( newTile, 0 );
                if ( newTile.GetPassable() & GetDirectionBitmask( direction, true ) && !newTile.isWater() ) {
                    if ( _cache[newIndex]._index == -1 || _cache[newIndex]._cost > moveCost ) {
                        processedNodes.push_back( newIndex );
                        _cache[newIndex]._index = newIndex;
                        _cache[newIndex]._from = currentNodeIdx;
                        _cache[newIndex]._cost = moveCost;
                    }
                }
            }
        }

        ++lastProcessedNode;
    }
}
