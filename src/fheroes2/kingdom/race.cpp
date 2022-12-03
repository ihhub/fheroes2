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

#include "race.h"
#include "rand.h"
#include "translations.h"

const char * Race::String( int race )
{
    switch ( race ) {
    case Race::KNGT:
        return _( "Knight" );
    case Race::BARB:
        return _( "Barbarian" );
    case Race::SORC:
        return _( "Sorceress" );
    case Race::WRLK:
        return _( "Warlock" );
    case Race::WZRD:
        return _( "Wizard" );
    case Race::NECR:
        return _( "Necromancer" );
    case Race::MULT:
        return _( "Multi" );
    case Race::RAND:
        return _( "race|Random" );
    case Race::NONE:
        return _( "race|Neutral" );
    default:
        // Did you add a new race? Add the logic above!
        assert( 0 );
        break;
    }

    return _( "race|Neutral" );
}

const char * Race::DoubleLinedString( int race )
{
    switch ( race ) {
    case Race::KNGT:
        return _( "doubleLined|Knight" );
    case Race::BARB:
        return _( "doubleLined|Barbarian" );
    case Race::SORC:
        return _( "doubleLined|Sorceress" );
    case Race::WRLK:
        return _( "doubleLined|Warlock" );
    case Race::WZRD:
        return _( "doubleLined|Wizard" );
    case Race::NECR:
        return _( "doubleLined|Necro-\nmancer" );
    case Race::MULT:
        return _( "doubleLinedRace|Multi" );
    case Race::RAND:
        return _( "doubleLinedRace|Random" );
    case Race::NONE:
        return _( "doubleLinedRace|Neutral" );
    default:
        // Did you add a new race? Add the logic above!
        assert( 0 );
        break;
    }

    return _( "doubleLinedRace|Neutral" );
}

int Race::Rand()
{
    switch ( Rand::Get( 1, 6 ) ) {
    case 1:
        return Race::KNGT;
    case 2:
        return Race::BARB;
    case 3:
        return Race::SORC;
    case 4:
        return Race::WRLK;
    case 5:
        return Race::WZRD;
    default:
        break;
    }

    return Race::NECR;
}
