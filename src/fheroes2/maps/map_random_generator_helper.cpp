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
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <map>
#include <utility>

#include "color.h"
#include "direction.h"
#include "ground.h"
#include "map_format_helper.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "map_random_generator_info.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "monster.h"
#include "mp2.h"
#include "race.h"
#include "rand.h"
#include "ui_map_object.h"
#include "world.h"
#include "world_object_uid.h"

namespace
{
    constexpr int randomCastleIndex{ 12 };
    constexpr int randomTownIndex{ 13 };
    constexpr int maxPlacementAttempts{ 30 };

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
        { MP2::OBJ_ABANDONED_MINE, 6000 },
        { MP2::OBJ_ALCHEMIST_LAB, 2000 },
        { MP2::OBJ_MINE, 2000 },
        { MP2::OBJ_SAWMILL, 750 },
        // Pickups.
        { MP2::OBJ_CAMPFIRE, 1000 },
        { MP2::OBJ_GENIE_LAMP, 6000 },
        { MP2::OBJ_RANDOM_ARTIFACT_MAJOR, 10000 },
        { MP2::OBJ_RANDOM_ARTIFACT_MINOR, 4500 },
        { MP2::OBJ_RANDOM_ARTIFACT_TREASURE, 2000 },
        { MP2::OBJ_RANDOM_RESOURCE, minimalTreasureValue },
        { MP2::OBJ_RESOURCE, minimalTreasureValue },
        { MP2::OBJ_TREASURE_CHEST, 1500 },
        // Powerups.
        { MP2::OBJ_FORT, 2000 },
        { MP2::OBJ_GAZEBO, 1500 },
        { MP2::OBJ_MERCENARY_CAMP, 2000 },
        { MP2::OBJ_SHRINE_FIRST_CIRCLE, 1000 },
        { MP2::OBJ_SHRINE_SECOND_CIRCLE, 2000 },
        { MP2::OBJ_SHRINE_THIRD_CIRCLE, 3000 },
        { MP2::OBJ_STANDING_STONES, 2000 },
        { MP2::OBJ_TREE_OF_KNOWLEDGE, 3000 },
        { MP2::OBJ_WITCH_DOCTORS_HUT, 2000 },
        { MP2::OBJ_XANADU, 8000 },
        // Dwellings.
        { MP2::OBJ_AIR_ALTAR, 4000 },
        { MP2::OBJ_ARCHER_HOUSE, 2000 },
        { MP2::OBJ_CAVE, 1500 },
        { MP2::OBJ_DESERT_TENT, 3000 },
        { MP2::OBJ_DWARF_COTTAGE, 2000 },
        { MP2::OBJ_EARTH_ALTAR, 4000 },
        { MP2::OBJ_EXCAVATION, 1500 },
        { MP2::OBJ_FIRE_ALTAR, 4000 },
        { MP2::OBJ_GOBLIN_HUT, 1500 },
        { MP2::OBJ_HALFLING_HOLE, 1500 },
        { MP2::OBJ_PEASANT_HUT, minimalTreasureValue },
        { MP2::OBJ_RUINS, 4000 },
        { MP2::OBJ_TREE_CITY, 1500 },
        { MP2::OBJ_TREE_HOUSE, 1500 },
        { MP2::OBJ_WAGON_CAMP, 2000 },
        { MP2::OBJ_WATCH_TOWER, 2000 },
        { MP2::OBJ_WATER_ALTAR, 4000 },
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

    void markNodeAsType( Maps::Random_Generator::NodeCache & data, const fheroes2::Point position, const Maps::Random_Generator::NodeType type )
    {
        if ( Maps::isValidAbsPoint( position.x, position.y ) ) {
            data.getNode( position ).type = type;
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

    MonsterSelection getMonstersByValue( const int32_t protectedObjectValue )
    {
        if ( protectedObjectValue > 8500 ) {
            // 171 -> 504 monster strength
            return { Monster::RANDOM_MONSTER_LEVEL_4,
                     { Monster::GIANT, Monster::GENIE, Monster::PHOENIX, Monster::BONE_DRAGON, Monster::GREEN_DRAGON, Monster::RED_DRAGON, Monster::TITAN,
                       Monster::BLACK_DRAGON } };
        }
        if ( protectedObjectValue > 6000 ) {
            // 49 -> 137 monster strength
            return { Monster::RANDOM_MONSTER,
                     { Monster::TROLL, Monster::CHAMPION, Monster::LICH, Monster::WAR_TROLL, Monster::MAGE, Monster::UNICORN, Monster::HYDRA, Monster::VAMPIRE_LORD,
                       Monster::ARCHMAGE, Monster::POWER_LICH, Monster::GHOST, Monster::PALADIN, Monster::CRUSADER, Monster::CYCLOPS, Monster::GENIE } };
        }
        if ( protectedObjectValue > 4500 ) {
            // 27 -> 48 monster strength
            return { Monster::RANDOM_MONSTER_LEVEL_3,
                     { Monster::MASTER_SWORDSMAN, Monster::EARTH_ELEMENT, Monster::AIR_ELEMENT, Monster::WATER_ELEMENT, Monster::FIRE_ELEMENT, Monster::DRUID,
                       Monster::GREATER_DRUID, Monster::MINOTAUR, Monster::CAVALRY, Monster::OGRE_LORD, Monster::ROC, Monster::VAMPIRE, Monster::MEDUSA,
                       Monster::MINOTAUR_KING, Monster::TROLL, Monster::CHAMPION } };
        }
        if ( protectedObjectValue > 3000 ) {
            // 17 -> 31 monster strength
            return { Monster::RANDOM_MONSTER,
                     { Monster::VETERAN_PIKEMAN, Monster::MUMMY, Monster::NOMAD, Monster::IRON_GOLEM, Monster::ELF, Monster::ROYAL_MUMMY, Monster::WOLF,
                       Monster::GRAND_ELF, Monster::SWORDSMAN, Monster::OGRE, Monster::STEEL_GOLEM, Monster::GRIFFIN, Monster::MASTER_SWORDSMAN, Monster::EARTH_ELEMENT,
                       Monster::AIR_ELEMENT, Monster::WATER_ELEMENT, Monster::FIRE_ELEMENT } };
        }
        if ( protectedObjectValue > 1500 ) {
            // 11 -> 18 monster strength
            return { Monster::RANDOM_MONSTER_LEVEL_2, {} };
        }
        if ( protectedObjectValue > 750 ) {
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

    bool canFitObject( const NodeCache & data, const ObjectInfo & info, const fheroes2::Point & mainTilePos, const bool skipBorders )
    {
        bool invalid = false;
        fheroes2::Rect objectRect;

        auto updateObjectArea = [&data, &mainTilePos, &objectRect, skipBorders, &invalid]( const auto & partInfo ) {
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

    bool canFitObjectSet( const NodeCache & data, const ObjectSet & set, const fheroes2::Point & mainTilePos )
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

    void markObjectPlacement( NodeCache & data, const ObjectInfo & info, const fheroes2::Point & mainTilePos )
    {
        fheroes2::Rect objectRect;

        auto updateObjectArea = [&data, &mainTilePos, &objectRect]( const auto & partInfo ) {
            Node & node = data.getNode( mainTilePos + partInfo.tileOffset );
            objectRect.x = std::min( objectRect.x, partInfo.tileOffset.x );
            objectRect.width = std::max( objectRect.width, partInfo.tileOffset.x );

            node.type = NodeType::OBSTACLE;
        };

        iterateOverObjectParts( info, updateObjectArea );

        if ( !MP2::isOffGameActionObject( info.objectType ) ) {
            return;
        }

        for ( int x = objectRect.x - 1; x <= objectRect.width + 1; ++x ) {
            markNodeAsType( data, mainTilePos + fheroes2::Point{ x, objectRect.height + 1 }, NodeType::PATH );
        }

        // Mark extra nodes as path to avoid objects clumping together
        markNodeAsType( data, mainTilePos + fheroes2::Point{ objectRect.x - 1, 0 }, NodeType::PATH );
        markNodeAsType( data, mainTilePos + fheroes2::Point{ objectRect.width + 1, 0 }, NodeType::PATH );

        markNodeAsType( data, mainTilePos, NodeType::ACTION );
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

    bool placeActionObject( Map_Format::MapFormat & mapFormat, NodeCache & data, Tile & tile, const ObjectGroup groupType, const int32_t type )
    {
        const fheroes2::Point tilePos = tile.GetCenter();
        const auto & objectInfo = Maps::getObjectInfo( groupType, type );
        if ( canFitObject( data, objectInfo, tilePos, true ) && putObjectOnMap( mapFormat, tile, groupType, type ) ) {
            markObjectPlacement( data, objectInfo, tilePos );
            return true;
        }

        return false;
    }

    bool placeCastle( Map_Format::MapFormat & mapFormat, NodeCache & data, const Region & region, const fheroes2::Point tilePos, const bool isCastle )
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

        if ( !putObjectOnMap( mapFormat, world.getTile( tile.GetIndex() - 1 ), ObjectGroup::LANDSCAPE_FLAGS, Color::GetIndex( color ) * 2 ) ) {
            return false;
        }

        Maps::setLastObjectUID( objectId );

        if ( !putObjectOnMap( mapFormat, world.getTile( tile.GetIndex() + 1 ), ObjectGroup::LANDSCAPE_FLAGS, Color::GetIndex( color ) * 2 + 1 ) ) {
            return false;
        }

        const ObjectInfo & townObjectInfo = Maps::getObjectInfo( ObjectGroup::KINGDOM_TOWNS, castleObjectId );
        const uint8_t race = Race::IndexToRace( static_cast<int>( townObjectInfo.metadata[0] ) );

        world.addCastle( tile.GetIndex(), race, color );

        markObjectPlacement( data, basementInfo, tilePos );
        markObjectPlacement( data, castleInfo, tilePos );

        // Force roads coming from the castle
        const int32_t nextIndex = Maps::GetDirectionIndex( bottomIndex, Direction::BOTTOM );
        if ( Maps::isValidAbsIndex( nextIndex ) ) {
            Maps::updateRoadOnTile( mapFormat, bottomIndex, true );
            Maps::updateRoadOnTile( mapFormat, nextIndex, true );
        }

        return true;
    }

    bool placeMine( Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & node, const int resource )
    {
        Tile & mineTile = world.getTile( node.index );
        const int32_t mineType = fheroes2::getMineObjectInfoId( resource, mineTile.GetGround() );
        return placeActionObject( mapFormat, data, mineTile, ObjectGroup::ADVENTURE_MINES, mineType );
    }

    bool placeRandomObstacle( Map_Format::MapFormat & mapFormat, const NodeCache & data, const Node & node, Rand::PCG32 & randomGenerator )
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

    bool placeSimpleObject( Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & centerNode, const ObjectPlacement & placement )
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

    void placeObjectSet( Map_Format::MapFormat & mapFormat, NodeCache & data, Region & region, std::vector<ObjectSet> objects, const uint8_t expectedCount,
                         Rand::PCG32 & randomGenerator )
    {
        int objectsPlaced = 0;
        for ( int attempt = 0; attempt < maxPlacementAttempts; ++attempt ) {
            if ( objectsPlaced == expectedCount || region.treasureLimit < 0 ) {
                break;
            }

            const Node & node = Rand::GetWithGen( region.nodes, randomGenerator );

            Rand::ShuffleWithGen( objects, randomGenerator );
            for ( const auto & prefab : objects ) {
                if ( !canFitObjectSet( data, prefab, Maps::GetPoint( node.index ) ) ) {
                    continue;
                }

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

                placeMonster( mapFormat, node.index, getMonstersByValue( groupValue ) );

                ++objectsPlaced;
                break;
            }
        }
    }
}
