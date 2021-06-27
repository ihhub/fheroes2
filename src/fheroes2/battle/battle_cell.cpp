/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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
#include "battle_troop.h"
#include "logging.h"
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

void Battle::Position::Set( s32 head, bool wide, bool reflect )
{
    first = Board::GetCell( head );

    if ( first && wide )
        second = Board::GetCell( first->GetIndex(), reflect ? RIGHT : LEFT );
}

void Battle::Position::Swap( void )
{
    if ( first && second )
        std::swap( first, second );
}

Battle::Cell * Battle::Position::GetHead( void )
{
    return first;
}

Battle::Cell * Battle::Position::GetTail( void )
{
    return second;
}

const Battle::Cell * Battle::Position::GetHead( void ) const
{
    return first;
}

const Battle::Cell * Battle::Position::GetTail( void ) const
{
    return second;
}

fheroes2::Rect Battle::Position::GetRect( void ) const
{
    if ( first )
        return second ? GetCommonRect( first->GetPos(), second->GetPos(), false ) : first->GetPos();

    return fheroes2::Rect();
}

Battle::Position Battle::Position::GetCorrect( const Unit & b, s32 head )
{
    Position result;

    result.first = Board::GetCell( head );

    if ( result.first && b.isWide() ) {
        result.second = Board::GetCell( head, b.isReflect() ? RIGHT : LEFT );

        if ( !result.second || ( result.second != b.GetPosition().GetHead() && !result.second->isPassable1( true ) ) ) {
            result.second = Board::GetCell( head, b.isReflect() ? LEFT : RIGHT );

            if ( !result.second )
                result.second = Board::GetCell( head, b.isReflect() ? RIGHT : LEFT );

            if ( result.second ) {
                std::swap( result.first, result.second );
            }
            else {
                DEBUG_LOG( DBG_BATTLE, DBG_WARN, "nullptr pointer, " << b.String() << ", dst: " << head );
            }
        }
    }

    return result;
}

bool Battle::Position::isReflect( void ) const
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
    , direction( UNKNOWN )
    , quality( 0 )
    , troop( nullptr )
{
    SetArea( fheroes2::Rect() );
}

void Battle::Cell::SetArea( const fheroes2::Rect & area )
{
    pos.x = area.x + 89 - ( ( ( index / ARENAW ) % 2 ) ? CELLW / 2 : 0 ) + ( CELLW ) * ( index % ARENAW );
    pos.y = area.y + 62 + ( ( CELLH - ( CELLH - cellHeightVerSide ) / 2 ) * ( index / ARENAW ) );
    pos.width = CELLW;
    pos.height = CELLH;

    // center
    coord[0] = fheroes2::Point( infl * pos.x + infl * pos.width / 2, infl * pos.y + infl * pos.height / 2 );
    // coordinates
    coord[1] = fheroes2::Point( infl * pos.x, infl * pos.y + infl * ( pos.height - cellHeightVerSide ) / 2 );
    coord[2] = fheroes2::Point( infl * pos.x + infl * pos.width / 2, infl * pos.y );
    coord[3] = fheroes2::Point( infl * pos.x + infl * pos.width, infl * pos.y + infl * ( pos.height - cellHeightVerSide ) / 2 );
    coord[4] = fheroes2::Point( infl * pos.x + infl * pos.width, infl * pos.y + infl * pos.height - infl * ( pos.height - cellHeightVerSide ) / 2 );
    coord[5] = fheroes2::Point( infl * pos.x + infl * pos.width / 2, infl * pos.y + infl * pos.height );
    coord[6] = fheroes2::Point( infl * pos.x, infl * pos.y + infl * pos.height - infl * ( pos.height - cellHeightVerSide ) / 2 );
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

s32 Battle::Cell::GetIndex( void ) const
{
    return index;
}

s32 Battle::Cell::GetQuality( void ) const
{
    return quality;
}

void Battle::Cell::SetObject( int val )
{
    object = val;
}

void Battle::Cell::SetDirection( int val )
{
    direction = val;
}

void Battle::Cell::SetQuality( u32 val )
{
    quality = val;
}

int Battle::Cell::GetObject( void ) const
{
    return object;
}

int Battle::Cell::GetDirection( void ) const
{
    return direction;
}

const fheroes2::Rect & Battle::Cell::GetPos( void ) const
{
    return pos;
}

const Battle::Unit * Battle::Cell::GetUnit( void ) const
{
    return troop;
}

Battle::Unit * Battle::Cell::GetUnit( void )
{
    return troop;
}

void Battle::Cell::SetUnit( Unit * val )
{
    troop = val;
}

bool Battle::Cell::isPassable4( const Unit & b, const Cell & from ) const
{
    if ( b.isWide() ) {
        const int dir = Board::GetDirection( from.index, index );

        switch ( dir ) {
        case BOTTOM_RIGHT:
        case TOP_RIGHT:
        case BOTTOM_LEFT:
        case TOP_LEFT: {
            const bool reflect = ( ( BOTTOM_LEFT | TOP_LEFT ) & dir ) != 0;
            const Cell * tail = Board::GetCell( index, reflect ? RIGHT : LEFT );
            return tail && tail->isPassable1( true ) && isPassable1( true );
        }

        case LEFT:
        case RIGHT:
            return isPassable1( true ) || index == b.GetTailIndex();

        default:
            break;
        }
    }

    return isPassable1( true );
}

bool Battle::Cell::isPassable3( const Unit & b, bool check_reflect ) const
{
    if ( index == b.GetHeadIndex() || index == b.GetTailIndex() )
        return true;

    if ( b.isWide() ) {
        if ( check_reflect ) {
            const Cell * cell = Board::GetCell( index, b.isReflect() ? RIGHT : LEFT );
            return cell && ( cell->isPassable1( true ) || cell->index == b.GetTailIndex() || cell->index == b.GetHeadIndex() ) && isPassable1( true );
        }
        else {
            const Cell * left = Board::GetCell( index, LEFT );
            const Cell * right = Board::GetCell( index, RIGHT );
            return ( ( left && ( left->isPassable1( true ) || left->index == b.GetTailIndex() || left->index == b.GetHeadIndex() ) )
                     || ( right && ( right->isPassable1( true ) || right->index == b.GetTailIndex() || right->index == b.GetHeadIndex() ) ) )
                   && isPassable1( true );
        }
    }

    return isPassable1( true );
}

bool Battle::Cell::isPassable1( bool check_troop ) const
{
    return 0 == object && ( !check_troop || nullptr == troop );
}

void Battle::Cell::ResetQuality( void )
{
    quality = 0;
}

void Battle::Cell::ResetDirection( void )
{
    direction = UNKNOWN;
}
