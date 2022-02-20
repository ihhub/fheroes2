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

#include "morale.h"
#include "tools.h"
#include "translations.h"

std::string Morale::String( int morale )
{
    switch ( morale ) {
    case Morale::TREASON:
        return _( "morale|Treason" );
    case Morale::AWFUL:
        return _( "morale|Awful" );
    case Morale::POOR:
        return _( "morale|Poor" );
    case Morale::NORMAL:
        return _( "morale|Normal" );
    case Morale::GOOD:
        return _( "morale|Good" );
    case Morale::GREAT:
        return _( "morale|Great" );
    case Morale::BLOOD:
        return _( "morale|Blood!" );
    default:
        break;
    }

    return "Unknown";
}

std::string Morale::Description( int morale )
{
    switch ( morale ) {
    case Morale::TREASON:
    case Morale::AWFUL:
    case Morale::POOR:
        return _( "Bad morale may cause your armies to freeze in combat." );
    case Morale::NORMAL:
        return _( "Neutral morale means your armies will never be blessed with extra attacks or freeze in combat." );
    case Morale::GOOD:
    case Morale::GREAT:
    case Morale::BLOOD:
        return _( "Good morale may give your armies extra attacks in combat." );
    default:
        break;
    }

    return "Unknown";
}

int Morale::Normalize( const int morale )
{
    return clamp( morale, static_cast<int>( Morale::TREASON ), static_cast<int>( Morale::BLOOD ) );
}
