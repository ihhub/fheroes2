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

#include <cstddef>
#include <cstdint>
#include <vector>

class Army;

namespace Maps
{
    class Tile;

    namespace Map_Format
    {
        struct MapFormat;
        struct TileInfo;
        struct TileObjectInfo;
        struct CastleMetadata;
        struct HeroMetadata;
    }

    enum class ObjectGroup : uint8_t;

    bool readMapInEditor( const Map_Format::MapFormat & map );
    bool readAllTiles( const Map_Format::MapFormat & map );

    bool saveMapInEditor( Map_Format::MapFormat & map );

    void readTileTerrain( Tile & tile, const Map_Format::TileInfo & info );
    bool readTileObject( Tile & tile, const Map_Format::TileObjectInfo & object );

    void writeTile( const Tile & tile, Map_Format::TileInfo & info );

    void addObjectToMap( Map_Format::MapFormat & map, const int32_t tileId, const ObjectGroup group, const uint32_t index );

    bool addStream( Map_Format::MapFormat & map, const int32_t tileId );

    // Update the existing streams around the center tile to properly connect them to the center stream.
    void updateStreamsAround( Map_Format::MapFormat & map, const int32_t centerTileId );

    // Update the existing streams to connect them to the River Delta.
    void updateStreamsToDeltaConnection( Map_Format::MapFormat & map, const int32_t tileId, const int deltaDirection );

    // Returns 'Direction::UNKNOWN' if the index does not belong to River Delta object
    int getRiverDeltaDirectionByIndex( const ObjectGroup group, const int32_t objectIndex );

    bool isRiverDeltaObject( const ObjectGroup group, const int32_t objectIndex );

    bool updateMapPlayers( Map_Format::MapFormat & map );

    uint8_t getTownColorIndex( const Map_Format::MapFormat & map, const size_t tileIndex, const uint32_t id );

    bool isJailObject( const ObjectGroup group, const uint32_t index );

    uint32_t getBuildingsFromVector( const std::vector<uint32_t> & buildingsVector );

    // Should be used only for the neutral color player.
    void setDefaultCastleDefenderArmy( Map_Format::CastleMetadata & metadata );

    // Returns true if all monsters are RANDOM_MONSTER and count is 0. Should be used only for the neutral color player.
    bool isDefaultCastleDefenderArmy( const Map_Format::CastleMetadata & metadata );

    // Returns false if there are no custom units set in metadata.
    bool loadCastleArmy( Army & army, const Map_Format::CastleMetadata & metadata );
    void saveCastleArmy( const Army & army, Map_Format::CastleMetadata & metadata );
    bool loadHeroArmy( Army & army, const Map_Format::HeroMetadata & metadata );
    void saveHeroArmy( const Army & army, Map_Format::HeroMetadata & metadata );
}
