/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

namespace
{
    uint32_t getSlowerSpeed( const uint32_t speed )
    {
        switch ( speed ) {
        case Speed::CRAWLING:
        case Speed::VERYSLOW:
            return Speed::CRAWLING;
        case Speed::SLOW:
        case Speed::AVERAGE:
            return Speed::VERYSLOW;
        case Speed::FAST:
        case Speed::VERYFAST:
            return Speed::SLOW;
        case Speed::ULTRAFAST:
        case Speed::BLAZING:
            return Speed::AVERAGE;
        case Speed::INSTANT:
            return Speed::FAST;
        default:
            break;
        }

        return Speed::STANDING;
    }

    uint32_t getFasterSpeed( const uint32_t speed )
    {
        switch ( speed ) {
        case Speed::CRAWLING:
            return Speed::SLOW;
        case Speed::VERYSLOW:
            return Speed::AVERAGE;
        case Speed::SLOW:
            return Speed::FAST;
        case Speed::AVERAGE:
            return Speed::VERYFAST;
        case Speed::FAST:
            return Speed::ULTRAFAST;
        case Speed::VERYFAST:
            return Speed::BLAZING;
        case Speed::ULTRAFAST:
        case Speed::BLAZING:
        case Speed::INSTANT:
            return Speed::INSTANT;
        default:
            break;
        }

        return Speed::STANDING;
    }
}

namespace Speed
{
    const char * String( const uint32_t speed )
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

    uint32_t getSlowSpeedFromSpell( const uint32_t currentSpeed )
    {
        const uint32_t spellExtraValue = Spell( Spell::SLOW ).ExtraValue();
        return spellExtraValue ? currentSpeed - spellExtraValue : getSlowerSpeed( currentSpeed );
    }

    uint32_t getHasteSpeedFromSpell( const uint32_t currentSpeed )
    {
        const uint32_t spellExtraValue = Spell( Spell::HASTE ).ExtraValue();
        return spellExtraValue ? currentSpeed + spellExtraValue : getFasterSpeed( currentSpeed );
    }
}
