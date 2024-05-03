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
    // ObjectInfo ObjctGroup based indicies do not match old objects
    const int NEUTRAL_COLOR = 6;
    const int RANDOM_CASTLE_INDEX = 12;
    const std::vector<int> playerStartingTerrain = { Ground::GRASS, Ground::DIRT, Ground::SNOW, Ground::LAVA, Ground::WASTELAND };
    const std::vector<int> neutralTerrain = { Ground::GRASS, Ground::DIRT, Ground::SNOW, Ground::LAVA, Ground::WASTELAND, Ground::BEACH, Ground::SWAMP, Ground::DESERT };

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

    enum class NodeType
    {
        OPEN,
        BORDER,
        ACTION,
        OBSTACLE,
        PATH
    };

    struct Node
    {
        int index = -1;
        NodeType type = NodeType::BORDER;
        uint32_t region = 0;
        uint16_t mapObject = 0;
        uint16_t passable = DIRECTION_ALL;

        Node() = default;
        explicit Node( int index_ )
            : index( index_ )
        {}
    };

    struct Region
    {
    public:
        uint32_t _id = 0;
        int32_t _centerIndex = -1;
        std::set<uint32_t> _neighbours;
        std::vector<Node> _nodes;
        size_t _sizeLimit;
        size_t _lastProcessedNode = 0;
        int _colorIndex = NEUTRAL_COLOR;
        int _groundType = Ground::GRASS;

        Region() = default;

        Region( uint32_t regionIndex, int32_t mapIndex, int playerColor, int ground, size_t expectedSize )
            : _id( regionIndex )
            , _sizeLimit( expectedSize )
            , _centerIndex( mapIndex )
            , _colorIndex( playerColor )
            , _groundType( ground )
        {
            _nodes.reserve( expectedSize );
            _nodes.emplace_back( mapIndex );
            _nodes[0].region = regionIndex;
        }

        size_t getNeighboursCount() const
        {
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
        Node & previousNode = region._nodes[region._lastProcessedNode];
        const int nodeIndex = previousNode.index;

        for ( uint8_t direction = 0; direction < 8; ++direction ) {
            if ( region._nodes.size() > region._sizeLimit ) {
                previousNode.type = NodeType::BORDER;
                break;
            }

            // only check diagonal 50% of the time to get more circular distribution; randomness for uneven edges
            if ( direction > 3 && Rand::Get( 1 ) ) {
                break;
            }
            const int newIndex = ConvertExtendedIndex( nodeIndex, rawDataWidth ) + offsets[direction];
            Node & newTile = rawData[newIndex];
            if ( newTile.passable & GetDirectionBitmask( direction, true ) ) {
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

    bool setObjectOnTile( Map_Format::MapFormat & mapFormat, Tiles & tile, const ObjectGroup groupType, const int32_t objectIndex )
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

    bool entranceCheck( const fheroes2::Point tilePos )
    {
        for ( int i = -2; i < 3; i++ ) {
            if ( !Maps::isValidAbsPoint( tilePos.x + i, tilePos.y + 1 ) || !Maps::isClearGround( world.GetTiles( tilePos.x + i, tilePos.y + 1 ) ) ) {
                return false;
            }
        }
        return true;
    }

    bool objectPlacer( Map_Format::MapFormat & mapFormat, Tiles & tile, ObjectGroup groupType, int32_t type )
    {
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
    }

    bool placeCastle( Map_Format::MapFormat & mapFormat, Region & region, int targetX, int targetY )
    {
        const int regionX = region._centerIndex % mapFormat.size;
        const int regionY = region._centerIndex / mapFormat.size;
        const int castleX = std::min( std::max( ( targetX + regionX ) / 2, 4 ), mapFormat.size - 4 );
        const int castleY = std::min( std::max( ( targetY + regionY ) / 2, 3 ), mapFormat.size - 3 );

        auto & tile = world.GetTiles( castleY * mapFormat.size + castleX );
        fheroes2::Point tilePos = tile.GetCenter();

        const int32_t basementId = fheroes2::getTownBasementId( tile.GetGround() );

        const auto & basementInfo = Maps::getObjectInfo( Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId );
        const auto & castleInfo = Maps::getObjectInfo( Maps::ObjectGroup::KINGDOM_TOWNS, RANDOM_CASTLE_INDEX );

        if ( isObjectPlacementAllowed( basementInfo, tilePos ) && isObjectPlacementAllowed( castleInfo, tilePos ) && isActionObjectAllowed( castleInfo, tilePos ) ) {
            setObjectOnTile( mapFormat, tile, Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId );

            assert( Maps::getLastObjectUID() > 0 );
            const uint32_t objectId = Maps::getLastObjectUID() - 1;

            Maps::setLastObjectUID( objectId );
            setObjectOnTile( mapFormat, tile, Maps::ObjectGroup::KINGDOM_TOWNS, 12 );

            const uint8_t color = Color::IndexToColor( region._colorIndex );
            // By default use random (default) army for the neutral race town/castle.
            if ( color == Color::NONE ) {
                Maps::setDefaultCastleDefenderArmy( mapFormat.castleMetadata[Maps::getLastObjectUID()] );
            }

            // Add flags.
            assert( tile.GetIndex() > 0 && tile.GetIndex() < world.w() * world.h() - 1 );
            Maps::setLastObjectUID( objectId );

            if ( !setObjectOnTile( mapFormat, world.GetTiles( tile.GetIndex() - 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, region._colorIndex * 2 ) ) {
                return false;
            }

            Maps::setLastObjectUID( objectId );

            if ( !setObjectOnTile( mapFormat, world.GetTiles( tile.GetIndex() + 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, region._colorIndex * 2 + 1 ) ) {
                return false;
            }

            world.addCastle( tile.GetIndex(), Race::IndexToRace( RANDOM_CASTLE_INDEX ), color );
        }
        return true;
    }

    bool placeMine( Map_Format::MapFormat & mapFormat, Region & region, const int resource )
    {
        const auto & node = Rand::Get( region._nodes );
        Maps::Tiles & mineTile = world.GetTiles( node.index );
        const int32_t mineType = fheroes2::getMineObjectInfoId( resource, mineTile.GetGround() );
        if ( node.type == NodeType::OPEN && objectPlacer( mapFormat, mineTile, Maps::ObjectGroup::ADVENTURE_MINES, mineType ) ) {
            return true;
        }
        return false;
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

        auto mapBoundsCheck = [width, height]( int x, int y ) {
            x = std::max( std::min( x, width - 1 ), 0 );
            y = std::max( std::min( y, height - 1 ), 0 );
            return x * width + y;
        };

        // Step 1. Map generator configuration
        // TODO: Balanced set up only / Pyramid later
        const int playerCount = config.playerCount;

        // Aiming for region size to be ~300 tiles in a 200-500 range
        const int minimumRegionCount = playerCount + 1;
        const int expectedRegionCount = ( width * height ) / config.regionSizeLimit;

        const uint32_t extendedWidth = width + 2;
        std::vector<Node> data( extendedWidth * ( height + 2 ) );
        for ( int y = 0; y < height; ++y ) {
            const int rowIndex = y * width;
            for ( int x = 0; x < width; ++x ) {
                const int index = rowIndex + x;
                Node & node = data[ConvertExtendedIndex( index, extendedWidth )];

                node.index = index;
                node.type = NodeType::OPEN;
            }
        }

        // Step 2. Determine region layout and placement
        // Insert empty region that represents water and map edges
        std::vector<Region> mapRegions = { { 0, 0, NEUTRAL_COLOR, Ground::WATER, 0 } };

        const int neutralRegionCount = std::max( 1, expectedRegionCount - playerCount );
        const int innerLayer = std::min( neutralRegionCount, playerCount );
        const int outerLayer = std::max( std::min( neutralRegionCount, innerLayer * 2 ), playerCount );

        const double radius = sqrt( ( innerLayer + outerLayer ) * config.regionSizeLimit / M_PI );
        const double outerRadius = ( ( innerLayer + outerLayer ) > expectedRegionCount ) ? std::max( width, height ) * 0.47 : radius * 0.85;
        const double innerRadius = innerLayer == 1 ? 0 : outerRadius / 3;

        const std::vector<std::pair<int, double>> mapLayers = { { innerLayer, innerRadius }, { outerLayer, outerRadius } };

        for ( int layer = 0; layer < mapLayers.size(); layer++ ) {
            const int regionCount = mapLayers[layer].first;
            const double startingAngle = Rand::Get( 360 );
            const double offsetAngle = 360.0 / regionCount;
            for ( int i = 0; i < regionCount; i++ ) {
                const double radians = ( startingAngle + offsetAngle * i ) * M_PI / 180;
                const double distance = mapLayers[layer].second;

                const int x = width / 2 + static_cast<int>( cos( radians ) * distance );
                const int y = height / 2 + static_cast<int>( sin( radians ) * distance );
                const int centerTile = mapBoundsCheck( x, y );

                const int factor = regionCount / playerCount;
                const bool isPlayerRegion = layer == 1 && ( i % factor ) == 0;

                const int groundType = isPlayerRegion ? Rand::Get( playerStartingTerrain ) : Rand::Get( neutralTerrain );
                const int regionColor = isPlayerRegion ? i / factor : NEUTRAL_COLOR;

                const uint32_t regionID = static_cast<uint32_t>( mapRegions.size() );
                mapRegions.emplace_back( regionID, centerTile, regionColor, groundType, config.regionSizeLimit );
                data[ConvertExtendedIndex( centerTile, extendedWidth )].region = regionID;
            }
        }

        // Step 3. Grow all regions one step at the time so they would compete for space
        const std::vector<int> & offsets = GetDirectionOffsets( static_cast<int>( extendedWidth ) );
        bool stillRoomToExpand = true;
        while ( stillRoomToExpand ) {
            stillRoomToExpand = false;
            // Skip the border region
            for ( size_t regionID = 1; regionID < mapRegions.size(); ++regionID ) {
                Region & region = mapRegions[regionID];
                RegionExpansion( data, extendedWidth, region, offsets );
                if ( region._lastProcessedNode != region._nodes.size() )
                    stillRoomToExpand = true;
            }
        }

        // Step 4. We're ready to save the result; reset the current world first
        world.generateForEditor( width );

        for ( Region & region : mapRegions ) {
            if ( region._id == 0 )
                continue;

            for ( const Node & node : region._nodes ) {
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
                    region._neighbours.insert( node.region );
                }
                world.GetTiles( node.index ).setTerrain( Maps::Ground::getRandomTerrainImageIndex( region._groundType, true ), false, false );
            }

            // Fix missing references
            for ( uint32_t adjacent : region._neighbours ) {
                mapRegions[adjacent]._neighbours.insert( region._id );
            }
        }

        // Step 5. Object placement
        for ( Region & region : mapRegions ) {
            if ( region._id == 0 )
                continue;

            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "Region #" << region._id << " size " << region._nodes.size() << " has " << region._neighbours.size() << "neighbours" )

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
                    Maps::setTerrainOnTiles( node.index, node.index, region._groundType );
                }
            }

            if ( region._colorIndex != NEUTRAL_COLOR && !placeCastle( mapFormat, region, ( xMin + xMax ) / 2, ( yMin + yMax ) / 2 ) ) {
                // return early if we can't place a starting player castle
                return false;
            }
            else if ( region._nodes.size() > 300 ) {
                // place non-mandatory castles in bigger neutral regions
                placeCastle( mapFormat, region, ( xMin + xMax ) / 2, ( yMin + yMax ) / 2 );
            }

            if ( config.basicOnly ) {
                continue;
            }

            const std::vector<int> resoures = { Resource::WOOD, Resource::ORE, Resource::CRYSTAL, Resource::SULFUR, Resource::GEMS, Resource::MERCURY, Resource::GOLD };
            for ( const int resource : resoures ) {
                // TODO: do a gradual distribution instead of guesses
                for ( int tries = 0; tries < 5; tries++ ) {
                    if ( placeMine( mapFormat, region, resource ) ) {
                        break;
                    }
                }
            }

            Maps::updateRoadOnTile( world.GetTiles( region._centerIndex ), true );
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