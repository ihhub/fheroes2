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

#include "battle_pathfinding.h"
#include "battle_bridge.h"
#include "battle_troop.h"
#include "castle.h"
#include "logging.h"
#include <algorithm>

namespace Battle
{
    void ArenaNode::resetNode()
    {
        _from = -1;
        _cost = MAX_MOVE_COST;
        _objectID = 0;
        _isOpen = true;
        _isLeftDirection = false;
    }

    ArenaPathfinder::ArenaPathfinder()
    {
        _cache.resize( ARENASIZE );
    }

    void ArenaPathfinder::reset()
    {
        for ( size_t i = 0; i < _cache.size(); ++i ) {
            _cache[i].resetNode();
        }
    }

    bool ArenaPathfinder::hexIsAccessible( int targetCell ) const
    {
        return _cache[targetCell]._from != -1;
    }

    bool ArenaPathfinder::hexIsPassable( int targetCell ) const
    {
        const size_t index = static_cast<size_t>( targetCell );
        return index < _cache.size() && nodeIsPassable( _cache[index] );
    }

    bool ArenaPathfinder::nodeIsPassable( const ArenaNode & node ) const
    {
        return node._cost == 0 || ( node._isOpen && node._from != -1 );
    }

    Indexes ArenaPathfinder::getAllAvailableMoves( uint32_t moveRange ) const
    {
        Indexes result;
        result.reserve( moveRange * 2u );

        for ( size_t index = 0; index < _cache.size(); ++index ) {
            const ArenaNode & node = _cache[index];
            if ( nodeIsPassable( node ) && node._cost <= moveRange ) {
                result.push_back( index );
            }
        }
        return result;
    }

    std::list<Route::Step> ArenaPathfinder::buildPath( int targetCell ) const
    {
        std::list<Route::Step> path;

        int currentNode = targetCell;
        while ( currentNode != targetCell && _cache[currentNode]._cost != 0 ) {
            const ArenaNode & node = _cache[currentNode];
            path.emplace_front( currentNode, node._from, Board::GetDirection( node._from, currentNode ), 1 );
            currentNode = node._from;
        }

        return path;
    }

    void ArenaPathfinder::calculate( const Unit & unit )
    {
        reset();

        const bool unitIsWide = unit.isWide();

        const Position & startPosition = unit.GetPosition();
        const Cell * unitHead = startPosition.GetHead();
        const Cell * unitTail = startPosition.GetTail();
        if ( !unitHead || ( unitIsWide && !unitTail ) ) {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Pathfinder: Invalid unit is passed in! " << unit.GetName() );
            return;
        }

        const Bridge * bridge = Arena::GetBridge();
        const Castle * castle = Arena::GetCastle();

        const bool isPassableBridge = bridge == nullptr || bridge->isPassable( unit.GetColor() );
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
                ArenaNode & node = _cache[idx];

                // isPassable3 checks if there's space for unit tail (for wide units)
                if ( it->isPassable3( unit, false ) ) {
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
                    ArenaNode & unitNode = _cache[unitIdx];

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
        else {
            // Walkers - explore moves sequentially from both head and tail cells
            std::vector<int32_t> nodesToExplore;
            nodesToExplore.push_back( pathStart );
            if ( unitIsWide )
                nodesToExplore.push_back( unitTail->GetIndex() );

            for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
                const int32_t fromNode = nodesToExplore[lastProcessedNode];
                const ArenaNode & previousNode = _cache[fromNode];

                Indexes availableMoves;
                if ( !unitIsWide )
                    availableMoves = Board::GetAroundIndexes( fromNode );
                else if ( previousNode._from < 0 )
                    availableMoves = Board::GetMoveWideIndexes( fromNode, unit.isReflect() );
                else
                    availableMoves = Board::GetMoveWideIndexes( fromNode, ( RIGHT_SIDE & Board::GetDirection( fromNode, previousNode._from ) ) );

                for ( const int32_t newNode : availableMoves ) {
                    const Cell * headCell = Board::GetCell( newNode );
                    const bool isLeftDirection = unitIsWide && Board::IsLeftDirection( fromNode, newNode, previousNode._isLeftDirection );

                    const int32_t newTailIndex = isLeftDirection ? newNode + 1 : newNode - 1;
                    const Cell * tailCell = ( unitIsWide && !startPosition.contains( newTailIndex ) ) ? Board::GetCell( newTailIndex ) : nullptr;

                    // Special case: headCell is *allowed* to have another unit in it, that's why we check isPassable1( false ) instead of isPassable4
                    if ( headCell->isPassable1( false ) && ( !tailCell || tailCell->isPassable1( true ) )
                         && ( isPassableBridge || !Board::isBridgeIndex( newNode, unit.GetColor() ) ) ) {
                        const uint32_t cost = previousNode._cost;
                        ArenaNode & node = _cache[newNode];

                        // Check if we're turning back. No movement at all.
                        uint32_t additionalCost = 1u;
                        if ( isLeftDirection != previousNode._isLeftDirection ) {
                            additionalCost = 0;
                        }
                        // Moat penalty consumes all remaining movement. Be careful when dealing with unsigned values.
                        else if ( isMoatBuilt && ( Board::isMoatIndex( newNode, unit.GetColor() ) || Board::isMoatIndex( newTailIndex, unit.GetColor() ) )
                                  && moatPenalty > previousNode._cost ) {
                            additionalCost = moatPenalty - cost;
                        }

                        // Now we check if headCell has a unit - this determines if hex is passable or just accessible (for attack)
                        if ( headCell->GetUnit() && cost < node._cost ) {
                            node._isOpen = false;
                            node._from = fromNode;
                            node._cost = cost;
                            node._isLeftDirection = isLeftDirection;
                        }
                        else if ( cost + additionalCost < node._cost ) {
                            node._isOpen = true;
                            node._from = fromNode;
                            node._cost = cost + additionalCost;
                            node._isLeftDirection = isLeftDirection;
                            nodesToExplore.push_back( newNode );
                        }
                    }
                }
            }
        }
    }
}
