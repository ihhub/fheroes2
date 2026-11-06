/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "multi_command_line.h"

#include <cstdlib>
#include <string>
#include <string_view>

#include "game.h"
#include "game_mode.h"
#include "logging.h"
#include "multi_game.h"
#include "multi_network.h"
#include "multi_selftest.h"
#include "multi_simultaneous.h"

namespace
{
    // The application closes when the game ends.
    int runNetworkGame( fheroes2::GameMode mode )
    {
        while ( mode == fheroes2::GameMode::START_GAME || mode == fheroes2::GameMode::MULTI_WAIT_FOR_TURN ) {
            mode = ( mode == fheroes2::GameMode::START_GAME ) ? Game::StartGame() : Multi::waitForTurn();
        }

        if ( Multi::isNetworkGame() ) {
            Multi::endGame();
        }

        return EXIT_SUCCESS;
    }
}

std::optional<int> Multi::runFromCommandLine( const int argc, const char * const * argv )
{
    if ( argc < 2 ) {
        return {};
    }

    const std::string_view command( argv[1] );

    if ( command == "--multi-host" && argc >= 3 && argc <= 5 ) {
        // "local": connections from this computer only
        bool isLocalOnly = false;
        bool isSimultaneous = false;
        bool isValid = true;

        for ( int i = 3; i < argc; ++i ) {
            const std::string_view option( argv[i] );
            isLocalOnly = isLocalOnly || option == "local";
            isSimultaneous = isSimultaneous || option == "simultaneous";
            isValid = isValid && ( option == "local" || option == "simultaneous" );
        }

        if ( isValid ) {
            setSimultaneousTurns( isSimultaneous );
            return runNetworkGame( hostGame( argv[2], defaultPort, isLocalOnly ) );
        }
    }

    if ( command == "--multi-join" && argc == 3 ) {
        return runNetworkGame( joinGame( argv[2], defaultPort ) );
    }

    if ( command.rfind( "--multi-", 0 ) == 0 && command.rfind( "--multi-selftest", 0 ) != 0 ) {
        ERROR_LOG( "[MULTI] Invalid command line." )
        return EXIT_FAILURE;
    }

    return runSelfTestIfRequested( argc, argv );
}
