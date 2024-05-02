/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                             *
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

#include "map_generator.h"

#include "logging.h"
#include "map_format_helper.h"
#include "map_object_info.h"
#include "maps_tiles_helper.h"
#include "race.h"
#include "rand.h"
#include "ui_map_object.h"
#include "world.h"
#include "world_object_uid.h"

namespace Maps::Generator
{
    enum
    {
        TOP = 0,
        RIGHT = 1,
        BOTTOM = 2,
        LEFT = 3,
        TOP_LEFT = 4,
        TOP_RIGHT = 5,
        BOTTOM_RIGHT = 6,
        BOTTOM_LEFT = 7,
    };

    enum
    {
        REGION_NODE_BLOCKED = 0,
        REGION_NODE_OPEN = 1,
        REGION_NODE_BORDER = 2,
        REGION_NODE_FOUND = 3
    };

    struct Node
    {
        int index = -1;
        uint32_t type = REGION_NODE_BLOCKED;
        uint16_t mapObject = 0;
        uint16_t passable = 0;
        bool isWater = false;

        Node() = default;
        explicit Node( int index_ )
            : index( index_ )
            , type( REGION_NODE_OPEN )
        {}
    };

    struct Region
    {
    public:
        uint32_t _id = REGION_NODE_FOUND;
        bool _isWater = false;
        std::set<uint32_t> _neighbours;
        std::vector<Node> _nodes;
        size_t _lastProcessedNode = 0;

        Region() = default;

        Region(int regionIndex, int mapIndex, bool water, size_t expectedSize)
            : _id(regionIndex)
            , _isWater(water)
        {
            _nodes.reserve( expectedSize );
            _nodes.emplace_back( mapIndex );
            _nodes[0].type = regionIndex;
        }

        size_t getNeighboursCount() const {
            return _neighbours.size();
        }
    };

    std::vector<int> GetDirectionOffsets( const int width )
    {
        std::vector<int> offsets( 8 );
        offsets[TOP] = -width;
        offsets[RIGHT] = 1;
        offsets[BOTTOM] = width;
        offsets[LEFT] = -1;
        offsets[TOP_LEFT] = -width - 1;
        offsets[TOP_RIGHT] = -width + 1;
        offsets[BOTTOM_RIGHT] = width + 1;
        offsets[BOTTOM_LEFT] = width - 1;
        return offsets;
    }

    uint16_t GetDirectionBitmask( uint8_t direction, bool reflect = false )
    {
        return 1 << ( reflect ? ( direction + 4 ) % 8 : direction );
    }

    int ConvertExtendedIndex( int index, uint32_t width )
    {
        const uint32_t originalWidth = width - 2;
        return ( index / originalWidth + 1 ) * width + ( index % originalWidth ) + 1;
    }

    void CheckAdjacentTiles( std::vector<Node> & rawData, Region & region, uint32_t rawDataWidth, const std::vector<int> & offsets )
    {
        const int nodeIndex = region._nodes[region._lastProcessedNode].index;

        for ( uint8_t direction = 0; direction < 8; ++direction ) {
            if ( direction > 3 && Rand::Get( 1 ) ) {
                break;
            }
            const int newIndex = ConvertExtendedIndex( nodeIndex, rawDataWidth ) + offsets[direction];
            Node & newTile = rawData[newIndex];
            if ( newTile.passable & GetDirectionBitmask( direction, true ) && newTile.isWater == region._isWater ) {
                if ( newTile.type == REGION_NODE_OPEN ) {
                    newTile.type = region._id;
                    region._nodes.push_back( newTile );
                }
                else if ( newTile.type >= REGION_NODE_BORDER && newTile.type != region._id ) {
                    region._nodes[region._lastProcessedNode].type = REGION_NODE_BORDER;
                    region._neighbours.insert( newTile.type );
                }
            }
        }
    }

    void RegionExpansion( std::vector<Node> & rawData, uint32_t rawDataWidth, Region & region, const std::vector<int> & offsets )
    {
        // Process only "open" nodes that exist at the start of the loop and ignore what's added
        const size_t nodesEnd = region._nodes.size();

        while ( region._lastProcessedNode < nodesEnd ) {
            CheckAdjacentTiles( rawData, region, rawDataWidth, offsets );
            ++region._lastProcessedNode;
        }
    }

    bool setObjectOnTile( Maps::Map_Format::MapFormat & mapFormat, Maps::Tiles & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex )
    {
        const auto & objectInfo = Maps::getObjectInfo( groupType, objectIndex );
        if ( objectInfo.empty() ) {
            // Check your logic as you are trying to insert an empty object!
            assert( 0 );
            return false;
        }

        if ( !Maps::setObjectOnTile( tile, objectInfo, true ) ) {
            return false;
        }

        Maps::addObjectToMap( mapFormat, tile.GetIndex(), groupType, static_cast<uint32_t>( objectIndex ) );

        return true;
    }

    bool generateWorld( Map_Format::MapFormat & mapFormat, Configuration config )
    {
        if ( config.playerCount < 2 || config.playerCount > 6 ) {
            return false;
        }
        if ( config.regionSizeLimit < 100 ) {
            return false;
        }

        const int32_t width = world.w();
        const int32_t height = world.h();

        // Step 1. Map generator configuration
        // TODO: Balanced set up only / Pyramid later
        const int playerCount = config.playerCount;

        // Aiming for region size to be ~300 tiles in a 200-500 range
        const int minimumRegionCount = playerCount + 1;
        const int expectedRegionCount = ( width * height ) / config.regionSizeLimit;
        // const double regionSize = ( playerArea > 500 ) ? playerArea / 2 : playerArea;
        // const int expectedRegionCount = static_cast<int> (( width * height ) / regionSize);

        // const double radius = sqrt( playerArea / M_PI );
        // const int side = static_cast<int>( radius / sqrt( 2 ) );

        auto mapBoundsCheck = [width, height]( int x, int y ) {
            x = std::max( std::min( x, width - 1 ), 0 );
            y = std::max( std::min( y, height - 1 ), 0 );
            return x * width + y;
        };

        // Step 2. Determine region layout and placement
        std::vector<int> regionCenters;

        const int neutralRegionCount = std::max( 1, expectedRegionCount - playerCount );
        const int innerLayer = std::min( neutralRegionCount, playerCount );
        const int outerLayer = std::max( std::min( neutralRegionCount, innerLayer * 2 ), playerCount );

        const double outerRadius = 0.2 + ( innerLayer + outerLayer ) / static_cast<double>( expectedRegionCount );
        const double innerRadius = innerLayer == 1 ? 0 : outerRadius / 3;

        const std::vector<std::pair<int, double>> mapLayers = { { innerLayer, innerRadius }, { outerLayer, outerRadius } };

        const double distance = std::max( width, height ) / 2.0;
        for ( const auto layer : mapLayers ) {
            const double startingAngle = Rand::Get( 360 );
            const double offsetAngle = 360.0 / layer.first;
            for ( int i = 0; i < layer.first; i++ ) {
                const double radians = ( startingAngle + offsetAngle * i ) * M_PI / 180;

                const int x = width / 2 + static_cast<int>( cos( radians ) * distance * layer.second );
                const int y = height / 2 + static_cast<int>( sin( radians ) * distance * layer.second );
                regionCenters.push_back( mapBoundsCheck( x, y ) );
            }
        }

        const uint32_t extendedWidth = width + 2;
        std::vector<Node> data( extendedWidth * ( height + 2 ) );
        for ( int y = 0; y < height; ++y ) {
            const int rowIndex = y * width;
            for ( int x = 0; x < width; ++x ) {
                const int index = rowIndex + x;
                Node & node = data[ConvertExtendedIndex( index, extendedWidth )];

                node.index = index;
                node.passable = DIRECTION_ALL;
                node.isWater = false;
                node.type = REGION_NODE_OPEN;
                node.mapObject = 0;
            }
        }

        size_t averageRegionSize = ( static_cast<size_t>( width ) * height * 2 ) / regionCenters.size();
        std::vector<Region> mapRegions = { { REGION_NODE_BLOCKED, 0, false, 0 }, { REGION_NODE_OPEN, 0, false, 0 }, { REGION_NODE_BORDER, 0, false, 0 } };

        for ( const int tileIndex : regionCenters ) {
            const int regionID = static_cast<int>( mapRegions.size() ); // Safe to do as we can't have so many regions
            mapRegions.emplace_back( regionID, tileIndex, false, averageRegionSize );
            data[ConvertExtendedIndex( tileIndex, extendedWidth )].type = regionID;
        }

        // Step 3. Grow all regions one step at the time so they would compete for space
        const std::vector<int> & offsets = GetDirectionOffsets( static_cast<int>( extendedWidth ) );
        bool stillRoomToExpand = true;
        while ( stillRoomToExpand ) {
            stillRoomToExpand = false;
            for ( size_t regionID = REGION_NODE_FOUND; regionID < mapRegions.size(); ++regionID ) {
                Region & region = mapRegions[regionID];
                RegionExpansion( data, extendedWidth, region, offsets );
                if ( region._lastProcessedNode != region._nodes.size() && region._nodes.size() < config.regionSizeLimit )
                    stillRoomToExpand = true;
            }
        }

        auto entranceCheck = []( const fheroes2::Point tilePos ) {
            for ( int i = -2; i < 3; i++ ) {
                if ( !Maps::isValidAbsPoint( tilePos.x + i, tilePos.y + 1 ) || !Maps::isClearGround( world.GetTiles( tilePos.x + i, tilePos.y + 1 ) ) ) {
                    return false;
                }
            }
            return true;
        };

        auto objectPlacer = [&mapFormat, entranceCheck]( Maps::Tiles & tile, Maps::ObjectGroup groupType, int32_t type ) {
            const fheroes2::Point tilePos = tile.GetCenter();
            const auto & objectInfo = Maps::getObjectInfo( groupType, type );
            if ( isObjectPlacementAllowed( objectInfo, tilePos ) && isActionObjectAllowed( objectInfo, tilePos ) && entranceCheck( tilePos ) ) {
                // do not update passabilities after every object
                if ( !Maps::setObjectOnTile( tile, objectInfo, true ) ) {
                    return false;
                }

                Maps::addObjectToMap( mapFormat, tile.GetIndex(), groupType, static_cast<uint32_t>( type ) );
                return true;
            }
            return false;
        };

        // Step 4. Reset world first
        world.generateForEditor( width );

        std::vector<int> cache( width * height );
        for ( Region & reg : mapRegions ) {
            if ( reg._id < REGION_NODE_FOUND )
                continue;

            const int terrainType = 1 << ( reg._id % 8 );
            for ( const Node & node : reg._nodes ) {
                if ( cache[node.index] >= REGION_NODE_FOUND ) {
                    break;
                }
                cache[node.index] = node.type;

                // connect regions through teleports
                MapsIndexes exits;

                if ( node.mapObject == MP2::OBJ_STONE_LITHS ) {
                    exits = world.GetTeleportEndPoints( node.index );
                }
                else if ( node.mapObject == MP2::OBJ_WHIRLPOOL ) {
                    exits = world.GetWhirlpoolEndPoints( node.index );
                }

                for ( const int exitIndex : exits ) {
                    // neighbours is a set that will force the uniqueness
                    reg._neighbours.insert( cache[exitIndex] );
                }
                world.GetTiles( node.index ).setTerrain( Maps::Ground::getRandomTerrainImageIndex( terrainType, true ), false, false );
            }

            // Fix missing references
            for ( uint32_t adjacent : reg._neighbours ) {
                mapRegions[adjacent]._neighbours.insert( reg._id );
            }
        }

        for ( Region & reg : mapRegions ) {
            if ( reg._id < REGION_NODE_FOUND )
                continue;

            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "Region #" << reg._id << " size " << reg._nodes.size() << " has " << reg._neighbours.size() << "neighbours" )

            int xMin = 0;
            int xMax = width;
            int yMin = 0;
            int yMax = height;

            const int terrainType = 1 << ( reg._id % 8 );
            for ( const Node & node : reg._nodes ) {
                const int nodeX = node.index % width;
                const int nodeY = node.index / width;
                xMin = std::max( xMin, nodeX );
                xMax = std::min( xMax, nodeX );
                yMin = std::max( yMin, nodeY );
                yMax = std::min( yMax, nodeY );

                if ( node.type == REGION_NODE_BORDER ) {
                    Maps::setTerrainOnTiles( node.index, node.index, terrainType );
                }
            }

            if ( config.terrainOnly ) {
                continue;
            }

            const int regionX = regionCenters[reg._id - 3] % width;
            const int regionY = regionCenters[reg._id - 3] / width;
            const int castleX = std::min( std::max( ( ( xMin + xMax ) / 2 + regionX ) / 2, 4 ), width - 4 );
            const int castleY = std::min( std::max( ( ( yMin + yMax ) / 2 + regionY ) / 2, 3 ), height - 3 );
            const int color = reg._id % 6;

            auto & tile = world.GetTiles( castleY * width + castleX );
            fheroes2::Point tilePos = tile.GetCenter();

            const int32_t basementId = fheroes2::getTownBasementId( tile.GetGround() );

            const auto & basementInfo = Maps::getObjectInfo( Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId );
            const auto & castleInfo = Maps::getObjectInfo( Maps::ObjectGroup::KINGDOM_TOWNS, 12 );

            if ( isObjectPlacementAllowed( basementInfo, tilePos ) && isObjectPlacementAllowed( castleInfo, tilePos ) && isActionObjectAllowed( castleInfo, tilePos ) ) {
                setObjectOnTile( mapFormat, tile, Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId );

                assert( Maps::getLastObjectUID() > 0 );
                const uint32_t objectId = Maps::getLastObjectUID() - 1;

                Maps::setLastObjectUID( objectId );
                setObjectOnTile( mapFormat, tile, Maps::ObjectGroup::KINGDOM_TOWNS, 12 );

                // By default use random (default) army for the neutral race town/castle.
                if ( Color::IndexToColor( color ) == Color::NONE ) {
                    Maps::setDefaultCastleDefenderArmy( mapFormat.castleMetadata[Maps::getLastObjectUID()] );
                }

                // Add flags.
                assert( tile.GetIndex() > 0 && tile.GetIndex() < world.w() * world.h() - 1 );
                Maps::setLastObjectUID( objectId );

                if ( !setObjectOnTile( mapFormat, world.GetTiles( tile.GetIndex() - 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, color * 2 ) ) {
                    return false;
                }

                Maps::setLastObjectUID( objectId );

                if ( !setObjectOnTile( mapFormat, world.GetTiles( tile.GetIndex() + 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, color * 2 + 1 ) ) {
                    return false;
                }

                world.addCastle( tile.GetIndex(), Race::IndexToRace( 12 ), Color::IndexToColor( color ) );
            }

            const std::vector<int> resoures = { Resource::WOOD, Resource::ORE, Resource::CRYSTAL, Resource::SULFUR, Resource::GEMS, Resource::MERCURY, Resource::GOLD };
            for ( const int resource : resoures ) {
                for ( int tries = 0; tries < 5; tries++ ) {
                    const auto & node = Rand::Get( reg._nodes );
                    Maps::Tiles & mineTile = world.GetTiles( node.index );
                    const int32_t mineType = fheroes2::getMineObjectInfoId( resource, mineTile.GetGround() );
                    if ( node.type != REGION_NODE_BORDER && objectPlacer( mineTile, Maps::ObjectGroup::ADVENTURE_MINES, mineType ) ) {
                        break;
                    }
                }
            }
        }

        if ( config.terrainOnly ) {
            return true;
        }

        for ( const int tileIndex : regionCenters ) {
            auto & tile = world.GetTiles( tileIndex );
            Maps::updateRoadOnTile( tile, true );
        }

        // set up region connectors based on frequency settings & border length
        // generate road based paths
        // place objects avoiding the borders
        //
        // make sure objects accessible before
        // make sure paths are accessible - delete obstacles
        // place treasures
        // place monsters
        return true;
    }
}