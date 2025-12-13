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

#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "math_base.h"

namespace Maps
{
    class Tile;
    struct ObjectInfo;
    enum class ObjectGroup : uint8_t;
}

namespace Maps::Random_Generator
{
    class MapStateManager;
    class MapEconomy;
    struct Node;
    struct ObjectPlacement;
    struct ObjectSet;
    struct Region;
    struct MonsterSelection;
    enum class MonsterStrength : uint8_t;
}

namespace Maps::Map_Format
{
    struct MapFormat;
}

namespace MP2
{
    enum MapObjectType : uint16_t;
}

namespace Rand
{
    class PCG32;
}

namespace Maps::Random_Generator
{
    int32_t getObjectGoldValue( const MP2::MapObjectType object );
    int32_t getObjectGoldValue( const ObjectGroup group, const int32_t objectIndex );
    MonsterSelection getMonstersByValue( const MonsterStrength monsterStrength, int32_t protectedObjectValue );
    std::pair<ObjectGroup, int32_t> getRandomTreasure( const int32_t goldValueLimit, Rand::PCG32 & randomGenerator );
    int32_t selectTerrainVariantForObject( const ObjectGroup groupType, const int32_t objectIndex, const int groundType );
    std::vector<int32_t> findPathToNearestRoad( const MapStateManager & nodes, const int32_t mapWidth, const uint32_t regionId, const int32_t start );
    std::vector<std::vector<int32_t>> findOpenTilesSortedJittered( const Region & region, int32_t mapWidth, Rand::PCG32 & randomGenerator );
    std::vector<int32_t> findOpenTiles( const Region & region );
    std::vector<int32_t> pickEvenlySpacedTiles( const std::vector<int32_t> & candidates, const size_t pickCount, const std::vector<int32_t> & avoidance );

    bool canPlaceBorderObstacle( const MapStateManager & data, const ObjectInfo & info, const fheroes2::Point & mainTilePos );
    bool canFitObjectSet( const MapStateManager & data, const ObjectSet & set, const fheroes2::Point & mainTilePos );
    void markObjectPlacement( MapStateManager & data, const ObjectInfo & info, const fheroes2::Point & mainTilePos );
    void forceTempRoadOnTile( Map_Format::MapFormat & mapFormat, const int32_t tileIndex );

    bool putObjectOnMap( Map_Format::MapFormat & mapFormat, Tile & tile, const ObjectGroup groupType, const int32_t objectIndex );
    bool placeActionObject( Map_Format::MapFormat & mapFormat, MapStateManager & data, Tile & tile, const ObjectGroup groupType, const int32_t type );
    bool placeCastle( Map_Format::MapFormat & mapFormat, MapStateManager & data, const Region & region, const fheroes2::Point tilePos, const bool isCastle );
    int32_t placeMine( Map_Format::MapFormat & mapFormat, MapStateManager & data, MapEconomy & economy, const std::vector<int32_t> & tileOptions, const int resource,
                       const MonsterStrength monsterStrength );
    bool placeBorderObstacle( Map_Format::MapFormat & mapFormat, MapStateManager & data, const Node & node, Rand::PCG32 & randomGenerator );
    void placeMonster( Map_Format::MapFormat & mapFormat, const int32_t index, const MonsterSelection & monster );
    bool placeSimpleObject( Map_Format::MapFormat & mapFormat, MapStateManager & data, const Node & centerNode, const ObjectPlacement & placement );

    std::vector<int32_t> findTilesForPlacement( MapStateManager & data, const int32_t mapWidth, const uint32_t regionId, const std::vector<int32_t> & nodes,
                                                const ObjectInfo & objectInfo );
    std::vector<std::pair<int32_t, ObjectSet>> planObjectPlacement( MapStateManager & data, const int32_t mapWidth, const Region & region,
                                                                    std::vector<ObjectSet> objectSets, Rand::PCG32 & randomGenerator );
    void placeObjectSet( Map_Format::MapFormat & mapFormat, MapStateManager & data, Region & region, std::vector<ObjectSet> objectSets,
                         const MonsterStrength monsterStrength, const uint8_t expectedCount, Rand::PCG32 & randomGenerator );

    // This function expects a valid tileIndex that will fit the set. Plan first before calling
    void placeValidTreasures( Map_Format::MapFormat & mapFormat, MapStateManager & data, Region & region, const ObjectSet & objectSet, const int32_t tileIndex,
                              const MonsterStrength monsterStrength, Rand::PCG32 & randomGenerator );
}
