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
    int GetValidMoveIndex( int fromCell, int directionMask, bool isWide )
    {
        int newIndex = -1;
        bool reverse = false;
        const int x = fromCell % ARENAW;
        const int y = fromCell / ARENAW;

        switch ( directionMask ) {
        case Battle::TOP_LEFT:
            if ( y > 0 && ( x > 0 || ( y % 2 ) == 0 ) ) {
                newIndex = fromCell - ARENAW - 1;
                reverse = true;
            }
            break;
        case Battle::TOP_RIGHT:
            if ( y > 0 && ( x < ARENAW - 1 || ( y % 2 ) == 1 ) )
                newIndex = fromCell - ARENAW + 1;
            break;
        case Battle::RIGHT:
            if ( x < ARENAW - 1 )
                newIndex = fromCell + 1;
            break;
        case Battle::BOTTOM_RIGHT:
            if ( y < ARENAH - 1 && ( x < ARENAW - 1 || ( y % 2 ) == 1 ) )
                newIndex = fromCell + ARENAW + 1;
            break;
        case Battle::BOTTOM_LEFT:
            if ( y < ARENAH - 1 && ( x > 0 || ( y % 2 ) == 0 ) ) {
                newIndex = fromCell + ARENAW - 1;
                reverse = true;
            }
            break;
        case Battle::LEFT:
            if ( x > 0 ) {
                newIndex = fromCell - 1;
                reverse = true;
            }
            break;
        default:
            break;
        }

        if ( newIndex != -1
             && ( !Board::GetCell( newIndex )->isPassable1( true ) || ( isWide && !Board::GetCell( newIndex + ( reverse ? -1 : 1 ) )->isPassable1( true ) ) ) ) {
            // invalidate move
            newIndex = -1;
        }
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
        }
    }

    void ArenaPathfinder::calculate( int index, bool isWide )
    {
        std::vector<int> nodesToExplore;
        nodesToExplore.push_back( index );
        for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
            const int nodeIdx = nodesToExplore[lastProcessedNode];

            for ( int direction = TOP_LEFT; direction < CENTER; direction << 1 ) {
                const int newNode = GetValidMoveIndex( nodeIdx, direction, isWide );

                if ( newNode != -1 ) {
                    const uint16_t cost = _cache[nodeIdx]._cost + 1;
                    ArenaNode & node = _cache[newNode];
                    if ( node._from == -1 || cost < node._cost ) {
                        node._from = nodeIdx;
                        node._cost = _cache[nodeIdx]._cost + 1;
                        nodesToExplore.push_back( Board::GetIndexDirection( newNode, direction ) );
                    }
                }
            }
        }
    }
}
