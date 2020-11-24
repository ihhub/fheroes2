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
#include "battle_troop.h"
#include <algorithm>

namespace Battle
{
    // If move is not valid this function returns -1 otherwise returns new index after the move.
    int GetValidMoveIndex( int fromCell, int directionMask, bool isWide )
    {
        int newIndex = -1;
        int wideUnitOffset = 0;
        const int x = fromCell % ARENAW;
        const int y = fromCell / ARENAW;
        const bool isOddRow = ( y % 2 ) == 1;

        switch ( directionMask ) {
        case Battle::TOP_LEFT:
            if ( y > 0 && ( x > 0 || !isOddRow ) ) {
                newIndex = fromCell - ARENAW - ( isOddRow ? 1 : 0 );
                wideUnitOffset = 1;
            }
            break;
        case Battle::TOP_RIGHT:
            if ( y > 0 && ( x < ARENAW - 1 || isOddRow ) ) {
                newIndex = fromCell - ARENAW + ( isOddRow ? 0 : 1 );
                wideUnitOffset = -1;
            }
            break;
        case Battle::RIGHT:
            if ( x < ARENAW - 1 ) {
                newIndex = fromCell + 1;
                wideUnitOffset = -1;
            }
            break;
        case Battle::BOTTOM_RIGHT:
            if ( y < ARENAH - 1 && ( x < ARENAW - 1 || isOddRow ) ) {
                newIndex = fromCell + ARENAW + ( isOddRow ? 0 : 1 );
                wideUnitOffset = -1;
            }
            break;
        case Battle::BOTTOM_LEFT:
            if ( y < ARENAH - 1 && ( x > 0 || !isOddRow ) ) {
                newIndex = fromCell + ARENAW - ( isOddRow ? 1 : 0 );
                wideUnitOffset = 1;
            }
            break;
        case Battle::LEFT:
            if ( x > 0 ) {
                newIndex = fromCell - 1;
                wideUnitOffset = 1;
            }
            break;
        default:
            return -1;
        }

        // invalidate move
        if ( newIndex == -1 || !Board::GetCell( newIndex )->isPassable1( false ) )
            return -1;

        if ( isWide && ( x + wideUnitOffset < 0 || x + wideUnitOffset > ARENAW - 1 || !Board::GetCell( newIndex + wideUnitOffset )->isPassable1( false ) ) )
            return -1;

        return newIndex;
    }

    void ArenaNode::resetNode()
    {
        _from = -1;
        _cost = MAX_MOVE_COST;
        _objectID = 0;
        _isOpen = true;
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

        const Cell * unitHead = unit.GetPosition().GetHead();
        if ( !unitHead ) {
            DEBUG( DBG_BATTLE, DBG_WARN, "Pathfinder: Invalid unit is passed in! " << unit.GetName() );
            return;
        }

        const Cell * unitTail = unit.GetPosition().GetTail();
        const bool unitIsWide = unit.isWide();

        const int headIdx = unitHead->GetIndex();
        _cache[headIdx]._cost = 0;
        _cache[headIdx]._isOpen = false;

        int tailIdx = -1;
        if ( unitTail ) {
            tailIdx = unitTail->GetIndex();
            _cache[tailIdx]._cost = 0;
            _cache[tailIdx]._isOpen = false;
        }

        if ( unit.isFlying() ) {
            const Board & board = *Arena::GetBoard();

            for ( Board::const_iterator it = board.begin(); it != board.end(); ++it ) {
                const int idx = it->GetIndex();
                ArenaNode & node = _cache[idx];

                if ( it->isPassable1( true ) ) {
                    node._isOpen = true;
                    node._from = headIdx;
                    node._cost = board.GetDistance( headIdx, idx );
                }
                else {
                    node._isOpen = false;
                }
            }
            for ( Board::const_iterator it = board.begin(); it != board.end(); ++it ) {
                const Unit * boardUnit = it->GetUnit();
                if ( boardUnit && boardUnit->GetUID() != unit.GetUID() ) {
                    const int unitIdx = it->GetIndex();
                    ArenaNode & unitNode = _cache[unitIdx];

                    const Indexes & around = board.GetAroundIndexes( unitIdx );
                    for ( const int cell : around ) {
                        const uint32_t flyingDist = static_cast<uint32_t>( board.GetDistance( headIdx, cell ) );
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
            std::vector<int> nodesToExplore;
            nodesToExplore.push_back( headIdx );

            if ( tailIdx != -1 )
                nodesToExplore.push_back( tailIdx );

            for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
                const int fromNode = nodesToExplore[lastProcessedNode];

                for ( int direction = TOP_LEFT; direction < CENTER; direction = direction << 1 ) {
                    const int newNode = GetValidMoveIndex( fromNode, direction, unitIsWide );

                    if ( newNode != -1 ) {
                        const uint32_t cost = _cache[fromNode]._cost;
                        ArenaNode & node = _cache[newNode];

                        if ( Board::GetCell( newNode )->GetUnit() && cost < node._cost ) {
                            node._isOpen = false;
                            node._from = fromNode;
                            node._cost = cost;
                        }
                        else if ( cost + 1 < node._cost ) {
                            node._isOpen = true;
                            node._from = fromNode;
                            node._cost = cost + 1;
                            nodesToExplore.push_back( newNode );
                        }
                    }
                }
            }
        }
    }
}
