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

#pragma once

#include <cstdint>

namespace Speed
{
    enum : uint32_t
    {
        STANDING = 0,
        CRAWLING = 1,
        VERYSLOW = 2,
        SLOW = 3,
        AVERAGE = 4,
        FAST = 5,
        VERYFAST = 6,
        ULTRAFAST = 7,
        BLAZING = 8,
        INSTANT = 9
    };

    const char * String( const uint32_t speed );

    uint32_t getSlowSpeedFromSpell( const uint32_t currentSpeed );
    uint32_t getHasteSpeedFromSpell( const uint32_t currentSpeed );
}
