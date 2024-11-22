/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <numeric>

#include "game_io.h"
#include "heroes.h"
#include "maps.h"
#include "maps_tiles.h"
#include "mp2.h"
#include "save_format_version.h"
#include "serialize.h"
#include "world.h"

Route::Path::Path( const Heroes & hero )
    : _hero( &hero )
    , _hide( true )
{}

bool Route::Path::isValidForMovement() const
{
    return !empty() && front().GetDirection() != Direction::UNKNOWN && Maps::isValidAbsIndex( front().GetIndex() );
}

bool Route::Path::isValidForTeleportation() const
{
    return !empty() && front().GetDirection() == Direction::UNKNOWN && Maps::isValidAbsIndex( front().GetIndex() );
}

int Route::Path::GetIndexSprite( const int from, const int to, const int cost )
{
    // ICN::ROUTE
    // start index 1, 25, 49, 73, 97, 121 (path arrow size)
    int index = 1;

    switch ( cost ) {
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

    const int32_t dstIndex = GetDestinationIndex();

    std::string output( "from: " );
    output += std::to_string( _hero->GetIndex() );
    output += ", to: ";
    output += std::to_string( dstIndex );
    output += ", obj: ";
    output += MP2::StringObject( Maps::isValidAbsIndex( dstIndex ) ? world.getTile( dstIndex ).getMainObjectType() : MP2::OBJ_NONE );
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

OStreamBase & Route::operator<<( OStreamBase & stream, const Step & step )
{
    return stream << step.from << step.direction << step.penalty;
}

OStreamBase & Route::operator<<( OStreamBase & stream, const Path & path )
{
    return stream << path._hide << static_cast<const std::list<Step> &>( path );
}

IStreamBase & Route::operator>>( IStreamBase & stream, Step & step )
{
    stream >> step.from >> step.direction >> step.penalty;
    step.currentIndex = Maps::GetDirectionIndex( step.from, step.direction );
    return stream;
}

IStreamBase & Route::operator>>( IStreamBase & stream, Path & path )
{
    std::list<Step> & base = path;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1007_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1007_RELEASE ) {
        int32_t dummy;

        stream >> dummy;
    }

    return stream >> path._hide >> base;
}

uint32_t Route::calculatePathPenalty( const std::list<Step> & path )
{
    return std::accumulate( path.begin(), path.end(), static_cast<uint32_t>( 0 ), []( const uint32_t total, const Step & step ) { return total + step.GetPenalty(); } );
}
