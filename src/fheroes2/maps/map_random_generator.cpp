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

#include "map_random_generator.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <set>
#include <utility>
#include <vector>

#include "color.h"
#include "editor_interface.h"
#include "ground.h"
#include "logging.h"
#include "map_format_helper.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "rand.h"
#include "resource.h"
#include "ui_map_object.h"
#include "world.h"

namespace
{
    const int neutralColorIndex{ Color::GetIndex( PlayerColor::UNUSED ) };
    const int randomCastleIndex = 12;
    const std::vector<int> playerStartingTerrain = { Maps::Ground::GRASS, Maps::Ground::DIRT, Maps::Ground::SNOW, Maps::Ground::LAVA, Maps::Ground::WASTELAND };
    const std::vector<int> neutralTerrain = { Maps::Ground::GRASS,     Maps::Ground::DIRT,  Maps::Ground::SNOW,  Maps::Ground::LAVA,
                                              Maps::Ground::WASTELAND, Maps::Ground::BEACH, Maps::Ground::SWAMP, Maps::Ground::DESERT };

    constexpr uint8_t directionCount{ 8 };
    const std::array<fheroes2::Point, directionCount> directionOffsets{ { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }, { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } } };

    const std::array<int, 7> resources{ Resource::WOOD, Resource::ORE, Resource::CRYSTAL, Resource::SULFUR, Resource::GEMS, Resource::MERCURY, Resource::GOLD };

    enum class NodeType : uint8_t
    {
        OPEN,
        BORDER,
        ACTION,
        OBSTACLE,
        PATH
    };

    struct Node final
    {
        int index{ -1 };
        uint32_t region{ 0 };
        NodeType type{ NodeType::OPEN };

        Node() = default;

        explicit Node( const int index_ )
            : index( index_ )
        {
            // Do nothing
        }
    };

    class NodeCache final
    {
    public:
        NodeCache( const int32_t width, const int32_t height )
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

        Node & getNode( const fheroes2::Point position )
        {
            if ( position.x < 0 || position.x >= _mapSize || position.y < 0 || position.y >= _mapSize ) {
                return _outOfBounds;
            }

            return _data[position.y * _mapSize + position.x];
        }

        Node & getNode( const int32_t index )
        {
            if ( index < 0 || index >= _mapSize * _mapSize ) {
                return _outOfBounds;
            }

            return _data[index];
        }

    private:
        const int32_t _mapSize{ 0 };
        Node _outOfBounds;
        std::vector<Node> _data;
    };

    struct Region final
    {
        uint32_t _id{ 0 };
        int32_t _centerIndex{ -1 };
        std::set<uint32_t> _neighbours;
        std::vector<Node> _nodes;
        size_t _sizeLimit{ 0 };
        size_t _lastProcessedNode{ 0 };
        int _colorIndex{ neutralColorIndex };
        int _groundType{ Maps::Ground::GRASS };

        Region() = default;

        Region( const uint32_t regionIndex, const int32_t mapIndex, const int playerColor, const int ground, const size_t expectedSize )
            : _id( regionIndex )
            , _centerIndex( mapIndex )
            , _sizeLimit( expectedSize )
            , _colorIndex( playerColor )
            , _groundType( ground )
        {
            assert( expectedSize > 0 );

            _nodes.reserve( expectedSize );
            _nodes.emplace_back( mapIndex );
            _nodes[0].region = regionIndex;
        }
    };

    void checkAdjacentTiles( NodeCache & rawData, Region & region )
    {
        Node & previousNode = region._nodes[region._lastProcessedNode];
        const int nodeIndex = previousNode.index;

        for ( uint8_t direction = 0; direction < directionCount; ++direction ) {
            if ( region._nodes.size() > region._sizeLimit ) {
                previousNode.type = NodeType::BORDER;
                break;
            }

            // Check diagonal direction only 50% of the time to get more circular distribution.
            // It gives randomness for uneven edges.
            if ( direction > 3 && Rand::Get( 1 ) ) {
                break;
            }

            // TODO: use node index and pre-calculate offsets in advance.
            //       This will speed up the below calculations.
            const fheroes2::Point newPosition = Maps::GetPoint( nodeIndex );
            Node & newTile = rawData.getNode( newPosition + directionOffsets[direction] );
            if ( newTile.region == 0 && newTile.type == NodeType::OPEN ) {
                newTile.region = region._id;
                region._nodes.push_back( newTile );
            }
            else if ( newTile.region != region._id ) {
                previousNode.type = NodeType::BORDER;
                region._neighbours.insert( newTile.region );
            }
        }
    }

    void regionExpansion( NodeCache & rawData, Region & region )
    {
        // Process only "open" nodes that exist at the start of the loop and ignore what's added.
        const size_t nodesEnd = region._nodes.size();

        while ( region._lastProcessedNode < nodesEnd ) {
            checkAdjacentTiles( rawData, region );
            ++region._lastProcessedNode;
        }
    }

    bool canFitObject( NodeCache & data, const Maps::ObjectInfo & info, const fheroes2::Point & mainTilePos, const bool isAction )
    {
        fheroes2::Rect objectRect;

        for ( const auto & objectPart : info.groundLevelParts ) {
            if ( objectPart.layerType == Maps::SHADOW_LAYER || objectPart.layerType == Maps::TERRAIN_LAYER ) {
                // Shadow and terrain layer parts are ignored.
                continue;
            }

            const Node & node = data.getNode( mainTilePos + objectPart.tileOffset );

            if ( node.index == -1 || node.type != NodeType::OPEN ) {
                return false;
            }

            objectRect.x = std::min( objectRect.x, objectPart.tileOffset.x );
            objectRect.width = std::max( objectRect.width, objectPart.tileOffset.x );
        }

        for ( const auto & partInfo : info.topLevelParts ) {
            const Node & node = data.getNode( mainTilePos + partInfo.tileOffset );

            if ( node.index == -1 || node.type != NodeType::OPEN ) {
                return false;
            }

            objectRect.x = std::min( objectRect.x, partInfo.tileOffset.x );
            objectRect.width = std::max( objectRect.width, partInfo.tileOffset.x );
        }

        if ( isAction ) {
            for ( int x = objectRect.x - 1; x <= objectRect.width + 1; ++x ) {
                const Node & pathNode = data.getNode( mainTilePos + fheroes2::Point{ x, 1 } );
                if ( pathNode.index == -1 || pathNode.type == NodeType::OBSTACLE ) {
                    return false;
                }
                if ( pathNode.type == NodeType::BORDER ) {
                    return false;
                }
            }
        }

        return true;
    }

    void markObjectPlacement( NodeCache & data, const Maps::ObjectInfo & objectInfo, const fheroes2::Point & mainTilePos, const bool isAction )
    {
        fheroes2::Rect objectRect;

        for ( const auto & objectPart : objectInfo.groundLevelParts ) {
            if ( objectPart.layerType == Maps::SHADOW_LAYER || objectPart.layerType == Maps::TERRAIN_LAYER ) {
                // Shadow and terrain layer parts are ignored.
                continue;
            }

            Node & node = data.getNode( mainTilePos + objectPart.tileOffset );
            objectRect.x = std::min( objectRect.x, objectPart.tileOffset.x );
            objectRect.y = std::min( objectRect.y, objectPart.tileOffset.y );
            objectRect.width = std::max( objectRect.width, objectPart.tileOffset.x );
            objectRect.height = std::max( objectRect.height, objectPart.tileOffset.y );

            node.type = NodeType::OBSTACLE;
        }

        for ( const auto & partInfo : objectInfo.topLevelParts ) {
            objectRect.x = std::min( objectRect.x, partInfo.tileOffset.x );
            objectRect.width = std::max( objectRect.width, partInfo.tileOffset.x );
        }

        if ( isAction ) {
            for ( int x = objectRect.x - 1; x <= objectRect.width + 1; ++x ) {
                Node & pathNode = data.getNode( mainTilePos + fheroes2::Point{ x, objectRect.height + 1 } );
                pathNode.type = NodeType::PATH;
            }
            data.getNode( mainTilePos ).type = NodeType::ACTION;
        }
    }

    bool putObjectOnMap( Maps::Map_Format::MapFormat & mapFormat, Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex )
    {
        const auto & objectInfo = Maps::getObjectInfo( groupType, objectIndex );
        if ( objectInfo.empty() ) {
            assert( 0 );
            return false;
        }

        // Do not update passabilities after every object placement.
        if ( !Maps::setObjectOnTile( tile, objectInfo, false ) ) {
            assert( 0 );
            return false;
        }

        Maps::addObjectToMap( mapFormat, tile.GetIndex(), groupType, static_cast<uint32_t>( objectIndex ) );

        return true;
    }

    bool actionObjectPlacer( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t type )
    {
        const fheroes2::Point tilePos = tile.GetCenter();
        const auto & objectInfo = Maps::getObjectInfo( groupType, type );
        if ( canFitObject( data, objectInfo, tilePos, true ) && putObjectOnMap( mapFormat, tile, groupType, type ) ) {
            markObjectPlacement( data, objectInfo, tilePos, true );
            return true;
        }

        return false;
    }

    bool placeCastle( Interface::EditorInterface & interface, const int32_t mapWidth, NodeCache & data, const Region & region, const int targetX, const int targetY )
    {
        const int regionX = region._centerIndex % mapWidth;
        const int regionY = region._centerIndex / mapWidth;
        const int castleX = std::min( std::max( ( targetX + regionX ) / 2, 4 ), mapWidth - 4 );
        const int castleY = std::min( std::max( ( targetY + regionY ) / 2, 3 ), mapWidth - 3 );

        const auto & tile = world.getTile( castleX, castleY );
        if ( tile.isWater() ) {
            return false;
        }

        const fheroes2::Point tilePos( castleX, castleY );

        const int32_t basementId = fheroes2::getTownBasementId( tile.GetGround() );

        const auto & basementInfo = Maps::getObjectInfo( Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId );
        const auto & castleInfo = Maps::getObjectInfo( Maps::ObjectGroup::KINGDOM_TOWNS, randomCastleIndex );

        if ( canFitObject( data, basementInfo, tilePos, false ) && canFitObject( data, castleInfo, tilePos, true ) ) {
            const PlayerColor color = Color::IndexToColor( region._colorIndex );

            if ( interface.placeCastle( castleX, castleY, color, randomCastleIndex ) ) {
                markObjectPlacement( data, basementInfo, tilePos, false );
                markObjectPlacement( data, castleInfo, tilePos, true );

                return true;
            }
        }

        return false;
    }

    bool placeMine( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Region & region, const int resource )
    {
        const auto & node = Rand::Get( region._nodes );
        Maps::Tile & mineTile = world.getTile( node.index );
        const int32_t mineType = fheroes2::getMineObjectInfoId( resource, mineTile.GetGround() );
        return actionObjectPlacer( mapFormat, data, mineTile, Maps::ObjectGroup::ADVENTURE_MINES, mineType );
    }
}

namespace Maps::Random_Generator
{
    bool generateMap( Map_Format::MapFormat & mapFormat, const Configuration & config, const int32_t width, const int32_t height )
    {
        if ( config.playerCount < 2 || config.playerCount > 6 ) {
            assert( config.playerCount <= 6 );
            return false;
        }
        if ( config.regionSizeLimit < 200 ) {
            return false;
        }

        // Initialization step. Reset the current map in `world` and `mapFormat` containers first.
        Interface::EditorInterface & interface = Interface::EditorInterface::Get();
        if ( !interface.generateNewMap( width ) ) {
            return false;
        }

        NodeCache data( width, height );

        auto mapBoundsCheck = [width, height]( int x, int y ) {
            x = std::clamp( x, 0, width - 1 );
            y = std::clamp( y, 0, height - 1 );
            return x * width + y;
        };

        // Step 1. Setup map generator configuration.
        // TODO: Implement balanced set up only / Pyramid later.

        // Aiming for region size to be ~400 tiles in a 300-600 range.
        const int expectedRegionCount = ( width * height ) / config.regionSizeLimit;

        // Step 2. Determine region layout and placement.
        //         Insert empty region that represents water and map edges
        std::vector<Region> mapRegions = { { 0, 0, neutralColorIndex, Ground::WATER, 1 } };

        const int neutralRegionCount = std::max( 1, expectedRegionCount - config.playerCount );
        const int innerLayer = std::min( neutralRegionCount, config.playerCount );
        const int outerLayer = std::max( std::min( neutralRegionCount, innerLayer * 2 ), config.playerCount );

        const double radius = sqrt( ( innerLayer + outerLayer ) * config.regionSizeLimit / M_PI );
        const double outerRadius = ( ( innerLayer + outerLayer ) > expectedRegionCount ) ? std::max( width, height ) * 0.47 : radius * 0.85;
        const double innerRadius = ( innerLayer == 1 ) ? 0 : outerRadius / 3;

        const std::vector<std::pair<int, double>> mapLayers = { { innerLayer, innerRadius }, { outerLayer, outerRadius } };

        for ( size_t layer = 0; layer < mapLayers.size(); ++layer ) {
            const int regionCount = mapLayers[layer].first;
            const double startingAngle = Rand::Get( 360 );
            const double offsetAngle = 360.0 / regionCount;
            for ( int i = 0; i < regionCount; ++i ) {
                const double radians = ( startingAngle + offsetAngle * i ) * M_PI / 180;
                const double distance = mapLayers[layer].second;

                const int x = width / 2 + static_cast<int>( cos( radians ) * distance );
                const int y = height / 2 + static_cast<int>( sin( radians ) * distance );
                const int centerTile = mapBoundsCheck( x, y );

                const int factor = regionCount / config.playerCount;
                const bool isPlayerRegion = ( layer == 1 ) && ( ( i % factor ) == 0 );

                const int groundType = isPlayerRegion ? Rand::Get( playerStartingTerrain ) : Rand::Get( neutralTerrain );
                const int regionColor = isPlayerRegion ? i / factor : neutralColorIndex;

                const uint32_t regionID = static_cast<uint32_t>( mapRegions.size() );
                mapRegions.emplace_back( regionID, centerTile, regionColor, groundType, config.regionSizeLimit );
                data.getNode( centerTile ).region = regionID;
            }
        }

        // Step 3. Grow all regions one step at the time so they would compete for space.
        bool stillRoomToExpand = true;
        while ( stillRoomToExpand ) {
            stillRoomToExpand = false;
            // Skip the first region which is the border region.
            for ( size_t regionID = 1; regionID < mapRegions.size(); ++regionID ) {
                Region & region = mapRegions[regionID];
                regionExpansion( data, region );
                if ( region._lastProcessedNode != region._nodes.size() ) {
                    stillRoomToExpand = true;
                }
            }
        }

        // Step 4. Apply terrain changes into the map format.
        for ( const Region & region : mapRegions ) {
            if ( region._id == 0 ) {
                continue;
            }

            for ( const Node & node : region._nodes ) {
                Maps::setTerrainOnTile( mapFormat, node.index, region._groundType );
            }

            // Fix missing references.
            for ( const uint32_t adjacent : region._neighbours ) {
                mapRegions[adjacent]._neighbours.insert( region._id );
            }
        }

        // Step 5. Object placement
        for ( Region & region : mapRegions ) {
            if ( region._id == 0 ) {
                // Skip the first region as we have nothing to do here for now.
                continue;
            }

            DEBUG_LOG( DBG_ENGINE, DBG_TRACE,
                       "Region #" << region._id << " of size " << region._nodes.size() << " tiles has " << region._neighbours.size() << " neighbours" )

            int xMin = 0;
            int xMax = width;
            int yMin = 0;
            int yMax = height;

            for ( const Node & node : region._nodes ) {
                const int nodeX = node.index % width;
                const int nodeY = node.index / width;
                xMin = std::max( xMin, nodeX );
                xMax = std::min( xMax, nodeX );
                yMin = std::max( yMin, nodeY );
                yMax = std::min( yMax, nodeY );

                if ( node.type == NodeType::BORDER ) {
                    Maps::setTerrainWithTransition( mapFormat, node.index, node.index, region._groundType );
                }
            }

            if ( region._colorIndex != neutralColorIndex && !placeCastle( interface, mapFormat.width, data, region, ( xMin + xMax ) / 2, ( yMin + yMax ) / 2 ) ) {
                // Return early if we can't place a starting player castle.
                return false;
            }

            if ( region._nodes.size() > 400 ) {
                // Place non-mandatory castles in bigger neutral regions.
                placeCastle( interface, mapFormat.width, data, region, ( xMin + xMax ) / 2, ( yMin + yMax ) / 2 );
            }

            if ( config.basicOnly ) {
                continue;
            }

            for ( const int resource : resources ) {
                // TODO: do a gradual distribution instead of guesses.
                for ( int tries = 0; tries < 5; ++tries ) {
                    if ( placeMine( mapFormat, data, region, resource ) ) {
                        break;
                    }
                }
            }

            Maps::updateRoadOnTile( mapFormat, region._centerIndex, true );
        }

        // TODO: set up region connectors based on frequency settings and border length.

        // TODO: generate road based paths.

        // TODO: place objects while avoiding the borders.
        //       Make sure that objects are accessible.
        //       Also, make sure that paths are accessible. Possibly delete obstacles.

        // TODO: place treasure objects.

        // TODO: place monsters.

        return true;
    }
}
