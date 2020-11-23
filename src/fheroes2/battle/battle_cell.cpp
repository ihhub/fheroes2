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
#include "army_troop.h"
#include "battle_board.h"
#include "battle_troop.h"
#include "settings.h"

#define INFL 12

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

Rect Battle::Position::GetRect( void ) const
{
    if ( first )
        return second ? Rect::Get( first->GetPos(), second->GetPos(), false ) : first->GetPos();

    return Rect();
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
                DEBUG( DBG_BATTLE, DBG_WARN, "NULL pointer, " << b.String() << ", dst: " << head );
            }
        }
    }

    return result;
}

bool Battle::Position::isReflect( void ) const
{
    return first && second && first->GetIndex() < second->GetIndex();
}

bool Battle::Position::isValid( void ) const
{
    return first && ( !second || ( ( LEFT | RIGHT ) & Board::GetDirection( first->GetIndex(), second->GetIndex() ) ) );
}

bool Battle::Position::contains( int cellIndex ) const
{
    return ( first && first->GetIndex() == cellIndex ) || ( second && second->GetIndex() == cellIndex );
}

Battle::Cell::Cell()
    : index( 0 )
    , object( 0 )
    , direction( UNKNOWN )
    , quality( 0 )
    , troop( NULL )
{}

Battle::Cell::Cell( s32 ii )
    : index( ii )
    , object( 0 )
    , direction( UNKNOWN )
    , quality( 0 )
    , troop( NULL )
{
    SetArea( Rect() );
}

void Battle::Cell::SetArea( const Rect & area )
{
    pos.x = area.x + 89 - ( ( ( index / ARENAW ) % 2 ) ? CELLW / 2 : 0 ) + ( CELLW ) * ( index % ARENAW );
    pos.y = area.y + 62 + ( ( ( CELLH - ( CELLH - CELLH_VER_SIDE ) / 2 ) ) * ( index / ARENAW ) );
    pos.w = CELLW;
    pos.h = CELLH;

    const short vertical_side_size = CELLH_VER_SIDE;
    // center
    coord[0] = Point( INFL * pos.x + INFL * pos.w / 2, INFL * pos.y + INFL * pos.h / 2 );
    // coordinates
    coord[1] = Point( INFL * pos.x, INFL * pos.y + INFL * ( pos.h - vertical_side_size ) / 2 );
    coord[2] = Point( INFL * pos.x + INFL * pos.w / 2, INFL * pos.y );
    coord[3] = Point( INFL * pos.x + INFL * pos.w, INFL * pos.y + INFL * ( pos.h - vertical_side_size ) / 2 );
    coord[4] = Point( INFL * pos.x + INFL * pos.w, INFL * pos.y + INFL * pos.h - INFL * ( pos.h - vertical_side_size ) / 2 );
    coord[5] = Point( INFL * pos.x + INFL * pos.w / 2, INFL * pos.y + INFL * pos.h );
    coord[6] = Point( INFL * pos.x, INFL * pos.y + INFL * pos.h - INFL * ( pos.h - vertical_side_size ) / 2 );
}

Battle::direction_t Battle::Cell::GetTriangleDirection( const Point & dst ) const
{
    const Point pt( INFL * dst.x, INFL * dst.y );

    if ( pt == coord[0] )
        return CENTER;
    else if ( pt.inABC( coord[0], coord[1], coord[2] ) )
        return TOP_LEFT;
    else if ( pt.inABC( coord[0], coord[2], coord[3] ) )
        return TOP_RIGHT;
    else if ( pt.inABC( coord[0], coord[3], coord[4] ) )
        return RIGHT;
    else if ( pt.inABC( coord[0], coord[4], coord[5] ) )
        return BOTTOM_RIGHT;
    else if ( pt.inABC( coord[0], coord[5], coord[6] ) )
        return BOTTOM_LEFT;
    else if ( pt.inABC( coord[0], coord[1], coord[6] ) )
        return LEFT;

    return UNKNOWN;
}

bool Battle::Cell::isPositionIncludePoint( const Point & pt ) const
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

const Rect & Battle::Cell::GetPos( void ) const
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
            Cell * left = Board::GetCell( index, LEFT );
            Cell * right = Board::GetCell( index, RIGHT );
            return ( ( left && ( left->isPassable1( true ) || left->index == b.GetTailIndex() || left->index == b.GetHeadIndex() ) )
                     || ( right && ( right->isPassable1( true ) || right->index == b.GetTailIndex() || right->index == b.GetHeadIndex() ) ) )
                   && isPassable1( true );
        }
    }

    return isPassable1( true );
}

bool Battle::Cell::isPassable1( bool check_troop ) const
{
    return 0 == object && ( !check_troop || NULL == troop );
}

void Battle::Cell::ResetQuality( void )
{
    quality = 0;
}

void Battle::Cell::ResetDirection( void )
{
    direction = UNKNOWN;
}

StreamBase & Battle::operator<<( StreamBase & msg, const Cell & c )
{
    return msg << c.index << c.object << c.direction << c.quality << ( c.troop ? c.troop->GetUID() : static_cast<u32>( 0 ) );
}

StreamBase & Battle::operator>>( StreamBase & msg, Cell & c )
{
    u32 uid = 0;
    msg >> c.index >> c.object >> c.direction >> c.quality >> uid;
    c.troop = GetArena()->GetTroopUID( uid );
    return msg;
}
