/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#include <array>
#include <cstdint>
#include <optional>
#include <utility>

#include "math_base.h"

namespace Battle
{
    class Unit;

    enum CellDirection : int
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

    class Cell final
    {
    public:
        // Width of the rendered cell in pixels
        static constexpr int widthPx{ 44 };
        // Height of the rendered cell in pixels
        static constexpr int heightPx{ 52 };

        explicit Cell( const int32_t idx );
        Cell( const Cell & ) = delete;
        Cell( Cell && ) = default;

        ~Cell() = default;

        Cell & operator=( const Cell & ) = delete;
        Cell & operator=( Cell && ) = delete;

        int32_t GetIndex() const;
        const fheroes2::Rect & GetPos() const;
        int GetObject() const;

        const Unit * GetUnit() const;
        Unit * GetUnit();

        CellDirection GetTriangleDirection( const fheroes2::Point & dst ) const;

        bool isPositionIncludePoint( const fheroes2::Point & pt ) const;

        void SetArea( const fheroes2::Rect & area );
        void SetObject( const int object );
        void SetUnit( Unit * unit );

        // Checks that the cell is passable for a given unit located in a certain adjacent cell
        bool isPassableFromAdjacent( const Unit & unit, const Cell & adjacent ) const;
        // Checks that the cell is passable for a given unit, i.e. unit can occupy it with his head or tail
        bool isPassableForUnit( const Unit & unit ) const;
        // Checks that the cell is passable, i.e. does not contain an obstacle or (optionally) a unit
        bool isPassable( const bool checkForUnit ) const;

    private:
        int32_t _index;
        fheroes2::Rect _pos;
        int _object;
        Unit * _unit;
        std::array<fheroes2::Point, 7> _coord;
    };

    class Position final : protected std::pair<Cell *, Cell *>
    {
    public:
        Position()
            : std::pair<Cell *, Cell *>( nullptr, nullptr )
        {}

        void Set( const int32_t head, const bool wide, const bool reflect );
        void Swap();

        bool isReflect() const;
        bool contains( const int32_t idx ) const;

        // Returns the position that a given unit would occupy after moving to the cell
        // with a given index (without taking into account the pathfinder's info) or an
        // empty Position object if this index is unreachable in principle for this unit.
        static Position GetPosition( const Unit & unit, const int32_t dst );

        // Returns the reachable position for a given unit, which corresponds to a given
        // index, or an empty Position object if this index is unreachable for this unit
        // on the current turn. if 'speed' is set, then this value will be used to check
        // the position reachability instead of the speed returned by 'unit'.
        static Position GetReachable( const Unit & unit, const int32_t dst, const std::optional<uint32_t> speed = {} );

        bool isEmpty() const;

        bool isValidForUnit( const Unit & unit ) const;

        bool isValidForUnit( const Unit * unit ) const
        {
            if ( unit == nullptr ) {
                return false;
            }

            return isValidForUnit( *unit );
        }

        fheroes2::Rect GetRect() const;

        const Cell * GetHead() const
        {
            return first;
        }

        const Cell * GetTail() const
        {
            return second;
        }

        Cell * GetHead()
        {
            return first;
        }

        Cell * GetTail()
        {
            return second;
        }

        bool operator<( const Position & other ) const;
    };
}

#endif
