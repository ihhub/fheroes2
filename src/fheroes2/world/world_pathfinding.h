/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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
#include <optional>
#include <utility>
#include <vector>

#include "color.h"
#include "skill.h"

class Heroes;
class IndexObject;

namespace Route
{
    class Step;
}

struct WorldNode final
{
    int _from{ -1 };
    uint32_t _cost{ 0 };
    // The number of movement points remaining for the hero after moving to this node
    uint32_t _remainingMovePoints{ 0 };

    // When calculating tile availability for an AI-controlled player, various relatively heavy computations are
    // performed, the result of which does not depend on the direction in which the tile is entered. The results
    // of these calculations can be cached.
    std::optional<bool> _isAccessibleForAI;
    struct
    {
        std::optional<bool> fromWater;
        std::optional<bool> fromLand;
    } _isAvailableForWalkThroughForAI;

    WorldNode() = default;

    void update( const int from, const uint32_t cost, const uint32_t remainingMovePoints )
    {
        _from = from;
        _cost = cost;
        _remainingMovePoints = remainingMovePoints;
    }

    void reset()
    {
        _from = -1;
        _cost = 0;
        _remainingMovePoints = 0;
    }
};

// Abstract class that provides basic functionality for navigating the World Map
class WorldPathfinder
{
public:
    WorldPathfinder() = default;
    WorldPathfinder( const WorldPathfinder & ) = delete;

    virtual ~WorldPathfinder() = default;

    WorldPathfinder & operator=( const WorldPathfinder & ) = delete;

    virtual void reset();

    uint32_t getDistance( int targetIndex ) const;

protected:
    void checkAdjacentNodes( std::vector<int> & nodesToExplore, const int currentNodeIdx );

    virtual void processWorldMap();

    // Checks whether moving from the source tile in the specified direction is allowed. The default implementation
    // can be overridden by a derived class.
    virtual bool isMovementAllowed( const int from, const int direction ) const;

    // Defines the pathfinding rules and should be implemented by a derived class.
    virtual void processCurrentNode( std::vector<int> & nodesToExplore, const int currentNodeIdx ) = 0;

    // Returns the maximum number of movement points, depending on whether the movement is performed by land or by
    // water. Should be implemented by a derived class.
    virtual uint32_t getMaxMovePoints( const bool onWater ) const = 0;

    // Calculates the movement penalty when moving from the source tile to the adjacent destination tile in the
    // specified direction. If the "last move" logic should be taken into account (when performing pathfinding
    // for a real hero on the map), then the source tile should be already accessible for this hero and it should
    // also have a valid information about the hero's remaining movement points. The default implementation can be
    // overridden by a derived class.
    virtual uint32_t getMovementPenalty( const int from, const int to, const int direction ) const;

    std::vector<WorldNode> _cache;
    std::vector<int> _mapOffset;

    // The hero properties used by the pathfinder are cached here not just for optimization, but also because some
    // of them may change even if the position of the hero does not change, so it should be possible to compare the
    // old values with the new ones to determine whether the pathfinder cache needs to be recalculated.
    int _pathStart{ -1 };
    int _color{ Color::NONE };
    uint32_t _remainingMovePoints{ 0 };
    uint8_t _pathfindingSkill{ Skill::Level::EXPERT };
};

class PlayerWorldPathfinder final : public WorldPathfinder
{
public:
    PlayerWorldPathfinder() = default;
    PlayerWorldPathfinder( const PlayerWorldPathfinder & ) = delete;

    ~PlayerWorldPathfinder() override = default;

    PlayerWorldPathfinder & operator=( const PlayerWorldPathfinder & ) = delete;

    void reset() override;

    void reEvaluateIfNeeded( const Heroes & hero );

    // Builds and returns a path to the tile with the index 'targetIndex'. If the destination tile is not reachable,
    // then an empty path is returned.
    std::list<Route::Step> buildPath( const int targetIndex ) const;

private:
    // Follows regular passability rules (for the human player)
    void processCurrentNode( std::vector<int> & nodesToExplore, const int currentNodeIdx ) override;

    // Returns the maximum number of movement points. This class is not intended for planning paths passing both on
    // land and on water at the same time, so the maximum number of movement points corresponding to the type of
    // surface on which the hero is currently located is used.
    uint32_t getMaxMovePoints( const bool onWater ) const override;

    // The hero properties used by the pathfinder are cached here not just for optimization, but also because some
    // of them may change even if the position of the hero does not change, so it should be possible to compare the
    // old values with the new ones to determine whether the pathfinder cache needs to be recalculated.
    uint32_t _maxMovePoints{ 0 };
};

class AIWorldPathfinder final : public WorldPathfinder
{
public:
    AIWorldPathfinder() = default;

    AIWorldPathfinder( const AIWorldPathfinder & ) = delete;

    ~AIWorldPathfinder() override = default;

    AIWorldPathfinder & operator=( const AIWorldPathfinder & ) = delete;

    void reset() override;

    void reEvaluateIfNeeded( const Heroes & hero );
    void reEvaluateIfNeeded( const int start, const int color, const double armyStrength, const uint8_t skill );

    // Finds the most profitable tile for fog discovery. Returns a pair consisting of the tile index (-1 if no suitable tile
    // was found) and a boolean value, which takes the value true if there is fog next to this tile (that is, most likely,
    // through this tile hero can get into some new areas), and false otherwise.
    std::pair<int32_t, bool> getFogDiscoveryTile( const Heroes & hero );

    // Used for cases when heroes are stuck because one hero might be blocking the way and we have to move him.
    int getNearestTileToMove( const Heroes & hero );

    static bool isHeroPossiblyBlockingWay( const Heroes & hero );

    std::vector<IndexObject> getObjectsOnTheWay( const int targetIndex ) const;

    // Builds and returns a path to the tile with the index 'targetIndex', consisting of tile indexes suitable for using the
    // Dimension Door spell to move between them. If the target tile is unsuitable for moving to it using the Dimension Door
    // spell, but there is a tile suitable for this next to it, from which it is possible to move to the target tile, then the
    // resulting path will end with this neighboring tile. If such a path could not be built, then an empty path is returned.
    std::list<Route::Step> buildDimensionDoorPath( const int targetIndex );

    // Builds and returns a path to the tile with the index 'targetIndex'. If there is a need to pass through any objects
    // on the way to this tile, then a path to the nearest such object is returned. If the destination tile is not reachable
    // in principle, then an empty path is returned.
    std::list<Route::Step> buildPath( const int targetIndex ) const;

    // Used for non-hero armies, like castles or monsters
    uint32_t getDistance( const int start, const int targetIndex, const int color, const double armyStrength, const uint8_t skill = Skill::Level::EXPERT );
    // Faster, but does not re-evaluate the map (exposed method of the base class)
    using WorldPathfinder::getDistance;

    // Returns the coefficient of the minimum required advantage in army strength in order to be able to "pass through"
    // protected tiles from the AI pathfinder's point of view
    double getMinimalArmyStrengthAdvantage() const
    {
        return _minimalArmyStrengthAdvantage;
    }

    // Sets the coefficient of the minimum required advantage in army strength in order to be able to "pass through"
    // protected tiles from the AI pathfinder's point of view
    void setMinimalArmyStrengthAdvantage( const double advantage );

    // Returns the spell points reservation factor for spells associated with the movement of the hero on the adventure
    // map (such as Dimension Door, Town Gate or Town Portal)
    double getSpellPointsReserveRatio() const
    {
        return _spellPointsReserveRatio;
    }

    // Sets the spell points reservation factor for spells associated with the movement of the hero on the adventure map
    // (such as Dimension Door, Town Gate or Town Portal)
    void setSpellPointsReserveRatio( const double ratio );

private:
    bool isTileAccessibleForAI( const int tileIndex );
    bool isTileAvailableForWalkThroughForAI( const int tileIndex, const bool fromWater );

    void processWorldMap() override;

    // Adds special logic for AI-controlled heroes to use Summon Boat spell to overcome water obstacles (if available)
    bool isMovementAllowed( const int from, const int direction ) const override;

    // Follows custom passability rules (for the AI)
    void processCurrentNode( std::vector<int> & nodesToExplore, const int currentNodeIdx ) override;

    // Returns the maximum number of movement points, depending on whether the movement is performed by land or by
    // water
    uint32_t getMaxMovePoints( const bool onWater ) const override;

    // Adds special logic for AI-controlled heroes to correctly calculate movement penalties when such a hero passes
    // through objects on the map or overcomes water obstacles using boats. If this logic should be taken into account
    // (when performing pathfinding for a real hero on the map), then the source tile should be already accessible for
    // this hero and it should also have a valid information about the hero's remaining movement points.
    uint32_t getMovementPenalty( const int from, const int to, const int direction ) const override;

    // The hero properties used by the pathfinder are cached here not just for optimization, but also because some
    // of them may change even if the position of the hero does not change, so it should be possible to compare the
    // old values with the new ones to determine whether the pathfinder cache needs to be recalculated.
    int32_t _patrolCenter{ -1 };
    uint32_t _patrolDistance{ 0 };
    uint32_t _maxMovePointsOnLand{ 0 };
    uint32_t _maxMovePointsOnWater{ 0 };
    uint32_t _remainingSpellPoints{ 0 };
    uint32_t _maxSpellPoints{ 0 };
    uint32_t _dimensionDoorSPCost{ 0 };
    uint32_t _dimensionDoorNumOfUses{ 0 };
    double _armyStrength{ -1 };
    bool _isOnPatrol{ false };
    bool _isArtifactsBagFull{ false };
    bool _isEquippedWithSpellBook{ false };
    bool _isSummonBoatSpellAvailable{ false };
    bool _isDimensionDoorSpellAvailable{ false };

    // The potential destinations of the Town Gate and Town Portal spells should be cached here because they can
    // change even if the hero's position does not change (e.g. when a new hero was hired in the nearby castle),
    // so it should be possible to compare the old values with the new ones to detect the need to recalculate the
    // pathfinder's cache
    int32_t _townGateCastleIndex{ -1 };
    std::vector<int32_t> _townPortalCastleIndexes;

    // Coefficient of the minimum required advantage in army strength in order to be able to "pass through" protected
    // tiles from the AI pathfinder's point of view
    double _minimalArmyStrengthAdvantage{ 1.0 };

    // Spell points reservation factor for spells associated with the movement of the hero on the adventure map
    // (such as Dimension Door, Town Gate or Town Portal)
    double _spellPointsReserveRatio{ 0.5 };
};
