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

#pragma once

#include <cstdint>

namespace Maps
{
    class Tiles;

    namespace Map_Format
    {
        struct MapFormat;
        struct TileInfo;
        struct ObjectInfo;
    }

    enum class ObjectGroup : int32_t;

    bool readMapInEditor( const Map_Format::MapFormat & map );
    bool readAllTiles( const Map_Format::MapFormat & map );

    bool saveMapInEditor( Map_Format::MapFormat & map );

    void readTileTerrain( Tiles & tile, const Map_Format::TileInfo & info );
    bool readTileObject( Tiles & tile, const Map_Format::ObjectInfo & object );

    void writeTile( const Tiles & tile, Map_Format::TileInfo & info );

    void addObjectToMap( Map_Format::MapFormat & map, const int32_t tileId, const ObjectGroup group, const uint32_t index );
}
