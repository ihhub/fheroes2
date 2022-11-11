/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#include <algorithm>
#include <cassert>
#include <vector>

#include "army_troop.h"
#include "battle.h"
#include "battle_arena.h"
#include "battle_board.h"
#include "battle_cell.h"
#include "battle_tower.h"
#include "castle.h"
#include "monster.h"
#include "rand.h"
#include "tools.h"
#include "translations.h"

Battle::Tower::Tower( const Castle & castle, int twr, const Rand::DeterministicRandomGenerator & randomGenerator, const uint32_t uid )
    : Unit( Troop( Monster::ARCHER, 0 ), {}, false, randomGenerator, uid )
    , type( twr )
    , color( castle.GetColor() )
    , bonus( 0 )
    , valid( true )
{
    count += castle.CountBuildings();

    if ( count > 20 )
        count = 20;
    if ( TWR_CENTER != type )
        count /= 2;
    if ( count == 0 )
        count = 1;
    bonus = castle.GetLevelMageGuild();

    SetModes( CAP_TOWER );
}

const char * Battle::Tower::GetName() const
{
    switch ( type ) {
    case TWR_LEFT:
        return _( "Left Turret" );
    case TWR_RIGHT:
        return _( "Right Turret" );
    case TWR_CENTER:
        return _( "Ballista" );
    default:
        // This is not a valid Tower type!
        assert( 0 );
        break;
    }

    return nullptr;
}

bool Battle::Tower::isValid() const
{
    return valid;
}

uint32_t Battle::Tower::GetType() const
{
    return type;
}

uint32_t Battle::Tower::GetBonus() const
{
    return bonus;
}

uint32_t Battle::Tower::GetAttack() const
{
    return Unit::GetAttack() + bonus;
}

int Battle::Tower::GetColor() const
{
    return color;
}

fheroes2::Point Battle::Tower::GetPortPosition() const
{
    switch ( type ) {
    case TWR_LEFT:
        return { 410, 70 };
        break;
    case TWR_RIGHT:
        return { 410, 320 };
    case TWR_CENTER:
        return { 560, 170 };
    default:
        break;
    }

    return {};
}

void Battle::Tower::SetDestroy()
{
    switch ( type ) {
    case TWR_LEFT:
        Board::GetCell( Arena::CASTLE_TOP_ARCHER_TOWER_POS )->SetObject( 1 );
        break;
    case TWR_RIGHT:
        Board::GetCell( Arena::CASTLE_BOTTOM_ARCHER_TOWER_POS )->SetObject( 1 );
        break;
    default:
        break;
    }
    valid = false;
}

std::string Battle::Tower::GetInfo( const Castle & cstl )
{
    if ( !cstl.isBuild( BUILD_CASTLE ) ) {
        return {};
    }

    std::vector<int> towers;
    towers.push_back( TWR_CENTER );

    if ( cstl.isBuild( BUILD_LEFTTURRET ) )
        towers.push_back( TWR_LEFT );
    if ( cstl.isBuild( BUILD_RIGHTTURRET ) )
        towers.push_back( TWR_RIGHT );

    const char * tmpl = _( "The %{name} fires with the strength of %{count} Archers" );
    const char * addn = _( "each with a +%{attack} bonus to their attack skill." );

    std::string msg;
    for ( std::vector<int>::const_iterator it = towers.begin(); it != towers.end(); ++it ) {
        Tower twr( cstl, *it, Rand::DeterministicRandomGenerator( 0 ), 0 );

        msg.append( tmpl );
        StringReplace( msg, "%{name}", twr.GetName() );
        StringReplace( msg, "%{count}", twr.GetCount() );

        if ( twr.GetBonus() ) {
            msg.append( ", " );
            msg.append( addn );
            StringReplace( msg, "%{attack}", twr.GetBonus() );
        }
        else {
            msg += '.';
        }

        if ( ( it + 1 ) != towers.end() )
            msg.append( "\n \n" );
    }

    return msg;
}
