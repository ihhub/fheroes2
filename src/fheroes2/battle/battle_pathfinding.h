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

        // Rebuilds the graph of available positions for the given unit
        void evaluateForUnit( const Unit & unit );
        // Checks whether the given position is reachable, either on the current turn or in principle
        bool isPositionReachable( const Position & position, const bool onCurrentTurn ) const;
        // Returns the distance to the given position (i.e. the number of cells that needs to be passed).
        // It's the caller's responsibility to make sure that this position is reachable before calling
        // this method.
        uint32_t getDistance( const Position & position ) const;
        // Builds and returns a path (or its part) to the given position that can be traversed during the
        // current turn. If this position is unreachable, then an empty path is returned.
        Indexes buildPath( const Position & position ) const;
        // Returns the indexes of all cells that can be occupied by the unit's head on the current turn
        Indexes getAllAvailableMoves() const;

    private:
        std::unordered_map<BattleNodeIndex, BattleNode, BattleNodeIndexHash> _cache;
        BattleNodeIndex _pathStart = { -1, -1 };
        uint32_t _unitSpeed = 0;
    };
}
