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

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "game_mode.h"
#include "math_base.h"
#include "ui_button.h"

namespace Multi
{
    // Returns the "NETWORK" button of the multi-player menu.
    fheroes2::ButtonSprite makeNetworkButton( const fheroes2::Rect & area );

    fheroes2::ButtonSprite makeTextButton( const fheroes2::Rect & area, const char * untranslatedText );

    // The online lobby, games by address and the network settings. Returns the next mode of the main loop.
    fheroes2::GameMode networkMenu();

    struct LobbyHosting;

    // Goes to the scenario screen or the saved games, then publishes the game on the lobby while waiting for a player.
    fheroes2::GameMode hostOnLobby( const LobbyHosting & hosting, const bool isSavedGame, const bool isSimultaneous );

    // Ends the network game if the main loop goes to a mode which has nothing to do with it.
    void onMainLoopMode( const fheroes2::GameMode mode );

    // Called by the main loop for START_GAME. Waits for the other player first if the player chose to host this game.
    fheroes2::GameMode startGame();

    // Parses "name" or "name:port". Returns an empty value if the address is not valid.
    std::optional<std::pair<std::string, uint16_t>> parseHostAddress( const std::string & text );

    // Saves the simultaneous turns setting of the host for the next games.
    void rememberSimultaneousTurns( const bool isEnabled );

    // Returns the "NETWORK" button of the file dialog, to change the simultaneous turns setting. Empty if this is not a network game.
    fheroes2::ButtonSprite makeFileDialogButton( const fheroes2::Rect & cancelArea );

    // Returns true if the file dialog should be closed.
    bool handleFileDialogButton( fheroes2::ButtonSprite & button );
}
