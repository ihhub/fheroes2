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

#include <cassert>

#include "rand.h"
#include "tools.h"
#include "translations.h"
#include "week.h"

namespace
{
    WeekName WeekRand( const uint32_t seed )
    {
        uint32_t weekTypeSeed = seed;
        fheroes2::hashCombine( weekTypeSeed, 367245 ); // Salt

        uint32_t weekType = Rand::GetWithSeed( 0, 3, weekTypeSeed );

        // A regular week, probability 75%
        if ( weekType < 3 ) {
            uint32_t weekSeed = seed;
            fheroes2::hashCombine( weekSeed, 1946256 ); // Salt

            return Rand::GetWithSeed( WeekName::SQUIRREL, WeekName::CONDOR, weekSeed );
        }

        // The Week of a monster, probability 25%
        return WeekName::MONSTERS;
    }

    WeekName MonthRand( const uint32_t seed )
    {
        uint32_t monthTypeSeed = seed;
        fheroes2::hashCombine( monthTypeSeed, 9536582 ); // Salt

        uint32_t monthType = Rand::GetWithSeed( 0, 9, monthTypeSeed );

        // A regular month, probability 50%
        if ( monthType < 5 ) {
            uint32_t monthSeed = seed;
            fheroes2::hashCombine( monthSeed, 5544783 ); // Salt

            return Rand::GetWithSeed( WeekName::ANT, WeekName::BEETLE, monthSeed );
        }

        // The Month of a monster, probability 40%
        if ( monthType < 9 ) {
            return WeekName::MONSTERS;
        }

        // The Month of the Plague, probability 10%
        return WeekName::PLAGUE;
    }

    Monster::MonsterType RandomMonsterWeekOf( const uint32_t seed )
    {
        uint32_t monsterSeed = seed;
        fheroes2::hashCombine( monsterSeed, 886473 ); // Salt

        return Rand::GetWithSeed( Monster::PEASANT, Monster::BONE_DRAGON, monsterSeed );
    }

    Monster::MonsterType RandomMonsterMonthOf( const uint32_t seed )
    {
        uint32_t monsterSeed = seed;
        fheroes2::hashCombine( monsterSeed, 1130906 ); // Salt

        switch ( Rand::GetWithSeed( 1, 12, monsterSeed ) ) {
        case 1:
            return Monster::PEASANT;
        case 2:
            return Monster::WOLF;
        case 3:
            return Monster::OGRE;
        case 4:
            return Monster::TROLL;
        case 5:
            return Monster::DWARF;
        case 6:
            return Monster::DRUID;
        case 7:
            return Monster::UNICORN;
        case 8:
            return Monster::CENTAUR;
        case 9:
            return Monster::GARGOYLE;
        case 10:
            return Monster::ROC;
        case 11:
            return Monster::VAMPIRE;
        case 12:
            return Monster::LICH;
        default:
            assert( 0 );
        }

        return Monster::UNKNOWN;
    }
}

const char * Week::GetName() const
{
    switch ( _week ) {
    case WeekName::UNNAMED:
        break;

    case WeekName::SQUIRREL:
        return _( "week|Squirrel" );
    case WeekName::RABBIT:
        return _( "week|Rabbit" );
    case WeekName::GOPHER:
        return _( "week|Gopher" );
    case WeekName::BADGER:
        return _( "week|Badger" );
    case WeekName::RAT:
        return _( "week|Rat" );
    case WeekName::EAGLE:
        return _( "week|Eagle" );
    case WeekName::WEASEL:
        return _( "week|Weasel" );
    case WeekName::RAVEN:
        return _( "week|Raven" );
    case WeekName::MONGOOSE:
        return _( "week|Mongoose" );
    case WeekName::DOG:
        return _( "week|Dog" );
    case WeekName::AARDVARK:
        return _( "week|Aardvark" );
    case WeekName::LIZARD:
        return _( "week|Lizard" );
    case WeekName::TORTOISE:
        return _( "week|Tortoise" );
    case WeekName::HEDGEHOG:
        return _( "week|Hedgehog" );
    case WeekName::CONDOR:
        return _( "week|Condor" );

    case WeekName::ANT:
        return _( "week|Ant" );
    case WeekName::GRASSHOPPER:
        return _( "week|Grasshopper" );
    case WeekName::DRAGONFLY:
        return _( "week|Dragonfly" );
    case WeekName::SPIDER:
        return _( "week|Spider" );
    case WeekName::BUTTERFLY:
        return _( "week|Butterfly" );
    case WeekName::BUMBLEBEE:
        return _( "week|Bumblebee" );
    case WeekName::LOCUST:
        return _( "week|Locust" );
    case WeekName::EARTHWORM:
        return _( "week|Earthworm" );
    case WeekName::HORNET:
        return _( "week|Hornet" );
    case WeekName::BEETLE:
        return _( "week|Beetle" );

    case WeekName::MONSTERS:
        return Monster( _monster ).GetName();

    case WeekName::PLAGUE:
        return _( "week|PLAGUE" );

    default:
        assert( 0 );
    }

    return _( "week|Unnamed" );
}

Week Week::RandomWeek( const bool isNewMonth, const uint32_t weekSeed )
{
    const WeekName weekName = isNewMonth ? MonthRand( weekSeed ) : WeekRand( weekSeed );

    if ( weekName == WeekName::MONSTERS ) {
        if ( isNewMonth ) {
            return { weekName, RandomMonsterMonthOf( weekSeed ) };
        }

        return { weekName, RandomMonsterWeekOf( weekSeed ) };
    }

    return { weekName, Monster::UNKNOWN };
}
