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

#include "army.h"
#include "color.h"
#include "pairs.h"
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
    bool isBlockedByObject( int target, bool fromWater = false ) const;
    uint32_t getMovementPenalty( int start, int target, int direction, uint8_t skill = Skill::Level::EXPERT ) const;

protected:
    void processWorldMap( int pathStart );
    void checkAdjacentNodes( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater );

    // This method defines pathfinding rules. This has to be implemented by the derived class.
    virtual void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater ) = 0;

    uint8_t _pathfindingSkill = Skill::Level::EXPERT;
    std::vector<int> _mapOffset;
};

class PlayerWorldPathfinder : public WorldPathfinder
{
public:
    PlayerWorldPathfinder() {}
    virtual void reset() override;

    void reEvaluateIfNeeded( const Heroes & hero );
    virtual std::list<Route::Step> buildPath( int targetIndex ) const override;

private:
    void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater ) override;
};

class AIWorldPathfinder : public WorldPathfinder
{
public:
    AIWorldPathfinder( double advantage )
        : _advantage( advantage )
    {}
    virtual void reset() override;

    void reEvaluateIfNeeded( int start, int color, double armyStrength, uint8_t skill );
    void reEvaluateIfNeeded( const Heroes & hero );
    int getFogDiscoveryTile( const Heroes & hero );
    std::vector<IndexObject> getObjectsOnTheWay( int targetIndex, bool checkAdjacent = false );
    uint32_t getDistance( const Heroes & hero, int targetIndex );

    // Used for non-hero armies, like castles or monsters
    uint32_t getDistance( int start, int targetIndex, int color, double armyStrength, uint8_t skill = Skill::Level::EXPERT );

    // Override builds path to the nearest valid object
    virtual std::list<Route::Step> buildPath( int targetIndex ) const override;

    // Faster, but does not re-evaluate the map (expose base class method)
    using Pathfinder::getDistance;

private:
    void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater ) override;

    int _currentColor = Color::NONE;
    double _armyStrength = -1;
    double _advantage = 1.0;
    Army _temporaryArmy; // for internal calculations
};
