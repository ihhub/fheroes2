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

#include "multi_network.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <deque>
#include <exception>
#include <iomanip>
#include <random>
#include <sstream>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#if defined( WITH_NETWORK )

// asio is only included here so the rest of the game never depends on it.
#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif
#define ASIO_NO_DEPRECATED

#if !__has_include( <asio.hpp> )
#error "asio is missing: install the standalone asio headers (script/windows/install_packages.bat with Visual Studio)"
#endif

#include <asio.hpp>

#if defined( _WIN32 )
#include <mstcpip.h>
#elif defined( __linux__ )
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

#include "logging.h"
#include "multi_lobby_protocol.h"
#include "multi_websocket.h"
#include "translations.h"

namespace
{
    constexpr std::chrono::seconds connectionTimeout{ 10 };

    // A slower reconnection attempt is dropped and a new one starts.
    constexpr std::chrono::seconds reconnectionAttemptTimeout{ 5 };

    // After this the guest waits for the player to ask for a retry.
    constexpr std::chrono::seconds automaticRetryDuration{ 120 };

    // How often each side tells the other how many messages it received.
    constexpr std::chrono::milliseconds acknowledgementDelay{ 250 };

    // Above this size of unconfirmed messages the session is closed.
    constexpr size_t maxUnacknowledgedBytes{ size_t{ 64 } * 1024 * 1024 };

    std::chrono::milliseconds getRetryDelay( const uint32_t attempt )
    {
        constexpr std::array<int, 6> delays{ 0, 500, 1000, 2000, 3000, 5000 };
        return std::chrono::milliseconds( delays[std::min<size_t>( attempt, delays.size() - 1 )] );
    }

    uint64_t generateSessionId()
    {
        std::random_device device;
        std::mt19937_64 generator( ( static_cast<uint64_t>( device() ) << 32 ) ^ device()
                                   ^ static_cast<uint64_t>( std::chrono::steady_clock::now().time_since_epoch().count() ) );

        const uint64_t id = generator();
        return id == 0 ? 1 : id;
    }

    std::string describeError( const std::error_code & error )
    {
        if ( error == asio::error::eof || error == asio::error::connection_reset || error == asio::error::connection_aborted ) {
            return gettext_noop( "the other player closed the connection" );
        }
        if ( error == asio::error::connection_refused ) {
            return gettext_noop( "nobody is hosting a game at this address" );
        }
        if ( error == asio::error::host_not_found || error == asio::error::host_not_found_try_again ) {
            return gettext_noop( "unknown address" );
        }
        if ( error == asio::error::address_in_use ) {
            return gettext_noop( "the port is already used by another program" );
        }
        if ( error == asio::error::timed_out ) {
            return gettext_noop( "connection timed out" );
        }

        return error.message();
    }

    // By default a computer which disappears (crash, unplugged cable) is noticed only after hours, here in under a minute.
    // A paused game (tablet with its screen off) still answers keep-alive probes.
    void enableFastKeepAlive( asio::ip::tcp::socket & socket )
    {
        std::error_code ignored;
        socket.set_option( asio::socket_base::keep_alive( true ), ignored );

#if defined( _WIN32 )
        tcp_keepalive settings{};
        settings.onoff = 1;
        settings.keepalivetime = 15000;
        settings.keepaliveinterval = 3000;

        DWORD bytesReturned = 0;
        if ( WSAIoctl( socket.native_handle(), SIO_KEEPALIVE_VALS, &settings, sizeof( settings ), nullptr, 0, &bytesReturned, nullptr, nullptr ) != 0 ) {
            ERROR_LOG( "[MULTI] Unable to set the keep-alive delays, error " << WSAGetLastError() )
        }
#elif defined( __linux__ )
        const int idleSeconds = 15;
        const int intervalSeconds = 3;
        const int probeCount = 5;

        setsockopt( socket.native_handle(), IPPROTO_TCP, TCP_KEEPIDLE, &idleSeconds, sizeof( idleSeconds ) );
        setsockopt( socket.native_handle(), IPPROTO_TCP, TCP_KEEPINTVL, &intervalSeconds, sizeof( intervalSeconds ) );
        setsockopt( socket.native_handle(), IPPROTO_TCP, TCP_KEEPCNT, &probeCount, sizeof( probeCount ) );
#endif
    }

    void configureSocket( asio::ip::tcp::socket & socket )
    {
        std::error_code ignored;
        socket.set_option( asio::ip::tcp::no_delay( true ), ignored );
        // Room for a whole saved game, sent even if update() is not called for a while.
        socket.set_option( asio::socket_base::send_buffer_size( 4 * 1024 * 1024 ), ignored );
        enableFastKeepAlive( socket );
    }

    bool isTailscaleAddress( const asio::ip::address_v4 & address )
    {
        // Tailscale uses the 100.64.0.0/10 range.
        const asio::ip::address_v4::bytes_type bytes = address.to_bytes();
        return bytes[0] == 100 && ( bytes[1] & 0xC0 ) == 64;
    }

    using Frame = std::shared_ptr<const std::vector<uint8_t>>;

    Frame makeFrame( const Multi::Message & message )
    {
        return std::make_shared<const std::vector<uint8_t>>( Multi::encodeMessage( message ) );
    }

    struct Link;

    // One TCP connection. It owns its buffers, pending operations of a lost connection still use them after another one took over.
    struct Transport
    {
        enum class Phase
        {
            // The connection currently used by its link.
            ACTIVE,
            // Host only. Someone connected, its first message tells who it is.
            CANDIDATE,
            // Guest only. Reconnection attempt waiting for the answer of the host.
            RESUMING
        };

        explicit Transport( asio::io_context & io )
            : socket( io )
        {
            // Do nothing.
        }

        asio::ip::tcp::socket socket;
        Phase phase{ Phase::ACTIVE };

        // The link it belongs to. None for a connection of the host which is not identified yet or waits on the relay.
        Link * link{ nullptr };

        // Host only. The link was dropped: what was sent still leaves, nothing more is read.
        bool isClosing{ false };

        std::array<uint8_t, Multi::messageHeaderSize> header{};
        Multi::Message incoming;

        std::deque<Frame> outgoing;
        size_t outgoingBytes{ 0 };
        bool isWriting{ false };

        // Once the first read started, the next one starts after each message.
        bool keepsReading{ false };
        bool isReading{ false };

        // A write failed: what the other side sent before closing is still read for a moment.
        std::error_code writeError;
        std::chrono::steady_clock::time_point writeFailedAt;

        bool isClosed{ false };
        std::chrono::steady_clock::time_point deadline;

        // Through the relay of a lobby the bytes of the session go in WebSocket messages, read back as a stream.
        std::unique_ptr<Multi::WebSocketClient> webSocket;
        // The relay paired this connection with the other computer.
        bool isRelayReady{ false };
        std::vector<uint8_t> readBuffer;
        std::vector<uint8_t> stream;
        // WebSocket frames which go first: the upgrade request, the first message to the relay, pongs, pings.
        std::deque<std::vector<uint8_t>> rawOutgoing;
        std::chrono::steady_clock::time_point lastPing;
    };

    void queueFrame( const std::shared_ptr<Transport> & connection, Frame frame )
    {
        connection->outgoingBytes += frame->size();
        connection->outgoing.push_back( std::move( frame ) );
    }

    // The connection with one other computer. Its session, numbering and unconfirmed messages outlive the TCP connections.
    struct Link
    {
        Multi::MachineId machine{ Multi::hostMachine };
        Multi::NetworkSession::State state{ Multi::NetworkSession::State::CONNECTING };
        std::string error;

        std::shared_ptr<Transport> transport;
        // Guest only. An attempt to take the lost connection back.
        std::shared_ptr<Transport> pending;

        std::deque<Multi::Message> received;

        // Session ID given by the host. Sent messages are kept until the other side confirms them.
        uint64_t sessionId{ 0 };
        uint64_t sentCount{ 0 };
        uint64_t receivedCount{ 0 };
        uint64_t acknowledgedCount{ 0 };
        std::chrono::steady_clock::time_point lastAcknowledgement;
        std::deque<std::pair<uint64_t, Frame>> unacknowledged;
        size_t unacknowledgedBytes{ 0 };

        std::chrono::steady_clock::time_point lostAt;
        bool isClosedByOther{ false };

        // Host only. Dropped by the game, its number may belong to another link now.
        bool isDropped{ false };

        // Guest only.
        asio::ip::tcp::endpoint hostEndpoint;
        uint32_t attempts{ 0 };
        std::chrono::steady_clock::time_point nextAttempt;
        std::chrono::steady_clock::time_point retryStart;
        bool isRetrying{ false };
        bool isResumeRefused{ false };

        // Used only by self-tests.
        std::chrono::steady_clock::time_point nextTestCut;
    };

    // Computers which are not given a place yet get numbers from here on.
    constexpr Multi::MachineId firstNewcomerNumber{ 16 };

    // More links or connections at once are refused, whatever the game does with them.
    constexpr size_t maxLiveLinks{ 12 };
    constexpr size_t maxCandidates{ 16 };

    // A dropped link gets this time for its last messages.
    constexpr std::chrono::seconds closingTimeout{ 2 };

    // After a failed write, the last messages of the other side are read for this long at most.
    constexpr std::chrono::seconds drainTimeout{ 2 };

    // What a relay connection is for.
    enum class RelayLinkPurpose
    {
        // Host only. Waits on the relay for a guest.
        WAITING,
        // Guest only. The first connection of the session.
        JOINING,
        // Guest only. Takes a lost connection back.
        RESUMING
    };

    // Keeps every frame under the limits of the relay and of the hosting.
    constexpr size_t maxRelayFrameSize{ size_t{ 1024 } * 1024 };

    // The relay and what lies on the way close connections which stay silent.
    constexpr std::chrono::seconds relayPingInterval{ 30 };

    constexpr std::chrono::seconds relayRetryDelay{ 5 };
}

bool Multi::isNetworkAvailable()
{
    return true;
}

std::vector<std::string> Multi::getLocalAddresses()
{
    asio::io_context io;
    asio::ip::tcp::resolver resolver( io );

    std::error_code errorCode;
    const std::string hostName = asio::ip::host_name( errorCode );
    if ( errorCode ) {
        return {};
    }

    const asio::ip::tcp::resolver::results_type results = resolver.resolve( asio::ip::tcp::v4(), hostName, "", errorCode );
    if ( errorCode ) {
        return {};
    }

    std::vector<std::string> tailscaleAddresses;
    std::vector<std::string> otherAddresses;

    for ( const auto & entry : results ) {
        const asio::ip::address address = entry.endpoint().address();
        if ( !address.is_v4() || address.is_loopback() ) {
            continue;
        }

        std::vector<std::string> & addresses = isTailscaleAddress( address.to_v4() ) ? tailscaleAddresses : otherAddresses;
        const std::string text = address.to_string();

        if ( std::find( addresses.begin(), addresses.end(), text ) == addresses.end() ) {
            addresses.push_back( text );
        }
    }

    tailscaleAddresses.insert( tailscaleAddresses.end(), otherAddresses.begin(), otherAddresses.end() );

    return tailscaleAddresses;
}

std::string Multi::getInternetAddressV6()
{
    // The address the system would use to reach a public IPv6 address. Nothing is sent to it.
    asio::io_context io;
    asio::ip::udp::socket socket( io );

    std::error_code errorCode;
    const asio::ip::udp::endpoint publicEndpoint( asio::ip::make_address_v6( "2606:4700:4700::1111", errorCode ), 53 );

    if ( !errorCode ) {
        socket.open( asio::ip::udp::v6(), errorCode );
    }
    if ( !errorCode ) {
        socket.connect( publicEndpoint, errorCode );
    }

    const asio::ip::udp::endpoint local = errorCode ? asio::ip::udp::endpoint() : socket.local_endpoint( errorCode );
    if ( errorCode || !local.address().is_v6() ) {
        return {};
    }

    // Only a global address (2000::/3) can be reached from the internet.
    const asio::ip::address_v6 address = local.address().to_v6();
    if ( ( address.to_bytes()[0] & 0xE0 ) != 0x20 ) {
        return {};
    }

    return address.to_string();
}

bool Multi::isIPv6Address( const std::string & text )
{
    std::error_code errorCode;
    const asio::ip::address_v6 address = asio::ip::make_address_v6( text, errorCode );

    return !errorCode && !address.is_v4_mapped();
}

struct Multi::NetworkSession::Impl
{
    // Must be destroyed last, with the pending operations which keep the connections alive.
    asio::io_context io;
    asio::ip::tcp::acceptor acceptor{ io };
    asio::ip::tcp::resolver resolver{ io };

    bool isHostSide{ false };
    bool isStarted{ false };
    bool isClosed{ false };
    std::string error;
    bool isAddressUnknown{ false };
    std::chrono::steady_clock::time_point connectionDeadline;

    // Every link made by this session, never destroyed before it. A guest has only the link with the host.
    std::vector<std::unique_ptr<Link>> links;
    uint64_t arrivalCount{ 0 };

    // Host only. Connections not identified yet, links of computers the game has not seen yet, and dropped links whose
    // last messages are still leaving.
    std::vector<std::shared_ptr<Transport>> candidates;
    std::deque<Newcomer> newcomers;
    MachineId nextNewcomerNumber{ firstNewcomerNumber };
    std::vector<std::shared_ptr<Transport>> closing;
    bool isAccepting{ false };

    std::string joinedAddress;
    uint16_t joinedPort{ 0 };

    // Host only. Key of the probes of the lobby where the game is published.
    std::optional<LobbyKey> probeKey;

    // Through the relay of a lobby. The host keeps a connection waiting there for a guest. A guest whose session goes
    // through the relay takes a lost connection back the same way.
    struct RelayRoute
    {
        std::string host;
        uint16_t port{ 0 };
        std::string path;
        // Host token or guest ticket.
        LobbyKey key{};
    };

    std::unique_ptr<RelayRoute> relayRoute;
    std::shared_ptr<Transport> relayWaiting;
    std::chrono::steady_clock::time_point relayNextAttempt;
    uint32_t relayJoinAttempts{ 0 };
    // Host only: how many times the lobby took the room.
    uint32_t relayPublishCount{ 0 };

    // Used only by self-tests.
    std::chrono::milliseconds testCutInterval{ 0 };
    uint32_t testRandom{ 2463534242U };

    SessionStats stats;

    uint32_t nextTestRandom()
    {
        testRandom ^= testRandom << 13;
        testRandom ^= testRandom >> 17;
        testRandom ^= testRandom << 5;
        return testRandom;
    }

    // Guest only.
    Link * hostLink() const
    {
        return links.empty() ? nullptr : links.front().get();
    }

    Link * findLink( const MachineId machine ) const
    {
        if ( !isHostSide ) {
            return machine == hostMachine ? hostLink() : nullptr;
        }

        for ( const std::unique_ptr<Link> & link : links ) {
            if ( link->machine == machine && !link->isDropped ) {
                return link.get();
            }
        }

        return nullptr;
    }

    Link * findLinkBySession( const uint64_t sessionId ) const
    {
        for ( const std::unique_ptr<Link> & link : links ) {
            if ( sessionId != 0 && link->sessionId == sessionId && !link->isDropped && link->state != State::CLOSED ) {
                return link.get();
            }
        }

        return nullptr;
    }

    // The link a received message came from.
    Link * findSenderLink( const Message & message ) const
    {
        return isHostSide ? findLink( message.sender ) : hostLink();
    }

    bool isCandidate( const std::shared_ptr<Transport> & connection ) const
    {
        return std::find( candidates.begin(), candidates.end(), connection ) != candidates.end();
    }

    bool isLive( const std::shared_ptr<Transport> & connection ) const
    {
        if ( isClosed || connection->isClosed ) {
            return false;
        }

        if ( connection->isClosing ) {
            return true;
        }

        if ( connection->link != nullptr ) {
            const Link & link = *connection->link;
            return !link.isDropped && link.state != State::CLOSED && ( connection == link.transport || connection == link.pending );
        }

        return connection == relayWaiting || isCandidate( connection );
    }

    // Every connection still in use.
    std::vector<std::shared_ptr<Transport>> getConnections() const
    {
        std::vector<std::shared_ptr<Transport>> connections = candidates;
        connections.insert( connections.end(), closing.begin(), closing.end() );

        if ( relayWaiting ) {
            connections.push_back( relayWaiting );
        }

        for ( const std::unique_ptr<Link> & link : links ) {
            if ( link->isDropped || link->state == State::CLOSED ) {
                continue;
            }
            if ( link->transport ) {
                connections.push_back( link->transport );
            }
            if ( link->pending ) {
                connections.push_back( link->pending );
            }
        }

        return connections;
    }

    static void closeTransport( const std::shared_ptr<Transport> & connection )
    {
        if ( !connection || connection->isClosed ) {
            return;
        }

        connection->isClosed = true;

        std::error_code ignored;
        connection->socket.shutdown( asio::ip::tcp::socket::shutdown_both, ignored );
        connection->socket.close( ignored );
    }

    // Closes the whole session.
    void fail( std::string reason )
    {
        if ( isClosed ) {
            return;
        }

        if ( !reason.empty() ) {
            ERROR_LOG( "[MULTI] Network session closed: " << reason )
        }

        isClosed = true;
        error = std::move( reason );

        std::error_code ignored;
        acceptor.close( ignored );
        resolver.cancel();

        for ( const std::unique_ptr<Link> & link : links ) {
            closeTransport( link->transport );
            closeTransport( link->pending );
            link->pending.reset();

            if ( link->state != State::CLOSED ) {
                link->state = State::CLOSED;
                link->error = error;
            }
        }

        for ( const std::vector<std::shared_ptr<Transport>> * list : { &candidates, &closing } ) {
            for ( const std::shared_ptr<Transport> & connection : *list ) {
                closeTransport( connection );
            }
        }
        candidates.clear();
        closing.clear();

        closeTransport( relayWaiting );
        relayWaiting.reset();
    }

    // The link is closed for good. On a guest this is the whole session.
    void failLink( Link & link, std::string reason )
    {
        if ( !isHostSide ) {
            fail( std::move( reason ) );
            return;
        }

        if ( link.state == State::CLOSED ) {
            return;
        }

        if ( !reason.empty() ) {
            ERROR_LOG( "[MULTI] Connection with computer " << static_cast<int>( link.machine ) << " closed: " << reason )
        }

        link.state = State::CLOSED;
        link.error = std::move( reason );

        closeTransport( link.transport );
        closeTransport( link.pending );
        link.pending.reset();
    }

    // Host only. The game no longer wants this link, its messages are dropped.
    static void dropLink( Link & link )
    {
        closeTransport( link.transport );
        closeTransport( link.pending );
        link.pending.reset();
        link.received.clear();
        link.state = State::CLOSED;
        link.isDropped = true;
    }

    // Before the session exists (connecting, handshake) any failure closes it, there is nothing to take back. Guest only.
    void onError( const std::error_code & errorCode )
    {
        // Cancelled by close().
        if ( errorCode == asio::error::operation_aborted ) {
            return;
        }

        isAddressUnknown = ( errorCode == asio::error::host_not_found || errorCode == asio::error::host_not_found_try_again );
        fail( describeError( errorCode ) );
    }

    // Guest only. The first connection with the host. Its reading has already started.
    void onConnected( const std::shared_ptr<Transport> & connection )
    {
        Link & link = *connection->link;
        link.transport = connection;
        connection->phase = Transport::Phase::ACTIVE;
        link.state = State::CONNECTED;

        configureSocket( connection->socket );

        std::error_code ignored;
        link.hostEndpoint = connection->socket.remote_endpoint( ignored );

        // Messages sent before connecting.
        for ( const auto & entry : link.unacknowledged ) {
            queueFrame( connection, entry.second );
        }

        writeNext( connection );
    }

    void removeCandidate( const std::shared_ptr<Transport> & connection )
    {
        candidates.erase( std::remove( candidates.begin(), candidates.end(), connection ), candidates.end() );
    }

    void dropCandidate( const std::shared_ptr<Transport> & connection )
    {
        closeTransport( connection );
        removeCandidate( connection );
    }

    std::optional<MachineId> takeNewcomerNumber()
    {
        const size_t liveLinks = static_cast<size_t>(
            std::count_if( links.begin(), links.end(), []( const std::unique_ptr<Link> & link ) { return !link->isDropped && link->state != State::CLOSED; } ) );
        if ( liveLinks >= maxLiveLinks ) {
            return {};
        }

        for ( int attempt = 0; attempt < 256; ++attempt ) {
            const MachineId number = nextNewcomerNumber;
            nextNewcomerNumber = ( nextNewcomerNumber == 255 ) ? firstNewcomerNumber : static_cast<MachineId>( nextNewcomerNumber + 1 );

            if ( findLink( number ) == nullptr ) {
                return number;
            }
        }

        return {};
    }

    // Host only. A computer the game has not seen yet: a new link with a temporary number. Its first message is the next
    // one received from it.
    void addNewcomer( const std::shared_ptr<Transport> & connection, Message message )
    {
        removeCandidate( connection );

        const std::optional<MachineId> number = takeNewcomerNumber();
        if ( !number ) {
            VERBOSE_LOG( "[MULTI] A connection was refused: too many computers at once" )
            closeTransport( connection );
            return;
        }

        links.push_back( std::make_unique<Link>() );
        Link & link = *links.back();

        link.machine = *number;
        link.state = State::CONNECTED;
        link.transport = connection;
        link.sessionId = generateSessionId();
        connection->link = &link;
        connection->phase = Transport::Phase::ACTIVE;

        queueFrame( connection, makeFrame( makeSessionId( link.sessionId ) ) );

        // Counted as received, given with the newcomer.
        ++link.receivedCount;
        message.sender = *number;
        newcomers.push_back( { *number, std::move( message ) } );

        writeNext( connection );
    }

    // A finished read or write lets the next one start from here, never from its own handler.
    void continueTransfers()
    {
        for ( const std::shared_ptr<Transport> & connection : getConnections() ) {
            if ( !isLive( connection ) ) {
                continue;
            }

            if ( connection->keepsReading && !connection->isReading && !connection->isClosing ) {
                readNext( connection );
            }

            writeNext( connection );
        }
    }

    void writeNext( const std::shared_ptr<Transport> & connection )
    {
        if ( !connection || connection->isWriting || connection->writeError || !isLive( connection ) ) {
            return;
        }

        // By default asio writes 64 KB at a time and needs update() calls to continue. Give the whole message to the system at once.
        const auto writeAllAtOnce
            = []( const std::error_code & errorCode, size_t /* bytes */ ) -> size_t { return errorCode ? 0 : messageHeaderSize + maxMessagePayloadSize; };

        // WebSocket frames of the connection itself go first.
        if ( !connection->rawOutgoing.empty() ) {
            connection->isWriting = true;
            auto bytes = std::make_shared<const std::vector<uint8_t>>( std::move( connection->rawOutgoing.front() ) );
            connection->rawOutgoing.pop_front();

            asio::async_write( connection->socket, asio::buffer( *bytes ), writeAllAtOnce,
                               [this, connection, bytes]( const std::error_code & errorCode, size_t /* bytes */ ) {
                                   connection->isWriting = false;

                                   if ( isLive( connection ) && errorCode ) {
                                       onWriteError( connection, errorCode );
                                   }
                               } );
            return;
        }

        // Through the relay, nothing of the session goes before the pairing.
        if ( connection->outgoing.empty() || ( connection->webSocket && !connection->isRelayReady ) ) {
            return;
        }

        connection->isWriting = true;

        const Frame frame = connection->outgoing.front();

        // Through the relay, the bytes of the session go in WebSocket messages of 1 MB at most.
        Frame bytes = frame;
        if ( connection->webSocket ) {
            auto wrapped = std::make_shared<std::vector<uint8_t>>();
            for ( size_t offset = 0; offset < frame->size(); offset += maxRelayFrameSize ) {
                const std::vector<uint8_t> part = connection->webSocket->makeMessage( frame->data() + offset, std::min( maxRelayFrameSize, frame->size() - offset ) );
                wrapped->insert( wrapped->end(), part.begin(), part.end() );
            }
            bytes = std::move( wrapped );
        }

        asio::async_write( connection->socket, asio::buffer( *bytes ), writeAllAtOnce,
                           [this, connection, frame, bytes]( const std::error_code & errorCode, size_t /* bytes */ ) {
                               connection->isWriting = false;

                               if ( !isLive( connection ) ) {
                                   return;
                               }
                               if ( errorCode ) {
                                   onWriteError( connection, errorCode );
                                   return;
                               }

                               connection->outgoingBytes -= frame->size();
                               connection->outgoing.pop_front();
                           } );
    }

    void readNext( const std::shared_ptr<Transport> & connection )
    {
        if ( connection->webSocket ) {
            readWebSocket( connection );
        }
        else {
            readHeader( connection );
        }
    }

    void readWebSocket( const std::shared_ptr<Transport> & connection )
    {
        connection->keepsReading = true;
        connection->isReading = true;
        connection->readBuffer.resize( 16384 );

        connection->socket.async_read_some( asio::buffer( connection->readBuffer ), [this, connection]( const std::error_code & errorCode, const size_t size ) {
            if ( !isLive( connection ) ) {
                return;
            }

            connection->isReading = false;

            if ( errorCode ) {
                onTransportError( connection, errorCode );
                return;
            }

            onWebSocketData( connection, size );
        } );
    }

    void onWebSocketData( const std::shared_ptr<Transport> & connection, const size_t size )
    {
        WebSocketClient & webSocket = *connection->webSocket;
        const bool wasOpen = webSocket.isOpen();

        if ( !webSocket.receive( connection->readBuffer.data(), size ) ) {
            VERBOSE_LOG( "[MULTI] Wrong answer of the relay of the lobby: " << webSocket.error() )
            onRelayLinkFailed( connection, {} );
            return;
        }

        if ( std::vector<uint8_t> replies = webSocket.takeReplies(); !replies.empty() ) {
            connection->rawOutgoing.push_back( std::move( replies ) );
        }
        (void)webSocket.takePongs();

        // The relay accepted the connection: tell it who this is.
        if ( !wasOpen && webSocket.isOpen() ) {
            const std::vector<uint8_t> hello = encodeMessage( makeLobbyKeyMessage( isHostSide ? MessageType::RELAY_HOST : MessageType::RELAY_GUEST, relayRoute->key ) );
            connection->rawOutgoing.push_back( webSocket.makeMessage( hello.data(), hello.size() ) );
        }

        while ( std::optional<std::vector<uint8_t>> data = webSocket.takeMessage() ) {
            if ( connection->isRelayReady ) {
                connection->stream.insert( connection->stream.end(), data->begin(), data->end() );
                continue;
            }

            // Before the pairing, messages of the relay itself.
            Message message;
            uint32_t payloadSize = 0;
            if ( data->size() < messageHeaderSize || !decodeMessageHeader( data->data(), message.type, payloadSize )
                 || payloadSize != data->size() - messageHeaderSize ) {
                onRelayLinkFailed( connection, {} );
                return;
            }
            message.payload.assign( data->begin() + messageHeaderSize, data->end() );

            if ( readEmptyLobbyMessage( message, MessageType::RELAY_READY ) ) {
                connection->isRelayReady = true;
                onRelayReady( connection );
                if ( !isLive( connection ) ) {
                    return;
                }
                continue;
            }

            onRelayLinkFailed( connection, readRelayRefused( message ) );
            return;
        }

        // Whole messages of the session, from the stream.
        size_t used = 0;
        while ( connection->stream.size() - used >= messageHeaderSize && isLive( connection ) ) {
            uint32_t payloadSize = 0;
            if ( !decodeMessageHeader( connection->stream.data() + used, connection->incoming.type, payloadSize ) ) {
                onInvalidData( connection );
                return;
            }
            if ( connection->stream.size() - used - messageHeaderSize < payloadSize ) {
                break;
            }

            const auto payloadStart = connection->stream.begin() + static_cast<std::ptrdiff_t>( used + messageHeaderSize );
            connection->incoming.payload.assign( payloadStart, payloadStart + payloadSize );
            used += messageHeaderSize + payloadSize;

            onFrame( connection );
        }
        connection->stream.erase( connection->stream.begin(), connection->stream.begin() + static_cast<std::ptrdiff_t>( used ) );

        if ( webSocket.isClosedByServer() && isLive( connection ) ) {
            // The other computer left the relay, or the room is gone.
            onTransportError( connection, asio::error::eof );
        }
    }

    // The relay paired this connection with the other computer.
    void onRelayReady( const std::shared_ptr<Transport> & connection )
    {
        if ( connection == relayWaiting ) {
            // A computer came through the relay: as if it connected to the port of the game. Another connection waits for the next one.
            VERBOSE_LOG( "[MULTI] A computer comes through the relay of the lobby" )

            relayWaiting.reset();
            relayNextAttempt = std::chrono::steady_clock::now();

            connection->phase = Transport::Phase::CANDIDATE;
            connection->deadline = std::chrono::steady_clock::now() + reconnectionAttemptTimeout;
            candidates.push_back( connection );
            return;
        }

        const Link & link = *connection->link;

        if ( connection->phase == Transport::Phase::RESUMING ) {
            queueFrame( connection, makeFrame( makeSessionResume( { link.sessionId, link.receivedCount } ) ) );
            writeNext( connection );
            return;
        }

        VERBOSE_LOG( "[MULTI] Connected to the host through the relay of the lobby" )
        relayJoinAttempts = 0;
        onConnected( connection );
    }

    void onRelayLinkFailed( const std::shared_ptr<Transport> & connection, const std::optional<RelayRefusal> refusal )
    {
        const auto now = std::chrono::steady_clock::now();

        if ( connection == relayWaiting ) {
            if ( refusal ) {
                ERROR_LOG( "[MULTI] The relay of the lobby refused to wait for a guest: " << describeRelayRefusal( refusal ) )
            }

            // After a restart of the lobby, the room is soon back.
            closeTransport( connection );
            relayWaiting.reset();
            relayNextAttempt = now + ( refusal == RelayRefusal::LIMIT_REACHED ? relayRetryDelay * 6 : relayRetryDelay );
            return;
        }

        if ( isCandidate( connection ) ) {
            dropCandidate( connection );
            return;
        }

        Link * link = connection->link;

        if ( link != nullptr && connection == link->pending ) {
            VERBOSE_LOG( "[MULTI] The relay of the lobby did not take the connection back: " << describeRelayRefusal( refusal ) )

            // The host left on purpose: no need to wait for it.
            if ( refusal == RelayRefusal::ROOM_CLOSED ) {
                link->isClosedByOther = true;
                fail( gettext_noop( "the other player closed the connection" ) );
                return;
            }

            dropPending( *link );
            return;
        }

        if ( link != nullptr && connection == link->transport && link->state == State::CONNECTING ) {
            // The host may be between two connections waiting on the relay.
            if ( refusal == RelayRefusal::NO_HOST_WAITING && relayJoinAttempts < 10 ) {
                closeTransport( connection );
                link->transport.reset();
                relayNextAttempt = now + std::chrono::milliseconds( 300 );
                return;
            }

            fail( describeRelayRefusal( refusal ) );
            return;
        }

        onInvalidData( connection );
    }

    static std::string describeRelayRefusal( const std::optional<RelayRefusal> refusal )
    {
        if ( refusal == RelayRefusal::NO_SUCH_ROOM ) {
            return gettext_noop( "the game is not on the lobby any more" );
        }
        if ( refusal == RelayRefusal::NO_HOST_WAITING ) {
            return gettext_noop( "the host does not wait on the relay of the lobby" );
        }
        if ( refusal == RelayRefusal::LIMIT_REACHED ) {
            return gettext_noop( "the relay of the lobby is busy, try again later" );
        }
        if ( refusal == RelayRefusal::ROOM_CLOSED ) {
            return gettext_noop( "the host closed the game" );
        }

        return gettext_noop( "the relay of the lobby refused the connection" );
    }

    void openRelayLink( const RelayLinkPurpose purpose )
    {
        auto connection = std::make_shared<Transport>( io );
        connection->webSocket = std::make_unique<WebSocketClient>( relayRoute->host, relayRoute->port, relayRoute->path, maxRelayFrameSize );
        connection->deadline = std::chrono::steady_clock::now() + reconnectionAttemptTimeout;

        switch ( purpose ) {
        case RelayLinkPurpose::WAITING:
            VERBOSE_LOG( "[MULTI] A connection waits for the other computers on the relay of the lobby" )
            relayWaiting = connection;
            break;
        case RelayLinkPurpose::JOINING: {
            Link & link = *hostLink();
            connection->link = &link;
            link.transport = connection;
            ++relayJoinAttempts;
            break;
        }
        case RelayLinkPurpose::RESUMING: {
            Link & link = *hostLink();
            connection->link = &link;
            connection->phase = Transport::Phase::RESUMING;
            link.pending = connection;
            ++link.attempts;
            break;
        }
        default:
            break;
        }

        const auto onConnect = [this, connection]( const std::error_code & connectError ) {
            if ( !isLive( connection ) ) {
                return;
            }
            if ( connectError ) {
                onTransportError( connection, connectError );
                return;
            }

            configureSocket( connection->socket );
            connection->lastPing = std::chrono::steady_clock::now();
            connection->rawOutgoing.push_back( connection->webSocket->makeRequest() );
            readNext( connection );
            writeNext( connection );
        };

        std::error_code addressError;
        const asio::ip::address numericAddress = asio::ip::make_address( relayRoute->host, addressError );
        if ( !addressError ) {
            connection->socket.async_connect( asio::ip::tcp::endpoint( numericAddress, relayRoute->port ), onConnect );
            return;
        }

        resolver.async_resolve( relayRoute->host, std::to_string( relayRoute->port ),
                                [this, connection, onConnect]( const std::error_code & resolveError, const asio::ip::tcp::resolver::results_type & endpoints ) {
                                    if ( !isLive( connection ) ) {
                                        return;
                                    }
                                    if ( resolveError ) {
                                        onTransportError( connection, resolveError );
                                        return;
                                    }

                                    asio::async_connect( connection->socket, endpoints,
                                                         [onConnect]( const std::error_code & connectError, const asio::ip::tcp::endpoint & /* endpoint */ ) {
                                                             onConnect( connectError );
                                                         } );
                                } );
    }

    // A connection waiting on the relay for a guest, the next try to join through it, and pings on relay connections.
    void updateRelay( const std::chrono::steady_clock::time_point now )
    {
        if ( !relayRoute || isClosed ) {
            return;
        }

        if ( isHostSide && !relayWaiting && now >= relayNextAttempt ) {
            openRelayLink( RelayLinkPurpose::WAITING );
        }

        if ( const Link * link = hostLink(); !isHostSide && link != nullptr && link->state == State::CONNECTING && !link->transport && now >= relayNextAttempt ) {
            openRelayLink( RelayLinkPurpose::JOINING );
        }

        for ( const std::shared_ptr<Transport> & connection : getConnections() ) {
            if ( connection->webSocket && connection->webSocket->isOpen() && isLive( connection ) && now - connection->lastPing >= relayPingInterval ) {
                connection->rawOutgoing.push_back( connection->webSocket->makePing() );
                connection->lastPing = now;
                writeNext( connection );
            }
        }
    }

    void readHeader( const std::shared_ptr<Transport> & connection )
    {
        connection->keepsReading = true;
        connection->isReading = true;

        asio::async_read( connection->socket, asio::buffer( connection->header ), [this, connection]( const std::error_code & errorCode, size_t /* bytes */ ) {
            if ( !isLive( connection ) ) {
                return;
            }
            if ( errorCode ) {
                connection->isReading = false;
                onTransportError( connection, errorCode );
                return;
            }

            uint32_t payloadSize = 0;
            if ( !decodeMessageHeader( connection->header.data(), connection->incoming.type, payloadSize ) ) {
                connection->isReading = false;
                onInvalidData( connection );
                return;
            }

            connection->incoming.payload.resize( payloadSize );

            if ( payloadSize == 0 ) {
                connection->isReading = false;
                onFrame( connection );
                return;
            }

            readPayload( connection );
        } );
    }

    void readPayload( const std::shared_ptr<Transport> & connection )
    {
        asio::async_read( connection->socket, asio::buffer( connection->incoming.payload ), [this, connection]( const std::error_code & errorCode, size_t /* bytes */ ) {
            if ( !isLive( connection ) ) {
                return;
            }

            connection->isReading = false;

            if ( errorCode ) {
                onTransportError( connection, errorCode );
                return;
            }

            onFrame( connection );
        } );
    }

    void onFrame( const std::shared_ptr<Transport> & connection )
    {
        Message message = std::move( connection->incoming );
        connection->incoming = {};

        // The game dropped this computer.
        if ( connection->isClosing ) {
            return;
        }

        switch ( connection->phase ) {
        case Transport::Phase::CANDIDATE:
            onCandidateMessage( connection, std::move( message ) );
            break;
        case Transport::Phase::RESUMING:
            onResumeReply( *connection->link, connection, message );
            break;
        default:
            onSessionMessage( *connection->link, connection, std::move( message ) );
            break;
        }
    }

    void deliver( Link & link, Message message )
    {
        message.arrival = ++arrivalCount;
        link.received.push_back( std::move( message ) );
    }

    // Kept until the other side confirms it, sent now if the link is connected.
    static void sendOnLink( Link & link, const Frame & frame )
    {
        ++link.sentCount;
        link.unacknowledged.emplace_back( link.sentCount, frame );
        link.unacknowledgedBytes += frame->size();

        if ( link.state == State::CONNECTED && link.transport ) {
            queueFrame( link.transport, frame );
        }
    }

    void onSessionMessage( Link & link, const std::shared_ptr<Transport> & connection, Message message )
    {
        switch ( message.type ) {
        case MessageType::SESSION_ID: {
            const std::optional<uint64_t> id = readSessionId( message );
            if ( isHostSide || !id || *id == 0 ) {
                onInvalidData( connection );
                return;
            }

            link.sessionId = *id;
            return;
        }
        case MessageType::SESSION_ACK: {
            const std::optional<uint64_t> count = readSessionAck( message );
            if ( !count || *count > link.sentCount ) {
                onInvalidData( connection );
                return;
            }

            onAcknowledged( link, *count );
            return;
        }
        case MessageType::SESSION_CLOSE:
            link.isClosedByOther = true;
            failLink( link, gettext_noop( "the other player closed the connection" ) );
            return;
        case MessageType::SESSION_RESUME:
        case MessageType::SESSION_RESUME_REPLY:
            onInvalidData( connection );
            return;
        default:
            break;
        }

        // This is a message for the game.
        ++link.receivedCount;

        if ( message.type != MessageType::ROUTED ) {
            message.sender = isHostSide ? link.machine : hostMachine;
            deliver( link, std::move( message ) );
            return;
        }

        std::optional<RoutedMessage> routed = readRouted( message );
        if ( !routed || ( isHostSide && ( link.machine == hostMachine || link.machine > maxMachineId ) ) ) {
            onInvalidData( connection );
            return;
        }

        if ( !isHostSide ) {
            routed->inner.sender = routed->sender;
            deliver( link, std::move( routed->inner ) );
            return;
        }

        // Carried to the other guests, and kept here when the host is one of the targets. Writing starts at the next transfer.
        routed->sender = link.machine;

        for ( MachineId target = hostMachine + 1; target <= maxMachineId; ++target ) {
            if ( target == link.machine || ( routed->targets & machineBit( target ) ) == 0 ) {
                continue;
            }

            if ( Link * targetLink = findLink( target ); targetLink != nullptr && targetLink->state != State::CLOSED ) {
                sendOnLink( *targetLink, makeFrame( makeRouted( *routed ) ) );
            }
        }

        if ( ( routed->targets & machineBit( hostMachine ) ) != 0 ) {
            routed->inner.sender = link.machine;
            deliver( link, std::move( routed->inner ) );
        }
    }

    static void onAcknowledged( Link & link, const uint64_t count )
    {
        while ( !link.unacknowledged.empty() && link.unacknowledged.front().first <= count ) {
            link.unacknowledgedBytes -= link.unacknowledged.front().second->size();
            link.unacknowledged.pop_front();
        }
    }

    // Checks if all the messages the other side missed are still kept here.
    static bool canResumeFrom( const Link & link, const uint64_t peerReceivedCount )
    {
        const uint64_t oldestKept = link.unacknowledged.empty() ? link.sentCount + 1 : link.unacknowledged.front().first;
        return peerReceivedCount <= link.sentCount && peerReceivedCount + 1 >= oldestKept;
    }

    // A lobby checks that this game can be reached directly. Only a probe with the key given to the lobby is answered.
    void answerProbe( const std::shared_ptr<Transport> & connection, const Message & message )
    {
        const std::optional<LobbyProbe> probe = readLobbyProbe( message );
        if ( probe && probeKey && probe->probeKey == *probeKey ) {
            std::error_code ignored;
            const Frame reply = makeFrame( makeLobbyKeyMessage( MessageType::LOBBY_PROBE_REPLY, probe->value ) );
            asio::write( connection->socket, asio::buffer( *reply ), ignored );

            VERBOSE_LOG( "[MULTI] The lobby checked that this game can be reached directly" )
        }

        dropCandidate( connection );
    }

    // Host only. Processes the first message of someone who connected.
    void onCandidateMessage( const std::shared_ptr<Transport> & connection, Message message )
    {
        if ( message.type == MessageType::LOBBY_PROBE ) {
            answerProbe( connection, message );
            return;
        }

        if ( message.type == MessageType::SESSION_RESUME ) {
            const std::optional<SessionResume> resume = readSessionResume( message );
            Link * link = resume ? findLinkBySession( resume->sessionId ) : nullptr;

            if ( link == nullptr || !canResumeFrom( *link, resume->receivedCount ) ) {
                VERBOSE_LOG( "[MULTI] A connection that does not belong to this game was refused" )

                std::error_code ignored;
                const Frame refusal = makeFrame( makeSessionResumeReply( { false, 0 } ) );
                asio::write( connection->socket, asio::buffer( *refusal ), ignored );

                dropCandidate( connection );
                return;
            }

            removeCandidate( connection );
            connection->link = link;
            queueFrame( connection, makeFrame( makeSessionResumeReply( { true, link->receivedCount } ) ) );
            takeOver( *link, connection, resume->receivedCount );
            return;
        }

        if ( isSessionMessage( message.type ) ) {
            dropCandidate( connection );
            return;
        }

        // Someone new, or a computer which joins again after a restart: the game decides.
        addNewcomer( connection, std::move( message ) );
    }

    // Guest only. Answer of the host to a reconnection attempt.
    void onResumeReply( Link & link, const std::shared_ptr<Transport> & connection, const Message & message )
    {
        const std::optional<SessionResumeReply> reply = readSessionResumeReply( message );
        if ( !reply ) {
            dropPending( link );
            return;
        }

        if ( !reply->isAccepted || !canResumeFrom( link, reply->receivedCount ) ) {
            link.isResumeRefused = true;
            fail( gettext_noop( "the other player does not know this game any more" ) );
            return;
        }

        takeOver( link, connection, reply->receivedCount );
    }

    // Both sides agreed on this connection. Sends again what the other side missed.
    void takeOver( Link & link, const std::shared_ptr<Transport> & connection, const uint64_t peerReceivedCount )
    {
        const bool wasConnected = ( link.state == State::CONNECTED );
        const auto lostTime = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - link.lostAt ).count();

        // The previous connection went silent but is not known as lost yet.
        if ( link.transport && link.transport != connection ) {
            closeTransport( link.transport );
        }

        link.transport = connection;
        if ( link.pending == connection ) {
            link.pending.reset();
        }
        connection->phase = Transport::Phase::ACTIVE;
        link.state = State::CONNECTED;
        link.attempts = 0;
        link.isRetrying = false;

        onAcknowledged( link, peerReceivedCount );

        for ( const auto & entry : link.unacknowledged ) {
            queueFrame( connection, entry.second );
        }

        ++stats.connectionsTakenBack;
        stats.messagesSentAgain += link.unacknowledged.size();

        if ( wasConnected ) {
            VERBOSE_LOG( "[MULTI] Connection replaced (the previous one had gone silent), " << link.unacknowledged.size() << " message(s) sent again" )
        }
        else {
            VERBOSE_LOG( "[MULTI] Connection taken back after " << lostTime << " ms, " << link.unacknowledged.size() << " message(s) sent again" )
        }

        writeNext( connection );
    }

    // Guest only.
    static void dropPending( Link & link )
    {
        closeTransport( link.pending );
        link.pending.reset();
        link.nextAttempt = std::chrono::steady_clock::now() + getRetryDelay( link.attempts );
    }

    void onInvalidData( const std::shared_ptr<Transport> & connection )
    {
        if ( isCandidate( connection ) ) {
            dropCandidate( connection );
            return;
        }

        Link * link = connection->link;
        if ( link == nullptr ) {
            closeTransport( connection );
            return;
        }

        if ( connection == link->pending ) {
            dropPending( *link );
            return;
        }

        // Nothing that comes next on this connection can be trusted.
        failLink( *link, gettext_noop( "invalid data received" ) );
    }

    // The other side may have closed right after its last messages, a game end for instance: they arrived and are read
    // first, the reading tells when the connection is really gone.
    void onWriteError( const std::shared_ptr<Transport> & connection, const std::error_code & errorCode )
    {
        if ( errorCode != asio::error::operation_aborted && connection->keepsReading && !connection->isClosing && connection->link != nullptr
             && connection->link->transport == connection ) {
            connection->writeError = errorCode;
            connection->writeFailedAt = std::chrono::steady_clock::now();
            return;
        }

        onTransportError( connection, errorCode );
    }

    void onTransportError( const std::shared_ptr<Transport> & connection, const std::error_code & errorCode )
    {
        if ( errorCode == asio::error::operation_aborted ) {
            return;
        }

        if ( connection->isClosing ) {
            closeTransport( connection );
            return;
        }

        if ( connection == relayWaiting ) {
            onRelayLinkFailed( connection, {} );
            return;
        }

        if ( isCandidate( connection ) ) {
            dropCandidate( connection );
            return;
        }

        Link * link = connection->link;
        if ( link == nullptr ) {
            closeTransport( connection );
            return;
        }

        if ( connection == link->pending ) {
            dropPending( *link );
            return;
        }

        if ( link->sessionId == 0 && connection->webSocket ) {
            fail( std::string( gettext_noop( "unable to reach the relay of the lobby" ) ) + ": " + describeError( errorCode ) );
            return;
        }

        if ( link->sessionId == 0 ) {
            onError( errorCode );
            return;
        }

        lose( *link, describeError( errorCode ) );
    }

    void lose( Link & link, const std::string & reason )
    {
        closeTransport( link.transport );
        link.transport.reset();

        link.state = State::RECONNECTING;
        link.lostAt = std::chrono::steady_clock::now();
        link.error = reason;
        ++stats.connectionsLost;

        if ( isHostSide ) {
            VERBOSE_LOG( "[MULTI] Connection with computer " << static_cast<int>( link.machine ) << " lost (" << reason << "), waiting for it to come back" )
            return;
        }

        VERBOSE_LOG( "[MULTI] Connection lost (" << reason << "), connecting again" )

        link.attempts = 0;
        link.nextAttempt = link.lostAt;
        link.retryStart = link.lostAt;
        link.isRetrying = true;
    }

    void cutForTest( Link & link, const std::chrono::milliseconds outage )
    {
        if ( link.state != State::CONNECTED || !link.transport || link.sessionId == 0 ) {
            return;
        }

        VERBOSE_LOG( "[MULTI] Self-test: connection cut" )

        lose( link, "connection cut for a self-test" );

        if ( !isHostSide ) {
            link.nextAttempt = std::chrono::steady_clock::now() + outage;
        }
    }

    // Opens the port of the game. On an IPv6 address the socket takes IPv4 connections too.
    bool listen( const asio::ip::tcp::endpoint & endpoint, std::error_code & errorCode )
    {
        acceptor.open( endpoint.protocol(), errorCode );

        if ( !errorCode && endpoint.address().is_v6() ) {
            acceptor.set_option( asio::ip::v6_only( false ), errorCode );
        }

#if !defined( _WIN32 )
        // On Windows this option would let another program take the same port.
        if ( !errorCode ) {
            acceptor.set_option( asio::socket_base::reuse_address( true ), errorCode );
        }
#endif

        if ( !errorCode ) {
            acceptor.bind( endpoint, errorCode );
        }
        if ( !errorCode ) {
            // Several guests may connect at the same moment.
            acceptor.listen( static_cast<int>( maxCandidates ), errorCode );
        }

        if ( errorCode ) {
            std::error_code ignored;
            acceptor.close( ignored );
            return false;
        }

        return true;
    }

    void acceptNext()
    {
        auto connection = std::make_shared<Transport>( io );

        acceptor.async_accept( connection->socket, [this, connection]( const std::error_code & acceptError ) { onAccepted( connection, acceptError ); } );
    }

    void onAccepted( const std::shared_ptr<Transport> & connection, const std::error_code & acceptError )
    {
        if ( isClosed || acceptError == asio::error::operation_aborted ) {
            return;
        }

        if ( acceptError ) {
            // The links still work, but nobody can join or come back.
            ERROR_LOG( "[MULTI] The port of the game no longer accepts connections: " << describeError( acceptError ) )
            isAccepting = false;
            return;
        }

        // Someone connects, its first message tells who it is.
        configureSocket( connection->socket );

        if ( candidates.size() >= maxCandidates ) {
            dropCandidate( candidates.front() );
        }

        connection->phase = Transport::Phase::CANDIDATE;
        connection->deadline = std::chrono::steady_clock::now() + reconnectionAttemptTimeout;
        candidates.push_back( connection );

        readHeader( connection );
        acceptNext();
    }

    // Guest only. Tries to take the lost connection back.
    void startAttempt()
    {
        if ( relayRoute ) {
            openRelayLink( RelayLinkPurpose::RESUMING );
            return;
        }

        Link & link = *hostLink();

        auto connection = std::make_shared<Transport>( io );
        connection->phase = Transport::Phase::RESUMING;
        connection->link = &link;
        connection->deadline = std::chrono::steady_clock::now() + reconnectionAttemptTimeout;

        link.pending = connection;
        ++link.attempts;

        connection->socket.async_connect( link.hostEndpoint, [this, connection]( const std::error_code & connectError ) {
            if ( !isLive( connection ) ) {
                return;
            }

            Link & resumed = *connection->link;
            if ( connectError ) {
                dropPending( resumed );
                return;
            }

            configureSocket( connection->socket );
            queueFrame( connection, makeFrame( makeSessionResume( { resumed.sessionId, resumed.receivedCount } ) ) );

            readHeader( connection );
            writeNext( connection );
        } );
    }

    Link & addHostLink()
    {
        links.push_back( std::make_unique<Link>() );
        return *links.back();
    }

    // Sends the goodbye on every connected link and gives it a moment to leave.
    void sayGoodbye()
    {
        std::vector<std::shared_ptr<Transport>> connections;

        for ( const std::unique_ptr<Link> & link : links ) {
            if ( link->isDropped || link->state != State::CONNECTED || !link->transport ) {
                continue;
            }

            queueFrame( link->transport, makeFrame( { MessageType::SESSION_CLOSE, {} } ) );
            writeNext( link->transport );
            connections.push_back( link->transport );
        }

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 1 );
        const auto isSending = [this, &connections]() {
            return std::any_of( connections.begin(), connections.end(),
                                [this]( const std::shared_ptr<Transport> & connection ) { return isLive( connection ) && !connection->outgoing.empty(); } );
        };

        while ( isSending() && std::chrono::steady_clock::now() < deadline ) {
            io.restart();
            io.poll();
            continueTransfers();
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    }

    // Received messages of all links, the one with the lowest arrival among those the filter accepts first in their queue.
    // Computers not given their place yet are left aside.
    std::optional<Message> take( const std::function<bool( const Message & message )> & filter, const bool isBarrierRespected )
    {
        Link * bestLink = nullptr;
        std::deque<Message>::iterator best;

        for ( const std::unique_ptr<Link> & link : links ) {
            if ( link->isDropped || ( isHostSide && link->machine > maxMachineId ) ) {
                continue;
            }

            for ( auto iter = link->received.begin(); iter != link->received.end(); ++iter ) {
                if ( filter( *iter ) ) {
                    if ( bestLink == nullptr || iter->arrival < best->arrival ) {
                        bestLink = link.get();
                        best = iter;
                    }
                    break;
                }

                if ( isBarrierRespected && isOrderBarrier( iter->type ) ) {
                    break;
                }
            }
        }

        if ( bestLink == nullptr ) {
            return {};
        }

        Message message = std::move( *best );
        bestLink->received.erase( best );

        return message;
    }
};

Multi::NetworkSession::NetworkSession()
    : _impl( std::make_unique<Impl>() )
{}

Multi::NetworkSession::~NetworkSession()
{
    try {
        close();
    }
    catch ( ... ) {
        // Nothing may leave a destructor: the connections are dropped without a goodbye, the other computers notice it by themselves.
        _impl.reset();
    }
}

bool Multi::NetworkSession::host( const uint16_t port, const bool localOnly )
{
    Impl & impl = *_impl;
    assert( !impl.isStarted );

    impl.isStarted = true;
    impl.isHostSide = true;

    // Over the network, IPv6 and IPv4 connections come to the same port. Without IPv6 on this computer, IPv4 only.
    std::error_code errorCode;
    bool isListening = !localOnly && impl.listen( asio::ip::tcp::endpoint( asio::ip::address_v6::any(), port ), errorCode );

    if ( !isListening ) {
        const asio::ip::address address = localOnly ? asio::ip::address( asio::ip::address_v4::loopback() ) : asio::ip::address( asio::ip::address_v4::any() );
        isListening = impl.listen( asio::ip::tcp::endpoint( address, port ), errorCode );
    }

    if ( !isListening ) {
        impl.fail( describeError( errorCode ) );
        return false;
    }

    VERBOSE_LOG( "[MULTI] Listening for " << ( impl.acceptor.local_endpoint( errorCode ).address().is_v6() ? "IPv6 and IPv4" : "IPv4" ) << " connections" )

    impl.isAccepting = true;
    impl.acceptNext();

    return true;
}

void Multi::NetworkSession::join( const std::string & address, const uint16_t port )
{
    Impl & impl = *_impl;
    assert( !impl.isStarted );

    impl.isStarted = true;
    impl.isHostSide = false;
    impl.joinedAddress = address;
    impl.joinedPort = port;
    impl.connectionDeadline = std::chrono::steady_clock::now() + connectionTimeout;

    Link & link = impl.addHostLink();

    auto connection = std::make_shared<Transport>( impl.io );
    connection->link = &link;
    link.transport = connection;

    // No name lookup for a numeric address. The lookup runs on a thread of asio and destroying the session waits for it.
    std::error_code addressError;
    const asio::ip::address numericAddress = asio::ip::make_address( address, addressError );

    if ( !addressError ) {
        connection->socket.async_connect( asio::ip::tcp::endpoint( numericAddress, port ), [impl = _impl.get(), connection]( const std::error_code & connectError ) {
            if ( !impl->isLive( connection ) ) {
                return;
            }
            if ( connectError ) {
                impl->onError( connectError );
                return;
            }

            impl->readHeader( connection );
            impl->onConnected( connection );
        } );

        return;
    }

    impl.resolver.async_resolve( address, std::to_string( port ),
                                 [impl = _impl.get(), connection]( const std::error_code & resolveError, const asio::ip::tcp::resolver::results_type & endpoints ) {
                                     if ( !impl->isLive( connection ) ) {
                                         return;
                                     }
                                     if ( resolveError ) {
                                         impl->onError( resolveError );
                                         return;
                                     }

                                     asio::async_connect( connection->socket, endpoints,
                                                          [impl, connection]( const std::error_code & connectError, const asio::ip::tcp::endpoint & /* endpoint */ ) {
                                                              if ( !impl->isLive( connection ) ) {
                                                                  return;
                                                              }
                                                              if ( connectError ) {
                                                                  impl->onError( connectError );
                                                                  return;
                                                              }

                                                              impl->readHeader( connection );
                                                              impl->onConnected( connection );
                                                          } );
                                 } );
}

void Multi::NetworkSession::update()
{
    Impl & impl = *_impl;

    if ( impl.io.stopped() ) {
        impl.io.restart();
    }

    try {
        // Until nothing more can be read or written right now.
        impl.continueTransfers();
        while ( impl.io.poll() > 0 ) {
            impl.continueTransfers();
        }
    }
    catch ( const std::exception & ex ) {
        impl.fail( std::string( gettext_noop( "network error" ) ) + ": " + ex.what() );
    }

    if ( impl.isClosed ) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    Link * hostLink = impl.isHostSide ? nullptr : impl.hostLink();

    if ( hostLink != nullptr && hostLink->state == State::CONNECTING && now > impl.connectionDeadline ) {
        impl.fail( gettext_noop( "connection timed out" ) );
        return;
    }

    for ( const std::unique_ptr<Link> & link : impl.links ) {
        if ( link->isDropped || link->state == State::CLOSED ) {
            continue;
        }

        if ( link->transport && link->transport->writeError && now - link->transport->writeFailedAt > drainTimeout ) {
            const std::shared_ptr<Transport> connection = link->transport;
            impl.onTransportError( connection, connection->writeError );
            continue;
        }

        // Tell the other side how many messages arrived.
        if ( link->state == State::CONNECTED && link->transport && link->receivedCount > link->acknowledgedCount
             && now - link->lastAcknowledgement >= acknowledgementDelay ) {
            queueFrame( link->transport, makeFrame( makeSessionAck( link->receivedCount ) ) );
            link->acknowledgedCount = link->receivedCount;
            link->lastAcknowledgement = now;
            impl.writeNext( link->transport );
        }

        if ( link->pending && now > link->pending->deadline ) {
            Impl::dropPending( *link );
        }

        if ( link->unacknowledgedBytes > maxUnacknowledgedBytes ) {
            impl.failLink( *link, gettext_noop( "too much data is waiting for the other player" ) );
            continue;
        }

        if ( impl.testCutInterval.count() > 0 && link->state == State::CONNECTED && link->sessionId != 0 && now >= link->nextTestCut ) {
            if ( link->nextTestCut != std::chrono::steady_clock::time_point{} ) {
                // One cut in four lasts long enough to pause the game.
                const uint32_t random = impl.nextTestRandom();
                impl.cutForTest( *link, std::chrono::milliseconds( ( random % 4 == 0 ) ? 2500 + random % 2000 : random % 300 ) );
            }

            link->nextTestCut
                = now + impl.testCutInterval / 2 + std::chrono::milliseconds( impl.nextTestRandom() % static_cast<uint32_t>( impl.testCutInterval.count() ) );
        }
    }

    // Connections which did not tell who they are in time.
    for ( const std::shared_ptr<Transport> & connection : std::vector<std::shared_ptr<Transport>>( impl.candidates ) ) {
        if ( now > connection->deadline ) {
            impl.dropCandidate( connection );
        }
    }

    // Dropped links once their last messages have left.
    for ( const std::shared_ptr<Transport> & connection : impl.closing ) {
        if ( connection->isClosed || now > connection->deadline || ( connection->outgoing.empty() && connection->rawOutgoing.empty() && !connection->isWriting ) ) {
            Impl::closeTransport( connection );
        }
    }
    impl.closing.erase( std::remove_if( impl.closing.begin(), impl.closing.end(), []( const std::shared_ptr<Transport> & connection ) { return connection->isClosed; } ),
                        impl.closing.end() );

    impl.updateRelay( now );

    if ( hostLink != nullptr && hostLink->state == State::RECONNECTING ) {
        if ( hostLink->isRetrying && now - hostLink->retryStart > automaticRetryDuration ) {
            VERBOSE_LOG( "[MULTI] The connection could not be taken back on its own, waiting for the player" )
            hostLink->isRetrying = false;
        }

        if ( hostLink->isRetrying && !hostLink->pending && now >= hostLink->nextAttempt ) {
            impl.startAttempt();
        }
    }
}

void Multi::NetworkSession::send( const MachineId machine, const Message & message )
{
    // A previous message may be fully sent already. This one can then go right away instead of waiting for the next update().
    update();

    Impl & impl = *_impl;
    if ( impl.isClosed ) {
        return;
    }

    // Session messages are sent only by the session itself.
    assert( !isSessionMessage( message.type ) && message.type != MessageType::ROUTED );

    const bool isRouted = !impl.isHostSide && machine != hostMachine;

    if ( message.payload.size() > maxMessagePayloadSize - ( isRouted ? 6 : 0 ) ) {
        impl.fail( std::string( gettext_noop( "message too big" ) ) + ": " + std::to_string( message.payload.size() ) + " bytes" );
        return;
    }

    Link * link = impl.isHostSide ? impl.findLink( machine ) : impl.hostLink();
    if ( link == nullptr || link->state == State::CLOSED ) {
        return;
    }

    Impl::sendOnLink( *link, makeFrame( isRouted ? makeRouted( { hostMachine, machineBit( machine ), message } ) : message ) );
    impl.writeNext( link->transport );
}

void Multi::NetworkSession::sendToMachines( const MachineSet machines, const Message & message )
{
    Impl & impl = *_impl;

    if ( !impl.isHostSide ) {
        if ( machines == machineBit( hostMachine ) ) {
            send( hostMachine, message );
            return;
        }

        if ( machines == 0 ) {
            return;
        }

        update();

        Link * link = impl.hostLink();
        if ( impl.isClosed || link == nullptr || link->state == State::CLOSED ) {
            return;
        }

        assert( !isSessionMessage( message.type ) && message.type != MessageType::ROUTED );

        if ( message.payload.size() > maxMessagePayloadSize - 6 ) {
            impl.fail( std::string( gettext_noop( "message too big" ) ) + ": " + std::to_string( message.payload.size() ) + " bytes" );
            return;
        }

        Impl::sendOnLink( *link, makeFrame( makeRouted( { hostMachine, machines, message } ) ) );
        impl.writeNext( link->transport );
        return;
    }

    update();

    if ( impl.isClosed ) {
        return;
    }

    assert( !isSessionMessage( message.type ) && message.type != MessageType::ROUTED );

    if ( message.payload.size() > maxMessagePayloadSize ) {
        impl.fail( std::string( gettext_noop( "message too big" ) ) + ": " + std::to_string( message.payload.size() ) + " bytes" );
        return;
    }

    // The same bytes for every guest.
    const Frame frame = makeFrame( message );

    for ( MachineId machine = hostMachine + 1; machine <= maxMachineId; ++machine ) {
        if ( ( machines & machineBit( machine ) ) == 0 ) {
            continue;
        }

        if ( Link * link = impl.findLink( machine ); link != nullptr && link->state != State::CLOSED ) {
            Impl::sendOnLink( *link, frame );
            impl.writeNext( link->transport );
        }
    }
}

std::optional<Multi::Message> Multi::NetworkSession::receive()
{
    return _impl->take( []( const Message & /* message */ ) { return true; }, false );
}

std::optional<Multi::Message> Multi::NetworkSession::receive( const MessageType type )
{
    return receive( { type } );
}

std::optional<Multi::Message> Multi::NetworkSession::receive( const std::initializer_list<MessageType> types )
{
    return _impl->take( [&types]( const Message & message ) { return std::find( types.begin(), types.end(), message.type ) != types.end(); }, true );
}

std::optional<Multi::Message> Multi::NetworkSession::receive( const std::function<bool( const Message & message )> & filter )
{
    return _impl->take( filter, true );
}

std::optional<Multi::Message> Multi::NetworkSession::receiveAnywhere( const MessageType type )
{
    return _impl->take( [type]( const Message & message ) { return message.type == type; }, false );
}

void Multi::NetworkSession::putBack( Message message )
{
    Link * link = _impl->findSenderLink( message );
    if ( link == nullptr ) {
        return;
    }

    auto iter = link->received.begin();
    while ( iter != link->received.end() && iter->arrival < message.arrival ) {
        ++iter;
    }

    link->received.insert( iter, std::move( message ) );
}

void Multi::NetworkSession::close()
{
    Impl & impl = *_impl;

    // Tell the others not to wait for us. Messages still being sent get a short time to leave first.
    if ( !impl.isClosed ) {
        impl.sayGoodbye();
    }

    impl.fail( {} );
}

Multi::NetworkSession::State Multi::NetworkSession::state() const
{
    const Impl & impl = *_impl;

    if ( impl.isHostSide ) {
        return impl.isClosed ? State::CLOSED : ( impl.isStarted ? State::LISTENING : State::IDLE );
    }

    const Link * link = impl.hostLink();
    if ( link == nullptr ) {
        return impl.isClosed ? State::CLOSED : State::IDLE;
    }

    return link->state;
}

Multi::NetworkSession::State Multi::NetworkSession::state( const MachineId machine ) const
{
    if ( _impl->isHostSide && machine == hostMachine ) {
        return state();
    }

    const Link * link = _impl->findLink( machine );
    return link != nullptr ? link->state : State::CLOSED;
}

bool Multi::NetworkSession::isAlive() const
{
    const State current = state();
    return current == State::LISTENING || current == State::CONNECTED || current == State::RECONNECTING;
}

bool Multi::NetworkSession::isAlive( const MachineId machine ) const
{
    const State current = state( machine );
    return current == State::LISTENING || current == State::CONNECTED || current == State::RECONNECTING;
}

bool Multi::NetworkSession::isHostSide() const
{
    return _impl->isHostSide;
}

bool Multi::NetworkSession::isClosedByOther() const
{
    const Link * link = _impl->isHostSide ? nullptr : _impl->hostLink();
    return link != nullptr && link->isClosedByOther;
}

bool Multi::NetworkSession::isClosedByOther( const MachineId machine ) const
{
    const Link * link = _impl->findLink( machine );
    return link != nullptr && link->isClosedByOther;
}

bool Multi::NetworkSession::isResumeRefused() const
{
    const Link * link = _impl->isHostSide ? nullptr : _impl->hostLink();
    return link != nullptr && link->isResumeRefused;
}

const std::string & Multi::NetworkSession::joinedAddress() const
{
    return _impl->joinedAddress;
}

uint16_t Multi::NetworkSession::joinedPort() const
{
    return _impl->joinedPort;
}

size_t Multi::NetworkSession::undeliveredBytes() const
{
    size_t bytes = 0;
    for ( const std::unique_ptr<Link> & link : _impl->links ) {
        if ( !link->isDropped ) {
            bytes += link->unacknowledgedBytes;
        }
    }

    return bytes;
}

size_t Multi::NetworkSession::undeliveredBytes( const MachineId machine ) const
{
    const Link * link = _impl->findLink( machine );
    return link != nullptr ? link->unacknowledgedBytes : 0;
}

std::chrono::steady_clock::duration Multi::NetworkSession::lostFor() const
{
    return _impl->isHostSide ? std::chrono::steady_clock::duration::zero() : lostFor( hostMachine );
}

std::chrono::steady_clock::duration Multi::NetworkSession::lostFor( const MachineId machine ) const
{
    const Link * link = _impl->findLink( machine );
    if ( link == nullptr || link->state != State::RECONNECTING ) {
        return {};
    }

    return std::chrono::steady_clock::now() - link->lostAt;
}

bool Multi::NetworkSession::isRetrying() const
{
    const Link * link = _impl->isHostSide ? nullptr : _impl->hostLink();
    return link != nullptr && link->state == State::RECONNECTING && link->isRetrying;
}

void Multi::NetworkSession::retry()
{
    Link * link = _impl->isHostSide ? nullptr : _impl->hostLink();
    if ( link == nullptr || link->state != State::RECONNECTING ) {
        return;
    }

    VERBOSE_LOG( "[MULTI] Trying again to take the connection back" )

    link->isRetrying = true;
    link->attempts = 0;
    link->retryStart = std::chrono::steady_clock::now();
    link->nextAttempt = link->retryStart;
}

std::vector<Multi::MachineId> Multi::NetworkSession::guests() const
{
    std::vector<MachineId> machines;
    if ( !_impl->isHostSide ) {
        return machines;
    }

    for ( const std::unique_ptr<Link> & link : _impl->links ) {
        if ( !link->isDropped && link->machine != hostMachine && link->machine <= maxMachineId ) {
            machines.push_back( link->machine );
        }
    }

    std::sort( machines.begin(), machines.end() );
    return machines;
}

std::optional<Multi::NetworkSession::Newcomer> Multi::NetworkSession::takeNewcomer()
{
    Impl & impl = *_impl;

    while ( !impl.newcomers.empty() ) {
        Newcomer newcomer = std::move( impl.newcomers.front() );
        impl.newcomers.pop_front();

        if ( impl.findLink( newcomer.machine ) != nullptr ) {
            return newcomer;
        }
    }

    return {};
}

void Multi::NetworkSession::admit( const MachineId newcomer, const MachineId machine )
{
    const Impl & impl = *_impl;
    assert( impl.isHostSide && machine != hostMachine && machine <= maxMachineId );

    Link * link = impl.findLink( newcomer );
    if ( link == nullptr || newcomer <= maxMachineId ) {
        return;
    }

    if ( Link * previous = impl.findLink( machine ); previous != nullptr ) {
        Impl::dropLink( *previous );
    }

    link->machine = machine;
    for ( Message & message : link->received ) {
        message.sender = machine;
    }

    VERBOSE_LOG( "[MULTI] A new computer is now computer " << static_cast<int>( machine ) )
}

void Multi::NetworkSession::closeLink( const MachineId machine )
{
    Impl & impl = *_impl;

    Link * link = impl.findLink( machine );
    if ( !impl.isHostSide || link == nullptr ) {
        return;
    }

    if ( link->state == State::CONNECTED && link->transport && !impl.isClosed ) {
        const std::shared_ptr<Transport> connection = link->transport;
        link->transport.reset();

        queueFrame( connection, makeFrame( { MessageType::SESSION_CLOSE, {} } ) );
        connection->isClosing = true;
        connection->deadline = std::chrono::steady_clock::now() + closingTimeout;
        impl.closing.push_back( connection );
        impl.writeNext( connection );
    }

    Impl::dropLink( *link );
    impl.newcomers.erase( std::remove_if( impl.newcomers.begin(), impl.newcomers.end(), [machine]( const Newcomer & newcomer ) { return newcomer.machine == machine; } ),
                          impl.newcomers.end() );
}

const Multi::SessionStats & Multi::NetworkSession::stats() const
{
    return _impl->stats;
}

void Multi::NetworkSession::cutForTest( const std::chrono::milliseconds outage )
{
    Impl & impl = *_impl;

    for ( const std::unique_ptr<Link> & link : impl.links ) {
        if ( !link->isDropped ) {
            impl.cutForTest( *link, outage );
        }
    }
}

void Multi::NetworkSession::setTestCuts( const std::chrono::milliseconds interval )
{
    _impl->testCutInterval = interval;
    _impl->testRandom ^= static_cast<uint32_t>( std::chrono::steady_clock::now().time_since_epoch().count() ) | 1U;
}

const std::string & Multi::NetworkSession::error() const
{
    const Impl & impl = *_impl;

    if ( const Link * link = impl.isHostSide ? nullptr : impl.hostLink(); link != nullptr && impl.error.empty() ) {
        return link->error;
    }

    return impl.error;
}

const std::string & Multi::NetworkSession::error( const MachineId machine ) const
{
    if ( const Link * link = _impl->findLink( machine ); link != nullptr ) {
        return link->error;
    }

    return _impl->error;
}

bool Multi::NetworkSession::isAddressUnknown() const
{
    return _impl->isAddressUnknown;
}

size_t Multi::NetworkSession::pendingOutgoingBytes() const
{
    size_t bytes = 0;

    for ( const std::unique_ptr<Link> & link : _impl->links ) {
        if ( link->isDropped ) {
            continue;
        }

        bytes += link->transport ? link->transport->outgoingBytes : link->unacknowledgedBytes;
    }

    for ( const std::shared_ptr<Transport> & connection : _impl->closing ) {
        bytes += connection->outgoingBytes;
    }

    return bytes;
}

void Multi::NetworkSession::setProbeKey( const LobbyKey & key )
{
    _impl->probeKey = key;
}

namespace
{
    // The relay of a room: the host and the port of the relay address, the path of the room.
    struct RelayLocation
    {
        std::string host;
        uint16_t port;
        std::string path;
    };

    std::optional<RelayLocation> getRelayLocation( const std::string & relayAddress, const uint64_t roomId )
    {
        const std::optional<Multi::LobbyAddress> address = Multi::parseLobbyAddress( relayAddress );
        if ( !address || address->isSecure ) {
            return {};
        }

        std::ostringstream path;
        path << "/relay/" << std::hex << std::setw( 16 ) << std::setfill( '0' ) << roomId;

        return RelayLocation{ address->host, address->port, path.str() };
    }
}

void Multi::NetworkSession::listenOnRelay( const std::string & relayAddress, const uint64_t roomId, const LobbyKey & hostToken, const uint32_t publishCount )
{
    Impl & impl = *_impl;
    if ( !impl.isHostSide || impl.isClosed ) {
        return;
    }

    const auto location = getRelayLocation( relayAddress, roomId );
    if ( !location ) {
        ERROR_LOG( "[MULTI] Wrong address of the relay of the lobby: " << relayAddress )
        return;
    }

    const auto & [host, port, path] = *location;
    if ( impl.relayRoute && impl.relayRoute->host == host && impl.relayRoute->port == port && impl.relayRoute->path == path && impl.relayRoute->key == hostToken ) {
        if ( publishCount != impl.relayPublishCount ) {
            impl.relayPublishCount = publishCount;
            if ( !impl.relayWaiting ) {
                impl.relayNextAttempt = std::chrono::steady_clock::now();
            }
        }
        return;
    }

    VERBOSE_LOG( "[MULTI] Waiting for the other computers on the relay of the lobby too" )

    impl.relayRoute = std::make_unique<Impl::RelayRoute>( Impl::RelayRoute{ host, port, path, hostToken } );
    impl.relayPublishCount = publishCount;
    Impl::closeTransport( impl.relayWaiting );
    impl.relayWaiting.reset();
    impl.relayNextAttempt = std::chrono::steady_clock::now();
}

void Multi::NetworkSession::joinThroughRelay( const std::string & relayAddress, const uint64_t roomId, const LobbyKey & ticket )
{
    Impl & impl = *_impl;
    assert( !impl.isStarted );

    impl.isStarted = true;
    impl.isHostSide = false;
    impl.connectionDeadline = std::chrono::steady_clock::now() + connectionTimeout;

    impl.addHostLink();

    const auto location = getRelayLocation( relayAddress, roomId );
    if ( !location ) {
        impl.fail( gettext_noop( "wrong address of the relay of the lobby" ) );
        return;
    }

    const auto & [host, port, path] = *location;
    impl.relayRoute = std::make_unique<Impl::RelayRoute>( Impl::RelayRoute{ host, port, path, ticket } );
    impl.openRelayLink( RelayLinkPurpose::JOINING );
}

bool Multi::NetworkSession::isThroughRelay() const
{
    const Link * link = _impl->isHostSide ? nullptr : _impl->hostLink();
    return link != nullptr && link->transport && link->transport->webSocket;
}

namespace
{
    // Lobbies answer the pings sent every 30 seconds. One which stays silent for a minute is gone.
    constexpr std::chrono::seconds lobbyPingInterval{ 30 };
    constexpr std::chrono::seconds lobbySilenceTimeout{ 60 };

    std::string describeLobbyError( const std::error_code & error )
    {
        if ( error == asio::error::eof || error == asio::error::connection_reset || error == asio::error::connection_aborted ) {
            return gettext_noop( "the lobby closed the connection" );
        }
        if ( error == asio::error::connection_refused ) {
            return gettext_noop( "there is no lobby at this address" );
        }

        return describeError( error );
    }

    // The TCP connection to a lobby. Pending operations keep it alive after the connection object is gone.
    struct LobbyLink
    {
        explicit LobbyLink( asio::io_context & io )
            : socket( io )
        {
            // Do nothing.
        }

        asio::ip::tcp::socket socket;
        std::array<uint8_t, 16384> readBuffer{};
        std::deque<std::vector<uint8_t>> outgoing;
        bool isConnected{ false };
        bool isReading{ false };
        bool isWriting{ false };
        bool isClosed{ false };
    };
}

struct Multi::LobbyConnection::Impl
{
    // Must be destroyed last, with the pending operations which keep the link alive.
    asio::io_context io;
    asio::ip::tcp::resolver resolver{ io };

    std::shared_ptr<LobbyLink> link;
    std::unique_ptr<WebSocketClient> client;

    State state{ State::IDLE };
    std::string error;
    std::deque<Message> received;
    // Sent once the lobby accepted the WebSocket connection.
    std::vector<Message> waiting;

    std::chrono::steady_clock::time_point deadline;
    std::chrono::steady_clock::time_point lastPing;
    bool hasPinged{ false };
    bool isPingAnswered{ true };
    std::chrono::milliseconds delay{ 0 };

    void closeLink() const
    {
        if ( !link || link->isClosed ) {
            return;
        }

        link->isClosed = true;

        std::error_code ignored;
        link->socket.shutdown( asio::ip::tcp::socket::shutdown_both, ignored );
        link->socket.close( ignored );
    }

    void fail( std::string reason )
    {
        if ( state == State::CLOSED ) {
            return;
        }

        if ( !reason.empty() ) {
            ERROR_LOG( "[MULTI] Lobby connection closed: " << reason )
        }

        state = State::CLOSED;
        error = std::move( reason );
        resolver.cancel();
        closeLink();
    }

    void onError( const std::error_code & errorCode )
    {
        if ( errorCode != asio::error::operation_aborted ) {
            fail( describeLobbyError( errorCode ) );
        }
    }

    void onConnected( const std::shared_ptr<LobbyLink> & connected ) const
    {
        std::error_code ignored;
        connected->socket.set_option( asio::ip::tcp::no_delay( true ), ignored );
        connected->isConnected = true;
        connected->outgoing.push_back( client->makeRequest() );
    }

    void sendNow( const Message & message ) const
    {
        const std::vector<uint8_t> bytes = encodeMessage( message );
        link->outgoing.push_back( client->makeMessage( bytes.data(), bytes.size() ) );
    }

    void onData( const size_t size )
    {
        const bool wasOpen = client->isOpen();
        if ( !client->receive( link->readBuffer.data(), size ) ) {
            fail( client->error() );
            return;
        }

        std::vector<uint8_t> replies = client->takeReplies();
        if ( !replies.empty() ) {
            link->outgoing.push_back( std::move( replies ) );
        }

        if ( !wasOpen && client->isOpen() ) {
            state = State::OPEN;
            for ( const Message & message : waiting ) {
                sendNow( message );
            }
            waiting.clear();
        }

        if ( client->takePongs() > 0 && !isPingAnswered ) {
            isPingAnswered = true;
            // At least 1 ms, 0 means unknown.
            delay = std::max( std::chrono::milliseconds( 1 ), std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - lastPing ) );
        }

        // One lobby message per WebSocket message.
        while ( std::optional<std::vector<uint8_t>> bytes = client->takeMessage() ) {
            Message message;
            uint32_t payloadSize = 0;
            if ( bytes->size() < messageHeaderSize || !decodeMessageHeader( bytes->data(), message.type, payloadSize )
                 || payloadSize != bytes->size() - messageHeaderSize ) {
                fail( gettext_noop( "the lobby sent wrong data" ) );
                return;
            }

            message.payload.assign( bytes->begin() + messageHeaderSize, bytes->end() );
            received.push_back( std::move( message ) );
        }
    }

    // A finished read or write lets the next one start from here, never from its own handler.
    void continueTransfers()
    {
        if ( !link || link->isClosed || !link->isConnected ) {
            return;
        }

        if ( !link->isReading ) {
            link->isReading = true;
            link->socket.async_read_some( asio::buffer( link->readBuffer ), [this, connected = link]( const std::error_code & errorCode, const size_t size ) {
                connected->isReading = false;
                if ( connected->isClosed ) {
                    return;
                }
                if ( errorCode ) {
                    onError( errorCode );
                    return;
                }

                onData( size );
            } );
        }

        if ( !link->isWriting && !link->outgoing.empty() ) {
            link->isWriting = true;
            auto bytes = std::make_shared<std::vector<uint8_t>>( std::move( link->outgoing.front() ) );
            link->outgoing.pop_front();

            asio::async_write( link->socket, asio::buffer( *bytes ), [this, connected = link, bytes]( const std::error_code & errorCode, size_t /* size */ ) {
                connected->isWriting = false;
                if ( !connected->isClosed && errorCode ) {
                    onError( errorCode );
                }
            } );
        }
    }

    void poll()
    {
        if ( io.stopped() ) {
            io.restart();
        }

        try {
            continueTransfers();
            while ( io.poll() > 0 ) {
                continueTransfers();
            }
        }
        catch ( const std::exception & ex ) {
            fail( std::string( gettext_noop( "network error" ) ) + ": " + ex.what() );
        }
    }
};

Multi::LobbyConnection::LobbyConnection()
    : _impl( std::make_unique<Impl>() )
{}

Multi::LobbyConnection::~LobbyConnection()
{
    try {
        close();
    }
    catch ( ... ) {
        // Nothing may leave a destructor: the lobby notices the connection is gone by itself.
        _impl.reset();
    }
}

void Multi::LobbyConnection::open( const std::string & address )
{
    Impl & impl = *_impl;
    assert( impl.state == State::IDLE );

    impl.state = State::CONNECTING;
    impl.deadline = std::chrono::steady_clock::now() + connectionTimeout;

    const std::optional<LobbyAddress> lobby = parseLobbyAddress( address );
    if ( !lobby ) {
        impl.fail( gettext_noop( "wrong lobby address" ) );
        return;
    }
    if ( lobby->isSecure ) {
        impl.fail( gettext_noop( "secure connections to lobbies are not supported yet" ) );
        return;
    }

    VERBOSE_LOG( "[MULTI] Connecting to the lobby " << lobby->host << ":" << lobby->port << lobby->path )

    impl.client = std::make_unique<WebSocketClient>( lobby->host, lobby->port, lobby->path, messageHeaderSize + maxLobbyMessagePayloadSize );

    auto link = std::make_shared<LobbyLink>( impl.io );
    impl.link = link;

    const auto onConnect = [impl = &impl, link]( const std::error_code & connectError ) {
        if ( link->isClosed ) {
            return;
        }
        if ( connectError ) {
            impl->onError( connectError );
            return;
        }

        impl->onConnected( link );
    };

    std::error_code addressError;
    const asio::ip::address numericAddress = asio::ip::make_address( lobby->host, addressError );
    if ( !addressError ) {
        link->socket.async_connect( asio::ip::tcp::endpoint( numericAddress, lobby->port ), onConnect );
        return;
    }

    impl.resolver.async_resolve( lobby->host, std::to_string( lobby->port ),
                                 [impl = &impl, link, onConnect]( const std::error_code & resolveError, const asio::ip::tcp::resolver::results_type & endpoints ) {
                                     if ( link->isClosed ) {
                                         return;
                                     }
                                     if ( resolveError ) {
                                         impl->onError( resolveError );
                                         return;
                                     }

                                     asio::async_connect( link->socket, endpoints,
                                                          [onConnect]( const std::error_code & connectError, const asio::ip::tcp::endpoint & /* endpoint */ ) {
                                                              onConnect( connectError );
                                                          } );
                                 } );
}

void Multi::LobbyConnection::update()
{
    Impl & impl = *_impl;
    if ( impl.state == State::IDLE || impl.state == State::CLOSED ) {
        return;
    }

    impl.poll();

    const auto now = std::chrono::steady_clock::now();

    if ( impl.state == State::CONNECTING && now > impl.deadline ) {
        impl.fail( gettext_noop( "the lobby does not answer" ) );
        return;
    }

    if ( impl.state != State::OPEN ) {
        return;
    }

    if ( impl.client->isClosedByServer() ) {
        // The answer to its close leaves before the connection is dropped.
        if ( impl.link->outgoing.empty() && !impl.link->isWriting ) {
            impl.fail( gettext_noop( "the lobby closed the connection" ) );
        }
        return;
    }

    if ( !impl.isPingAnswered && now - impl.lastPing > lobbySilenceTimeout ) {
        impl.fail( gettext_noop( "the lobby does not answer" ) );
        return;
    }

    // The first ping goes as soon as the connection is open, to know the delay. Lobbies on some hostings do not see pings:
    // a message tells them that the game is still there.
    if ( impl.isPingAnswered && ( !impl.hasPinged || now - impl.lastPing >= lobbyPingInterval ) ) {
        impl.sendNow( makeEmptyLobbyMessage( MessageType::LOBBY_KEEPALIVE ) );
        impl.link->outgoing.push_back( impl.client->makePing() );
        impl.lastPing = now;
        impl.hasPinged = true;
        impl.isPingAnswered = false;
        impl.poll();
    }
}

void Multi::LobbyConnection::send( const Message & message )
{
    Impl & impl = *_impl;

    if ( impl.state == State::CONNECTING ) {
        impl.waiting.push_back( message );
    }
    else if ( impl.state == State::OPEN ) {
        impl.sendNow( message );
        impl.poll();
    }
}

std::optional<Multi::Message> Multi::LobbyConnection::receive()
{
    if ( _impl->received.empty() ) {
        return {};
    }

    Message message = std::move( _impl->received.front() );
    _impl->received.pop_front();

    return message;
}

void Multi::LobbyConnection::close()
{
    Impl & impl = *_impl;

    // A goodbye, given a short time to leave.
    if ( impl.state == State::OPEN && !impl.client->isClosedByServer() ) {
        impl.link->outgoing.push_back( impl.client->makeClose( 1000 ) );

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds( 200 );
        while ( impl.state == State::OPEN && ( !impl.link->outgoing.empty() || impl.link->isWriting ) && std::chrono::steady_clock::now() < deadline ) {
            impl.poll();
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    }

    impl.fail( {} );
}

Multi::LobbyConnection::State Multi::LobbyConnection::state() const
{
    return _impl->state;
}

const std::string & Multi::LobbyConnection::error() const
{
    return _impl->error;
}

std::chrono::milliseconds Multi::LobbyConnection::delay() const
{
    return _impl->delay;
}

#else

#include "translations.h"

// A build without network games: asio is missing, or the platform has no network (the web version).
struct Multi::NetworkSession::Impl
{
    std::string error{ gettext_noop( "network games are not available in this version of fheroes2" ) };
    std::string address;
    SessionStats stats;
};

bool Multi::isNetworkAvailable()
{
    return false;
}

std::vector<std::string> Multi::getLocalAddresses()
{
    return {};
}

std::string Multi::getInternetAddressV6()
{
    return {};
}

bool Multi::isIPv6Address( const std::string & /* text */ )
{
    return false;
}

Multi::NetworkSession::NetworkSession()
    : _impl( std::make_unique<Impl>() )
{}

Multi::NetworkSession::~NetworkSession() = default;

bool Multi::NetworkSession::host( const uint16_t /* port */, const bool /* localOnly */ )
{
    return false;
}

void Multi::NetworkSession::join( const std::string & address, const uint16_t /* port */ )
{
    _impl->address = address;
}

void Multi::NetworkSession::update()
{
    // Do nothing.
}

void Multi::NetworkSession::send( const MachineId /* machine */, const Message & /* message */ )
{
    // Do nothing.
}

void Multi::NetworkSession::sendToMachines( const MachineSet /* machines */, const Message & /* message */ )
{
    // Do nothing.
}

std::optional<Multi::Message> Multi::NetworkSession::receive()
{
    return {};
}

std::optional<Multi::Message> Multi::NetworkSession::receive( const MessageType /* type */ )
{
    return {};
}

std::optional<Multi::Message> Multi::NetworkSession::receive( const std::initializer_list<MessageType> /* types */ )
{
    return {};
}

std::optional<Multi::Message> Multi::NetworkSession::receive( const std::function<bool( const Message & message )> & /* filter */ )
{
    return {};
}

std::optional<Multi::Message> Multi::NetworkSession::receiveAnywhere( const MessageType /* type */ )
{
    return {};
}

void Multi::NetworkSession::putBack( Message /* message */ )
{
    // Do nothing.
}

void Multi::NetworkSession::close()
{
    // Do nothing.
}

Multi::NetworkSession::State Multi::NetworkSession::state() const
{
    return State::CLOSED;
}

Multi::NetworkSession::State Multi::NetworkSession::state( const MachineId /* machine */ ) const
{
    return State::CLOSED;
}

bool Multi::NetworkSession::isAlive() const
{
    return false;
}

bool Multi::NetworkSession::isAlive( const MachineId /* machine */ ) const
{
    return false;
}

bool Multi::NetworkSession::isHostSide() const
{
    return false;
}

bool Multi::NetworkSession::isClosedByOther() const
{
    return false;
}

bool Multi::NetworkSession::isClosedByOther( const MachineId /* machine */ ) const
{
    return false;
}

bool Multi::NetworkSession::isResumeRefused() const
{
    return false;
}

const std::string & Multi::NetworkSession::joinedAddress() const
{
    return _impl->address;
}

uint16_t Multi::NetworkSession::joinedPort() const
{
    return defaultPort;
}

size_t Multi::NetworkSession::undeliveredBytes() const
{
    return 0;
}

size_t Multi::NetworkSession::undeliveredBytes( const MachineId /* machine */ ) const
{
    return 0;
}

std::chrono::steady_clock::duration Multi::NetworkSession::lostFor() const
{
    return std::chrono::steady_clock::duration::zero();
}

std::chrono::steady_clock::duration Multi::NetworkSession::lostFor( const MachineId /* machine */ ) const
{
    return std::chrono::steady_clock::duration::zero();
}

bool Multi::NetworkSession::isRetrying() const
{
    return false;
}

void Multi::NetworkSession::retry()
{
    // Do nothing.
}

std::vector<Multi::MachineId> Multi::NetworkSession::guests() const
{
    return {};
}

std::optional<Multi::NetworkSession::Newcomer> Multi::NetworkSession::takeNewcomer()
{
    return {};
}

void Multi::NetworkSession::admit( const MachineId /* newcomer */, const MachineId /* machine */ )
{
    // Do nothing.
}

void Multi::NetworkSession::closeLink( const MachineId /* machine */ )
{
    // Do nothing.
}

const Multi::SessionStats & Multi::NetworkSession::stats() const
{
    return _impl->stats;
}

void Multi::NetworkSession::cutForTest( const std::chrono::milliseconds /* outage */ )
{
    // Do nothing.
}

void Multi::NetworkSession::setTestCuts( const std::chrono::milliseconds /* interval */ )
{
    // Do nothing.
}

const std::string & Multi::NetworkSession::error() const
{
    return _impl->error;
}

const std::string & Multi::NetworkSession::error( const MachineId /* machine */ ) const
{
    return _impl->error;
}

bool Multi::NetworkSession::isAddressUnknown() const
{
    return false;
}

size_t Multi::NetworkSession::pendingOutgoingBytes() const
{
    return 0;
}

void Multi::NetworkSession::setProbeKey( const LobbyKey & /* key */ )
{
    // Do nothing.
}

void Multi::NetworkSession::listenOnRelay( const std::string & /* relayAddress */, const uint64_t /* roomId */, const LobbyKey & /* hostToken */,
                                           const uint32_t /* publishCount */ )
{
    // Do nothing.
}

void Multi::NetworkSession::joinThroughRelay( const std::string & /* relayAddress */, const uint64_t /* roomId */, const LobbyKey & /* ticket */ )
{
    // Do nothing.
}

bool Multi::NetworkSession::isThroughRelay() const
{
    return false;
}

struct Multi::LobbyConnection::Impl
{
    std::string error{ gettext_noop( "network games are not available in this version of fheroes2" ) };
};

Multi::LobbyConnection::LobbyConnection()
    : _impl( std::make_unique<Impl>() )
{}

Multi::LobbyConnection::~LobbyConnection() = default;

void Multi::LobbyConnection::open( const std::string & /* address */ )
{
    // Do nothing.
}

void Multi::LobbyConnection::update()
{
    // Do nothing.
}

void Multi::LobbyConnection::send( const Message & /* message */ )
{
    // Do nothing.
}

std::optional<Multi::Message> Multi::LobbyConnection::receive()
{
    return {};
}

void Multi::LobbyConnection::close()
{
    // Do nothing.
}

Multi::LobbyConnection::State Multi::LobbyConnection::state() const
{
    return State::CLOSED;
}

const std::string & Multi::LobbyConnection::error() const
{
    return _impl->error;
}

std::chrono::milliseconds Multi::LobbyConnection::delay() const
{
    return std::chrono::milliseconds( 0 );
}

#endif
