/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

    Monster::monster_t RandomMonsterWeekOf( const uint32_t seed )
    {
        uint32_t monsterSeed = seed;
        fheroes2::hashCombine( monsterSeed, 886473 ); // Salt

        switch ( Rand::GetWithSeed( 1, 47, monsterSeed ) ) {
        case 1:
            return Monster::PEASANT;
        case 2:
            return Monster::ARCHER;
        case 3:
            return Monster::RANGER;
        case 4:
            return Monster::PIKEMAN;
        case 5:
            return Monster::VETERAN_PIKEMAN;
        case 6:
            return Monster::SWORDSMAN;
        case 7:
            return Monster::MASTER_SWORDSMAN;
        case 8:
            return Monster::CAVALRY;
        case 9:
            return Monster::CHAMPION;
        case 10:
            return Monster::GOBLIN;
        case 11:
            return Monster::ORC;
        case 12:
            return Monster::ORC_CHIEF;
        case 13:
            return Monster::WOLF;
        case 14:
            return Monster::OGRE;
        case 15:
            return Monster::OGRE_LORD;
        case 16:
            return Monster::TROLL;
        case 17:
            return Monster::WAR_TROLL;
        case 18:
            return Monster::SPRITE;
        case 19:
            return Monster::DWARF;
        case 20:
            return Monster::BATTLE_DWARF;
        case 21:
            return Monster::ELF;
        case 22:
            return Monster::GRAND_ELF;
        case 23:
            return Monster::DRUID;
        case 24:
            return Monster::GREATER_DRUID;
        case 25:
            return Monster::UNICORN;
        case 26:
            return Monster::CENTAUR;
        case 27:
            return Monster::GARGOYLE;
        case 28:
            return Monster::GRIFFIN;
        case 29:
            return Monster::MINOTAUR;
        case 30:
            return Monster::MINOTAUR_KING;
        case 31:
            return Monster::HYDRA;
        case 32:
            return Monster::HALFLING;
        case 33:
            return Monster::BOAR;
        case 34:
            return Monster::IRON_GOLEM;
        case 35:
            return Monster::STEEL_GOLEM;
        case 36:
            return Monster::ROC;
        case 37:
            return Monster::MAGE;
        case 38:
            return Monster::ARCHMAGE;
        case 39:
            return Monster::SKELETON;
        case 40:
            return Monster::ZOMBIE;
        case 41:
            return Monster::MUTANT_ZOMBIE;
        case 42:
            return Monster::MUMMY;
        case 43:
            return Monster::ROYAL_MUMMY;
        case 44:
            return Monster::VAMPIRE;
        case 45:
            return Monster::VAMPIRE_LORD;
        case 46:
            return Monster::LICH;
        case 47:
            return Monster::POWER_LICH;
        default:
            assert( 0 );
        }

        return Monster::UNKNOWN;
    }

    Monster::monster_t RandomMonsterMonthOf( const uint32_t seed )
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
    case WeekName::HOUND:
        return _( "week|Hound" );
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

    return "Unnamed";
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
