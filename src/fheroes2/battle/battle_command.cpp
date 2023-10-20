/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "battle_command.h"

#include <algorithm>

#include "tools.h"

int Battle::Command::GetNextValue()
{
    int val = 0;

    *this >> val;

    return val;
}

uint32_t Battle::Command::updateSeed( uint32_t seed ) const
{
    switch ( _type ) {
    case CommandType::ATTACK:
        assert( size() == 4 );

        fheroes2::hashCombine( seed, _type );
        // Use only attacker & defender UIDs, because cell index and direction may differ depending on whether the AI or the human player gives the command
        fheroes2::hashCombine( seed, at( 2 ) );
        fheroes2::hashCombine( seed, at( 3 ) );
        break;

    case CommandType::MOVE:
    case CommandType::SPELLCAST:
    case CommandType::MORALE:
    case CommandType::CATAPULT:
    case CommandType::TOWER:
    case CommandType::RETREAT:
    case CommandType::SURRENDER:
    case CommandType::SKIP:
        fheroes2::hashCombine( seed, _type );
        std::for_each( begin(), end(), [&seed]( const int param ) { fheroes2::hashCombine( seed, param ); } );
        break;

    // Ignore the end turn command, because the AI and the human player give them differently
    // TODO: eventually get rid of this command
    case CommandType::END_TURN:
    // These commands should never affect the seed generation
    case CommandType::AUTO_SWITCH:
    case CommandType::AUTO_FINISH:
        break;

    default:
        assert( 0 );
        break;
    }

    return seed;
}

Battle::Command & Battle::Command::operator<<( const int val )
{
    push_back( val );

    return *this;
}

Battle::Command & Battle::Command::operator>>( int & val )
{
    if ( !empty() ) {
        val = back();
        pop_back();
    }

    return *this;
}
