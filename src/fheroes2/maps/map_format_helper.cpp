/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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

#include "map_format_helper.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <utility>

#include "army.h"
#include "army_troop.h"
#include "castle.h"
#include "color.h"
#include "gamedefs.h"
#include "heroes.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "mp2.h"
#include "race.h"
#include "world.h"
#include "world_object_uid.h"

namespace
{
    void addObjectToTile( Maps::Map_Format::TileInfo & info, const Maps::ObjectGroup group, const uint32_t index, const uint32_t uid )
    {
        auto & object = info.objects.emplace_back();
        object.id = uid;
        object.group = group;
        object.index = index;
    }

    struct IndexedObjectInfo
    {
        int32_t tileIndex{ -1 };

        const Maps::Map_Format::TileObjectInfo * info{ nullptr };
    };

    void loadArmyFromMetadata( Army & army, const std::array<int32_t, 5> & unitType, const std::array<int32_t, 5> & unitCount )
    {
        std::vector<Troop> troops( unitType.size() );
        for ( size_t i = 0; i < troops.size(); ++i ) {
            assert( unitType[i] >= 0 && unitCount[i] >= 0 );
            troops[i] = Troop{ unitType[i], static_cast<uint32_t>( unitCount[i] ) };
        }

        army.Assign( troops.data(), troops.data() + troops.size() );
    }

    void saveArmyToMetadata( const Army & army, std::array<int32_t, 5> & unitType, std::array<int32_t, 5> & unitCount )
    {
        const size_t armySize = army.Size();
        assert( unitType.size() == armySize );

        // Update army metadata.
        for ( size_t i = 0; i < armySize; ++i ) {
            const Troop * troop = army.GetTroop( i );
            assert( troop != nullptr );

            unitType[i] = troop->GetID();
            unitCount[i] = static_cast<int32_t>( troop->GetCount() );
        }
    }

    void updateWorldCastlesHeroes( const Maps::Map_Format::MapFormat & map )
    {
        const auto & townObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_TOWNS );
        const auto & heroObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_HEROES );

        for ( size_t i = 0; i < map.tiles.size(); ++i ) {
            for ( const auto & object : map.tiles[i].objects ) {
                if ( object.group == Maps::ObjectGroup::KINGDOM_TOWNS ) {
                    const uint8_t color = Color::IndexToColor( Maps::getTownColorIndex( map, i, object.id ) );
                    const uint8_t race = Race::IndexToRace( static_cast<int>( townObjects[object.index].metadata[0] ) );

                    world.addCastle( static_cast<int32_t>( i ), race, color );
                }
                else if ( object.group == Maps::ObjectGroup::KINGDOM_HEROES ) {
                    const auto & metadata = heroObjects[object.index].metadata;
                    const uint8_t color = Color::IndexToColor( static_cast<int>( metadata[0] ) );

                    Heroes * hero = world.GetHeroForHire( static_cast<int>( metadata[1] ) );
                    if ( hero ) {
                        hero->SetCenter( { static_cast<int32_t>( i ) % world.w(), static_cast<int32_t>( i ) / world.w() } );
                        hero->SetColor( color );
                    }
                }
            }
        }
    }
}

namespace Maps
{
    bool readMapInEditor( const Map_Format::MapFormat & map )
    {
        world.generateForEditor( map.size );

        if ( !readAllTiles( map ) ) {
            return false;
        }

        world.updatePassabilities();

        updateWorldCastlesHeroes( map );

        return true;
    }

    bool readAllTiles( const Map_Format::MapFormat & map )
    {
        assert( static_cast<size_t>( world.w() ) * world.h() == map.tiles.size() );

        // We must clear all tiles before writing something on them.
        for ( size_t i = 0; i < map.tiles.size(); ++i ) {
            auto & tile = world.GetTiles( static_cast<int32_t>( i ) );
            tile = {};

            tile.setIndex( static_cast<int32_t>( i ) );
        }

        for ( size_t i = 0; i < map.tiles.size(); ++i ) {
            readTileTerrain( world.GetTiles( static_cast<int32_t>( i ) ), map.tiles[i] );
        }

        // Read objects from all tiles and place them based on their IDs.
        auto sortObjects = []( const IndexedObjectInfo & left, const IndexedObjectInfo & right ) { return left.info->id < right.info->id; };
        std::multiset<IndexedObjectInfo, decltype( sortObjects )> sortedObjects( sortObjects );

        for ( size_t i = 0; i < map.tiles.size(); ++i ) {
            for ( const auto & object : map.tiles[i].objects ) {
                IndexedObjectInfo info;
                info.tileIndex = static_cast<int32_t>( i );
                info.info = &object;
                sortedObjects.emplace( info );
            }
        }

        for ( const auto & info : sortedObjects ) {
            assert( info.info != nullptr );
            if ( !readTileObject( world.GetTiles( info.tileIndex ), *info.info ) ) {
                return false;
            }
        }

        return true;
    }

    bool saveMapInEditor( Map_Format::MapFormat & map )
    {
        assert( world.w() > 0 && world.w() == world.h() );

        map.size = world.w();

        const size_t size = static_cast<size_t>( map.size ) * map.size;

        map.tiles.resize( size );

        for ( size_t i = 0; i < size; ++i ) {
            writeTile( world.GetTiles( static_cast<int32_t>( i ) ), map.tiles[i] );
        }

        return true;
    }

    void readTileTerrain( Tiles & tile, const Map_Format::TileInfo & info )
    {
        tile.setTerrain( info.terrainIndex, info.terrainFlag & 2, info.terrainFlag & 1 );
    }

    bool readTileObject( Tiles & tile, const Map_Format::TileObjectInfo & object )
    {
        const auto & objectInfos = getObjectsByGroup( object.group );
        if ( object.index >= objectInfos.size() ) {
            // This is a bad map format!
            assert( 0 );
            return false;
        }

        // Object UID is set through global object UID counter. Therefore, we need to update it before running the operation.
        if ( object.id == 0 ) {
            // This object UID is not set!
            assert( 0 );
            return false;
        }

        setLastObjectUID( object.id - 1 );
        // We don't update map passabilities as it is a very expensive process.
        // Let's do it once everything is being loaded.
        return setObjectOnTile( tile, objectInfos[object.index], false );
    }

    void writeTile( const Tiles & tile, Map_Format::TileInfo & info )
    {
        // A tile cannot contain an exactly the same road or stream parts.
        std::set<std::pair<uint32_t, uint8_t>> roadParts;
        std::set<std::pair<uint32_t, uint8_t>> streamParts;

        const MP2::ObjectIcnType mainObjectIcnType = tile.getObjectIcnType();
        if ( mainObjectIcnType == MP2::OBJ_ICN_TYPE_ROAD ) {
            roadParts.emplace( tile.GetObjectUID(), tile.GetObjectSpriteIndex() );
        }
        else if ( mainObjectIcnType == MP2::OBJ_ICN_TYPE_STREAM ) {
            streamParts.emplace( tile.GetObjectUID(), tile.GetObjectSpriteIndex() );
        }

        for ( const auto & addon : tile.getBottomLayerAddons() ) {
            if ( addon._objectIcnType == MP2::OBJ_ICN_TYPE_ROAD ) {
                roadParts.emplace( addon._uid, addon._imageIndex );
            }
            else if ( addon._objectIcnType == MP2::OBJ_ICN_TYPE_STREAM ) {
                streamParts.emplace( addon._uid, addon._imageIndex );
            }
        }

        for ( size_t objectIndex = 0; objectIndex < info.objects.size(); ) {
            auto & object = info.objects[objectIndex];

            if ( object.group == ObjectGroup::ROADS ) {
                if ( roadParts.empty() ) {
                    // This object was removed from the tile. Delete the object.
                    info.objects.erase( info.objects.begin() + static_cast<std::vector<Maps::Map_Format::TileObjectInfo>::difference_type>( objectIndex ) );
                    continue;
                }

                object.id = roadParts.begin()->first;
                object.index = roadParts.begin()->second;
                roadParts.erase( roadParts.begin() );
            }
            else if ( object.group == ObjectGroup::STREAMS ) {
                if ( streamParts.empty() ) {
                    // This object was removed from the tile. Delete the object.
                    info.objects.erase( info.objects.begin() + static_cast<std::vector<Maps::Map_Format::TileObjectInfo>::difference_type>( objectIndex ) );
                    continue;
                }

                object.id = streamParts.begin()->first;
                object.index = streamParts.begin()->second;
                streamParts.erase( streamParts.begin() );
            }

            ++objectIndex;
        }

        for ( const auto & [uid, index] : roadParts ) {
            addObjectToTile( info, ObjectGroup::ROADS, index, uid );
        }

        for ( const auto & [uid, index] : streamParts ) {
            addObjectToTile( info, ObjectGroup::STREAMS, index, uid );
        }

        info.terrainIndex = tile.getTerrainImageIndex();
        info.terrainFlag = tile.getTerrainFlags();
    }

    void addObjectToMap( Map_Format::MapFormat & map, const int32_t tileId, const ObjectGroup group, const uint32_t index )
    {
        assert( tileId >= 0 && map.tiles.size() > static_cast<size_t>( tileId ) );

        // At this time it is assumed that object was added into world object to be rendered using Maps::setObjectOnTile() function.
        const uint32_t uid = getLastObjectUID();
        assert( uid > 0 );

        addObjectToTile( map.tiles[tileId], group, index, uid );

        // Towns and heroes have extra metadata.
        if ( group == ObjectGroup::KINGDOM_HEROES ) {
            auto [heroMetadata, isMetadataEmplaced] = map.heroMetadata.try_emplace( uid );
            assert( isMetadataEmplaced );

            const auto & objects = Maps::getObjectsByGroup( group );
            assert( index < objects.size() );
            // Set race according the object metadata.
            heroMetadata->second.race = Race::IndexToRace( static_cast<int>( objects[index].metadata[1] ) );
        }
        else if ( group == ObjectGroup::KINGDOM_TOWNS ) {
            auto [metadata, isMetadataEmplaced] = map.castleMetadata.try_emplace( uid );
            assert( isMetadataEmplaced );

            const auto & objects = Maps::getObjectsByGroup( group );
            assert( index < objects.size() );
            // Add town or castle main buildings.
            metadata->second.builtBuildings.push_back( objects[index].metadata[1] == 0 ? BUILD_TENT : BUILD_CASTLE );
        }
        else if ( isJailObject( group, index ) ) {
            auto [heroMetadata, isMetadataEmplaced] = map.heroMetadata.try_emplace( uid );
            assert( isMetadataEmplaced );

            // Set Random race for the jailed hero by default.
            heroMetadata->second.race = Race::RAND;
        }
        else if ( group == ObjectGroup::MONSTERS ) {
            const auto [dummy, isMetadataEmplaced] = map.standardMetadata.try_emplace( uid );
            assert( isMetadataEmplaced );

#ifdef NDEBUG
            (void)isMetadataEmplaced;
#endif
        }
        else if ( group == Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS ) {
            const auto & objects = Maps::getObjectsByGroup( group );

            assert( index < objects.size() );
            const auto objectType = objects[index].objectType;

            switch ( objectType ) {
            case MP2::OBJ_EVENT: {
                const auto [dummy, isMetadataEmplaced] = map.adventureMapEventMetadata.try_emplace( uid );
                assert( isMetadataEmplaced );

#ifdef NDEBUG
                (void)isMetadataEmplaced;
#endif
                break;
            }
            case MP2::OBJ_SIGN: {
                const auto [dummy, isMetadataEmplaced] = map.signMetadata.try_emplace( uid );
                assert( isMetadataEmplaced );

#ifdef NDEBUG
                (void)isMetadataEmplaced;
#endif
                break;
            }
            case MP2::OBJ_SPHINX: {
                const auto [dummy, isMetadataEmplaced] = map.sphinxMetadata.try_emplace( uid );
                assert( isMetadataEmplaced );

#ifdef NDEBUG
                (void)isMetadataEmplaced;
#endif
                break;
            }
            default:
                break;
            }
        }
        else if ( group == Maps::ObjectGroup::ADVENTURE_WATER ) {
            const auto & objects = Maps::getObjectsByGroup( group );

            assert( index < objects.size() );
            const auto objectType = objects[index].objectType;
            if ( objectType == MP2::OBJ_BOTTLE ) {
                const auto [dummy, isMetadataEmplaced] = map.signMetadata.try_emplace( uid );
                assert( isMetadataEmplaced );

#ifdef NDEBUG
                (void)isMetadataEmplaced;
#endif
            }
        }
        else if ( group == Maps::ObjectGroup::ADVENTURE_ARTIFACTS ) {
            assert( index < Maps::getObjectsByGroup( group ).size() );

            const auto [dummy, isMetadataEmplaced] = map.standardMetadata.try_emplace( uid );
            assert( isMetadataEmplaced );

#ifdef NDEBUG
            (void)isMetadataEmplaced;
#endif
        }
    }

    bool updateMapPlayers( Map_Format::MapFormat & map )
    {
        static_assert( Color::BLUE == 1 << 0, "The kingdom color values have changed. You are going to break map format!" );
        static_assert( Color::GREEN == 1 << 1, "The kingdom color values have changed. You are going to break map format!" );
        static_assert( Color::RED == 1 << 2, "The kingdom color values have changed. You are going to break map format!" );
        static_assert( Color::YELLOW == 1 << 3, "The kingdom color values have changed. You are going to break map format!" );
        static_assert( Color::ORANGE == 1 << 4, "The kingdom color values have changed. You are going to break map format!" );
        static_assert( Color::PURPLE == 1 << 5, "The kingdom color values have changed. You are going to break map format!" );

        static_assert( Race::NONE == 0, "The race values have changed. You are going to break map format!" );
        static_assert( Race::KNGT == 1 << 0, "The race values have changed. You are going to break map format!" );
        static_assert( Race::BARB == 1 << 1, "The race values have changed. You are going to break map format!" );
        static_assert( Race::SORC == 1 << 2, "The race values have changed. You are going to break map format!" );
        static_assert( Race::WRLK == 1 << 3, "The race values have changed. You are going to break map format!" );
        static_assert( Race::WZRD == 1 << 4, "The race values have changed. You are going to break map format!" );
        static_assert( Race::NECR == 1 << 5, "The race values have changed. You are going to break map format!" );
        static_assert( Race::MULT == 1 << 6, "The race values have changed. You are going to break map format!" );
        static_assert( Race::RAND == 1 << 7, "The race values have changed. You are going to break map format!" );

        constexpr size_t mainColors{ KINGDOMMAX };

        if ( map.playerRace.size() != mainColors ) {
            // Possibly corrupted map.
            assert( 0 );
            return false;
        }

        // Gather all information about all kingdom colors and races.
        std::array<bool, mainColors> heroColorsPresent{ false };
        // Towns can be neutral so 1 more color for them.
        std::array<bool, mainColors + 1> townColorsPresent{ false };
        std::array<uint8_t, mainColors> heroRacesPresent{ 0 };
        std::array<uint8_t, mainColors + 1> townRacesPresent{ 0 };

        const auto & heroObjects = getObjectsByGroup( ObjectGroup::KINGDOM_HEROES );
        const auto & townObjects = getObjectsByGroup( ObjectGroup::KINGDOM_TOWNS );

        for ( size_t tileIndex = 0; tileIndex < map.tiles.size(); ++tileIndex ) {
            const auto & mapTile = map.tiles[tileIndex];

            for ( const auto & object : mapTile.objects ) {
                if ( object.group == ObjectGroup::KINGDOM_HEROES ) {
                    if ( object.index >= heroObjects.size() ) {
                        assert( 0 );
                        return false;
                    }

                    const auto & metadata = heroObjects[object.index].metadata;

                    const uint32_t color = metadata[0];
                    if ( color >= heroColorsPresent.size() ) {
                        assert( 0 );
                        return false;
                    }

                    heroColorsPresent[color] = true;

                    const uint32_t race = metadata[1];
                    heroRacesPresent[color] |= ( 1 << race );
                }
                else if ( object.group == ObjectGroup::KINGDOM_TOWNS ) {
                    if ( object.index >= townObjects.size() ) {
                        assert( 0 );
                        return false;
                    }

                    // Towns and Castles have 2 flags on both sides on the entrance. Verify, that they exist and have the same color.
                    assert( tileIndex > 0 && tileIndex < map.tiles.size() - 1 );

                    const uint8_t color = getTownColorIndex( map, tileIndex, object.id );

                    if ( color >= townColorsPresent.size() ) {
                        assert( 0 );
                        return false;
                    }

                    townColorsPresent[color] = true;

                    const uint32_t race = townObjects[object.index].metadata[0];
                    townRacesPresent[color] |= ( 1 << race );
                }
            }
        }

        // Update map format settings based on the gathered information.
        map.availablePlayerColors = 0;
        for ( size_t i = 0; i < mainColors; ++i ) {
            map.playerRace[i] = ( heroRacesPresent[i] | townRacesPresent[i] );

            if ( map.playerRace[i] != 0 ) {
                map.availablePlayerColors += static_cast<uint8_t>( 1 << i );
            }

            // Only one race can be present.
            if ( ( map.playerRace[i] & Race::RAND ) != 0 ) {
                map.playerRace[i] = Race::RAND;
            }
            if ( ( map.playerRace[i] & Race::MULT ) != 0 ) {
                map.playerRace[i] = Race::MULT;
            }
            else {
                size_t raceCount = 0;
                for ( uint8_t raceIdx = 0; raceIdx < 6; ++raceIdx ) {
                    if ( ( map.playerRace[i] & ( 1 << raceIdx ) ) != 0 ) {
                        ++raceCount;
                    }
                }

                if ( raceCount > 1 ) {
                    map.playerRace[i] = Race::MULT;
                }
            }
        }

        map.computerPlayerColors = map.computerPlayerColors & map.availablePlayerColors;
        map.humanPlayerColors = map.humanPlayerColors & map.availablePlayerColors;

        if ( map.availablePlayerColors > 0 ) {
            // Human and computer player colors might be set previously. If they are, do not update them.
            for ( size_t i = 0; i < mainColors; ++i ) {
                const uint8_t color = static_cast<uint8_t>( 1 << i );
                if ( ( map.availablePlayerColors & color ) != 0 && ( map.computerPlayerColors & color ) == 0 && ( map.humanPlayerColors & color ) == 0 ) {
                    // This color was not set for anyone.
                    map.computerPlayerColors |= color;
                    map.humanPlayerColors |= color;
                }
            }

            // Make sure that at least one human color exist.
            if ( map.humanPlayerColors == 0 ) {
                for ( size_t i = 0; i < mainColors; ++i ) {
                    const uint8_t color = static_cast<uint8_t>( 1 << i );
                    if ( ( map.availablePlayerColors & color ) != 0 ) {
                        map.humanPlayerColors |= color;
                        break;
                    }
                }
            }

            if ( !map.alliances.empty() ) {
                // Verify that alliances are set correctly:
                // - each alliance has at least one color
                // - no color should be repeated more than once
                std::array<bool, mainColors> usedAllianceColors{ false };

                for ( auto iter = map.alliances.begin(); iter != map.alliances.end(); ) {
                    uint8_t & allianceColors = *iter;

                    for ( size_t i = 0; i < mainColors; ++i ) {
                        const uint8_t color = static_cast<uint8_t>( 1 << i );
                        if ( ( allianceColors & color ) != 0 && usedAllianceColors[i] ) {
                            // This color is used in another alliance. Remove it from here.
                            allianceColors = allianceColors & ( ~color );
                        }
                    }

                    if ( allianceColors == 0 ) {
                        // This alliance is invalid.
                        iter = map.alliances.erase( iter );
                        continue;
                    }

                    ++iter;
                }

                if ( map.alliances.size() == 1 ) {
                    // Everyone cannot be in one alliance!
                    map.alliances.clear();
                }
                else {
                    // Check that all colors being used in alliances. If not then add missing colors to the last alliance.
                    for ( size_t i = 0; i < mainColors; ++i ) {
                        const uint8_t color = static_cast<uint8_t>( 1 << i );
                        if ( ( map.availablePlayerColors & color ) != 0 && !usedAllianceColors[i] ) {
                            if ( map.alliances.empty() ) {
                                map.alliances.push_back( color );
                            }
                            else {
                                map.alliances.back() |= color;
                            }
                        }
                    }
                }
            }
        }
        else {
            // No colors are set so no alliances should exist.
            map.alliances = { 0 };

            // No races are set.
            map.playerRace = { 0 };
        }

        // Update events according to the possible changes in human and/or AI player colors.
        for ( auto & [dummy, eventMetadata] : map.adventureMapEventMetadata ) {
            eventMetadata.humanPlayerColors = eventMetadata.humanPlayerColors & map.humanPlayerColors;
            eventMetadata.computerPlayerColors = eventMetadata.computerPlayerColors & map.computerPlayerColors;
        }

        // Check and update the special victory and loss conditions that depend on player objects.

        // Returns true if all is OK.
        auto checkSpecialCondition = [&map, &heroObjects, &townObjects]( const std::vector<uint32_t> & conditionMetadata, const ObjectGroup objectGroup ) {
            if ( conditionMetadata.size() != 2 ) {
                return false;
            }

            // Verify that this is a valid map object.
            const uint32_t tileIndex = conditionMetadata[0];

            assert( tileIndex < map.tiles.size() );

            for ( const auto & object : map.tiles[tileIndex].objects ) {
                if ( object.group != objectGroup ) {
                    continue;
                }

                switch ( objectGroup ) {
                case Maps::ObjectGroup::KINGDOM_TOWNS: {
                    if ( object.index >= townObjects.size() ) {
                        assert( 0 );
                        continue;
                    }

                    const uint32_t color = Color::IndexToColor( Maps::getTownColorIndex( map, tileIndex, object.id ) );
                    if ( color != conditionMetadata[1] ) {
                        // Current town color is incorrect.
                        continue;
                    }

                    return true;
                }
                case Maps::ObjectGroup::KINGDOM_HEROES: {
                    if ( object.index >= heroObjects.size() ) {
                        assert( 0 );
                        continue;
                    }

                    const uint32_t color = 1 << heroObjects[object.index].metadata[0];
                    if ( color != conditionMetadata[1] ) {
                        // Current hero color is incorrect.
                        continue;
                    }

                    return true;
                }
                default:
                    // Have you added a new object type for victory or loss conditions? Update the logic!
                    assert( 0 );
                    break;
                }
            }

            return false;
        };

        switch ( map.victoryConditionType ) {
        case Maps::FileInfo::VICTORY_CAPTURE_TOWN:
            if ( !checkSpecialCondition( map.victoryConditionMetadata, Maps::ObjectGroup::KINGDOM_TOWNS ) ) {
                map.victoryConditionMetadata.clear();
                map.victoryConditionType = Maps::FileInfo::VICTORY_DEFEAT_EVERYONE;
            }

            break;
        case Maps::FileInfo::VICTORY_KILL_HERO:
            if ( !checkSpecialCondition( map.victoryConditionMetadata, Maps::ObjectGroup::KINGDOM_HEROES ) ) {
                map.victoryConditionMetadata.clear();
                map.victoryConditionType = Maps::FileInfo::VICTORY_DEFEAT_EVERYONE;
            }

            break;
        default:
            break;
        }

        switch ( map.lossConditionType ) {
        case Maps::FileInfo::LOSS_TOWN:
            if ( !checkSpecialCondition( map.lossConditionMetadata, Maps::ObjectGroup::KINGDOM_TOWNS ) ) {
                map.lossConditionMetadata.clear();
                map.lossConditionType = Maps::FileInfo::LOSS_EVERYTHING;
            }

            break;
        case Maps::FileInfo::LOSS_HERO:
            if ( !checkSpecialCondition( map.lossConditionMetadata, Maps::ObjectGroup::KINGDOM_HEROES ) ) {
                map.lossConditionMetadata.clear();
                map.lossConditionType = Maps::FileInfo::LOSS_EVERYTHING;
            }

            break;
        default:
            break;
        }

        return true;
    }

    uint8_t getTownColorIndex( const Map_Format::MapFormat & map, const size_t tileIndex, const uint32_t id )
    {
        if ( map.tiles.empty() ) {
            assert( 0 );
            return 0;
        }

        if ( tileIndex == 0 || tileIndex >= map.tiles.size() - 1 ) {
            assert( 0 );
            return 0;
        }

        const auto & flagObjects = getObjectsByGroup( ObjectGroup::LANDSCAPE_FLAGS );

        uint32_t leftFlagColor = 0;
        uint32_t rightFlagColor = 0;

        for ( const auto & tempObject : map.tiles[tileIndex - 1].objects ) {
            if ( tempObject.group == ObjectGroup::LANDSCAPE_FLAGS && tempObject.id == id ) {
                if ( tempObject.index >= flagObjects.size() ) {
                    assert( 0 );
                    return 0;
                }

                leftFlagColor = flagObjects[tempObject.index].metadata[0];
                break;
            }
        }

        for ( const auto & tempObject : map.tiles[tileIndex + 1].objects ) {
            if ( tempObject.group == ObjectGroup::LANDSCAPE_FLAGS && tempObject.id == id ) {
                if ( tempObject.index >= flagObjects.size() ) {
                    assert( 0 );
                    return 0;
                }

                rightFlagColor = flagObjects[tempObject.index].metadata[0];
                break;
            }
        }

        if ( leftFlagColor != rightFlagColor ) {
            assert( 0 );
            return 0;
        }

        return static_cast<uint8_t>( leftFlagColor );
    }

    bool isJailObject( const ObjectGroup group, const uint32_t index )
    {
        return ( group == Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS && getObjectInfo( group, static_cast<int32_t>( index ) ).objectType == MP2::OBJ_JAIL );
    }

    uint32_t getBuildingsFromVector( const std::vector<uint32_t> & buildingsVector )
    {
        uint32_t buildings{ BUILD_NOTHING };
        for ( const uint32_t building : buildingsVector ) {
            buildings |= building;
        }

        return buildings;
    }

    void setDefaultCastleDefenderArmy( Map_Format::CastleMetadata & metadata )
    {
        for ( int32_t & type : metadata.defenderMonsterType ) {
            type = -1;
        }
        for ( int32_t & count : metadata.defenderMonsterCount ) {
            count = 0;
        }
    }

    bool isDefaultCastleDefenderArmy( const Map_Format::CastleMetadata & metadata )
    {
        return std::all_of( metadata.defenderMonsterType.begin(), metadata.defenderMonsterType.end(), []( const int32_t type ) { return type < 0; } );
    }

    bool loadCastleArmy( Army & army, const Map_Format::CastleMetadata & metadata )
    {
        if ( isDefaultCastleDefenderArmy( metadata ) ) {
            return false;
        }

        loadArmyFromMetadata( army, metadata.defenderMonsterType, metadata.defenderMonsterCount );
        return true;
    }

    void saveCastleArmy( const Army & army, Map_Format::CastleMetadata & metadata )
    {
        saveArmyToMetadata( army, metadata.defenderMonsterType, metadata.defenderMonsterCount );
    }

    bool loadHeroArmy( Army & army, const Map_Format::HeroMetadata & metadata )
    {
        if ( std::all_of( metadata.armyMonsterType.begin(), metadata.armyMonsterType.end(), []( const int32_t type ) { return type <= 0; } ) ) {
            // There is no custom army.
            return false;
        }

        loadArmyFromMetadata( army, metadata.armyMonsterType, metadata.armyMonsterCount );
        return true;
    }

    void saveHeroArmy( const Army & army, Map_Format::HeroMetadata & metadata )
    {
        saveArmyToMetadata( army, metadata.armyMonsterType, metadata.armyMonsterCount );
    }
}
