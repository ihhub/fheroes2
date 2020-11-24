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

#include "route.h"

// Base representation of the dataset that mirrors the 2D map being traversed
struct PathfindingNode
{
    int _from = -1;
    uint32_t _cost = 0;
    uint16_t _objectID = 0;

    PathfindingNode() {}
    PathfindingNode( int node, uint32_t cost, uint16_t object )
        : _from( node )
        , _cost( cost )
        , _objectID( object )
    {}
    virtual ~PathfindingNode() = default;
    // Sets node values back to the defaults; used before processing new path
    virtual void resetNode()
    {
        _from = -1;
        _cost = 0;
        _objectID = 0;
    }
};

// Template class has to be either PathfindingNode or its derivative
template <class T>
class Pathfinder
{
public:
    virtual void reset() = 0;
    virtual std::list<Route::Step> buildPath( int targetIndex ) const = 0;

    virtual uint32_t getDistance( int targetIndex ) const
    {
        return _cache[targetIndex]._cost;
    }

    virtual const T & getNode( int targetIndex ) const
    {
        return _cache[targetIndex];
    }

protected:
    std::vector<T> _cache;
    int _pathStart = -1;
};
