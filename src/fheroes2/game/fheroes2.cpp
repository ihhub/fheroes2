/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2026                                             *
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

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#include <SDL_main.h> // IWYU pragma: keep

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif

#if defined( _WIN32 )
#include <cassert>
#endif

#include "component_base.h"
#include "exception.h"
#include "game.h"
#include "game_init.h"
#include "game_invalid_assets.h"
#include "logging.h"

int main( int argc, char ** argv )
{
// SDL2main.lib converts argv to UTF-8, but this application expects ANSI, use the original argv
#if defined( _WIN32 )
    assert( argc == __argc );

    argv = __argv;
#else
    (void)argc;
#endif

    try {
        auto hardwareComponent = Game::createHardwareComponent();

        Game::initLogging();
        Game::initDataDir();
        Game::initConfigDir( argv[0] );

        auto coreComponent = Game::createCoreComponent();
        auto displayComponent = Game::createDisplayComponent();
        auto dataComponent = Game::createDataComponent();
        auto audioComponent = Game::createAudioComponent( dataComponent.get() );

        Game::initPalette();
        Game::initTranslations();
        Game::initEventHandler();
        Game::initAnimation();
        Game::initHotKeys();

        try {
            Game::runMainGameLoop();
        }
        catch ( const fheroes2::InvalidDataResources & ex ) {
            ERROR_LOG( ex.what() )
            showMissingAssetsImage();
            return EXIT_FAILURE;
        }
        catch ( const fheroes2::CorruptedExecutable & ex ) {
            ERROR_LOG( ex.what() )
            return EXIT_FAILURE;
        }
        catch ( const fheroes2::UserRequestedApplicationClosure & ) {
            // Yes, this an evil way of doing things but our application design doesn't allow to simply propagate application closure event.
            return EXIT_SUCCESS;
        }
    }
    catch ( const std::exception & ex ) {
        ERROR_LOG( "Exception '" << ex.what() << "' occurred during application runtime." )
        return EXIT_FAILURE;
    }
    catch ( ... ) {
        ERROR_LOG( "An unknown exception occurred during application runtime." )
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
