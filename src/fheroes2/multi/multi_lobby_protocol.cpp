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

#include "multi_lobby_protocol.h"

#include <algorithm>
#include <string_view>

#include "multi_payload.h"
#include "serialize.h"

namespace
{
    struct TextLimit
    {
        uint32_t maxBytes{ 0 };
        uint32_t minCharacters{ 0 };
        uint32_t maxCharacters{ 0 };
        bool isAsciiOnly{ false };
    };

    constexpr TextLimit nicknameLimit{ 64, 1, 20, false };
    constexpr TextLimit gameNameLimit{ 120, 1, 40, false };
    constexpr TextLimit mapNameLimit{ 120, 0, 40, false };
    constexpr TextLimit lobbyNameLimit{ 120, 0, 40, false };
    constexpr TextLimit messageOfTheDayLimit{ 1000, 0, 300, false };
    constexpr TextLimit platformLimit{ 32, 0, 32, false };
    constexpr TextLimit languageLimit{ 8, 0, 8, true };
    constexpr TextLimit addressLimit{ 64, 1, 64, true };
    constexpr TextLimit relayAddressLimit{ 120, 0, 120, true };
    constexpr TextLimit chatLimit{ 400, 1, 120, false };

    constexpr std::string_view inviteCodeCharacters{ "ABCDEFGHJKLMNPQRSTUVWXYZ23456789" };
    constexpr size_t inviteCodeSize{ 6 };

    constexpr uint32_t maxPublicAddresses{ 3 };
    constexpr uint32_t maxLocalAddresses{ 8 };
    constexpr uint32_t maxJoinCandidates{ 8 };

    // Number of characters, or nothing for wrong UTF-8 or a control character.
    std::optional<uint32_t> countCharacters( const std::string & text )
    {
        // The smallest code point for each length, anything below is an overlong form.
        constexpr std::array<uint32_t, 5> minCodePoints{ 0, 0, 0x80, 0x800, 0x10000 };

        uint32_t count = 0;
        size_t position = 0;

        while ( position < text.size() ) {
            const auto first = static_cast<uint8_t>( text[position] );

            uint32_t codePoint = 0;
            size_t length = 0;

            if ( first < 0x80 ) {
                codePoint = first;
                length = 1;
            }
            else if ( ( first & 0xE0 ) == 0xC0 ) {
                codePoint = first & 0x1FU;
                length = 2;
            }
            else if ( ( first & 0xF0 ) == 0xE0 ) {
                codePoint = first & 0x0FU;
                length = 3;
            }
            else if ( ( first & 0xF8 ) == 0xF0 ) {
                codePoint = first & 0x07U;
                length = 4;
            }
            else {
                return {};
            }

            if ( length > text.size() - position ) {
                return {};
            }

            for ( size_t i = 1; i < length; ++i ) {
                const auto next = static_cast<uint8_t>( text[position + i] );
                if ( ( next & 0xC0 ) != 0x80 ) {
                    return {};
                }

                codePoint = ( codePoint << 6 ) | ( next & 0x3FU );
            }

            const bool isSurrogate = ( codePoint >= 0xD800 && codePoint <= 0xDFFF );
            const bool isControl = ( codePoint < 0x20 || ( codePoint >= 0x7F && codePoint <= 0x9F ) );

            if ( codePoint < minCodePoints[length] || codePoint > 0x10FFFF || isSurrogate || isControl ) {
                return {};
            }

            position += length;
            ++count;
        }

        return count;
    }

    bool isValidText( const std::string & text, const TextLimit & limit )
    {
        if ( text.size() > limit.maxBytes ) {
            return false;
        }

        if ( limit.isAsciiOnly && std::any_of( text.begin(), text.end(), []( const char c ) { return static_cast<uint8_t>( c ) >= 0x80; } ) ) {
            return false;
        }

        const std::optional<uint32_t> count = countCharacters( text );

        return count && *count >= limit.minCharacters && *count <= limit.maxCharacters;
    }

    bool readText( ROStreamBuf & stream, std::string & text, const TextLimit & limit )
    {
        return Multi::readString( stream, text, limit.maxBytes ) && isValidText( text, limit );
    }

    bool readBool( ROStreamBuf & stream, bool & value )
    {
        uint8_t byte = 0;
        stream >> byte;

        value = ( byte == 1 );

        return !stream.fail() && byte <= 1;
    }

    // Reads an enumeration stored in one byte, from 0 to the given last value.
    template <typename T>
    bool readEnum( ROStreamBuf & stream, T & value, const T last )
    {
        uint8_t byte = 0;
        stream >> byte;

        value = static_cast<T>( byte );

        return !stream.fail() && byte <= static_cast<uint8_t>( last );
    }

    void writeKey( OStreamBase & stream, const Multi::LobbyKey & key )
    {
        stream.putRaw( key.data(), key.size() );
    }

    bool readKey( ROStreamBuf & stream, Multi::LobbyKey & key )
    {
        if ( stream.size() < key.size() ) {
            return false;
        }

        const std::vector<uint8_t> bytes = stream.getRaw( key.size() );
        std::copy( bytes.begin(), bytes.end(), key.begin() );

        return true;
    }

    bool readCount( ROStreamBuf & stream, uint32_t & count, const uint32_t maxCount )
    {
        stream >> count;

        return !stream.fail() && count <= maxCount;
    }

    void writeTexts( OStreamBase & stream, const std::vector<std::string> & texts )
    {
        stream << static_cast<uint32_t>( texts.size() );

        for ( const std::string & text : texts ) {
            stream << text;
        }
    }

    bool readTexts( ROStreamBuf & stream, std::vector<std::string> & texts, const uint32_t maxCount, const TextLimit & limit )
    {
        uint32_t count = 0;
        if ( !readCount( stream, count, maxCount ) ) {
            return false;
        }

        texts.resize( count );

        return std::all_of( texts.begin(), texts.end(), [&stream, &limit]( std::string & text ) { return readText( stream, text, limit ); } );
    }

    void writeGameVersion( OStreamBase & stream, const Multi::HandshakeInfo & info )
    {
        for ( const uint8_t number : info.gameVersion ) {
            stream << number;
        }

        stream << info.protocolVersion;
        Multi::writeUint64( stream, info.buildId );
        stream << info.platform;
    }

    bool readGameVersion( ROStreamBuf & stream, Multi::HandshakeInfo & info )
    {
        for ( uint8_t & number : info.gameVersion ) {
            stream >> number;
        }

        stream >> info.protocolVersion;
        info.buildId = Multi::readUint64( stream );

        return !stream.fail() && readText( stream, info.platform, platformLimit );
    }

    uint8_t toByte( const Multi::RoomReach & reach )
    {
        return static_cast<uint8_t>( ( reach.isIPv4 ? 1 : 0 ) | ( reach.isIPv6 ? 2 : 0 ) | ( reach.isProbeDone ? 4 : 0 ) | ( reach.isRelayAvailable ? 8 : 0 ) );
    }

    bool readReach( ROStreamBuf & stream, Multi::RoomReach & reach )
    {
        uint8_t byte = 0;
        stream >> byte;

        reach.isIPv4 = ( byte & 1 ) != 0;
        reach.isIPv6 = ( byte & 2 ) != 0;
        reach.isProbeDone = ( byte & 4 ) != 0;
        reach.isRelayAvailable = ( byte & 8 ) != 0;

        return !stream.fail() && byte < 16;
    }

    void writeRoomGame( OStreamBase & stream, const Multi::RoomGame & game )
    {
        stream << game.mapName << game.mapWidth << game.mapHeight << game.players << game.humanPlayers << game.guestHumanPlayers << static_cast<uint8_t>( game.turns )
               << game.day;
    }

    bool readRoomGame( ROStreamBuf & stream, Multi::RoomGame & game )
    {
        if ( !readText( stream, game.mapName, mapNameLimit ) ) {
            return false;
        }

        stream >> game.mapWidth >> game.mapHeight >> game.players >> game.humanPlayers >> game.guestHumanPlayers;

        if ( !readEnum( stream, game.turns, Multi::RoomTurns::SIMULTANEOUS ) ) {
            return false;
        }

        stream >> game.day;

        return !stream.fail() && game.mapWidth > 0 && game.mapHeight > 0 && game.humanPlayers <= game.players && game.guestHumanPlayers <= game.humanPlayers
               && game.day > 0;
    }

    void writeRoomFields( OStreamBase & stream, const Multi::RoomInfo & room )
    {
        Multi::writeUint64( stream, room.roomId );
        stream << static_cast<uint8_t>( room.state ) << toByte( room.reach ) << room.gameName << room.hostNickname << room.hostLanguage;
        writeGameVersion( stream, room.hostGame );
        writeRoomGame( stream, room.game );
    }

    bool readRoomFields( ROStreamBuf & stream, Multi::RoomInfo & room )
    {
        room.roomId = Multi::readUint64( stream );

        return !stream.fail() && room.roomId != 0 && readEnum( stream, room.state, Multi::RoomState::IN_GAME ) && readReach( stream, room.reach )
               && readText( stream, room.gameName, gameNameLimit ) && readText( stream, room.hostNickname, nicknameLimit )
               && readText( stream, room.hostLanguage, languageLimit ) && readGameVersion( stream, room.hostGame ) && readRoomGame( stream, room.game );
    }

    Multi::Message makeMessage( const Multi::MessageType type, const RWStreamBuf & stream )
    {
        return { type, Multi::toPayload( stream ) };
    }

    // The whole payload must have been read.
    bool isFullyRead( ROStreamBuf & stream )
    {
        return !stream.fail() && stream.size() == 0;
    }
}

bool Multi::isValidNickname( const std::string & nickname )
{
    return isValidText( nickname, nicknameLimit );
}

bool Multi::isValidGameName( const std::string & name )
{
    return isValidText( name, gameNameLimit );
}

bool Multi::isValidMapName( const std::string & name )
{
    return isValidText( name, mapNameLimit );
}

bool Multi::isValidInviteCode( const std::string & code )
{
    return code.size() == inviteCodeSize
           && std::all_of( code.begin(), code.end(), []( const char c ) { return inviteCodeCharacters.find( c ) != std::string_view::npos; } );
}

Multi::Message Multi::makeLobbyHello( const LobbyHello & hello )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << hello.lobbyProtocolVersion;
    writeGameVersion( stream, hello.game );
    stream << hello.language << hello.nickname;

    return makeMessage( MessageType::LOBBY_HELLO, stream );
}

std::optional<Multi::LobbyHello> Multi::readLobbyHello( const Message & message )
{
    if ( message.type != MessageType::LOBBY_HELLO ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    LobbyHello hello;
    stream >> hello.lobbyProtocolVersion;

    if ( !readGameVersion( stream, hello.game ) || !readText( stream, hello.language, languageLimit ) || !readText( stream, hello.nickname, nicknameLimit )
         || !isFullyRead( stream ) ) {
        return {};
    }

    return hello;
}

Multi::Message Multi::makeLobbyWelcome( const LobbyWelcome & welcome )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << static_cast<uint8_t>( welcome.result ) << welcome.lobbyProtocolVersion << welcome.lobbyName << welcome.messageOfTheDay << welcome.relayAddress;

    return makeMessage( MessageType::LOBBY_WELCOME, stream );
}

std::optional<Multi::LobbyWelcome> Multi::readLobbyWelcome( const Message & message )
{
    if ( message.type != MessageType::LOBBY_WELCOME ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    LobbyWelcome welcome;
    if ( !readEnum( stream, welcome.result, LobbyWelcomeResult::TOO_MANY_CONNECTIONS ) ) {
        return {};
    }

    stream >> welcome.lobbyProtocolVersion;

    if ( stream.fail() || !readText( stream, welcome.lobbyName, lobbyNameLimit ) || !readText( stream, welcome.messageOfTheDay, messageOfTheDayLimit )
         || !readText( stream, welcome.relayAddress, relayAddressLimit ) || !isFullyRead( stream ) ) {
        return {};
    }

    return welcome;
}

Multi::Message Multi::makeLobbyStats( const LobbyStats & stats )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << stats.playersOnline << stats.waitingRooms << stats.roomsInGame;

    return makeMessage( MessageType::LOBBY_STATS, stream );
}

std::optional<Multi::LobbyStats> Multi::readLobbyStats( const Message & message )
{
    if ( message.type != MessageType::LOBBY_STATS ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    LobbyStats stats;
    stream >> stats.playersOnline >> stats.waitingRooms >> stats.roomsInGame;

    if ( !isFullyRead( stream ) ) {
        return {};
    }

    return stats;
}

Multi::Message Multi::makeRoomPublish( const RoomPublish & publish )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << static_cast<uint8_t>( publish.visibility ) << static_cast<uint8_t>( publish.isRelayOnly ? 1 : 0 ) << publish.gameName;
    writeRoomGame( stream, publish.game );
    stream << publish.gamePort;
    writeKey( stream, publish.probeKey );
    writeTexts( stream, publish.publicAddresses );
    writeTexts( stream, publish.localAddresses );
    writeUint64( stream, publish.roomId );
    stream << publish.inviteCode << static_cast<uint8_t>( publish.state );
    writeKey( stream, publish.hostToken );
    writeKey( stream, publish.guestTicket );

    return makeMessage( MessageType::ROOM_PUBLISH, stream );
}

std::optional<Multi::RoomPublish> Multi::readRoomPublish( const Message & message )
{
    if ( message.type != MessageType::ROOM_PUBLISH ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    RoomPublish publish;
    if ( !readEnum( stream, publish.visibility, RoomVisibility::INVITE_ONLY ) || !readBool( stream, publish.isRelayOnly )
         || !readText( stream, publish.gameName, gameNameLimit ) || !readRoomGame( stream, publish.game ) ) {
        return {};
    }

    stream >> publish.gamePort;

    if ( stream.fail() || publish.gamePort == 0 || !readKey( stream, publish.probeKey ) || !readTexts( stream, publish.publicAddresses, maxPublicAddresses, addressLimit )
         || !readTexts( stream, publish.localAddresses, maxLocalAddresses, addressLimit ) ) {
        return {};
    }

    publish.roomId = readUint64( stream );

    if ( stream.fail() || !readString( stream, publish.inviteCode, inviteCodeSize ) || !readEnum( stream, publish.state, RoomState::IN_GAME )
         || !readKey( stream, publish.hostToken ) || !readKey( stream, publish.guestTicket ) || !isFullyRead( stream ) ) {
        return {};
    }

    // Both a room id and its invite code, or none.
    const bool isNew = ( publish.roomId == 0 && publish.inviteCode.empty() );
    const bool isAgain = ( publish.roomId != 0 && isValidInviteCode( publish.inviteCode ) );
    if ( !isNew && !isAgain ) {
        return {};
    }

    return publish;
}

Multi::Message Multi::makeRoomStatus( const RoomStatus & status )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << static_cast<uint8_t>( status.result );

    if ( status.result == RoomStatusResult::PUBLISHED ) {
        writeUint64( stream, status.roomId );
        stream << status.inviteCode << toByte( status.reach );
    }

    return makeMessage( MessageType::ROOM_STATUS, stream );
}

std::optional<Multi::RoomStatus> Multi::readRoomStatus( const Message & message )
{
    if ( message.type != MessageType::ROOM_STATUS ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    RoomStatus status;
    if ( !readEnum( stream, status.result, RoomStatusResult::TOO_MANY_ROOMS ) ) {
        return {};
    }

    if ( status.result == RoomStatusResult::PUBLISHED ) {
        status.roomId = readUint64( stream );

        if ( stream.fail() || status.roomId == 0 || !readString( stream, status.inviteCode, inviteCodeSize ) || !isValidInviteCode( status.inviteCode )
             || !readReach( stream, status.reach ) ) {
            return {};
        }
    }

    if ( !isFullyRead( stream ) ) {
        return {};
    }

    return status;
}

Multi::Message Multi::makeRoomUpdate( const RoomUpdate & update )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << static_cast<uint8_t>( update.state );
    writeRoomGame( stream, update.game );

    return makeMessage( MessageType::ROOM_UPDATE, stream );
}

std::optional<Multi::RoomUpdate> Multi::readRoomUpdate( const Message & message )
{
    if ( message.type != MessageType::ROOM_UPDATE ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    RoomUpdate update;
    if ( !readEnum( stream, update.state, RoomState::IN_GAME ) || !readRoomGame( stream, update.game ) || !isFullyRead( stream ) ) {
        return {};
    }

    return update;
}

Multi::Message Multi::makeEmptyLobbyMessage( const MessageType type )
{
    return { type, {} };
}

bool Multi::readEmptyLobbyMessage( const Message & message, const MessageType type )
{
    return message.type == type && message.payload.empty();
}

Multi::Message Multi::makeRoomListSubscribe( const bool isSubscribed )
{
    return { MessageType::ROOM_LIST_SUBSCRIBE, { static_cast<uint8_t>( isSubscribed ? 1 : 0 ) } };
}

std::optional<bool> Multi::readRoomListSubscribe( const Message & message )
{
    if ( message.type != MessageType::ROOM_LIST_SUBSCRIBE || message.payload.size() != 1 || message.payload[0] > 1 ) {
        return {};
    }

    return message.payload[0] == 1;
}

Multi::Message Multi::makeRoomList( const RoomList & list )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << static_cast<uint8_t>( list.isLastPart ? 1 : 0 ) << static_cast<uint32_t>( list.rooms.size() );

    for ( const RoomInfo & room : list.rooms ) {
        writeRoomFields( stream, room );
    }

    return makeMessage( MessageType::ROOM_LIST, stream );
}

std::optional<Multi::RoomList> Multi::readRoomList( const Message & message )
{
    if ( message.type != MessageType::ROOM_LIST ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    RoomList list;
    uint32_t count = 0;

    if ( !readBool( stream, list.isLastPart ) || !readCount( stream, count, maxRoomsPerListMessage ) ) {
        return {};
    }

    list.rooms.resize( count );

    if ( !std::all_of( list.rooms.begin(), list.rooms.end(), [&stream]( RoomInfo & room ) { return readRoomFields( stream, room ); } ) || !isFullyRead( stream ) ) {
        return {};
    }

    return list;
}

Multi::Message Multi::makeRoomInfo( const RoomInfo & room )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    writeRoomFields( stream, room );

    return makeMessage( MessageType::ROOM_INFO, stream );
}

std::optional<Multi::RoomInfo> Multi::readRoomInfo( const Message & message )
{
    if ( message.type != MessageType::ROOM_INFO ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    RoomInfo room;
    if ( !readRoomFields( stream, room ) || !isFullyRead( stream ) ) {
        return {};
    }

    return room;
}

Multi::Message Multi::makeRoomRemoved( const uint64_t roomId )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    writeUint64( stream, roomId );

    return makeMessage( MessageType::ROOM_REMOVED, stream );
}

std::optional<uint64_t> Multi::readRoomRemoved( const Message & message )
{
    if ( message.type != MessageType::ROOM_REMOVED ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    const uint64_t roomId = readUint64( stream );
    if ( !isFullyRead( stream ) || roomId == 0 ) {
        return {};
    }

    return roomId;
}

Multi::Message Multi::makeRoomJoin( const RoomJoin & join )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    writeUint64( stream, join.roomId );
    stream << join.inviteCode;

    return makeMessage( MessageType::ROOM_JOIN, stream );
}

std::optional<Multi::RoomJoin> Multi::readRoomJoin( const Message & message )
{
    if ( message.type != MessageType::ROOM_JOIN ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    RoomJoin join;
    join.roomId = readUint64( stream );

    if ( stream.fail() || !readString( stream, join.inviteCode, inviteCodeSize ) || !isFullyRead( stream ) ) {
        return {};
    }

    // Either a room id or an invite code.
    const bool isByRoomId = ( join.roomId != 0 && join.inviteCode.empty() );
    const bool isByCode = ( join.roomId == 0 && isValidInviteCode( join.inviteCode ) );
    if ( !isByRoomId && !isByCode ) {
        return {};
    }

    return join;
}

Multi::Message Multi::makeJoinPlan( const JoinPlan & plan )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << static_cast<uint8_t>( plan.result );

    if ( plan.result == JoinResult::READY ) {
        writeRoomFields( stream, plan.room );

        stream << static_cast<uint32_t>( plan.candidates.size() );
        for ( const JoinCandidate & candidate : plan.candidates ) {
            stream << static_cast<uint8_t>( candidate.kind ) << candidate.address << candidate.port;
        }

        writeKey( stream, plan.relayTicket );
    }

    return makeMessage( MessageType::JOIN_PLAN, stream );
}

std::optional<Multi::JoinPlan> Multi::readJoinPlan( const Message & message )
{
    if ( message.type != MessageType::JOIN_PLAN ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    JoinPlan plan;
    if ( !readEnum( stream, plan.result, JoinResult::INVALID_MESSAGE ) ) {
        return {};
    }

    if ( plan.result == JoinResult::READY ) {
        uint32_t count = 0;
        if ( !readRoomFields( stream, plan.room ) || !readCount( stream, count, maxJoinCandidates ) ) {
            return {};
        }

        plan.candidates.resize( count );

        for ( JoinCandidate & candidate : plan.candidates ) {
            if ( !readEnum( stream, candidate.kind, JoinCandidateKind::IPV4 ) || !readText( stream, candidate.address, addressLimit ) ) {
                return {};
            }

            stream >> candidate.port;

            if ( stream.fail() || candidate.port == 0 ) {
                return {};
            }
        }

        if ( !readKey( stream, plan.relayTicket ) ) {
            return {};
        }
    }

    if ( !isFullyRead( stream ) ) {
        return {};
    }

    return plan;
}

Multi::Message Multi::makeLobbyKeyMessage( const MessageType type, const LobbyKey & key )
{
    return { type, { key.begin(), key.end() } };
}

std::optional<Multi::LobbyKey> Multi::readLobbyKeyMessage( const Message & message, const MessageType type )
{
    if ( message.type != type || message.payload.size() != LobbyKey().size() ) {
        return {};
    }

    LobbyKey key{};
    std::copy( message.payload.begin(), message.payload.end(), key.begin() );

    return key;
}

Multi::Message Multi::makeRelayRefused( const RelayRefusal reason )
{
    return { MessageType::RELAY_REFUSED, { static_cast<uint8_t>( reason ) } };
}

std::optional<Multi::RelayRefusal> Multi::readRelayRefused( const Message & message )
{
    if ( message.type != MessageType::RELAY_REFUSED || message.payload.size() != 1 || message.payload[0] > static_cast<uint8_t>( RelayRefusal::ROOM_CLOSED ) ) {
        return {};
    }

    return static_cast<RelayRefusal>( message.payload[0] );
}

Multi::Message Multi::makeLobbyProbe( const LobbyProbe & probe )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    writeKey( stream, probe.probeKey );
    writeKey( stream, probe.value );

    return makeMessage( MessageType::LOBBY_PROBE, stream );
}

std::optional<Multi::LobbyProbe> Multi::readLobbyProbe( const Message & message )
{
    if ( message.type != MessageType::LOBBY_PROBE ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    LobbyProbe probe;
    if ( !readKey( stream, probe.probeKey ) || !readKey( stream, probe.value ) || !isFullyRead( stream ) ) {
        return {};
    }

    return probe;
}

Multi::Message Multi::makeWaitingRoom( const WaitingRoomInfo & info )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << info.gameName;
    writeRoomGame( stream, info.game );
    stream << info.humanColors << info.computerColors << static_cast<uint8_t>( info.players.size() );

    for ( const RoomPlayer & player : info.players ) {
        stream << player.machine << player.nickname << player.colors;
    }

    stream << info.receiver;

    return makeMessage( MessageType::WAITING_ROOM, stream );
}

std::optional<Multi::WaitingRoomInfo> Multi::readWaitingRoom( const Message & message )
{
    if ( message.type != MessageType::WAITING_ROOM ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    WaitingRoomInfo info;
    uint8_t playerCount = 0;
    if ( !readText( stream, info.gameName, gameNameLimit ) || !readRoomGame( stream, info.game ) ) {
        return {};
    }

    stream >> info.humanColors >> info.computerColors >> playerCount;

    // Six player colors, each one human or computer, and the host with one guest at least.
    if ( stream.fail() || ( ( info.humanColors | info.computerColors ) & ~allPlayerColors ) != 0 || ( info.humanColors & info.computerColors ) != 0 || playerCount < 2
         || playerCount > maxGuests + 1 ) {
        return {};
    }

    uint8_t takenColors = 0;

    for ( uint8_t i = 0; i < playerCount; ++i ) {
        RoomPlayer player;
        stream >> player.machine;

        if ( !readString( stream, player.nickname, nicknameLimit.maxBytes ) || ( !player.nickname.empty() && !isValidNickname( player.nickname ) ) ) {
            return {};
        }

        stream >> player.colors;

        const bool isInOrder = info.players.empty() ? ( player.machine == hostMachine ) : ( player.machine > info.players.back().machine );
        if ( stream.fail() || !isInOrder || player.machine > maxMachineId || ( player.colors & ~info.humanColors ) != 0 || ( player.colors & takenColors ) != 0 ) {
            return {};
        }

        takenColors |= player.colors;
        info.players.push_back( std::move( player ) );
    }

    stream >> info.receiver;

    const bool isReceiverIn = std::any_of( info.players.begin(), info.players.end(), [&info]( const RoomPlayer & player ) { return player.machine == info.receiver; } );

    if ( !isFullyRead( stream ) || info.receiver == hostMachine || !isReceiverIn ) {
        return {};
    }

    return info;
}

Multi::Message Multi::makePlayerInfo( const uint8_t wantedColors )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << wantedColors;

    return makeMessage( MessageType::PLAYER_INFO, stream );
}

std::optional<uint8_t> Multi::readPlayerInfo( const Message & message )
{
    if ( message.type != MessageType::PLAYER_INFO ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    uint8_t wantedColors = 0;
    stream >> wantedColors;

    if ( !isFullyRead( stream ) || wantedColors == 0 || ( wantedColors & ~allPlayerColors ) != 0 ) {
        return {};
    }

    return wantedColors;
}

bool Multi::isValidChatText( const std::string & text )
{
    return isValidText( text, chatLimit );
}

Multi::Message Multi::makeChat( const std::string & text )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << text;

    return makeMessage( MessageType::CHAT, stream );
}

std::optional<std::string> Multi::readChat( const Message & message )
{
    if ( message.type != MessageType::CHAT ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    std::string text;
    if ( !readText( stream, text, chatLimit ) || !isFullyRead( stream ) ) {
        return {};
    }

    return text;
}
