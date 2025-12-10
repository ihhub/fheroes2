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

#include "map_random_generator_helper.h"

#include <algorithm>
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
#include "ui_map_object.h"
#include "world.h"
#include "world_object_uid.h"

namespace
{
    struct RoadBuilderNode final
    {
        int32_t _from{ -1 };
        uint32_t _cost{ 0 };
        int _direction{ Direction::UNKNOWN };
    };

    constexpr int randomCastleIndex{ 12 };
    constexpr int randomTownIndex{ 13 };
    constexpr int randomHeroIndex{ 7 };
    constexpr int maxPlacementAttempts{ 30 };
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
        // Powerups.
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

    std::pair<Maps::ObjectGroup, int32_t> convertMP2ToObjectInfo( const MP2::MapObjectType mp2Type )
    {
        static std::map<MP2::MapObjectType, std::pair<Maps::ObjectGroup, int32_t>> lookup;

        if ( lookup.empty() ) {
            const std::vector<Maps::ObjectGroup> limitedGroupList{ Maps::ObjectGroup::ADVENTURE_ARTIFACTS, Maps::ObjectGroup::ADVENTURE_DWELLINGS,
                                                                   Maps::ObjectGroup::ADVENTURE_MINES,     Maps::ObjectGroup::ADVENTURE_POWER_UPS,
                                                                   Maps::ObjectGroup::ADVENTURE_TREASURES, Maps::ObjectGroup::MONSTERS };

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

    bool iterateOverObjectParts( const Maps::ObjectInfo & info, const std::function<void( const Maps::ObjectPartInfo & )> & lambda )
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

    void markNodeAsType( Maps::Random_Generator::MapStateManager & data, const fheroes2::Point position, const Maps::Random_Generator::NodeType type )
    {
        if ( Maps::isValidAbsPoint( position.x, position.y ) ) {
            auto & node = data.getNodeToUpdate( position );
            // Never override border types (no assert needed; can happen when placing overlapping obstacles)
            if ( node.type != Maps::Random_Generator::NodeType::BORDER ) {
                node.type = type;
            }
        }
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
            // 171 -> 504 monster strength
            return { Monster::RANDOM_MONSTER_LEVEL_4,
                     { Monster::GIANT, Monster::GENIE, Monster::PHOENIX, Monster::BONE_DRAGON, Monster::GREEN_DRAGON, Monster::RED_DRAGON, Monster::TITAN,
                       Monster::BLACK_DRAGON } };
        }
        if ( protectedObjectValue >= 9000 ) {
            // 49 -> 137 monster strength
            return { Monster::RANDOM_MONSTER,
                     { Monster::TROLL, Monster::CHAMPION, Monster::LICH, Monster::WAR_TROLL, Monster::MAGE, Monster::UNICORN, Monster::HYDRA, Monster::VAMPIRE_LORD,
                       Monster::ARCHMAGE, Monster::POWER_LICH, Monster::GHOST, Monster::PALADIN, Monster::CRUSADER, Monster::CYCLOPS } };
        }
        if ( protectedObjectValue >= 7000 ) {
            // 27 -> 48 monster strength
            return { Monster::RANDOM_MONSTER_LEVEL_3,
                     { Monster::MASTER_SWORDSMAN, Monster::EARTH_ELEMENT, Monster::AIR_ELEMENT, Monster::WATER_ELEMENT, Monster::FIRE_ELEMENT, Monster::DRUID,
                       Monster::GREATER_DRUID, Monster::MINOTAUR, Monster::CAVALRY, Monster::OGRE_LORD, Monster::ROC, Monster::VAMPIRE, Monster::MEDUSA,
                       Monster::MINOTAUR_KING, Monster::TROLL, Monster::CHAMPION } };
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

    int32_t selectTerrainVariantForObject( const ObjectGroup groupType, const int32_t objectIndex, const int groundType )
    {
        if ( groupType == ObjectGroup::LANDSCAPE_TREES ) {
            assert( objectIndex < 6 );
            return treeTypeFromGroundType( groundType ) + objectIndex;
        }

        if ( groupType == ObjectGroup::LANDSCAPE_MOUNTAINS ) {
            assert( objectIndex < 8 );
            return mountainTypeFromGroundType( groundType ) + objectIndex;
        }
        return objectIndex;
    }

    std::vector<int32_t> findPathToNearestRoad( const Maps::Random_Generator::MapStateManager & nodes, const int32_t mapWidth, const uint32_t regionId,
                                                const int32_t start )
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
        const Directions & directions = Direction::All();

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

            if ( comeFromStraight && currentNodeIdx != start && currentNode._cost < bestRoadCost && world.getTile( currentNodeIdx ).isRoad() ) {
                bestRoadIndex = currentNodeIdx;
                bestRoadCost = currentNode._cost;
            }

            for ( const int direction : directions ) {
                if ( !Maps::isValidDirection( currentNodeIdx, direction ) ) {
                    continue;
                }

                const bool isDiagonal = Direction::isDiagonal( direction );
                // Edge case: force straight roads out of action tiles (e.g. mines)
                if ( fromActionTile && currentNodeIdx == start && isDiagonal ) {
                    continue;
                }

                // Edge case: avoid tight road turns for a better visuals
                if ( isDiagonal && currentNode._from != -1 ) {
                    const int previousDirection = cache[currentNode._from]._direction;

                    if ( previousDirection != direction && Direction::isDiagonal( previousDirection ) ) {
                        continue;
                    }
                    if ( previousDirection == Direction::BOTTOM && ( currentNode._direction == Direction::LEFT || currentNode._direction == Direction::RIGHT ) ) {
                        continue;
                    }
                }

                const int newIndex = Maps::GetDirectionIndex( currentNodeIdx, direction );
                if ( newIndex == start ) {
                    continue;
                }

                const Node & other = nodes.getNode( newIndex );

                if ( other.region != regionId || ( other.type != Maps::Random_Generator::NodeType::OPEN && other.type != Maps::Random_Generator::NodeType::PATH ) ) {
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

        for ( int32_t currentStep = bestRoadIndex;; ) {
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

        const int centerX = region.centerIndex % mapWidth;
        const int centerY = region.centerIndex / mapWidth;

        for ( const Node & node : region.nodes ) {
            if ( node.type != NodeType::OPEN || node.index < 0 ) {
                continue;
            }

            const int dx = ( node.index % mapWidth ) - centerX;
            const int dy = ( node.index / mapWidth ) - centerY;

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

    bool canFitObject( const MapStateManager & data, const ObjectInfo & info, const fheroes2::Point & mainTilePos, const bool skipBorders )
    {
        bool invalid = false;
        fheroes2::Rect objectRect;

        auto updateObjectArea = [&data, &mainTilePos, &objectRect, skipBorders, &invalid]( const auto & partInfo ) {
            const Node & node = data.getNode( mainTilePos + partInfo.tileOffset );

            if ( node.index == -1 || node.region == 0 ) {
                invalid = true;
                return;
            }
            if ( node.type != NodeType::OPEN && ( skipBorders || ( node.type != NodeType::BORDER && node.type != NodeType::OBSTACLE ) ) ) {
                invalid = true;
                return;
            }

            objectRect.x = std::min( objectRect.x, partInfo.tileOffset.x );
            objectRect.width = std::max( objectRect.width, partInfo.tileOffset.x );
        };

        iterateOverObjectParts( info, updateObjectArea );

        if ( invalid ) {
            return false;
        }

        if ( MP2::isOffGameActionObject( info.objectType ) ) {
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

    bool canFitObjectSet( const MapStateManager & data, const ObjectSet & set, const fheroes2::Point & mainTilePos )
    {
        for ( const fheroes2::Point offset : set.entranceCheck ) {
            const Node & node = data.getNode( mainTilePos + offset );
            if ( node.index == -1 || node.type != NodeType::OPEN ) {
                return false;
            }
        }

        for ( const auto * placements : { &set.obstacles, &set.valuables, &randomMonsterSet } ) {
            for ( const auto & placement : *placements ) {
                const auto & info = Maps::getObjectInfo( placement.groupType, placement.objectIndex );
                const bool isAction = MP2::isInGameActionObject( info.objectType );
                const fheroes2::Point position = mainTilePos + placement.offset;
                bool invalid = false;

                auto isValidObjectPart = [&data, &position, isAction, &invalid]( const auto & partInfo ) {
                    const Node & node = data.getNode( position + partInfo.tileOffset );

                    if ( node.index == -1 || node.region == 0 ) {
                        invalid = true;
                        return;
                    }
                    if ( node.type != NodeType::OPEN && ( isAction || node.type != NodeType::BORDER ) ) {
                        invalid = true;
                        return;
                    }
                };

                iterateOverObjectParts( info, isValidObjectPart );

                if ( invalid ) {
                    return false;
                }
            }
        }

        return true;
    }

    void markObjectPlacement( MapStateManager & data, const ObjectInfo & info, const fheroes2::Point & mainTilePos )
    {
        iterateOverObjectParts( info, [&data, &mainTilePos]( const auto & partInfo ) { markNodeAsType( data, mainTilePos + partInfo.tileOffset, NodeType::OBSTACLE ); } );

        if ( !MP2::isOffGameActionObject( info.objectType ) ) {
            return;
        }

        markNodeAsType( data, mainTilePos, NodeType::ACTION );
    }

    // Wouldn't render correctly but will speed up placement
    void forceTempRoadOnTile( Map_Format::MapFormat & mapFormat, const int32_t tileIndex )
    {
        Maps::writeRoadSpriteToTile( mapFormat.tiles[tileIndex], tileIndex, 0 );
    }

    bool putObjectOnMap( Map_Format::MapFormat & mapFormat, Tile & tile, const ObjectGroup groupType, const int32_t objectIndex )
    {
        const auto & objectInfo = Maps::getObjectInfo( groupType, objectIndex );
        if ( objectInfo.empty() ) {
            assert( 0 );
            return false;
        }

        // Maps::setObjectOnTile isn't idempotent, check if object was already placed
        if ( MP2::isInGameActionObject( objectInfo.objectType ) && tile.getMainObjectType() == objectInfo.objectType ) {
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

        if ( canFitObject( data, objectInfo, tilePos, true ) ) {
            MapStateTransaction transaction( data );
            markObjectPlacement( data, objectInfo, tilePos );

            const int32_t tileIndex = tile.GetIndex();
            const auto & roadToObject = findPathToNearestRoad( data, mapFormat.width, data.getNode( tileIndex ).region, tileIndex );
            if ( roadToObject.empty() ) {
                return false;
            }

            if ( putObjectOnMap( mapFormat, tile, groupType, type ) ) {
                for ( const auto & step : roadToObject ) {
                    data.getNodeToUpdate( step ).type = NodeType::PATH;
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

        if ( !canFitObject( data, basementInfo, tilePos, true ) || !canFitObject( data, castleInfo, tilePos, true ) ) {
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
            data.getNodeToUpdate( bottomIndex ).type = NodeType::PATH;
            Maps::updateRoadOnTile( mapFormat, bottomIndex, true );
            Maps::updateRoadOnTile( mapFormat, nextIndex, true );
        }

        return true;
    }

    bool placeMine( Map_Format::MapFormat & mapFormat, MapStateManager & data, const Node & node, const int resource )
    {
        Tile & mineTile = world.getTile( node.index );
        const int32_t mineType = fheroes2::getMineObjectInfoId( resource, mineTile.GetGround() );
        return placeActionObject( mapFormat, data, mineTile, ObjectGroup::ADVENTURE_MINES, mineType );
    }

    bool placeBorderObstacle( Map_Format::MapFormat & mapFormat, MapStateManager & data, const Node & node, Rand::PCG32 & randomGenerator )
    {
        Tile & tile = world.getTile( node.index );
        const auto it = obstaclesPerGround.find( tile.GetGround() );
        if ( it == obstaclesPerGround.end() || it->second.empty() ) {
            return false;
        }

        std::vector<int> obstacleList = it->second;
        Rand::ShuffleWithGen( obstacleList, randomGenerator );

        const fheroes2::Point tilePos = tile.GetCenter();
        for ( const auto & obstacleId : obstacleList ) {
            const auto & objectInfo = Maps::getObjectInfo( ObjectGroup::LANDSCAPE_TREES, obstacleId );
            if ( canFitObject( data, objectInfo, tilePos, false ) && putObjectOnMap( mapFormat, tile, ObjectGroup::LANDSCAPE_TREES, obstacleId ) ) {
                markObjectPlacement( data, objectInfo, tilePos );
                return true;
            }
        }
        return false;
    }

    void placeMonster( Map_Format::MapFormat & mapFormat, const int32_t index, const MonsterSelection & monster )
    {
        if ( monster.monsterId == -1 || !Maps::isValidAbsIndex( index ) ) {
            return;
        }

        putObjectOnMap( mapFormat, world.getTile( index ), ObjectGroup::MONSTERS, static_cast<int32_t>( Monster( monster.monsterId ).GetSpriteIndex() ) );

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

    bool placeSimpleObject( Map_Format::MapFormat & mapFormat, MapStateManager & data, const Node & centerNode, const ObjectPlacement & placement )
    {
        const fheroes2::Point position = Maps::GetPoint( centerNode.index ) + placement.offset;
        if ( !Maps::isValidAbsPoint( position.x, position.y ) ) {
            return false;
        }

        Tile & tile = world.getTile( position.x, position.y );
        const int32_t objectIndex = selectTerrainVariantForObject( placement.groupType, placement.objectIndex, tile.GetGround() );
        const auto & objectInfo = Maps::getObjectInfo( placement.groupType, objectIndex );
        if ( putObjectOnMap( mapFormat, tile, placement.groupType, objectIndex ) ) {
            markObjectPlacement( data, objectInfo, position );
            return true;
        }
        return false;
    }

    void placeObjectSet( Map_Format::MapFormat & mapFormat, MapStateManager & data, Region & region, std::vector<ObjectSet> objectSets,
                         const MonsterStrength monsterStrength, const uint8_t expectedCount, Rand::PCG32 & randomGenerator )
    {
        int objectsPlaced = 0;
        for ( int attempt = 0; attempt < maxPlacementAttempts; ++attempt ) {
            if ( objectsPlaced == expectedCount || region.treasureLimit < 0 ) {
                break;
            }

            const Node & node = Rand::GetWithGen( region.nodes, randomGenerator );

            Rand::ShuffleWithGen( objectSets, randomGenerator );
            for ( const auto & prefab : objectSets ) {
                if ( !canFitObjectSet( data, prefab, Maps::GetPoint( node.index ) ) ) {
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
                    data.getNodeToUpdate( step ).type = NodeType::PATH;
                    forceTempRoadOnTile( mapFormat, step );
                }
                transaction.commit();

                for ( const auto & obstacle : prefab.obstacles ) {
                    placeSimpleObject( mapFormat, data, node, obstacle );
                }

                const int32_t groupValueLimit = std::min( region.treasureLimit, maximumTreasureGroupValue );
                int32_t groupValue = 0;
                for ( const auto & treasure : prefab.valuables ) {
                    ObjectGroup groupType = treasure.groupType;
                    int32_t objectIndex = treasure.objectIndex;
                    if ( treasure.groupType != ObjectGroup::ADVENTURE_POWER_UPS ) {
                        const int32_t valueLimit = std::max( minimalTreasureValue, groupValueLimit - groupValue );
                        const auto & selection = getRandomTreasure( valueLimit, randomGenerator );
                        groupType = selection.first;
                        objectIndex = selection.second;
                    }
                    placeSimpleObject( mapFormat, data, node, { treasure.offset, groupType, objectIndex } );
                    groupValue += getObjectGoldValue( groupType, objectIndex );
                }
                // It is possible to go into the negatives; intentional
                region.treasureLimit -= groupValue;

                placeMonster( mapFormat, node.index, getMonstersByValue( monsterStrength, groupValue ) );

                ++objectsPlaced;
                break;
            }
        }
    }
}
