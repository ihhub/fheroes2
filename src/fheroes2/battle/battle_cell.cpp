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

#include <cassert>

#include "battle_arena.h"
#include "battle_board.h"
#include "battle_cell.h"
#include "battle_troop.h"
#include "tools.h"

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

    if ( first && wide )
        second = Board::GetCell( first->GetIndex(), reflect ? RIGHT : LEFT );
}

void Battle::Position::Swap()
{
    if ( first && second )
        std::swap( first, second );
}

Battle::Cell * Battle::Position::GetHead()
{
    return first;
}

Battle::Cell * Battle::Position::GetTail()
{
    return second;
}

const Battle::Cell * Battle::Position::GetHead() const
{
    return first;
}

const Battle::Cell * Battle::Position::GetTail() const
{
    return second;
}

fheroes2::Rect Battle::Position::GetRect() const
{
    if ( first )
        return second ? getBoundaryRect( first->GetPos(), second->GetPos() ) : first->GetPos();

    return fheroes2::Rect();
}

Battle::Position Battle::Position::GetPosition( const Unit & unit, const int32_t dst )
{
    Position result;

    if ( unit.isWide() ) {
        auto checkCells = [&unit]( Cell * headCell, Cell * tailCell ) {
            Position res;

            if ( headCell == nullptr || ( !unit.GetPosition().contains( headCell->GetIndex() ) && !headCell->isPassable( true ) ) ) {
                return res;
            }

            if ( tailCell == nullptr || ( !unit.GetPosition().contains( tailCell->GetIndex() ) && !tailCell->isPassable( true ) ) ) {
                return res;
            }

            res.first = headCell;
            res.second = tailCell;

            return res;
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

Battle::Position Battle::Position::GetReachable( const Unit & currentUnit, const int32_t dst )
{
    const Arena * arena = GetArena();
    assert( arena != nullptr );

    if ( currentUnit.isWide() ) {
        auto checkCells = [arena]( Cell * headCell, Cell * tailCell ) -> Position {
            if ( headCell == nullptr || tailCell == nullptr ) {
                return {};
            }

            Position pos;

            pos.first = headCell;
            pos.second = tailCell;

            if ( arena->isPositionReachable( pos, true ) ) {
                return pos;
            }

            return {};
        };

        auto tryHead = [&currentUnit, dst, &checkCells]() -> Position {
            const int tailDirection = currentUnit.isReflect() ? RIGHT : LEFT;

            if ( Board::isValidDirection( dst, tailDirection ) ) {
                Cell * headCell = Board::GetCell( dst );
                Cell * tailCell = Board::GetCell( Board::GetIndexDirection( dst, tailDirection ) );

                return checkCells( headCell, tailCell );
            }

            return {};
        };

        auto tryTail = [&currentUnit, dst, &checkCells]() -> Position {
            const int headDirection = currentUnit.isReflect() ? LEFT : RIGHT;

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

    if ( arena->isPositionReachable( pos, true ) ) {
        return pos;
    }

    return {};
}

bool Battle::Position::isReflect() const
{
    return first && second && first->GetIndex() < second->GetIndex();
}

bool Battle::Position::contains( int cellIndex ) const
{
    return ( first && first->GetIndex() == cellIndex ) || ( second && second->GetIndex() == cellIndex );
}

Battle::Cell::Cell( int32_t ii )
    : index( ii )
    , object( 0 )
    , quality( 0 )
    , troop( nullptr )
{
    SetArea( fheroes2::Rect() );
}

void Battle::Cell::SetArea( const fheroes2::Rect & area )
{
    pos.x = area.x + 89 - ( ( ( index / ARENAW ) % 2 ) ? CELLW / 2 : 0 ) + CELLW * ( index % ARENAW );
    pos.y = area.y + 62 + ( ( CELLH - ( CELLH - cellHeightVerSide ) / 2 ) * ( index / ARENAW ) );
    pos.width = CELLW;
    pos.height = CELLH;

    // center
    coord[0] = { infl * pos.x + infl * pos.width / 2, infl * pos.y + infl * pos.height / 2 };
    // coordinates
    coord[1] = { infl * pos.x, infl * pos.y + infl * ( pos.height - cellHeightVerSide ) / 2 };
    coord[2] = { infl * pos.x + infl * pos.width / 2, infl * pos.y };
    coord[3] = { infl * pos.x + infl * pos.width, infl * pos.y + infl * ( pos.height - cellHeightVerSide ) / 2 };
    coord[4] = { infl * pos.x + infl * pos.width, infl * pos.y + infl * pos.height - infl * ( pos.height - cellHeightVerSide ) / 2 };
    coord[5] = { infl * pos.x + infl * pos.width / 2, infl * pos.y + infl * pos.height };
    coord[6] = { infl * pos.x, infl * pos.y + infl * pos.height - infl * ( pos.height - cellHeightVerSide ) / 2 };
}

Battle::direction_t Battle::Cell::GetTriangleDirection( const fheroes2::Point & dst ) const
{
    const fheroes2::Point pt( infl * dst.x, infl * dst.y );

    if ( pt == coord[0] )
        return CENTER;
    else if ( inABC( pt, coord[0], coord[1], coord[2] ) )
        return TOP_LEFT;
    else if ( inABC( pt, coord[0], coord[2], coord[3] ) )
        return TOP_RIGHT;
    else if ( inABC( pt, coord[0], coord[3], coord[4] ) )
        return RIGHT;
    else if ( inABC( pt, coord[0], coord[4], coord[5] ) )
        return BOTTOM_RIGHT;
    else if ( inABC( pt, coord[0], coord[5], coord[6] ) )
        return BOTTOM_LEFT;
    else if ( inABC( pt, coord[0], coord[1], coord[6] ) )
        return LEFT;

    return UNKNOWN;
}

bool Battle::Cell::isPositionIncludePoint( const fheroes2::Point & pt ) const
{
    return UNKNOWN != GetTriangleDirection( pt );
}

int32_t Battle::Cell::GetIndex() const
{
    return index;
}

int32_t Battle::Cell::GetQuality() const
{
    return quality;
}

void Battle::Cell::SetObject( int val )
{
    object = val;
}

void Battle::Cell::SetQuality( uint32_t val )
{
    quality = val;
}

int Battle::Cell::GetObject() const
{
    return object;
}

const fheroes2::Rect & Battle::Cell::GetPos() const
{
    return pos;
}

const Battle::Unit * Battle::Cell::GetUnit() const
{
    return troop;
}

Battle::Unit * Battle::Cell::GetUnit()
{
    return troop;
}

void Battle::Cell::SetUnit( Unit * val )
{
    troop = val;
}

bool Battle::Cell::isPassableFromAdjacent( const Unit & unit, const Cell & adjacent ) const
{
    assert( Board::isNearIndexes( index, adjacent.GetIndex() ) );

    if ( unit.isWide() ) {
        const int dir = Board::GetDirection( adjacent.index, index );

        switch ( dir ) {
        case BOTTOM_RIGHT:
        case TOP_RIGHT:
        case BOTTOM_LEFT:
        case TOP_LEFT: {
            const bool reflect = ( ( BOTTOM_LEFT | TOP_LEFT ) & dir ) != 0;
            const Cell * tail = Board::GetCell( index, reflect ? RIGHT : LEFT );

            return tail && tail->isPassable( true ) && isPassable( true );
        }

        case LEFT:
        case RIGHT:
            return isPassable( true ) || index == unit.GetTailIndex();

        default:
            break;
        }
    }

    return isPassable( true );
}

bool Battle::Cell::isPassableForUnit( const Unit & unit ) const
{
    const Position unitPos = Position::GetPosition( unit, index );

    return unitPos.GetHead() != nullptr && ( !unit.isWide() || unitPos.GetTail() != nullptr );
}

bool Battle::Cell::isPassable( const bool checkForUnit ) const
{
    return object == 0 && ( !checkForUnit || troop == nullptr );
}

void Battle::Cell::ResetQuality()
{
    quality = 0;
}
