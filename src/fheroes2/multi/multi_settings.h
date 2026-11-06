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
#include <string>
#include <vector>

namespace Multi
{
    // The lobby every game knows. Players can add other ones.
    constexpr const char * sharedLobbyAddress{ "ws://lobby.fheroes2-multi.workers.dev" };

    // Choices of the player for network games, kept in multi.cfg in the configuration directory.
    struct NetworkSettings
    {
        // The last address typed to join a game.
        std::string address;
        bool isSimultaneous{ false };
        // UTF-8, as in lobby messages. Empty until the player chose one.
        std::string nickname;
        // Lobbies added by the player, after the shared one.
        std::vector<std::string> lobbies;
        // Neither player learns the address of the other.
        bool isRelayAlways{ false };
        // New games are shown in the list of the lobby, or only to the players who have their invite code.
        bool isPublic{ true };
        // Only in the file, for tests and groups of friends with a lobby of their own.
        bool isSharedLobbyUsed{ true };
        // Only in the file. Tells the host this computer when it joins again, 0 until made.
        uint64_t machineKey{ 0 };
    };

    NetworkSettings loadNetworkSettings();
    void saveNetworkSettings( const NetworkSettings & settings );

    // The shared lobby first, unless the file says otherwise, then the ones added by the player.
    std::vector<std::string> getLobbyAddresses( const NetworkSettings & settings );

    // The key of this computer, made and kept the first time.
    uint64_t getMachineKey();
}
