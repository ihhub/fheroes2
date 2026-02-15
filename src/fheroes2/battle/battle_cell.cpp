/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2026                                             *
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
        second = Board::GetCell( first->GetIndex(), reflect ? CellDirection::RIGHT : CellDirection::LEFT );
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

        const CellDirection tailDirection = unit.isReflect() ? CellDirection::RIGHT : CellDirection::LEFT;

        if ( Board::isValidDirection( dst, tailDirection ) ) {
            Cell * headCell = Board::GetCell( dst );
            Cell * tailCell = Board::GetCell( Board::GetIndexDirection( dst, tailDirection ) );

            result = checkCells( headCell, tailCell );
        }

        if ( result.GetHead() == nullptr || result.GetTail() == nullptr ) {
            const CellDirection headDirection = unit.isReflect() ? CellDirection::LEFT : CellDirection::RIGHT;

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
            const CellDirection tailDirection = unit.isReflect() ? CellDirection::RIGHT : CellDirection::LEFT;

            if ( Board::isValidDirection( dst, tailDirection ) ) {
                Cell * headCell = Board::GetCell( dst );
                Cell * tailCell = Board::GetCell( Board::GetIndexDirection( dst, tailDirection ) );

                return checkCells( headCell, tailCell );
            }

            return {};
        };

        Position headPos = tryHead();

        if ( headPos.GetHead() != nullptr && headPos.GetTail() != nullptr ) {
            return headPos;
        }

        const auto tryTail = [&unit, dst, &checkCells]() -> Position {
            const CellDirection headDirection = unit.isReflect() ? CellDirection::LEFT : CellDirection::RIGHT;

            if ( Board::isValidDirection( dst, headDirection ) ) {
                Cell * headCell = Board::GetCell( Board::GetIndexDirection( dst, headDirection ) );
                Cell * tailCell = Board::GetCell( dst );

                return checkCells( headCell, tailCell );
            }

            return {};
        };

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

void Battle::Cell::SetArea( const fheroes2::Rect & area )
{
    _pos.x = area.x + 89 - ( ( ( _index / Board::widthInCells ) % 2 ) ? widthPx / 2 : 0 ) + widthPx * ( _index % Board::widthInCells );
    _pos.y = area.y + 62 + ( ( heightPx - ( heightPx - cellHeightVerSide ) / 2 ) * ( _index / Board::widthInCells ) );
    _pos.width = widthPx;
    _pos.height = heightPx;

    // Cache some calculations.
    const int32_t offsetX = infl * _pos.x;
    const int32_t offsetY = infl * _pos.y;
    const int32_t extraOffsetY = infl * ( _pos.height - cellHeightVerSide ) / 2;

    // center
    _coord[0] = { offsetX + infl * _pos.width / 2, offsetY + infl * _pos.height / 2 };
    // coordinates
    _coord[1] = { offsetX, offsetY + extraOffsetY };
    _coord[2] = { _coord[0].x, offsetY };
    _coord[3] = { offsetX + infl * _pos.width, _coord[1].y };
    _coord[5] = { _coord[0].x, offsetY + infl * _pos.height };
    _coord[4] = { _coord[3].x, _coord[5].y - extraOffsetY };
    _coord[6] = { offsetX, _coord[4].y };
}

Battle::AttackDirection Battle::Cell::GetTriangleDirection( const fheroes2::Point & dst ) const
{
    const fheroes2::Point pt( infl * dst.x, infl * dst.y );

    const fheroes2::Point oneHalf( ( _coord[1].x + _coord[2].x ) / 2, ( _coord[1].y + _coord[2].y ) / 2 );
    const fheroes2::Point twoHalf( ( _coord[2].x + _coord[3].x ) / 2, ( _coord[2].y + _coord[3].y ) / 2 );
    const fheroes2::Point fourHalf( ( _coord[4].x + _coord[5].x ) / 2, ( _coord[4].y + _coord[5].y ) / 2 );
    const fheroes2::Point fiveHalf( ( _coord[5].x + _coord[6].x ) / 2, ( _coord[5].y + _coord[6].y ) / 2 );

    if ( pt == _coord[0] ) {
        return AttackDirection::CENTER;
    }
    if ( inABC( pt, _coord[0], _coord[1], oneHalf ) ) {
        return AttackDirection::TOP_LEFT;
    }
    if ( inABC( pt, _coord[0], oneHalf, twoHalf ) ) {
        return AttackDirection::TOP;
    }
    if ( inABC( pt, _coord[0], twoHalf, _coord[3] ) ) {
        return AttackDirection::TOP_RIGHT;
    }
    if ( inABC( pt, _coord[0], _coord[3], _coord[4] ) ) {
        return AttackDirection::RIGHT;
    }
    if ( inABC( pt, _coord[0], _coord[4], fourHalf ) ) {
        return AttackDirection::BOTTOM_RIGHT;
    }
    if ( inABC( pt, _coord[0], fourHalf, fiveHalf ) ) {
        return AttackDirection::BOTTOM;
    }
    if ( inABC( pt, _coord[0], fiveHalf, _coord[6] ) ) {
        return AttackDirection::BOTTOM_LEFT;
    }
    if ( inABC( pt, _coord[0], _coord[1], _coord[6] ) ) {
        return AttackDirection::LEFT;
    }

    return AttackDirection::UNKNOWN;
}

bool Battle::Cell::isPositionIncludePoint( const fheroes2::Point & pt ) const
{
    return AttackDirection::UNKNOWN != GetTriangleDirection( pt );
}

bool Battle::Cell::isPassableFromAdjacent( const Unit & unit, const Cell & adjacent ) const
{
    assert( Board::isNearIndexes( _index, adjacent._index ) );

    if ( !unit.isWide() ) {
        return isPassable( true );
    }

    const CellDirection dir = Board::GetDirection( adjacent._index, _index );

    if ( dir == CellDirection::LEFT || dir == CellDirection::RIGHT ) {
        return isPassable( true ) || _index == unit.GetTailIndex();
    }

    // When a wide unit moves diagonally to the left (to the top left or bottom left cell), its tail should be on the right side (because units can't move "tail first"),
    // and vise versa - when such a unit moves diagonally to the right, its tail should be on the left side.
    const CellDirection tailDir = ( dir == CellDirection::TOP_LEFT || dir == CellDirection::BOTTOM_LEFT ) ? CellDirection::RIGHT : CellDirection::LEFT;
    // If 'tailDir' is LEFT, then 'dir' is either TOP_RIGHT or BOTTOM_RIGHT
    assert( tailDir == CellDirection::RIGHT || dir == CellDirection::TOP_RIGHT || dir == CellDirection::BOTTOM_RIGHT );

    const Cell * tail = Board::GetCell( _index, tailDir );

    return tail && tail->isPassable( true ) && isPassable( true );
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
