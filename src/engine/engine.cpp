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

#include "engine.h"
#include "font.h"
#include "localevent.h"
#include "logging.h"
#include "sdlnet.h"

#if defined( FHEROES2_VITA )
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>

// allocating memory for application on Vita
int _newlib_heap_size_user = 192 * 1024 * 1024;
#endif

namespace Mixer
{
    void Init();
    void Quit();
}

#ifdef WITH_AUDIOCD
namespace Cdrom
{
    void Open( void );
    void Close( void );
}
#endif

bool SDL::Init( const uint32_t system )
{
    if ( 0 > SDL_Init( system ) ) {
        ERROR_LOG( SDL_GetError() );
        return false;
    }

    if ( SDL_INIT_AUDIO & system )
        Mixer::Init();
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( SDL_INIT_GAMECONTROLLER & system ) {
        LocalEvent::Get().OpenController();
    }
    LocalEvent::Get().OpenTouchpad();
#endif
#ifdef WITH_AUDIOCD
    if ( SDL_INIT_CDROM & system )
        Cdrom::Open();
#endif
#ifdef WITH_TTF
    FontTTF::Init();
#endif
#ifdef WITH_NET
    Network::Init();
#endif

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#else
    SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
#endif

    return true;
}

void SDL::Quit()
{
#ifdef WITH_NET
    Network::Quit();
#endif
#ifdef WITH_TTF
    FontTTF::Quit();
#endif
#ifdef WITH_AUDIOCD
    if ( SubSystem( SDL_INIT_CDROM ) )
        Cdrom::Close();
#endif
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( SubSystem( SDL_INIT_GAMECONTROLLER ) ) {
        LocalEvent::Get().CloseController();
    }
#endif
    if ( SubSystem( SDL_INIT_AUDIO ) )
        Mixer::Quit();

    SDL_Quit();
}

bool SDL::SubSystem( const uint32_t system )
{
    return system & SDL_WasInit( system );
}

void InitHardware()
{
#if defined( FHEROES2_VITA )
    scePowerSetArmClockFrequency( 444 );
    scePowerSetBusClockFrequency( 222 );
    scePowerSetGpuClockFrequency( 222 );
    scePowerSetGpuXbarClockFrequency( 166 );
#endif
}

void CloseHardware()
{
#if defined( FHEROES2_VITA )
    sceKernelExitProcess( 0 );
#endif
}
