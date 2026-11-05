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

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "multi_protocol.h"

// Messages between a game and a lobby server. The lobby server is a separate program (server/lobby), which must read and
// write exactly the same bytes (server/lobby/protocol.js reads and writes the same messages).
//
// Texts are UTF-8. Every reader takes the whole payload and checks every field: anything wrong makes the message invalid.
namespace Multi
{
    // Increase this value whenever the content of a lobby message changes. A lobby server can serve several versions.
    constexpr uint32_t lobbyProtocolVersion{ 1 };

    // Lobby messages are small, a list of rooms comes in several parts.
    constexpr uint32_t maxLobbyMessagePayloadSize{ 64 * 1024 };

    constexpr size_t maxRoomsPerListMessage{ 100 };

    // Probe keys, host tokens and relay tickets.
    using LobbyKey = std::array<uint8_t, 16>;

    bool isValidNickname( const std::string & nickname );
    bool isValidGameName( const std::string & name );
    bool isValidMapName( const std::string & name );

    // Six characters, without 0, O, 1 and I.
    bool isValidInviteCode( const std::string & code );

    struct LobbyHello
    {
        uint32_t lobbyProtocolVersion{ 0 };
        HandshakeInfo game;
        std::string language;
        std::string nickname;
    };

    enum class LobbyWelcomeResult : uint8_t
    {
        ACCEPTED = 0,
        INVALID_MESSAGE = 1,
        INCOMPATIBLE_VERSIONS = 2,
        NICKNAME_REFUSED = 3,
        LOBBY_FULL = 4,
        TOO_MANY_CONNECTIONS = 5
    };

    struct LobbyWelcome
    {
        LobbyWelcomeResult result{ LobbyWelcomeResult::INVALID_MESSAGE };
        uint32_t lobbyProtocolVersion{ 0 };
        std::string lobbyName;
        std::string messageOfTheDay;
        // Empty when the relay is at the address of the lobby.
        std::string relayAddress;
    };

    struct LobbyStats
    {
        uint32_t playersOnline{ 0 };
        uint32_t waitingRooms{ 0 };
        uint32_t roomsInGame{ 0 };
    };

    enum class RoomTurns : uint8_t
    {
        ONE_AFTER_ANOTHER = 0,
        SIMULTANEOUS = 1
    };

    struct RoomGame
    {
        std::string mapName;
        uint16_t mapWidth{ 0 };
        uint16_t mapHeight{ 0 };
        uint8_t players{ 0 };
        uint8_t humanPlayers{ 0 };
        uint8_t guestHumanPlayers{ 0 };
        RoomTurns turns{ RoomTurns::ONE_AFTER_ANOTHER };
        // 1 for a new game.
        uint32_t day{ 1 };
    };

    enum class RoomState : uint8_t
    {
        WAITING_FOR_PLAYERS = 0,
        IN_GAME = 1
    };

    enum class RoomVisibility : uint8_t
    {
        PUBLIC = 0,
        INVITE_ONLY = 1
    };

    struct RoomPublish
    {
        RoomVisibility visibility{ RoomVisibility::PUBLIC };
        // No address given to guests, no probe.
        bool isRelayOnly{ false };
        std::string gameName;
        RoomGame game;
        uint16_t gamePort{ 0 };
        // The host answers only probes with this key.
        LobbyKey probeKey{};
        // Public addresses known by the host: its IPv6 address, the IPv4 address given by the box.
        std::vector<std::string> publicAddresses;
        // Tried first by a guest behind the same box.
        std::vector<std::string> localAddresses;
        // 0 and empty for a new room. Published again after a cut or a restart of the lobby, the room keeps them.
        uint64_t roomId{ 0 };
        std::string inviteCode;
        RoomState state{ RoomState::WAITING_FOR_PLAYERS };
        // Chosen by the host: they let a lobby which was restarted bring the room back as it was.
        LobbyKey hostToken{};
        // Given to guests to come through the relay.
        LobbyKey guestTicket{};
    };

    enum class RoomStatusResult : uint8_t
    {
        PUBLISHED = 0,
        INVALID_MESSAGE = 1,
        GAME_NAME_REFUSED = 2,
        TOO_MANY_ROOMS = 3
    };

    struct RoomReach
    {
        bool isIPv4{ false };
        bool isIPv6{ false };
        bool isProbeDone{ false };
        bool isRelayAvailable{ false };

        bool operator==( const RoomReach & other ) const
        {
            return isIPv4 == other.isIPv4 && isIPv6 == other.isIPv6 && isProbeDone == other.isProbeDone && isRelayAvailable == other.isRelayAvailable;
        }
    };

    // Sent again when the probe is over. Only the result is sent if the room is refused.
    struct RoomStatus
    {
        RoomStatusResult result{ RoomStatusResult::INVALID_MESSAGE };
        uint64_t roomId{ 0 };
        std::string inviteCode;
        RoomReach reach;
    };

    struct RoomUpdate
    {
        RoomState state{ RoomState::WAITING_FOR_PLAYERS };
        RoomGame game;
    };

    // A room as shown in the list, without any address.
    struct RoomInfo
    {
        uint64_t roomId{ 0 };
        RoomState state{ RoomState::WAITING_FOR_PLAYERS };
        RoomReach reach;
        std::string gameName;
        std::string hostNickname;
        std::string hostLanguage;
        HandshakeInfo hostGame;
        RoomGame game;
    };

    // A part of the list. The list is complete with the last part.
    struct RoomList
    {
        bool isLastPart{ true };
        std::vector<RoomInfo> rooms;
    };

    // By room id, or by invite code when the room id is 0.
    struct RoomJoin
    {
        uint64_t roomId{ 0 };
        std::string inviteCode;
    };

    enum class JoinCandidateKind : uint8_t
    {
        LOCAL = 0,
        IPV6 = 1,
        IPV4 = 2
    };

    struct JoinCandidate
    {
        JoinCandidateKind kind{ JoinCandidateKind::LOCAL };
        std::string address;
        uint16_t port{ 0 };
    };

    enum class JoinResult : uint8_t
    {
        READY = 0,
        NO_SUCH_ROOM = 1,
        ROOM_IN_GAME = 2,
        OTHER_VERSION = 3,
        INVALID_MESSAGE = 4
    };

    // Only the result is sent if the guest cannot join.
    struct JoinPlan
    {
        JoinResult result{ JoinResult::INVALID_MESSAGE };
        RoomInfo room;
        // Direct addresses in the order to try.
        std::vector<JoinCandidate> candidates;
        // All zeros if there is no relay.
        LobbyKey relayTicket{};
    };

    enum class RelayRefusal : uint8_t
    {
        INVALID_MESSAGE = 0,
        NO_SUCH_ROOM = 1,
        NO_HOST_WAITING = 2,
        LIMIT_REACHED = 3,
        // The host closed the game on purpose.
        ROOM_CLOSED = 4
    };

    struct LobbyProbe
    {
        LobbyKey probeKey{};
        LobbyKey value{};
    };

    Message makeLobbyHello( const LobbyHello & hello );
    std::optional<LobbyHello> readLobbyHello( const Message & message );

    Message makeLobbyWelcome( const LobbyWelcome & welcome );
    std::optional<LobbyWelcome> readLobbyWelcome( const Message & message );

    Message makeLobbyStats( const LobbyStats & stats );
    std::optional<LobbyStats> readLobbyStats( const Message & message );

    Message makeRoomPublish( const RoomPublish & publish );
    std::optional<RoomPublish> readRoomPublish( const Message & message );

    Message makeRoomStatus( const RoomStatus & status );
    std::optional<RoomStatus> readRoomStatus( const Message & message );

    Message makeRoomUpdate( const RoomUpdate & update );
    std::optional<RoomUpdate> readRoomUpdate( const Message & message );

    // LOBBY_KEEPALIVE, ROOM_CLOSE and RELAY_READY have no payload.
    Message makeEmptyLobbyMessage( const MessageType type );
    bool readEmptyLobbyMessage( const Message & message, const MessageType type );

    Message makeRoomListSubscribe( const bool isSubscribed );
    std::optional<bool> readRoomListSubscribe( const Message & message );

    Message makeRoomList( const RoomList & list );
    std::optional<RoomList> readRoomList( const Message & message );

    Message makeRoomInfo( const RoomInfo & room );
    std::optional<RoomInfo> readRoomInfo( const Message & message );

    Message makeRoomRemoved( const uint64_t roomId );
    std::optional<uint64_t> readRoomRemoved( const Message & message );

    Message makeRoomJoin( const RoomJoin & join );
    std::optional<RoomJoin> readRoomJoin( const Message & message );

    Message makeJoinPlan( const JoinPlan & plan );
    std::optional<JoinPlan> readJoinPlan( const Message & message );

    // RELAY_HOST (host token), RELAY_GUEST (ticket) and LOBBY_PROBE_REPLY (random value of the probe) contain only a key.
    Message makeLobbyKeyMessage( const MessageType type, const LobbyKey & key );
    std::optional<LobbyKey> readLobbyKeyMessage( const Message & message, const MessageType type );

    Message makeRelayRefused( const RelayRefusal reason );
    std::optional<RelayRefusal> readRelayRefused( const Message & message );

    Message makeLobbyProbe( const LobbyProbe & probe );
    std::optional<LobbyProbe> readLobbyProbe( const Message & message );

    // The waiting room, before the game starts. These messages go between the games through their session, never
    // through the lobby, so that any game gets a waiting room, also when joined by address.

    // The host or a guest in the waiting room.
    struct RoomPlayer
    {
        MachineId machine{ hostMachine };
        // Empty while the player has none.
        std::string nickname;
        // The human colors (as in PlayerColorsSet) the player takes.
        uint8_t colors{ 0 };
    };

    // Sent by the host to each guest whenever it changes.
    struct WaitingRoomInfo
    {
        std::string gameName;
        RoomGame game;
        // Colors (as in PlayerColorsSet) of the human players and of the computer players. Human colors which nobody
        // takes are played by the computer.
        uint8_t humanColors{ 0 };
        uint8_t computerColors{ 0 };
        // The host first, then the guests by number. Each color is taken by one player at most.
        std::vector<RoomPlayer> players;
        // The computer which gets the message.
        MachineId receiver{ hostMachine };
    };

    Message makeWaitingRoom( const WaitingRoomInfo & info );
    std::optional<WaitingRoomInfo> readWaitingRoom( const Message & message );

    // Sent by a guest when its player clicks a color: the human colors it wants to play, one at least. The host gives the
    // ones nobody else has.
    Message makePlayerInfo( const uint8_t wantedColors );
    std::optional<uint8_t> readPlayerInfo( const Message & message );

    // One line of the chat, 1 to 120 characters.
    bool isValidChatText( const std::string & text );
    Message makeChat( const std::string & text );
    std::optional<std::string> readChat( const Message & message );
}
