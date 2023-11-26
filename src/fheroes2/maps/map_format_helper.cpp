/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#include "map_format_info.h"
#include "map_object_info.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "world.h"
#include "world_object_uid.h"

namespace Maps
{
    bool readMapInEditor( const Map_Format::MapFormat & map )
    {
        world.generateForEditor( map.size );

        assert( static_cast<size_t>( world.w() ) * world.h() == map.tiles.size() );

        for ( size_t i = 0; i < map.tiles.size(); ++i ) {
            readTile( world.GetTiles( static_cast<int32_t>( i ) ), map.tiles[i] );
        }

        return true;
    }

    bool saveMapInEditor( Map_Format::MapFormat & map )
    {
        assert( world.w() == world.h() );

        map.size = world.w();

        const size_t size = map.size * map.size;

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
        // Only bottom layer addons / objects parts can be stored within the map format.
        ObjectGroup group;
        uint32_t index;

        for ( const auto & addon : tile.getBottomLayerAddons() ) {
            if ( getObjectInfo( addon._objectIcnType, addon._imageIndex, group, index ) ) {
                info.objects.emplace_back();
                info.objects.back().id = addon._uid;
                info.objects.back().group = group;
                info.objects.back().index = index;
            }
        }

        if ( getObjectInfo( tile.getObjectIcnType(), tile.GetObjectSpriteIndex(), group, index ) ) {
            info.objects.emplace_back();
            info.objects.back().id = tile.GetObjectUID();
            info.objects.back().group = group;
            info.objects.back().index = index;
        }

        info.terrainIndex = tile.getTerrainImageIndex();
        info.terrainFlag = tile.getTerrainFlags();
    }
}
