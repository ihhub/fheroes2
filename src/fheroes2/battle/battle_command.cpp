/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
#include "spell.h"

Battle::Command::Command( const CommandType cmd )
    : _type( cmd )
{}

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

int Battle::Command::GetValue()
{
    int val = 0;
    *this >> val;
    return val;
}

Battle::Command::Command( const CommandType cmd, const int param1, const int param2 /* = -1 */, const int param3 /* = -1 */, const int param4 /* = -1 */ )
    : _type( cmd )
{
    switch ( _type ) {
    case CommandType::MSG_BATTLE_AUTO_SWITCH:
        *this << param1; // color
        break;

    case CommandType::MSG_BATTLE_SURRENDER:
    case CommandType::MSG_BATTLE_RETREAT:
    case CommandType::MSG_BATTLE_AUTO_FINISH:
        break;

    case CommandType::MSG_BATTLE_TOWER:
        *this << param2 << param1; // enemy uid, type
        break;

    case CommandType::MSG_BATTLE_CATAPULT: // battle_arena.cpp
        break;

    case CommandType::MSG_BATTLE_CAST:
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

    case CommandType::MSG_BATTLE_END_TURN:
        *this << param1; // uid
        break;

    case CommandType::MSG_BATTLE_SKIP:
        *this << param1; // uid
        break;

    case CommandType::MSG_BATTLE_MOVE:
        *this << param2 << param1; // dst, uid
        break;

    case CommandType::MSG_BATTLE_ATTACK:
        *this << param4 << param3 << param2 << param1; // direction, dst, uid, uid
        break;

    case CommandType::MSG_BATTLE_MORALE:
        *this << param2 << param1; // state, uid
        break;

    default:
        break;
    }
}
