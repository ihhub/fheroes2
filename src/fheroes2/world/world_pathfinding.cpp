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

void BuildPath( int from, int target )
{
    std::vector<int> path;
    int currentNode = 709;
    while ( currentNode != 1900 && currentNode != -1 ) {
        PathfindingNode & node = searchArray[currentNode];
        path.push_back( currentNode );
        currentNode = node._from;
    }
}

void RecomputePathfindingCache( int start )
{
    const int width = world.w();
    const int height = world.h();

    std::vector<PathfindingNode> searchArray( width * height );
    searchArray[start] = PathfindingNode( start, -1, 0, false );

    std::vector<int> processedNodes;
    processedNodes.push_back( start );
    size_t lastProcessedNode = 0;
    while ( lastProcessedNode != processedNodes.size() ) {
        const int currentNodeIdx = processedNodes[lastProcessedNode];
        for ( uint8_t direction = 0; direction < 8; ++direction ) {
            if ( Maps::isValidDirection( currentNodeIdx, GetDirectionBitmask( direction ) ) ) {
                const int newIndex = Maps::GetDirectionIndex( currentNodeIdx, GetDirectionBitmask( direction ) );
                const Maps::Tiles & newTile = world.GetTiles( newIndex );
                uint32_t moveCost = searchArray[currentNodeIdx]._cost + Maps::Ground::GetPenalty( newTile, 0 );
                if ( newTile.GetPassable() & GetDirectionBitmask( direction, true ) && !newTile.isWater() ) {
                    if ( searchArray[newIndex]._isOpen || searchArray[newIndex]._cost > moveCost ) {
                        processedNodes.push_back( newIndex );
                        searchArray[newIndex]._index = newIndex;
                        searchArray[newIndex]._from = currentNodeIdx;
                        searchArray[newIndex]._cost = moveCost;
                        searchArray[newIndex]._isOpen = false;
                    }
                }
            }
        }

        ++lastProcessedNode;
    }
}
