/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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

#include <cstdint>
#include <string>

#include "ai.h"
#include "heroes_base.h"
#include "mp2.h"
#include "serialize.h"
#include "translations.h"

class Castle;
class Heroes;

namespace AI
{
    int Base::GetPersonality() const
    {
        return _personality;
    }

    std::string Base::GetPersonalityString() const
    {
        switch ( _personality ) {
        case WARRIOR:
            return _( "Warrior" );
        case BUILDER:
            return _( "Builder" );
        case EXPLORER:
            return _( "Explorer" );
        default:
            break;
        }

        return _( "None" );
    }

    void Base::Reset()
    {
        // Do nothing.
    }

    void Base::CastlePreBattle( Castle & )
    {
        // Do nothing.
    }

    void Base::CastleAfterBattle( Castle &, bool )
    {
        // Do nothing.
    }

    void Base::CastleAdd( const Castle & )
    {
        // Do nothing.
    }

    void Base::CastleRemove( const Castle & )
    {
        // Do nothing.
    }

    void Base::HeroesAdd( const Heroes & )
    {
        // Do nothing.
    }

    void Base::HeroesRemove( const Heroes & )
    {
        // Do nothing.
    }

    void Base::HeroesBeginMovement( Heroes & /* hero */ )
    {
        // Do nothing.
    }

    void Base::HeroesFinishMovement( Heroes & /* hero */ )
    {
        // Do nothing.
    }

    void Base::HeroesPreBattle( HeroBase &, bool )
    {
        // Do nothing.
    }

    void Base::HeroesAfterBattle( HeroBase & hero, bool /*unused*/ )
    {
        hero.ActionAfterBattle();
    }

    void Base::HeroesActionNewPosition( Heroes & )
    {
        // Do nothing.
    }

    std::string Base::HeroesString( const Heroes & )
    {
        return std::string();
    }

    void Base::HeroesActionComplete( Heroes & /*hero*/, const int32_t /* tileIndex*/, const MP2::MapObjectType /*objectType*/ )
    {
        // Do nothing.
    }

    void Base::HeroesLevelUp( Heroes & )
    {
        // Do nothing.
    }

    void Base::HeroesPostLoad( Heroes & )
    {
        // Do nothing.
    }

    StreamBase & operator<<( StreamBase & msg, const AI::Base & instance )
    {
        return msg << instance._personality;
    }

    StreamBase & operator>>( StreamBase & msg, AI::Base & instance )
    {
        return msg >> instance._personality;
    }
}
