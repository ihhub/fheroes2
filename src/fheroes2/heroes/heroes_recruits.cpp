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

#include "heroes_recruits.h"
#include "heroes.h"
#include "serialize.h"
#include "world.h"

Recruits::Recruits()
    : heroId1( HeroInfo::Id::UNKNOWN )
    , heroId2( HeroInfo::Id::UNKNOWN )
{}

void Recruits::Reset()
{
    heroId1 = HeroInfo::Id::UNKNOWN;
    heroId2 = HeroInfo::Id::UNKNOWN;
}

HeroInfo::Id Recruits::GetID1() const
{
    return heroId1;
}

HeroInfo::Id Recruits::GetID2() const
{
    return heroId2;
}

Heroes * Recruits::GetHero1() const
{
    return world.GetHeroes( heroId1 );
}

Heroes * Recruits::GetHero2() const
{
    return world.GetHeroes( heroId2 );
}

void Recruits::SetHero1( const Heroes * hero )
{
    heroId1 = hero ? hero->id : HeroInfo::Id::UNKNOWN;
}

void Recruits::SetHero2( const Heroes * hero )
{
    heroId2 = hero ? hero->id : HeroInfo::Id::UNKNOWN;
}

StreamBase & operator>>( StreamBase & sb, Recruits & rt )
{
    int heroId1;
    int heroId2;
    sb >> heroId1 >> heroId2;
    rt.heroId1 = static_cast<HeroInfo::Id>( heroId1 );
    rt.heroId2 = static_cast<HeroInfo::Id>( heroId2 );
    return sb;
}

StreamBase & operator<<( StreamBase & sb, const Recruits & rt )
{
    const int heroId1 = static_cast<int>( rt.heroId1 );
    const int heroId2 = static_cast<int>( rt.heroId2 );
    return sb << heroId1 << heroId2;
}
