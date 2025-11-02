/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#include "rand.h"

int Battle::Command::GetNextValue()
{
    int val = 0;

    *this >> val;

    return val;
}

uint64_t Battle::Command::updatePCG32Stream( uint64_t stream ) const
{
    switch ( _type ) {
    case CommandType::ATTACK:
        assert( size() == 5 );

        Rand::combineSeedWithValueHash( stream, _type );
        // Use only cell index to move and attacker & defender UIDs, because cell index to attack and attack direction may differ depending on whether the AI or the human
        // player gives the command
        Rand::combineSeedWithValueHash( stream, at( 2 ) );
        Rand::combineSeedWithValueHash( stream, at( 3 ) );
        Rand::combineSeedWithValueHash( stream, at( 4 ) );
        break;

    case CommandType::MOVE:
    case CommandType::SPELLCAST:
    case CommandType::MORALE:
    case CommandType::CATAPULT:
    case CommandType::TOWER:
    case CommandType::RETREAT:
    case CommandType::SURRENDER:
    case CommandType::SKIP:
        Rand::combineSeedWithValueHash( stream, _type );
        std::for_each( begin(), end(), [&stream]( const int param ) { Rand::combineSeedWithValueHash( stream, param ); } );
        break;

    // These commands should never affect the stream
    case CommandType::TOGGLE_AUTO_COMBAT:
    case CommandType::QUICK_COMBAT:
        break;

    default:
        assert( 0 );
        break;
    }

    return stream;
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
