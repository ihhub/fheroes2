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

#include "thread.h"
#include "system.h"

using namespace SDL;

Timer::Timer()
    : id( 0 )
{}

void Timer::Run( u32 interval, u32 ( *fn )( u32, void * ), void * param )
{
    if ( id )
        Remove();

    id = SDL_AddTimer( interval, fn, param );
}

void Timer::Remove( void )
{
    if ( id ) {
        SDL_RemoveTimer( id );
        id = 0;
    }
}

bool Timer::IsValid( void ) const
{
    return id;
}

Time::Time() {}

void Time::Start( void )
{
    tick2 = tick1 = SDL_GetTicks();
}

void Time::Stop( void )
{
    tick2 = SDL_GetTicks();
}

u32 Time::Get( void ) const
{
    return tick2 > tick1 ? tick2 - tick1 : 0;
}

void Time::Print( const char * header ) const
{
    ERROR( ( header ? header : "time: " ) << Get() << " ms" );
}
