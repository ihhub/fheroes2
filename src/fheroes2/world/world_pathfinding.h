/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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
#include <list>
#include <vector>

#include "color.h"
#include "mp2.h"
#include "pathfinding.h"
#include "skill.h"

class Heroes;
class IndexObject;

namespace Route
{
    class Step;
}

struct WorldNode : public PathfindingNode<MP2::MapObjectType>
{
    // The number of movement points remaining for the hero after moving to this node
    uint32_t _remainingMovePoints = 0;

    WorldNode() = default;

    WorldNode( const int node, const uint32_t cost, const MP2::MapObjectType object, const uint32_t remainingMovePoints )
        : PathfindingNode( node, cost, object )
        , _remainingMovePoints( remainingMovePoints )
    {}

    WorldNode( const WorldNode & ) = delete;
    WorldNode( WorldNode && ) = default;

    WorldNode & operator=( const WorldNode & ) = delete;
    WorldNode & operator=( WorldNode && ) = default;

    void resetNode() override
    {
        PathfindingNode::resetNode();

        _remainingMovePoints = 0;
    }
};

// Abstract class that provides base functionality to path through World map
class WorldPathfinder : public Pathfinder<WorldNode>
{
public:
    WorldPathfinder() = default;
    WorldPathfinder( const WorldPathfinder & ) = delete;

    WorldPathfinder & operator=( const WorldPathfinder & ) = delete;

    // This method resizes the cache and re-calculates map offsets if values are out of sync with World class
    virtual void checkWorldSize();

    static uint32_t calculatePathPenalty( const std::list<Route::Step> & path );

protected:
    void processWorldMap();
    void checkAdjacentNodes( std::vector<int> & nodesToExplore, int currentNodeIdx );

    // This method defines pathfinding rules. This has to be implemented by the derived class.
    virtual void processCurrentNode( std::vector<int> & nodesToExplore, const int currentNodeIdx ) = 0;

    // Calculates the movement penalty when moving from the src tile to the adjacent dst tile in the specified direction.
    // If the "last move" logic should be taken into account (when performing pathfinding for a real hero on the map),
    // then the src tile should be already accessible for this hero and it should also have a valid information about
    // the hero's remaining movement points. The default implementation can be overridden by a derived class.
    virtual uint32_t getMovementPenalty( int src, int dst, int direction ) const;

    // Subtracts movement points taking the transition between turns into account
    uint32_t substractMovePoints( const uint32_t movePoints, const uint32_t substractedMovePoints ) const;

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
    PlayerWorldPathfinder( const PlayerWorldPathfinder & ) = delete;

    PlayerWorldPathfinder & operator=( const PlayerWorldPathfinder & ) = delete;

    void reset() override;

    void reEvaluateIfNeeded( const Heroes & hero );
    std::list<Route::Step> buildPath( const int targetIndex ) const;

private:
    // Follows regular passability rules (for the human player)
    void processCurrentNode( std::vector<int> & nodesToExplore, const int currentNodeIdx ) override;
};

class AIWorldPathfinder : public WorldPathfinder
{
public:
    explicit AIWorldPathfinder( double advantage )
        : _advantage( advantage )
    {}

    AIWorldPathfinder( const AIWorldPathfinder & ) = delete;

    AIWorldPathfinder & operator=( const AIWorldPathfinder & ) = delete;

    void reset() override;

    void reEvaluateIfNeeded( const Heroes & hero );
    void reEvaluateIfNeeded( const int start, const int color, const double armyStrength, const uint8_t skill, const bool isArtifactBagFull );
    int getFogDiscoveryTile( const Heroes & hero );

    // Used for cases when heroes are stuck because one hero might be blocking the way and we have to move him.
    int getNearestTileToMove( const Heroes & hero );

    static bool isHeroPossiblyBlockingWay( const Heroes & hero );

    std::vector<IndexObject> getObjectsOnTheWay( const int targetIndex, const bool checkAdjacent = false ) const;

    std::list<Route::Step> getDimensionDoorPath( const Heroes & hero, int targetIndex ) const;

    // Used for non-hero armies, like castles or monsters
    uint32_t getDistance( int start, int targetIndex, int color, double armyStrength, uint8_t skill = Skill::Level::EXPERT );

    // Override builds path to the nearest valid object
    std::list<Route::Step> buildPath( const int targetIndex, const bool isPlanningMode = false ) const;

    // Faster, but does not re-evaluate the map (expose base class method)
    using Pathfinder::getDistance;

    double getCurrentArmyStrengthMultiplier() const
    {
        return _advantage;
    }

    void setArmyStrengthMultiplier( const double multiplier );
    void setSpellPointReserve( const double reserve );

private:
    // Follows custom passability rules (for the AI)
    void processCurrentNode( std::vector<int> & nodesToExplore, const int currentNodeIdx ) override;

    // Adds special logic for AI-controlled heroes to encourage them to overcome water obstacles using boats.
    // If this logic should be taken into account (when performing pathfinding for a real hero on the map),
    // then the src tile should be already accessible for this hero and it should also have a valid information
    // about the hero's remaining movement points.
    uint32_t getMovementPenalty( int src, int dst, int direction ) const override;

    double _armyStrength{ -1 };
    double _advantage{ 1.0 };
    double _spellPointsReserved{ 0.5 };
    bool _isArtifactBagFull{ false };
};
