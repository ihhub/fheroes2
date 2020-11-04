/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include <set>
#include <vector>

enum
{
    REGION_NODE_BLOCKED = 0,
    REGION_NODE_OPEN = 1,
    REGION_NODE_BORDER = 2,
    REGION_NODE_FOUND = 3
};

struct MapRegionNode
{
    int index = -1;
    uint32_t type = REGION_NODE_BLOCKED;
    uint16_t mapObject = 0;
    uint16_t passable = 0;
    bool isWater = false;

    MapRegionNode() {}
    MapRegionNode( int index )
        : index( index )
        , type( REGION_NODE_OPEN )
    {}
    MapRegionNode( int index, uint16_t pass, bool water )
        : index( index )
        , type( REGION_NODE_OPEN )
        , passable( pass )
        , isWater( water )
    {}
};

struct MapRegion
{
public:
    uint32_t _id = REGION_NODE_FOUND;
    bool _isWater = false;
    std::set<int> _neighbours;
    std::vector<MapRegionNode> _nodes;
    size_t _lastProcessedNode = 0;

    MapRegion(){};
    MapRegion( int regionIndex, int mapIndex, bool water, size_t expectedSize );
    std::vector<int> getNeighbours() const;
    size_t getNeighboursCount() const;
    std::vector<IndexObject> getObjectList() const;
    int getObjectCount() const;
    double getFogRatio( int color ) const;
};
