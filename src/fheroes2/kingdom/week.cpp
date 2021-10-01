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

#include "week.h"
#include "rand.h"
#include "serialize.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

namespace
{
    WeekName WeekRand( const World & worldInstance, const size_t seed )
    {
        return ( 0 == ( worldInstance.CountWeek() + 1 ) % 3 ) ? WeekName::MONSTERS : Rand::GetWithSeed( WeekName::ANT, WeekName::CONDOR, static_cast<uint32_t>( seed ) );
    }

    WeekName MonthRand( const World & worldInstance, const size_t seed )
    {
        return ( 0 == ( worldInstance.GetMonth() + 1 ) % 3 ) ? WeekName::MONSTERS
                                                             : Rand::GetWithSeed( WeekName::PLAGUE, WeekName::CONDOR, static_cast<uint32_t>( seed ) );
    }

    Monster::monster_t RandomMonsterWeekOf( const size_t seed )
    {
        switch ( Rand::GetWithSeed( 1, 47, static_cast<uint32_t>( seed ) ) ) {
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
            return Monster::UNKNOWN;
        }
    }

    Monster::monster_t RandomMonsterMonthOf( const size_t seed )
    {
        switch ( Rand::GetWithSeed( 1, 30, static_cast<uint32_t>( seed ) ) ) {
        case 1:
            return Monster::PEASANT;
        case 2:
            return Monster::ARCHER;
        case 3:
            return Monster::PIKEMAN;
        case 4:
            return Monster::SWORDSMAN;
        case 5:
            return Monster::CAVALRY;
        case 6:
            return Monster::GOBLIN;
        case 7:
            return Monster::ORC;
        case 8:
            return Monster::WOLF;
        case 9:
            return Monster::OGRE;
        case 10:
            return Monster::TROLL;
        case 11:
            return Monster::SPRITE;
        case 12:
            return Monster::DWARF;
        case 13:
            return Monster::ELF;
        case 14:
            return Monster::DRUID;
        case 15:
            return Monster::UNICORN;
        case 16:
            return Monster::CENTAUR;
        case 17:
            return Monster::GARGOYLE;
        case 18:
            return Monster::GRIFFIN;
        case 19:
            return Monster::MINOTAUR;
        case 20:
            return Monster::HYDRA;
        case 21:
            return Monster::HALFLING;
        case 22:
            return Monster::BOAR;
        case 23:
            return Monster::IRON_GOLEM;
        case 24:
            return Monster::ROC;
        case 25:
            return Monster::MAGE;
        case 26:
            return Monster::SKELETON;
        case 27:
            return Monster::ZOMBIE;
        case 28:
            return Monster::MUMMY;
        case 29:
            return Monster::VAMPIRE;
        case 30:
            return Monster::LICH;
        default:
            assert( 0 );
            return Monster::UNKNOWN;
        }
    }
}

const char * Week::GetName( void ) const
{
    switch ( _week ) {
    case WeekName::PLAGUE:
        return _( "week|PLAGUE" );
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
    case WeekName::SQUIRREL:
        return _( "week|Squirrel" );
    case WeekName::RABBIT:
        return _( "week|Rabbit" );
    case WeekName::GOPHER:
        return _( "week|Gopher" );
    case WeekName::BADGER:
        return _( "week|Badger" );
    case WeekName::EAGLE:
        return _( "week|Eagle" );
    case WeekName::WEASEL:
        return _( "week|Weasel" );
    case WeekName::RAVEN:
        return _( "week|Raven" );
    case WeekName::MONGOOSE:
        return _( "week|Mongoose" );
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
    case WeekName::MONSTERS:
        return Monster( _monster ).GetName();
    default:
        break;
    }

    return "Unnamed";
}

Week Week::RandomWeek( const World & worldInstance, const bool isNewMonth, const size_t weekSeed )
{
    size_t weekTypeSeed = weekSeed;
    fheroes2::hashCombine( weekTypeSeed, 34582445 ); // random value to add salt

    const WeekName weekName = isNewMonth ? MonthRand( worldInstance, weekTypeSeed ) : WeekRand( worldInstance, weekTypeSeed );

    if ( weekName == WeekName::MONSTERS ) {
        size_t monsterTypeSeed = weekSeed;
        fheroes2::hashCombine( monsterTypeSeed, 284631 ); // random value to add salt
        if ( isNewMonth ) {
            return { weekName, RandomMonsterMonthOf( monsterTypeSeed ) };
        }

        return { weekName, RandomMonsterWeekOf( monsterTypeSeed ) };
    }

    return { weekName, Monster::UNKNOWN };
}

StreamBase & operator>>( StreamBase & stream, Week & week )
{
    int32_t weekType;
    int32_t monster;
    StreamBase & sb = stream >> weekType >> monster;
    week._week = static_cast<WeekName>( weekType );
    week._monster = static_cast<Monster::monster_t>( monster );
    return sb;
}

StreamBase & operator<<( StreamBase & stream, const Week & week )
{
    return stream << static_cast<int32_t>( week.GetType() ) << static_cast<int32_t>( week.GetMonster() );
}
