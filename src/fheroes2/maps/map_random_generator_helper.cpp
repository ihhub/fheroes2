/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025 - 2026                                             *
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

#include "map_random_generator_helper.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <limits>
#include <map>
#include <utility>

#include "color.h"
#include "direction.h"
#include "ground.h"
#include "map_format_helper.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "map_random_generator.h"
#include "map_random_generator_info.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "monster.h"
#include "mp2.h"
#include "rand.h"
#include "resource.h"
#include "ui_map_object.h"
#include "world.h"
#include "world_object_uid.h"

namespace
{
    struct RoadBuilderNode final
    {
        int32_t _from{ -1 };
        uint32_t _cost{ 0 };
        int32_t _direction{ Direction::UNKNOWN };
    };

    constexpr int32_t randomCastleIndex{ 12 };
    constexpr int32_t randomTownIndex{ 13 };
    constexpr int32_t randomHeroIndex{ 7 };
    constexpr size_t maxPlacementAttempts{ 99 };
    constexpr uint32_t roadBuilderMargin{ Maps::Ground::defaultGroundPenalty * 3 };

    const std::map<int, std::vector<int>> obstaclesPerGround = {
        { Maps::Ground::DESERT, { 24, 25, 26, 27, 28, 29 } },    { Maps::Ground::SNOW, { 30, 31, 32, 33, 34, 35 } },  { Maps::Ground::SWAMP, { 18, 19, 20, 21, 22, 23 } },
        { Maps::Ground::WASTELAND, { 12, 13, 14, 15, 16, 17 } }, { Maps::Ground::BEACH, { 24, 25, 26, 27, 28, 29 } }, { Maps::Ground::LAVA, { 6, 7, 8, 9, 10, 11 } },
        { Maps::Ground::DIRT, { 18, 19, 20, 21, 22, 23 } },      { Maps::Ground::GRASS, { 0, 1, 2, 3, 4, 5 } },       { Maps::Ground::WATER, {} },
    };

    const std::vector<Maps::Random_Generator::ObjectPlacement> randomMonsterSet{ { { 0, 0 }, Maps::ObjectGroup::MONSTERS, 0 } };

    constexpr int32_t minimalTreasureValue{ 650 };
    constexpr int32_t maximumTreasureGroupValue{ 14000 };

    // Evaluating all treasure and power-ups using gold equivalent
    // Benchmark is 1500 gold = 1000 experience; 1 primary attribute point worth 2000 gold
    // Artifacts: treasure is 1 attribute, minor is 2, major is 4 with a bonus for rarity
    const std::map<MP2::MapObjectType, int32_t> objectGoldValue = {
        // Mines.
        { MP2::OBJ_ABANDONED_MINE, 7500 },
        { MP2::OBJ_ALCHEMIST_LAB, 3500 },
        { MP2::OBJ_MINE, 3500 },
        { MP2::OBJ_SAWMILL, 1000 },
        // Pickups.
        { MP2::OBJ_CAMPFIRE, 1000 },
        { MP2::OBJ_GENIE_LAMP, 7000 },
        { MP2::OBJ_RANDOM_ARTIFACT_MAJOR, 10000 },
        { MP2::OBJ_RANDOM_ARTIFACT_MINOR, 5000 },
        { MP2::OBJ_RANDOM_ARTIFACT_TREASURE, 2000 },
        { MP2::OBJ_RANDOM_RESOURCE, minimalTreasureValue },
        { MP2::OBJ_RESOURCE, minimalTreasureValue },
        { MP2::OBJ_TREASURE_CHEST, 1500 },
        // Power-ups.
        { MP2::OBJ_FORT, 3000 },
        { MP2::OBJ_GAZEBO, 2000 },
        { MP2::OBJ_MERCENARY_CAMP, 3000 },
        { MP2::OBJ_SHRINE_FIRST_CIRCLE, 1000 },
        { MP2::OBJ_SHRINE_SECOND_CIRCLE, 2500 },
        { MP2::OBJ_SHRINE_THIRD_CIRCLE, 4000 },
        { MP2::OBJ_STANDING_STONES, 3000 },
        { MP2::OBJ_TREE_OF_KNOWLEDGE, 5000 },
        { MP2::OBJ_WITCH_DOCTORS_HUT, 3000 },
        { MP2::OBJ_XANADU, 10000 },
        // Dwellings.
        { MP2::OBJ_AIR_ALTAR, 5000 },
        { MP2::OBJ_ARCHER_HOUSE, 2000 },
        { MP2::OBJ_CAVE, 1500 },
        { MP2::OBJ_DESERT_TENT, 3000 },
        { MP2::OBJ_DWARF_COTTAGE, 2000 },
        { MP2::OBJ_EARTH_ALTAR, 5000 },
        { MP2::OBJ_EXCAVATION, 1500 },
        { MP2::OBJ_FIRE_ALTAR, 5000 },
        { MP2::OBJ_GOBLIN_HUT, 1500 },
        { MP2::OBJ_HALFLING_HOLE, 1500 },
        { MP2::OBJ_PEASANT_HUT, minimalTreasureValue },
        { MP2::OBJ_RUINS, 5000 },
        { MP2::OBJ_TREE_CITY, 1500 },
        { MP2::OBJ_TREE_HOUSE, 1500 },
        { MP2::OBJ_WAGON_CAMP, 2000 },
        { MP2::OBJ_WATCH_TOWER, 2000 },
        { MP2::OBJ_WATER_ALTAR, 5000 },
    };

    constexpr int32_t treeTypeFromGroundType( const int32_t groundType )
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

    constexpr int32_t mountainTypeFromGroundType( const int32_t groundType )
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

    constexpr MP2::MapObjectType getFakeMP2MineType( const int32_t resource )
    {
        switch ( resource ) {
        case Resource::WOOD:
        case Resource::ORE:
            return MP2::OBJ_SAWMILL;
        case Resource::SULFUR:
        case Resource::CRYSTAL:
        case Resource::GEMS:
        case Resource::MERCURY:
            return MP2::OBJ_MINE;
        case Resource::GOLD:
            return MP2::OBJ_ABANDONED_MINE;
        default:
            // Have you added a new resource type?!
            assert( 0 );
            break;
        }
        return MP2::OBJ_NONE;
    }

    void iterateOverObjectParts( const Maps::ObjectInfo & info, const std::function<void( const Maps::ObjectPartInfo & )> & lambda )
    {
        for ( const auto & objectPart : info.groundLevelParts ) {
            if ( objectPart.layerType == Maps::SHADOW_LAYER || objectPart.layerType == Maps::TERRAIN_LAYER ) {
                // Shadow and terrain layer parts are ignored.
                continue;
            }

            lambda( objectPart );
        }
    }

    void markNodeIndexAsType( Maps::Random_Generator::MapStateManager & data, const int32_t index, const Maps::Random_Generator::NodeType type )
    {
        auto & node = data.getNodeToUpdate( index );
        // Never override border types (no assert needed; can happen when placing overlapping obstacles)
        if ( node.type != Maps::Random_Generator::NodeType::BORDER && node.type != Maps::Random_Generator::NodeType::ACTION ) {
            node.type = type;
        }
    }

    void markNodeAsType( Maps::Random_Generator::MapStateManager & data, const fheroes2::Point position, const Maps::Random_Generator::NodeType type )
    {
        if ( Maps::isValidAbsPoint( position.x, position.y ) ) {
            markNodeIndexAsType( data, Maps::GetIndexFromAbsPoint( position ), type );
        }
    }

    bool canPlaceObject( const Maps::Random_Generator::MapStateManager & data, const Maps::ObjectInfo & info, const fheroes2::Point position )
    {
        const bool isAction = MP2::isInGameActionObject( info.objectType );
        bool validPlacement = true;

        auto isValidObjectPart = [&data, &position, isAction, &validPlacement]( const auto & partInfo ) {
            const Maps::Random_Generator::Node & node = data.getNode( position + partInfo.tileOffset );

            if ( node.index == -1 || node.region == 0 ) {
                validPlacement = false;
                return;
            }
            if ( node.type == Maps::Random_Generator::NodeType::ACTION || node.type == Maps::Random_Generator::NodeType::PATH ) {
                validPlacement = false;
                return;
            }
            if ( node.type != Maps::Random_Generator::NodeType::OPEN && ( isAction || node.type != Maps::Random_Generator::NodeType::BORDER ) ) {
                validPlacement = false;
                return;
            }
        };

        iterateOverObjectParts( info, isValidObjectPart );

        for ( const auto & objectPart : info.topLevelParts ) {
            const Maps::Random_Generator::Node & node = data.getNode( position + objectPart.tileOffset );
            if ( node.index == -1 || node.region == 0 ) {
                return false;
            }
        }

        if ( isAction ) {
            const Maps::Random_Generator::Node & node = data.getNode( position + fheroes2::Point( 0, 1 ) );
            if ( node.index == -1 || node.region == 0 ) {
                return false;
            }
            if ( node.type != Maps::Random_Generator::NodeType::OPEN && node.type != Maps::Random_Generator::NodeType::PATH ) {
                return false;
            }
        }

        return validPlacement;
    }
}

namespace Maps::Random_Generator
{

    int32_t getObjectGoldValue( const MP2::MapObjectType object )
    {
        const auto it = objectGoldValue.find( object );
        if ( it == objectGoldValue.end() ) {
            // No valuation for the object? Add it!
            assert( 0 );
            return 0;
        }

        return it->second;
    }

    int32_t getObjectGoldValue( const ObjectGroup group, const int32_t objectIndex )
    {
        return getObjectGoldValue( Maps::getObjectInfo( group, objectIndex ).objectType );
    }

    MonsterSelection getMonstersByValue( const Maps::Random_Generator::MonsterStrength monsterStrength, int32_t protectedObjectValue )
    {
        if ( monsterStrength == MonsterStrength::DEADLY ) {
            protectedObjectValue += 2500;
        }
        else if ( monsterStrength == MonsterStrength::STRONG ) {
            protectedObjectValue += 1500;
        }
        else if ( monsterStrength == MonsterStrength::WEAK ) {
            protectedObjectValue -= 1500;
        }

        if ( protectedObjectValue >= 13000 ) {
            // 227 -> 504 monster strength
            return { Monster::RANDOM_MONSTER_LEVEL_4,
                     { Monster::GIANT, Monster::PHOENIX, Monster::BONE_DRAGON, Monster::GREEN_DRAGON, Monster::RED_DRAGON, Monster::TITAN, Monster::BLACK_DRAGON } };
        }
        if ( protectedObjectValue >= 11500 ) {
            // 70 -> 240 monster strength
            return { Monster::RANDOM_MONSTER_LEVEL_4,
                     { Monster::ARCHMAGE, Monster::POWER_LICH, Monster::PALADIN, Monster::CRUSADER, Monster::CYCLOPS, Monster::GENIE, Monster::GIANT, Monster::PHOENIX,
                       Monster::BONE_DRAGON } };
        }
        if ( protectedObjectValue >= 10000 ) {
            // 58 -> 171 monster strength
            return { Monster::RANDOM_MONSTER,
                     { Monster::LICH, Monster::WAR_TROLL, Monster::MAGE, Monster::UNICORN, Monster::HYDRA, Monster::VAMPIRE_LORD, Monster::ARCHMAGE, Monster::POWER_LICH,
                       Monster::GHOST, Monster::PALADIN, Monster::CRUSADER, Monster::CYCLOPS, Monster::GENIE } };
        }
        if ( protectedObjectValue >= 8500 ) {
            // 39 -> 72 monster strength
            return { Monster::RANDOM_MONSTER_LEVEL_3,
                     { Monster::GREATER_DRUID, Monster::OGRE_LORD, Monster::ROC, Monster::CAVALRY, Monster::VAMPIRE, Monster::MEDUSA, Monster::MINOTAUR_KING,
                       Monster::TROLL, Monster::CHAMPION, Monster::LICH, Monster::WAR_TROLL, Monster::MAGE, Monster::VAMPIRE_LORD, Monster::ARCHMAGE, Monster::GHOST } };
        }
        if ( protectedObjectValue >= 7000 ) {
            // 27 -> 47 monster strength
            return { Monster::RANDOM_MONSTER_LEVEL_3,
                     { Monster::GRIFFIN, Monster::MASTER_SWORDSMAN, Monster::EARTH_ELEMENT, Monster::AIR_ELEMENT, Monster::WATER_ELEMENT, Monster::FIRE_ELEMENT,
                       Monster::DRUID, Monster::GREATER_DRUID, Monster::MINOTAUR, Monster::OGRE_LORD, Monster::ROC, Monster::CAVALRY, Monster::VAMPIRE, Monster::MEDUSA,
                       Monster::MINOTAUR_KING } };
        }
        if ( protectedObjectValue >= 5500 ) {
            // 17 -> 31 monster strength
            return { Monster::RANDOM_MONSTER,
                     { Monster::VETERAN_PIKEMAN, Monster::MUMMY, Monster::NOMAD, Monster::IRON_GOLEM, Monster::ELF, Monster::ROYAL_MUMMY, Monster::WOLF,
                       Monster::GRAND_ELF, Monster::SWORDSMAN, Monster::OGRE, Monster::STEEL_GOLEM, Monster::GRIFFIN, Monster::MASTER_SWORDSMAN, Monster::EARTH_ELEMENT,
                       Monster::AIR_ELEMENT, Monster::WATER_ELEMENT, Monster::FIRE_ELEMENT } };
        }
        if ( protectedObjectValue >= 3500 ) {
            // 11 -> 18 monster strength
            return { Monster::RANDOM_MONSTER_LEVEL_2, {} };
        }
        if ( protectedObjectValue >= 2000 ) {
            // 0.92 -> 9 monster strength
            return { Monster::RANDOM_MONSTER_LEVEL_1, {} };
        }

        return {};
    }

    std::pair<ObjectGroup, int32_t> getRandomTreasure( const int32_t goldValueLimit, Rand::PCG32 & randomGenerator )
    {
        std::vector<MP2::MapObjectType> possibilities{
            MP2::OBJ_TREASURE_CHEST, MP2::OBJ_TREASURE_CHEST,           MP2::OBJ_RANDOM_RESOURCE,       MP2::OBJ_RANDOM_RESOURCE,
            MP2::OBJ_CAMPFIRE,       MP2::OBJ_RANDOM_ARTIFACT_TREASURE, MP2::OBJ_RANDOM_ARTIFACT_MINOR, MP2::OBJ_RANDOM_ARTIFACT_MAJOR,
        };

        possibilities.erase( std::remove_if( possibilities.begin(), possibilities.end(),
                                             [goldValueLimit]( const MP2::MapObjectType object ) { return getObjectGoldValue( object ) > goldValueLimit; } ),
                             possibilities.end() );

        if ( possibilities.empty() ) {
            return {};
        }

        return convertMP2ToObjectInfo( Rand::GetWithGen( possibilities, randomGenerator ) );
    }

    std::pair<ObjectGroup, int32_t> convertMP2ToObjectInfo( const MP2::MapObjectType mp2Type )
    {
        static std::map<MP2::MapObjectType, std::pair<Maps::ObjectGroup, int32_t>> lookup;

        if ( lookup.empty() ) {
            const std::vector<ObjectGroup> limitedGroupList{ ObjectGroup::ADVENTURE_ARTIFACTS, ObjectGroup::ADVENTURE_DWELLINGS, ObjectGroup::ADVENTURE_MINES,
                                                             ObjectGroup::ADVENTURE_POWER_UPS, ObjectGroup::ADVENTURE_TREASURES, ObjectGroup::MONSTERS };

            for ( const auto & group : limitedGroupList ) {
                const auto & groupObjects = Maps::getObjectsByGroup( group );
                for ( size_t index = 0; index < groupObjects.size(); ++index ) {
                    const MP2::MapObjectType type = groupObjects[index].objectType;
                    lookup.try_emplace( type, std::make_pair( group, static_cast<int32_t>( index ) ) );
                }
            }
        }

        const auto it = lookup.find( mp2Type );
        if ( it != lookup.end() ) {
            return it->second;
        }

        return {};
    }

    int32_t selectTerrainVariantForObject( const ObjectGroup groupType, const int32_t objectIndex, const int32_t groundType )
    {
        if ( groupType == ObjectGroup::LANDSCAPE_TREES && objectIndex < 6 ) {
            return treeTypeFromGroundType( groundType ) + objectIndex;
        }

        if ( groupType == ObjectGroup::LANDSCAPE_MOUNTAINS && objectIndex < 8 ) {
            return mountainTypeFromGroundType( groundType ) + objectIndex;
        }
        return objectIndex;
    }

    std::vector<int32_t> findPathToNearestRoad( const MapStateManager & nodes, const int32_t mapWidth, const uint32_t regionId, const int32_t start )
    {
        assert( start > 0 );
        assert( mapWidth > 0 );
        assert( start < mapWidth * mapWidth );

        std::vector<RoadBuilderNode> cache( static_cast<size_t>( mapWidth ) * mapWidth );
        assert( start < static_cast<int32_t>( cache.size() ) );

        std::vector<int32_t> result;
        std::vector<int32_t> nodesToExplore;
        nodesToExplore.push_back( start );
        cache[static_cast<size_t>( start )]._from = start;
        cache[static_cast<size_t>( start )]._cost = 0;

        const bool fromActionTile = ( nodes.getNode( start ).type == Maps::Random_Generator::NodeType::ACTION );

        int32_t bestRoadIndex = -1;
        uint32_t bestRoadCost = std::numeric_limits<uint32_t>::max();
        for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
            const int32_t currentNodeIdx = nodesToExplore[lastProcessedNode];
            const RoadBuilderNode & currentNode = cache[currentNodeIdx];

            // Continue to search until margin is hit to find a potential optimal route
            if ( bestRoadIndex != -1 && currentNode._cost > bestRoadCost + roadBuilderMargin ) {
                break;
            }

            const bool comeFromStraight = !Direction::isDiagonal( currentNode._direction );

            if ( comeFromStraight && currentNodeIdx != start && currentNode._cost < bestRoadCost && nodes.getNode( currentNodeIdx ).type == NodeType::PATH ) {
                bestRoadIndex = currentNodeIdx;
                bestRoadCost = currentNode._cost;
            }

            for ( const int32_t direction : Direction::allNeighboringDirections ) {
                if ( !Maps::isValidDirection( currentNodeIdx, direction ) ) {
                    continue;
                }

                const bool isDiagonal = Direction::isDiagonal( direction );
                // Edge case: force straight roads out of action tiles (e.g. mines)
                if ( fromActionTile && currentNodeIdx == start && ( isDiagonal || direction == Direction::TOP ) ) {
                    continue;
                }

                // Edge case: avoid tight road turns for a better visuals
                if ( isDiagonal && currentNode._from != -1 ) {
                    const int32_t previousDirection = cache[currentNode._from]._direction;

                    if ( previousDirection != direction && Direction::isDiagonal( previousDirection ) ) {
                        continue;
                    }
                    if ( previousDirection == Direction::BOTTOM && ( currentNode._direction == Direction::LEFT || currentNode._direction == Direction::RIGHT ) ) {
                        continue;
                    }
                }

                const int32_t newIndex = Maps::GetDirectionIndex( currentNodeIdx, direction );
                if ( newIndex == start ) {
                    continue;
                }

                const Node & other = nodes.getNode( newIndex );

                if ( other.region != regionId ) {
                    continue;
                }

                if ( other.type != NodeType::OPEN && other.type != NodeType::PATH && other.type != NodeType::CONNECTOR ) {
                    continue;
                }

                RoadBuilderNode & newNode = cache[newIndex];
                constexpr uint32_t diagonalMovePenalty = Ground::defaultGroundPenalty * 3 / 2;
                const uint32_t movementPenalty = isDiagonal ? diagonalMovePenalty : Ground::defaultGroundPenalty;
                const uint32_t movementCost = currentNode._cost + movementPenalty;

                if ( newNode._from == -1 || newNode._cost > movementCost ) {
                    newNode._from = currentNodeIdx;
                    newNode._cost = movementCost;
                    newNode._direction = direction;

                    nodesToExplore.push_back( newIndex );
                }
            }
        }

        if ( bestRoadIndex == -1 ) {
            return result;
        }

        int32_t currentStep = bestRoadIndex;

        while ( true ) {
            const RoadBuilderNode & rbNode = cache[static_cast<size_t>( currentStep )];

            // Adding additional 0 cost road step to fix road transitions to compensate for missing sprites
            if ( result.size() == 1 ) {
                if ( rbNode._direction == Direction::BOTTOM_RIGHT ) {
                    result.emplace_back( Maps::GetDirectionIndex( currentStep, Direction::LEFT ) );
                }
                else if ( rbNode._direction == Direction::BOTTOM_LEFT ) {
                    result.emplace_back( Maps::GetDirectionIndex( currentStep, Direction::RIGHT ) );
                }
            }

            result.emplace_back( currentStep );

            if ( currentStep == rbNode._from ) {
                break;
            }

            currentStep = rbNode._from;
        }

        return result;
    }

    std::vector<std::vector<int32_t>> findOpenTilesSortedJittered( const Region & region, int32_t mapWidth, Rand::PCG32 & randomGenerator )
    {
        if ( region.centerIndex < 0 || region.nodes.empty() || mapWidth <= 0 ) {
            return {};
        }

        std::vector<std::vector<int32_t>> buckets;

        const int32_t centerX = region.centerIndex % mapWidth;
        const int32_t centerY = region.centerIndex / mapWidth;

        for ( const Node & node : region.nodes ) {
            if ( node.type != NodeType::OPEN || node.index < 0 ) {
                continue;
            }

            const int32_t dx = ( node.index % mapWidth ) - centerX;
            const int32_t dy = ( node.index / mapWidth ) - centerY;

            const double distance = std::sqrt( static_cast<double>( dx * dx + dy * dy ) );
            const uint32_t noise = static_cast<int>( Rand::GetWithGen( 0, 2, randomGenerator ) );
            const size_t ring = static_cast<size_t>( std::floor( distance ) + noise );
            if ( ring >= buckets.size() ) {
                buckets.resize( ring + 1 );
            }

            buckets[ring].push_back( node.index );
        }

        for ( auto & bucket : buckets ) {
            if ( bucket.empty() ) {
                continue;
            }

            Rand::ShuffleWithGen( bucket, randomGenerator );
        }
        return buckets;
    }

    std::vector<int32_t> findTilesByType( const Region & region, const NodeType type )
    {
        std::vector<int32_t> result;
        for ( const Node & node : region.nodes ) {
            if ( node.type == type ) {
                result.push_back( node.index );
            }
        }
        return result;
    }

    std::vector<int32_t> pickEvenlySpacedTiles( const std::vector<int32_t> & candidates, const size_t pickCount, const std::vector<int32_t> & avoidance )
    {
        assert( !candidates.empty() );
        assert( pickCount > 0 );

        const size_t candidatesCount = candidates.size();
        const size_t targetCount = ( pickCount > candidatesCount ) ? candidatesCount : pickCount;

        std::vector<int32_t> result;
        result.reserve( targetCount );

        std::vector<std::pair<uint32_t, bool>> cache;
        cache.reserve( candidatesCount );

        for ( const int32_t candidate : candidates ) {
            uint32_t minDistance = std::numeric_limits<uint32_t>::max();
            for ( const int32_t avoid : avoidance ) {
                const uint32_t distance = Maps::GetApproximateDistance( candidate, avoid );
                if ( distance < minDistance ) {
                    minDistance = distance;
                }
            }
            cache.emplace_back( minDistance, false );
        }

        auto selectNextIndex = [&]() {
            int32_t bestCandidateIndex = -1;
            uint32_t bestDistance = 0;

            // We're searching for the largest distance that hasn't been used yet.
            for ( size_t idx = 0; idx < cache.size(); ++idx ) {
                if ( cache[idx].second ) {
                    continue;
                }

                if ( cache[idx].first > bestDistance ) {
                    bestDistance = cache[idx].first;
                    bestCandidateIndex = static_cast<int32_t>( idx );
                }
            }

            return bestCandidateIndex;
        };

        for ( size_t placed = 0; placed < targetCount; ++placed ) {
            const int32_t pointIndex = selectNextIndex();
            if ( pointIndex < 0 ) {
                break;
            }

            const int32_t chosenPoint = candidates[pointIndex];

            cache[pointIndex].second = true;
            result.push_back( chosenPoint );

            // Update distance for remaining candidates using the newly chosen point.
            for ( size_t idx = 0; idx < cache.size(); ++idx ) {
                if ( cache[idx].second ) {
                    continue;
                }

                const uint32_t distance = Maps::GetApproximateDistance( candidates[idx], chosenPoint );
                if ( distance < cache[idx].first ) {
                    cache[idx].first = distance;
                }
            }
        }

        return result;
    }

    bool canPlaceBorderObstacle( const MapStateManager & data, const ObjectInfo & info, const fheroes2::Point & position )
    {
        bool validPlacement = true;

        auto updateObjectArea = [&data, &position, &validPlacement]( const auto & partInfo ) {
            const Node & node = data.getNode( position + partInfo.tileOffset );

            if ( node.index == -1 || node.region == 0 ) {
                validPlacement = false;
                return;
            }
            if ( node.type != NodeType::OPEN && node.type != NodeType::BORDER && node.type != NodeType::OBSTACLE ) {
                validPlacement = false;
                return;
            }
        };

        iterateOverObjectParts( info, updateObjectArea );

        for ( const auto & objectPart : info.topLevelParts ) {
            const Node & node = data.getNode( position + objectPart.tileOffset );
            if ( node.index == -1 || node.region == 0 ) {
                return false;
            }
        }

        return validPlacement;
    }

    bool canPlaceAllObjects( const MapStateManager & data, const std::vector<ObjectPlacement> & objects, const fheroes2::Point & position, const int32_t ground )
    {
        return std::all_of( objects.begin(), objects.end(), [&ground, &data, &position]( const ObjectPlacement & object ) {
            const int32_t objectIndex = selectTerrainVariantForObject( object.groupType, object.objectIndex, ground );
            return canPlaceObject( data, Maps::getObjectInfo( object.groupType, objectIndex ), position + object.offset );
        } );
    }

    bool canFitObjectSet( const MapStateManager & data, const ObjectSet & set, const fheroes2::Point & position, const int32_t ground )
    {
        for ( const fheroes2::Point offset : set.entranceCheck ) {
            const Node & node = data.getNode( position + offset );
            if ( node.index == -1 || node.type != NodeType::OPEN ) {
                return false;
            }
        }

        for ( const auto * placements : { &set.obstacles, &set.valuables, &randomMonsterSet } ) {
            if ( !canPlaceAllObjects( data, *placements, position, ground ) ) {
                return false;
            }
        }

        return true;
    }

    void markObjectPlacement( MapStateManager & data, const ObjectInfo & info, const fheroes2::Point & position )
    {
        iterateOverObjectParts( info, [&data, &position]( const auto & partInfo ) { markNodeAsType( data, position + partInfo.tileOffset, NodeType::OBSTACLE ); } );

        if ( !MP2::isOffGameActionObject( info.objectType ) ) {
            return;
        }

        markNodeAsType( data, position, NodeType::ACTION );
    }

    // Wouldn't render correctly but will speed up placement
    void forceTempRoadOnTile( Map_Format::MapFormat & mapFormat, const int32_t tileIndex )
    {
        if ( Maps::doesContainRoad( mapFormat.tiles[tileIndex] ) ) {
            return;
        }

        const auto & objectInfo = Maps::getObjectInfo( Maps::ObjectGroup::ROADS, 2 );
        if ( objectInfo.empty() ) {
            assert( 0 );
            return;
        }

        // We just increase the UID counter to use the last UID in `Maps::addObjectToMap()`.
        // All road objects will be updated and placed to the `world` tiles only after all of the road parts are placet to the `Map_Format`.
        Maps::getNewObjectUID();

        Maps::addObjectToMap( mapFormat, tileIndex, Maps::ObjectGroup::ROADS, 2 );
    }

    bool putObjectOnMap( Map_Format::MapFormat & mapFormat, Tile & tile, const ObjectGroup groupType, const int32_t objectIndex )
    {
        const auto & objectInfo = Maps::getObjectInfo( groupType, objectIndex );
        if ( objectInfo.empty() ) {
            assert( 0 );
            return false;
        }

        if ( MP2::isOffGameActionObject( objectInfo.objectType ) ) {
            const auto & tileObjects = mapFormat.tiles[tile.GetIndex()].objects;
            const bool tileHasActionObject = std::any_of( tileObjects.cbegin(), tileObjects.cend(), []( const Maps::Map_Format::TileObjectInfo & tileObjectinfo ) {
                const auto & info = Maps::getObjectInfo( tileObjectinfo.group, static_cast<int32_t>( tileObjectinfo.index ) );
                return MP2::isOffGameActionObject( info.objectType );
            } );

            if ( tileHasActionObject ) {
                // Two action objects cannot be placed on one tile.
                return false;
            }
        }

        // Maps::setObjectOnTile isn't idempotent, check if object was already placed
        if ( tile.getMainObjectType() == objectInfo.objectType && MP2::isInGameActionObject( objectInfo.objectType ) ) {
            return false;
        }

        // Do not update passabilities after every object placement.
        if ( !Maps::setObjectOnTile( tile, objectInfo, false ) ) {
            return false;
        }

        Maps::addObjectToMap( mapFormat, tile.GetIndex(), groupType, static_cast<uint32_t>( objectIndex ) );

        return true;
    }

    bool placeActionObject( Map_Format::MapFormat & mapFormat, MapStateManager & data, Tile & tile, const ObjectGroup groupType, const int32_t type )
    {
        const fheroes2::Point tilePos = tile.GetCenter();
        const auto & objectInfo = Maps::getObjectInfo( groupType, type );

        if ( canPlaceObject( data, objectInfo, tilePos ) ) {
            MapStateTransaction transaction( data );
            markObjectPlacement( data, objectInfo, tilePos );

            const int32_t tileIndex = tile.GetIndex();
            const auto & roadToObject = findPathToNearestRoad( data, mapFormat.width, data.getNode( tileIndex ).region, tileIndex );
            if ( roadToObject.empty() ) {
                return false;
            }

            if ( putObjectOnMap( mapFormat, tile, groupType, type ) ) {
                for ( const auto & step : roadToObject ) {
                    markNodeIndexAsType( data, step, NodeType::PATH );
                    forceTempRoadOnTile( mapFormat, step );
                }
                transaction.commit();
                return true;
            }
        }

        return false;
    }

    bool placeCastle( Map_Format::MapFormat & mapFormat, MapStateManager & data, const Region & region, const fheroes2::Point tilePos, const bool isCastle )
    {
        auto & tile = world.getTile( tilePos.x, tilePos.y );
        if ( tile.isWater() ) {
            return false;
        }

        const int32_t basementId = fheroes2::getTownBasementId( tile.GetGround() );
        const int32_t castleObjectId = isCastle ? randomCastleIndex : randomTownIndex;

        const auto & basementInfo = Maps::getObjectInfo( ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId );
        const auto & castleInfo = Maps::getObjectInfo( ObjectGroup::KINGDOM_TOWNS, castleObjectId );

        if ( !canPlaceObject( data, basementInfo, tilePos ) || !canPlaceObject( data, castleInfo, tilePos ) ) {
            return false;
        }

        if ( !putObjectOnMap( mapFormat, tile, ObjectGroup::LANDSCAPE_TOWN_BASEMENTS, basementId ) ) {
            return false;
        }

        // Since the whole object consists of multiple "objects" we have to put the same ID for all of them.
        // Every time an object is being placed on a map the counter is going to be increased by 1.
        // Therefore, we set the counter by 1 less for each object to match object UID for all of them.
        assert( Maps::getLastObjectUID() > 0 );
        const uint32_t objectId = Maps::getLastObjectUID() - 1;

        Maps::setLastObjectUID( objectId );

        if ( !putObjectOnMap( mapFormat, tile, ObjectGroup::KINGDOM_TOWNS, castleObjectId ) ) {
            return false;
        }

        // By default use random (default) army for the neutral race town/castle.
        const PlayerColor color = Color::IndexToColor( region.colorIndex );
        if ( color == PlayerColor::NONE ) {
            Maps::setDefaultCastleDefenderArmy( mapFormat.castleMetadata[Maps::getLastObjectUID()] );
        }

        // Add flags.
        assert( tile.GetIndex() > 0 && tile.GetIndex() < world.w() * world.h() - 1 );
        Maps::setLastObjectUID( objectId );

        if ( !putObjectOnMap( mapFormat, world.getTile( tile.GetIndex() - 1 ), ObjectGroup::LANDSCAPE_FLAGS, Color::GetIndex( color ) * 2 ) ) {
            return false;
        }

        Maps::setLastObjectUID( objectId );

        if ( !putObjectOnMap( mapFormat, world.getTile( tile.GetIndex() + 1 ), ObjectGroup::LANDSCAPE_FLAGS, Color::GetIndex( color ) * 2 + 1 ) ) {
            return false;
        }

        const int32_t bottomIndex = Maps::GetDirectionIndex( tile.GetIndex(), Direction::BOTTOM );

        if ( color != PlayerColor::NONE ) {
            const int32_t spriteIndex = Color::GetIndex( color ) * randomHeroIndex + ( randomHeroIndex - 1 );
            putObjectOnMap( mapFormat, world.getTile( bottomIndex ), ObjectGroup::KINGDOM_HEROES, spriteIndex );
        }

        markObjectPlacement( data, basementInfo, tilePos );
        markObjectPlacement( data, castleInfo, tilePos );

        // Force roads coming from the castle
        const int32_t nextIndex = Maps::GetDirectionIndex( bottomIndex, Direction::BOTTOM );
        if ( Maps::isValidAbsIndex( nextIndex ) ) {
            markNodeIndexAsType( data, bottomIndex, NodeType::PATH );
            forceTempRoadOnTile( mapFormat, bottomIndex );
            forceTempRoadOnTile( mapFormat, nextIndex );
        }

        return true;
    }

    int32_t placeMine( Map_Format::MapFormat & mapFormat, MapStateManager & data, MapEconomy & economy, const std::vector<int32_t> & tileOptions, const int32_t resource,
                       const MonsterStrength monsterStrength )
    {
        for ( const int32_t nodeIndex : tileOptions ) {
            Tile & mineTile = world.getTile( nodeIndex );
            const int32_t mineType = fheroes2::getMineObjectInfoId( resource, mineTile.GetGround() );
            if ( placeActionObject( mapFormat, data, mineTile, ObjectGroup::ADVENTURE_MINES, mineType ) ) {
                economy.increaseMineCount( resource );

                const int32_t mineValue = getObjectGoldValue( getFakeMP2MineType( resource ) );
                placeMonster( mapFormat, data, Maps::GetDirectionIndex( nodeIndex, Direction::BOTTOM ), getMonstersByValue( monsterStrength, mineValue ) );
                return nodeIndex;
            }
        }
        return -1;
    }

    bool placeBorderObstacle( Map_Format::MapFormat & mapFormat, MapStateManager & data, const Node & node, const int32_t ground, Rand::PCG32 & randomGenerator )
    {
        Tile & tile = world.getTile( node.index );
        const auto it = obstaclesPerGround.find( ground );
        if ( it == obstaclesPerGround.end() || it->second.empty() ) {
            return false;
        }

        std::vector<int> obstacleList = it->second;
        Rand::ShuffleWithGen( obstacleList, randomGenerator );

        const fheroes2::Point tilePos = tile.GetCenter();
        for ( const auto & obstacleId : obstacleList ) {
            const auto & objectInfo = Maps::getObjectInfo( ObjectGroup::LANDSCAPE_TREES, obstacleId );
            if ( canPlaceBorderObstacle( data, objectInfo, tilePos ) && putObjectOnMap( mapFormat, tile, ObjectGroup::LANDSCAPE_TREES, obstacleId ) ) {
                markObjectPlacement( data, objectInfo, tilePos );
                return true;
            }
        }
        return false;
    }

    void placeMonster( Map_Format::MapFormat & mapFormat, MapStateManager & data, const int32_t index, const MonsterSelection & monster )
    {
        if ( monster.monsterId == Monster::UNKNOWN || !Maps::isValidAbsIndex( index ) ) {
            return;
        }

        if ( putObjectOnMap( mapFormat, world.getTile( index ), ObjectGroup::MONSTERS, static_cast<int32_t>( Monster( monster.monsterId ).GetSpriteIndex() ) ) ) {
            markNodeIndexAsType( data, index, NodeType::ACTION );
        }

        if ( monster.allowedMonsters.empty() ) {
            return;
        }

        for ( const Map_Format::TileObjectInfo & info : mapFormat.tiles[index].objects ) {
            if ( info.group == ObjectGroup::MONSTERS ) {
                mapFormat.monsterMetadata[info.id].selected = monster.allowedMonsters;
                return;
            }
        }
    }

    bool placeSimpleObject( Map_Format::MapFormat & mapFormat, MapStateManager & data, const Node & centerNode, const ObjectPlacement & placement, const int32_t ground )
    {
        const fheroes2::Point position = Maps::GetPoint( centerNode.index ) + placement.offset;
        if ( !Maps::isValidAbsPoint( position.x, position.y ) ) {
            return false;
        }

        Tile & tile = world.getTile( position.x, position.y );
        const int32_t objectIndex = selectTerrainVariantForObject( placement.groupType, placement.objectIndex, ground );
        const auto & objectInfo = Maps::getObjectInfo( placement.groupType, objectIndex );
        if ( putObjectOnMap( mapFormat, tile, placement.groupType, objectIndex ) ) {
            markObjectPlacement( data, objectInfo, position );
            return true;
        }
        return false;
    }

    std::vector<int32_t> findTilesForPlacement( MapStateManager & data, const int32_t mapWidth, const uint32_t regionId, const std::vector<int32_t> & nodes,
                                                const ObjectInfo & objectInfo )
    {
        // Use the below object to revert all changes made for MapStateManager
        // as this function shouldn't do any changes yet.
        const MapStateTransaction transaction( data );

        std::vector<int32_t> options;

        for ( const int32_t & nodeIndex : nodes ) {
            const fheroes2::Point mapPoint = Maps::GetPoint( nodeIndex );

            if ( !canPlaceObject( data, objectInfo, mapPoint ) ) {
                continue;
            }

            MapStateTransaction secondaryTx( data );
            markObjectPlacement( data, objectInfo, mapPoint );

            const auto & routeToObject = findPathToNearestRoad( data, mapWidth, regionId, nodeIndex );
            if ( routeToObject.empty() ) {
                continue;
            }

            for ( const auto & step : routeToObject ) {
                markNodeIndexAsType( data, step, NodeType::PATH );
            }
            secondaryTx.commit();

            options.push_back( nodeIndex );

            if ( options.size() > maxPlacementAttempts ) {
                break;
            }
        }

        return options;
    }

    std::vector<std::pair<int32_t, ObjectSet>> planObjectPlacement( MapStateManager & data, const int32_t mapWidth, const Region & region,
                                                                    std::vector<ObjectSet> objectSets, Rand::PCG32 & randomGenerator )
    {
        if ( region.treasureLimit <= 0 ) {
            return {};
        }

        // Use the below object to revert all changes made for MapStateManager
        // as this function shouldn't do any changes yet.
        const MapStateTransaction transaction( data );

        Rand::ShuffleWithGen( objectSets, randomGenerator );

        int32_t treasureLimit = region.treasureLimit;
        std::vector<std::pair<int32_t, ObjectSet>> objectSetsPlanned;

        std::vector<int32_t> openTiles = findTilesByType( region, NodeType::OPEN );
        Rand::ShuffleWithGen( openTiles, randomGenerator );

        size_t attempt = 0;
        for ( const int32_t nodeIndex : openTiles ) {
            if ( attempt >= maxPlacementAttempts || treasureLimit <= 0 ) {
                break;
            }
            ++attempt;

            const fheroes2::Point mapPoint = Maps::GetPoint( nodeIndex );

            for ( const auto & prefab : objectSets ) {
                if ( !canFitObjectSet( data, prefab, mapPoint, region.groundType ) ) {
                    continue;
                }

                // Start transaction so we can revert a single object set if no path will be found.
                MapStateTransaction secondaryTx( data );
                for ( const auto & obstacle : prefab.obstacles ) {
                    const fheroes2::Point position = mapPoint + obstacle.offset;
                    const auto & objectInfo = Maps::getObjectInfo( obstacle.groupType, obstacle.objectIndex );
                    markObjectPlacement( data, objectInfo, position );
                }

                const auto & routeToGroup = findPathToNearestRoad( data, mapWidth, region.id, nodeIndex );
                if ( routeToGroup.empty() ) {
                    continue;
                }

                for ( const auto & step : routeToGroup ) {
                    markNodeIndexAsType( data, step, NodeType::PATH );
                }

                for ( const auto & treasure : prefab.valuables ) {
                    const fheroes2::Point position = Maps::GetPoint( nodeIndex ) + treasure.offset;
                    const auto & objectInfo = Maps::getObjectInfo( treasure.groupType, treasure.objectIndex );
                    markObjectPlacement( data, objectInfo, position );

                    treasureLimit -= getObjectGoldValue( treasure.groupType, treasure.objectIndex );
                }
                secondaryTx.commit();

                objectSetsPlanned.emplace_back( nodeIndex, prefab );
                break;
            }
        }
        return objectSetsPlanned;
    }

    void placeValidTreasures( Map_Format::MapFormat & mapFormat, MapStateManager & data, Region & region, const ObjectSet & objectSet, const int32_t tileIndex,
                              const MonsterStrength monsterStrength, Rand::PCG32 & randomGenerator )
    {
        const Node & node = data.getNode( tileIndex );

        for ( const auto & obstacle : objectSet.obstacles ) {
            if ( !placeSimpleObject( mapFormat, data, node, obstacle, region.groundType ) ) {
                // Validate that object set can be placed before calling this!
                assert( 0 );
            }
        }

        const int32_t groupValueLimit = std::min( region.treasureLimit, maximumTreasureGroupValue );
        int32_t groupValue = 0;
        for ( const auto & treasure : objectSet.valuables ) {
            if ( treasure.groupType == ObjectGroup::ADVENTURE_POWER_UPS ) {
                placeSimpleObject( mapFormat, data, node, treasure, region.groundType );
                groupValue += getObjectGoldValue( treasure.groupType, treasure.objectIndex );
            }
            else {
                const int32_t valueLimit = std::max( minimalTreasureValue, groupValueLimit - groupValue );

                // Randomize what treasure to pick
                const auto & [groupType, objectIndex] = getRandomTreasure( valueLimit, randomGenerator );
                placeSimpleObject( mapFormat, data, node, { treasure.offset, groupType, objectIndex }, region.groundType );
                groupValue += getObjectGoldValue( groupType, objectIndex );
            }
        }
        // It is possible to go into the negatives; intentional
        region.treasureLimit -= groupValue;

        placeMonster( mapFormat, data, node.index, getMonstersByValue( monsterStrength, groupValue ) );
    }

    void placeObjectSet( Map_Format::MapFormat & mapFormat, MapStateManager & data, Region & region, std::vector<ObjectSet> objectSets,
                         const MonsterStrength monsterStrength, const uint8_t expectedCount, Rand::PCG32 & randomGenerator )
    {
        int32_t objectsPlaced = 0;
        for ( size_t attempt = 0; attempt < maxPlacementAttempts; ++attempt ) {
            if ( objectsPlaced == expectedCount || region.treasureLimit <= 0 ) {
                break;
            }

            const Node & node = Rand::GetWithGen( region.nodes, randomGenerator );

            Rand::ShuffleWithGen( objectSets, randomGenerator );
            for ( const auto & prefab : objectSets ) {
                if ( !canFitObjectSet( data, prefab, Maps::GetPoint( node.index ), region.groundType ) ) {
                    continue;
                }

                MapStateTransaction transaction( data );
                for ( const auto & obstacle : prefab.obstacles ) {
                    const fheroes2::Point position = Maps::GetPoint( node.index ) + obstacle.offset;
                    const auto & objectInfo = Maps::getObjectInfo( obstacle.groupType, obstacle.objectIndex );
                    markObjectPlacement( data, objectInfo, position );
                }

                const auto & routeToGroup = findPathToNearestRoad( data, mapFormat.width, region.id, node.index );
                if ( routeToGroup.empty() ) {
                    continue;
                }

                for ( const auto & step : routeToGroup ) {
                    markNodeIndexAsType( data, step, NodeType::PATH );
                    forceTempRoadOnTile( mapFormat, step );
                }
                transaction.commit();

                for ( const auto & obstacle : prefab.obstacles ) {
                    placeSimpleObject( mapFormat, data, node, obstacle, region.groundType );
                }

                const int32_t groupValueLimit = std::min( region.treasureLimit, maximumTreasureGroupValue );
                int32_t groupValue = 0;
                for ( const auto & treasure : prefab.valuables ) {
                    if ( treasure.groupType == ObjectGroup::ADVENTURE_POWER_UPS ) {
                        placeSimpleObject( mapFormat, data, node, { treasure.offset, treasure.groupType, treasure.objectIndex }, region.groundType );
                        groupValue += getObjectGoldValue( treasure.groupType, treasure.objectIndex );
                    }
                    else {
                        const int32_t valueLimit = std::max( minimalTreasureValue, groupValueLimit - groupValue );

                        const auto & [groupType, objectIndex] = getRandomTreasure( valueLimit, randomGenerator );
                        placeSimpleObject( mapFormat, data, node, { treasure.offset, groupType, objectIndex }, region.groundType );
                        groupValue += getObjectGoldValue( groupType, objectIndex );
                    }
                }
                // It is possible to go into the negatives; intentional
                region.treasureLimit -= groupValue;

                placeMonster( mapFormat, data, node.index, getMonstersByValue( monsterStrength, groupValue ) );

                ++objectsPlaced;
                break;
            }
        }
    }

    void placeDecorations( Map_Format::MapFormat & mapFormat, MapStateManager & data, Region & region, const std::vector<DecorationSet> & decorations,
                           Rand::PCG32 & randomGenerator )
    {
        if ( decorations.empty() ) {
            return;
        }

        std::vector<int32_t> openTiles = findTilesByType( region, NodeType::OPEN );

        const auto openSpaceFilter = [&data]( const int32_t idx ) {
            return std::any_of( Direction::allNeighboringDirections.begin(), Direction::allNeighboringDirections.end(), [&]( const auto direction ) {
                if ( !Maps::isValidDirection( idx, direction ) ) {
                    return false;
                }

                return data.getNode( Maps::GetDirectionIndex( idx, direction ) ).type != NodeType::OPEN;
            } );
        };
        openTiles.erase( std::remove_if( openTiles.begin(), openTiles.end(), openSpaceFilter ), openTiles.end() );

        if ( openTiles.empty() ) {
            return;
        }

        const int32_t individualObjectCopies = std::max( 1, static_cast<int32_t>( openTiles.size() ) / 300 );
        std::vector<int32_t> objectCount( decorations.size(), individualObjectCopies );

        const size_t possibilitiesCount = decorations.size() * individualObjectCopies * 3;
        const std::vector<int32_t> tileIndicies = pickEvenlySpacedTiles( openTiles, possibilitiesCount, {} );

        for ( const int32_t tileIndex : tileIndicies ) {
            const size_t setIndex = Rand::GetWithGen( 0, static_cast<uint32_t>( decorations.size() - 1 ), randomGenerator );
            if ( objectCount[setIndex] <= 0 ) {
                continue;
            }
            const auto & objectSet = decorations[setIndex];

            const fheroes2::Point position = Maps::GetPoint( tileIndex );
            if ( !canPlaceAllObjects( data, objectSet.obstacles, position, region.groundType ) ) {
                continue;
            }

            const Node & node = data.getNode( tileIndex );

            for ( const auto & obstacle : objectSet.obstacles ) {
                placeSimpleObject( mapFormat, data, node, obstacle, region.groundType );
            }
            for ( const auto & obstacle : objectSet.optional ) {
                const int32_t objectIndex = selectTerrainVariantForObject( obstacle.groupType, obstacle.objectIndex, region.groundType );
                const auto & objectInfo = Maps::getObjectInfo( obstacle.groupType, objectIndex );
                if ( Rand::GetWithGen( 0, 1, randomGenerator ) && canPlaceBorderObstacle( data, objectInfo, position + obstacle.offset ) ) {
                    placeSimpleObject( mapFormat, data, node, obstacle, region.groundType );
                }
            }

            --objectCount[setIndex];
        }
    }
}
