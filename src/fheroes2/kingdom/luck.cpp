/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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

#include "luck.h"
#include "tools.h"
#include "translations.h"

std::string Luck::String( int luck )
{
    switch ( luck ) {
    case Luck::CURSED:
        return _( "luck|Cursed" );
    case Luck::AWFUL:
        return _( "luck|Awful" );
    case Luck::BAD:
        return _( "luck|Bad" );
    case Luck::NORMAL:
        return _( "luck|Normal" );
    case Luck::GOOD:
        return _( "luck|Good" );
    case Luck::GREAT:
        return _( "luck|Great" );
    case Luck::IRISH:
        return _( "luck|Irish" );
    default:
        break;
    }

    return "Unknown";
}

std::string Luck::Description( int luck )
{
    switch ( luck ) {
    case Luck::CURSED:
    case Luck::AWFUL:
    case Luck::BAD:
        return _( "Bad luck sometimes falls on your armies in combat, causing their attacks to only do half damage." );
    case Luck::NORMAL:
        return _( "Neutral luck means your armies will never get lucky or unlucky attacks on the enemy." );
    case Luck::GOOD:
    case Luck::GREAT:
    case Luck::IRISH:
        return _( "Good luck sometimes lets your armies get lucky attacks (double strength) in combat." );
    default:
        break;
    }

    return "Unknown";
}

int Luck::Normalize( const int luck )
{
    return clamp( luck, static_cast<int>( Luck::CURSED ), static_cast<int>( Luck::IRISH ) );
}
