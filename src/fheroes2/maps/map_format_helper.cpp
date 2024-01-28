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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <set>
#include <vector>

#include "map_format_info.h"
#include "map_object_info.h"
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

        const Maps::Map_Format::ObjectInfo * info{ nullptr };
    };
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

    bool readTileObject( Tiles & tile, const Map_Format::ObjectInfo & object )
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
        // Roads and streams are the only objects that are needed to be saved separately.
        // This is because modification on one tile affects all neighboring tiles as well.
        // Check all existing objects and delete all roads and streams.
        info.objects.erase( std::remove_if( info.objects.begin(), info.objects.end(),
                                            []( const Maps::Map_Format::ObjectInfo & object ) {
                                                return object.group == ObjectGroup::ROADS || object.group == ObjectGroup::STREAMS;
                                            } ),
                            info.objects.end() );

        for ( const auto & addon : tile.getBottomLayerAddons() ) {
            if ( addon._objectIcnType == MP2::OBJ_ICN_TYPE_ROAD || addon._objectIcnType == MP2::OBJ_ICN_TYPE_STREAM ) {
                const ObjectGroup group = ( addon._objectIcnType == MP2::OBJ_ICN_TYPE_ROAD ) ? ObjectGroup::ROADS : ObjectGroup::STREAMS;

                const auto & objectInfos = getObjectsByGroup( group );
                if ( addon._imageIndex < objectInfos.size() ) {
                    // This is the correct object.
                    addObjectToTile( info, group, addon._imageIndex, addon._uid );
                }
            }
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
    }

    void updateMapPlayers( Map_Format::MapFormat & map )
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

        const size_t mainColors{ 6 };

        assert( map.playerRace.size() == mainColors );

        // Gather all information about all kingdom colors and races.
        std::array<bool, mainColors> heroColorsPresent{ false };
        // Towns can be neutral so 1 more color for them.
        std::array<bool, mainColors + 1> townColorsPresent{ false };
        std::array<uint8_t, mainColors> heroRacesPresent{ 0 };
        std::array<uint8_t, mainColors> townRacesPresent{ 0 };

        const auto & heroObjects = getObjectsByGroup( ObjectGroup::KINGDOM_HEROES );
        const auto & townObjects = getObjectsByGroup( ObjectGroup::KINGDOM_TOWNS );

        for ( size_t tileIndex = 0; tileIndex < map.tiles.size(); ++tileIndex ) {
            const auto & mapTile = map.tiles[tileIndex];

            for ( const auto & object : mapTile.objects ) {
                if ( object.group == ObjectGroup::KINGDOM_HEROES ) {
                    assert( object.index < heroObjects.size() );

                    const auto & metadata = heroObjects[object.index].metadata;

                    const uint32_t color = metadata[0];
                    assert( color < heroColorsPresent.size() );

                    heroColorsPresent[color] = true;

                    const uint32_t race = metadata[1];
                    heroRacesPresent[color] |= ( 1 << race );
                }
                else if ( object.group == ObjectGroup::KINGDOM_TOWNS ) {
                    assert( object.index < townObjects.size() );

                    // Towns and Castles have 2 flags on both sides on the entrance. Verify, that they exist and have the same color.
                    assert( tileIndex > 0 && tileIndex < map.tiles.size() - 1 );

                    const uint8_t color = getTownColorIndex( map, tileIndex, object.id );

                    assert( color < townColorsPresent.size() );

                    townColorsPresent[color] = true;

                    const uint32_t race = townObjects[object.index].metadata[0];
                    townRacesPresent[color] |= ( 1 << race );
                }
            }
        }

        // Update map format settings based on the gathered information.
        uint8_t numberOfColorsPresent = 0;

        map.availablePlayerColors = 0;
        for ( size_t i = 0; i < mainColors; ++i ) {
            if ( heroColorsPresent[i] || townColorsPresent[i] ) {
                assert( heroRacesPresent[i] != 0 || townRacesPresent[i] != 0 );

                map.availablePlayerColors += static_cast<uint8_t>( 1 << i );

                ++numberOfColorsPresent;

                map.playerRace[i] &= ( heroRacesPresent[i] | townRacesPresent[i] );

                if ( map.playerRace[i] == 0 ) {
                    map.playerRace[i] = ( heroRacesPresent[i] | townRacesPresent[i] );
                }
            }
            else {
                assert( heroRacesPresent[i] == 0 && townRacesPresent[i] == 0 );

                map.playerRace[i] = 0;
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

            // If only color present this color cannot be used by AI.
            if ( numberOfColorsPresent == 1 ) {
                map.computerPlayerColors = 0;
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
                            map.alliances.back() |= color;
                        }
                    }
                }
            }
        }
        else {
            // No colors are set so no alliances should exist.
            map.alliances = {};

            // No races are set.
            map.playerRace = {};
        }
    }

    uint8_t getTownColorIndex( const Map_Format::MapFormat & map, const size_t tileIndex, const uint32_t id )
    {
        if ( tileIndex >= map.tiles.size() ) {
            assert( 0 );
            return 0;
        }

        const auto & flagObjects = getObjectsByGroup( ObjectGroup::LANDSCAPE_FLAGS );

        uint32_t leftFlagColor = 0;
        uint32_t rightFlagColor = 0;

        for ( const auto & tempObject : map.tiles[tileIndex - 1].objects ) {
            if ( tempObject.group == ObjectGroup::LANDSCAPE_FLAGS && tempObject.id == id ) {
                assert( tempObject.index < flagObjects.size() );

                leftFlagColor = flagObjects[tempObject.index].metadata[0];
                break;
            }
        }

        for ( const auto & tempObject : map.tiles[tileIndex + 1].objects ) {
            if ( tempObject.group == ObjectGroup::LANDSCAPE_FLAGS && tempObject.id == id ) {
                assert( tempObject.index < flagObjects.size() );

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
}
