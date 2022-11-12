/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2022                                             *
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

#include "battle_board.h"
#include "battle_cell.h"
#include "pathfinding.h"

namespace Battle
{
    class Unit;

    const uint16_t MAX_MOVE_COST = ARENASIZE;

    /* BattleNode, different situations
     * default:  from: -1, isOpen: true, cost: MAX
     * starting: from: -1, isOpen: false, cost: 0
     * passable: from: 0-98, isOpen: true, cost: 1+
     * oth.unit: from: 0-98, isOpen: false, cost: 0+
     * terrain:  from: -1, isOpen: false, cost: MAX
     * if tile wouldn't be reached it stays as default
     */
    struct BattleNode : public PathfindingNode<uint16_t>
    {
        bool _isOpen = true;
        bool _isLeftDirection = false;

        // BattleNode uses different default values
        BattleNode()
            : PathfindingNode( -1, MAX_MOVE_COST, 0 )
        {}

        BattleNode( int node, uint16_t cost, bool isOpen, bool isLeftDirection )
            : PathfindingNode( node, cost, 0 )
            , _isOpen( isOpen )
            , _isLeftDirection( isLeftDirection )
        {}

        BattleNode( const BattleNode & ) = delete;
        BattleNode( BattleNode && ) = default;

        BattleNode & operator=( const BattleNode & ) = delete;
        BattleNode & operator=( BattleNode && ) = delete;

        // Override the base version of the call to use proper values
        void resetNode() override;
    };

    class AIBattlePathfinder : public Pathfinder<BattleNode>
    {
    public:
        AIBattlePathfinder();
        AIBattlePathfinder( const AIBattlePathfinder & ) = delete;

        AIBattlePathfinder & operator=( const AIBattlePathfinder & ) = delete;

        void reset() override;
        void calculate( const Unit & unit );
        Indexes buildPath( int targetCell ) const;
        Indexes findTwoMovesOverlap( int targetCell, uint32_t movementRange ) const;
        bool hexIsPassable( int targetCell ) const;
        Indexes getAllAvailableMoves( uint32_t moveRange ) const;

    private:
        bool nodeIsPassable( const BattleNode & node ) const;

        Position _start;
    };
}
