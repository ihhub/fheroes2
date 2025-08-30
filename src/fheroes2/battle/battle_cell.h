/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <utility>

#include "math_base.h"

namespace Battle
{
    class Unit;

    enum class CellDirection
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
    };

    inline CellDirection operator<<( const CellDirection d, const int shift )
    {
        return static_cast<CellDirection>( static_cast<int>( d ) << shift );
    }

    inline CellDirection operator>>( const CellDirection d, const int shift )
    {
        return static_cast<CellDirection>( static_cast<int>( d ) >> shift );
    }

    inline bool isLeftSide( const CellDirection d )
    {
        return static_cast<int>( d ) & static_cast<int>( CellDirection::LEFT_SIDE );
    }

    inline bool isRightSide( const CellDirection d )
    {
        return static_cast<int>( d ) & static_cast<int>( CellDirection::RIGHT_SIDE );
    }

    enum class AttackDirection
    {
        UNKNOWN = 0x00,
        TOP_LEFT = 0x01,
        TOP = 0x02,
        TOP_RIGHT = 0x04,
        RIGHT = 0x08,
        BOTTOM_RIGHT = 0x10,
        BOTTOM = 0x20,
        BOTTOM_LEFT = 0x40,
        LEFT = 0x80,
        CENTER = 0x100,
    };

    inline AttackDirection operator<<( const AttackDirection d, const int shift )
    {
        return static_cast<AttackDirection>( static_cast<int>( d ) << shift );
    }

    inline AttackDirection operator>>( const AttackDirection d, const int shift )
    {
        return static_cast<AttackDirection>( static_cast<int>( d ) >> shift );
    }

    inline AttackDirection asAttackDirection( const CellDirection d )
    {
        switch ( d ) {
        case CellDirection::TOP_LEFT:
            return AttackDirection::TOP_LEFT;
        case CellDirection::TOP_RIGHT:
            return AttackDirection::TOP_RIGHT;
        case CellDirection::RIGHT:
            return AttackDirection::RIGHT;
        case CellDirection::BOTTOM_RIGHT:
            return AttackDirection::BOTTOM_RIGHT;
        case CellDirection::BOTTOM_LEFT:
            return AttackDirection::BOTTOM_LEFT;
        case CellDirection::LEFT:
            return AttackDirection::LEFT;
        default:
            return AttackDirection::UNKNOWN;
        }
    }

    class Cell final
    {
    public:
        // Width of the rendered cell in pixels
        static constexpr int widthPx{ 44 };
        // Height of the rendered cell in pixels
        static constexpr int heightPx{ 52 };

        explicit Cell( const int32_t idx )
            : _index( idx )
        {
            SetArea( {} );
        }

        Cell( const Cell & ) = delete;
        Cell( Cell && ) = default;

        ~Cell() = default;

        Cell & operator=( const Cell & ) = delete;
        Cell & operator=( Cell && ) = delete;

        int32_t GetIndex() const
        {
            return _index;
        }

        const fheroes2::Rect & GetPos() const
        {
            return _pos;
        }

        int GetObject() const
        {
            return _object;
        }

        const Unit * GetUnit() const
        {
            return _unit;
        }

        Unit * GetUnit()
        {
            return _unit;
        }

        AttackDirection GetTriangleDirection( const fheroes2::Point & dst ) const;

        bool isPositionIncludePoint( const fheroes2::Point & pt ) const;

        void SetArea( const fheroes2::Rect & area );

        void SetObject( const int object )
        {
            _object = object;
        }

        void SetUnit( Unit * unit )
        {
            _unit = unit;
        }

        // Checks that the cell is passable for a given unit located in a certain adjacent cell
        bool isPassableFromAdjacent( const Unit & unit, const Cell & adjacent ) const;
        // Checks that the cell is passable for a given unit, i.e. unit can occupy it with his head or tail
        bool isPassableForUnit( const Unit & unit ) const;
        // Checks that the cell is passable, i.e. does not contain an obstacle or (optionally) a unit
        bool isPassable( const bool checkForUnit ) const
        {
            return _object == 0 && ( !checkForUnit || _unit == nullptr );
        }

    private:
        int32_t _index{ 0 };
        fheroes2::Rect _pos;
        int _object{ 0 };
        Unit * _unit{ nullptr };
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
