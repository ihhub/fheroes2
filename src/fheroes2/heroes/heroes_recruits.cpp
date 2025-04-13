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

#include "heroes_recruits.h"

#include <cassert>

#include "game_io.h"
#include "heroes.h"
#include "save_format_version.h"
#include "serialize.h"
#include "world.h"

Recruit::Recruit()
    : _id( Heroes::UNKNOWN )
    , _surrenderDay( 0 )
{}

Recruit::Recruit( const Heroes & hero, const uint32_t surrenderDay )
    : _id( hero.GetID() )
    , _surrenderDay( surrenderDay )
{}

Recruit::Recruit( const Heroes & hero )
    : Recruit( hero, 0 )
{}

Recruits::Recruits()
    : std::pair<Recruit, Recruit>()
{}

void Recruits::Reset()
{
    first = {};
    second = {};
}

int Recruits::GetID1() const
{
    return first.getID();
}

int Recruits::GetID2() const
{
    return second.getID();
}

Heroes * Recruits::GetHero1() const
{
    return world.GetHeroes( first.getID() );
}

Heroes * Recruits::GetHero2() const
{
    return world.GetHeroes( second.getID() );
}

uint32_t Recruits::getSurrenderDayOfHero1() const
{
    return first.getSurrenderDay();
}

uint32_t Recruits::getSurrenderDayOfHero2() const
{
    return second.getSurrenderDay();
}

void Recruits::SetHero1( Heroes * hero )
{
    if ( hero ) {
        hero->SetModes( Heroes::RECRUIT );

        first = Recruit( *hero );
    }
    else {
        first = {};
    }
}

void Recruits::SetHero2( Heroes * hero )
{
    if ( hero ) {
        hero->SetModes( Heroes::RECRUIT );

        second = Recruit( *hero );
    }
    else {
        second = {};
    }
}

void Recruits::appendSurrenderedHero( Heroes & hero, const uint32_t heroSurrenderDay )
{
    assert( heroSurrenderDay > 0 );

    hero.SetModes( Heroes::RECRUIT );

    // The original game offers to hire only the hero who retreated or surrendered last. fheroes2 offers to hire up to the last two heroes of this kind.
    Recruit & recruit = ( first.getSurrenderDay() > second.getSurrenderDay() ? second : first );

    recruit = Recruit( hero, heroSurrenderDay );
}

OStreamBase & operator<<( OStreamBase & stream, const Recruit & recruit )
{
    return stream << recruit._id << recruit._surrenderDay;
}

IStreamBase & operator>>( IStreamBase & stream, Recruit & recruit )
{
    stream >> recruit._id >> recruit._surrenderDay;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1010_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1010_RELEASE ) {
        ++recruit._id;
    }

    return stream;
}
