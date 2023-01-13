/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2BATTLE_CELL_H
#define H2BATTLE_CELL_H

#include <cstdint>
#include <utility>

#include "math_base.h"

#define CELLW 44
#define CELLH 52

namespace Battle
{
    class Unit;

    enum direction_t
    {
        UNKNOWN = 0x00,
        TOP_LEFT = 0x01,
        TOP_RIGHT = 0x02,
        RIGHT = 0x04,
        BOTTOM_RIGHT = 0x08,
        BOTTOM_LEFT = 0x10,
        LEFT = 0x20,
        CENTER = 0x40,
        RIGHT_SIDE = TOP_RIGHT | RIGHT | BOTTOM_RIGHT,
        LEFT_SIDE = TOP_LEFT | LEFT | BOTTOM_LEFT,
        AROUND = RIGHT_SIDE | LEFT_SIDE
    };

    class Cell
    {
    public:
        explicit Cell( int32_t );
        Cell( const Cell & ) = delete;
        Cell( Cell && ) = default;

        Cell & operator=( const Cell & ) = delete;
        Cell & operator=( Cell && ) = delete;

        void ResetQuality();

        void SetObject( int );
        void SetQuality( uint32_t );

        void SetArea( const fheroes2::Rect & );

        // Checks that the cell is passable for the given unit located in a certain adjacent cell
        bool isPassableFromAdjacent( const Unit & unit, const Cell & adjacent ) const;
        // Checks that the cell is passable for the given unit, i.e. unit can occupy it with his head or tail
        bool isPassableForUnit( const Unit & unit ) const;
        // Checks that the cell is passable, i.e. does not contain an obstacle or (optionally) a unit
        bool isPassable( const bool checkForUnit ) const;

        bool isPositionIncludePoint( const fheroes2::Point & ) const;

        int32_t GetIndex() const;
        const fheroes2::Rect & GetPos() const;
        int GetObject() const;
        int32_t GetQuality() const;
        direction_t GetTriangleDirection( const fheroes2::Point & ) const;

        const Unit * GetUnit() const;
        Unit * GetUnit();
        void SetUnit( Unit * );

    private:
        int32_t index;
        fheroes2::Rect pos;
        int object;
        int32_t quality;
        Unit * troop;
        fheroes2::Point coord[7];
    };

    class Position : protected std::pair<Cell *, Cell *>
    {
    public:
        Position()
            : std::pair<Cell *, Cell *>( nullptr, nullptr )
        {}

        void Set( const int32_t head, const bool wide, const bool reflect );
        void Swap();
        bool isReflect() const;
        bool contains( int cellIndex ) const;

        // Returns the position that the given unit would occupy after moving to the cell
        // with the given index (without taking into account the current pathfinder graph)
        // or an empty Position object if the given index is unreachable in principle for
        // the given unit
        static Position GetPosition( const Unit & unit, const int32_t dst );

        // Returns the reachable position for the current unit (to which the current
        // pathfinder graph relates) which corresponds to the given index or an empty
        // Position object if the given index is unreachable on the current turn
        static Position GetReachable( const Unit & currentUnit, const int32_t dst );

        fheroes2::Rect GetRect() const;
        Cell * GetHead();
        const Cell * GetHead() const;
        Cell * GetTail();
        const Cell * GetTail() const;
    };
}

#endif
