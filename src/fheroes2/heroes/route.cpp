/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "route.h"

#include <cassert>
#include <memory>

#include "heroes.h"
#include "maps.h"
#include "maps_tiles.h"
#include "mp2.h"
#include "serialize.h"
#include "world.h"

Route::Path::Path( const Heroes & hero )
    : _hero( &hero )
    , _dst( -1 )
    , _hide( true )
{}

int32_t Route::Path::GetFrontFrom() const
{
    assert( _hero != nullptr );

    return empty() ? _hero->GetIndex() : front().GetFrom();
}

int Route::Path::GetFrontDirection() const
{
    if ( empty() ) {
        if ( Maps::isValidAbsIndex( _dst ) ) {
            assert( _hero != nullptr );

            return Maps::GetDirection( _hero->GetIndex(), _dst );
        }

        return Direction::UNKNOWN;
    }

    return front().GetDirection();
}

uint32_t Route::Path::GetFrontPenalty() const
{
    return empty() ? 0 : front().GetPenalty();
}

int Route::Path::GetNextStepDirection() const
{
    if ( size() < 2 ) {
        return Direction::UNKNOWN;
    }

    auto iter = cbegin();

    return ( ++iter )->GetDirection();
}

void Route::Path::PopFront()
{
    if ( empty() ) {
        return;
    }

    pop_front();
}

int32_t Route::Path::GetDestinationIndex( const bool returnLastStep /* = false */ ) const
{
    if ( returnLastStep ) {
        return empty() ? _dst : back().GetIndex();
    }

    return _dst;
}

void Route::Path::setPath( const std::list<Route::Step> & path, int32_t destIndex )
{
    assign( path.begin(), path.end() );

    _dst = destIndex;
}

void Route::Path::Reset()
{
    _dst = -1;

    if ( empty() ) {
        return;
    }

    _hide = true;

    clear();
}

bool Route::Path::isValidForMovement() const
{
    return !empty() && front().GetDirection() != Direction::UNKNOWN && Maps::isValidAbsIndex( front().GetIndex() );
}

bool Route::Path::isValidForTeleportation() const
{
    return !empty() && front().GetDirection() == Direction::UNKNOWN && Maps::isValidAbsIndex( front().GetIndex() );
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

bool Route::Path::hasAllowedSteps() const
{
    if ( !isValidForMovement() ) {
        return false;
    }

    assert( _hero != nullptr );

    return _hero->GetMovePoints() >= front().GetPenalty();
}

int Route::Path::GetAllowedSteps() const
{
    assert( _hero != nullptr );

    int green = 0;
    uint32_t movePoints = _hero->GetMovePoints();

    for ( const_iterator it = begin(); it != end() && movePoints > 0; ++it ) {
        uint32_t penalty = it->GetPenalty();

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

std::string Route::Path::String() const
{
    assert( _hero != nullptr );

    std::string output( "from: " );
    output += std::to_string( _hero->GetIndex() );
    output += ", to: ";
    output += std::to_string( _dst );
    output += ", obj: ";
    output += MP2::StringObject( Maps::isValidAbsIndex( _dst ) ? world.GetTiles( _dst ).GetObject() : MP2::OBJ_NONE );
    output += ", dump: ";

    for ( const Step & step : *this ) {
        output += Direction::String( step.GetDirection() );
        output += '(';
        output += std::to_string( step.GetPenalty() );
        output += ')';
    }

    output += "end";

    return output;
}

StreamBase & Route::operator<<( StreamBase & msg, const Step & step )
{
    return msg << step.from << step.direction << step.penalty;
}

StreamBase & Route::operator<<( StreamBase & msg, const Path & path )
{
    return msg << path._dst << path._hide << static_cast<const std::list<Step> &>( path );
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
    return msg >> path._dst >> path._hide >> base;
}
