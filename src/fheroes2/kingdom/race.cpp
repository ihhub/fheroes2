/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2022                                                    *
 *                                                                         *
 *   Free Heroes2 Engine:http://sourceforge.net/projects/fheroes2          *
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
#include "rand.h"
#include "translations.h"

std::string Race::String( int race )
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
        break;
    }

    return _( "race|Neutral" );
}

int Race::Rand( void )
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
