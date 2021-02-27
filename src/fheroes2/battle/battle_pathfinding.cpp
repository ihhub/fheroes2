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
        return _cache[targetCell]._cost == 0 || ( _cache[targetCell]._isOpen && _cache[targetCell]._from != -1 );
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

        const Cell * unitHead = unit.GetPosition().GetHead();
        const Cell * unitTail = unit.GetPosition().GetTail();
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
        const int32_t headIdx = unitHead->GetIndex();
        _cache[headIdx]._cost = 0;
        _cache[headIdx]._isOpen = false;
        _cache[headIdx]._isLeftDirection = unit.isReflect();

        if ( unitIsWide ) {
            const int32_t tailIdx = unitTail->GetIndex();
            _cache[tailIdx]._from = headIdx;
            _cache[tailIdx]._cost = 0;
            _cache[tailIdx]._isOpen = true;
            _cache[headIdx]._isLeftDirection = !unit.isReflect();
        }

        if ( unit.isFlying() ) {
            const Board & board = *Arena::GetBoard();

            for ( Board::const_iterator it = board.begin(); it != board.end(); ++it ) {
                const int32_t idx = it->GetIndex();
                ArenaNode & node = _cache[idx];

                if ( it->isPassable1( true ) ) {
                    node._isOpen = true;
                    node._from = headIdx;
                    node._cost = Battle::Board::GetDistance( headIdx, idx );
                }
                else {
                    node._isOpen = false;
                }
            }
            for ( Board::const_iterator it = board.begin(); it != board.end(); ++it ) {
                const Unit * boardUnit = it->GetUnit();
                if ( boardUnit && boardUnit->GetUID() != unit.GetUID() ) {
                    const int32_t unitIdx = it->GetIndex();
                    ArenaNode & unitNode = _cache[unitIdx];

                    const Indexes & around = Battle::Board::GetAroundIndexes( unitIdx );
                    for ( const int32_t cell : around ) {
                        const uint32_t flyingDist = static_cast<uint32_t>( Battle::Board::GetDistance( headIdx, cell ) );
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
            std::vector<int32_t> nodesToExplore;
            nodesToExplore.push_back( headIdx );
            if ( unitIsWide )
                nodesToExplore.push_back( unitTail->GetIndex() );

            for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
                const int32_t fromNode = nodesToExplore[lastProcessedNode];
                ArenaNode & previousNode = _cache[fromNode];

                Indexes aroundCellIds;
                if ( !unitIsWide )
                    aroundCellIds = Board::GetAroundIndexes( fromNode );
                else if ( previousNode._from < 0 )
                    aroundCellIds = Board::GetMoveWideIndexes( fromNode, unit.isReflect() );
                else
                    aroundCellIds = Board::GetMoveWideIndexes( fromNode, ( RIGHT_SIDE & Board::GetDirection( fromNode, previousNode._from ) ) );

                for ( const int32_t newNode : aroundCellIds ) {
                    const Cell * headCell = Board::GetCell( newNode );

                    const bool isLeftDirection = unitIsWide && Board::IsLeftDirection( fromNode, newNode, previousNode._isLeftDirection );
                    const Cell * tailCell = unitIsWide ? Board::GetCell( isLeftDirection ? newNode + 1 : newNode - 1 ) : nullptr;

                    if ( headCell->isPassable1( false ) && ( !tailCell || tailCell->isPassable1( false ) ) && ( isPassableBridge || !Board::isBridgeIndex( newNode ) ) ) {
                        const uint32_t cost = _cache[fromNode]._cost;
                        ArenaNode & node = _cache[newNode];

                        // Check if we're turning back. No movement at all.
                        uint32_t additionalCost = ( isLeftDirection != previousNode._isLeftDirection ) ? 0 : 1;

                        // Moat penalty consumes all remaining movement. Be careful when dealing with unsigned values
                        if ( isMoatBuilt && Board::isMoatIndex( newNode ) ) {
                            additionalCost += ( moatPenalty > previousNode._cost ) ? moatPenalty - previousNode._cost : 1u;
                        }

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
