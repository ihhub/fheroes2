/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include "core.h"

#include <cassert>
#include <cstdint>
#include <stdexcept>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#include <SDL.h>
#include <SDL_error.h>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif

#include "audio.h"
#include "localevent.h"
#include "logging.h"

#if defined( TARGET_PS_VITA )
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>

// PlayStation Vita requires definition of such global variable for memory usage.
int _newlib_heap_size_user = 192 * 1024 * 1024;
#endif

namespace
{
#if defined( TARGET_PS_VITA )
    void initHardwareInternally()
    {
        // CPU clock speed, MHz. Possible values: 41, 83, 111, 166, 222, 333, 444, 500
        scePowerSetArmClockFrequency( 444 );
        // CPU memory clock speed, MHz. Possible values: 55, 83, 111, 166, 222
        scePowerSetBusClockFrequency( 222 );
        // GPU clock speed, MHz. Possible values: 41, 55, 83, 111, 166, 222
        scePowerSetGpuClockFrequency( 222 );
        // GPU memory clock speed, MHz. Possible values: 83, 111, 166
        scePowerSetGpuXbarClockFrequency( 166 );
    }

    void freeHardwareInternally()
    {
        sceKernelExitProcess( 0 );
    }
#else
    void initHardwareInternally()
    {
        // Do nothing.
    }

    void freeHardwareInternally()
    {
        // Do nothing.
    }
#endif

    uint32_t convertToSDLFlag( const fheroes2::SystemInitializationComponent component )
    {
        switch ( component ) {
        case fheroes2::SystemInitializationComponent::Audio:
            return SDL_INIT_AUDIO;
        case fheroes2::SystemInitializationComponent::Video:
            return SDL_INIT_VIDEO;
        case fheroes2::SystemInitializationComponent::GameController:
            return SDL_INIT_GAMECONTROLLER;
        default:
            // Did you add a new component?
            assert( 0 );
            break;
        }

        return 0;
    }

    uint32_t getSDLInitFlags( const std::set<fheroes2::SystemInitializationComponent> & components )
    {
        uint32_t flags = 0;
        for ( const fheroes2::SystemInitializationComponent component : components ) {
            flags |= convertToSDLFlag( component );
        }
        return flags;
    }

    // For now only SDL library is supported.
    bool initCoreInternally( const std::set<fheroes2::SystemInitializationComponent> & components )
    {
        const uint32_t sdlFlags = getSDLInitFlags( components );

        if ( SDL_Init( sdlFlags ) < 0 ) {
            ERROR_LOG( SDL_GetError() )
            return false;
        }

        if ( components.count( fheroes2::SystemInitializationComponent::Audio ) > 0 ) {
            Audio::Init();
        }

        if ( components.count( fheroes2::SystemInitializationComponent::GameController ) > 0 ) {
            LocalEvent::Get().initController();
        }

        LocalEvent::initEventEngine();

        return true;
    }

    void freeCoreInternally()
    {
        if ( fheroes2::isComponentInitialized( fheroes2::SystemInitializationComponent::GameController ) ) {
            LocalEvent::Get().CloseController();
        }

        if ( fheroes2::isComponentInitialized( fheroes2::SystemInitializationComponent::Audio ) ) {
            Audio::Quit();
        }

        SDL_Quit();
    }

    bool isComponentInitializedInternally( const fheroes2::SystemInitializationComponent component )
    {
        const uint32_t sdlFlag = convertToSDLFlag( component );
        assert( sdlFlag != 0 );

        return SDL_WasInit( sdlFlag ) != 0;
    }
}

namespace fheroes2
{
    HardwareInitializer::HardwareInitializer()
    {
        initHardwareInternally();
    }

    HardwareInitializer::~HardwareInitializer()
    {
        freeHardwareInternally();
    }

    CoreInitializer::CoreInitializer( const std::set<SystemInitializationComponent> & components )
    {
        if ( !initCoreInternally( components ) ) {
            throw std::logic_error( "Core module initialization failed." );
        }
    }

    CoreInitializer::~CoreInitializer()
    {
        freeCoreInternally();
    }

    bool isComponentInitialized( const SystemInitializationComponent component )
    {
        return isComponentInitializedInternally( component );
    }
}
