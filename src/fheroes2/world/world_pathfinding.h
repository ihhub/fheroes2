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

    // This method resizes the cache and re-calculates map offsets if values are out of sync with World class
    virtual void checkWorldSize();

    // Shared helper methods
    virtual std::list<Route::Step> buildPath( int target ) const;
    bool isBlockedByObject( int target, bool fromWater = false ) const;
    uint32_t getMovementPenalty( int start, int target, int direction, uint8_t skill = Skill::Level::EXPERT ) const;

protected:
    void processWorldMap( int pathStart );
    void checkAdjacentNodes( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater );

    // This methods defines pathfinding rules. This has to be implemented by the derived class.
    virtual void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater ) = 0;

    uint8_t _pathfindingSkill = Skill::Level::EXPERT;
    std::vector<int> _mapOffset;
};

class PlayerWorldPathfinder : public WorldPathfinder
{
public:
    PlayerWorldPathfinder() {}
    void reset();

    void reEvaluateIfNeeded( const Heroes & hero );

private:
    void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater );
};

class AIWorldPathfinder : public WorldPathfinder
{
public:
    AIWorldPathfinder() {}
    void reset();

    void reEvaluateIfNeeded( int start, int color, double armyStrength, uint8_t skill );
    void reEvaluateIfNeeded( const Heroes & hero );
    int searchForFog( const Heroes & hero );
    uint32_t getDistance( const Heroes & hero, int targetIndex );

    // Used for non-hero armies, like castles or monsters
    uint32_t getDistance( int start, int targetIndex, int color, double armyStrength, uint8_t skill = Skill::Level::EXPERT );

private:
    void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater );

    int _currentColor = Color::NONE;
    double _armyStrength = -1;
};
