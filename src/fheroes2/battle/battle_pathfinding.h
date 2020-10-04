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

#pragma once

#include "battle_board.h"

namespace Battle
{
    const uint16_t MAX_MOVE_COST = ARENASIZE;

    /* ArenaNode, different situations
     * default:  from: -1, isOpen: true, cost: MAX
     * starting: from: -1, isOpen: false, cost: 0
     * passable: from: 0-98, isOpen: true, cost: 1+
     * oth.unit: from: 0-98, isOpen: false, cost: 0+
     * terrain:  from: -1, isOpen: false, cost: MAX
     * if tile wouldn't be reached it stays as default
     */
    struct ArenaNode
    {
        int _from = -1;
        uint16_t _cost = MAX_MOVE_COST;
        bool _isOpen = true;

        ArenaNode() {}
        ArenaNode( int node, uint16_t cost, bool isOpen )
            : _from( node )
            , _cost( cost )
            , _isOpen( isOpen )
        {}
    };

    class ArenaPathfinder
    {
    public:
        ArenaPathfinder();
        void reset();
        void calculate( const Unit & unit );
        std::vector<int> getPath( int targetCell ) const;
        uint32_t getDistance( int targetCell ) const;
        const ArenaNode & getNode( int targetCell ) const;
        bool hexIsAccessible( int targetCell ) const;
        bool hexIsPassable( int targetCell ) const;

    private:
        std::vector<ArenaNode> _cache;
    };
}
