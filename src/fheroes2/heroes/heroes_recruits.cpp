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

#include <cassert>

#include "game.h"
#include "heroes_recruits.h"
#include "save_format_version.h"
#include "serialize.h"
#include "world.h"

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
    return first.id;
}

int Recruits::GetID2() const
{
    return second.id;
}

Heroes * Recruits::GetHero1() const
{
    return world.GetHeroes( first.id );
}

Heroes * Recruits::GetHero2() const
{
    return world.GetHeroes( second.id );
}

uint32_t Recruits::getSurrenderDay1() const
{
    return first.surrenderDay;
}

uint32_t Recruits::getSurrenderDay2() const
{
    return second.surrenderDay;
}

void Recruits::SetHero1( Heroes * hero )
{
    Heroes * oldHero = world.GetHeroes( first.id );

    if ( oldHero ) {
        oldHero->ResetModes( Heroes::AVAILFORHIRE );
    }

    if ( hero ) {
        first = Recruit( *hero );

        hero->SetModes( Heroes::AVAILFORHIRE );
    }
    else {
        first = {};
    }
}

void Recruits::SetHero2( Heroes * hero )
{
    Heroes * oldHero = world.GetHeroes( second.id );

    if ( oldHero ) {
        oldHero->ResetModes( Heroes::AVAILFORHIRE );
    }

    if ( hero ) {
        second = Recruit( *hero );

        hero->SetModes( Heroes::AVAILFORHIRE );
    }
    else {
        second = {};
    }
}

void Recruits::SetHero2Tmp( Heroes * hero, const uint32_t heroSurrenderDay )
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_0912_RELEASE, "Remove this method." );
    assert( hero != nullptr );

    SetHero2( hero );

    second.surrenderDay = heroSurrenderDay;
}

void Recruits::appendSurrenderedHero( Heroes & hero, const uint32_t heroSurrenderDay )
{
    assert( heroSurrenderDay > 0 );

    Recruit & recruit = ( first.surrenderDay > second.surrenderDay ? second : first );

    Heroes * oldHero = world.GetHeroes( recruit.id );

    if ( oldHero ) {
        oldHero->ResetModes( Heroes::AVAILFORHIRE );
    }

    recruit = Recruit( hero, heroSurrenderDay );

    hero.SetModes( Heroes::AVAILFORHIRE );
}

StreamBase & operator<<( StreamBase & msg, const Recruit & recruit )
{
    return msg << recruit.id << recruit.surrenderDay;
}

StreamBase & operator>>( StreamBase & msg, Recruit & recruit )
{
    msg >> recruit.id;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_0912_RELEASE, "Remove the check below." );
    if ( Game::GetLoadVersion() >= FORMAT_VERSION_0912_RELEASE ) {
        msg >> recruit.surrenderDay;
    }
    else {
        recruit.surrenderDay = 0;
    }

    return msg;
}
