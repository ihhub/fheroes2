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

#include <algorithm>
#include <cassert>

#include "battle_pathfinding.h"
#include "battle_arena.h"
#include "battle_bridge.h"
#include "battle_troop.h"
#include "castle.h"
#include "logging.h"

namespace Battle
{
    void BattleNode::resetNode()
    {
        _from = -1;
        _cost = MAX_MOVE_COST;
        _objectID = 0;
        _isOpen = true;
        _isLeftDirection = false;
    }

    AIBattlePathfinder::AIBattlePathfinder()
    {
        _cache.resize( ARENASIZE );
    }

    void AIBattlePathfinder::reset()
    {
        _start.Set( -1, false, false );
        for ( size_t i = 0; i < _cache.size(); ++i ) {
            _cache[i].resetNode();
        }
    }

    bool AIBattlePathfinder::hexIsPassable( int targetCell ) const
    {
        const size_t index = static_cast<size_t>( targetCell );
        return index < _cache.size() && nodeIsPassable( _cache[index] );
    }

    bool AIBattlePathfinder::nodeIsPassable( const BattleNode & node ) const
    {
        return node._cost == 0 || ( node._isOpen && node._from != -1 );
    }

    Indexes AIBattlePathfinder::getAllAvailableMoves( uint32_t moveRange ) const
    {
        Indexes result;
        result.reserve( moveRange * 2u );

        for ( size_t index = 0; index < _cache.size(); ++index ) {
            const BattleNode & node = _cache[index];
            if ( nodeIsPassable( node ) && node._cost <= moveRange ) {
                result.push_back( static_cast<int>( index ) );
            }
        }
        return result;
    }

    Indexes AIBattlePathfinder::buildPath( int targetCell ) const
    {
        Indexes path;

        if ( static_cast<size_t>( targetCell ) >= _cache.size() )
            return path;

        int currentNode = targetCell;
        while ( _cache[currentNode]._cost != 0 && !_start.contains( currentNode ) ) {
            const BattleNode & node = _cache[currentNode];
            path.push_back( currentNode );
            currentNode = node._from;
        }
        std::reverse( path.begin(), path.end() );

        return path;
    }

    Indexes AIBattlePathfinder::findTwoMovesOverlap( int targetCell, uint32_t movementRange ) const
    {
        Indexes path;
        if ( static_cast<size_t>( targetCell ) >= _cache.size() )
            return path;

        const uint32_t pathCost = _cache[targetCell]._cost;
        if ( pathCost >= movementRange * 2 )
            return path;

        int currentNode = targetCell;
        uint32_t nodeCost = pathCost;

        while ( nodeCost != 0 && !_start.contains( currentNode ) ) {
            const BattleNode & node = _cache[currentNode];
            // Upper limit
            if ( movementRange == 0 || node._cost <= movementRange ) {
                path.push_back( currentNode );
            }
            currentNode = node._from;
            nodeCost = _cache[currentNode]._cost;

            // Lower limit
            if ( movementRange > 0 && !path.empty() && pathCost - nodeCost >= movementRange )
                break;
        }
        std::reverse( path.begin(), path.end() );

        return path;
    }

    void AIBattlePathfinder::calculate( const Unit & unit )
    {
        reset();

        const bool unitIsWide = unit.isWide();

        _start = unit.GetPosition();
        const Cell * unitHead = _start.GetHead();
        const Cell * unitTail = _start.GetTail();
        if ( !unitHead || ( unitIsWide && !unitTail ) ) {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Pathfinder: Invalid unit is passed in! " << unit.GetName() )
            return;
        }

        const Bridge * bridge = Arena::GetBridge();
        const Castle * castle = Arena::GetCastle();

        const bool isPassableBridge = bridge == nullptr || bridge->isPassable( unit );
        const bool isMoatBuilt = castle && castle->isBuild( BUILD_MOAT );
        const uint32_t moatPenalty = unit.GetSpeed();

        // Initialize the starting cells
        const int32_t pathStart = unitHead->GetIndex();
        _cache[pathStart]._cost = 0;
        _cache[pathStart]._isOpen = false;
        _cache[pathStart]._isLeftDirection = unitIsWide && unit.isReflect();

        if ( unitIsWide ) {
            const int32_t tailIdx = unitTail->GetIndex();
            _cache[tailIdx]._from = pathStart;
            _cache[tailIdx]._cost = 0;
            _cache[tailIdx]._isOpen = true;
            _cache[tailIdx]._isLeftDirection = !unit.isReflect();
        }

        if ( unit.isFlying() ) {
            const Board & board = *Arena::GetBoard();

            // Find all free spaces on the battle board - flyers can move to any of them
            for ( Board::const_iterator it = board.begin(); it != board.end(); ++it ) {
                const int32_t idx = it->GetIndex();
                BattleNode & node = _cache[idx];

                // isPassableForUnit checks if there's space for unit tail (for wide units)
                if ( it->isPassableForUnit( unit ) && ( isPassableBridge || !Board::isBridgeIndex( it - board.begin(), unit ) ) ) {
                    node._isOpen = true;
                    node._from = pathStart;
                    node._cost = Battle::Board::GetDistance( pathStart, idx );
                }
                else {
                    node._isOpen = false;
                }
            }
            // Once board movement is determined we look for units save shortest flight path to them
            for ( Board::const_iterator it = board.begin(); it != board.end(); ++it ) {
                const Unit * boardUnit = it->GetUnit();
                if ( boardUnit && boardUnit->GetUID() != unit.GetUID() ) {
                    const int32_t unitIdx = it->GetIndex();
                    BattleNode & unitNode = _cache[unitIdx];

                    const Indexes & around = Battle::Board::GetAroundIndexes( unitIdx );
                    for ( const int32_t cell : around ) {
                        const uint32_t flyingDist = Battle::Board::GetDistance( pathStart, cell );
                        if ( hexIsPassable( cell ) && ( flyingDist < unitNode._cost ) ) {
                            unitNode._isOpen = false;
                            unitNode._from = cell;
                            unitNode._cost = flyingDist;
                        }
                    }
                }
            }
        }
        // Walking units - explore the movements sequentially from both the head and tail cells
        else {
            std::vector<int32_t> nodesToExplore;

            nodesToExplore.push_back( pathStart );
            if ( unitIsWide ) {
                nodesToExplore.push_back( unitTail->GetIndex() );
            }

            for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
                const int32_t fromNode = nodesToExplore[lastProcessedNode];
                const BattleNode & previousNode = _cache[fromNode];

                const Cell * fromCell = Board::GetCell( fromNode );
                assert( fromCell != nullptr );

                Indexes availableMoves;

                if ( !unitIsWide ) {
                    availableMoves = Board::GetAroundIndexes( fromNode );
                }
                else if ( previousNode._from < 0 ) {
                    availableMoves = Board::GetMoveWideIndexes( fromNode, unit.isReflect() );
                }
                else {
                    availableMoves = Board::GetMoveWideIndexes( fromNode, ( RIGHT_SIDE & Board::GetDirection( fromNode, previousNode._from ) ) != 0 );
                }

                for ( const int32_t newNode : availableMoves ) {
                    const bool isLeftDirection = unitIsWide && Board::IsLeftDirection( fromNode, newNode, previousNode._isLeftDirection );

                    const Cell * newCell = Board::GetCell( newNode );
                    assert( newCell != nullptr );

                    if ( newCell->isPassableFromAdjacent( unit, *fromCell ) && ( isPassableBridge || !Board::isBridgeIndex( newNode, unit ) ) ) {
                        const uint32_t cost = previousNode._cost;
                        BattleNode & node = _cache[newNode];

                        uint32_t additionalCost = 1u;

                        // Turning back is not a movement
                        if ( isLeftDirection != previousNode._isLeftDirection ) {
                            additionalCost = 0;
                        }
                        else {
                            const int32_t newTailIndex = isLeftDirection ? newNode + 1 : newNode - 1;

                            // The moat penalty consumes all remaining movement. Be careful when dealing with unsigned values.
                            if ( isMoatBuilt && ( Board::isMoatIndex( newNode, unit ) || Board::isMoatIndex( newTailIndex, unit ) )
                                 && moatPenalty > previousNode._cost ) {
                                additionalCost = moatPenalty - cost;
                            }
                        }

                        if ( cost + additionalCost < node._cost ) {
                            node._isOpen = true;
                            node._from = fromNode;
                            node._cost = cost + additionalCost;
                            node._isLeftDirection = isLeftDirection;

                            nodesToExplore.push_back( newNode );
                        }
                    }
                    // Special case: there is a unit in this cell, mark this cell as impassable, but available for a possible attack
                    else if ( newCell->GetUnit() ) {
                        const uint32_t cost = previousNode._cost;
                        BattleNode & node = _cache[newNode];

                        if ( cost < node._cost ) {
                            node._isOpen = false;
                            node._from = fromNode;
                            node._cost = cost;
                            node._isLeftDirection = isLeftDirection;
                        }
                    }
                }
            }
        }
    }
}
