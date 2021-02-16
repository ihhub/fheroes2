/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <iostream>
#include <sstream>

#include "game.h"
#include "heroes.h"
#include "maps.h"
#include "route.h"
#include "settings.h"
#include "world.h"

s32 Route::Step::GetIndex( void ) const
{
    return currentIndex;
}

s32 Route::Step::GetFrom( void ) const
{
    return from;
}

int Route::Step::GetDirection( void ) const
{
    return direction;
}

u32 Route::Step::GetPenalty( void ) const
{
    return penalty;
}

bool Route::Step::isBad( void ) const
{
    return from < 0 || ( direction == Direction::UNKNOWN || direction == Direction::CENTER );
}

/* construct */
Route::Path::Path( const Heroes & h )
    : hero( &h )
    , dst( h.GetIndex() )
    , hide( true )
{}

Route::Path::Path( const Path & p )
    : std::list<Step>( p )
    , hero( p.hero )
    , dst( p.dst )
    , hide( p.hide )
{}

Route::Path & Route::Path::operator=( const Path & p )
{
    assign( p.begin(), p.end() );

    hero = p.hero;
    dst = p.dst;
    hide = p.hide;

    return *this;
}

int Route::Path::GetFrontDirection( void ) const
{
    return empty() ? ( dst != hero->GetIndex() ? Maps::GetDirection( hero->GetIndex(), dst ) : Direction::CENTER ) : front().GetDirection();
}

u32 Route::Path::GetFrontPenalty( void ) const
{
    return empty() ? 0 : front().GetPenalty();
}

void Route::Path::PopFront( void )
{
    if ( !empty() )
        pop_front();
}

void Route::Path::PopBack( void )
{
    if ( !empty() ) {
        pop_back();
        dst = empty() ? -1 : back().GetIndex();
    }
}

s32 Route::Path::GetDestinationIndex( void ) const
{
    return empty() ? GetDestinedIndex() : GetLastIndex();
}

s32 Route::Path::GetLastIndex( void ) const
{
    return empty() ? -1 : back().GetIndex();
}

s32 Route::Path::GetDestinedIndex( void ) const
{
    return dst;
}

void Route::Path::setPath( const std::list<Route::Step> & path, int32_t destIndex )
{
    assign( path.begin(), path.end() );
    dst = destIndex;
}

void Route::Path::Reset( void )
{
    dst = hero->GetIndex();

    if ( !empty() ) {
        clear();
        hide = true;
    }
}

bool Route::Path::isComplete( void ) const
{
    return dst == hero->GetIndex() || ( empty() && Direction::UNKNOWN != Maps::GetDirection( hero->GetIndex(), dst ) );
}

bool Route::Path::isValid( void ) const
{
    return !empty() && front().GetDirection() != Direction::UNKNOWN;
}

int Route::Path::GetIndexSprite( int from, int to, int mod )
{
    // ICN::ROUTE
    // start index 1, 25, 49, 73, 97, 121 (path arrow size)
    int index = 1;

    switch ( mod ) {
    case 200:
        index = 121;
        break;
    case 175:
        index = 97;
        break;
    case 150:
        index = 73;
        break;
    case 125:
        index = 49;
        break;
    case 100:
        index = 25;
        break;

    default:
        break;
    }

    switch ( from ) {
    case Direction::TOP:
        switch ( to ) {
        case Direction::TOP:
            index += 8;
            break;
        case Direction::TOP_RIGHT:
            index += 17;
            break;
        case Direction::RIGHT:
            index += 18;
            break;
        case Direction::LEFT:
            index += 6;
            break;
        case Direction::TOP_LEFT:
            index += 7;
            break;
        case Direction::BOTTOM_LEFT:
            index += 5;
            break;
        case Direction::BOTTOM_RIGHT:
            index += 19;
            break;
        default:
            index = 0;
            break;
        }
        break;

    case Direction::TOP_RIGHT:
        switch ( to ) {
        case Direction::TOP:
            index += 0;
            break;
        case Direction::TOP_RIGHT:
            index += 9;
            break;
        case Direction::RIGHT:
            index += 18;
            break;
        case Direction::BOTTOM_RIGHT:
            index += 19;
            break;
        case Direction::TOP_LEFT:
            index += 7;
            break;
        case Direction::BOTTOM:
            index += 20;
            break;
        case Direction::LEFT:
            index += 6;
            break;
        default:
            index = 0;
            break;
        }
        break;

    case Direction::RIGHT:
        switch ( to ) {
        case Direction::TOP:
            index += 0;
            break;
        case Direction::BOTTOM:
            index += 20;
            break;
        case Direction::BOTTOM_RIGHT:
            index += 19;
            break;
        case Direction::RIGHT:
            index += 10;
            break;
        case Direction::TOP_RIGHT:
            index += 1;
            break;
        case Direction::TOP_LEFT:
            index += 7;
            break;
        case Direction::BOTTOM_LEFT:
            index += 21;
            break;
        default:
            index = 0;
            break;
        }
        break;

    case Direction::BOTTOM_RIGHT:
        switch ( to ) {
        case Direction::TOP_RIGHT:
            index += 1;
            break;
        case Direction::RIGHT:
            index += 2;
            break;
        case Direction::BOTTOM_RIGHT:
            index += 11;
            break;
        case Direction::BOTTOM:
            index += 20;
            break;
        case Direction::BOTTOM_LEFT:
            index += 21;
            break;
        case Direction::TOP:
            index += 0;
            break;
        case Direction::LEFT:
            index += 22;
            break;
        default:
            index = 0;
            break;
        }
        break;

    case Direction::BOTTOM:
        switch ( to ) {
        case Direction::RIGHT:
            index += 2;
            break;
        case Direction::BOTTOM_RIGHT:
            index += 3;
            break;
        case Direction::BOTTOM:
            index += 12;
            break;
        case Direction::BOTTOM_LEFT:
            index += 21;
            break;
        case Direction::LEFT:
            index += 22;
            break;
        case Direction::TOP_LEFT:
            index += 16;
            break;
        case Direction::TOP_RIGHT:
            index += 1;
            break;
        default:
            index = 0;
            break;
        }
        break;

    case Direction::BOTTOM_LEFT:
        switch ( to ) {
        case Direction::BOTTOM_RIGHT:
            index += 3;
            break;
        case Direction::BOTTOM:
            index += 4;
            break;
        case Direction::BOTTOM_LEFT:
            index += 13;
            break;
        case Direction::LEFT:
            index += 22;
            break;
        case Direction::TOP_LEFT:
            index += 23;
            break;
        case Direction::TOP:
            index += 16;
            break;
        case Direction::RIGHT:
            index += 2;
            break;
        default:
            index = 0;
            break;
        }
        break;

    case Direction::LEFT:
        switch ( to ) {
        case Direction::TOP:
            index += 16;
            break;
        case Direction::BOTTOM:
            index += 4;
            break;
        case Direction::BOTTOM_LEFT:
            index += 5;
            break;
        case Direction::LEFT:
            index += 14;
            break;
        case Direction::TOP_LEFT:
            index += 23;
            break;
        case Direction::TOP_RIGHT:
            index += 17;
            break;
        case Direction::BOTTOM_RIGHT:
            index += 3;
            break;
        default:
            index = 0;
            break;
        }
        break;

    case Direction::TOP_LEFT:
        switch ( to ) {
        case Direction::TOP:
            index += 16;
            break;
        case Direction::TOP_RIGHT:
            index += 17;
            break;
        case Direction::BOTTOM_LEFT:
            index += 5;
            break;
        case Direction::LEFT:
            index += 6;
            break;
        case Direction::TOP_LEFT:
            index += 15;
            break;
        case Direction::BOTTOM:
            index += 4;
            break;
        case Direction::RIGHT:
            index += 18;
            break;
        default:
            index = 0;
            break;
        }
        break;

    default:
        index = 0;
        break;
    }

    return index;
}

/* total penalty cast */
u32 Route::Path::GetTotalPenalty( void ) const
{
    u32 result = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        result += ( *it ).GetPenalty();

    return result;
}

uint32_t Route::Path::getLastMovePenalty() const
{
    const Route::Step & firstStep = front();
    const uint32_t penalty = firstStep.GetPenalty();
    return Direction::isDiagonal( firstStep.GetDirection() ) ? ( penalty * 2 / 3 ) : penalty;
}

int Route::Path::GetAllowedSteps( void ) const
{
    int green = 0;
    uint32_t movePoints = hero->GetMovePoints();

    for ( const_iterator it = begin(); it != end() && movePoints > 0; ++it ) {
        uint32_t penalty = it->GetPenalty();

        // allow diagonal move at a lower cost if it's a last one
        if ( movePoints < penalty && Direction::isDiagonal( it->GetDirection() ) ) {
            penalty = penalty * 2 / 3;
        }

        if ( movePoints >= penalty ) {
            movePoints -= penalty;
            ++green;
        }
        else {
            movePoints = 0;
        }
    }

    return green;
}

std::string Route::Path::String( void ) const
{
    std::ostringstream os;

    os << "from: " << hero->GetIndex() << ", to: " << GetLastIndex() << ", obj: " << MP2::StringObject( world.GetTiles( dst ).GetObject() ) << ", dump: ";

    for ( const_iterator it = begin(); it != end(); ++it )
        os << Direction::String( ( *it ).GetDirection() ) << "(" << ( *it ).GetPenalty() << ")"
           << ", ";

    os << "end";
    return os.str();
}

bool StepIsObstacle( const Route::Step & s )
{
    s32 index = s.GetIndex();
    int obj = 0 <= index ? world.GetTiles( index ).GetObject() : MP2::OBJ_ZERO;

    switch ( obj ) {
    case MP2::OBJ_HEROES:
    case MP2::OBJ_MONSTER:
        return true;

    default:
        break;
    }

    return false;
}

bool Route::Path::hasObstacle( void ) const
{
    const_iterator it = std::find_if( begin(), end(), StepIsObstacle );
    return it != end() && ( *it ).GetIndex() != GetLastIndex();
}

void Route::Path::RescanObstacle( void )
{
    // scan obstacle
    iterator it = std::find_if( begin(), end(), StepIsObstacle );

    if ( it != end() && ( *it ).GetIndex() != GetLastIndex() ) {
        size_t size1 = size();
        s32 reduce = ( *it ).GetFrom();

        std::list<Step> path = world.getPath( *hero, dst );
        const bool reducePath = path.size() > size1 * 2;
        // reduce
        if ( reducePath )
            path = world.getPath( *hero, reduce );

        setPath( path, reducePath ? reduce : dst );
    }
}

void Route::Path::RescanPassable( void )
{
    // scan passable
    iterator it = begin();

    for ( ; it != end(); ++it ) {
        if ( !world.GetTiles( it->GetFrom() ).isPassable( it->GetDirection(), hero->isShipMaster(), false, hero->GetColor() ) ) {
            break;
        }
    }

    if ( hero->isControlAI() ) {
        Reset();
    }
    else if ( it != end() ) {
        if ( it == begin() )
            Reset();
        else {
            dst = it->GetFrom();
            erase( it, end() );
        }
    }
}

StreamBase & Route::operator<<( StreamBase & msg, const Step & step )
{
    return msg << step.from << step.direction << step.penalty;
}

StreamBase & Route::operator<<( StreamBase & msg, const Path & path )
{
    return msg << path.dst << path.hide << static_cast<std::list<Step> >( path );
}

StreamBase & Route::operator>>( StreamBase & msg, Step & step )
{
    msg >> step.from >> step.direction >> step.penalty;
    step.currentIndex = Maps::GetDirectionIndex( step.from, step.direction );
    return msg;
}

StreamBase & Route::operator>>( StreamBase & msg, Path & path )
{
    std::list<Step> & base = path;
    return msg >> path.dst >> path.hide >> base;
}
