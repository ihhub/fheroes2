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
#include "mp2.h"
#include "pathfinding.h"
#include "skill.h"

class IndexObject;

// Abstract class that provides base functionality to path through World map
class WorldPathfinder : public Pathfinder<PathfindingNode<MP2::MapObjectType>>
{
public:
    WorldPathfinder() = default;

    // This method resizes the cache and re-calculates map offsets if values are out of sync with World class
    virtual void checkWorldSize();

protected:
    void processWorldMap( int pathStart );
    void checkAdjacentNodes( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater );

    // This method defines pathfinding rules. This has to be implemented by the derived class.
    virtual void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater ) = 0;

    // Calculates the movement penalty when moving from src tile to adjacent dst tile in the specified direction.
    uint32_t getMovementPenalty( int src, int dst, int direction, uint32_t consumedMovePoints ) const;

    uint8_t _pathfindingSkill = Skill::Level::EXPERT;
    int _currentColor = Color::NONE;
    uint32_t _remainingMovePoints = 0;
    uint32_t _maxMovePoints = 0;
    std::vector<int> _mapOffset;
};

class PlayerWorldPathfinder : public WorldPathfinder
{
public:
    PlayerWorldPathfinder() = default;

    void reset() override;

    void reEvaluateIfNeeded( const Heroes & hero );
    std::list<Route::Step> buildPath( int targetIndex ) const;

private:
    void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater ) override;
};

class AIWorldPathfinder : public WorldPathfinder
{
public:
    explicit AIWorldPathfinder( double advantage )
        : _advantage( advantage )
    {}

    void reset() override;

    void reEvaluateIfNeeded( int start, int color, double armyStrength, uint8_t skill );
    void reEvaluateIfNeeded( const Heroes & hero );
    int getFogDiscoveryTile( const Heroes & hero );

    // Used for cases when heroes are stuck because one hero might be blocking the way and we have to move him.
    int getNeareastTileToMove( const Heroes & hero );

    bool isHeroPossiblyBlockingWay( const Heroes & hero );

    std::vector<IndexObject> getObjectsOnTheWay( int targetIndex, bool checkAdjacent = false );

    // Used for non-hero armies, like castles or monsters
    uint32_t getDistance( int start, int targetIndex, int color, double armyStrength, uint8_t skill = Skill::Level::EXPERT );

    // Override builds path to the nearest valid object
    std::list<Route::Step> buildPath( int targetIndex, bool isPlanningMode = false ) const;

    // Faster, but does not re-evaluate the map (expose base class method)
    using Pathfinder::getDistance;

    double getCurrentArmyStrengthMultiplier() const
    {
        return _advantage;
    }

    void setArmyStrengthMultplier( const double multiplier );

private:
    void processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater ) override;

    double _armyStrength = -1;
    double _advantage = 1.0;
    Army _temporaryArmy; // for internal calculations
};
