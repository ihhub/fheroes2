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

#include <cstdint>
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
    class NodeCache;
    struct Node;
    struct ObjectPlacement;
    struct ObjectSet;
    struct Region;
}

namespace Maps::Map_Format
{
    struct MapFormat;
}

namespace Rand
{
    class PCG32;
}

namespace Maps::Random_Generator
{
    int32_t getObjectGoldValue( const ObjectGroup group, const int32_t objectIndex );
    int32_t pickMonsterByValue( const int32_t protectedObjectValue );
    int32_t selectTerrainVariantForObject( const ObjectGroup groupType, const int32_t objectIndex, const int groundType );
    std::vector<std::vector<int32_t>> findOpenTilesSortedJittered( const Region & region, int32_t mapWidth, Rand::PCG32 & randomGenerator );

    bool canFitObject( const NodeCache & data, const ObjectInfo & info, const fheroes2::Point & mainTilePos, const bool skipBorders );
    bool canFitObjectSet( const NodeCache & data, const ObjectSet & set, const fheroes2::Point & mainTilePos );
    void markObjectPlacement( NodeCache & data, const ObjectInfo & info, const fheroes2::Point & mainTilePos );

    bool putObjectOnMap( Map_Format::MapFormat & mapFormat, Tile & tile, const ObjectGroup groupType, const int32_t objectIndex );
    bool placeActionObject( Map_Format::MapFormat & mapFormat, NodeCache & data, Tile & tile, const ObjectGroup groupType, const int32_t type );
    bool placeCastle( Map_Format::MapFormat & mapFormat, NodeCache & data, const Region & region, const fheroes2::Point tilePos, const bool isCastle );
    bool placeMine( Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & node, const int resource );
    bool placeObstacle( Map_Format::MapFormat & mapFormat, const NodeCache & data, const Node & node, Rand::PCG32 & randomGenerator );
    bool placeSimpleObject( Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & centerNode, const ObjectPlacement & placement );
    void placeObjectSet( Map_Format::MapFormat & mapFormat, NodeCache & data, const Region & region, std::vector<ObjectSet> objects, const uint8_t expectedCount,
                         Rand::PCG32 & randomGenerator );
}
