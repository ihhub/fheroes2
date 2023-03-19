/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include "speed.h"
#include "spell.h"
#include "translations.h"

std::string Speed::String( int speed )
{
    switch ( speed ) {
    case STANDING:
        return _( "speed|Standing" );
    case CRAWLING:
        return _( "speed|Crawling" );
    case VERYSLOW:
        return _( "speed|Very Slow" );
    case SLOW:
        return _( "speed|Slow" );
    case AVERAGE:
        return _( "speed|Average" );
    case FAST:
        return _( "speed|Fast" );
    case VERYFAST:
        return _( "speed|Very Fast" );
    case ULTRAFAST:
        return _( "speed|Ultra Fast" );
    case BLAZING:
        return _( "speed|Blazing" );
    case INSTANT:
        return _( "speed|Instant" );
    default:
        break;
    }

    return "Unknown";
}

int Speed::GetOriginalSlow( int speed )
{
    switch ( speed ) {
    case CRAWLING:
    case VERYSLOW:
        return CRAWLING;
    case SLOW:
    case AVERAGE:
        return VERYSLOW;
    case FAST:
    case VERYFAST:
        return SLOW;
    case ULTRAFAST:
    case BLAZING:
        return AVERAGE;
    case INSTANT:
        return FAST;
    default:
        break;
    }

    return STANDING;
}

int Speed::GetOriginalFast( int speed )
{
    switch ( speed ) {
    case CRAWLING:
        return SLOW;
    case VERYSLOW:
        return AVERAGE;
    case SLOW:
        return FAST;
    case AVERAGE:
        return VERYFAST;
    case FAST:
        return ULTRAFAST;
    case VERYFAST:
        return BLAZING;
    case ULTRAFAST:
    case BLAZING:
    case INSTANT:
        return INSTANT;
    default:
        break;
    }

    return STANDING;
}

int Speed::GetSlowSpeedFromSpell( const int currentSpeed )
{
    Spell spell = Spell::SLOW;
    return spell.ExtraValue() ? currentSpeed - spell.ExtraValue() : Speed::GetOriginalSlow( currentSpeed );
}

int Speed::GetHasteSpeedFromSpell( const int currentSpeed )
{
    Spell spell = Spell::HASTE;
    return spell.ExtraValue() ? currentSpeed + spell.ExtraValue() : Speed::GetOriginalFast( currentSpeed );
}
