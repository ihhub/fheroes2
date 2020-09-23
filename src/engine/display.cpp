/***************************************************************************
 *   Copyright (C) 2008 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <string>

#include "display.h"
#include "error.h"
#include "system.h"
#include "tools.h"
#include "types.h"

// This is new Graphics engine. To change the code slowly we have to do some hacks here for now
#include "screen.h"

Display::Display()
{
    _isDisplay = true;
}

Display::~Display() {}

Size Display::GetSize( void ) const
{
    const fheroes2::Display & display = fheroes2::Display::instance();
    return Size( display.width(), display.height() );
}

Size Display::GetDefaultSize( void )
{
    return Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );
}

std::string Display::GetInfo( void ) const
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    std::ostringstream os;
    os << "Display::GetInfo: " << GetString( GetSize() ) << ", "
       << "driver: " << SDL_GetCurrentVideoDriver();
    return os.str();
#else
    std::ostringstream os;
    char namebuf[12];

    os << "Display::"
       << "GetInfo: " << GetString( GetSize() ) << ", "
       << "driver: " << SDL_VideoDriverName( namebuf, 12 );

    return os.str();
#endif
}

/* hide system cursor */
void Display::HideCursor( void )
{
    SDL_ShowCursor( SDL_DISABLE );
}

/* show system cursor */
void Display::ShowCursor( void )
{
    SDL_ShowCursor( SDL_ENABLE );
}

/* get video display */
Display & Display::Get( void )
{
    static Display inside;
    return inside;
}
