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

#include "map_random_generator_info.h"

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
    int32_t localizeObjectToTerrain( const ObjectGroup groupType, const int32_t objectIndex, const int groundType );
    std::vector<std::vector<int32_t>> findOpenTilesSortedJittered( const Region & region, int32_t mapWidth, Rand::PCG32 & randomGenerator );

    bool putObjectOnMap( Maps::Map_Format::MapFormat & mapFormat, Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex );
    bool placeCastle( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Region & region, const fheroes2::Point tilePos, const bool isCastle );
    bool placeMine( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & node, const int resource );
    bool placeObstacle( Maps::Map_Format::MapFormat & mapFormat, const NodeCache & data, const Node & node, Rand::PCG32 & randomGenerator );
    bool placeSimpleObject( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & centerNode, const ObjectPlacement & placement );
    void placeObjectSet( Maps::Map_Format::MapFormat & mapFormat, NodeCache & data, const Region & region, const std::vector<ObjectSet> & set,
                         const uint8_t expectedCount, Rand::PCG32 & randomGenerator );
}
