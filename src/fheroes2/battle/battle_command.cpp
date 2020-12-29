/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>

#include "battle_arena.h"
#include "battle_command.h"
#include "spell.h"

bool Battle::Actions::HaveCommand( u32 cmd ) const
{
    return end() != std::find_if( begin(), end(), [cmd]( const Battle::Command & v ) { return v.isType( cmd ); } );
}

Battle::Command::Command( int cmd )
    : type( cmd )
{}

Battle::Command & Battle::Command::operator<<( const int val )
{
    push_back( val );
    return *this;
}

Battle::Command & Battle::Command::operator>>( int & val )
{
    if ( size() ) {
        val = back();
        pop_back();
    }
    return *this;
}

int Battle::Command::GetValue( void )
{
    int val = 0;
    *this >> val;
    return val;
}

Battle::Command::Command( int cmd, int param1, int param2, const Indexes & param3 )
    : type( cmd )
{
    switch ( type ) {
    case MSG_BATTLE_MOVE:
        for ( Indexes::const_reverse_iterator it = param3.rbegin(); it != param3.rend(); ++it )
            *this << *it;
        *this << param3.size() << param2 << param1; // path, dst, uid
        break;

    default:
        break;
    }
}

Battle::Command::Command( int cmd, int param1, int param2, int param3, int param4 )
    : type( cmd )
{
    switch ( type ) {
    case MSG_BATTLE_AUTO:
        *this << param1; // color
        break;

    case MSG_BATTLE_SURRENDER:
    case MSG_BATTLE_RETREAT:
        break;

    case MSG_BATTLE_TOWER:
        *this << param2 << param1; // enemy uid, type
        break;

    case MSG_BATTLE_CATAPULT: // battle_arena.cpp
        break;

    case MSG_BATTLE_CAST:
        switch ( param1 ) {
        case Spell::MIRRORIMAGE:
            *this << param2 << param1; // who, spell
            break;

        case Spell::TELEPORT:
            *this << param3 << param2 << param1; // dst, src, spell
            break;

        default:
            *this << param2 << param1; // dst, spell
            break;
        }
        break;

    case MSG_BATTLE_END_TURN:
        *this << param1; // uid
        break;

    case MSG_BATTLE_SKIP:
        *this << param2 << param1; // hard_skip, uid
        break;

    case MSG_BATTLE_MOVE:
        *this << static_cast<int>( 0 ) << param2 << param1; // path size, dst, uid
        break;

    case MSG_BATTLE_ATTACK:
        *this << param4 << param3 << param2 << param1; // direction, dst, uid, uid
        break;

    case MSG_BATTLE_MORALE:
        *this << param2 << param1; // state, uid
        break;

    default:
        break;
    }
}
