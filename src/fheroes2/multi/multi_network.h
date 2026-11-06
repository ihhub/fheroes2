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
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "multi_lobby_protocol.h"
#include "multi_protocol.h"

namespace Multi
{
    constexpr uint16_t defaultPort{ 47219 };

    // Returns the IPv4 addresses of this computer, Tailscale ones first.
    std::vector<std::string> getLocalAddresses();

    // The IPv6 address of this computer on the internet, empty without one. The other player can reach it without opening
    // the box in IPv4, if the box lets IPv6 connections in.
    std::string getInternetAddressV6();

    // A numeric IPv6 address, written without brackets.
    bool isIPv6Address( const std::string & text );

    // False for a build without network games (see WITH_NETWORK).
    bool isNetworkAvailable();

    // Used by logs and self-tests.
    struct SessionStats
    {
        uint32_t connectionsLost{ 0 };
        uint32_t connectionsTakenBack{ 0 };
        uint64_t messagesSentAgain{ 0 };
    };

    // The connections of this computer with the other computers of the game: on the host one link per guest, on a guest
    // the link with the host. Guests never connect to each other, the host carries their messages. Nothing is blocking,
    // update() must be called once per frame.
    // A link outlives its TCP connections. The guest reconnects on its own and unconfirmed messages are sent again in order.
    class NetworkSession
    {
    public:
        enum class State
        {
            IDLE,
            // Host only: the port is open.
            LISTENING,
            CONNECTING,
            CONNECTED,
            // Connection lost. Messages sent meanwhile are delivered once it is back.
            RECONNECTING,
            CLOSED
        };

        NetworkSession();
        NetworkSession( const NetworkSession & ) = delete;

        ~NetworkSession();

        NetworkSession & operator=( const NetworkSession & ) = delete;

        // Waits for the other computers, from this computer only if localOnly is set. The port stays open until the session is closed.
        bool host( const uint16_t port, const bool localOnly );

        // The address can be an IP address or a host name.
        void join( const std::string & address, const uint16_t port );

        void update();

        // Messages sent before connecting are kept until connected. A guest sends to other guests through the host.
        void send( const MachineId machine, const Message & message );
        void sendToMachines( const MachineSet machines, const Message & message );

        // Received messages can still be read after the connection is closed. Each one tells its sender.
        std::optional<Message> receive();

        // Returns the first message of the given type, never one past an order barrier (see isOrderBarrier()) in the
        // messages of the same sender. On a guest all messages come from the host and follow one order.
        std::optional<Message> receive( const MessageType type );

        // Same as above but for any of the given types.
        std::optional<Message> receive( const std::initializer_list<MessageType> types );

        // Same as above but for the messages the filter accepts.
        std::optional<Message> receive( const std::function<bool( const Message & message )> & filter );

        // Same as receive( type ) but ignores order barriers.
        std::optional<Message> receiveAnywhere( const MessageType type );

        // Puts a received message back to its original place in the queue.
        void putBack( Message message );

        // The other computers are told not to wait for this computer to come back.
        void close();

        // Guest: the link with the host. Host: LISTENING until closed.
        State state() const;

        // The link with another computer. A host's link stays CLOSED until closeLink() or admit() drops it.
        State state( const MachineId machine ) const;

        // Returns true if connected or waiting for the connection to come back (guest), or still hosting (host).
        bool isAlive() const;
        bool isAlive( const MachineId machine ) const;

        bool isHostSide() const;

        // Returns true if the other side closed the link on purpose.
        bool isClosedByOther() const;
        bool isClosedByOther( const MachineId machine ) const;

        // Guest only. The host game was restarted and does not know this session anymore. The guest must join again as a new player.
        bool isResumeRefused() const;

        // Guest only.
        const std::string & joinedAddress() const;
        uint16_t joinedPort() const;

        // Returns the size of sent messages not confirmed yet, by all computers or by one.
        size_t undeliveredBytes() const;
        size_t undeliveredBytes( const MachineId machine ) const;

        // Returns for how long the connection has been lost while in RECONNECTING state.
        std::chrono::steady_clock::duration lostFor() const;
        std::chrono::steady_clock::duration lostFor( const MachineId machine ) const;

        // The guest reconnects on its own for two minutes, then waits for retry().
        bool isRetrying() const;
        void retry();

        // Host only. The guests with a link, closed links included until dropped. New computers are not among them.
        std::vector<MachineId> guests() const;

        // A computer which connected to the host, and the first message it sent (a handshake).
        struct Newcomer
        {
            // Temporary, never one of a guest.
            MachineId machine{ hostMachine };
            Message firstMessage;
        };

        // Host only. admit() gives the new computer its place, closeLink() refuses it. Until then nothing it sends is received.
        std::optional<Newcomer> takeNewcomer();

        // Host only. The new computer becomes this guest. A previous link of that guest is dropped with its messages.
        void admit( const MachineId newcomer, const MachineId machine );

        // Host only. Tells the computer, once the messages already sent have left, and drops the link with its messages.
        void closeLink( const MachineId machine );

        const SessionStats & stats() const;

        // Used by self-tests. Cuts every link, a guest waits for the given time before reconnecting.
        void cutForTest( const std::chrono::milliseconds outage );

        // Used by self-tests. Cuts each link at about the given interval, sometimes for a few seconds.
        void setTestCuts( const std::chrono::milliseconds interval );

        // Empty if the session was closed on purpose.
        const std::string & error() const;
        const std::string & error( const MachineId machine ) const;

        // Returns true if the address to join could not be found on the network.
        bool isAddressUnknown() const;

        // The size of messages still waiting to leave this computer.
        size_t pendingOutgoingBytes() const;

        // Host only. The lobby where the game is published checks with this key that the game can be reached directly.
        void setProbeKey( const LobbyKey & key );

        // Host only. A connection waits on the relay of the lobby for the other computers, as on the port of the game, until
        // the session is closed. The relay address is the one given by the lobby, or the address of the lobby. When the
        // room was published again, after a restart of the lobby for example, a connection waits again at once.
        void listenOnRelay( const std::string & relayAddress, const uint64_t roomId, const LobbyKey & hostToken, const uint32_t publishCount );

        // Instead of join(): through the relay of the lobby, and the same way to take a lost connection back.
        void joinThroughRelay( const std::string & relayAddress, const uint64_t roomId, const LobbyKey & ticket );

        // Guest only.
        bool isThroughRelay() const;

    private:
        struct Impl;

        std::unique_ptr<Impl> _impl;
    };

    // Connection to a lobby server, which carries lobby messages over WebSocket (see multi_lobby_protocol.h). Nothing is
    // blocking, update() must be called once per frame.
    class LobbyConnection
    {
    public:
        enum class State
        {
            IDLE,
            CONNECTING,
            OPEN,
            CLOSED
        };

        LobbyConnection();
        LobbyConnection( const LobbyConnection & ) = delete;

        ~LobbyConnection();

        LobbyConnection & operator=( const LobbyConnection & ) = delete;

        // The address as typed by the player, see parseLobbyAddress().
        void open( const std::string & address );

        void update();

        // Messages sent before the connection is open are kept until then.
        void send( const Message & message );

        std::optional<Message> receive();

        void close();

        State state() const;

        // Empty if the connection was closed on purpose.
        const std::string & error() const;

        // Time for a WebSocket ping to come back, 0 until it is known.
        std::chrono::milliseconds delay() const;

    private:
        struct Impl;

        std::unique_ptr<Impl> _impl;
    };
}
