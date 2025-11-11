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
#include "editor_interface.h"
#include "ground.h"
#include "logging.h"
#include "map_format_helper.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "map_random_generator_helper.h"
#include "map_random_generator_info.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "rand.h"
#include "resource.h"
#include "route.h"
#include "skill.h"
#include "world.h"
#include "world_pathfinding.h"

namespace
{
    constexpr int32_t smallestStartingRegionSize{ 200 };
    constexpr int32_t emptySpacePercentage{ 40 };
    constexpr int randomMonsterIndex{ 68 };
    const std::vector<int> playerStartingTerrain = { Maps::Ground::GRASS, Maps::Ground::DIRT, Maps::Ground::SNOW, Maps::Ground::LAVA, Maps::Ground::WASTELAND };
    const std::vector<int> neutralTerrain = { Maps::Ground::GRASS,     Maps::Ground::DIRT,  Maps::Ground::SNOW,  Maps::Ground::LAVA,
                                              Maps::Ground::WASTELAND, Maps::Ground::BEACH, Maps::Ground::SWAMP, Maps::Ground::DESERT };

    constexpr std::array<Maps::Random_Generator::RegionalObjects, static_cast<size_t>( Maps::Random_Generator::ResourceDensity::ITEM_COUNT )> regionObjectSetup = { {
        { 1, 2, 1, 1, 2 }, // ResourceDensity::SCARCE
        { 1, 6, 2, 1, 3 }, // ResourceDensity::NORMAL
        { 1, 7, 2, 2, 5 } // ResourceDensity::ABUNDANT
    } };

    const std::vector<Maps::Random_Generator::ObjectPlacement> randomMonsterSet{ { { 0, 0 }, Maps::ObjectGroup::MONSTERS, randomMonsterIndex } };

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
}

namespace Maps::Random_Generator
{
    const std::vector<ObjectSet> prefabObjectSets{ ObjectSet{ // Obstacles.
                                                              { { { 0, -1 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                                                                { { 3, -1 }, ObjectGroup::LANDSCAPE_TREES, 2 },
                                                                { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 3 } },
                                                              // Valuables.
                                                              { { { 1, -1 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 2, -1 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                                                              // Monsters.
                                                              randomMonsterSet,
                                                              // Entrance check.
                                                              { { -1, 0 }, { -1, 1 }, { 0, 1 } } },
                                                   ObjectSet{ // Obstacles.
                                                              { { { -3, 0 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                                                                { { -3, 1 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                                                                { { 0, 2 }, ObjectGroup::LANDSCAPE_TREES, 3 } },
                                                              // Valuables.
                                                              { { { -2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -2, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -1, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                                                              // Monsters.
                                                              randomMonsterSet,
                                                              // Entrance check.
                                                              { { 0, -1 }, { 1, -1 }, { 1, 0 } } },
                                                   ObjectSet{ // Obstacles.
                                                              { { { 3, -1 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                                                                { { 1, 2 }, ObjectGroup::LANDSCAPE_TREES, 5 },
                                                                { { 3, 2 }, ObjectGroup::LANDSCAPE_TREES, 3 } },
                                                              // Valuables.
                                                              { { { 1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 1, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 2, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                                                              // Monsters.
                                                              randomMonsterSet,
                                                              // Entrance check.
                                                              { { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, -1 } } },
                                                   ObjectSet{ // Obstacles.
                                                              { { { -3, -1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                                                                { { -1, 2 }, ObjectGroup::LANDSCAPE_TREES, 4 },
                                                                { { -3, 2 }, ObjectGroup::LANDSCAPE_TREES, 2 } },
                                                              // Valuables.
                                                              { { { -1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -1, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -2, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                                                              // Monsters.
                                                              randomMonsterSet,
                                                              // Entrance check.
                                                              { { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, -1 } } },
                                                   ObjectSet{ // Obstacles.
                                                              { { { -3, 0 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                                                                { { -1, -1 }, ObjectGroup::LANDSCAPE_TREES, 5 },
                                                                { { -3, 1 }, ObjectGroup::LANDSCAPE_TREES, 2 } },
                                                              // Valuables.
                                                              { { { -2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 }, { { -1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                                                              // Monsters.
                                                              randomMonsterSet,
                                                              // Entrance check.
                                                              { { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, -1 }, { 0, 1 } } } };

    const std::vector<ObjectSet> powerupObjectSets{
        ObjectSet{ // Obstacles.
                   { { { 0, -1 }, ObjectGroup::LANDSCAPE_TREES, 3 }, { { 3, -1 }, ObjectGroup::LANDSCAPE_TREES, 2 }, { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 3 } },
                   // Valuables.
                   { { { 1, -1 }, ObjectGroup::ADVENTURE_POWER_UPS, 10 },
                     { { 1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                     { { 2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                   // Monsters.
                   randomMonsterSet,
                   // Entrance check.
                   { { -1, 0 }, { -1, 1 }, { 0, 1 } } },
        ObjectSet{ // Obstacles.
                   { { { 0, -1 }, ObjectGroup::LANDSCAPE_TREES, 3 }, { { 3, -1 }, ObjectGroup::LANDSCAPE_TREES, 2 }, { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 3 } },
                   // Valuables.
                   { { { 1, -1 }, ObjectGroup::ADVENTURE_POWER_UPS, 12 },
                     { { 1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                     { { 2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                   // Monsters.
                   randomMonsterSet,
                   // Entrance check.
                   { { -1, 0 }, { -1, 1 }, { 0, 1 } } },
    };

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
                if ( mapRegions[regionID].regionExpansion( data, randomGenerator ) ) {
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
                const fheroes2::Point castlePos = region.adjustRegionToFitCastle( mapFormat );
                if ( !placeCastle( mapFormat, data, region, castlePos, true ) ) {
                    // Return early if we can't place a starting player castle.
                    DEBUG_LOG( DBG_DEVEL, DBG_WARN, "Not able to place a starting player castle on tile " << castlePos.x << ", " << castlePos.y )
                    return false;
                }
                startingLocations.insert( region.centerIndex );
            }
            else if ( static_cast<int32_t>( region.nodes.size() ) > regionSizeLimit ) {
                // Place non-mandatory castles in bigger neutral regions.
                const bool useNeutralCastles = ( config.resourceDensity == ResourceDensity::ABUNDANT );
                const fheroes2::Point castlePos = region.adjustRegionToFitCastle( mapFormat );
                placeCastle( mapFormat, data, region, castlePos, useNeutralCastles );
            }

            const std::vector<std::vector<int32_t>> tileRings = findOpenTilesSortedJittered( region, width, randomGenerator );

            const auto tryToPlaceMine = [&]( const std::vector<int32_t> & ring, const int resource ) {
                for ( const int32_t tileIndex : ring ) {
                    const auto & node = data.getNode( tileIndex );
                    if ( placeMine( mapFormat, data, node, resource ) ) {
                        mapEconomy.increaseMineCount( resource );
                        actionLocations.insert( tileIndex );
                        return tileIndex;
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
                        putObjectOnMap( mapFormat, world.getTile( mineIndex + mapFormat.width ), ObjectGroup::MONSTERS, randomMonsterIndex );
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
                region.checkNodeForConnections( data, mapRegions, node );
            }
        }

        for ( const Region & region : mapRegions ) {
            for ( const Node & node : region.nodes ) {
                if ( node.type == NodeType::BORDER ) {
                    placeObstacle( mapFormat, data, node, randomGenerator );
                }
            }
        }

        // Step 7. Set up pathfinder and generate road network.
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

        // Step 8: Place powerups and treasure clusters while avoiding the paths.
        const auto & regionConfiguration = regionObjectSetup[static_cast<size_t>( config.resourceDensity )];
        for ( const Region & region : mapRegions ) {
            placeObjectSet( mapFormat, data, region, powerupObjectSets, regionConfiguration.powerUpsCount, randomGenerator );
            placeObjectSet( mapFormat, data, region, prefabObjectSets, regionConfiguration.treasureCount, randomGenerator );
        }

        // TODO: Step 9: Detect and fill empty areas with decorative/flavour objects.

        // Step 10: Place missing monsters.
        for ( const Region & region : mapRegions ) {
            for ( const auto & [regionId, tileIndex] : region.connections ) {
                putObjectOnMap( mapFormat, world.getTile( tileIndex ), ObjectGroup::MONSTERS, randomMonsterIndex );
            }
        }

        // Step 11: Validate that map is playable.
        for ( const int32_t start : startingLocations ) {
            pathfinder.reEvaluateIfNeeded( start, testPlayer, 999999.9, Skill::Level::EXPERT );
            for ( const int action : actionLocations ) {
                if ( pathfinder.getDistance( action ) == 0 ) {
                    DEBUG_LOG( DBG_DEVEL, DBG_WARN, "Not able to find path from " << start << " to " << action )
                    return false;
                }
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
