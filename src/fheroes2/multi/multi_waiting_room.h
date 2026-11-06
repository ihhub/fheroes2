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
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "multi_lobby_protocol.h"

// The waiting room, from the moment a game is hosted or joined until the host starts it: the game, the players and the
// colors they take, how to join and a chat between the players. The same for games of the lobby and games joined by
// address.
namespace Multi
{
    class NetworkSession;

    // How to join the game of the host, shown in its waiting room.
    struct JoinHelp
    {
        // Shown in big letters. Empty without lobby.
        std::string inviteCode;
        // Translated, in the code page of the game.
        std::string text;
    };

    struct HostRoomHooks
    {
        // The session waiting for the other computers, nothing on error.
        std::function<std::unique_ptr<NetworkSession>( std::string & error )> openSession;

        // Called every frame: the internet box and the lobby. Returns false with an error to stop hosting.
        std::function<bool( NetworkSession & session, JoinHelp & help, std::string & error )> update;

        // A player came in or left, or took or gave back colors: the number of human colors nobody has taken.
        std::function<void( int freeColors )> onPlayersChange;
    };

    // A guest of the waiting room, given to the game when it starts.
    struct RoomGuest
    {
        MachineId machine{ hostMachine };
        // UTF-8, may be empty.
        std::string nickname;
        uint64_t machineKey{ 0 };
        uint8_t colors{ 0 };
    };

    // The players once the game starts.
    struct RoomPlayers
    {
        uint8_t hostColors{ 0 };
        std::vector<RoomGuest> guests;
    };

    enum class WaitingRoomEnd
    {
        // The game starts.
        STARTED,
        // The player left the waiting room, or the host closed it.
        LEFT,
        QUIT_GAME,
        // The error is set.
        FAILED
    };

    // Host. Takes in the other computers as they come, until its player starts the game and every guest is ready. The
    // first human color is the host's. A guest gets the first free one and can take others or give them back.
    WaitingRoomEnd runHostWaitingRoom( const WaitingRoomInfo & info, const HostRoomHooks & hooks, std::unique_ptr<NetworkSession> & session, RoomPlayers & players,
                                       std::string & error );

    // Guest, after the handshake. On STARTED the game sent by the host is next in the session.
    WaitingRoomEnd runGuestWaitingRoom( NetworkSession & session, std::string & error );

    // The waiting room of the game about to start: its name (UTF-8), the human and computer colors. No player yet.
    WaitingRoomInfo makeWaitingRoomInfo( const std::string & gameName );

    // Self-tests, without screen. The host takes in the computers which come, gives each one the colors it had before a
    // restart of the host, or else the next colors of the test, and starts once all of them are there. keepWaiting() is
    // called at every frame. A guest waits for the start. With isPickingColors, it first asks for every free color, then
    // gives them back, as a player clicking on them.
    bool startGameForTest( NetworkSession & session, const WaitingRoomInfo & info, const std::vector<uint8_t> & guestColors, const std::function<bool()> & keepWaiting,
                           RoomPlayers & players );
    bool waitForStartForTest( NetworkSession & session, const bool isPickingColors );

    // Shows the waiting room of a host until Enter or Esc, with guests and a few lines of chat or without. Used by
    // self-tests only.
    void showWaitingRoomForTest( const WaitingRoomInfo & info, const JoinHelp & help, const bool hasGuests );

    // During the game, for messages of the waiting room which come late. Returns false if the message is not one of them.
    bool handleLateWaitingRoomMessage( NetworkSession & session, const Message & message );
}
