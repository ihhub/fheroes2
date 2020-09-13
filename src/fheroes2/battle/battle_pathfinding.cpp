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

namespace Battle
{
    // If move is not valid this function returns -1 otherwise returns new index after the move.
    int GetValidMoveIndex( int fromCell, int directionMask, bool isWide )
    {
        int newIndex = -1;
        int wideUnitOffset = 0;
        const int x = fromCell % ARENAW;
        const int y = fromCell / ARENAW;

        switch ( directionMask ) {
        case Battle::TOP_LEFT:
            if ( y > 0 && ( x > 0 || ( y % 2 ) == 0 ) ) {
                newIndex = fromCell - ARENAW - ( ( y % 2 ) ? 1 : 0 );
                wideUnitOffset = 1;
            }
            break;
        case Battle::TOP_RIGHT:
            if ( y > 0 && ( x < ARENAW - 1 || ( y % 2 ) == 1 ) ) {
                newIndex = fromCell - ARENAW + ( ( y % 2 ) ? 0 : 1 );
                wideUnitOffset = -1;
            }
            break;
        case Battle::RIGHT:
            if ( x < ARENAW - 1 )
                newIndex = fromCell + 1;
            break;
        case Battle::BOTTOM_RIGHT:
            if ( y < ARENAH - 1 && ( x < ARENAW - 1 || ( y % 2 ) == 1 ) ) {
                newIndex = fromCell + ARENAW + ( ( y % 2 ) ? 0 : 1 );
                wideUnitOffset = -1;
            }
            break;
        case Battle::BOTTOM_LEFT:
            if ( y < ARENAH - 1 && ( x > 0 || ( y % 2 ) == 0 ) ) {
                newIndex = fromCell + ARENAW - ( ( y % 2 ) ? 1 : 0 );
                wideUnitOffset = 1;
            }
            break;
        case Battle::LEFT:
            if ( x > 0 )
                newIndex = fromCell - 1;
            break;
        default:
            break;
        }

        // invalidate move
        if ( newIndex != -1 && ( !Board::GetCell( newIndex )->isPassable1( true ) ) )
            return -1;

        if ( isWide && ( x + wideUnitOffset < 0 || x + wideUnitOffset > ARENAW - 1 || !Board::GetCell( newIndex + wideUnitOffset )->isPassable1( true ) ) )
            return -1;
        
        return newIndex;
    }

    ArenaPathfinder::ArenaPathfinder()
        : _cache( ARENASIZE )
    {}

    void ArenaPathfinder::reset()
    {
        for ( size_t i = 0; i < _cache.size(); ++i ) {
            _cache[i]._from = -1;
            _cache[i]._cost = 0;
            _cache[i]._isOpen = true;
        }
    }

    uint32_t ArenaPathfinder::getDistance( int targetCell ) const
    {
        return _cache[targetCell]._cost;
    }

    std::vector<int> ArenaPathfinder::getPath( int targetCell ) const
    {
        std::vector<int> path;

        int nodeID = targetCell;
        const ArenaNode & node = _cache[targetCell];
        while ( _cache[nodeID]._cost != 0 ) {
            path.push_back( nodeID );
            nodeID = _cache[nodeID]._from;
        }

        if ( path.size() > 1 )
            std::reverse( path.begin(), path.end() );
            
        return path;
    }

    void ArenaPathfinder::calculate( const Position & start, bool isWide )
    {
        reset();

        std::vector<int> nodesToExplore;

        if ( start.GetHead() ) {
            const int headIdx = start.GetHead()->GetIndex();
            nodesToExplore.push_back( headIdx );
            _cache[headIdx]._isOpen = false;
        }
        if ( start.GetTail() ) {
            const int tailIdx = start.GetTail()->GetIndex();
            nodesToExplore.push_back( tailIdx );
            _cache[tailIdx]._isOpen = false;
        }

        for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
            const int fromNode = nodesToExplore[lastProcessedNode];

            for ( int direction = TOP_LEFT; direction < CENTER; direction = direction << 1 ) {
                const int newNode = GetValidMoveIndex( fromNode, direction, isWide );

                if ( newNode != -1 ) {
                    const uint16_t cost = _cache[fromNode]._cost + 1;
                    ArenaNode & node = _cache[newNode];
                    if ( node._isOpen || cost < node._cost ) {
                        node._isOpen = false;
                        node._from = fromNode;
                        node._cost = cost;
                        nodesToExplore.push_back( newNode );
                    }
                }
            }
        }
    }
}
