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

#include "multi_lobby_client.h"

#include <algorithm>
#include <array>
#include <random>
#include <utility>

#include "logging.h"
#include "maps_fileinfo.h"
#include "multi_network.h"
#include "multi_simultaneous.h"
#include "multi_text.h"
#include "players.h"
#include "settings.h"
#include "translations.h"
#include "ui_language.h"
#include "world.h"

namespace
{
    // A lobby which failed or was lost is tried again after this time.
    constexpr std::chrono::seconds lobbyRetryDelay{ 10 };

    std::string describeWelcomeRefusal( const Multi::LobbyWelcomeResult result )
    {
        switch ( result ) {
        case Multi::LobbyWelcomeResult::INCOMPATIBLE_VERSIONS:
            return gettext_noop( "the lobby does not know this version of the game" );
        case Multi::LobbyWelcomeResult::NICKNAME_REFUSED:
            return gettext_noop( "the lobby refused the nickname" );
        case Multi::LobbyWelcomeResult::LOBBY_FULL:
            return gettext_noop( "the lobby is full" );
        case Multi::LobbyWelcomeResult::TOO_MANY_CONNECTIONS:
            return gettext_noop( "too many games from this address" );
        default:
            return gettext_noop( "the lobby refused the connection" );
        }
    }

    std::string describeRoomRefusal( const Multi::RoomStatusResult result )
    {
        switch ( result ) {
        case Multi::RoomStatusResult::GAME_NAME_REFUSED:
            return gettext_noop( "the lobby refused the name of the game" );
        case Multi::RoomStatusResult::TOO_MANY_ROOMS:
            return gettext_noop( "too many games from this address" );
        default:
            return gettext_noop( "the lobby refused the game" );
        }
    }

    bool isSameGame( const Multi::HandshakeInfo & hostGame )
    {
        return Multi::checkHandshake( Multi::getLocalHandshakeInfo(), hostGame ) == Multi::HandshakeResult::ACCEPTED;
    }

    // A refusal to join, from the most useful to tell to the least useful.
    int getRefusalRank( const Multi::JoinResult result )
    {
        switch ( result ) {
        case Multi::JoinResult::OTHER_VERSION:
            return 3;
        case Multi::JoinResult::ROOM_IN_GAME:
            return 2;
        case Multi::JoinResult::NO_SUCH_ROOM:
            return 1;
        default:
            return 0;
        }
    }
}

struct Multi::LobbyBrowser::Impl
{
    struct Lobby
    {
        std::unique_ptr<LobbyConnection> connection;
        LobbyState state;
        // The next ROOM_LIST starts the rooms of this lobby again.
        bool isNewList{ true };
        std::chrono::steady_clock::time_point retryAt;
        bool isAskedToJoin{ false };
    };

    std::string nickname;
    std::vector<Lobby> lobbies;
    std::vector<LobbyState> states;
    std::vector<ListedRoom> rooms;
    bool isChanged{ true };

    bool isJoining{ false };
    std::optional<JoinAnswer> answer;
    std::optional<JoinAnswer> bestRefusal;

    void connect( Lobby & lobby ) const
    {
        lobby.connection = std::make_unique<LobbyConnection>();
        lobby.connection->open( lobby.state.address );
        lobby.connection->send( makeLobbyHello( makeLobbyHelloOfThisGame( nickname ) ) );
        lobby.connection->send( makeRoomListSubscribe( true ) );

        lobby.state.status = LobbyState::Status::CONNECTING;
        lobby.isNewList = true;
    }

    void removeRooms( const size_t lobbyIndex )
    {
        rooms.erase( std::remove_if( rooms.begin(), rooms.end(), [lobbyIndex]( const ListedRoom & room ) { return room.lobbyIndex == lobbyIndex; } ), rooms.end() );
    }

    // New rooms come first, then the ones this game can join are moved before the others.
    void addRoom( const size_t lobbyIndex, const RoomInfo & info, const bool isAtEnd )
    {
        const auto existing = std::find_if( rooms.begin(), rooms.end(),
                                            [lobbyIndex, &info]( const ListedRoom & room ) { return room.lobbyIndex == lobbyIndex && room.info.roomId == info.roomId; } );

        if ( existing != rooms.end() ) {
            existing->info = info;
        }
        else {
            ListedRoom room{ lobbyIndex, info, isSameGame( info.hostGame ) };
            rooms.insert( isAtEnd ? rooms.end() : rooms.begin(), std::move( room ) );
        }

        std::stable_partition( rooms.begin(), rooms.end(), []( const ListedRoom & room ) { return room.isSameGame; } );
    }

    void onJoinAnswer( const size_t lobbyIndex, JoinPlan plan )
    {
        Lobby & lobby = lobbies[lobbyIndex];
        if ( !isJoining || !lobby.isAskedToJoin ) {
            return;
        }

        lobby.isAskedToJoin = false;

        if ( plan.result == JoinResult::READY ) {
            answer = JoinAnswer{ lobbyIndex, std::move( plan ), getRelayAddress( lobby.state.address, lobby.state.welcome ) };
            isJoining = false;
            for ( Lobby & other : lobbies ) {
                other.isAskedToJoin = false;
            }
            return;
        }

        if ( !bestRefusal || getRefusalRank( plan.result ) > getRefusalRank( bestRefusal->plan.result ) ) {
            bestRefusal = JoinAnswer{ lobbyIndex, std::move( plan ), {} };
        }

        endJoinIfNoAnswerLeft();
    }

    void endJoinIfNoAnswerLeft()
    {
        if ( !isJoining || std::any_of( lobbies.begin(), lobbies.end(), []( const Lobby & lobby ) { return lobby.isAskedToJoin; } ) ) {
            return;
        }

        isJoining = false;
        if ( bestRefusal ) {
            answer = std::move( bestRefusal );
        }
        else {
            JoinPlan noRoom;
            noRoom.result = JoinResult::NO_SUCH_ROOM;
            answer = JoinAnswer{ 0, noRoom, {} };
        }
        bestRefusal.reset();
    }

    // Returns false if the message is wrong: the lobby is then dropped.
    bool onMessage( const size_t lobbyIndex, const Message & message )
    {
        Lobby & lobby = lobbies[lobbyIndex];

        switch ( message.type ) {
        case MessageType::LOBBY_WELCOME: {
            const std::optional<LobbyWelcome> welcome = readLobbyWelcome( message );
            if ( !welcome ) {
                return false;
            }

            lobby.state.welcome = *welcome;
            if ( welcome->result == LobbyWelcomeResult::ACCEPTED ) {
                lobby.state.status = LobbyState::Status::OPEN;
                lobby.state.error.clear();
            }
            else {
                lobby.state.status = LobbyState::Status::REFUSED;
                lobby.state.error = describeWelcomeRefusal( welcome->result );
                ERROR_LOG( "[MULTI] The lobby " << lobby.state.address << " refused this game: " << lobby.state.error )
            }
            return true;
        }
        case MessageType::LOBBY_STATS: {
            const std::optional<LobbyStats> stats = readLobbyStats( message );
            if ( !stats ) {
                return false;
            }
            lobby.state.stats = *stats;
            return true;
        }
        case MessageType::ROOM_LIST: {
            const std::optional<RoomList> list = readRoomList( message );
            if ( !list ) {
                return false;
            }

            if ( lobby.isNewList ) {
                removeRooms( lobbyIndex );
            }
            lobby.isNewList = list->isLastPart;

            for ( const RoomInfo & room : list->rooms ) {
                addRoom( lobbyIndex, room, true );
            }
            return true;
        }
        case MessageType::ROOM_INFO: {
            const std::optional<RoomInfo> room = readRoomInfo( message );
            if ( !room ) {
                return false;
            }
            addRoom( lobbyIndex, *room, false );
            return true;
        }
        case MessageType::ROOM_REMOVED: {
            const std::optional<uint64_t> roomId = readRoomRemoved( message );
            if ( !roomId ) {
                return false;
            }
            rooms.erase( std::remove_if( rooms.begin(), rooms.end(),
                                         [lobbyIndex, roomId]( const ListedRoom & room ) { return room.lobbyIndex == lobbyIndex && room.info.roomId == *roomId; } ),
                         rooms.end() );
            return true;
        }
        case MessageType::JOIN_PLAN: {
            std::optional<JoinPlan> plan = readJoinPlan( message );
            if ( !plan ) {
                return false;
            }
            onJoinAnswer( lobbyIndex, std::move( *plan ) );
            return true;
        }
        default:
            return false;
        }
    }

    void lose( const size_t lobbyIndex, std::string error )
    {
        Lobby & lobby = lobbies[lobbyIndex];

        if ( lobby.state.status != LobbyState::Status::REFUSED ) {
            lobby.state.status = LobbyState::Status::LOST;
            lobby.state.error = std::move( error );
        }

        lobby.connection.reset();
        lobby.retryAt = std::chrono::steady_clock::now() + lobbyRetryDelay;
        removeRooms( lobbyIndex );

        if ( lobby.isAskedToJoin ) {
            lobby.isAskedToJoin = false;
            endJoinIfNoAnswerLeft();
        }

        isChanged = true;
    }
};

Multi::LobbyBrowser::LobbyBrowser( const std::vector<std::string> & addresses, const std::string & nickname )
    : _impl( std::make_unique<Impl>() )
{
    _impl->nickname = nickname;

    for ( const std::string & address : addresses ) {
        Impl::Lobby lobby;
        lobby.state.address = address;
        _impl->connect( lobby );
        _impl->lobbies.push_back( std::move( lobby ) );
    }
}

Multi::LobbyBrowser::~LobbyBrowser() = default;

bool Multi::LobbyBrowser::update()
{
    Impl & impl = *_impl;
    const auto now = std::chrono::steady_clock::now();

    for ( size_t i = 0; i < impl.lobbies.size(); ++i ) {
        Impl::Lobby & lobby = impl.lobbies[i];

        if ( !lobby.connection ) {
            if ( lobby.state.status == LobbyState::Status::LOST && now >= lobby.retryAt ) {
                impl.connect( lobby );
                impl.isChanged = true;
            }
            continue;
        }

        lobby.connection->update();

        while ( std::optional<Message> message = lobby.connection->receive() ) {
            if ( !impl.onMessage( i, *message ) ) {
                impl.lose( i, gettext_noop( "the lobby sent wrong data" ) );
                break;
            }
            impl.isChanged = true;
        }

        if ( !lobby.connection ) {
            continue;
        }

        if ( lobby.connection->delay() != lobby.state.delay ) {
            lobby.state.delay = lobby.connection->delay();
            impl.isChanged = true;
        }

        if ( lobby.connection->state() == LobbyConnection::State::CLOSED ) {
            impl.lose( i, lobby.connection->error().empty() ? gettext_noop( "the lobby closed the connection" ) : lobby.connection->error() );
        }
    }

    impl.states.clear();
    for ( const Impl::Lobby & lobby : impl.lobbies ) {
        impl.states.push_back( lobby.state );
    }

    return std::exchange( impl.isChanged, false );
}

const std::vector<Multi::LobbyState> & Multi::LobbyBrowser::lobbies() const
{
    return _impl->states;
}

const std::vector<Multi::ListedRoom> & Multi::LobbyBrowser::rooms() const
{
    return _impl->rooms;
}

void Multi::LobbyBrowser::join( const ListedRoom & room )
{
    Impl & impl = *_impl;
    impl.answer.reset();
    impl.bestRefusal.reset();

    for ( Impl::Lobby & lobby : impl.lobbies ) {
        lobby.isAskedToJoin = false;
    }

    impl.isJoining = true;

    if ( room.lobbyIndex < impl.lobbies.size() && impl.lobbies[room.lobbyIndex].state.status == LobbyState::Status::OPEN ) {
        Impl::Lobby & lobby = impl.lobbies[room.lobbyIndex];
        lobby.isAskedToJoin = true;
        lobby.connection->send( makeRoomJoin( { room.info.roomId, {} } ) );
    }

    impl.endJoinIfNoAnswerLeft();
}

void Multi::LobbyBrowser::joinByCode( const std::string & code )
{
    Impl & impl = *_impl;
    impl.answer.reset();
    impl.bestRefusal.reset();
    impl.isJoining = true;

    for ( Impl::Lobby & lobby : impl.lobbies ) {
        lobby.isAskedToJoin = ( lobby.state.status == LobbyState::Status::OPEN );
        if ( lobby.isAskedToJoin ) {
            lobby.connection->send( makeRoomJoin( { 0, code } ) );
        }
    }

    impl.endJoinIfNoAnswerLeft();
}

bool Multi::LobbyBrowser::isJoining() const
{
    return _impl->isJoining;
}

std::optional<Multi::JoinAnswer> Multi::LobbyBrowser::takeJoinAnswer()
{
    return std::exchange( _impl->answer, std::nullopt );
}

struct Multi::PublishedRoom::Impl
{
    std::string lobbyAddress;
    std::string nickname;
    RoomPublish room;
    // Sent again to the lobby after a cut.
    std::optional<RoomUpdate> lastUpdate;
    bool isUpdateSent{ true };

    std::unique_ptr<LobbyConnection> connection;
    State state{ State::CONNECTING };
    std::string error;
    RoomStatus status;
    LobbyWelcome welcome;

    // After a cut or a restart of the lobby, the room is published again with its id: the lobby gives it back as it was.
    bool isReclaiming{ false };
    uint32_t publishCount{ 0 };
    uint32_t attempts{ 0 };
    std::chrono::steady_clock::time_point nextAttempt;

    // The room as it is now. After a cut, with its id and code.
    RoomPublish makePublish( const bool isAgain ) const
    {
        RoomPublish publish = room;
        if ( isAgain ) {
            publish.roomId = status.roomId;
            publish.inviteCode = status.inviteCode;
        }
        if ( lastUpdate ) {
            publish.state = lastUpdate->state;
            publish.game = lastUpdate->game;
        }
        return publish;
    }

    void connect()
    {
        connection = std::make_unique<LobbyConnection>();
        connection->open( lobbyAddress );
        connection->send( makeLobbyHello( makeLobbyHelloOfThisGame( nickname ) ) );

        isReclaiming = ( status.roomId != 0 );
        connection->send( makeRoomPublish( makePublish( isReclaiming ) ) );
        isUpdateSent = true;
    }

    void fail( std::string reason )
    {
        if ( state == State::FAILED ) {
            return;
        }

        ERROR_LOG( "[MULTI] The lobby refused this game: " << reason )

        state = State::FAILED;
        error = std::move( reason );
        if ( connection ) {
            connection->close();
        }
    }

    void lose( std::string reason )
    {
        constexpr std::array<int, 5> delays{ 1, 2, 5, 10, 30 };

        if ( attempts == 0 ) {
            ERROR_LOG( "[MULTI] No connection to the lobby (" << reason << "), connecting again" )
        }

        error = std::move( reason );
        connection.reset();
        nextAttempt = std::chrono::steady_clock::now() + std::chrono::seconds( delays[std::min<size_t>( attempts, delays.size() - 1 )] );
        ++attempts;
    }

    void onStatus( const RoomStatus & newStatus )
    {
        if ( newStatus.result != RoomStatusResult::PUBLISHED ) {
            if ( !isReclaiming ) {
                fail( describeRoomRefusal( newStatus.result ) );
                return;
            }

            // The lobby does not give the room back: a new one.
            VERBOSE_LOG( "[MULTI] The lobby does not give the room of this game back, publishing it again" )

            isReclaiming = false;
            status = {};
            connection->send( makeRoomPublish( makePublish( false ) ) );
            isUpdateSent = true;
            return;
        }

        if ( state != State::PUBLISHED || isReclaiming ) {
            VERBOSE_LOG( "[MULTI] This game is " << ( isReclaiming ? "back" : "published" ) << " on the lobby " << welcome.lobbyName )
            ++publishCount;
        }
        if ( newStatus.reach.isProbeDone && !status.reach.isProbeDone ) {
            VERBOSE_LOG( "[MULTI] The lobby can reach this game directly: " << ( newStatus.reach.isIPv4 ? "IPv4" : "" ) << ( newStatus.reach.isIPv6 ? " IPv6" : "" )
                                                                            << ( !newStatus.reach.isIPv4 && !newStatus.reach.isIPv6 ? "no" : "" ) )
        }

        isReclaiming = false;
        attempts = 0;
        error.clear();
        status = newStatus;
        state = State::PUBLISHED;

        // Changed while the room was being published.
        if ( !isUpdateSent && lastUpdate ) {
            connection->send( makeRoomUpdate( *lastUpdate ) );
            isUpdateSent = true;
        }
    }

    void onMessage( const Message & message )
    {
        switch ( message.type ) {
        case MessageType::LOBBY_WELCOME: {
            const std::optional<LobbyWelcome> welcomeMessage = readLobbyWelcome( message );
            if ( !welcomeMessage ) {
                lose( gettext_noop( "the lobby sent wrong data" ) );
                return;
            }

            welcome = *welcomeMessage;
            if ( welcome.result != LobbyWelcomeResult::ACCEPTED ) {
                fail( describeWelcomeRefusal( welcome.result ) );
            }
            return;
        }
        case MessageType::ROOM_STATUS: {
            const std::optional<RoomStatus> newStatus = readRoomStatus( message );
            if ( !newStatus ) {
                lose( gettext_noop( "the lobby sent wrong data" ) );
                return;
            }

            onStatus( *newStatus );
            return;
        }
        default:
            // The stats and such are of no use here.
            return;
        }
    }
};

Multi::PublishedRoom::PublishedRoom( const std::string & lobbyAddress, const std::string & nickname, const RoomPublish & room )
    : _impl( std::make_unique<Impl>() )
{
    _impl->lobbyAddress = lobbyAddress;
    _impl->nickname = nickname;
    _impl->room = room;

    // Keys of the room, chosen here so that the room can come back as it was after a restart of the lobby.
    const LobbyKey noKey{};
    if ( _impl->room.hostToken == noKey ) {
        _impl->room.hostToken = makeRandomLobbyKey();
    }
    if ( _impl->room.guestTicket == noKey ) {
        _impl->room.guestTicket = makeRandomLobbyKey();
    }

    _impl->connect();
}

Multi::PublishedRoom::~PublishedRoom()
{
    try {
        if ( _impl->connection ) {
            if ( _impl->connection->state() == LobbyConnection::State::OPEN ) {
                _impl->connection->send( makeEmptyLobbyMessage( MessageType::ROOM_CLOSE ) );
            }
            _impl->connection->close();
        }
    }
    catch ( ... ) {
        // Nothing may leave a destructor: the lobby closes the room by itself a little later.
        _impl.reset();
    }
}

void Multi::PublishedRoom::update()
{
    Impl & impl = *_impl;
    if ( impl.state == State::FAILED ) {
        return;
    }

    if ( !impl.connection ) {
        if ( std::chrono::steady_clock::now() >= impl.nextAttempt ) {
            impl.connect();
        }
        return;
    }

    impl.connection->update();

    while ( impl.connection ) {
        const std::optional<Message> message = impl.connection->receive();
        if ( !message ) {
            break;
        }
        impl.onMessage( *message );
    }

    if ( impl.state != State::FAILED && impl.connection && impl.connection->state() == LobbyConnection::State::CLOSED ) {
        impl.lose( impl.connection->error().empty() ? gettext_noop( "the lobby closed the connection" ) : impl.connection->error() );
    }
}

Multi::PublishedRoom::State Multi::PublishedRoom::state() const
{
    return _impl->state;
}

bool Multi::PublishedRoom::isConnected() const
{
    return _impl->connection && _impl->connection->state() == LobbyConnection::State::OPEN && !_impl->isReclaiming;
}

const std::string & Multi::PublishedRoom::error() const
{
    return _impl->error;
}

const Multi::RoomStatus & Multi::PublishedRoom::status() const
{
    return _impl->status;
}

const Multi::LobbyWelcome & Multi::PublishedRoom::lobby() const
{
    return _impl->welcome;
}

const std::string & Multi::PublishedRoom::lobbyAddress() const
{
    return _impl->lobbyAddress;
}

const Multi::LobbyKey & Multi::PublishedRoom::hostToken() const
{
    return _impl->room.hostToken;
}

uint32_t Multi::PublishedRoom::publishCount() const
{
    return _impl->publishCount;
}

void Multi::PublishedRoom::setState( const RoomState state, const RoomGame & game )
{
    Impl & impl = *_impl;
    impl.lastUpdate = RoomUpdate{ state, game };
    impl.isUpdateSent = ( impl.state == State::PUBLISHED && isConnected() );

    if ( impl.isUpdateSent ) {
        impl.connection->send( makeRoomUpdate( *impl.lastUpdate ) );
    }
}

void Multi::PublishedRoom::cutForTest()
{
    if ( _impl->connection ) {
        _impl->connection->close();
        _impl->lose( "connection cut for a self-test" );
    }
}

std::string Multi::getRelayAddress( const std::string & lobbyAddress, const LobbyWelcome & welcome )
{
    return welcome.relayAddress.empty() ? lobbyAddress : welcome.relayAddress;
}

void Multi::listenOnRelayOfRoom( const PublishedRoom & room, NetworkSession & session )
{
    const RoomStatus & status = room.status();
    if ( room.state() == PublishedRoom::State::PUBLISHED && status.reach.isRelayAvailable ) {
        session.listenOnRelay( getRelayAddress( room.lobbyAddress(), room.lobby() ), status.roomId, room.hostToken(), room.publishCount() );
    }
}

Multi::LobbyKey Multi::makeRandomLobbyKey()
{
    std::random_device device;
    std::mt19937 generator( device() );

    LobbyKey key{};
    for ( uint8_t & byte : key ) {
        byte = static_cast<uint8_t>( generator() );
    }

    return key;
}

Multi::RoomGame Multi::makeRoomGameOfCurrentGame( const uint8_t guestHumanPlayers )
{
    const Maps::FileInfo & map = Settings::Get().getCurrentMapInfo();

    RoomGame game;

    // Cut to what the list can show.
    game.mapName = toUtf8( map.name );
    while ( !game.mapName.empty() && !isValidMapName( game.mapName ) ) {
        game.mapName.pop_back();
    }

    game.mapWidth = map.width;
    game.mapHeight = map.height;

    for ( const Player * player : Settings::Get().GetPlayers().getVector() ) {
        ++game.players;
        if ( player->isControlHuman() ) {
            ++game.humanPlayers;
        }
    }

    game.guestHumanPlayers = std::min( guestHumanPlayers, game.humanPlayers );
    game.turns = areSimultaneousTurnsEnabled() ? RoomTurns::SIMULTANEOUS : RoomTurns::ONE_AFTER_ANOTHER;
    game.day = std::max<uint32_t>( 1, world.CountDay() );

    return game;
}

Multi::LobbyHello Multi::makeLobbyHelloOfThisGame( const std::string & nickname )
{
    return { lobbyProtocolVersion, getLocalHandshakeInfo(), fheroes2::getLanguageAbbreviation( fheroes2::getCurrentLanguage() ), nickname };
}
