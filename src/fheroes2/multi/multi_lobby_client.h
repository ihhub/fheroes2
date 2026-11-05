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

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "multi_lobby_protocol.h"

// What the game does with lobbies: the list of the rooms of every lobby of the settings, joining one of them, and the room
// published by a host. Texts given to and returned by these classes are UTF-8, as in lobby messages.
namespace Multi
{
    struct LobbyState
    {
        enum class Status
        {
            CONNECTING,
            OPEN,
            // The lobby does not take this game (version, nickname, full...). No new try.
            REFUSED,
            // The connection failed or was lost. The game tries again a few seconds later.
            LOST
        };

        std::string address;
        Status status{ Status::CONNECTING };
        // Name, message of the day and relay of the lobby, once open.
        LobbyWelcome welcome;
        LobbyStats stats;
        std::chrono::milliseconds delay{ 0 };
        // Why the lobby refused this game or was lost.
        std::string error;
    };

    struct ListedRoom
    {
        size_t lobbyIndex{ 0 };
        RoomInfo info;
        // Built from the same sources as this game: the room can be joined.
        bool isSameGame{ false };
    };

    struct JoinAnswer
    {
        size_t lobbyIndex{ 0 };
        JoinPlan plan;
        // Where the relay of this lobby is.
        std::string relayAddress;
    };

    // Connections to the lobbies, for the lobby screen. Their rooms come in one list, the ones this game can join first,
    // then the newest first. Nothing is blocking, update() must be called once per frame.
    class LobbyBrowser
    {
    public:
        LobbyBrowser( const std::vector<std::string> & addresses, const std::string & nickname );
        LobbyBrowser( const LobbyBrowser & ) = delete;

        ~LobbyBrowser();

        LobbyBrowser & operator=( const LobbyBrowser & ) = delete;

        // Returns true if anything to show changed since the last call.
        bool update();

        const std::vector<LobbyState> & lobbies() const;
        const std::vector<ListedRoom> & rooms() const;

        // Asks the lobby of the room how to join it. The answer comes with takeJoinAnswer().
        void join( const ListedRoom & room );

        // Asks every open lobby. The answer of the lobby which knows the code comes with takeJoinAnswer().
        void joinByCode( const std::string & code );

        bool isJoining() const;

        std::optional<JoinAnswer> takeJoinAnswer();

    private:
        struct Impl;

        std::unique_ptr<Impl> _impl;
    };

    // A room published by this computer. It stays until the end of the game: the relay needs it, and a guest who lost the
    // connection finds the host again this way. Nothing is blocking, update() must be called regularly.
    class PublishedRoom
    {
    public:
        enum class State
        {
            // Not published yet. After a failed connection the game tries again, error() tells why.
            CONNECTING,
            PUBLISHED,
            // The lobby refused the game for good.
            FAILED
        };

        PublishedRoom( const std::string & lobbyAddress, const std::string & nickname, const RoomPublish & room );
        PublishedRoom( const PublishedRoom & ) = delete;

        // The room is closed.
        ~PublishedRoom();

        PublishedRoom & operator=( const PublishedRoom & ) = delete;

        void update();

        State state() const;

        // False during a cut of the connection to the lobby. The game connects again on its own and takes the room back.
        bool isConnected() const;

        // Why the room is not published or the connection was lost.
        const std::string & error() const;

        // Invite code and ways to reach the host. Sent again by the lobby when its probe is over.
        const RoomStatus & status() const;

        const LobbyWelcome & lobby() const;

        const std::string & lobbyAddress() const;

        // Lets the host wait for guests on the relay.
        const LobbyKey & hostToken() const;

        // Grows each time the lobby takes the room, again after a cut or a restart of the lobby.
        uint32_t publishCount() const;

        // Kept and sent again after a cut.
        void setState( const RoomState state, const RoomGame & game );

        // Used by self-tests.
        void cutForTest();

    private:
        struct Impl;

        std::unique_ptr<Impl> _impl;
    };

    // The relay of a lobby: the one it gave, or the lobby itself.
    std::string getRelayAddress( const std::string & lobbyAddress, const LobbyWelcome & welcome );

    class NetworkSession;

    // Host only. The session waits for the other computer on the relay of the room too, from the moment the room is
    // published. To call regularly: the room may change after a long cut of the connection to the lobby.
    void listenOnRelayOfRoom( const PublishedRoom & room, NetworkSession & session );

    // Random bytes for the probe key of a room.
    LobbyKey makeRandomLobbyKey();

    // The current game as shown in the list of a lobby. Texts are converted to UTF-8.
    RoomGame makeRoomGameOfCurrentGame( const uint8_t guestHumanPlayers );

    // Hello of this game. The nickname is UTF-8.
    LobbyHello makeLobbyHelloOfThisGame( const std::string & nickname );
}
