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

#include <algorithm>
#include <cassert>
#include <ostream>
#include <utility>

#include "logging.h"
#include "map_format_info.h"
#include "maps.h"
#include "rand.h"

namespace
{
    constexpr uint8_t directionCount{ 8 };
    const std::array<fheroes2::Point, directionCount> directionOffsets{ { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }, { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } } };
}

namespace Maps::Random_Generator
{
    NodeCache::NodeCache( const int32_t width, const int32_t height )
        : _mapSize( width )
        , _data( static_cast<size_t>( width ) * height )
    {
        assert( width > 0 && height > 0 );
        assert( width == height );

        for ( size_t i = 0; i < _data.size(); ++i ) {
            _data[i].index = static_cast<int>( i );
        }
    }

    void MapEconomy::increaseMineCount( const int resourceType )
    {
        const auto it = _minesCount.find( resourceType );
        assert( it != _minesCount.end() );
        ++it->second;
    }

    int MapEconomy::pickNextMineResource()
    {
        const auto it = std::min_element( secondaryResources.begin(), secondaryResources.end(),
                                          [this]( const auto & a, const auto & b ) { return _minesCount.at( a ) < _minesCount.at( b ); } );
        assert( it != secondaryResources.end() );

        return *it;
    }

    void Region::checkAdjacentTiles( NodeCache & rawData, Rand::PCG32 & randomGenerator )
    {
        Node & previousNode = nodes[lastProcessedNode];
        const int nodeIndex = previousNode.index;

        for ( uint8_t direction = 0; direction < directionCount; ++direction ) {
            if ( nodes.size() > sizeLimit ) {
                previousNode.type = NodeType::BORDER;
                break;
            }

            // Check diagonal direction only 50% of the time to get more circular distribution.
            // It gives randomness for uneven edges.
            if ( direction > 3 && Rand::GetWithGen( 0, 1, randomGenerator ) ) {
                continue;
            }

            // TODO: use node index and pre-calculate offsets in advance.
            //       This will speed up the below calculations.
            const fheroes2::Point newPosition = Maps::GetPoint( nodeIndex ) + directionOffsets[direction];
            if ( !Maps::isValidAbsPoint( newPosition.x, newPosition.y ) ) {
                continue;
            }

            Node & newTile = rawData.getNode( newPosition );
            if ( newTile.region == 0 && newTile.type == NodeType::OPEN ) {
                newTile.region = id;
                nodes.emplace_back( newTile );
            }
            else if ( newTile.region != id ) {
                previousNode.type = NodeType::BORDER;
                neighbours.insert( newTile.region );
            }
        }
    }

    bool Region::regionExpansion( NodeCache & rawData, Rand::PCG32 & randomGenerator )
    {
        // Process only "open" nodes that exist at the start of the loop and ignore what's added.
        const size_t nodesEnd = nodes.size();

        while ( lastProcessedNode < nodesEnd ) {
            checkAdjacentTiles( rawData, randomGenerator );
            ++lastProcessedNode;
        }
        return lastProcessedNode != nodes.size();
    }

    bool Region::checkNodeForConnections( NodeCache & data, std::vector<Region> & mapRegions, Node & node )
    {
        if ( node.type != NodeType::BORDER ) {
            return false;
        }

        const fheroes2::Point position = Maps::GetPoint( node.index );

        int distinctNeighbours = 0;
        uint32_t seen = node.region;
        for ( uint8_t direction = 0; direction < directionCount; ++direction ) {
            const Node & newTile = data.getNode( position + directionOffsets[direction] );

            if ( newTile.index == -1 || newTile.region == id ) {
                continue;
            }

            if ( seen != newTile.region ) {
                seen = newTile.region;

                ++distinctNeighbours;
                if ( distinctNeighbours > 1 ) {
                    return false;
                }
            }
        }

        for ( uint8_t direction = 0; direction < 4; ++direction ) {
            Node & adjacent = data.getNode( position + directionOffsets[direction] );

            if ( adjacent.index == -1 || adjacent.region == id || mapRegions[adjacent.region].groundType == Ground::WATER ) {
                continue;
            }

            Node & twoAway = data.getNode( position + directionOffsets[direction] + directionOffsets[direction] );
            if ( twoAway.index == -1 || twoAway.type != NodeType::OPEN ) {
                continue;
            }

            if ( connections.find( adjacent.region ) == connections.end() ) {
                DEBUG_LOG( DBG_DEVEL, DBG_TRACE, "Found a connection between " << id << " and " << adjacent.region << ", via " << node.index )
                connections.emplace( adjacent.region, node.index );
                mapRegions[adjacent.region].connections.emplace( id, node.index );
                node.type = NodeType::CONNECTOR;
                adjacent.type = NodeType::PATH;
                twoAway.type = NodeType::PATH;

                Node & stepBack = data.getNode( position - directionOffsets[direction] );
                if ( stepBack.index != -1 ) {
                    stepBack.type = NodeType::PATH;
                }

                break;
            }
        }

        return node.type == NodeType::CONNECTOR;
    }

    fheroes2::Point Region::adjustRegionToFitCastle( const Map_Format::MapFormat & mapFormat )
    {
        const fheroes2::Point startingLocation = Maps::GetPoint( centerIndex );
        const int32_t castleX = std::min( std::max( startingLocation.x, 4 ), mapFormat.width - 4 );
        const int32_t castleY = std::min( std::max( startingLocation.y, 4 ), mapFormat.width - 4 );
        centerIndex = Maps::GetIndexFromAbsPoint( castleX, castleY + 2 );
        return { castleX, castleY };
    }
}
