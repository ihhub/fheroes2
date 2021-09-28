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
#include "translations.h"
#include "world.h"

const char * Week::GetName( void ) const
{
    switch ( first ) {
    case PLAGUE:
        return _( "week|PLAGUE" );
    case ANT:
        return _( "week|Ant" );
    case GRASSHOPPER:
        return _( "week|Grasshopper" );
    case DRAGONFLY:
        return _( "week|Dragonfly" );
    case SPIDER:
        return _( "week|Spider" );
    case BUTTERFLY:
        return _( "week|Butterfly" );
    case BUMBLEBEE:
        return _( "week|Bumblebee" );
    case LOCUST:
        return _( "week|Locust" );
    case EARTHWORM:
        return _( "week|Earthworm" );
    case HORNET:
        return _( "week|Hornet" );
    case BEETLE:
        return _( "week|Beetle" );
    case SQUIRREL:
        return _( "week|Squirrel" );
    case RABBIT:
        return _( "week|Rabbit" );
    case GOPHER:
        return _( "week|Gopher" );
    case BADGER:
        return _( "week|Badger" );
    case EAGLE:
        return _( "week|Eagle" );
    case WEASEL:
        return _( "week|Weasel" );
    case RAVEN:
        return _( "week|Raven" );
    case MONGOOSE:
        return _( "week|Mongoose" );
    case AARDVARK:
        return _( "week|Aardvark" );
    case LIZARD:
        return _( "week|Lizard" );
    case TORTOISE:
        return _( "week|Tortoise" );
    case HEDGEHOG:
        return _( "week|Hedgehog" );
    case CONDOR:
        return _( "week|Condor" );
    case MONSTERS:
        return Monster( second ).GetName();
    default:
        break;
    }

    return "Unnamed";
}

int Week::WeekRand( void )
{
    return ( 0 == ( world.CountWeek() + 1 ) % 3 ) ? static_cast<int>( MONSTERS ) : Rand::Get( ANT, CONDOR );
}

int Week::MonthRand( void )
{
    return ( 0 == ( world.GetMonth() + 1 ) % 3 ) ? static_cast<int>( MONSTERS ) : Rand::Get( PLAGUE, CONDOR );
}

StreamBase & operator>>( StreamBase & sb, Week & st )
{
    return sb >> st.first >> st.second;
}
