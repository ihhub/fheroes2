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
#include <memory>
#include <vector>

#include "map_format_info.h"
#include "map_object_info.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
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
}

namespace Maps
{
    bool readMapInEditor( const Map_Format::MapFormat & map )
    {
        world.generateForEditor( map.size );

        assert( static_cast<size_t>( world.w() ) * world.h() == map.tiles.size() );

        // We must clear all tiles before writing something on them.
        for ( size_t i = 0; i < map.tiles.size(); ++i ) {
            auto & tile = world.GetTiles( static_cast<int32_t>( i ) );
            tile = {};

            tile.setIndex( static_cast<int32_t>( i ) );
        }

        for ( size_t i = 0; i < map.tiles.size(); ++i ) {
            readTile( world.GetTiles( static_cast<int32_t>( i ) ), map.tiles[i] );
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

    void readTile( Tiles & tile, const Map_Format::TileInfo & info )
    {
        tile.setTerrain( info.terrainIndex, info.terrainFlag & 2, info.terrainFlag & 1 );

        for ( const auto & object : info.objects ) {
            const auto & objectInfos = getObjectsByGroup( object.group );
            if ( object.index >= objectInfos.size() ) {
                // This is a bad map format!
                assert( 0 );
                continue;
            }

            // Object UID is set through global object UID counter. Therefore, we need to update it before running the operation.
            if ( object.id == 0 ) {
                // This object UID is not set!
                assert( 0 );
                continue;
            }

            setLastObjectUID( object.id - 1 );
            setObjectOnTile( tile, objectInfos[object.index] );
        }
    }

    void writeTile( const Tiles & tile, Map_Format::TileInfo & info )
    {
        // Roads and streams are the only objects that are needed to be saved separately.
        // This is because modification on one tile affects all neighboring tiles as well.
        // Check all existing objects and delete all roads and streams.
        info.objects.erase( std::remove_if( info.objects.begin(), info.objects.end(), []( const Maps::Map_Format::ObjectInfo & object ) {
            return object.group == ObjectGroup::ROADS || object.group == ObjectGroup::STREAMS; } ), info.objects.end() );

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
}
