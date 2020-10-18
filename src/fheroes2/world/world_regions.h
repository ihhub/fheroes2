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

#include <set>

#include "world.h"

enum
{
    BLOCKED = 0,
    OPEN = 1,
    BORDER = 2,
    REGION = 3
};

struct MapRegionNode
{
    int index = -1;
    int type = BLOCKED;
    uint16_t mapObject = 0;
    uint16_t passable = 0;
    bool isWater = false;

    MapRegionNode() {}
    MapRegionNode( int index )
        : index( index )
        , type( OPEN )
    {}
    MapRegionNode( int index, uint16_t pass, bool water )
        : index( index )
        , type( OPEN )
        , passable( pass )
        , isWater( water )
    {}
};

struct MapRegion
{
    int id = REGION;
    bool isWater = false;
    std::set<int> neighbours;
    std::vector<MapRegionNode> nodes;
    size_t lastProcessedNode = 0;

    MapRegion( int regionIndex, int mapIndex, bool water, size_t expectedSize )
        : id( REGION + regionIndex )
        , isWater( water )
    {
        nodes.reserve( expectedSize );
        nodes.push_back( { mapIndex } );
        nodes[0].type = id;
    }
};
