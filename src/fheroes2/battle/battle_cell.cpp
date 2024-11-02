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

#include "battle_cell.h"

#include <cassert>

#include "battle_arena.h"
#include "battle_board.h"
#include "battle_troop.h"
#include "math_tools.h"

namespace
{
    bool inABC( const fheroes2::Point & base, const fheroes2::Point & pt1, const fheroes2::Point & pt2, const fheroes2::Point & pt3 )
    {
        const int32_t a = ( pt1.x - base.x ) * ( pt2.y - pt1.y ) - ( pt2.x - pt1.x ) * ( pt1.y - base.y );
        const int32_t b = ( pt2.x - base.x ) * ( pt3.y - pt2.y ) - ( pt3.x - pt2.x ) * ( pt2.y - base.y );
        const int32_t c = ( pt3.x - base.x ) * ( pt1.y - pt3.y ) - ( pt1.x - pt3.x ) * ( pt3.y - base.y );

        return ( ( a >= 0 && b >= 0 && c >= 0 ) || ( a < 0 && b < 0 && c < 0 ) );
    }

    const int32_t cellHeightVerSide = 32;
    const int32_t infl = 12;
}

void Battle::Position::Set( const int32_t head, const bool wide, const bool reflect )
{
    first = Board::GetCell( head );

    if ( first && wide ) {
        second = Board::GetCell( first->GetIndex(), reflect ? RIGHT : LEFT );
    }
    else {
        second = nullptr;
    }
}

void Battle::Position::Swap()
{
    if ( first == nullptr || second == nullptr ) {
        return;
    }

    std::swap( first, second );
}

fheroes2::Rect Battle::Position::GetRect() const
{
    if ( first == nullptr ) {
        return {};
    }

    return second ? getBoundaryRect( first->GetPos(), second->GetPos() ) : first->GetPos();
}

Battle::Position Battle::Position::GetPosition( const Unit & unit, const int32_t dst )
{
    Position result;

    if ( unit.isWide() ) {
        const auto checkCells = [&unit]( Cell * headCell, Cell * tailCell ) {
            Position pos;

            if ( headCell == nullptr || ( !unit.GetPosition().contains( headCell->GetIndex() ) && !headCell->isPassable( true ) ) ) {
                return pos;
            }

            if ( tailCell == nullptr || ( !unit.GetPosition().contains( tailCell->GetIndex() ) && !tailCell->isPassable( true ) ) ) {
                return pos;
            }

            pos.first = headCell;
            pos.second = tailCell;

            return pos;
        };

        const int tailDirection = unit.isReflect() ? RIGHT : LEFT;

        if ( Board::isValidDirection( dst, tailDirection ) ) {
            Cell * headCell = Board::GetCell( dst );
            Cell * tailCell = Board::GetCell( Board::GetIndexDirection( dst, tailDirection ) );

            result = checkCells( headCell, tailCell );
        }

        if ( result.GetHead() == nullptr || result.GetTail() == nullptr ) {
            const int headDirection = unit.isReflect() ? LEFT : RIGHT;

            if ( Board::isValidDirection( dst, headDirection ) ) {
                Cell * headCell = Board::GetCell( Board::GetIndexDirection( dst, headDirection ) );
                Cell * tailCell = Board::GetCell( dst );

                result = checkCells( headCell, tailCell );
            }
        }
    }
    else {
        Cell * headCell = Board::GetCell( dst );

        if ( headCell != nullptr && ( unit.GetPosition().contains( headCell->GetIndex() ) || headCell->isPassable( true ) ) ) {
            result.first = headCell;
        }
    }

    return result;
}

Battle::Position Battle::Position::GetReachable( const Unit & unit, const int32_t dst, const std::optional<uint32_t> speed /* = {} */ )
{
    Arena * arena = GetArena();
    assert( arena != nullptr );

    const auto checkReachability = [&unit, speed, arena]( const Position & pos ) -> Position {
        if ( speed ) {
            if ( arena->isPositionReachable( unit, pos, false ) && arena->CalculateMoveCost( unit, pos ) <= *speed ) {
                return pos;
            }
        }
        else if ( arena->isPositionReachable( unit, pos, true ) ) {
            return pos;
        }

        return {};
    };

    if ( unit.isWide() ) {
        const auto checkCells = [&checkReachability]( Cell * headCell, Cell * tailCell ) -> Position {
            if ( headCell == nullptr || tailCell == nullptr ) {
                return {};
            }

            Position pos;

            pos.first = headCell;
            pos.second = tailCell;

            return checkReachability( pos );
        };

        const auto tryHead = [&unit, dst, &checkCells]() -> Position {
            const int tailDirection = unit.isReflect() ? RIGHT : LEFT;

            if ( Board::isValidDirection( dst, tailDirection ) ) {
                Cell * headCell = Board::GetCell( dst );
                Cell * tailCell = Board::GetCell( Board::GetIndexDirection( dst, tailDirection ) );

                return checkCells( headCell, tailCell );
            }

            return {};
        };

        const auto tryTail = [&unit, dst, &checkCells]() -> Position {
            const int headDirection = unit.isReflect() ? LEFT : RIGHT;

            if ( Board::isValidDirection( dst, headDirection ) ) {
                Cell * headCell = Board::GetCell( Board::GetIndexDirection( dst, headDirection ) );
                Cell * tailCell = Board::GetCell( dst );

                return checkCells( headCell, tailCell );
            }

            return {};
        };

        Position headPos = tryHead();

        if ( headPos.GetHead() != nullptr && headPos.GetTail() != nullptr ) {
            return headPos;
        }

        return tryTail();
    }

    Cell * headCell = Board::GetCell( dst );

    if ( headCell == nullptr ) {
        return {};
    }

    Position pos;

    pos.first = headCell;

    return checkReachability( pos );
}

bool Battle::Position::isReflect() const
{
    return first && second && first->GetIndex() < second->GetIndex();
}

bool Battle::Position::contains( const int32_t idx ) const
{
    return ( first && first->GetIndex() == idx ) || ( second && second->GetIndex() == idx );
}

bool Battle::Position::isEmpty() const
{
    if ( first != nullptr ) {
        return false;
    }

    assert( second == nullptr );

    return true;
}

bool Battle::Position::isValidForUnit( const Unit & unit ) const
{
    if ( first == nullptr ) {
        return false;
    }

    return unit.isWide() ? second != nullptr : second == nullptr;
}

bool Battle::Position::operator<( const Position & other ) const
{
    assert( first == nullptr || Board::isValidIndex( first->GetIndex() ) );
    assert( second == nullptr || Board::isValidIndex( second->GetIndex() ) );

    assert( other.first == nullptr || Board::isValidIndex( other.first->GetIndex() ) );
    assert( other.second == nullptr || Board::isValidIndex( other.second->GetIndex() ) );

    // It is necessary to guarantee a stable order of positions, it should depend on the indexes of cells, and not on the values of pointers to these cells
    const auto theseIndexes = std::make_pair( first ? first->GetIndex() : -1, second ? second->GetIndex() : -1 );
    const auto otherIndexes = std::make_pair( other.first ? other.first->GetIndex() : -1, other.second ? other.second->GetIndex() : -1 );

    return theseIndexes < otherIndexes;
}

Battle::Cell::Cell( const int32_t idx )
    : _index( idx )
    , _object( 0 )
    , _unit( nullptr )
{
    SetArea( fheroes2::Rect() );
}

void Battle::Cell::SetArea( const fheroes2::Rect & area )
{
    _pos.x = area.x + 89 - ( ( ( _index / Board::widthInCells ) % 2 ) ? widthPx / 2 : 0 ) + widthPx * ( _index % Board::widthInCells );
    _pos.y = area.y + 62 + ( ( heightPx - ( heightPx - cellHeightVerSide ) / 2 ) * ( _index / Board::widthInCells ) );
    _pos.width = widthPx;
    _pos.height = heightPx;

    // center
    _coord[0] = { infl * _pos.x + infl * _pos.width / 2, infl * _pos.y + infl * _pos.height / 2 };
    // coordinates
    _coord[1] = { infl * _pos.x, infl * _pos.y + infl * ( _pos.height - cellHeightVerSide ) / 2 };
    _coord[2] = { infl * _pos.x + infl * _pos.width / 2, infl * _pos.y };
    _coord[3] = { infl * _pos.x + infl * _pos.width, infl * _pos.y + infl * ( _pos.height - cellHeightVerSide ) / 2 };
    _coord[4] = { infl * _pos.x + infl * _pos.width, infl * _pos.y + infl * _pos.height - infl * ( _pos.height - cellHeightVerSide ) / 2 };
    _coord[5] = { infl * _pos.x + infl * _pos.width / 2, infl * _pos.y + infl * _pos.height };
    _coord[6] = { infl * _pos.x, infl * _pos.y + infl * _pos.height - infl * ( _pos.height - cellHeightVerSide ) / 2 };
}

Battle::CellDirection Battle::Cell::GetTriangleDirection( const fheroes2::Point & dst ) const
{
    const fheroes2::Point pt( infl * dst.x, infl * dst.y );

    if ( pt == _coord[0] ) {
        return CENTER;
    }
    if ( inABC( pt, _coord[0], _coord[1], _coord[2] ) ) {
        return TOP_LEFT;
    }
    if ( inABC( pt, _coord[0], _coord[2], _coord[3] ) ) {
        return TOP_RIGHT;
    }
    if ( inABC( pt, _coord[0], _coord[3], _coord[4] ) ) {
        return RIGHT;
    }
    if ( inABC( pt, _coord[0], _coord[4], _coord[5] ) ) {
        return BOTTOM_RIGHT;
    }
    if ( inABC( pt, _coord[0], _coord[5], _coord[6] ) ) {
        return BOTTOM_LEFT;
    }
    if ( inABC( pt, _coord[0], _coord[1], _coord[6] ) ) {
        return LEFT;
    }

    return UNKNOWN;
}

bool Battle::Cell::isPositionIncludePoint( const fheroes2::Point & pt ) const
{
    return UNKNOWN != GetTriangleDirection( pt );
}

int32_t Battle::Cell::GetIndex() const
{
    return _index;
}

void Battle::Cell::SetObject( const int object )
{
    _object = object;
}

int Battle::Cell::GetObject() const
{
    return _object;
}

const fheroes2::Rect & Battle::Cell::GetPos() const
{
    return _pos;
}

const Battle::Unit * Battle::Cell::GetUnit() const
{
    return _unit;
}

Battle::Unit * Battle::Cell::GetUnit()
{
    return _unit;
}

void Battle::Cell::SetUnit( Unit * unit )
{
    _unit = unit;
}

bool Battle::Cell::isPassableFromAdjacent( const Unit & unit, const Cell & adjacent ) const
{
    assert( Board::isNearIndexes( _index, adjacent._index ) );

    if ( unit.isWide() ) {
        const int dir = Board::GetDirection( adjacent._index, _index );

        switch ( dir ) {
        case BOTTOM_RIGHT:
        case TOP_RIGHT:
        case BOTTOM_LEFT:
        case TOP_LEFT: {
            const bool reflect = ( ( BOTTOM_LEFT | TOP_LEFT ) & dir ) != 0;
            const Cell * tail = Board::GetCell( _index, reflect ? RIGHT : LEFT );

            return tail && tail->isPassable( true ) && isPassable( true );
        }

        case LEFT:
        case RIGHT:
            return isPassable( true ) || _index == unit.GetTailIndex();

        default:
            break;
        }
    }

    return isPassable( true );
}

bool Battle::Cell::isPassableForUnit( const Unit & unit ) const
{
    const Position unitPos = Position::GetPosition( unit, _index );
    if ( unitPos.GetHead() == nullptr ) {
        return false;
    }

    assert( unitPos.isValidForUnit( unit ) );

    return true;
}

bool Battle::Cell::isPassable( const bool checkForUnit ) const
{
    return _object == 0 && ( !checkForUnit || _unit == nullptr );
}
