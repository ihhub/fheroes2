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

#include "color.h"
#include "pathfinding.h"
#include "route.h"

// Abstract class that provides base functionality to path through World map
class WorldPathfinder : public Pathfinder<PathfindingNode>
{
public:
    WorldPathfinder() {}
    virtual void reset();

    // Common methods
    virtual std::list<Route::Step> buildPath( int target ) const;
    bool isBlockedByObject( int target, bool fromWater = false ) const;
    uint32_t getMovementPenalty( int start, int target, int direction, uint8_t skill = Skill::Level::NONE ) const;

protected:
    void processWorldMap( int pathStart );
    void checkAdjacentNodes( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater );

    // This methods defines pathfinding rules. This has to be implemented by the derived class.
    virtual void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater ) = 0;

    uint8_t _pathfindingSkill = 0;
    std::vector<int> _mapOffset;
};

class PlayerWorldPathfinder : public WorldPathfinder
{
public:
    PlayerWorldPathfinder() {}
    virtual void reset();

    void reEvaluateIfNeeded( int start, uint8_t skill );

private:
    void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater );

};

class AIWorldPathfinder : public WorldPathfinder
{
public:
    AIWorldPathfinder() {}
    virtual void reset();
    void reEvaluateIfNeeded( int start, uint8_t skill, double armyStrength, int color );
    int searchForFog( int start, uint8_t skill, double armyStrength, int color );

private:
    void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater );

    double _armyStrength = -1;
    int _currentColor = Color::NONE;
};
