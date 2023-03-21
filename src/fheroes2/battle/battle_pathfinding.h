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

#include <bitset>
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
        BattleNodeIndex _from = { -1, -1 };
        // The cost of moving to this cell. May differ from _distance due to penalties (e.g. moat penalty).
        uint32_t _cost = 0;
        // The distance to this cell, measured in the number of cells that needs to be passed to get here.
        uint32_t _distance = 0;

        BattleNode() = default;
        BattleNode( BattleNodeIndex node, const uint32_t cost, const uint32_t distance )
            : _from( std::move( node ) )
            , _cost( cost )
            , _distance( distance )
        {}
    };

    class BattlePathfinder final
    {
    public:
        BattlePathfinder() = default;
        BattlePathfinder( const BattlePathfinder & ) = delete;

        ~BattlePathfinder() = default;

        BattlePathfinder & operator=( const BattlePathfinder & ) = delete;

        // Checks whether a given position is reachable for a given unit, either on the current turn or in principle
        bool isPositionReachable( const Unit & unit, const Position & position, const bool isOnCurrentTurn );

        // Returns the distance to a given position (i.e. the number of cells to be traversed) for a given unit.
        // It's the caller's responsibility to make sure that this position is reachable before calling this method.
        uint32_t getDistance( const Unit & unit, const Position & position );

        // Builds and returns a path (or its part) for a given unit to a given position that can be traversed during the
        // current turn. If this position is unreachable by this unit, then an empty path is returned.
        Indexes buildPath( const Unit & unit, const Position & position );

        // Returns the indexes of all cells that can be occupied by a given unit's head on the current turn
        Indexes getAllAvailableMoves( const Unit & unit );

    private:
        // Rebuilds the graph of available positions for a given unit if necessary (if it is not already cached)
        void reEvaluateIfNeeded( const Unit & unit );

        std::unordered_map<BattleNodeIndex, BattleNode, BattleNodeIndexHash> _cache;

        // Parameters of the unit for which the current cache is created
        BattleNodeIndex _pathStart = { -1, -1 };
        uint32_t _speed = 0;
        bool _isWide = false;
        bool _isFlying = false;
        // The unit's color (or rather, the unit's army color) affects the ability to pass the castle bridge
        int _color = 0;
        // Board cells passability status at the time of current cache creation
        std::bitset<ARENASIZE> _boardStatus;
    };
}
