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

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>

#include "battle_board.h"

namespace Battle
{
    class Position;
    class Unit;

    using BattleNodeIndex = std::pair<int32_t, int32_t>;

    struct BattleNodeIndexHash final
    {
        std::size_t operator()( const BattleNodeIndex & index ) const noexcept
        {
            const uint64_t f = index.first;
            const uint64_t s = index.second;

            std::hash<uint64_t> hasher;

            return hasher( ( f << 32 ) + s );
        }
    };

    struct BattleNode final
    {
        BattleNodeIndex _from{ -1, -1 };
        // Cost of moving to this node. May differ from _distance due to penalties (e.g. moat penalty) and
        // also in the case of flying units.
        uint32_t _cost{ 0 };
        // Distance to this node, measured in the number of movements that need to be performed to get here.
        // The reversal of a wide unit is not considered as a movement. For flying units, this distance is
        // estimated as the straight line distance to the position corresponding to this node.
        uint32_t _distance{ 0 };

        BattleNode() = default;

        void update( BattleNodeIndex from, const uint32_t cost, const uint32_t distance )
        {
            _from = std::move( from );
            _cost = cost;
            _distance = distance;
        }
    };

    class BattlePathfinder final
    {
    public:
        BattlePathfinder() = default;
        BattlePathfinder( const BattlePathfinder & ) = delete;

        ~BattlePathfinder() = default;

        BattlePathfinder & operator=( const BattlePathfinder & ) = delete;

        // Checks whether the given position is reachable for the given unit, either on the current turn or in principle
        bool isPositionReachable( const Unit & unit, const Position & position, const bool isOnCurrentTurn );

        // Returns the cost of moving to the given position for the given unit. It's the caller's responsibility to make
        // sure that this position is reachable before calling this method.
        uint32_t getCost( const Unit & unit, const Position & position );

        // Returns the distance to the given position (i.e. the number of movements that need to be performed to get to
        // this position, the reversal of a wide unit is not considered as a movement) for the given unit. For flying
        // units, this distance is estimated as the straight line distance to the given position. It's the caller's
        // responsibility to make sure that this position is reachable before calling this method.
        uint32_t getDistance( const Unit & unit, const Position & position );

        // Builds and returns the path (or its part) for the given unit to the given position that can be traversed during
        // the current turn. If this position is unreachable by this unit, then an empty path is returned.
        Indexes buildPath( const Unit & unit, const Position & position );

        // Returns the indexes of all cells that can be occupied by the given unit's head on the current turn
        Indexes getAllAvailableMoves( const Unit & unit );

        // Returns the position on the path for the given unit to the given position, which is reachable on the current
        // turn and is as close as possible to the destination (excluding the current position of the unit). If the given
        // position is unreachable by the given unit, then an empty Position object is returned.
        Position getClosestReachablePosition( const Unit & unit, const Position & position );

    private:
        // Rebuilds the graph of available positions for the given unit if necessary (if it is not already cached)
        void reEvaluateIfNeeded( const Unit & unit );

        std::unordered_map<BattleNodeIndex, BattleNode, BattleNodeIndexHash> _cache;

        // Parameters of the unit for which the current cache is created
        BattleNodeIndex _pathStart{ -1, -1 };
        uint32_t _speed{ 0 };
        bool _isWide{ false };
        bool _isFlying{ false };
        // The unit's color (or rather, the unit's army color) affects the ability to pass the castle bridge
        int _color{ 0 };
        // Board cells passability status at the time of current cache creation
        std::array<bool, Board::sizeInCells> _boardStatus{};
    };
}
