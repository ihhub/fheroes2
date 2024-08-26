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
#include "race.h"

#include <cassert>

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

bool Race::isMagicalRace( const int race )
{
    switch ( race ) {
    case KNGT:
    case BARB:
        return false;
    case SORC:
    case WRLK:
    case WZRD:
    case NECR:
        return true;
    default:
        assert( 0 );
        break;
    }

    return false;
}

uint8_t Race::IndexToRace( const int index )
{
    switch ( index ) {
    case 0:
        return Race::KNGT;
    case 1:
        return Race::BARB;
    case 2:
        return Race::SORC;
    case 3:
        return Race::WRLK;
    case 4:
        return Race::WZRD;
    case 5:
        return Race::NECR;
    case 6:
        return Race::MULT;
    case 7:
        return Race::RAND;
    default:
        break;
    }

    return Race::NONE;
}

uint32_t Race::getRaceIcnIndex( const int race, const bool isActivePlayer )
{
    switch ( race ) {
    case Race::KNGT:
        return isActivePlayer ? 51 : 70;
    case Race::BARB:
        return isActivePlayer ? 52 : 71;
    case Race::SORC:
        return isActivePlayer ? 53 : 72;
    case Race::WRLK:
        return isActivePlayer ? 54 : 73;
    case Race::WZRD:
        return isActivePlayer ? 55 : 74;
    case Race::NECR:
        return isActivePlayer ? 56 : 75;
    case Race::MULT:
        return isActivePlayer ? 57 : 76;
    case Race::RAND:
        return 58;
    default:
        // Did you add a new race? Add the logic above!
        assert( 0 );
        break;
    }

    return 58;
}

int Race::getNextRace( const int race )
{
    switch ( race ) {
    case Race::KNGT:
        return Race::BARB;
    case Race::BARB:
        return Race::SORC;
    case Race::SORC:
        return Race::WRLK;
    case Race::WRLK:
        return Race::WZRD;
    case Race::WZRD:
        return Race::NECR;
    case Race::NECR:
        return Race::RAND;
    case Race::RAND:
        return Race::KNGT;
    default:
        // Did you add a new race? Add the logic above
        assert( 0 );
        break;
    }
    return Race::NONE;
}

int Race::getPreviousRace( const int race )
{
    switch ( race ) {
    case Race::KNGT:
        return Race::RAND;
    case Race::BARB:
        return Race::KNGT;
    case Race::SORC:
        return Race::BARB;
    case Race::WRLK:
        return Race::SORC;
    case Race::WZRD:
        return Race::WRLK;
    case Race::NECR:
        return Race::WZRD;
    case Race::RAND:
        return Race::NECR;
    default:
        // Did you add a new race? Add the logic above
        assert( 0 );
        break;
    }
    return Race::NONE;
}
