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
#include <functional>
#include <initializer_list>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "direction.h"
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
#include "mp2.h"
#include "race.h"
#include "rand.h"
#include "resource.h"
#include "route.h"
#include "skill.h"
#include "ui_map_object.h"
#include "world.h"
#include "world_object_uid.h"
#include "world_pathfinding.h"

namespace
{
    const int neutralColorIndex{ Color::GetIndex( PlayerColor::UNUSED ) };
    constexpr int32_t smallestStartingRegionSize{ 200 };
    constexpr int32_t emptySpacePercentage{ 40 };
    constexpr int randomCastleIndex{ 12 };
    constexpr int randomTownIndex{ 13 };
    constexpr int randomMonsterIndex{ 68 };
    constexpr int maxPlacementAttempts{ 30 };
    const std::vector<int> playerStartingTerrain = { Maps::Ground::GRASS, Maps::Ground::DIRT, Maps::Ground::SNOW, Maps::Ground::LAVA, Maps::Ground::WASTELAND };
    const std::vector<int> neutralTerrain = { Maps::Ground::GRASS,     Maps::Ground::DIRT,  Maps::Ground::SNOW,  Maps::Ground::LAVA,
                                              Maps::Ground::WASTELAND, Maps::Ground::BEACH, Maps::Ground::SWAMP, Maps::Ground::DESERT };

    constexpr uint8_t directionCount{ 8 };
    const std::array<fheroes2::Point, directionCount> directionOffsets{ { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }, { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } } };
    constexpr std::array<int, 4> secondaryResources = { Resource::CRYSTAL, Resource::SULFUR, Resource::GEMS, Resource::MERCURY };

    // TODO: replace list of object indicies with a struct for a better variety
    const std::map<int, std::vector<int>> obstaclesPerGround = {
        { Maps::Ground::DESERT, { 24, 25, 26, 27, 28, 29 } },    { Maps::Ground::SNOW, { 30, 31, 32, 33, 34, 35 } },  { Maps::Ground::SWAMP, { 18, 19, 20, 21, 22, 23 } },
        { Maps::Ground::WASTELAND, { 12, 13, 14, 15, 16, 17 } }, { Maps::Ground::BEACH, { 24, 25, 26, 27, 28, 29 } }, { Maps::Ground::LAVA, { 6, 7, 8, 9, 10, 11 } },
        { Maps::Ground::DIRT, { 18, 19, 20, 21, 22, 23 } },      { Maps::Ground::GRASS, { 0, 1, 2, 3, 4, 5 } },       { Maps::Ground::WATER, {} },
    };

    enum class NodeType : uint8_t
    {
        OPEN,
        BORDER,
        ACTION,
        OBSTACLE,
        CONNECTOR,
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
                // We shouldn't try to get a tile with an invalid index.
                // TODO: here we must add an assertion and make sure that we never reach this place.
                return _outOfBounds;
            }

            return _data[position.y * _mapSize + position.x];
        }

        const Node & getNode( const fheroes2::Point position ) const
        {
            if ( position.x < 0 || position.x >= _mapSize || position.y < 0 || position.y >= _mapSize ) {
                // We shouldn't try to get a tile with an invalid index.
                // However, since we are accessing const value it is fine to get an invalid item.
                return _outOfBounds;
            }

            return _data[position.y * _mapSize + position.x];
        }

        Node & getNode( const int32_t index )
        {
            if ( index < 0 || index >= _mapSize * _mapSize ) {
                // We shouldn't try to get a tile with an invalid index.
                assert( 0 );
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
        uint32_t id{ 0 };
        int32_t centerIndex{ -1 };
        std::set<uint32_t> neighbours;
        std::vector<std::reference_wrapper<Node>> nodes;
        std::map<uint32_t, int32_t> connections;
        size_t sizeLimit{ 0 };
        size_t lastProcessedNode{ 0 };
        int colorIndex{ neutralColorIndex };
        int groundType{ Maps::Ground::GRASS };

        Region() = default;

        Region( const uint32_t regionIndex, Node & centerNode, const int playerColor, const int ground, const size_t expectedSize )
            : id( regionIndex )
            , centerIndex( centerNode.index )
            , sizeLimit( expectedSize )
            , colorIndex( playerColor )
            , groundType( ground )
        {
            assert( expectedSize > 0 );

            centerNode.region = regionIndex;
            nodes.reserve( expectedSize );
            nodes.emplace_back( centerNode );
        }
    };

    struct ObjectPlacement final
    {
        fheroes2::Point offset;
        Maps::ObjectGroup groupType{ Maps::ObjectGroup::NONE };
        int32_t objectIndex{ 0 };
    };

    struct ObjectSet final
    {
        std::vector<ObjectPlacement> obstacles;
        std::vector<ObjectPlacement> valuables;
        std::vector<ObjectPlacement> monsters;
        std::vector<fheroes2::Point> entranceCheck;
    };

    const std::vector<ObjectSet> prefabObjectSets
        = { { ObjectSet{
                  { ObjectPlacement{ { 0, -1 }, Maps::ObjectGroup::LANDSCAPE_TREES, 3 }, ObjectPlacement{ { 3, -1 }, Maps::ObjectGroup::LANDSCAPE_TREES, 2 },
                    ObjectPlacement{ { 3, 1 }, Maps::ObjectGroup::LANDSCAPE_TREES, 3 } }, // obstacles
                  { ObjectPlacement{ { 1, -1 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 }, ObjectPlacement{ { 2, -1 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 },
                    ObjectPlacement{ { 1, 0 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 },
                    ObjectPlacement{ { 2, 0 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 } }, // valuables
                  { ObjectPlacement{ { 0, 0 }, Maps::ObjectGroup::MONSTERS, randomMonsterIndex } }, // monsters
                  { { -1, 0 }, { -1, 1 }, { 0, 1 } } // entranceCheck
              },
              ObjectSet{
                  { ObjectPlacement{ { -3, 0 }, Maps::ObjectGroup::LANDSCAPE_TREES, 3 }, ObjectPlacement{ { -3, 1 }, Maps::ObjectGroup::LANDSCAPE_TREES, 0 },
                    ObjectPlacement{ { 0, 2 }, Maps::ObjectGroup::LANDSCAPE_TREES, 3 } }, // obstacles
                  { ObjectPlacement{ { -2, 0 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 }, ObjectPlacement{ { -1, 0 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 },
                    ObjectPlacement{ { -2, 1 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 },
                    ObjectPlacement{ { -1, 1 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 } }, // valuables
                  { ObjectPlacement{ { 0, 0 }, Maps::ObjectGroup::MONSTERS, randomMonsterIndex } }, // monsters
                  { { 0, -1 }, { 1, -1 }, { 1, 0 } } // entranceCheck
              },
              ObjectSet{
                  { ObjectPlacement{ { 3, -1 }, Maps::ObjectGroup::LANDSCAPE_TREES, 0 }, ObjectPlacement{ { 1, 2 }, Maps::ObjectGroup::LANDSCAPE_TREES, 5 },
                    ObjectPlacement{ { 3, 2 }, Maps::ObjectGroup::LANDSCAPE_TREES, 3 } }, // obstacles
                  { ObjectPlacement{ { 1, 0 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 }, ObjectPlacement{ { 2, 0 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 },
                    ObjectPlacement{ { 1, 1 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 },
                    ObjectPlacement{ { 2, 1 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 } }, // valuables
                  { ObjectPlacement{ { 0, 0 }, Maps::ObjectGroup::MONSTERS, randomMonsterIndex } }, // monsters
                  { { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, -1 } } // entranceCheck
              },
              ObjectSet{
                  { ObjectPlacement{ { -3, -1 }, Maps::ObjectGroup::LANDSCAPE_TREES, 1 }, ObjectPlacement{ { -1, 2 }, Maps::ObjectGroup::LANDSCAPE_TREES, 4 },
                    ObjectPlacement{ { -3, 2 }, Maps::ObjectGroup::LANDSCAPE_TREES, 2 } }, // obstacles
                  { ObjectPlacement{ { -1, 0 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 }, ObjectPlacement{ { -2, 0 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 },
                    ObjectPlacement{ { -1, 1 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 },
                    ObjectPlacement{ { -2, 1 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 } }, // valuables
                  { ObjectPlacement{ { 0, 0 }, Maps::ObjectGroup::MONSTERS, randomMonsterIndex } }, // monsters
                  { { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, -1 } } // entranceCheck
              },
              ObjectSet{
                  { ObjectPlacement{ { -3, 0 }, Maps::ObjectGroup::LANDSCAPE_TREES, 3 }, ObjectPlacement{ { -1, -1 }, Maps::ObjectGroup::LANDSCAPE_TREES, 5 },
                    ObjectPlacement{ { -3, 1 }, Maps::ObjectGroup::LANDSCAPE_TREES, 2 } }, // obstacles
                  { ObjectPlacement{ { -2, 0 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 },
                    ObjectPlacement{ { -1, 0 }, Maps::ObjectGroup::ADVENTURE_TREASURES, 9 } }, // valuables
                  { ObjectPlacement{ { 0, 0 }, Maps::ObjectGroup::MONSTERS, randomMonsterIndex } }, // monsters
                  { { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, -1 }, { 0, 1 } } // entranceCheck
              } } };

    struct RegionalObjects final
    {
        uint8_t castleCount{ 0 };
        uint8_t mineCount{ 0 };
        uint8_t objectCount{ 0 };
        uint8_t powerUpsCount{ 0 };
        uint8_t treasureCount{ 0 };
    };

    constexpr std::array<RegionalObjects, static_cast<size_t>( Maps::Random_Generator::ResourceDensity::ITEM_COUNT )> regionObjectSetup = { {
        { 1, 2, 1, 1, 2 }, // ResourceDensity::SCARCE
        { 1, 6, 2, 1, 3 }, // ResourceDensity::NORMAL
        { 1, 7, 2, 2, 5 } // ResourceDensity::ABUNDANT
    } };

    struct MapEconomy final
    {
        std::map<int, uint32_t> minesCount{ { Resource::WOOD, 0 }, { Resource::ORE, 0 },     { Resource::CRYSTAL, 0 }, { Resource::SULFUR, 0 },
                                            { Resource::GEMS, 0 }, { Resource::MERCURY, 0 }, { Resource::GOLD, 0 } };

        void increaseMineCount( const int resource )
        {
            const auto it = minesCount.find( resource );
            assert( it != minesCount.end() );
            ++it->second;
        }

        int pickNextMineResource()
        {
            const auto it = std::min_element( secondaryResources.begin(), secondaryResources.end(),
                                              [this]( const auto & a, const auto & b ) { return minesCount.at( a ) < minesCount.at( b ); } );
            assert( it != secondaryResources.end() );

            return *it;
        }
    };

    int32_t calculateRegionSizeLimit( const Maps::Random_Generator::Configuration & config, const int32_t width, const int32_t height )
    {
        // Water percentage cannot be 100 or more, or negative.
        assert( config.waterPercentage >= 0 && config.waterPercentage < 100 );

        int32_t requiredSpace = 0;

        // Determine required space based on expected object count and their footprint (in tiles).
        const auto & objectSet = regionObjectSetup[static_cast<size_t>( config.resourceDensity )];
        requiredSpace += objectSet.castleCount * 49;
        requiredSpace += objectSet.mineCount * 15;
        requiredSpace += objectSet.objectCount * 6;
        requiredSpace += objectSet.powerUpsCount * 9;
        requiredSpace += objectSet.treasureCount * 16;

        DEBUG_LOG( DBG_DEVEL, DBG_TRACE, "Space required for density " << static_cast<int32_t>( config.resourceDensity ) << " is " << requiredSpace );

        requiredSpace = requiredSpace * 100 / ( 100 - emptySpacePercentage );

        const double innerRadius = std::ceil( sqrt( requiredSpace / M_PI ) );
        const int32_t borderSize = static_cast<int32_t>( 2 * ( innerRadius + 1 ) * M_PI );
        const int32_t targetRegionSize = requiredSpace + borderSize;

        DEBUG_LOG( DBG_DEVEL, DBG_TRACE, "Region target size is " << requiredSpace << " + " << borderSize << " = " << targetRegionSize );

        // Inner and outer circles, update later to handle other layouts.
        const int32_t upperLimit = config.playerCount * 3;

        const int32_t totalTileCount = width * height;
        const int32_t groundTiles = totalTileCount * ( 100 - config.waterPercentage ) / 100;

        const int32_t average = groundTiles / targetRegionSize;
        const int32_t canFit = std::min( std::max( config.playerCount + 1, average ), upperLimit );

        return groundTiles / canFit;
    }

    struct PlacementTile final
    {
        int index{ 0 };
        float distance{ 0 };
        int ring{ 0 };
    };

    std::vector<std::vector<PlacementTile>> findOpenTilesSortedJittered( const Region & region, int mapWidth, Rand::PCG32 & randomGenerator )
    {
        if ( region.centerIndex < 0 || region.nodes.empty() || mapWidth <= 0 ) {
            return {};
        }

        std::vector<std::vector<PlacementTile>> buckets;

        const int centerX = region.centerIndex % mapWidth;
        const int centerY = region.centerIndex / mapWidth;

        for ( const Node & node : region.nodes ) {
            if ( node.type != NodeType::OPEN || node.index < 0 ) {
                continue;
            }

            const int dx = ( node.index % mapWidth ) - centerX;
            const int dy = ( node.index / mapWidth ) - centerY;

            const float distance = std::sqrt( static_cast<float>( dx * dx + dy * dy ) );
            const int noise = static_cast<int>( Rand::GetWithGen( 0, 2, randomGenerator ) );
            const int ring = static_cast<int>( std::floor( distance ) ) + noise;
            if ( static_cast<size_t>( ring ) >= buckets.size() ) {
                buckets.resize( ring + 1 );
            }

            buckets[ring].push_back( { node.index, distance, ring } );
        }

        for ( auto & bucket : buckets ) {
            if ( bucket.empty() ) {
                continue;
            }

            Rand::ShuffleWithGen( bucket, randomGenerator );
        }
        return buckets;
    }

    constexpr int32_t treeTypeFromGroundType( const int groundType )
    {
        switch ( groundType ) {
        case Maps::Ground::WATER:
            assert( 0 );
            return 0;
        case Maps::Ground::GRASS:
            return 0;
        case Maps::Ground::SNOW:
            return 30;
        case Maps::Ground::SWAMP:
            return 0;
        case Maps::Ground::LAVA:
            return 6;
        case Maps::Ground::DESERT:
            return 24;
        case Maps::Ground::DIRT:
            return 18;
        case Maps::Ground::WASTELAND:
            return 12;
        case Maps::Ground::BEACH:
            return 24;
        default:
            // Have you added a new ground? Add the logic above!
            assert( 0 );
            break;
        }
        return 0;
    }

    constexpr int32_t mountainTypeFromGroundType( const int groundType )
    {
        switch ( groundType ) {
        case Maps::Ground::WATER:
            assert( 0 );
            return 0;
        case Maps::Ground::GRASS:
            return 6;
        case Maps::Ground::SNOW:
            return 12;
        case Maps::Ground::SWAMP:
            return 18;
        case Maps::Ground::LAVA:
            return 24;
        case Maps::Ground::DESERT:
            return 30;
        case Maps::Ground::DIRT:
            return 36;
        case Maps::Ground::WASTELAND:
            return 44;
        case Maps::Ground::BEACH:
            return 0;
        default:
            // Have you added a new ground? Add the logic above!
            assert( 0 );
            break;
        }
        return 0;
    }

    bool checkForRegionConnectors( NodeCache & data, std::vector<Region> & mapRegions, Region & region, Node & node )
    {
        if ( node.type != NodeType::BORDER ) {
            return false;
        }

        const fheroes2::Point position = Maps::GetPoint( node.index );

        int distinctNeighbours = 0;
        uint32_t seen = node.region;
        for ( uint8_t direction = 0; direction < directionCount; ++direction ) {
            const Node & newTile = data.getNode( position + directionOffsets[direction] );

            if ( newTile.index == -1 || newTile.region == region.id ) {
                continue;
            }

            if ( seen != newTile.region ) {
                seen = newTile.region;
                if ( ++distinctNeighbours > 1 ) {
                    return false;
                }
            }
        }

        for ( uint8_t direction = 0; direction < 4; ++direction ) {
            Node & adjacent = data.getNode( position + directionOffsets[direction] );

            if ( adjacent.index == -1 || adjacent.region == region.id || mapRegions[adjacent.region].groundType == Maps::Ground::WATER ) {
                continue;
            }

            Node & twoAway = data.getNode( position + directionOffsets[direction] + directionOffsets[direction] );
            if ( twoAway.index == -1 || twoAway.type != NodeType::OPEN ) {
                continue;
            }

            if ( region.connections.find( adjacent.region ) == region.connections.end() ) {
                DEBUG_LOG( DBG_DEVEL, DBG_TRACE, "Found a connection between " << region.id << " and " << adjacent.region << ", via " << node.index )
                region.connections.emplace( adjacent.region, node.index );
                mapRegions[adjacent.region].connections.emplace( region.id, node.index );
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

    void checkAdjacentTiles( NodeCache & rawData, Region & region, Rand::PCG32 & randomGenerator )
    {
        Node & previousNode = region.nodes[region.lastProcessedNode];
        const int nodeIndex = previousNode.index;

        for ( uint8_t direction = 0; direction < directionCount; ++direction ) {
            if ( region.nodes.size() > region.sizeLimit ) {
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
                newTile.region = region.id;
                region.nodes.push_back( newTile );
            }
            else if ( newTile.region != region.id ) {
                previousNode.type = NodeType::BORDER;
                region.neighbours.insert( newTile.region );
            }
        }
    }

    void regionExpansion( NodeCache & rawData, Region & region, Rand::PCG32 & randomGenerator )
    {
        // Process only "open" nodes that exist at the start of the loop and ignore what's added.
        const size_t nodesEnd = region.nodes.size();

        while ( region.lastProcessedNode < nodesEnd ) {
            checkAdjacentTiles( rawData, region, randomGenerator );
            ++region.lastProcessedNode;
        }
    }

    template <class F>
    bool iterateOverObjectParts( const Maps::ObjectInfo & info, const F & lambda )
    {
        for ( const auto & objectPart : info.groundLevelParts ) {
            if ( objectPart.layerType == Maps::SHADOW_LAYER || objectPart.layerType == Maps::TERRAIN_LAYER ) {
                // Shadow and terrain layer parts are ignored.
                continue;
            }

            lambda( objectPart );
        }
        for ( const auto & objectPart : info.topLevelParts ) {
            lambda( objectPart );
        }
        return true;
    }

    bool canFitObject( const NodeCache & data, const Maps::ObjectInfo & info, const fheroes2::Point & mainTilePos, const bool isAction, const bool skipBorders )
    {
        bool invalid = false;
        fheroes2::Rect objectRect;

        iterateOverObjectParts( info, [&data, &mainTilePos, &objectRect, skipBorders, &invalid]( const auto & partInfo ) {
            const Node & node = data.getNode( mainTilePos + partInfo.tileOffset );

            if ( node.index == -1 || node.region == 0 ) {
                invalid = true;
                return;
            }
            if ( node.type != NodeType::OPEN && ( skipBorders || node.type != NodeType::BORDER ) ) {
                invalid = true;
                return;
            }

            objectRect.x = std::min( objectRect.x, partInfo.tileOffset.x );
            objectRect.width = std::max( objectRect.width, partInfo.tileOffset.x );
        } );

        if ( invalid ) {
            return false;
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

    bool canFitObjectSet( const NodeCache & data, const ObjectSet & set, const fheroes2::Point & mainTilePos )
    {
        for ( const fheroes2::Point offset : set.entranceCheck ) {
            const Node & node = data.getNode( mainTilePos + offset );
            if ( node.index == -1 || node.type != NodeType::OPEN ) {
                return false;
            }
        }

        for ( const auto * placements : { &set.obstacles, &set.valuables, &set.monsters } ) {
            for ( const auto & placement : *placements ) {
                const auto & info = Maps::getObjectInfo( placement.groupType, placement.objectIndex );
                const bool isAction = MP2::isInGameActionObject( info.objectType );
                const fheroes2::Point position = mainTilePos + placement.offset;
                bool invalid = false;

                iterateOverObjectParts( info, [&data, &position, isAction, &invalid]( const auto & partInfo ) {
                    const Node & node = data.getNode( position + partInfo.tileOffset );

                    if ( node.index == -1 || node.region == 0 ) {
                        invalid = true;
                        return;
                    }
                    if ( node.type != NodeType::OPEN && ( isAction || node.type != NodeType::BORDER ) ) {
                        invalid = true;
                        return;
                    }
                } );

                if ( invalid ) {
                    return false;
                }
            }
        }

        return true;
    }

    void markObjectPlacement( NodeCache & data, const Maps::ObjectInfo & info, const fheroes2::Point & mainTilePos, const bool isAction )
    {
        fheroes2::Rect objectRect;

        iterateOverObjectParts( info, [&data, &mainTilePos, &objectRect]( const auto & partInfo ) {
            Node & node = data.getNode( mainTilePos + partInfo.tileOffset );
            objectRect.x = std::min( objectRect.x, partInfo.tileOffset.x );
            objectRect.width = std::max( objectRect.width, partInfo.tileOffset.x );

            node.type = NodeType::OBSTACLE;
        } );

        if ( !isAction ) {
            return;
        }

        for ( int x = objectRect.x - 1; x <= objectRect.width + 1; ++x ) {
            data.getNode( mainTilePos + fheroes2::Point{ x, objectRect.height + 1 } ).type = NodeType::PATH;
        }

        // Mark extra nodes as path to avoid objects clumping together
        data.getNode( mainTilePos + fheroes2::Point{ objectRect.x - 1, 0 } ).type = NodeType::PATH;
        data.getNode( mainTilePos + fheroes2::Point{ objectRect.width + 1, 0 } ).type = NodeType::PATH;

        data.getNode( mainTilePos ).type = NodeType::ACTION;
    }

    bool putObjectOnMap( Maps::Map_Format::MapFormat & mapFormat, Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex )
    {
        const auto & objectInfo = Maps::getObjectInfo( groupType, objectIndex );
        if ( objectInfo.empty() ) {
            assert( 0 );
            return false;
        }

        const MP2::MapObjectType mp2Type = objectInfo.objectType;
        // Maps::setObjectOnTile isn't idempotent, check if object was already placed
        if ( MP2::isInGameActionObject( mp2Type ) && tile.getMainObjectType() == mp2Type ) {
            return false;
        }

        // Do not update passabilities after every object placement.
        if ( !Maps::setObjectOnTile( tile, objectInfo, false ) ) {
            return false;
        }

        Maps::addObjectToMap( mapFormat, tile.GetIndex(), groupType, static_cast<uint32_t>( objectIndex ) );

        return true;
    }

    bool actionObjectPlacer( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t type )
    {
        const fheroes2::Point tilePos = tile.GetCenter();
        const auto & objectInfo = Maps::getObjectInfo( groupType, type );
        if ( canFitObject( data, objectInfo, tilePos, true, true ) && putObjectOnMap( mapFormat, tile, groupType, type ) ) {
            markObjectPlacement( data, objectInfo, tilePos, true );
            return true;
        }

        return false;
    }

    fheroes2::Point adjustRegionToFitCastle( const Maps::Map_Format::MapFormat & mapFormat, Region & region )
    {
        const fheroes2::Point startingLocation = Maps::GetPoint( region.centerIndex );
        const int32_t castleX = std::min( std::max( startingLocation.x, 4 ), mapFormat.width - 4 );
        const int32_t castleY = std::min( std::max( startingLocation.y, 4 ), mapFormat.width - 4 );
        region.centerIndex = Maps::GetIndexFromAbsPoint( castleX, castleY + 2 );
        return { castleX, castleY };
    }

    bool placeCastle( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Region & region, const fheroes2::Point tilePos, const bool isCastle )
    {
        auto & tile = world.getTile( tilePos.x, tilePos.y );
        if ( tile.isWater() ) {
            return false;
        }

        const int32_t basementId = fheroes2::getTownBasementId( tile.GetGround() );
        const int32_t castleObjectId = isCastle ? randomCastleIndex : randomTownIndex;

        const auto & basementInfo = Maps::getObjectInfo( Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId );
        const auto & castleInfo = Maps::getObjectInfo( Maps::ObjectGroup::KINGDOM_TOWNS, castleObjectId );

        if ( canFitObject( data, basementInfo, tilePos, false, true ) && canFitObject( data, castleInfo, tilePos, true, true ) ) {
            if ( !putObjectOnMap( mapFormat, tile, Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId ) ) {
                return false;
            }

            // Since the whole object consists of multiple "objects" we have to put the same ID for all of them.
            // Every time an object is being placed on a map the counter is going to be increased by 1.
            // Therefore, we set the counter by 1 less for each object to match object UID for all of them.
            assert( Maps::getLastObjectUID() > 0 );
            const uint32_t objectId = Maps::getLastObjectUID() - 1;

            Maps::setLastObjectUID( objectId );

            if ( !putObjectOnMap( mapFormat, tile, Maps::ObjectGroup::KINGDOM_TOWNS, castleObjectId ) ) {
                return false;
            }

            const int32_t bottomIndex = Maps::GetDirectionIndex( tile.GetIndex(), Direction::BOTTOM );

            if ( Maps::isValidAbsIndex( bottomIndex ) && Maps::doesContainRoads( mapFormat.tiles[bottomIndex] ) ) {
                // Update road if there is one in front of the town/castle entrance.
                Maps::updateRoadSpriteOnTile( mapFormat, bottomIndex, false );
            }

            // By default use random (default) army for the neutral race town/castle.
            const PlayerColor color = Color::IndexToColor( region.colorIndex );
            if ( color == PlayerColor::NONE ) {
                Maps::setDefaultCastleDefenderArmy( mapFormat.castleMetadata[Maps::getLastObjectUID()] );
            }

            // Add flags.
            assert( tile.GetIndex() > 0 && tile.GetIndex() < world.w() * world.h() - 1 );
            Maps::setLastObjectUID( objectId );

            if ( !putObjectOnMap( mapFormat, world.getTile( tile.GetIndex() - 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, Color::GetIndex( color ) * 2 ) ) {
                return false;
            }

            Maps::setLastObjectUID( objectId );

            if ( !putObjectOnMap( mapFormat, world.getTile( tile.GetIndex() + 1 ), Maps::ObjectGroup::LANDSCAPE_FLAGS, Color::GetIndex( color ) * 2 + 1 ) ) {
                return false;
            }

            const Maps::ObjectInfo & townObjectInfo = Maps::getObjectInfo( Maps::ObjectGroup::KINGDOM_TOWNS, castleObjectId );
            const uint8_t race = Race::IndexToRace( static_cast<int>( townObjectInfo.metadata[0] ) );

            world.addCastle( tile.GetIndex(), race, color );

            markObjectPlacement( data, basementInfo, tilePos, false );
            markObjectPlacement( data, castleInfo, tilePos, true );

            // Force roads coming from the castle
            const int32_t nextIndex = Maps::GetDirectionIndex( bottomIndex, Direction::BOTTOM );
            if ( Maps::isValidAbsIndex( nextIndex ) ) {
                Maps::updateRoadOnTile( mapFormat, bottomIndex, true );
                Maps::updateRoadOnTile( mapFormat, nextIndex, true );
            }

            return true;
        }

        return false;
    }

    bool placeMine( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & node, const int resource )
    {
        Maps::Tile & mineTile = world.getTile( node.index );
        const int32_t mineType = fheroes2::getMineObjectInfoId( resource, mineTile.GetGround() );
        return actionObjectPlacer( mapFormat, data, mineTile, Maps::ObjectGroup::ADVENTURE_MINES, mineType );
    }

    int32_t localizeObjectToTerrain( const Maps::ObjectGroup groupType, const int32_t objectIndex, const int groundType )
    {
        if ( groupType == Maps::ObjectGroup::LANDSCAPE_TREES ) {
            assert( objectIndex < 6 );
            return treeTypeFromGroundType( groundType ) + objectIndex;
        }

        if ( groupType == Maps::ObjectGroup::LANDSCAPE_MOUNTAINS ) {
            assert( objectIndex < 8 );
            return mountainTypeFromGroundType( groundType ) + objectIndex;
        }
        return objectIndex;
    }

    bool placeObstacle( Maps::Map_Format::MapFormat & mapFormat, const NodeCache & data, const Node & node, Rand::PCG32 & randomGenerator )
    {
        Maps::Tile & tile = world.getTile( node.index );
        const auto it = obstaclesPerGround.find( tile.GetGround() );
        if ( it == obstaclesPerGround.end() || it->second.empty() ) {
            return false;
        }

        std::vector<int> obstacleList = it->second;
        Rand::ShuffleWithGen( obstacleList, randomGenerator );

        const fheroes2::Point tilePos = tile.GetCenter();
        for ( const auto & obstacleId : obstacleList ) {
            const auto & objectInfo = Maps::getObjectInfo( Maps::ObjectGroup::LANDSCAPE_TREES, obstacleId );
            if ( canFitObject( data, objectInfo, tilePos, false, false ) && putObjectOnMap( mapFormat, tile, Maps::ObjectGroup::LANDSCAPE_TREES, obstacleId ) ) {
                return true;
            }
        }
        return false;
    }

    bool placeSimpleObject( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & centerNode, const ObjectPlacement & placement )
    {
        const fheroes2::Point position = Maps::GetPoint( centerNode.index ) + placement.offset;
        if ( !Maps::isValidAbsPoint( position.x, position.y ) ) {
            return false;
        }

        Maps::Tile & tile = world.getTile( position.x, position.y );
        const int32_t objectIndex = localizeObjectToTerrain( placement.groupType, placement.objectIndex, tile.GetGround() );
        const auto & objectInfo = Maps::getObjectInfo( placement.groupType, objectIndex );
        if ( putObjectOnMap( mapFormat, tile, placement.groupType, objectIndex ) ) {
            markObjectPlacement( data, objectInfo, position, false );
            return true;
        }
        return false;
    }
}

namespace Maps::Random_Generator
{
    bool generateMap( Map_Format::MapFormat & mapFormat, const Configuration & config, const int32_t width, const int32_t height )
    {
        // Make sure that we are generating a valid map.
        assert( width > 0 && height > 0 );

        if ( config.playerCount < 2 || config.playerCount > 6 ) {
            assert( config.playerCount <= 6 );
            return false;
        }

        // Initialization step. Reset the current map in `world` and `mapFormat` containers first.
        Interface::EditorInterface & interface = Interface::EditorInterface::Get();
        if ( !interface.generateNewMap( width ) ) {
            return false;
        }

        const int32_t regionSizeLimit = calculateRegionSizeLimit( config, width, height );
        if ( regionSizeLimit < smallestStartingRegionSize ) {
            return false;
        }

        const uint32_t generatorSeed = ( config.seed > 0 ) ? config.seed : Rand::Get( 999999 );
        DEBUG_LOG( DBG_DEVEL, DBG_INFO, "Generating a map with seed " << generatorSeed );
        DEBUG_LOG( DBG_DEVEL, DBG_INFO, "Region size limit " << regionSizeLimit << ", water " << config.waterPercentage << "%" );

        Rand::PCG32 randomGenerator( generatorSeed );

        NodeCache data( width, height );

        auto mapBoundsCheck = [width, height]( int x, int y ) {
            x = std::clamp( x, 0, width - 1 );
            y = std::clamp( y, 0, height - 1 );
            return x * width + y;
        };

        // Step 1. Setup map generator configuration.
        // TODO: Add support for layouts other than MIRRORED
        const int32_t groundTiles = ( width * height ) * ( 100 - config.waterPercentage ) / 100;
        const int expectedRegionCount = groundTiles / regionSizeLimit;

        // Step 2. Determine region layout and placement.
        //         Insert empty region that represents water and map edges
        std::vector<Region> mapRegions = { { 0, data.getNode( 0 ), neutralColorIndex, Ground::WATER, 1 } };

        const int neutralRegionCount = std::max( 1, expectedRegionCount - config.playerCount );
        const int innerLayer = std::min( neutralRegionCount, config.playerCount );
        const int outerLayer = std::max( std::min( neutralRegionCount, innerLayer * 2 ), config.playerCount );

        const double radius = sqrt( ( innerLayer + outerLayer ) * regionSizeLimit / M_PI );
        const double outerRadius = ( ( innerLayer + outerLayer ) > expectedRegionCount ) ? std::max( width, height ) * 0.47 : radius * 0.85;
        const double innerRadius = ( innerLayer == 1 ) ? 0 : outerRadius / 3;

        const std::vector<std::pair<int, double>> mapLayers = { { innerLayer, innerRadius }, { outerLayer, outerRadius } };

        for ( size_t layer = 0; layer < mapLayers.size(); ++layer ) {
            const int regionCount = mapLayers[layer].first;
            const double startingAngle = Rand::GetWithGen( 0, 360, randomGenerator );
            const double offsetAngle = 360.0 / regionCount;
            for ( int i = 0; i < regionCount; ++i ) {
                const double radians = ( startingAngle + offsetAngle * i ) * M_PI / 180;
                const double distance = mapLayers[layer].second;

                const int x = width / 2 + static_cast<int>( cos( radians ) * distance );
                const int y = height / 2 + static_cast<int>( sin( radians ) * distance );
                const int centerTile = mapBoundsCheck( x, y );

                const int factor = regionCount / config.playerCount;
                const bool isPlayerRegion = ( layer == 1 ) && ( ( i % factor ) == 0 );

                const int groundType = isPlayerRegion ? Rand::GetWithGen( playerStartingTerrain, randomGenerator ) : Rand::GetWithGen( neutralTerrain, randomGenerator );
                const int regionColor = isPlayerRegion ? i / factor : neutralColorIndex;

                const uint32_t regionID = static_cast<uint32_t>( mapRegions.size() );
                Node & centerNode = data.getNode( centerTile );
                mapRegions.emplace_back( regionID, centerNode, regionColor, groundType, regionSizeLimit );

                DEBUG_LOG( DBG_DEVEL, DBG_TRACE,
                           "Region " << regionID << " defined. Location " << centerTile << ", " << Ground::String( groundType ) << " terrain, owner "
                                     << Color::String( Color::IndexToColor( regionColor ) ) )
            }
        }

        // Step 3. Grow all regions one step at the time so they would compete for space.
        bool stillRoomToExpand = true;
        while ( stillRoomToExpand ) {
            stillRoomToExpand = false;
            // Skip the first region which is the border region.
            for ( size_t regionID = 1; regionID < mapRegions.size(); ++regionID ) {
                Region & region = mapRegions[regionID];
                regionExpansion( data, region, randomGenerator );
                if ( region.lastProcessedNode != region.nodes.size() ) {
                    stillRoomToExpand = true;
                }
            }
        }

        // Step 4. Apply terrain changes into the map format.
        for ( const Region & region : mapRegions ) {
            if ( region.id == 0 ) {
                continue;
            }

            for ( const Node & node : region.nodes ) {
                Maps::setTerrainOnTile( mapFormat, node.index, region.groundType );
            }

            // Fix missing references.
            for ( const uint32_t adjacent : region.neighbours ) {
                const auto & [iter, inserted] = mapRegions[adjacent].neighbours.insert( region.id );
                if ( inserted ) {
                    DEBUG_LOG( DBG_DEVEL, DBG_WARN, "Missing link between " << region.id << " and " << adjacent )
                }
            }
        }

        // Step 5. Castles and mines placement
        std::set<int32_t> startingLocations;
        std::set<int32_t> actionLocations;

        MapEconomy mapEconomy;

        for ( Region & region : mapRegions ) {
            if ( region.id == 0 ) {
                // Skip the first region as we have nothing to do here for now.
                continue;
            }

            DEBUG_LOG( DBG_ENGINE, DBG_TRACE,
                       "Region #" << region.id << " of size " << region.nodes.size() << " tiles has " << region.neighbours.size() << " neighbours" )

            for ( const Node & node : region.nodes ) {
                if ( node.type == NodeType::BORDER ) {
                    Maps::setTerrainWithTransition( mapFormat, node.index, node.index, region.groundType );
                }
            }

            if ( region.colorIndex != neutralColorIndex ) {
                const fheroes2::Point castlePos = adjustRegionToFitCastle( mapFormat, region );
                if ( !placeCastle( mapFormat, data, region, castlePos, true ) ) {
                    // Return early if we can't place a starting player castle.
                    DEBUG_LOG( DBG_DEVEL, DBG_WARN, "Not able to place a starting player castle on tile " << castlePos.x << ", " << castlePos.y )
                    return false;
                }
                startingLocations.insert( region.centerIndex );
            }
            else if ( static_cast<int32_t>( region.nodes.size() ) > regionSizeLimit ) {
                // Place non-mandatory castles in bigger neutral regions.
                const bool useNeutralCastles = config.resourceDensity == Maps::Random_Generator::ResourceDensity::ABUNDANT;
                const fheroes2::Point castlePos = adjustRegionToFitCastle( mapFormat, region );
                placeCastle( mapFormat, data, region, castlePos, useNeutralCastles );
            }

            const std::vector<std::vector<PlacementTile>> tileRings = findOpenTilesSortedJittered( region, width, randomGenerator );

            const auto tryToPlaceMine = [&]( const std::vector<PlacementTile> & ring, const int resource ) {
                for ( const PlacementTile & tile : ring ) {
                    const auto & node = data.getNode( tile.index );
                    if ( placeMine( mapFormat, data, node, resource ) ) {
                        mapEconomy.increaseMineCount( resource );
                        actionLocations.insert( tile.index );
                        return tile.index;
                    }
                }
                return -1;
            };

            for ( const int resource : { Resource::WOOD, Resource::ORE } ) {
                for ( size_t ringIndex = 4; ringIndex < tileRings.size(); ++ringIndex ) {
                    if ( tryToPlaceMine( tileRings[ringIndex], resource ) != -1 ) {
                        break;
                    }
                }
            }

            if ( tileRings.size() < 4 ) {
                continue;
            }

            for ( size_t idx = 0; idx < secondaryResources.size(); ++idx ) {
                const int resource = mapEconomy.pickNextMineResource();
                for ( size_t ringIndex = tileRings.size() - 3; ringIndex > 0; --ringIndex ) {
                    const int mineIndex = tryToPlaceMine( tileRings[ringIndex], resource );
                    if ( mineIndex != -1 ) {
                        putObjectOnMap( mapFormat, world.getTile( mineIndex + mapFormat.width ), Maps::ObjectGroup::MONSTERS, randomMonsterIndex );
                        break;
                    }
                }
            }
        }

        // Step 6. Set up region connectors based on frequency settings and border length.
        for ( Region & region : mapRegions ) {
            if ( region.groundType == Ground::WATER ) {
                continue;
            }
            for ( Node & node : region.nodes ) {
                checkForRegionConnectors( data, mapRegions, region, node );
            }
        }

        for ( const Region & region : mapRegions ) {
            for ( const Node & node : region.nodes ) {
                if ( node.type == NodeType::BORDER ) {
                    placeObstacle( mapFormat, data, node, randomGenerator );
                }
            }
        }

        // Step 7. Set up pathfinder to generate road based paths.
        AIWorldPathfinder pathfinder;
        pathfinder.reset();
        const PlayerColor testPlayer = PlayerColor::BLUE;

        // Have to remove fog first otherwise pathfinder won't work
        for ( int idx = 0; idx < width * height; ++idx ) {
            world.getTile( idx ).removeFogForPlayers( static_cast<PlayerColorsSet>( testPlayer ) );
        }
        world.resetPathfinder();
        world.updatePassabilities();

        // Set explicit paths
        for ( const Region & region : mapRegions ) {
            pathfinder.reEvaluateIfNeeded( region.centerIndex, testPlayer, 999999.9, Skill::Level::EXPERT );
            for ( const auto & [regionId, tileIndex] : region.connections ) {
                const auto & path = pathfinder.buildPath( tileIndex );
                for ( const auto & step : path ) {
                    data.getNode( step.GetIndex() ).type = NodeType::PATH;
                    Maps::updateRoadOnTile( mapFormat, step.GetIndex(), true );
                }
            }
        }

        // TODO: place objects while avoiding the borders.

        // TODO: place treasure objects.
        for ( const Region & region : mapRegions ) {
            int treasureObjectsPlaced = 0;

            for ( int attempt = 0; attempt < maxPlacementAttempts; ++attempt ) {
                if ( treasureObjectsPlaced == regionObjectSetup[static_cast<size_t>( config.resourceDensity )].treasureCount ) {
                    break;
                }

                const Node & node = Rand::GetWithGen( region.nodes, randomGenerator );

                std::vector<ObjectSet> shuffledObjectSets = prefabObjectSets;
                Rand::ShuffleWithGen( shuffledObjectSets, randomGenerator );
                for ( const auto & prefab : shuffledObjectSets ) {
                    if ( !canFitObjectSet( data, prefab, Maps::GetPoint( node.index ) ) ) {
                        continue;
                    }

                    for ( const auto & obstacle : prefab.obstacles ) {
                        placeSimpleObject( mapFormat, data, node, obstacle );
                    }
                    for ( const auto & treasure : prefab.valuables ) {
                        placeSimpleObject( mapFormat, data, node, treasure );
                    }
                    for ( const auto & monster : prefab.monsters ) {
                        const fheroes2::Point position = Maps::GetPoint( node.index ) + monster.offset;
                        putObjectOnMap( mapFormat, world.getTile( position.x, position.y ), Maps::ObjectGroup::MONSTERS, monster.objectIndex );
                    }
                    ++treasureObjectsPlaced;
                    break;
                }
            }
        }

        // TODO: fill big empty islands with obstacles

        for ( const int start : startingLocations ) {
            pathfinder.reEvaluateIfNeeded( start, testPlayer, 999999.9, Skill::Level::EXPERT );
            for ( const int action : actionLocations ) {
                if ( pathfinder.getDistance( action ) == 0 ) {
                    DEBUG_LOG( DBG_DEVEL, DBG_WARN, "Not able to find path from " << start << " to " << action )
                    return false;
                }
            }
        }

        // Step 11: place monsters.
        for ( const Region & region : mapRegions ) {
            for ( const auto & [regionId, tileIndex] : region.connections ) {
                putObjectOnMap( mapFormat, world.getTile( tileIndex ), Maps::ObjectGroup::MONSTERS, randomMonsterIndex );
            }
        }

        // Visual debug
        for ( const Region & region : mapRegions ) {
            for ( const Node & node : region.nodes ) {
                const uint32_t metadata = static_cast<uint32_t>( node.type ) + 100 * node.region;
                world.getTile( node.index ).UpdateRegion( metadata );
            }
        }

        // Set random map name and description to be unique.
        mapFormat.name = "Random map " + std::to_string( generatorSeed );
        mapFormat.description = "Randomly generated map of " + std::to_string( width ) + "x" + std::to_string( height ) + " with seed " + std::to_string( generatorSeed )
                                + ", " + std::to_string( config.playerCount ) + " players and " + std::to_string( config.waterPercentage ) + "% of water.";

        return true;
    }
}
