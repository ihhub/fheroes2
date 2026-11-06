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

#include "multi_protocol.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iomanip>
#include <sstream>

#include "multi_build_id.h"
#include "multi_lobby_protocol.h"
#include "multi_payload.h"
#include "serialize.h"
#include "translations.h"
#include "version.h"

using Multi::readString;
using Multi::readUint64;
using Multi::toPayload;
using Multi::writeUint64;

namespace
{
    constexpr std::array<uint8_t, 3> messageMarker{ 'f', 'h', '2' };

    // Never trust a length coming from the network.
    constexpr uint32_t maxStringSize{ 256 };

    void writeUint32( uint8_t * out, const uint32_t value )
    {
        out[0] = static_cast<uint8_t>( value >> 24 );
        out[1] = static_cast<uint8_t>( value >> 16 );
        out[2] = static_cast<uint8_t>( value >> 8 );
        out[3] = static_cast<uint8_t>( value );
    }

    uint32_t readUint32( const uint8_t * in )
    {
        return ( static_cast<uint32_t>( in[0] ) << 24 ) | ( static_cast<uint32_t>( in[1] ) << 16 ) | ( static_cast<uint32_t>( in[2] ) << 8 )
               | static_cast<uint32_t>( in[3] );
    }

    void writeHandshakeInfo( OStreamBase & stream, const Multi::HandshakeInfo & info )
    {
        for ( const uint8_t number : info.gameVersion ) {
            stream << number;
        }

        stream << info.protocolVersion;
        writeUint64( stream, info.buildId );
        stream << info.platform;
        writeUint64( stream, info.machineKey );
        stream << info.nickname << info.claimedColors;
    }

    bool readHandshakeInfo( ROStreamBuf & stream, Multi::HandshakeInfo & info )
    {
        for ( uint8_t & number : info.gameVersion ) {
            stream >> number;
        }

        stream >> info.protocolVersion;
        info.buildId = readUint64( stream );

        if ( !readString( stream, info.platform, maxStringSize ) ) {
            return false;
        }

        info.machineKey = readUint64( stream );

        if ( !readString( stream, info.nickname, maxStringSize ) || ( !info.nickname.empty() && !Multi::isValidNickname( info.nickname ) ) ) {
            return false;
        }

        stream >> info.claimedColors;

        return !stream.fail() && ( info.claimedColors & ~Multi::allPlayerColors ) == 0;
    }

    std::string describeBuild( const Multi::HandshakeInfo & info )
    {
        std::ostringstream os;
        os << static_cast<int>( info.gameVersion[0] ) << '.' << static_cast<int>( info.gameVersion[1] ) << '.' << static_cast<int>( info.gameVersion[2] ) << ' '
           << info.platform << " [" << std::hex << std::setw( 16 ) << std::setfill( '0' ) << info.buildId << "]";

        return os.str();
    }

    std::string describeOtherVersion( const Multi::HandshakeInfo & local, const Multi::HandshakeInfo & remote )
    {
        return std::string( gettext_noop( "the players do not run the same version of fheroes2 (this computer / the other player)" ) ) + ": " + describeBuild( local )
               + " / " + describeBuild( remote );
    }
}

std::vector<uint8_t> Multi::encodeMessage( const Message & message )
{
    assert( message.payload.size() <= maxMessagePayloadSize );

    std::vector<uint8_t> data( messageHeaderSize + message.payload.size() );

    std::copy( messageMarker.begin(), messageMarker.end(), data.begin() );
    writeUint32( data.data() + 3, static_cast<uint32_t>( message.type ) );
    writeUint32( data.data() + 7, static_cast<uint32_t>( message.payload.size() ) );
    std::copy( message.payload.begin(), message.payload.end(), data.begin() + messageHeaderSize );

    return data;
}

bool Multi::decodeMessageHeader( const uint8_t * header, MessageType & type, uint32_t & payloadSize )
{
    if ( !std::equal( messageMarker.begin(), messageMarker.end(), header ) ) {
        return false;
    }

    type = static_cast<MessageType>( readUint32( header + 3 ) );
    payloadSize = readUint32( header + 7 );

    return payloadSize <= maxMessagePayloadSize;
}

bool Multi::isSessionMessage( const MessageType type )
{
    switch ( type ) {
    case MessageType::SESSION_ID:
    case MessageType::SESSION_ACK:
    case MessageType::SESSION_RESUME:
    case MessageType::SESSION_RESUME_REPLY:
    case MessageType::SESSION_CLOSE:
        return true;
    default:
        return false;
    }
}

namespace
{
    Multi::Message makeCountMessage( const Multi::MessageType type, const uint64_t value )
    {
        RWStreamBuf stream;
        stream.setBigendian( true );
        writeUint64( stream, value );

        return { type, toPayload( stream ) };
    }

    std::optional<uint64_t> readCountMessage( const Multi::Message & message, const Multi::MessageType type )
    {
        if ( message.type != type || message.payload.size() != 8 ) {
            return {};
        }

        ROStreamBuf stream( message.payload );
        stream.setBigendian( true );

        return readUint64( stream );
    }
}

Multi::Message Multi::makeSessionId( const uint64_t sessionId )
{
    return makeCountMessage( MessageType::SESSION_ID, sessionId );
}

std::optional<uint64_t> Multi::readSessionId( const Message & message )
{
    return readCountMessage( message, MessageType::SESSION_ID );
}

Multi::Message Multi::makeSessionAck( const uint64_t receivedCount )
{
    return makeCountMessage( MessageType::SESSION_ACK, receivedCount );
}

std::optional<uint64_t> Multi::readSessionAck( const Message & message )
{
    return readCountMessage( message, MessageType::SESSION_ACK );
}

Multi::Message Multi::makeSessionResume( const SessionResume & resume )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    writeUint64( stream, resume.sessionId );
    writeUint64( stream, resume.receivedCount );

    return { MessageType::SESSION_RESUME, toPayload( stream ) };
}

std::optional<Multi::SessionResume> Multi::readSessionResume( const Message & message )
{
    if ( message.type != MessageType::SESSION_RESUME || message.payload.size() != 16 ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    SessionResume resume;
    resume.sessionId = readUint64( stream );
    resume.receivedCount = readUint64( stream );

    return resume;
}

Multi::Message Multi::makeSessionResumeReply( const SessionResumeReply & reply )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << static_cast<uint8_t>( reply.isAccepted ? 1 : 0 );
    writeUint64( stream, reply.receivedCount );

    return { MessageType::SESSION_RESUME_REPLY, toPayload( stream ) };
}

std::optional<Multi::SessionResumeReply> Multi::readSessionResumeReply( const Message & message )
{
    if ( message.type != MessageType::SESSION_RESUME_REPLY || message.payload.size() != 9 ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    uint8_t accepted = 0;
    stream >> accepted;
    if ( accepted > 1 ) {
        return {};
    }

    SessionResumeReply reply;
    reply.isAccepted = ( accepted == 1 );
    reply.receivedCount = readUint64( stream );

    return reply;
}

Multi::Message Multi::makeRouted( const RoutedMessage & routed )
{
    Message message{ MessageType::ROUTED, std::vector<uint8_t>( 6 + routed.inner.payload.size() ) };
    message.payload[0] = routed.sender;
    message.payload[1] = routed.targets;
    writeUint32( message.payload.data() + 2, static_cast<uint32_t>( routed.inner.type ) );
    std::copy( routed.inner.payload.begin(), routed.inner.payload.end(), message.payload.begin() + 6 );

    return message;
}

std::optional<Multi::RoutedMessage> Multi::readRouted( const Message & message )
{
    if ( message.type != MessageType::ROUTED || message.payload.size() < 6 ) {
        return {};
    }

    RoutedMessage routed;
    routed.sender = message.payload[0];
    routed.targets = message.payload[1];
    routed.inner.type = static_cast<MessageType>( readUint32( message.payload.data() + 2 ) );
    routed.inner.payload.assign( message.payload.begin() + 6, message.payload.end() );

    if ( routed.sender > maxMachineId || isSessionMessage( routed.inner.type ) || routed.inner.type == MessageType::ROUTED ) {
        return {};
    }

    return routed;
}

bool Multi::isOrderBarrier( const MessageType type )
{
    switch ( type ) {
    case MessageType::TURN_STATE:
    case MessageType::GAME_SNAPSHOT:
    case MessageType::BATTLE_START:
    case MessageType::DAY_START:
    case MessageType::DAY_END:
        return true;
    default:
        return false;
    }
}

Multi::HandshakeInfo Multi::getLocalHandshakeInfo()
{
    HandshakeInfo info;
    info.gameVersion = { MAJOR_VERSION, MINOR_VERSION, INTERMEDIATE_VERSION };
    info.protocolVersion = protocolVersion;
    info.buildId = getBuildId();
    info.platform = getPlatformName();
    return info;
}

Multi::HandshakeResult Multi::checkHandshake( const HandshakeInfo & local, const HandshakeInfo & remote )
{
    if ( local.gameVersion != remote.gameVersion || local.protocolVersion != remote.protocolVersion ) {
        return HandshakeResult::INCOMPATIBLE_VERSIONS;
    }

    // An executable which could not be identified is never considered as the same one.
    if ( local.buildId == 0 || remote.buildId == 0 || local.buildId != remote.buildId ) {
        return HandshakeResult::BUILD_MISMATCH;
    }

    return HandshakeResult::ACCEPTED;
}

std::string Multi::describeHandshakeResult( const HandshakeResult result, const HandshakeInfo & local, const HandshakeInfo & remote )
{
    switch ( result ) {
    case HandshakeResult::ACCEPTED:
        return gettext_noop( "accepted" );
    case HandshakeResult::INVALID_MESSAGE:
        return gettext_noop( "invalid handshake message" );
    case HandshakeResult::INCOMPATIBLE_VERSIONS:
        if ( local.protocolVersion != remote.protocolVersion ) {
            return std::string( gettext_noop( "different network protocol versions (this computer / the other player)" ) ) + ": "
                   + std::to_string( local.protocolVersion ) + " / " + std::to_string( remote.protocolVersion );
        }
        return describeOtherVersion( local, remote );
    case HandshakeResult::NOT_A_HOST:
        return gettext_noop( "the other computer does not host a game" );
    case HandshakeResult::BUILD_MISMATCH:
        return describeOtherVersion( local, remote );
    case HandshakeResult::GAME_FULL:
        return gettext_noop( "every player of the game is already taken" );
    case HandshakeResult::GAME_STARTED:
        return gettext_noop( "the game has started without this computer" );
    default:
        break;
    }

    return gettext_noop( "unknown handshake result" );
}

Multi::Message Multi::makeHandshake( const HandshakeInfo & info )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    writeHandshakeInfo( stream, info );

    return { MessageType::HANDSHAKE, toPayload( stream ) };
}

std::optional<Multi::HandshakeInfo> Multi::readHandshake( const Message & message )
{
    if ( message.type != MessageType::HANDSHAKE ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    HandshakeInfo info;
    if ( !readHandshakeInfo( stream, info ) || stream.size() != 0 ) {
        return {};
    }

    return info;
}

Multi::Message Multi::makeHandshakeReply( const HandshakeResult result, const HandshakeInfo & host )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << static_cast<uint8_t>( result );

    const std::array<uint8_t, handshakeKeySize> noKey{};
    stream.putRaw( noKey.data(), noKey.size() );

    writeHandshakeInfo( stream, host );

    return { MessageType::HANDSHAKE_REPLY, toPayload( stream ) };
}

std::optional<Multi::HandshakeReply> Multi::readHandshakeReply( const Message & message )
{
    if ( message.type != MessageType::HANDSHAKE_REPLY ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    uint8_t result = 0;
    stream >> result;

    if ( result > static_cast<uint8_t>( HandshakeResult::GAME_STARTED ) || stream.size() < handshakeKeySize ) {
        return {};
    }

    HandshakeReply reply;
    reply.result = static_cast<HandshakeResult>( result );

    const std::vector<uint8_t> key = stream.getRaw( handshakeKeySize );
    std::copy( key.begin(), key.end(), reply.publicKey.begin() );

    if ( !readHandshakeInfo( stream, reply.host ) || stream.size() != 0 ) {
        return {};
    }

    return reply;
}

Multi::Message Multi::makeTurnState( const TurnState & state, const MessageType type /* = MessageType::TURN_STATE */ )
{
    assert( type == MessageType::TURN_STATE || type == MessageType::GAME_SNAPSHOT || type == MessageType::DAY_START );

    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << state.color << state.receiverColors << static_cast<uint8_t>( state.stage ) << static_cast<uint8_t>( state.isSenderOut ? 1 : 0 );
    writeUint64( stream, state.fingerprint );
    stream.putRaw( state.saveData.data(), state.saveData.size() );

    return { type, toPayload( stream ) };
}

std::optional<Multi::TurnState> Multi::readTurnState( const Message & message )
{
    if ( message.type != MessageType::TURN_STATE && message.type != MessageType::GAME_SNAPSHOT && message.type != MessageType::DAY_START ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    TurnState state;
    uint8_t stage = 0;
    uint8_t isSenderOut = 0;
    stream >> state.color >> state.receiverColors >> stage >> isSenderOut;
    state.fingerprint = readUint64( stream );

    if ( stream.fail() || stage > static_cast<uint8_t>( TurnStage::ENDED ) || isSenderOut > 1 || stream.size() == 0 ) {
        return {};
    }

    state.stage = static_cast<TurnStage>( stage );
    state.isSenderOut = ( isSenderOut == 1 );
    state.saveData = stream.getRaw( 0 );

    return state;
}

Multi::Message Multi::makeBattleStart( const BattleStart & start )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << start.battleId << start.attackerHeroId << static_cast<uint8_t>( start.defender ) << start.defenderHeroId << start.tileIndex << start.seed
           << start.followerColors;
    writeUint64( stream, start.fingerprint );
    stream << static_cast<uint8_t>( start.isShown ? 1 : 0 ) << start.defenderObject;
    stream.putRaw( start.saveData.data(), start.saveData.size() );

    return { MessageType::BATTLE_START, toPayload( stream ) };
}

std::optional<Multi::BattleStart> Multi::readBattleStart( const Message & message )
{
    if ( message.type != MessageType::BATTLE_START ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    BattleStart start;
    uint8_t defender = 0;
    stream >> start.battleId >> start.attackerHeroId >> defender >> start.defenderHeroId >> start.tileIndex >> start.seed >> start.followerColors;
    start.fingerprint = readUint64( stream );

    uint8_t isShown = 0;
    stream >> isShown >> start.defenderObject;

    if ( stream.fail() || defender > static_cast<uint8_t>( BattleStart::Defender::MONSTERS ) || isShown > 1 ) {
        return {};
    }

    start.isShown = ( isShown != 0 );

    start.defender = static_cast<BattleStart::Defender>( defender );

    // getRaw( 0 ) returns all the remaining data.
    if ( stream.size() > 0 ) {
        start.saveData = stream.getRaw( 0 );
    }

    return start;
}

namespace
{
    // An order never has more than a few values, and a unit gives only a few orders at once.
    constexpr uint32_t maxBattleCommands{ 64 };
    constexpr uint32_t maxBattleCommandValues{ 64 };
}

Multi::Message Multi::makeBattleActions( const BattleActions & actions )
{
    assert( actions.commands.size() <= maxBattleCommands );

    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << actions.battleId << static_cast<uint32_t>( actions.commands.size() );

    for ( const BattleCommandData & command : actions.commands ) {
        assert( command.values.size() <= maxBattleCommandValues );

        stream << command.type << static_cast<uint32_t>( command.values.size() );

        for ( const int32_t value : command.values ) {
            stream << value;
        }
    }

    return { MessageType::BATTLE_ACTIONS, toPayload( stream ) };
}

std::optional<Multi::BattleActions> Multi::readBattleActions( const Message & message )
{
    if ( message.type != MessageType::BATTLE_ACTIONS ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    BattleActions actions;
    uint32_t count = 0;
    stream >> actions.battleId >> count;

    if ( stream.fail() || count > maxBattleCommands ) {
        return {};
    }

    actions.commands.resize( count );

    for ( BattleCommandData & command : actions.commands ) {
        uint32_t valueCount = 0;
        stream >> command.type >> valueCount;

        if ( stream.fail() || valueCount > maxBattleCommandValues ) {
            return {};
        }

        command.values.resize( valueCount );

        for ( int32_t & value : command.values ) {
            stream >> value;
        }
    }

    if ( stream.fail() || stream.size() != 0 ) {
        return {};
    }

    return actions;
}

Multi::Message Multi::makeBattleEnd( const BattleEnd & end )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << end.battleId;
    writeUint64( stream, end.fingerprint );

    return { MessageType::BATTLE_END, toPayload( stream ) };
}

std::optional<Multi::BattleEnd> Multi::readBattleEnd( const Message & message )
{
    if ( message.type != MessageType::BATTLE_END ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    BattleEnd end;
    stream >> end.battleId;
    end.fingerprint = readUint64( stream );

    if ( stream.fail() || stream.size() != 0 ) {
        return {};
    }

    return end;
}

Multi::Message Multi::makeHeroLevelUp( const HeroLevelUp & levelUp )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << levelUp.heroId << levelUp.primarySkill << levelUp.firstSkill << levelUp.firstSkillLevel << levelUp.secondSkill << levelUp.secondSkillLevel;

    return { MessageType::HERO_LEVEL_UP, toPayload( stream ) };
}

std::optional<Multi::HeroLevelUp> Multi::readHeroLevelUp( const Message & message )
{
    if ( message.type != MessageType::HERO_LEVEL_UP ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    HeroLevelUp levelUp;
    stream >> levelUp.heroId >> levelUp.primarySkill >> levelUp.firstSkill >> levelUp.firstSkillLevel >> levelUp.secondSkill >> levelUp.secondSkillLevel;

    if ( stream.fail() || stream.size() != 0 ) {
        return {};
    }

    return levelUp;
}

Multi::Message Multi::makeHeroLevelUpChoice( const int32_t skill )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << skill;

    return { MessageType::HERO_LEVEL_UP_CHOICE, toPayload( stream ) };
}

std::optional<int32_t> Multi::readHeroLevelUpChoice( const Message & message )
{
    if ( message.type != MessageType::HERO_LEVEL_UP_CHOICE ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    int32_t skill = 0;
    stream >> skill;

    if ( stream.fail() || stream.size() != 0 ) {
        return {};
    }

    return skill;
}

namespace
{
    // A hero learns only a few spells or artifacts at once.
    constexpr uint32_t maxHeroNoticeValues{ 64 };
}

Multi::Message Multi::makeHeroNotice( const HeroNotice & notice )
{
    assert( notice.values.size() <= maxHeroNoticeValues && notice.text.size() <= maxStringSize );

    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << static_cast<uint8_t>( notice.kind ) << notice.heroId << static_cast<uint32_t>( notice.values.size() );

    for ( const int32_t value : notice.values ) {
        stream << value;
    }

    stream << notice.text;

    return { MessageType::HERO_NOTICE, toPayload( stream ) };
}

std::optional<Multi::HeroNotice> Multi::readHeroNotice( const Message & message )
{
    if ( message.type != MessageType::HERO_NOTICE ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    HeroNotice notice;
    uint8_t kind = 0;
    uint32_t count = 0;
    stream >> kind >> notice.heroId >> count;

    if ( stream.fail() || kind > static_cast<uint8_t>( HeroNotice::Kind::ASSEMBLED_ARTIFACT ) || count > maxHeroNoticeValues ) {
        return {};
    }

    notice.kind = static_cast<HeroNotice::Kind>( kind );
    notice.values.resize( count );

    for ( int32_t & value : notice.values ) {
        stream >> value;
    }

    if ( !readString( stream, notice.text, maxStringSize ) || stream.fail() || stream.size() != 0 ) {
        return {};
    }

    return notice;
}

Multi::Message Multi::makeGameResult( const GameResult & gameResult )
{
    const uint8_t flags = static_cast<uint8_t>( ( gameResult.isGameOver ? 1 : 0 ) | ( gameResult.isOverForReceiver ? 2 : 0 ) );

    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << gameResult.color << gameResult.result << flags;

    return { MessageType::GAME_RESULT, toPayload( stream ) };
}

std::optional<Multi::GameResult> Multi::readGameResult( const Message & message )
{
    if ( message.type != MessageType::GAME_RESULT ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    GameResult gameResult;
    uint8_t flags = 0;
    stream >> gameResult.color >> gameResult.result >> flags;

    if ( stream.fail() || stream.size() != 0 || flags > 3 ) {
        return {};
    }

    gameResult.isGameOver = ( flags & 1 ) != 0;
    gameResult.isOverForReceiver = ( flags & 2 ) != 0;

    return gameResult;
}

Multi::Message Multi::makeWorldUpdate( const WorldUpdate & update )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << update.sequence;
    writeUint64( stream, update.worldHash );
    stream << static_cast<uint8_t>( update.isIncomplete ? 1 : 0 );

    stream.put32( static_cast<uint32_t>( update.units.size() ) );
    for ( const WorldUnit & unit : update.units ) {
        stream << static_cast<uint8_t>( unit.kind ) << unit.key;
        stream.put32( static_cast<uint32_t>( unit.data.size() ) );
        stream.putRaw( unit.data.data(), unit.data.size() );
    }

    return { MessageType::WORLD_UPDATE, toPayload( stream ) };
}

std::optional<Multi::WorldUpdate> Multi::readWorldUpdate( const Message & message )
{
    if ( message.type != MessageType::WORLD_UPDATE ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    WorldUpdate update;
    uint8_t flags = 0;
    stream >> update.sequence;
    update.worldHash = readUint64( stream );
    stream >> flags;

    const uint32_t count = stream.get32();

    // Each unit takes at least its kind, its key and the size of its data.
    if ( stream.fail() || flags > 1 || count > stream.size() / 9 ) {
        return {};
    }

    update.isIncomplete = ( flags != 0 );
    update.units.resize( count );

    for ( WorldUnit & unit : update.units ) {
        uint8_t kind = 0;
        stream >> kind >> unit.key;

        const uint32_t size = stream.get32();
        if ( stream.fail() || size > stream.size() ) {
            return {};
        }

        // getRaw( 0 ) would return all the remaining data.
        if ( size > 0 ) {
            unit.data = stream.getRaw( size );
        }

        if ( kind < static_cast<uint8_t>( WorldUnit::Kind::TILE ) || kind > static_cast<uint8_t>( WorldUnit::Kind::FOG ) ) {
            return {};
        }

        unit.kind = static_cast<WorldUnit::Kind>( kind );
    }

    if ( stream.fail() || stream.size() != 0 ) {
        return {};
    }

    return update;
}

Multi::Message Multi::makeReserveRequest( const ReserveRequest & request )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << request.requestId << request.day << request.kingdoms;

    stream.put32( static_cast<uint32_t>( request.tiles.size() ) );
    for ( const int32_t tile : request.tiles ) {
        stream << tile;
    }

    return { MessageType::RESERVE, toPayload( stream ) };
}

std::optional<Multi::ReserveRequest> Multi::readReserveRequest( const Message & message )
{
    if ( message.type != MessageType::RESERVE ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    ReserveRequest request;
    stream >> request.requestId >> request.day >> request.kingdoms;

    const uint32_t count = stream.get32();
    if ( stream.fail() || count > stream.size() / 4 ) {
        return {};
    }

    request.tiles.resize( count );
    for ( int32_t & tile : request.tiles ) {
        stream >> tile;
    }

    if ( stream.fail() || stream.size() != 0 ) {
        return {};
    }

    return request;
}

Multi::Message Multi::makeReserveReply( const ReserveReply & reply )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << reply.requestId << static_cast<uint8_t>( reply.isGranted ? 1 : 0 ) << reply.takenTile;

    return { MessageType::RESERVE_REPLY, toPayload( stream ) };
}

std::optional<Multi::ReserveReply> Multi::readReserveReply( const Message & message )
{
    if ( message.type != MessageType::RESERVE_REPLY ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    ReserveReply reply;
    uint8_t isGranted = 0;
    stream >> reply.requestId >> isGranted >> reply.takenTile;

    if ( stream.fail() || stream.size() != 0 || isGranted > 1 ) {
        return {};
    }

    reply.isGranted = ( isGranted != 0 );

    return reply;
}

namespace
{
    // More visits than one day can have means broken data.
    constexpr uint32_t maxAlliedVisits{ 10000 };

    void writeAlliedVisits( RWStreamBuf & stream, const std::vector<Multi::AlliedVisit> & visits )
    {
        stream << static_cast<uint32_t>( visits.size() );
        for ( const Multi::AlliedVisit & visit : visits ) {
            stream << visit.heroId << visit.tileIndex;
        }
    }

    bool readAlliedVisits( ROStreamBuf & stream, std::vector<Multi::AlliedVisit> & visits )
    {
        uint32_t count = 0;
        stream >> count;

        if ( stream.fail() || count > maxAlliedVisits || stream.size() < static_cast<size_t>( count ) * 8 ) {
            return false;
        }

        visits.resize( count );
        for ( Multi::AlliedVisit & visit : visits ) {
            stream >> visit.heroId >> visit.tileIndex;
        }

        return !stream.fail();
    }
}

Multi::Message Multi::makeDayDone( const DayDone & dayDone )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << dayDone.day << static_cast<uint8_t>( dayDone.isBattleAsked ? 1 : 0 );
    writeAlliedVisits( stream, dayDone.alliedVisits );

    return { MessageType::DAY_DONE, toPayload( stream ) };
}

std::optional<Multi::DayDone> Multi::readDayDone( const Message & message )
{
    if ( message.type != MessageType::DAY_DONE ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    DayDone dayDone;
    uint8_t flags = 0;
    stream >> dayDone.day >> flags;

    if ( !readAlliedVisits( stream, dayDone.alliedVisits ) || stream.size() != 0 || flags > 1 ) {
        return {};
    }

    dayDone.isBattleAsked = ( flags != 0 );

    return dayDone;
}

Multi::Message Multi::makeDayEnd( const DayEnd & dayEnd )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << dayEnd.day;
    writeUint64( stream, dayEnd.worldHash );
    writeAlliedVisits( stream, dayEnd.alliedVisits );
    stream.putRaw( dayEnd.world.data(), dayEnd.world.size() );

    return { MessageType::DAY_END, toPayload( stream ) };
}

std::optional<Multi::DayEnd> Multi::readDayEnd( const Message & message )
{
    if ( message.type != MessageType::DAY_END ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    DayEnd dayEnd;
    stream >> dayEnd.day;
    dayEnd.worldHash = readUint64( stream );

    if ( stream.fail() || !readAlliedVisits( stream, dayEnd.alliedVisits ) ) {
        return {};
    }

    // getRaw( 0 ) returns all the remaining data.
    if ( stream.size() > 0 ) {
        dayEnd.world = stream.getRaw( 0 );
    }

    return dayEnd;
}

Multi::Message Multi::makeBattleRequest( const BattleRequest & request )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << request.requestId << request.attackerHeroId << request.targetIndex;

    return { MessageType::BATTLE_REQUEST, toPayload( stream ) };
}

std::optional<Multi::BattleRequest> Multi::readBattleRequest( const Message & message )
{
    if ( message.type != MessageType::BATTLE_REQUEST ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    BattleRequest request;
    stream >> request.requestId >> request.attackerHeroId >> request.targetIndex;

    if ( stream.fail() || stream.size() != 0 ) {
        return {};
    }

    return request;
}

Multi::Message Multi::makeBattleRequestReply( const BattleRequestReply & reply )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << reply.requestId << static_cast<uint8_t>( reply.isAccepted ? 1 : 0 );

    return { MessageType::BATTLE_REQUEST_REPLY, toPayload( stream ) };
}

std::optional<Multi::BattleRequestReply> Multi::readBattleRequestReply( const Message & message )
{
    if ( message.type != MessageType::BATTLE_REQUEST_REPLY ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    BattleRequestReply reply;
    uint8_t isAccepted = 0;
    stream >> reply.requestId >> isAccepted;

    if ( stream.fail() || stream.size() != 0 || isAccepted > 1 ) {
        return {};
    }

    reply.isAccepted = ( isAccepted != 0 );

    return reply;
}

Multi::Message Multi::makeLiveBattleDone( const uint32_t requestId )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << requestId;

    return { MessageType::LIVE_BATTLE_DONE, toPayload( stream ) };
}

std::optional<uint32_t> Multi::readLiveBattleDone( const Message & message )
{
    if ( message.type != MessageType::LIVE_BATTLE_DONE ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    uint32_t requestId = 0;
    stream >> requestId;

    if ( stream.fail() || stream.size() != 0 ) {
        return {};
    }

    return requestId;
}

Multi::Message Multi::makeSpectatedMessage( Message message )
{
    switch ( message.type ) {
    case MessageType::BATTLE_START:
        message.type = MessageType::SPECTATE_START;
        break;
    case MessageType::BATTLE_ACTIONS:
        message.type = MessageType::SPECTATE_ACTIONS;
        break;
    case MessageType::BATTLE_END:
        message.type = MessageType::SPECTATE_END;
        break;
    default:
        assert( 0 );
        break;
    }

    return message;
}

std::optional<Multi::Message> Multi::readSpectatedMessage( const Message & message )
{
    Message battleMessage = message;

    switch ( message.type ) {
    case MessageType::SPECTATE_START:
        battleMessage.type = MessageType::BATTLE_START;
        break;
    case MessageType::SPECTATE_ACTIONS:
        battleMessage.type = MessageType::BATTLE_ACTIONS;
        break;
    case MessageType::SPECTATE_END:
        battleMessage.type = MessageType::BATTLE_END;
        break;
    default:
        return {};
    }

    return battleMessage;
}

Multi::Message Multi::makeDayMessage( const MessageType type, const uint32_t day )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << day;

    return { type, toPayload( stream ) };
}

std::optional<uint32_t> Multi::readDayMessage( const Message & message, const MessageType type )
{
    if ( message.type != type ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    uint32_t day = 0;
    stream >> day;

    if ( stream.fail() || stream.size() != 0 ) {
        return {};
    }

    return day;
}

Multi::Message Multi::makeSimultaneousTurns( const SimultaneousTurns & setting )
{
    return { MessageType::SIMULTANEOUS_TURNS,
             { static_cast<uint8_t>( setting.isEnabled ? 1 : 0 ), static_cast<uint8_t>( setting.isChangedByPlayer ? 1 : 0 ), setting.changedBy } };
}

std::optional<Multi::SimultaneousTurns> Multi::readSimultaneousTurns( const Message & message )
{
    if ( message.type != MessageType::SIMULTANEOUS_TURNS || message.payload.size() != 3 || message.payload[0] > 1 || message.payload[1] > 1
         || message.payload[2] > maxMachineId ) {
        return {};
    }

    return SimultaneousTurns{ message.payload[0] != 0, message.payload[1] != 0, message.payload[2] };
}

std::string Multi::translateError( const std::string & error )
{
    std::string translated;
    size_t start = 0;

    while ( true ) {
        const size_t end = error.find( ": ", start );
        translated += _( error.substr( start, end - start ) );

        if ( end == std::string::npos ) {
            return translated;
        }

        translated += " : ";
        start = end + 2;
    }
}
