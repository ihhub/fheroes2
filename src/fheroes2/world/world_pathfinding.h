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

struct PathfindingNode
{
    int _from = -1;
    uint32_t _cost = 0;

    PathfindingNode() {}
    PathfindingNode( int node, uint32_t cost )
        : _from( node )
        , _cost( cost )
    {}
};

class Pathfinder
{
public:
    Pathfinder() {}
    void reset();
    void evaluateMap( int start, uint8_t skill );
    std::list<Route::Step> buildPath( int from, int target, uint8_t skill = Skill::Level::NONE );
    uint32_t getDistance( int from, int target, uint8_t skill = Skill::Level::NONE );
    uint32_t getMovementPenalty( int from, int target, int direction, uint8_t skill = Skill::Level::NONE ) const;
    bool isBlockedByObject( int from, int target, bool fromWater = false );
    int searchForFog( int playerColor, int start, uint8_t skill = Skill::Level::NONE );

private:
    void reEvaluateIfNeeded( int from, uint8_t skill );

    std::vector<PathfindingNode> _cache;
    int _pathStart = -1;
    uint8_t _pathfindingSkill = 0;
};
