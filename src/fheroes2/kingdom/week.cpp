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
#include "engine.h"
#include "game.h"
#include "gamedefs.h"
#include "settings.h"
#include "world.h"

const char * Week::GetName( void ) const
{
    const char * str_name[] = {"Unnamed",
                               _( "week|PLAGUE" ),
                               _( "week|Ant" ),
                               _( "week|Grasshopper" ),
                               _( "week|Dragonfly" ),
                               _( "week|Spider" ),
                               _( "week|Butterfly" ),
                               _( "week|Bumblebee" ),
                               _( "week|Locust" ),
                               _( "week|Earthworm" ),
                               _( "week|Hornet" ),
                               _( "week|Beetle" ),
                               _( "week|Squirrel" ),
                               _( "week|Rabbit" ),
                               _( "week|Gopher" ),
                               _( "week|Badger" ),
                               _( "week|Eagle" ),
                               _( "week|Weasel" ),
                               _( "week|Raven" ),
                               _( "week|Mongoose" ),
                               _( "week|Aardvark" ),
                               _( "week|Lizard" ),
                               _( "week|Tortoise" ),
                               _( "week|Hedgehog" ),
                               _( "week|Condor" )};

    switch ( first ) {
    case PLAGUE:
        return str_name[1];
    case ANT:
        return str_name[2];
    case GRASSHOPPER:
        return str_name[3];
    case DRAGONFLY:
        return str_name[4];
    case SPIDER:
        return str_name[5];
    case BUTTERFLY:
        return str_name[6];
    case BUMBLEBEE:
        return str_name[7];
    case LOCUST:
        return str_name[8];
    case EARTHWORM:
        return str_name[9];
    case HORNET:
        return str_name[10];
    case BEETLE:
        return str_name[11];
    case SQUIRREL:
        return str_name[12];
    case RABBIT:
        return str_name[13];
    case GOPHER:
        return str_name[14];
    case BADGER:
        return str_name[15];
    case EAGLE:
        return str_name[16];
    case WEASEL:
        return str_name[17];
    case RAVEN:
        return str_name[18];
    case MONGOOSE:
        return str_name[19];
    case AARDVARK:
        return str_name[20];
    case LIZARD:
        return str_name[21];
    case TORTOISE:
        return str_name[22];
    case HEDGEHOG:
        return str_name[23];
    case CONDOR:
        return str_name[24];
    case MONSTERS:
        return Monster( second ).GetName();
    default:
        break;
    }

    return str_name[0];
}

int Week::WeekRand( void )
{
    return ( ( 0 == ( world.CountWeek() + 1 ) % 3 ) && ( !Settings::Get().ExtWorldBanWeekOf() ) ) ? static_cast<int>( MONSTERS ) : Rand::Get( ANT, CONDOR );
}

int Week::MonthRand( void )
{
    return ( ( 0 == ( world.GetMonth() + 1 ) % 3 ) && ( !Settings::Get().ExtWorldBanWeekOf() ) )
               ? static_cast<int>( MONSTERS )
               : Rand::Get( Settings::Get().ExtWorldBanPlagues() ? ANT : PLAGUE, CONDOR );
}

StreamBase & operator>>( StreamBase & sb, Week & st )
{
    return sb >> st.first >> st.second;
}
