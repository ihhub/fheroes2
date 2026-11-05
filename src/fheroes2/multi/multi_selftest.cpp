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

#include "multi_selftest.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "artifact.h"
#include "battle.h"
#include "battle_command.h"
#include "castle.h"
#include "color.h"
#include "dialog.h"
#include "direction.h"
#include "game.h"
#include "game_io.h"
#include "game_mode.h"
#include "game_over.h"
#include "heroes.h"
#include "kingdom.h"
#include "logging.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "mp2.h"
#include "multi_action_watch.h"
#include "multi_battle.h"
#include "multi_build_id.h"
#include "multi_connection.h"
#include "multi_contact.h"
#include "multi_fingerprint.h"
#include "multi_game.h"
#include "multi_game_over.h"
#include "multi_hero.h"
#include "multi_live.h"
#include "multi_live_battle.h"
#include "multi_lobby.h"
#include "multi_lobby_client.h"
#include "multi_lobby_protocol.h"
#include "multi_menu.h"
#include "multi_network.h"
#include "multi_notice.h"
#include "multi_port_mapping.h"
#include "multi_protocol.h"
#include "multi_reservation.h"
#include "multi_save_parts.h"
#include "multi_simultaneous.h"
#include "multi_spectator.h"
#include "multi_text.h"
#include "multi_waiting_room.h"
#include "multi_websocket.h"
#include "players.h"
#include "serialize.h"
#include "settings.h"
#include "spell.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "world.h"
#include "world_pathfinding.h"

namespace
{
    // Test with another device such as a tablet. The host listens on the local network.
    bool isLanTest{ false };

    // The connection is cut every few seconds.
    bool isCutTest{ false };

    // The host publishes its game on this lobby, the guest finds it there.
    std::string lobbyTestAddress;

    // The guest joins through the relay of the lobby, never directly.
    bool isRelayTest{ false };

    // The guest of a self-test takes then gives back colors in the waiting room.
    bool isPickingColorsTest{ false };

    // Name of the rooms of self-tests in the lobby. The guest can look for a room published from the lobby screen.
    std::string lobbyTestGameName{ "Self-test" };

    // Guest computers of a game self-test. The first human player plays on the host, the others are dealt to the guests.
    uint32_t testGuestCount{ 1 };

    void applyTestCuts( Multi::NetworkSession & session )
    {
        if ( isCutTest ) {
            session.setTestCuts( std::chrono::seconds( 6 ) );
        }
    }

    using Clock = std::chrono::steady_clock;

    constexpr std::chrono::seconds networkTestTimeout{ 20 };

    // Big enough to need many socket writes, like a save file
    constexpr size_t networkTestDataSize{ size_t{ 4 } * 1024 * 1024 };

    // The host does not update its session for this long after sending, like a busy player
    constexpr std::chrono::seconds networkTestBusyTime{ 5 };

    bool passed( const std::string_view details )
    {
        VERBOSE_LOG( "[MULTI] Self-test PASSED: " << details )
        return true;
    }

    bool failed( const std::string_view reason )
    {
        ERROR_LOG( "[MULTI] Self-test FAILED: " << reason )
        return false;
    }

    // "box" looks for the box of the local network, anything else is the description address of a fake box.
    bool runPortMappingSelfTest( const std::string & descriptionUrl, const std::string & expected )
    {
        std::string outcome;
        std::string details;

        {
            Multi::PortMapping mapping;

            if ( descriptionUrl == "box" ) {
                mapping.start( Multi::defaultPort );
            }
            else {
                mapping.startWithDescription( Multi::defaultPort, descriptionUrl );
            }

            const auto start = std::chrono::steady_clock::now();
            while ( mapping.state() == Multi::PortMapping::State::WORKING && std::chrono::steady_clock::now() - start < std::chrono::seconds( 20 ) ) {
                mapping.update();
                std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
            }

            const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - start ).count();

            switch ( mapping.state() ) {
            case Multi::PortMapping::State::OPEN:
                outcome = "open";
                // Do not write the address of a real box to the log.
                details = descriptionUrl == "box" ? std::string( mapping.externalAddress().empty() ? "no address" : "address found" )
                                                  : "address " + mapping.externalAddress();
                if ( mapping.isBehindAnotherNetwork() ) {
                    details += ", behind another network";
                }
                break;
            case Multi::PortMapping::State::FAILED:
                outcome = "failed";
                details = mapping.error();
                break;
            default:
                outcome = "still working";
                break;
            }

            details = outcome + " in " + std::to_string( milliseconds ) + " ms: " + details;
        }

        if ( outcome != expected ) {
            return failed( "port mapping " + details );
        }

        return passed( "port mapping " + details );
    }

    bool loadSaveFile( const std::string & filePath )
    {
        Settings & conf = Settings::Get();

        // Game::Load() refuses a file of another game type. The real type is then read from the file.
        conf.SetGameType( Game::TYPE_STANDARD | Game::TYPE_CAMPAIGN | Game::TYPE_HOTSEAT );

        return Game::Load( filePath ) != fheroes2::GameMode::CANCEL;
    }

    bool runSaveLoadSelfTest( const std::string & filePath )
    {
        VERBOSE_LOG( "[MULTI] Self-test: save / load round trip of '" << filePath << "'" )

        if ( !loadSaveFile( filePath ) ) {
            return failed( "unable to load '" + filePath + "'" );
        }

        const Multi::StateFingerprint loaded = Multi::computeStateFingerprint();

        const std::string tempFilePath = System::concatPath( System::GetParentDirectory( filePath ), "multi_selftest.tmp" );
        if ( !Game::Save( tempFilePath ) ) {
            return failed( "unable to save '" + tempFilePath + "'" );
        }

        const bool isReloaded = loadSaveFile( tempFilePath );
        System::Unlink( tempFilePath );

        if ( !isReloaded ) {
            return failed( "unable to load '" + tempFilePath + "'" );
        }

        const Multi::StateFingerprint reloaded = Multi::computeStateFingerprint();

        if ( loaded.hash != reloaded.hash || loaded.size != reloaded.size ) {
            return failed( "the fingerprint changed after a save / load round trip: " + loaded.hashString() + " (" + std::to_string( loaded.size ) + " bytes) -> "
                           + reloaded.hashString() + " (" + std::to_string( reloaded.size ) + " bytes)" );
        }

        return passed( loaded.hashString() + " (" + std::to_string( loaded.size ) + " bytes)" );
    }

    std::vector<uint8_t> toBytes( const RWStreamBuf & stream )
    {
        return { stream.data(), stream.data() + stream.size() };
    }

    bool runProtocolSelfTest()
    {
        std::vector<std::string> errors;

        const auto check = [&errors]( const bool condition, const char * description ) {
            if ( !condition ) {
                errors.emplace_back( description );
            }
        };

        {
            const Multi::Message message{ Multi::MessageType::TEST_DATA, { 1, 2, 3, 4, 5 } };
            const std::vector<uint8_t> encoded = Multi::encodeMessage( message );

            Multi::MessageType type = Multi::MessageType::HANDSHAKE;
            uint32_t payloadSize = 0;

            check( encoded.size() == Multi::messageHeaderSize + 5, "encoded message size" );
            check( Multi::decodeMessageHeader( encoded.data(), type, payloadSize ), "valid header is rejected" );
            check( type == Multi::MessageType::TEST_DATA && payloadSize == 5, "header round trip" );
            check( std::vector<uint8_t>( encoded.begin() + Multi::messageHeaderSize, encoded.end() ) == message.payload, "payload round trip" );

            std::vector<uint8_t> wrongMarker = encoded;
            wrongMarker[0] = 'X';
            check( !Multi::decodeMessageHeader( wrongMarker.data(), type, payloadSize ), "wrong marker is accepted" );

            std::vector<uint8_t> tooBig = encoded;
            tooBig[7] = 0xFF;
            check( !Multi::decodeMessageHeader( tooBig.data(), type, payloadSize ), "too big payload is accepted" );
        }

        const Multi::HandshakeInfo local = Multi::getLocalHandshakeInfo();
        check( local.buildId != 0, "build id of the executable" );

        {
            const std::optional<Multi::HandshakeInfo> info = Multi::readHandshake( Multi::makeHandshake( local ) );
            check( info.has_value(), "valid handshake is rejected" );
            check( info && info->protocolVersion == local.protocolVersion && info->gameVersion == local.gameVersion && info->buildId == local.buildId
                       && info->platform == local.platform && !local.platform.empty(),
                   "handshake round trip" );
        }

        {
            Multi::HandshakeInfo guest = local;
            guest.machineKey = 0x0123456789ABCDEF;
            guest.nickname = "Marc";
            guest.claimedColors = static_cast<uint8_t>( PlayerColor::GREEN ) | static_cast<uint8_t>( PlayerColor::PURPLE );

            const std::optional<Multi::HandshakeInfo> info = Multi::readHandshake( Multi::makeHandshake( guest ) );
            check( info && info->machineKey == guest.machineKey && info->nickname == guest.nickname && info->claimedColors == guest.claimedColors,
                   "handshake of a guest round trip" );

            guest.claimedColors = 0x40;
            check( !Multi::readHandshake( Multi::makeHandshake( guest ) ), "handshake with an unknown color is accepted" );
        }

        {
            Multi::HandshakeInfo other = local;
            check( Multi::checkHandshake( local, other ) == Multi::HandshakeResult::ACCEPTED, "same build is refused" );

            other.protocolVersion = local.protocolVersion + 1;
            check( Multi::checkHandshake( local, other ) == Multi::HandshakeResult::INCOMPATIBLE_VERSIONS, "other protocol version" );

            other = local;
            other.buildId = local.buildId + 1;
            check( Multi::checkHandshake( local, other ) == Multi::HandshakeResult::BUILD_MISMATCH, "other executable" );

            other = local;
            other.gameVersion[2] = static_cast<uint8_t>( local.gameVersion[2] + 1 );
            check( Multi::checkHandshake( local, other ) == Multi::HandshakeResult::INCOMPATIBLE_VERSIONS, "other game version" );

            // The same sources built for another system must be accepted.
            other = local;
            other.platform = "Android";
            check( Multi::checkHandshake( local, other ) == Multi::HandshakeResult::ACCEPTED, "same sources on another system are refused" );

            Multi::HandshakeInfo unknown = local;
            unknown.buildId = 0;
            check( Multi::checkHandshake( unknown, unknown ) == Multi::HandshakeResult::BUILD_MISMATCH, "unknown executables are accepted" );
        }

        {
            Multi::Message message = Multi::makeHandshake( local );

            Multi::Message truncated = message;
            truncated.payload.pop_back();
            check( !Multi::readHandshake( truncated ), "truncated handshake is accepted" );

            Multi::Message extended = message;
            extended.payload.push_back( 0 );
            check( !Multi::readHandshake( extended ), "handshake with extra data is accepted" );

            // As in the network design of the project, the request starts with the version of the game.
            check( message.payload.size() > 3 && std::equal( local.gameVersion.begin(), local.gameVersion.end(), message.payload.begin() ),
                   "the handshake does not start with the game version" );

            RWStreamBuf stream;
            stream.setBigendian( true );
            stream << local.gameVersion[0] << local.gameVersion[1] << local.gameVersion[2] << local.protocolVersion << static_cast<uint32_t>( 0 )
                   << static_cast<uint32_t>( 1 ) << static_cast<uint32_t>( 0xFFFFFFFF );
            check( !Multi::readHandshake( { Multi::MessageType::HANDSHAKE, toBytes( stream ) } ), "huge string length is accepted" );

            message.type = Multi::MessageType::TEST_DATA;
            check( !Multi::readHandshake( message ), "handshake with a wrong type is accepted" );
        }

        {
            const Multi::Message replyMessage = Multi::makeHandshakeReply( Multi::HandshakeResult::BUILD_MISMATCH, local );
            const std::optional<Multi::HandshakeReply> reply = Multi::readHandshakeReply( replyMessage );
            check( reply && reply->result == Multi::HandshakeResult::BUILD_MISMATCH && reply->host.buildId == local.buildId, "handshake reply round trip" );

            // The result, then the public key of the host, all zeros as long as messages are not encrypted.
            const auto keyBegin = replyMessage.payload.begin() + 1;
            check( replyMessage.payload.size() > 1 + Multi::handshakeKeySize && replyMessage.payload[0] == static_cast<uint8_t>( Multi::HandshakeResult::BUILD_MISMATCH )
                       && std::all_of( keyBegin, keyBegin + Multi::handshakeKeySize, []( const uint8_t value ) { return value == 0; } ),
                   "the handshake reply does not start with the result and the key" );

            Multi::Message unknownResult = Multi::makeHandshakeReply( Multi::HandshakeResult::ACCEPTED, local );
            unknownResult.payload[0] = 42;
            check( !Multi::readHandshakeReply( unknownResult ), "unknown handshake result is accepted" );
        }

        {
            Multi::HeroLevelUp levelUp;
            levelUp.heroId = 12;
            levelUp.primarySkill = 3;
            levelUp.firstSkill = 7;
            levelUp.firstSkillLevel = 2;
            levelUp.secondSkill = 0;
            levelUp.secondSkillLevel = 0;

            const std::optional<Multi::HeroLevelUp> read = Multi::readHeroLevelUp( Multi::makeHeroLevelUp( levelUp ) );
            check( read && read->heroId == 12 && read->primarySkill == 3 && read->firstSkill == 7 && read->firstSkillLevel == 2 && read->secondSkill == 0,
                   "hero level up round trip" );

            Multi::Message truncated = Multi::makeHeroLevelUp( levelUp );
            truncated.payload.pop_back();
            check( !Multi::readHeroLevelUp( truncated ), "truncated hero level up is accepted" );

            const std::optional<int32_t> choice = Multi::readHeroLevelUpChoice( Multi::makeHeroLevelUpChoice( 7 ) );
            check( choice && *choice == 7, "hero level up choice round trip" );
        }

        {
            Multi::HeroNotice notice;
            notice.kind = Multi::HeroNotice::Kind::LEARNED_SPELLS;
            notice.heroId = 5;
            notice.values = { 3, 18 };
            notice.text = "text";

            const std::optional<Multi::HeroNotice> read = Multi::readHeroNotice( Multi::makeHeroNotice( notice ) );
            check( read && read->kind == notice.kind && read->heroId == 5 && read->values == notice.values && read->text == "text", "hero notice round trip" );

            Multi::Message unknownKind = Multi::makeHeroNotice( notice );
            unknownKind.payload[0] = 42;
            check( !Multi::readHeroNotice( unknownKind ), "unknown hero notice is accepted" );

            Multi::Message tooManyValues = Multi::makeHeroNotice( notice );
            tooManyValues.payload[8] = 0xFF;
            check( !Multi::readHeroNotice( tooManyValues ), "hero notice with too many values is accepted" );
        }

        {
            Multi::GameResult gameResult;
            gameResult.color = 4;
            gameResult.result = 0x100;
            gameResult.isOverForReceiver = true;

            const std::optional<Multi::GameResult> read = Multi::readGameResult( Multi::makeGameResult( gameResult ) );
            check( read && read->color == 4 && read->result == 0x100 && !read->isGameOver && read->isOverForReceiver, "game result round trip" );

            Multi::Message unknownFlags = Multi::makeGameResult( gameResult );
            unknownFlags.payload.back() = 0x80;
            check( !Multi::readGameResult( unknownFlags ), "game result with unknown flags is accepted" );
        }

        {
            Multi::BattleActions actions;
            actions.battleId = 7;
            actions.commands.push_back( { static_cast<uint32_t>( Battle::CommandType::SKIP ), { 3 } } );

            const std::optional<Multi::BattleActions> readActions = Multi::readBattleActions( Multi::makeBattleActions( actions ) );
            check( readActions && readActions->battleId == 7 && readActions->commands.size() == 1 && readActions->commands[0].values == std::vector<int32_t>{ 3 },
                   "battle actions round trip" );

            Multi::BattleEnd end;
            end.battleId = 9;
            end.fingerprint = 0x123456789ABCDEF0ULL;
            const std::optional<Multi::BattleEnd> readEnd = Multi::readBattleEnd( Multi::makeBattleEnd( end ) );
            check( readEnd && readEnd->battleId == 9 && readEnd->fingerprint == end.fingerprint, "battle end round trip" );

            // Some orders from the other computer must be refused before they reach the arena.
            const auto order = []( const Battle::CommandType type, std::vector<int32_t> values ) {
                return Multi::BattleCommandData{ static_cast<uint32_t>( type ), std::move( values ) };
            };

            check( Multi::isValidBattleCommand( order( Battle::CommandType::SKIP, { 3 } ) ), "valid skip order is refused" );
            check( Multi::isValidBattleCommand( order( Battle::CommandType::ATTACK, { 0, 1, 2, 3, 4 } ) ), "valid attack order is refused" );
            check( !Multi::isValidBattleCommand( order( Battle::CommandType::ATTACK, { 0, 1 } ) ), "attack order with missing values is accepted" );
            check( !Multi::isValidBattleCommand( order( Battle::CommandType::MOVE, { -1, 5 } ) ), "move order out of the battlefield is accepted" );
            check( !Multi::isValidBattleCommand( order( Battle::CommandType::SPELLCAST, { -1, 20, Spell::TELEPORT } ) ), "teleport out of the battlefield is accepted" );
            check( !Multi::isValidBattleCommand( order( Battle::CommandType::CATAPULT, { -1 } ) ), "catapult order is accepted" );
            check( !Multi::isValidBattleCommand( order( Battle::CommandType::QUICK_COMBAT, {} ) ), "quick combat order is accepted" );
            check( !Multi::isValidBattleCommand( { 1000, {} } ), "unknown order is accepted" );
        }

        {
            Multi::DayDone dayDone;
            dayDone.day = 12;
            dayDone.isBattleAsked = true;
            dayDone.alliedVisits = { { 3, 1500 }, { 40, 77 } };

            const std::optional<Multi::DayDone> readDone = Multi::readDayDone( Multi::makeDayDone( dayDone ) );
            check( readDone && readDone->day == 12 && readDone->isBattleAsked && readDone->alliedVisits.size() == 2 && readDone->alliedVisits[1].heroId == 40
                       && readDone->alliedVisits[1].tileIndex == 77,
                   "day done round trip" );

            Multi::Message truncatedDone = Multi::makeDayDone( dayDone );
            truncatedDone.payload.pop_back();
            check( !Multi::readDayDone( truncatedDone ), "truncated day done is accepted" );

            Multi::DayEnd dayEnd;
            dayEnd.day = 12;
            dayEnd.worldHash = 0x0123456789ABCDEFULL;
            dayEnd.alliedVisits = dayDone.alliedVisits;
            dayEnd.world = { 9, 8, 7 };

            const std::optional<Multi::DayEnd> readEnd = Multi::readDayEnd( Multi::makeDayEnd( dayEnd ) );
            check( readEnd && readEnd->day == 12 && readEnd->worldHash == dayEnd.worldHash && readEnd->alliedVisits.size() == 2 && readEnd->world == dayEnd.world,
                   "day end round trip" );

            // The count of visits is much bigger than the data that follows it.
            Multi::Message hugeCount = Multi::makeDayEnd( dayEnd );
            hugeCount.payload[12] = 0x7F;
            check( !Multi::readDayEnd( hugeCount ), "day end with a huge count of visits is accepted" );

            const std::optional<Multi::SimultaneousTurns> setting = Multi::readSimultaneousTurns( Multi::makeSimultaneousTurns( { true, false } ) );
            check( setting && setting->isEnabled && !setting->isChangedByPlayer, "simultaneous turns setting round trip" );

            const std::optional<Multi::BattleRequest> request = Multi::readBattleRequest( Multi::makeBattleRequest( { 5, 17, 1234 } ) );
            check( request && request->requestId == 5 && request->attackerHeroId == 17 && request->targetIndex == 1234, "battle request round trip" );

            const std::optional<Multi::BattleRequestReply> reply = Multi::readBattleRequestReply( Multi::makeBattleRequestReply( { 5, true } ) );
            check( reply && reply->requestId == 5 && reply->isAccepted, "battle request reply round trip" );

            const std::optional<uint32_t> done = Multi::readLiveBattleDone( Multi::makeLiveBattleDone( 5 ) );
            check( done && *done == 5, "live battle end round trip" );

            const std::optional<uint32_t> locked = Multi::readDayMessage( Multi::makeDayMessage( Multi::MessageType::DAY_LOCKED, 12 ), Multi::MessageType::DAY_LOCKED );
            check( locked && *locked == 12, "day lock round trip" );
            check( !Multi::readDayMessage( Multi::makeDayMessage( Multi::MessageType::DAY_RESUME, 12 ), Multi::MessageType::DAY_LOCKED ),
                   "day message of another type is accepted" );

            Multi::BattleStart watchedStart;
            watchedStart.battleId = 8;
            watchedStart.defender = Multi::BattleStart::Defender::MONSTERS;
            watchedStart.defenderObject = 1234;
            watchedStart.isShown = false;
            const Multi::Message watchedMessage = Multi::makeSpectatedMessage( Multi::makeBattleStart( watchedStart ) );
            const std::optional<Multi::Message> asBattle = Multi::readSpectatedMessage( watchedMessage );
            const std::optional<Multi::BattleStart> readWatched = asBattle ? Multi::readBattleStart( *asBattle ) : std::nullopt;
            check( watchedMessage.type == Multi::MessageType::SPECTATE_START && readWatched && readWatched->battleId == 8
                       && readWatched->defender == Multi::BattleStart::Defender::MONSTERS && readWatched->defenderObject == 1234 && !readWatched->isShown,
                   "watched battle start round trip" );
            check( !Multi::readSpectatedMessage( Multi::makeBattleStart( watchedStart ) ), "a battle start is read as a watched one" );

            Multi::BattleStart liveStart;
            liveStart.battleId = 3;
            liveStart.tileIndex = 77;
            const std::optional<Multi::BattleStart> readLiveStart = Multi::readBattleStart( Multi::makeBattleStart( liveStart ) );
            check( readLiveStart && readLiveStart->battleId == 3 && readLiveStart->saveData.empty(), "battle start without a game round trip" );

            for ( const char * text : { "192.168.1.30", " 100.101.102.103 ", "pc-ami.tail1234.ts.net", "localhost:47300", "127.0.0.1:1", "::1", "2001:db8::42",
                                        "[2001:db8::42]", "[2001:db8::42]:47300" } ) {
                check( Multi::parseHostAddress( text ).has_value(), ( std::string( "valid address refused: " ) + text ).c_str() );
            }
            for ( const char * text :
                  { "", "   ", "300.1.2.3", "1.2.3", "1.2.3.4.5", "1..2.3", "0001.2.3.4", "1.2.3.4:0", "1.2.3.4:65536", "host:", "a b", "jo\xc3\xablle", "2001:db8::g",
                    "1:2:3:4:5:6:7:8:9", "[2001:db8::42", "[2001:db8::42]47300", "[2001:db8::42]:0", "[host]:47300", "::ffff:1.2.3.4" } ) {
                check( !Multi::parseHostAddress( text ).has_value(), ( std::string( "invalid address accepted: " ) + text ).c_str() );
            }

            const std::optional<std::pair<std::string, uint16_t>> withBrackets = Multi::parseHostAddress( "[2001:db8::42]:47300" );
            check( withBrackets && withBrackets->first == "2001:db8::42" && withBrackets->second == 47300, "IPv6 address with a port" );
            const std::optional<std::pair<std::string, uint16_t>> withPort = Multi::parseHostAddress( "10.0.0.2:47300" );
            check( withPort && withPort->first == "10.0.0.2" && withPort->second == 47300, "address with a port" );
            const std::optional<std::pair<std::string, uint16_t>> withoutPort = Multi::parseHostAddress( "10.0.0.2" );
            check( withoutPort && withoutPort->second == Multi::defaultPort, "address without a port" );

            Multi::Message unknownValue = Multi::makeSimultaneousTurns( { true, true } );
            unknownValue.payload[0] = 2;
            check( !Multi::readSimultaneousTurns( unknownValue ), "simultaneous turns setting with an unknown value is accepted" );
        }

        {
            // Errors are logged in English and translated part by part when shown.
            const std::string translated = Multi::translateError( "unable to connect: 127.0.0.1: unknown address" );
            check( translated == std::string( _( "unable to connect" ) ) + " : 127.0.0.1 : " + _( "unknown address" ), "error translation" );

            if ( fheroes2::getCurrentLanguage() == fheroes2::SupportedLanguage::French ) {
                check( translated == "impossible de se connecter : 127.0.0.1 : adresse inconnue", "French error translation" );
            }
        }

        {
            // Random or damaged messages from a buggy or hostile peer must never crash the decoders.
            uint32_t state = 0x9E3779B9U;
            const auto next = [&state]() {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                return state;
            };

            const auto decodeAll = []( Multi::Message message ) {
                for ( uint32_t type = 0; type <= static_cast<uint32_t>( Multi::MessageType::SPECTATE_END ); ++type ) {
                    message.type = static_cast<Multi::MessageType>( type );

                    (void)Multi::readHandshake( message );
                    (void)Multi::readHandshakeReply( message );
                    (void)Multi::readTurnState( message );
                    (void)Multi::readBattleStart( message );
                    (void)Multi::readBattleActions( message );
                    (void)Multi::readBattleEnd( message );
                    (void)Multi::readHeroLevelUp( message );
                    (void)Multi::readHeroLevelUpChoice( message );
                    (void)Multi::readHeroNotice( message );
                    (void)Multi::readGameResult( message );
                    (void)Multi::readWorldUpdate( message );
                    (void)Multi::readReserveRequest( message );
                    (void)Multi::readReserveReply( message );
                    (void)Multi::readDayDone( message );
                    (void)Multi::readDayEnd( message );
                    (void)Multi::readSimultaneousTurns( message );
                    (void)Multi::readBattleRequest( message );
                    (void)Multi::readBattleRequestReply( message );
                    (void)Multi::readLiveBattleDone( message );
                    (void)Multi::readDayMessage( message, message.type );
                    if ( const std::optional<Multi::Message> spectated = Multi::readSpectatedMessage( message ); spectated ) {
                        (void)Multi::readBattleStart( *spectated );
                        (void)Multi::readBattleActions( *spectated );
                        (void)Multi::readBattleEnd( *spectated );
                    }
                }
            };

            Multi::HeroNotice notice;
            notice.kind = Multi::HeroNotice::Kind::ASSEMBLED_ARTIFACT;
            notice.values = { 4 };
            notice.text = "text";

            Multi::BattleActions actions;
            actions.commands.push_back( { static_cast<uint32_t>( Battle::CommandType::ATTACK ), { 0, 1, 2, 3, 4 } } );

            const std::vector<Multi::Message> validMessages = { Multi::makeHandshake( local ),
                                                                Multi::makeHandshakeReply( Multi::HandshakeResult::ACCEPTED, local ),
                                                                Multi::makeBattleActions( actions ),
                                                                Multi::makeHeroNotice( notice ),
                                                                Multi::makeGameResult( {} ),
                                                                Multi::makeHeroLevelUp( {} ),
                                                                Multi::makeDayDone( { 3, false, { { 1, 2 } } } ),
                                                                Multi::makeDayEnd( { 3, 4, { { 1, 2 } }, { 5 } } ),
                                                                Multi::makeReserveRequest( { 1, 3, 2, { 10, 11 } } ) };

            for ( uint32_t i = 0; i < 3000; ++i ) {
                Multi::Message message;
                message.payload.resize( next() % 96 );
                for ( uint8_t & value : message.payload ) {
                    value = static_cast<uint8_t>( next() );
                }
                decodeAll( message );

                // Take a valid message and either truncate it or change a few bytes.
                Multi::Message damaged = validMessages[i % validMessages.size()];
                if ( !damaged.payload.empty() ) {
                    if ( i % 2 == 0 ) {
                        damaged.payload.resize( next() % damaged.payload.size() );
                    }
                    else {
                        for ( uint32_t k = 0; k < 3; ++k ) {
                            damaged.payload[next() % damaged.payload.size()] = static_cast<uint8_t>( next() );
                        }
                    }
                }
                decodeAll( damaged );

                std::array<uint8_t, Multi::messageHeaderSize> header{};
                for ( uint8_t & value : header ) {
                    value = static_cast<uint8_t>( next() );
                }
                Multi::MessageType type = Multi::MessageType::HANDSHAKE;
                uint32_t payloadSize = 0;
                (void)Multi::decodeMessageHeader( header.data(), type, payloadSize );

                Multi::BattleCommandData command;
                command.type = next() % 16;
                command.values.resize( next() % 8 );
                for ( int32_t & value : command.values ) {
                    value = static_cast<int32_t>( next() % 300 ) - 100;
                }
                (void)Multi::isValidBattleCommand( command );

                std::string error;
                for ( uint32_t k = next() % 24; k > 0; --k ) {
                    error += ": |%x"[next() % 6];
                }
                (void)Multi::translateError( error );
            }
        }

        if ( !errors.empty() ) {
            std::string reason = "protocol:";
            for ( const std::string & error : errors ) {
                reason += " [" + error + "]";
            }
            return failed( reason );
        }

        return passed( "protocol" );
    }

    std::vector<uint8_t> makeTestData( const size_t size )
    {
        std::vector<uint8_t> data( size );
        uint32_t state = 2463534242U;

        for ( uint8_t & value : data ) {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            value = static_cast<uint8_t>( state );
        }

        return data;
    }

    Multi::Message makeTestDataReply( const uint64_t hash, const uint32_t size )
    {
        RWStreamBuf stream;
        stream.setBigendian( true );
        stream << static_cast<uint32_t>( hash >> 32 ) << static_cast<uint32_t>( hash & 0xFFFFFFFFU ) << size;

        return { Multi::MessageType::TEST_DATA_REPLY, toBytes( stream ) };
    }

    // Sleeps a little between two network updates and stops waiting after the timeout.
    Multi::KeepWaiting keepWaitingForTest( const std::chrono::seconds timeout = networkTestTimeout )
    {
        const Clock::time_point deadline = Clock::now() + timeout;

        return [deadline]() {
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            return Clock::now() < deadline;
        };
    }

    std::string describeSessionError( const Multi::NetworkSession & session )
    {
        return session.error().empty() ? "timeout" : session.error();
    }

    bool runHostSelfTest( const uint16_t port )
    {
        Multi::NetworkSession session;
        if ( !session.host( port, !isLanTest ) ) {
            return failed( "unable to host: " + session.error() );
        }

        VERBOSE_LOG( "[MULTI] Self-test: listening on port " << port )

        constexpr Multi::MachineId guest{ 1 };

        if ( const std::string error = Multi::acceptPlayer( session, keepWaitingForTest(), guest ); !error.empty() ) {
            return failed( error );
        }

        // Like a save sent at the start of a turn, it must reach the other player while the host is busy playing.
        const std::vector<uint8_t> hostData = makeTestData( networkTestDataSize );
        session.send( guest, { Multi::MessageType::TEST_DATA, hostData } );
        std::this_thread::sleep_for( networkTestBusyTime );

        const std::optional<Multi::Message> hostDataReply = Multi::waitForMessage( session, keepWaitingForTest() );
        const Multi::Message expectedHostDataReply
            = makeTestDataReply( Multi::computeFnv1aHash( hostData.data(), hostData.size() ), static_cast<uint32_t>( hostData.size() ) );

        if ( !hostDataReply || hostDataReply->type != expectedHostDataReply.type || hostDataReply->payload != expectedHostDataReply.payload ) {
            return failed( "the other player did not get the data sent while the host was busy: " + describeSessionError( session ) );
        }

        const std::optional<Multi::Message> data = Multi::waitForMessage( session, keepWaitingForTest() );
        if ( !data || data->type != Multi::MessageType::TEST_DATA ) {
            return failed( "no test data received: " + describeSessionError( session ) );
        }

        session.send( guest, makeTestDataReply( Multi::computeFnv1aHash( data->payload.data(), data->payload.size() ), static_cast<uint32_t>( data->payload.size() ) ) );

        // The other player closes the connection once it has checked the reply.
        if ( Multi::waitForMessage( session, [&session]() { return session.state( guest ) != Multi::NetworkSession::State::CLOSED && keepWaitingForTest()(); } )
             || session.state( guest ) != Multi::NetworkSession::State::CLOSED ) {
            return failed( "the disconnection of the other player was not detected" );
        }

        return passed( "host received " + std::to_string( data->payload.size() ) + " bytes, then detected the disconnection (" + session.error( guest ) + ")" );
    }

    bool runJoinSelfTest( const std::string & address, const uint16_t port )
    {
        VERBOSE_LOG( "[MULTI] Self-test: joining " << address << ":" << port )

        const Clock::time_point start = Clock::now();

        Multi::NetworkSession session;
        if ( const std::string error = Multi::joinHost( session, address, port, keepWaitingForTest() ); !error.empty() ) {
            return failed( error );
        }

        // Must arrive while the host is still busy.
        const std::optional<Multi::Message> hostData = Multi::waitForMessage( session, keepWaitingForTest( networkTestBusyTime - std::chrono::seconds( 1 ) ) );
        if ( !hostData || hostData->type != Multi::MessageType::TEST_DATA ) {
            return failed( "the data of the host did not arrive while the host was busy: " + describeSessionError( session ) );
        }

        session.send( Multi::hostMachine, makeTestDataReply( Multi::computeFnv1aHash( hostData->payload.data(), hostData->payload.size() ),
                                                             static_cast<uint32_t>( hostData->payload.size() ) ) );

        const std::vector<uint8_t> testData = makeTestData( networkTestDataSize );
        const uint64_t expectedHash = Multi::computeFnv1aHash( testData.data(), testData.size() );

        session.send( Multi::hostMachine, { Multi::MessageType::TEST_DATA, testData } );

        const std::optional<Multi::Message> dataReply = Multi::waitForMessage( session, keepWaitingForTest() );
        if ( !dataReply ) {
            return failed( "no reply to the test data: " + describeSessionError( session ) );
        }

        const Multi::Message expectedReply = makeTestDataReply( expectedHash, static_cast<uint32_t>( testData.size() ) );
        if ( dataReply->type != expectedReply.type || dataReply->payload != expectedReply.payload ) {
            return failed( "the host received different data" );
        }

        session.close();

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( Clock::now() - start ).count();

        return passed( "sent " + std::to_string( testData.size() ) + " bytes, checked by the host, in " + std::to_string( elapsed ) + " ms" );
    }

    // Message number followed by bytes depending on it. Some are as big as a save file.
    Multi::Message makeNumberedMessage( const uint32_t number )
    {
        const uint32_t size = ( number % 97 == 0 ) ? 300000 : ( number * 7919 ) % 2000;

        RWStreamBuf stream;
        stream.setBigendian( true );
        stream << number << size;

        std::vector<uint8_t> payload = toBytes( stream );
        payload.reserve( payload.size() + size );

        for ( uint32_t i = 0; i < size; ++i ) {
            payload.push_back( static_cast<uint8_t>( ( number * 31 + i ) & 0xFF ) );
        }

        return { Multi::MessageType::TEST_DATA, std::move( payload ) };
    }

    bool isNumberedMessage( const Multi::Message & message, const uint32_t number )
    {
        const Multi::Message expected = makeNumberedMessage( number );
        return message.type == expected.type && message.payload == expected.payload;
    }

    // Numbered messages both ways while the connection is cut several times. Each one must arrive once and in order.
    bool runResumeExchange( Multi::NetworkSession & session, const bool isHost )
    {
        constexpr uint32_t messageCount = 3000;

        // The guest cuts after receiving these counts (3 seconds before reconnecting the second time), the host cuts once.
        const std::array<uint32_t, 3> joiningCuts{ 400, 1300, 2200 };
        constexpr uint32_t hostCut = 1700;

        uint32_t sent = 0;
        uint32_t received = 0;
        size_t nextCut = 0;
        bool isHostCutDone = false;

        const Multi::MachineId other = isHost ? Multi::MachineId{ 1 } : Multi::hostMachine;

        const Clock::time_point deadline = Clock::now() + std::chrono::seconds( 120 );

        const auto areCutsDone = [&]() { return isHost ? isHostCutDone : nextCut == joiningCuts.size(); };

        while ( sent < messageCount || received < messageCount || !areCutsDone() ) {
            session.update();

            // One message at a time. The cuts must happen while both are still sending.
            if ( sent < messageCount ) {
                session.send( other, makeNumberedMessage( ++sent ) );
            }

            if ( !isHost && nextCut < joiningCuts.size() && received >= joiningCuts[nextCut] && session.state( other ) == Multi::NetworkSession::State::CONNECTED ) {
                session.cutForTest( std::chrono::milliseconds( nextCut == 1 ? 3000 : 0 ) );
                ++nextCut;
            }

            if ( isHost && !isHostCutDone && received >= hostCut && session.state( other ) == Multi::NetworkSession::State::CONNECTED ) {
                session.cutForTest( std::chrono::milliseconds( 0 ) );
                isHostCutDone = true;
            }

            // Messages after the last numbered one are read later.
            while ( received < messageCount ) {
                const std::optional<Multi::Message> message = session.receive();
                if ( !message ) {
                    break;
                }

                if ( !isNumberedMessage( *message, received + 1 ) ) {
                    return failed( "message " + std::to_string( received + 1 ) + " missing or damaged after a reconnection" );
                }

                ++received;
            }

            if ( session.state( other ) == Multi::NetworkSession::State::CLOSED ) {
                return failed( "the session was closed: " + session.error( other ) );
            }

            if ( Clock::now() > deadline ) {
                return failed( "timeout: " + std::to_string( sent ) + " sent, " + std::to_string( received ) + " received" );
            }

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }

        // Both say they got everything, then the guest leaves.
        session.send( other, { Multi::MessageType::TEST_DATA_REPLY, {} } );

        if ( const std::optional<Multi::Message> done = Multi::waitForMessage( session, keepWaitingForTest() );
             !done || done->type != Multi::MessageType::TEST_DATA_REPLY ) {
            return failed( "the other computer did not finish: " + describeSessionError( session ) );
        }

        if ( isHost ) {
            if ( Multi::waitForMessage( session,
                                        [&session, other]() { return session.state( other ) != Multi::NetworkSession::State::CLOSED && keepWaitingForTest()(); } )
                 || session.state( other ) != Multi::NetworkSession::State::CLOSED ) {
                return failed( "the other computer did not leave" );
            }
        }
        else {
            // Give the last acknowledgements time to be sent before closing.
            const Clock::time_point end = Clock::now() + std::chrono::milliseconds( 500 );
            while ( Clock::now() < end ) {
                session.update();
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            }

            session.close();
        }

        const Multi::SessionStats & stats = session.stats();
        const uint32_t expectedTakenBack = static_cast<uint32_t>( joiningCuts.size() ) + 1;

        if ( stats.connectionsTakenBack != expectedTakenBack ) {
            return failed( std::to_string( stats.connectionsTakenBack ) + " connection(s) taken back instead of " + std::to_string( expectedTakenBack ) );
        }

        return passed( "reconnection: " + std::to_string( messageCount ) + " messages each way, " + std::to_string( stats.connectionsLost ) + " connection(s) lost, "
                       + std::to_string( stats.connectionsTakenBack ) + " taken back, " + std::to_string( stats.messagesSentAgain ) + " message(s) sent again" );
    }

    bool runResumeHostSelfTest( const uint16_t port )
    {
        Multi::NetworkSession session;
        if ( !session.host( port, true ) ) {
            return failed( "unable to host: " + session.error() );
        }

        VERBOSE_LOG( "[MULTI] Self-test: listening on port " << port )

        if ( const std::string error = Multi::acceptPlayer( session, keepWaitingForTest(), 1 ); !error.empty() ) {
            return failed( error );
        }

        return runResumeExchange( session, true );
    }

    bool runResumeJoinSelfTest( const std::string & address, const uint16_t port )
    {
        Multi::NetworkSession session;
        if ( const std::string error = Multi::joinHost( session, address, port, keepWaitingForTest() ); !error.empty() ) {
            return failed( error );
        }

        return runResumeExchange( session, false );
    }

    // Messages of the routing test: sender, target (or every computer), number, then bytes depending on them.
    constexpr uint32_t routedTestCount{ 400 };
    constexpr uint8_t everyComputer{ 0xFF };

    Multi::Message makeRoutedTestMessage( const Multi::MachineId from, const uint8_t to, const uint32_t number )
    {
        const uint32_t size = ( number * 37 + from * 11 + to ) % 700;

        std::vector<uint8_t> payload{ from,
                                      to,
                                      static_cast<uint8_t>( number >> 24 ),
                                      static_cast<uint8_t>( number >> 16 ),
                                      static_cast<uint8_t>( number >> 8 ),
                                      static_cast<uint8_t>( number ) };
        for ( uint32_t i = 0; i < size; ++i ) {
            payload.push_back( static_cast<uint8_t>( number * 13 + i * 7 + from ) );
        }

        return { Multi::MessageType::TEST_DATA, std::move( payload ) };
    }

    // Checks a routed test message: it must be the next one of its sender to this target.
    bool isNextRoutedMessage( const Multi::Message & message, const uint8_t to, std::map<Multi::MachineId, uint32_t> & counts )
    {
        if ( message.type != Multi::MessageType::TEST_DATA || message.payload.size() < 6 || message.payload[0] != message.sender || message.payload[1] != to ) {
            return false;
        }

        const uint32_t number = ( static_cast<uint32_t>( message.payload[2] ) << 24 ) | ( static_cast<uint32_t>( message.payload[3] ) << 16 )
                                | ( static_cast<uint32_t>( message.payload[4] ) << 8 ) | message.payload[5];

        uint32_t & count = counts[message.sender];
        if ( number != count + 1 || message.payload != makeRoutedTestMessage( message.sender, to, number ).payload ) {
            return false;
        }

        ++count;
        return true;
    }

    // Every guest sends numbered messages to each other guest and to every computer, through the host, while the
    // connections are cut. The host checks what is meant for it and lets the guests finish.
    bool runRoutingHostSelfTest( const uint16_t port, const Multi::MachineId guestCount )
    {
        Multi::NetworkSession session;
        if ( !session.host( port, true ) ) {
            return failed( "unable to host: " + session.error() );
        }

        VERBOSE_LOG( "[MULTI] Self-test: listening on port " << port << " for " << guestCount << " guest(s)" )

        for ( Multi::MachineId machine = 1; machine <= guestCount; ++machine ) {
            if ( const std::string error = Multi::acceptPlayer( session, keepWaitingForTest( std::chrono::seconds( 60 ) ), machine ); !error.empty() ) {
                return failed( error );
            }
        }

        // Each guest learns how many there are and its own number.
        for ( Multi::MachineId machine = 1; machine <= guestCount; ++machine ) {
            session.send( machine, { Multi::MessageType::TEST_DATA_REPLY, { static_cast<uint8_t>( guestCount ), machine } } );
        }

        std::map<Multi::MachineId, uint32_t> received;
        std::set<Multi::MachineId> finished;
        bool areCutsDone = false;

        const Clock::time_point deadline = Clock::now() + std::chrono::seconds( 150 );

        while ( finished.size() < guestCount ) {
            session.update();

            while ( std::optional<Multi::Message> message = session.receive() ) {
                if ( message->type == Multi::MessageType::TEST_DATA_REPLY && message->payload.empty() ) {
                    finished.insert( message->sender );
                    continue;
                }

                if ( !isNextRoutedMessage( *message, everyComputer, received ) ) {
                    return failed( "message from computer " + std::to_string( message->sender ) + " missing, damaged or out of order" );
                }
            }

            // The host cuts every link once, in the middle of the exchange.
            if ( !areCutsDone && !received.empty() && received.begin()->second >= routedTestCount / 2 ) {
                session.cutForTest( std::chrono::milliseconds( 0 ) );
                areCutsDone = true;
            }

            for ( Multi::MachineId machine = 1; machine <= guestCount; ++machine ) {
                if ( session.state( machine ) == Multi::NetworkSession::State::CLOSED ) {
                    return failed( "computer " + std::to_string( machine ) + " was lost: " + session.error( machine ) );
                }
            }

            if ( Clock::now() > deadline ) {
                return failed( "timeout, " + std::to_string( finished.size() ) + " of " + std::to_string( guestCount ) + " guest(s) done" );
            }

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }

        for ( Multi::MachineId machine = 1; machine <= guestCount; ++machine ) {
            if ( received[machine] != routedTestCount ) {
                return failed( std::to_string( received[machine] ) + " message(s) received from computer " + std::to_string( machine ) );
            }

            session.send( machine, { Multi::MessageType::TEST_DATA_REPLY, {} } );
        }

        // The guests leave once they know that everybody is done.
        const Clock::time_point end = Clock::now() + std::chrono::seconds( 20 );
        const auto isAnyLeft = [&session, guestCount]() {
            for ( Multi::MachineId machine = 1; machine <= guestCount; ++machine ) {
                if ( session.state( machine ) != Multi::NetworkSession::State::CLOSED ) {
                    return true;
                }
            }
            return false;
        };

        while ( isAnyLeft() && Clock::now() < end ) {
            session.update();
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }

        const Multi::SessionStats & stats = session.stats();
        return passed( "routing: " + std::to_string( guestCount ) + " guest(s), " + std::to_string( routedTestCount )
                       + " messages from each one to every computer and to " + "each other guest, " + std::to_string( stats.connectionsLost ) + " connection(s) lost and "
                       + std::to_string( stats.connectionsTakenBack ) + " taken back here" );
    }

    bool runRoutingJoinSelfTest( const std::string & address, const uint16_t port )
    {
        Multi::NetworkSession session;
        if ( const std::string error = Multi::joinHost( session, address, port, keepWaitingForTest( std::chrono::seconds( 60 ) ) ); !error.empty() ) {
            return failed( error );
        }

        const std::optional<Multi::Message> start = Multi::waitForMessage( session, keepWaitingForTest( std::chrono::seconds( 60 ) ) );
        if ( !start || start->type != Multi::MessageType::TEST_DATA_REPLY || start->payload.size() != 2 ) {
            return failed( "the host did not start the test: " + describeSessionError( session ) );
        }

        const Multi::MachineId guestCount = start->payload[0];
        const Multi::MachineId self = start->payload[1];

        Multi::MachineSet others = Multi::machineBit( Multi::hostMachine );
        for ( Multi::MachineId machine = 1; machine <= guestCount; ++machine ) {
            if ( machine != self ) {
                others |= Multi::machineBit( machine );
            }
        }

        std::map<Multi::MachineId, uint32_t> direct;
        std::map<Multi::MachineId, uint32_t> broadcast;
        uint32_t sent = 0;
        size_t cuts = 0;

        const auto isEverythingReceived = [&]() {
            for ( Multi::MachineId machine = 1; machine <= guestCount; ++machine ) {
                if ( machine != self && ( direct[machine] != routedTestCount || broadcast[machine] != routedTestCount ) ) {
                    return false;
                }
            }
            return true;
        };

        const Clock::time_point deadline = Clock::now() + std::chrono::seconds( 150 );

        while ( sent < routedTestCount || !isEverythingReceived() ) {
            session.update();

            // One message to each other guest and one to every computer at a time.
            if ( sent < routedTestCount ) {
                ++sent;
                for ( Multi::MachineId machine = 1; machine <= guestCount; ++machine ) {
                    if ( machine != self ) {
                        session.send( machine, makeRoutedTestMessage( self, machine, sent ) );
                    }
                }
                session.sendToMachines( others, makeRoutedTestMessage( self, everyComputer, sent ) );
            }

            // Two cuts, the second one long enough to keep the messages of the others waiting at the host.
            if ( cuts < 2 && sent >= ( cuts + 1 ) * routedTestCount / 3 && session.state() == Multi::NetworkSession::State::CONNECTED ) {
                session.cutForTest( std::chrono::milliseconds( cuts == 1 ? 1500 : 0 ) );
                ++cuts;
            }

            while ( std::optional<Multi::Message> message = session.receive() ) {
                const bool isDirect = message->payload.size() >= 2 && message->payload[1] == self;
                if ( !isNextRoutedMessage( *message, isDirect ? self : everyComputer, isDirect ? direct : broadcast ) ) {
                    return failed( "message from computer " + std::to_string( message->sender ) + " missing, damaged or out of order" );
                }
            }

            if ( session.state() == Multi::NetworkSession::State::CLOSED ) {
                return failed( "the session was closed: " + describeSessionError( session ) );
            }

            if ( Clock::now() > deadline ) {
                return failed( "timeout: " + std::to_string( sent ) + " sent" );
            }

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }

        session.send( Multi::hostMachine, { Multi::MessageType::TEST_DATA_REPLY, {} } );

        const std::optional<Multi::Message> done = Multi::waitForMessage( session, keepWaitingForTest( std::chrono::seconds( 60 ) ) );
        if ( !done || done->type != Multi::MessageType::TEST_DATA_REPLY ) {
            return failed( "the host did not end the test: " + describeSessionError( session ) );
        }

        session.close();

        return passed( "routing: computer " + std::to_string( self ) + " of " + std::to_string( guestCount ) + ", everything received in order, "
                       + std::to_string( session.stats().connectionsTakenBack ) + " connection(s) taken back" );
    }

    PlayerColorsSet getHumanColors()
    {
        PlayerColorsSet colors = 0;

        for ( const Player * player : Settings::Get().GetPlayers().getVector() ) {
            if ( player->isControlHuman() ) {
                colors |= player->GetColor();
            }
        }

        return colors;
    }

    std::string getFileExtension( const std::string & filePath )
    {
        const std::string fileName = System::GetFileName( filePath );
        const size_t dot = fileName.find_last_of( '.' );

        return ( dot == std::string::npos ) ? std::string() : StringLower( fileName.substr( dot + 1 ) );
    }

    bool isMapFile( const std::string & filePath )
    {
        const std::string extension = getFileExtension( filePath );

        return extension == "mp2" || extension == "mx2" || extension == "fh2m";
    }

    // The first colors that can be human get the human players. Returns an error, empty on success.
    std::string startNewGameOnMap( const std::string & filePath, const uint32_t humanCount )
    {
        Maps::FileInfo fileInfo;
        const bool isResurrectionMap = ( getFileExtension( filePath ) == "fh2m" );
        const bool isRead = isResurrectionMap ? fileInfo.readResurrectionMap( filePath, false, fheroes2::getCurrentLanguage() ) : fileInfo.readMP2Map( filePath, false );

        if ( !isRead ) {
            return "unable to read the map '" + filePath + "'";
        }

        Settings & conf = Settings::Get();
        conf.SetGameType( Game::TYPE_HOTSEAT );
        conf.setCurrentMapInfo( fileInfo );

        Players & players = conf.GetPlayers();
        players.Init( conf.getCurrentMapInfo() );

        uint32_t humans = 0;

        for ( Player * player : players.getVector() ) {
            const PlayerColor color = player->GetColor();
            const bool mustBeHuman = ( fileInfo.HumanOnlyColors() & color ) != 0;
            const bool canBeHuman = ( fileInfo.colorsAvailableForHumans & color ) != 0;

            if ( mustBeHuman || ( canBeHuman && humans < humanCount ) ) {
                player->SetControl( CONTROL_HUMAN );
                ++humans;
            }
            else {
                player->SetControl( CONTROL_AI );
            }
        }

        if ( humans != humanCount ) {
            return "the map '" + filePath + "' cannot be played by " + std::to_string( humanCount ) + " human players";
        }

        players.SetStartGame();

        const bool isLoaded = isResurrectionMap ? world.loadResurrectionMap( fileInfo.filename )
                                                : world.LoadMapMP2( fileInfo.filename, fileInfo.version == GameVersion::SUCCESSION_WARS );
        if ( !isLoaded ) {
            return "unable to load the map '" + filePath + "'";
        }

        return {};
    }

    std::vector<uint8_t> readFileBytes( const std::string & filePath )
    {
        StreamFile file;
        if ( !file.open( filePath, "rb" ) ) {
            return {};
        }

        return file.getRaw( 0 );
    }

    bool writeFileBytes( const std::string & filePath, const std::vector<uint8_t> & bytes )
    {
        StreamFile file;
        if ( !file.open( filePath, "wb" ) ) {
            return false;
        }

        file.putRaw( bytes.data(), bytes.size() );

        return !file.fail();
    }

    template <typename Unit>
    bool isSameBytes( const Unit & unit, const std::vector<uint8_t> & data, const Multi::ByteRange & range )
    {
        RWStreamBuf stream;
        stream.setBigendian( true );
        stream << unit;

        return stream.size() == range.end - range.begin
               && std::equal( stream.data(), stream.data() + stream.size(), data.begin() + static_cast<std::ptrdiff_t>( range.begin ) );
    }

    // Split and written back, the world must give the same bytes and game. Returns an error, empty on success.
    std::string checkSaveParts( const std::string & filePath )
    {
        if ( isMapFile( filePath ) ) {
            if ( std::string error = startNewGameOnMap( filePath, 2 ); !error.empty() ) {
                return error;
            }
        }
        else if ( !loadSaveFile( filePath ) ) {
            return "unable to load '" + filePath + "'";
        }

        const Multi::StateFingerprint original = Multi::computeStateFingerprint();

        const std::string savePath = System::concatPath( Game::GetSaveDir(), "multi_selftest_parts.savh" );
        if ( !Game::Save( savePath ) ) {
            return "unable to save '" + savePath + "'";
        }

        const std::vector<uint8_t> fileBytes = readFileBytes( savePath );
        System::Unlink( savePath );

        const Clock::time_point start = Clock::now();

        Multi::SaveFileData save;
        if ( const std::string error = Multi::readSaveFileData( fileBytes, save ); !error.empty() ) {
            return "unable to read the save: " + error;
        }

        Multi::WorldParts parts;
        if ( const std::string error = Multi::splitWorld( save.data, parts ); !error.empty() ) {
            return "unable to split the world: " + error;
        }

        const std::vector<uint8_t> assembled = Multi::assembleWorld( save.data, parts );

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( Clock::now() - start ).count();

        if ( assembled != save.data ) {
            return "the world written again is different";
        }

        // Each part must be exactly where the game writes it.
        for ( int32_t index = 0; index < static_cast<int32_t>( parts.tiles.size() ); ++index ) {
            if ( !isSameBytes( world.getTile( index ), save.data, parts.tiles[index] ) ) {
                return "wrong bounds for tile " + std::to_string( index );
            }
        }

        for ( int32_t heroId = 0; heroId < static_cast<int32_t>( parts.heroes.size() ); ++heroId ) {
            const Heroes * hero = world.GetHeroes( heroId );
            if ( hero != nullptr && !isSameBytes( *hero, save.data, parts.heroes[heroId] ) ) {
                return "wrong bounds for hero " + std::to_string( heroId );
            }
        }

        // The rebuilt save must load as a regular save of the same game.
        Multi::SaveFileData rebuilt = save;
        rebuilt.data = assembled;

        if ( !writeFileBytes( savePath, Multi::writeSaveFileData( rebuilt ) ) ) {
            return "unable to write '" + savePath + "'";
        }

        const bool isReloaded = loadSaveFile( savePath );
        System::Unlink( savePath );

        if ( !isReloaded ) {
            return "unable to load the save written again";
        }

        const Multi::StateFingerprint reloaded = Multi::computeStateFingerprint();
        if ( reloaded.hash != original.hash || reloaded.size != original.size ) {
            return "the game changed: " + original.hashString() + " -> " + reloaded.hashString();
        }

        // Broken data must be refused without a crash or a huge allocation.
        for ( size_t cut = 1; cut <= 16; ++cut ) {
            const std::vector<uint8_t> truncated( save.data.begin(), save.data.begin() + static_cast<std::ptrdiff_t>( parts.worldEnd * cut / 17 ) );

            Multi::WorldParts brokenParts;
            if ( Multi::splitWorld( truncated, brokenParts ).empty() ) {
                return "a truncated world is accepted";
            }
        }

        std::vector<uint8_t> wrongTileCount = save.data;
        for ( size_t i = 0; i < 4; ++i ) {
            wrongTileCount[parts.size.end + i] = 0x7F;
        }

        Multi::WorldParts brokenParts;
        if ( Multi::splitWorld( wrongTileCount, brokenParts ).empty() ) {
            return "a wrong number of tiles is accepted";
        }

        std::vector<uint8_t> wrongMagic = fileBytes;
        wrongMagic[0] = 0;
        const std::vector<uint8_t> truncatedFile( fileBytes.begin(), fileBytes.begin() + static_cast<std::ptrdiff_t>( save.header.size() + 20 ) );

        Multi::SaveFileData brokenSave;
        if ( Multi::readSaveFileData( wrongMagic, brokenSave ).empty() || Multi::readSaveFileData( truncatedFile, brokenSave ).empty() ) {
            return "a broken save file is accepted";
        }

        // A few bytes changed anywhere must never cause a crash.
        uint32_t state = static_cast<uint32_t>( save.data.size() ) | 1U;
        for ( uint32_t i = 0; i < 300; ++i ) {
            std::vector<uint8_t> damaged = save.data;

            for ( uint32_t k = 0; k < 1 + i % 4; ++k ) {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                damaged[state % parts.worldEnd] = static_cast<uint8_t>( state >> 8 );
            }

            (void)Multi::splitWorld( damaged, brokenParts );
        }

        VERBOSE_LOG( "[MULTI] Save parts of '" << System::GetFileName( filePath ) << "': " << world.w() << "x" << world.h() << ", " << parts.tiles.size() << " tiles, "
                                               << parts.heroes.size() << " heroes, " << parts.castles.size() << " castles, " << parts.capturedObjects.size()
                                               << " captured objects, " << parts.mapObjects.size() << " map objects, " << save.data.size()
                                               << " bytes, split and written again in " << elapsed << " ms" )

        return {};
    }

    bool runSavePartsSelfTest( const std::vector<std::string> & filePaths )
    {
        std::string errors;

        for ( const std::string & filePath : filePaths ) {
            if ( const std::string error = checkSaveParts( filePath ); !error.empty() ) {
                errors += " [" + System::GetFileName( filePath ) + ": " + error + "]";
            }
        }

        if ( !errors.empty() ) {
            return failed( "save parts:" + errors );
        }

        return passed( "save parts of " + std::to_string( filePaths.size() ) + " file(s): identical bytes and game, broken data refused" );
    }

    bool isSameReach( const Multi::ContactVerdict & first, const Multi::ContactVerdict & second )
    {
        if ( first.result != second.result || first.reaches.size() != second.reaches.size() ) {
            return false;
        }

        for ( size_t i = 0; i < first.reaches.size(); ++i ) {
            if ( first.reaches[i].stand.bits() != second.reaches[i].stand.bits() || first.reaches[i].touch.bits() != second.reaches[i].touch.bits() ) {
                return false;
            }
        }

        return true;
    }

    // The contact computation must not change the game, must be stable and fast, and must cover every tile a hero can reach today.
    // Returns an error, empty on success.
    std::string checkContact( const std::string & filePath, std::string & summary )
    {
        if ( isMapFile( filePath ) ) {
            if ( std::string error = startNewGameOnMap( filePath, 2 ); !error.empty() ) {
                return error;
            }
        }
        else if ( !loadSaveFile( filePath ) ) {
            return "unable to load '" + filePath + "'";
        }

        const PlayerColorsSet humans = Settings::Get().GetPlayers().GetColors( CONTROL_HUMAN, true );
        const PlayerColorsSet hostColors = static_cast<PlayerColorsSet>( Color::GetFirst( humans ) );
        const PlayerColorsSet guestColors = humans & ~hostColors;

        if ( hostColors == 0 || guestColors == 0 ) {
            return "the game needs two human players";
        }

        const uint64_t fingerprint = Multi::computeStateFingerprint().hash;

        const Multi::ContactVerdict verdict = Multi::computeContactVerdict( { hostColors, guestColors } );
        if ( !isSameReach( verdict, Multi::computeContactVerdict( { hostColors, guestColors } ) ) ) {
            return "two computations gave different results";
        }

        if ( Multi::computeStateFingerprint().hash != fingerprint ) {
            return "the computation changed the game";
        }

        if ( verdict.milliseconds > 500 ) {
            return "too slow: " + std::to_string( verdict.milliseconds ) + " ms";
        }

        PlayerWorldPathfinder pathfinder;

        for ( const Multi::PlayerReach & reach : verdict.reaches ) {
            for ( const Heroes * hero : world.GetKingdom( reach.color ).GetHeroes() ) {
                pathfinder.reset();
                pathfinder.reEvaluateIfNeeded( *hero );

                for ( int32_t index = 0; index < static_cast<int32_t>( world.getSize() ); ++index ) {
                    const uint32_t distance = pathfinder.getDistance( index );

                    if ( distance > 0 && distance <= hero->GetMovePoints() && !reach.stand.contains( index ) ) {
                        const fheroes2::Point point = Maps::GetPoint( index );
                        return "tile [" + std::to_string( point.x ) + ", " + std::to_string( point.y ) + "] reached by " + hero->GetName()
                               + " is missing from the reach of " + Color::String( reach.color );
                    }
                }
            }
        }

        summary += " [" + System::GetFileName( filePath ) + ": " + Multi::contactResultName( verdict.result ) + ", " + std::to_string( verdict.milliseconds ) + " ms";
        for ( const Multi::PlayerReach & reach : verdict.reaches ) {
            summary += ", " + Color::String( reach.color ) + " " + std::to_string( reach.stand.count() ) + "/" + std::to_string( reach.touch.count() );
        }
        summary += "]";

        return {};
    }

    bool runContactSelfTest( const std::vector<std::string> & filePaths )
    {
        std::string errors;
        std::string summary;

        for ( const std::string & filePath : filePaths ) {
            if ( const std::string error = checkContact( filePath, summary ); !error.empty() ) {
                errors += " [" + System::GetFileName( filePath ) + ": " + error + "]";
            }
        }

        if ( !errors.empty() ) {
            return failed( "contact:" + errors );
        }

        return passed( "contact of " + std::to_string( filePaths.size() ) + " file(s), read only, stable, wider than the moves of every hero:" + summary );
    }

    int32_t getTileDistance( const int32_t first, const int32_t second )
    {
        const fheroes2::Point a = Maps::GetPoint( first );
        const fheroes2::Point b = Maps::GetPoint( second );

        return std::max( std::abs( a.x - b.x ), std::abs( a.y - b.y ) );
    }

    bool isFreeLandTile( const int32_t index )
    {
        const Maps::Tile & tile = world.getTile( index );

        return !tile.isWater() && tile.getHero() == nullptr && Maps::isClearGround( tile ) && tile.GetPassable() == DIRECTION_ALL;
    }

    // Returns the free land tile in the distance range that is farthest from the tiles to avoid, or -1.
    int32_t findFreeTile( const int32_t target, const int32_t minDistance, const int32_t maxDistance, const std::vector<int32_t> & tilesToAvoid )
    {
        int32_t best = -1;
        int32_t bestDistance = -1;

        for ( int32_t index = 0; index < static_cast<int32_t>( world.getSize() ); ++index ) {
            const int32_t distance = ( target == -1 ) ? minDistance : getTileDistance( index, target );
            if ( distance < minDistance || distance > maxDistance || !isFreeLandTile( index ) ) {
                continue;
            }

            int32_t avoidDistance = std::numeric_limits<int32_t>::max();
            for ( const int32_t tile : tilesToAvoid ) {
                avoidDistance = std::min( avoidDistance, getTileDistance( index, tile ) );
            }

            if ( avoidDistance > bestDistance ) {
                best = index;
                bestDistance = avoidDistance;
            }
        }

        return best;
    }

    // Returns the tiles of the heroes and castles of the given kingdoms.
    std::vector<int32_t> getKingdomTiles( const PlayerColorsSet colors, const std::vector<const Heroes *> & heroesToSkip )
    {
        std::vector<int32_t> tiles;

        for ( const PlayerColor color : PlayerColorsVector( colors ) ) {
            const Kingdom & kingdom = world.GetKingdom( color );

            for ( const Heroes * hero : kingdom.GetHeroes() ) {
                if ( std::find( heroesToSkip.begin(), heroesToSkip.end(), hero ) == heroesToSkip.end() ) {
                    tiles.push_back( hero->GetIndex() );
                }
            }

            for ( const Castle * castle : kingdom.GetCastles() ) {
                tiles.push_back( castle->GetIndex() );
            }
        }

        return tiles;
    }

    // Places the heroes in a few known situations and checks each verdict.
    bool runContactCasesSelfTest( const std::string & filePath )
    {
        if ( const std::string error = startNewGameOnMap( filePath, 2 ); !error.empty() ) {
            return failed( error );
        }

        const PlayerColorsSet humans = Settings::Get().GetPlayers().GetColors( CONTROL_HUMAN, true );
        const PlayerColor hostColor = Color::GetFirst( humans );
        const PlayerColor guestColor = Color::GetFirst( static_cast<PlayerColorsSet>( humans & ~static_cast<PlayerColorsSet>( hostColor ) ) );

        Kingdom & hostKingdom = world.GetKingdom( hostColor );
        Kingdom & guestKingdom = world.GetKingdom( guestColor );

        if ( hostKingdom.GetHeroes().empty() || guestKingdom.GetHeroes().empty() || hostKingdom.GetCastles().empty() ) {
            return failed( "each human player needs a hero and the first one a castle" );
        }

        Heroes * hostHero = hostKingdom.GetHeroes().front();
        Heroes * guestHero = guestKingdom.GetHeroes().front();
        const std::vector<const Heroes *> movedHeroes{ hostHero, guestHero };
        const std::vector<int32_t> allTiles = getKingdomTiles( Color::allPlayerColors(), movedHeroes );

        std::string errors;
        std::string summary;
        uint32_t casesRun = 0;

        const auto check = [&]( const char * name, const Multi::ContactResult expected ) {
            const Multi::ContactVerdict verdict
                = Multi::computeContactVerdict( { static_cast<PlayerColorsSet>( hostColor ), static_cast<PlayerColorsSet>( guestColor ) } );
            ++casesRun;

            std::string description = std::string( name ) + ": " + Multi::contactResultName( verdict.result );
            if ( verdict.witnessTile != -1 ) {
                const fheroes2::Point point = Maps::GetPoint( verdict.witnessTile );
                description += " at (" + std::to_string( point.x ) + ", " + std::to_string( point.y ) + ")";
            }

            if ( verdict.result != expected ) {
                errors += " [" + description + ", expected " + Multi::contactResultName( expected ) + "]";
            }
            summary += " [" + description + "]";
        };

        const auto skip = [&summary]( const char * name ) { summary += std::string( " [" ) + name + ": skipped, no suitable place on this map]"; };

        // Far from everything, there is nothing to share today.
        const int32_t hostFarTile = findFreeTile( -1, 0, 0, allTiles );
        std::vector<int32_t> tilesToAvoid = allTiles;
        tilesToAvoid.push_back( hostFarTile );
        const int32_t guestFarTile = findFreeTile( -1, 0, 0, tilesToAvoid );

        if ( hostFarTile == -1 || guestFarTile == -1 ) {
            return failed( "no free tile on this map" );
        }

        hostHero->Move2Dest( hostFarTile );
        guestHero->Move2Dest( guestFarTile );
        check( "far apart", Multi::ContactResult::SIMULTANEOUS );

        if ( const int32_t tile = findFreeTile( hostFarTile, 3, 3, {} ); tile != -1 ) {
            guestHero->Move2Dest( tile );
            check( "3 tiles apart", Multi::ContactResult::CONTACT );
        }
        else {
            skip( "3 tiles apart" );
        }

        // A hero hired in the host castle could walk to the guest hero.
        const int32_t hostCastle = hostKingdom.GetCastles().front()->GetIndex();
        if ( const int32_t tile = findFreeTile( hostCastle, 6, 8, { hostFarTile } ); tile != -1 ) {
            guestHero->Move2Dest( tile );
            check( "near the castle of the other player", Multi::ContactResult::CONTACT );
        }
        else {
            skip( "near the castle of the other player" );
        }

        // Near two distant exits of the same stone liths
        bool isTeleportCaseDone = false;
        for ( int32_t entrance = 0; entrance < static_cast<int32_t>( world.getSize() ) && !isTeleportCaseDone; ++entrance ) {
            if ( world.getTile( entrance ).getMainObjectType( false ) != MP2::OBJ_STONE_LITHS || world.getTile( entrance ).isWater() ) {
                continue;
            }

            for ( const int32_t exitIndex : world.GetTeleportEndPoints( entrance ) ) {
                if ( getTileDistance( entrance, exitIndex ) < 50 ) {
                    continue;
                }

                const int32_t hostTile = findFreeTile( entrance, 1, 2, allTiles );
                std::vector<int32_t> avoid = allTiles;
                avoid.push_back( entrance );
                const int32_t guestTile = findFreeTile( exitIndex, 3, 4, avoid );

                if ( hostTile == -1 || guestTile == -1 ) {
                    continue;
                }

                hostHero->Move2Dest( hostTile );
                guestHero->Move2Dest( guestTile );
                check( "on both sides of stone liths", Multi::ContactResult::CONTACT );

                isTeleportCaseDone = true;
                break;
            }
        }
        if ( !isTeleportCaseDone ) {
            skip( "on both sides of stone liths" );
        }

        // Both humans can reach the same AI player, at two places far apart.
        bool isComputerCaseDone = false;
        const PlayerColorsSet computers = static_cast<PlayerColorsSet>( Color::allPlayerColors() & ~humans );
        for ( const PlayerColor color : PlayerColorsVector( computers ) ) {
            Kingdom & kingdom = world.GetKingdom( color );
            if ( !kingdom.isPlay() || kingdom.GetCastles().empty() || kingdom.GetHeroes().empty() ) {
                continue;
            }

            // Move its hero far from its castle and from everybody else.
            const int32_t castle = kingdom.GetCastles().front()->GetIndex();
            std::vector<int32_t> avoid = allTiles;
            avoid.push_back( castle );

            const int32_t heroTile = findFreeTile( castle, 50, std::numeric_limits<int32_t>::max(), avoid );
            if ( heroTile == -1 ) {
                continue;
            }

            Heroes * hero = kingdom.GetHeroes().back();
            hero->Move2Dest( heroTile );

            // Host near the castle, guest near the hero, close enough for obstacles not to matter
            const int32_t hostTile = findFreeTile( castle, 3, 3, {} );
            const int32_t guestTile = findFreeTile( hero->GetIndex(), 1, 1, {} );
            if ( hostTile == -1 || guestTile == -1 ) {
                continue;
            }

            hostHero->Move2Dest( hostTile );
            guestHero->Move2Dest( guestTile );
            check( "both near the same computer player", Multi::ContactResult::CONTACT );

            isComputerCaseDone = true;
            break;
        }
        if ( !isComputerCaseDone ) {
            skip( "both near the same computer player" );
        }

        if ( !errors.empty() ) {
            return failed( "contact cases:" + errors );
        }

        return passed( "contact cases on " + System::GetFileName( filePath ) + ", " + std::to_string( casesRun ) + " checked:" + summary );
    }

    std::string describePlayers( const std::vector<uint8_t> & guestColors )
    {
        std::string description;

        for ( const Player * player : Settings::Get().GetPlayers().getVector() ) {
            if ( !description.empty() ) {
                description += ", ";
            }

            description += Color::String( player->GetColor() ) + " ";

            const auto guest = std::find_if( guestColors.begin(), guestColors.end(), [player]( const uint8_t colors ) { return ( colors & player->GetColor() ) != 0; } );

            if ( !player->isControlHuman() ) {
                description += "AI";
            }
            else if ( guest != guestColors.end() ) {
                description += "human (computer " + std::to_string( guest - guestColors.begin() + 1 ) + ")";
            }
            else {
                description += "human (here)";
            }
        }

        return description;
    }

    // The first human player plays on the host, the others are dealt to the guests in turn.
    std::vector<uint8_t> dealGuestColors( const uint32_t guestCount )
    {
        const PlayerColorsVector humans( getHumanColors() );
        std::vector<uint8_t> guestColors( std::min<size_t>( guestCount, humans.empty() ? 0 : humans.size() - 1 ), 0 );

        for ( size_t i = 1; i < humans.size() && !guestColors.empty(); ++i ) {
            guestColors[( i - 1 ) % guestColors.size()] |= static_cast<uint8_t>( humans[i] );
        }

        return guestColors;
    }

    // Waits for the guests of a self-test and starts the game with them.
    bool startHostGameForTest( std::unique_ptr<Multi::NetworkSession> session, const std::vector<uint8_t> & guestColors, const Multi::KeepWaiting & keepWaiting,
                               const Multi::NetworkGameTestOptions testOptions )
    {
        Multi::RoomPlayers players;
        if ( !Multi::startGameForTest( *session, Multi::makeWaitingRoomInfo( lobbyTestGameName ), guestColors, keepWaiting, players ) ) {
            return false;
        }

        applyTestCuts( *session );

        Multi::startHostGame( std::move( session ), players.guests, testOptions );
        return true;
    }

    // Returns true if the game is over or if this computer has no player left in it.
    bool isGameFinished()
    {
        const Multi::GameOverStats & stats = Multi::getGameOverStats();

        return stats.isGameOver || stats.isOutOfGame;
    }

    std::string describeGameOver()
    {
        const Multi::GameOverStats & stats = Multi::getGameOverStats();

        std::string description;

        if ( stats.isOutOfGame ) {
            description += ", no player left here";
        }
        if ( stats.isPlayingAlone ) {
            description += ", went on alone";
        }
        if ( stats.isGameOver ) {
            description += ", game over on day " + std::to_string( world.CountDay() );
        }
        if ( stats.resultsSent + stats.resultsReceived > 0 ) {
            description += " (" + std::to_string( stats.resultsSent ) + " result(s) sent, " + std::to_string( stats.resultsReceived ) + " received)";
        }

        return description;
    }

    // Every live update must have given the same world as on the other computer.
    std::string checkLiveView()
    {
        const Multi::LiveStats & live = Multi::getLiveStats();

        if ( live.hashMismatches > 0 || live.updatesRefused > 0 ) {
            return "live view: " + Multi::describeLiveStats();
        }

        if ( Multi::getSimultaneousDayStats().dayEndMismatches > 0 ) {
            return "simultaneous turns: " + Multi::describeSimultaneousDays();
        }

        return {};
    }

    // From a save file or a new game on a map. The first human plays on the host, the others on the guest, all of them by the AI.
    bool runGameHostSelfTest( const std::string & filePath, const uint16_t port, const uint32_t days, const uint32_t humanCount )
    {
        if ( isMapFile( filePath ) ) {
            if ( const std::string error = startNewGameOnMap( filePath, humanCount ); !error.empty() ) {
                return failed( error );
            }
        }
        else if ( !loadSaveFile( filePath ) ) {
            return failed( "unable to load '" + filePath + "'" );
        }

        const std::vector<uint8_t> guestColors = dealGuestColors( testGuestCount );

        if ( guestColors.empty() ) {
            return failed( "the game must have at least two human players" );
        }

        VERBOSE_LOG( "[MULTI] Self-test game: " << Settings::Get().getCurrentMapInfo().name << " (" << world.w() << "x" << world.h() << "), "
                                                << describePlayers( guestColors ) )

        auto session = std::make_unique<Multi::NetworkSession>();
        if ( !session->host( port, !isLanTest ) ) {
            return failed( "unable to host: " + session->error() );
        }

        VERBOSE_LOG( "[MULTI] Self-test: listening on port " << port )

        std::unique_ptr<Multi::PublishedRoom> room;
        if ( !lobbyTestAddress.empty() ) {
            const Multi::LobbyKey probeKey = Multi::makeRandomLobbyKey();
            session->setProbeKey( probeKey );

            Multi::RoomPublish publish;
            publish.gameName = lobbyTestGameName;
            publish.game = Multi::makeRoomGameOfCurrentGame( static_cast<uint8_t>( Color::Count( getHumanColors() ) - 1 ) );
            publish.gamePort = port;
            publish.probeKey = probeKey;

            // The game listens on this computer only, except for tests with another device.
            publish.localAddresses = isLanTest ? Multi::getLocalAddresses() : std::vector<std::string>{ "127.0.0.1" };
            publish.localAddresses.resize( std::min<size_t>( publish.localAddresses.size(), 8 ) );

            room = std::make_unique<Multi::PublishedRoom>( lobbyTestAddress, "Self-test host", publish );
        }

        // A computer which joins again a game going on may need a while to come to its messages.
        const Multi::KeepWaiting keepWaiting = keepWaitingForTest( std::chrono::seconds( 120 ) );
        Multi::NetworkSession & hosting = *session;
        const auto keepWaitingWithRoom = [&room, &hosting, &keepWaiting]() {
            if ( room ) {
                room->update();
                Multi::listenOnRelayOfRoom( *room, hosting );
            }
            return keepWaiting();
        };

        const uint32_t firstDay = world.CountDay();
        const uint32_t lastDay = firstDay + days;

        if ( !startHostGameForTest( std::move( session ), guestColors, keepWaitingWithRoom, Multi::NetworkGameTestOptions{ true, lastDay } ) ) {
            const std::string error = "the other computers did not all come";
            return failed( room && room->state() == Multi::PublishedRoom::State::FAILED ? error + ", lobby: " + room->error() : error );
        }

        if ( room ) {
            const Multi::RoomReach & reach = room->status().reach;
            VERBOSE_LOG( "[MULTI] Self-test: room " << room->status().inviteCode << " on the lobby " << room->lobby().lobbyName
                                                    << ", reachable directly: " << ( reach.isIPv4 || reach.isIPv6 ? "yes" : "no" ) )
            Multi::keepPublishedRoom( std::move( room ) );
        }

        // It can end earlier if someone wins or a computer has no player left.
        while ( !isGameFinished() ) {
            const fheroes2::GameMode mode = Game::StartGame();
            if ( mode != fheroes2::GameMode::MULTI_WAIT_FOR_TURN ) {
                if ( isGameFinished() || Multi::isTestDayLimitReached() ) {
                    break;
                }

                Multi::endGame();
                return failed( "the turn loop stopped unexpectedly, mode " + std::to_string( static_cast<int>( mode ) ) );
            }

            // Battles led by the other computer and such are handled while waiting.
            fheroes2::GameMode waitMode = fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
            while ( waitMode == fheroes2::GameMode::MULTI_WAIT_FOR_TURN ) {
                waitMode = Multi::waitForTurn();
            }

            if ( waitMode != fheroes2::GameMode::START_GAME ) {
                if ( isGameFinished() ) {
                    break;
                }

                return failed( "the network game has been stopped" );
            }

            if ( world.CountDay() > lastDay ) {
                break;
            }
        }

        const Multi::NetworkGameStats stats = Multi::getStats();
        Multi::endGame();

        if ( stats.fingerprintMismatches > 0 ) {
            return failed( std::to_string( stats.fingerprintMismatches ) + " fingerprint mismatch(es)" );
        }
        if ( const std::string error = checkLiveView(); !error.empty() ) {
            return failed( error );
        }

        return passed( "host played days " + std::to_string( firstDay ) + " to " + std::to_string( world.CountDay() ) + ", " + std::to_string( stats.turnsSent )
                       + " turns sent, " + std::to_string( stats.turnsReceived ) + " received, all fingerprints identical, live view: " + Multi::describeLiveStats()
                       + describeGameOver() );
    }

    // Finds the room of the host in the list of the lobby and joins it.
    std::unique_ptr<Multi::NetworkSession> joinThroughLobbyForTest( std::string & error )
    {
        Multi::LobbyBrowser browser( { lobbyTestAddress }, "Self-test guest" );
        const Multi::KeepWaiting keepWaiting = keepWaitingForTest();

        std::optional<Multi::ListedRoom> found;
        while ( !found ) {
            browser.update();

            for ( const Multi::ListedRoom & room : browser.rooms() ) {
                if ( room.info.gameName == lobbyTestGameName && room.isSameGame ) {
                    found = room;
                }
            }

            if ( !found && !keepWaiting() ) {
                error = "the room of the host is not in the list of the lobby";
                if ( !browser.lobbies().empty() ) {
                    error += ": " + browser.lobbies().front().error;
                }
                return {};
            }
        }

        VERBOSE_LOG( "[MULTI] Self-test: room found in the list of the lobby, " << found->info.game.mapWidth << "x" << found->info.game.mapHeight << ", host "
                                                                                << found->info.hostNickname )

        browser.join( *found );

        std::optional<Multi::JoinAnswer> answer;
        while ( !( answer = browser.takeJoinAnswer() ) ) {
            browser.update();
            if ( !keepWaiting() ) {
                error = "no answer of the lobby to join the room";
                return {};
            }
        }

        if ( answer->plan.result != Multi::JoinResult::READY ) {
            error = "the lobby refused to join the room: " + std::to_string( static_cast<int>( answer->plan.result ) );
            return {};
        }

        VERBOSE_LOG( "[MULTI] Self-test: the lobby gave " << answer->plan.candidates.size() << " direct address(es)" )

        return Multi::joinRoom( answer->plan, answer->relayAddress, isRelayTest, keepWaiting, error );
    }

    // Plays until the host ends the game. Returns an error, empty on success.
    std::string playAsGuest( const std::string & address, const uint16_t port )
    {
        std::unique_ptr<Multi::NetworkSession> session;

        if ( !lobbyTestAddress.empty() ) {
            std::string error;
            session = joinThroughLobbyForTest( error );
            if ( !session ) {
                return error;
            }
        }
        else {
            session = std::make_unique<Multi::NetworkSession>();
            if ( std::string error = Multi::joinHost( *session, address, port, keepWaitingForTest() ); !error.empty() ) {
                return error;
            }
        }

        if ( !Multi::waitForStartForTest( *session, isPickingColorsTest ) ) {
            return "the host did not start the game";
        }

        applyTestCuts( *session );

        Multi::startGuestGame( std::move( session ), Multi::NetworkGameTestOptions{ true } );

        fheroes2::GameMode mode = fheroes2::GameMode::MULTI_WAIT_FOR_TURN;

        while ( mode == fheroes2::GameMode::MULTI_WAIT_FOR_TURN ) {
            mode = Multi::waitForTurn();

            if ( mode == fheroes2::GameMode::START_GAME ) {
                mode = Game::StartGame();

                // Game over for the guest, or it went on alone to the end
                if ( mode != fheroes2::GameMode::MULTI_WAIT_FOR_TURN && !isGameFinished() ) {
                    Multi::endGame();
                    return "the turn loop stopped unexpectedly, mode " + std::to_string( static_cast<int>( mode ) );
                }
            }
        }

        const Multi::GameOverStats & gameOver = Multi::getGameOverStats();
        if ( !Multi::getStats().isEndedByHost && !gameOver.isGameOver && !gameOver.isOutOfGame ) {
            return "the network game has been stopped";
        }

        return {};
    }

    bool runGameJoinSelfTest( const std::string & address, const uint16_t port )
    {
        if ( const std::string error = playAsGuest( address, port ); !error.empty() ) {
            return failed( error );
        }

        const Multi::NetworkGameStats & stats = Multi::getStats();

        if ( stats.fingerprintMismatches > 0 ) {
            return failed( std::to_string( stats.fingerprintMismatches ) + " fingerprint mismatch(es)" );
        }
        // Simultaneous days send the whole game, no turn.
        const bool isSimultaneous = Multi::getSimultaneousDayStats().days > 0;

        if ( stats.turnsReceived == 0 ) {
            return failed( "no turn received" );
        }

        // Snapshots come with host turns. There are none if the host players were all vanquished first.
        if ( stats.snapshotsReceived == 0 && !isSimultaneous && !Multi::getGameOverStats().isPlayingAlone ) {
            return failed( "no game snapshot received" );
        }
        if ( const std::string error = checkLiveView(); !error.empty() ) {
            return failed( error );
        }

        return passed( "guest received " + std::to_string( stats.turnsReceived ) + " turns and " + std::to_string( stats.snapshotsReceived ) + " snapshots, sent "
                       + std::to_string( stats.turnsSent ) + ", all fingerprints identical, live view: " + Multi::describeLiveStats() + describeGameOver() );
    }

    Heroes * getFirstHero( const PlayerColor color )
    {
        const VecHeroes & heroes = world.GetKingdom( color ).GetHeroes();

        return heroes.empty() ? nullptr : heroes.front();
    }

    // A hero of the first human attacks one of the second. Each army is played by the AI on its own computer.
    bool runBattleHostSelfTest( const std::string & filePath, const uint16_t port )
    {
        if ( !loadSaveFile( filePath ) ) {
            return failed( "unable to load '" + filePath + "'" );
        }

        const PlayerColorsSet humanColors = getHumanColors();
        const PlayerColorsSet localColor = humanColors & static_cast<PlayerColorsSet>( ~humanColors + 1 );
        const PlayerColorsSet remoteColors = humanColors & static_cast<PlayerColorsSet>( ~localColor );

        if ( remoteColors == 0 ) {
            return failed( "the save file must have at least two human players" );
        }

        // One guest with every other human player.
        const std::vector<uint8_t> guestColors{ static_cast<uint8_t>( remoteColors ) };

        Heroes * attacker = getFirstHero( static_cast<PlayerColor>( localColor ) );
        Heroes * defender = getFirstHero( static_cast<PlayerColor>( remoteColors & static_cast<PlayerColorsSet>( ~remoteColors + 1 ) ) );

        if ( attacker == nullptr || defender == nullptr ) {
            return failed( "both human players need a hero" );
        }

        auto session = std::make_unique<Multi::NetworkSession>();
        if ( !session->host( port, !isLanTest ) ) {
            return failed( "unable to host: " + session->error() );
        }

        VERBOSE_LOG( "[MULTI] Self-test: listening on port " << port )

        if ( !startHostGameForTest( std::move( session ), guestColors, keepWaitingForTest(), Multi::NetworkGameTestOptions{ true } ) ) {
            return failed( "the other computer did not start the game" );
        }

        // No player is human in self-tests and no dialog is shown at the end.
        const Battle::Result result = Battle::Loader( attacker->GetArmy(), defender->GetArmy(), defender->GetIndex() );

        // Two levels and a few notices for the defender, the skills are picked on the guest.
        const int level = defender->GetLevel();
        uint32_t experience = 0;
        while ( Heroes::GetLevelFromExperience( defender->GetExperience() + experience ) < level + 2 ) {
            experience += 100;
        }

        defender->IncreaseExperience( experience );

        Multi::notifyRaisedSkeletons( *defender, 12 );
        Multi::notifyLearnedSpells( *defender, { Spell( Spell::BLESS ), Spell( Spell::HASTE ) } );
        Multi::notifyAssembledArtifacts( *defender, { ArtifactSetData( Artifact::BATTLE_GARB, "The three Anduran artifacts magically combine into one." ) } );

        const Multi::NetworkBattleStats stats = Multi::getBattleStats();
        const Multi::HeroEventStats heroStats = Multi::getHeroEventStats();
        Multi::endGame();

        if ( stats.battlesLed != 1 || stats.ordersSent == 0 || stats.ordersReceived == 0 ) {
            return failed( "the battle was not played over the network: " + std::to_string( stats.ordersSent ) + " orders sent, " + std::to_string( stats.ordersReceived )
                           + " received" );
        }

        if ( defender->GetLevel() != level + 2 || heroStats.levelUpsSent != 2 || heroStats.choicesReceived != heroStats.choicesAsked || heroStats.noticesSent != 3 ) {
            return failed( "level ups of the other player's hero: level " + std::to_string( level ) + " to " + std::to_string( defender->GetLevel() ) + ", "
                           + std::to_string( heroStats.levelUpsSent ) + " sent, " + std::to_string( heroStats.choicesReceived ) + " of "
                           + std::to_string( heroStats.choicesAsked ) + " choices received, " + std::to_string( heroStats.noticesSent ) + " notices sent" );
        }

        return passed( "network battle " + std::string( result.isAttackerWin() ? "won" : "lost" ) + " by the attacker, " + std::to_string( stats.ordersSent )
                       + " orders sent, " + std::to_string( stats.ordersReceived ) + " received, " + std::to_string( heroStats.choicesReceived )
                       + " skills picked on the other computer" );
    }

    bool runBattleJoinSelfTest( const std::string & address, const uint16_t port )
    {
        if ( const std::string error = playAsGuest( address, port ); !error.empty() ) {
            return failed( error );
        }

        const Multi::NetworkBattleStats & stats = Multi::getBattleStats();

        if ( stats.battlesFollowed != 1 || stats.fingerprintMatches != 1 || stats.fingerprintMismatches != 0 ) {
            return failed( std::to_string( stats.battlesFollowed ) + " battle(s) followed, " + std::to_string( stats.fingerprintMatches ) + " identical, "
                           + std::to_string( stats.fingerprintMismatches ) + " different" );
        }

        const Multi::HeroEventStats & heroStats = Multi::getHeroEventStats();

        if ( heroStats.levelUpsReceived != 2 || heroStats.noticesReceived != 3 ) {
            return failed( std::to_string( heroStats.levelUpsReceived ) + " level up(s) and " + std::to_string( heroStats.noticesReceived )
                           + " notice(s) received for the hero of this player" );
        }

        return passed( "both battles ended the same way, " + std::to_string( stats.ordersSent ) + " orders sent, " + std::to_string( stats.ordersReceived )
                       + " received, 2 level ups and 3 notices for the hero of this player" );
    }

    // The guest players are vanquished. The guest leaves and the host goes on alone.
    bool runGameOverHostSelfTest( const std::string & filePath, const uint16_t port )
    {
        if ( !loadSaveFile( filePath ) ) {
            return failed( "unable to load '" + filePath + "'" );
        }

        const PlayerColorsSet humanColors = getHumanColors();
        const PlayerColorsSet localColor = humanColors & static_cast<PlayerColorsSet>( ~humanColors + 1 );
        const PlayerColorsSet remoteColors = humanColors & static_cast<PlayerColorsSet>( ~localColor );

        if ( remoteColors == 0 ) {
            return failed( "the save file must have at least two human players" );
        }

        auto session = std::make_unique<Multi::NetworkSession>();
        if ( !session->host( port, !isLanTest ) ) {
            return failed( "unable to host: " + session->error() );
        }

        VERBOSE_LOG( "[MULTI] Self-test: listening on port " << port )

        if ( !startHostGameForTest( std::move( session ), { static_cast<uint8_t>( remoteColors ) }, keepWaitingForTest(), Multi::NetworkGameTestOptions{ true } ) ) {
            return failed( "the other computer did not start the game" );
        }

        // The guest learns which players it plays and then loses all of them.
        Multi::onTurnStart( static_cast<PlayerColor>( localColor ) );

        for ( const PlayerColor color : PlayerColorsVector( remoteColors ) ) {
            world.GetKingdom( color ).LossPostActions();
        }

        Settings::Get().SetCurrentColor( static_cast<PlayerColor>( localColor ) );
        const fheroes2::GameMode mode = GameOver::Result::Get().checkGameOver();

        // As the turn loop does before the next player: no guest has players left, the game goes on here alone.
        Multi::leaveIfNetworkGameOver();

        const Multi::GameOverStats stats = Multi::getGameOverStats();
        const bool isStillNetworkGame = Multi::isNetworkGame();
        Multi::endGame();

        if ( stats.resultsSent == 0 || isStillNetworkGame ) {
            return failed( "the other computer was not told that its players are vanquished" );
        }

        if ( mode == fheroes2::GameMode::CANCEL ) {
            return passed( "the players of the other computer are vanquished, the game goes on here alone" );
        }

        return passed( "the players of the other computer are vanquished, game over here (mode " + std::to_string( static_cast<int>( mode ) ) + ")" );
    }

    bool runGameOverJoinSelfTest( const std::string & address, const uint16_t port )
    {
        if ( const std::string error = playAsGuest( address, port ); !error.empty() ) {
            return failed( error );
        }

        const Multi::GameOverStats & stats = Multi::getGameOverStats();

        if ( stats.resultsReceived == 0 || ( !stats.isOutOfGame && !stats.isGameOver ) ) {
            return failed( "the defeat of the players of this computer was not received" );
        }

        return passed( "the players of this computer are vanquished" + describeGameOver() );
    }

    // A text of the game with a made-up player name.
    std::string nameForTest( const char * text )
    {
        std::string named = _( text );
        StringReplace( named, "%{name}", "Marc" );
        return named;
    }

    // A script takes a screenshot of each dialog when its log line appears, then closes it.
    bool runDialogsSelfTest( const std::string & filePath )
    {
        if ( !loadSaveFile( filePath ) ) {
            return failed( "unable to load " + filePath );
        }

        const PlayerColorsVector humans( Players::HumanColors() );
        if ( humans.size() < 2 ) {
            return failed( "the game needs two human players" );
        }

        const Multi::HandshakeInfo local = Multi::getLocalHandshakeInfo();
        Multi::HandshakeInfo remote = local;
        remote.buildId = ~local.buildId;
        ++remote.protocolVersion;

        const auto showError = []( const char * header, const std::string & reason ) {
            fheroes2::showStandardTextMessage( _( "Network game" ), std::string( _( header ) ) + "\n\n" + Multi::translateError( reason ), Dialog::OK );
        };

        const auto showSimultaneous = []( const char * text, const int buttons ) { fheroes2::showStandardTextMessage( _( "Simultaneous turns" ), _( text ), buttons ); };

        // The adventure map and the castle are shown for the first human player.
        Settings::Get().SetCurrentColor( humans[0] );
        const VecCastles & castles = world.GetKingdom( humans[0] ).GetCastles();

        std::string turnText = _( "%{color} player's turn." );
        StringReplace( turnText, "%{color}", Color::String( humans[0] ) );

        const std::vector<std::pair<const char *, std::function<void()>>>
            dialogs{ { "leave",
                       [] {
                           fheroes2::showStandardTextMessage( _( "Network game" ), _( "Are you sure you want to leave the network game?" ), Dialog::YES | Dialog::NO );
                       } },
                     { "build mismatch",
                       [&] {
                           showError( gettext_noop( "Unable to join the network game:" ),
                                      std::string( gettext_noop( "handshake refused" ) ) + ": "
                                          + Multi::describeHandshakeResult( Multi::HandshakeResult::BUILD_MISMATCH, local, remote ) );
                       } },
                     { "protocol mismatch",
                       [&] {
                           showError( gettext_noop( "Unable to start the network game:" ),
                                      std::string( gettext_noop( "handshake refused" ) ) + ": "
                                          + Multi::describeHandshakeResult( Multi::HandshakeResult::INCOMPATIBLE_VERSIONS, local, remote ) );
                       } },
                     { "no host",
                       [&] {
                           showError( gettext_noop( "Unable to join the network game:" ),
                                      std::string( gettext_noop( "unable to connect" ) ) + ": " + gettext_noop( "nobody is hosting a game at this address" ) );
                       } },
                     { "connection lost",
                       [&] {
                           showError( gettext_noop( "The network game has been stopped:" ), std::string( gettext_noop( "the connection with the host was lost" ) ) + ": "
                                                                                                + gettext_noop( "the other player closed the connection" ) );
                       } },
                     { "inconsistent turn",
                       [&] { showError( gettext_noop( "The network game has been stopped:" ), gettext_noop( "the turn sent by the host is inconsistent" ) ); } },
                     { "two humans",
                       [&] {
                           showError( gettext_noop( "Unable to start the network game:" ),
                                      gettext_noop( "this game needs at least two human players still in the game" ) );
                       } },
                     { "host left", [] { fheroes2::showStandardTextMessage( _( "Network game" ), _( "The host has left the game." ), Dialog::OK ); } },
                     { "invalid address", [] { fheroes2::showStandardTextMessage( _( "Network game" ), _( "This address is not valid." ), Dialog::OK ); } },
                     { "won", [&] { Game::DialogPlayers( humans[1], _( "Major Event!" ), _( "%{color} player has won the game." ) ); } },
                     { "vanquished", [&] { Game::DialogPlayers( humans[1], _( "Major Event!" ), _( "%{color} player has been vanquished!" ) ); } },
                     { "game over", [] { fheroes2::showStandardTextMessage( _( "Network game" ), _( "The game is over." ), Dialog::OK ); } },
                     { "no player left",
                       [] {
                           fheroes2::showStandardTextMessage( _( "Network game" ), _( "You have no player left in this game. It goes on without you." ), Dialog::OK );
                       } },
                     { "no player left on the host",
                       [] {
                           fheroes2::showStandardTextMessage( _( "Network game" ),
                                                              _( "You have no player left in this game. Your computer keeps hosting it for the other players." ),
                                                              Dialog::OK );
                       } },
                     { "help new game",
                       [] {
                           fheroes2::showStandardTextMessage( _( "Host a new game" ), _( "Start a new game with other players. Each one takes a color when joining." ),
                                                              Dialog::OK );
                       } },
                     { "help saved game",
                       [] {
                           fheroes2::showStandardTextMessage( _( "Host a saved game" ),
                                                              _( "Load a Hot Seat game and play it with other players. Each one takes a color when joining." ),
                                                              Dialog::OK );
                       } },
                     { "help join", [] { fheroes2::showStandardTextMessage( _( "Join a network game" ), _( "Join the game of another player." ), Dialog::OK ); } },
                     { "help simultaneous turns",
                       [&] {
                           showSimultaneous( gettext_noop( "Every player plays their day at the same time and sees what the others do. The computer players "
                                                           "play afterwards. The host makes the choice." ),
                                             Dialog::OK );
                       } },
                     { "host waiting local", [] { Multi::showHostWaitingScreenForTest( Multi::HostWaitingCase::LOCAL ); } },
                     { "host waiting opening box", [] { Multi::showHostWaitingScreenForTest( Multi::HostWaitingCase::OPENING ); } },
                     { "host waiting box open", [] { Multi::showHostWaitingScreenForTest( Multi::HostWaitingCase::OPEN ); } },
                     { "host waiting behind another network", [] { Multi::showHostWaitingScreenForTest( Multi::HostWaitingCase::BEHIND_ANOTHER_NETWORK ); } },
                     { "host waiting box failed", [] { Multi::showHostWaitingScreenForTest( Multi::HostWaitingCase::FAILED ); } },
                     { "host waiting on lobby", [] { Multi::showHostWaitingScreenForTest( Multi::HostWaitingCase::LOBBY ); } },
                     { "waiting room with guest", [] { Multi::showHostWaitingScreenForTest( Multi::HostWaitingCase::GUEST_IN ); } },
                     { "switch help",
                       [] {
                           fheroes2::showStandardTextMessage( _( "Network game" ), _( "Switch between simultaneous turns and turns one after the other." ), Dialog::OK );
                       } },
                     { "switch on", [&] { showSimultaneous( gettext_noop( "Switch to simultaneous turns from tomorrow?" ), Dialog::YES | Dialog::NO ); } },
                     { "switch off", [&] { showSimultaneous( gettext_noop( "Switch back to turns one after the other from tomorrow?" ), Dialog::YES | Dialog::NO ); } },
                     { "turn", [&] { Multi::showMapNoteForTest( turnText ); } },
                     { "switched on there", [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "%{name} switches to simultaneous turns from tomorrow." ) ) ); } },
                     { "switched off there",
                       [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "%{name} switches back to turns one after the other from tomorrow." ) ) ); } },
                     { "other player done", [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "%{name} has ended their turn." ) ) ); } },
                     { "place taken", [] { Multi::showMapNoteForTest( _( "Another player has already been here. This place is theirs until tomorrow." ) ); } },
                     { "battle refused",
                       [] { Multi::showMapNoteForTest( _( "No battle between players during simultaneous turns. Tomorrow, each player will play in turn." ) ); } },
                     { "other player busy", [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "%{name} is busy. Try again in a moment." ) ) ); } },
                     { "target moved away", [] { Multi::showMapNoteForTest( _( "Your target has moved away." ) ); } },
                     { "attacked", [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "%{name} is attacking you!" ) ) ); } },
                     { "attack waiting", [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "Waiting for %{name} (Esc to call off the attack)" ) ) ); } },
                     { "finishing move", [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "%{name} is finishing their move..." ) ) ); } },
                     { "choosing skill", [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "%{name} is choosing a skill for their hero..." ) ) ); } },
                     { "end of day", [] { Multi::showMapNoteForTest( _( "Waiting for the other players..." ) ); } },
                     { "player cut", [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "The connection with %{name} is cut." ) ) ); } },
                     { "player left", [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "%{name} has left the game." ) ) ); } },
                     { "player back", [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "%{name} is back." ) ) ); } },
                     { "computer plays for", [] { Multi::showMapNoteForTest( nameForTest( gettext_noop( "The computer plays for %{name} from now on." ) ) ); } },
                     { "connection lost, host", [] { Multi::showReconnectionWindowForTest( true, true ); } },
                     { "connection lost, joining", [] { Multi::showReconnectionWindowForTest( false, true ); } },
                     { "connection not restored", [] { Multi::showReconnectionWindowForTest( false, false ); } },
                     { "castle visit", [&] {
                          if ( !castles.empty() ) {
                              Multi::visitCastle( *castles.front() );
                          }
                      } } };

        for ( size_t i = 0; i < dialogs.size(); ++i ) {
            VERBOSE_LOG( "[MULTI] Dialog " << i + 1 << "/" << dialogs.size() << ": " << dialogs[i].first )
            dialogs[i].second();
        }

        return passed( std::to_string( dialogs.size() ) + " dialogs shown" );
    }

    template <typename T>
    std::optional<T> parseNumber( const std::string_view text, const T minValue, const T maxValue )
    {
        T value = 0;
        const auto [end, error] = std::from_chars( text.data(), text.data() + text.size(), value );

        if ( error != std::errc() || end != text.data() + text.size() || value < minValue || value > maxValue ) {
            return {};
        }

        return value;
    }

    std::optional<uint16_t> parsePort( const std::string_view text )
    {
        return parseNumber<uint16_t>( text, 1, 65535 );
    }

    Multi::LobbyKey makeTestKey( const uint8_t first )
    {
        Multi::LobbyKey key{};
        for ( size_t i = 0; i < key.size(); ++i ) {
            key[i] = static_cast<uint8_t>( first + i );
        }

        return key;
    }

    // The same samples as server/lobby/test/samples.js.
    std::vector<std::pair<std::string, Multi::Message>> makeLobbySamples()
    {
        const Multi::LobbyKey probeKey = makeTestKey( 0x01 );
        const Multi::LobbyKey hostToken = makeTestKey( 0xA0 );
        const Multi::LobbyKey relayTicket = makeTestKey( 0xB0 );
        const Multi::LobbyKey probeValue = makeTestKey( 0xC0 );
        const uint64_t roomId = 0x1122334455667788;

        // "Élodie" and "Duel 東京 𝄞" in UTF-8.
        const std::string nickname = "\xC3\x89lodie";
        const std::string gameName = "Duel \xE6\x9D\xB1\xE4\xBA\xAC \xF0\x9D\x84\x9E";

        Multi::HandshakeInfo pcGame;
        pcGame.gameVersion = { 1, 1, 17 };
        pcGame.protocolVersion = 18;
        pcGame.buildId = 0x0123456789ABCDEF;
        pcGame.platform = "Windows x64";

        Multi::HandshakeInfo tabletGame = pcGame;
        tabletGame.platform = "Android arm64";

        const Multi::RoomGame roomGame{ "Brokenshores", 72, 72, 4, 2, 1, Multi::RoomTurns::SIMULTANEOUS, 1 };

        Multi::RoomGame laterRoomGame = roomGame;
        laterRoomGame.day = 23;

        const Multi::RoomInfo roomInfo{ roomId, Multi::RoomState::WAITING_FOR_PLAYERS, { false, true, true, true }, gameName, nickname, "fr", tabletGame, roomGame };
        const Multi::RoomInfo otherRoomInfo{ 0x99,
                                             Multi::RoomState::WAITING_FOR_PLAYERS,
                                             { false, false, true, true },
                                             "Entre amis",
                                             "Marc",
                                             "en",
                                             pcGame,
                                             { "", 144, 144, 6, 6, 3, Multi::RoomTurns::ONE_AFTER_ANOTHER, 112 } };

        const Multi::RoomPublish publish{ Multi::RoomVisibility::PUBLIC,
                                          false,
                                          "La partie du samedi",
                                          roomGame,
                                          47219,
                                          probeKey,
                                          { "2001:db8::25", "203.0.113.7" },
                                          { "192.168.1.30" },
                                          0,
                                          "",
                                          Multi::RoomState::WAITING_FOR_PLAYERS,
                                          hostToken,
                                          relayTicket };

        Multi::RoomPublish publishAgain = publish;
        publishAgain.roomId = roomId;
        publishAgain.inviteCode = "K7Q2MX";
        publishAgain.state = Multi::RoomState::IN_GAME;

        Multi::RoomPublish privatePublish = publish;
        privatePublish.visibility = Multi::RoomVisibility::INVITE_ONLY;
        privatePublish.isRelayOnly = true;
        privatePublish.gameName = "Entre amis";
        privatePublish.game.turns = Multi::RoomTurns::ONE_AFTER_ANOTHER;
        privatePublish.game.day = 23;
        privatePublish.publicAddresses.clear();
        privatePublish.localAddresses.clear();

        Multi::RoomStatus refusedStatus;
        refusedStatus.result = Multi::RoomStatusResult::TOO_MANY_ROOMS;

        const Multi::JoinPlan plan{ Multi::JoinResult::READY,
                                    roomInfo,
                                    { { Multi::JoinCandidateKind::LOCAL, "192.168.1.30", 47219 },
                                      { Multi::JoinCandidateKind::IPV6, "2001:db8::25", 47219 },
                                      { Multi::JoinCandidateKind::IPV4, "203.0.113.7", 47219 } },
                                    relayTicket };

        Multi::JoinPlan relayOnlyPlan = plan;
        relayOnlyPlan.candidates.clear();

        Multi::JoinPlan refusedPlan;
        refusedPlan.result = Multi::JoinResult::NO_SUCH_ROOM;

        // Every text as long as it can be.
        std::string longestNickname;
        for ( int i = 0; i < 20; ++i ) {
            longestNickname += "\xC3\x89";
        }

        std::string longestMessage;
        for ( int i = 0; i < 25; ++i ) {
            longestMessage += "Bienvenue ! ";
        }

        using Multi::MessageType;

        return { { "hello", Multi::makeLobbyHello( { 1, pcGame, "fr", nickname } ) },
                 { "hello_longest", Multi::makeLobbyHello( { 1, pcGame, "pt_BR", longestNickname } ) },
                 { "welcome_longest",
                   Multi::makeLobbyWelcome( { Multi::LobbyWelcomeResult::ACCEPTED, 1, std::string( 40, 'L' ), longestMessage, "ws://" + std::string( 115, 'r' ) } ) },
                 { "welcome", Multi::makeLobbyWelcome(
                                  { Multi::LobbyWelcomeResult::ACCEPTED, 1, "Salon fheroes2", "Bienvenue ! Soyez courtois.", "ws://relay.example.org:8080" } ) },
                 { "welcome_refused", Multi::makeLobbyWelcome( { Multi::LobbyWelcomeResult::INCOMPATIBLE_VERSIONS, 3, "Salon fheroes2", "", "" } ) },
                 { "stats", Multi::makeLobbyStats( { 12, 3, 4 } ) },
                 { "keepalive", Multi::makeEmptyLobbyMessage( MessageType::LOBBY_KEEPALIVE ) },
                 { "publish", Multi::makeRoomPublish( publish ) },
                 { "publish_private", Multi::makeRoomPublish( privatePublish ) },
                 { "publish_again", Multi::makeRoomPublish( publishAgain ) },
                 { "status", Multi::makeRoomStatus( { Multi::RoomStatusResult::PUBLISHED, roomId, "K7Q2MX", { true, true, true, true } } ) },
                 { "status_refused", Multi::makeRoomStatus( refusedStatus ) },
                 { "update", Multi::makeRoomUpdate( { Multi::RoomState::IN_GAME, laterRoomGame } ) },
                 { "close", Multi::makeEmptyLobbyMessage( MessageType::ROOM_CLOSE ) },
                 { "subscribe", Multi::makeRoomListSubscribe( true ) },
                 { "unsubscribe", Multi::makeRoomListSubscribe( false ) },
                 { "list", Multi::makeRoomList( { false, { roomInfo, otherRoomInfo } } ) },
                 { "list_empty", Multi::makeRoomList( { true, {} } ) },
                 { "info", Multi::makeRoomInfo( roomInfo ) },
                 { "removed", Multi::makeRoomRemoved( roomId ) },
                 { "join_id", Multi::makeRoomJoin( { roomId, "" } ) },
                 { "join_code", Multi::makeRoomJoin( { 0, "K7Q2MX" } ) },
                 { "plan", Multi::makeJoinPlan( plan ) },
                 { "plan_relay_only", Multi::makeJoinPlan( relayOnlyPlan ) },
                 { "plan_refused", Multi::makeJoinPlan( refusedPlan ) },
                 { "relay_host", Multi::makeLobbyKeyMessage( MessageType::RELAY_HOST, hostToken ) },
                 { "relay_guest", Multi::makeLobbyKeyMessage( MessageType::RELAY_GUEST, relayTicket ) },
                 { "relay_ready", Multi::makeEmptyLobbyMessage( MessageType::RELAY_READY ) },
                 { "relay_refused", Multi::makeRelayRefused( Multi::RelayRefusal::NO_HOST_WAITING ) },
                 { "relay_refused_closed", Multi::makeRelayRefused( Multi::RelayRefusal::ROOM_CLOSED ) },
                 { "probe", Multi::makeLobbyProbe( { probeKey, probeValue } ) },
                 { "probe_reply", Multi::makeLobbyKeyMessage( MessageType::LOBBY_PROBE_REPLY, probeValue ) } };
    }

    template <typename Value, typename Make>
    std::optional<std::vector<uint8_t>> writeBack( const std::optional<Value> & value, const Make & make )
    {
        if ( !value ) {
            return {};
        }

        return make( *value ).payload;
    }

    // Reads a lobby message with the reader of its type and writes it again. Returns nothing if the reader refuses it.
    std::optional<std::vector<uint8_t>> rewriteLobbyMessage( const Multi::Message & message )
    {
        using Multi::MessageType;

        switch ( message.type ) {
        case MessageType::LOBBY_HELLO:
            return writeBack( Multi::readLobbyHello( message ), Multi::makeLobbyHello );
        case MessageType::LOBBY_WELCOME:
            return writeBack( Multi::readLobbyWelcome( message ), Multi::makeLobbyWelcome );
        case MessageType::LOBBY_STATS:
            return writeBack( Multi::readLobbyStats( message ), Multi::makeLobbyStats );
        case MessageType::ROOM_PUBLISH:
            return writeBack( Multi::readRoomPublish( message ), Multi::makeRoomPublish );
        case MessageType::ROOM_STATUS:
            return writeBack( Multi::readRoomStatus( message ), Multi::makeRoomStatus );
        case MessageType::ROOM_UPDATE:
            return writeBack( Multi::readRoomUpdate( message ), Multi::makeRoomUpdate );
        case MessageType::ROOM_LIST_SUBSCRIBE:
            return writeBack( Multi::readRoomListSubscribe( message ), Multi::makeRoomListSubscribe );
        case MessageType::ROOM_LIST:
            return writeBack( Multi::readRoomList( message ), Multi::makeRoomList );
        case MessageType::ROOM_INFO:
            return writeBack( Multi::readRoomInfo( message ), Multi::makeRoomInfo );
        case MessageType::ROOM_REMOVED:
            return writeBack( Multi::readRoomRemoved( message ), Multi::makeRoomRemoved );
        case MessageType::ROOM_JOIN:
            return writeBack( Multi::readRoomJoin( message ), Multi::makeRoomJoin );
        case MessageType::JOIN_PLAN:
            return writeBack( Multi::readJoinPlan( message ), Multi::makeJoinPlan );
        case MessageType::RELAY_REFUSED:
            return writeBack( Multi::readRelayRefused( message ), Multi::makeRelayRefused );
        case MessageType::LOBBY_PROBE:
            return writeBack( Multi::readLobbyProbe( message ), Multi::makeLobbyProbe );
        case MessageType::LOBBY_KEEPALIVE:
        case MessageType::ROOM_CLOSE:
        case MessageType::RELAY_READY:
            if ( !Multi::readEmptyLobbyMessage( message, message.type ) ) {
                return {};
            }
            return Multi::makeEmptyLobbyMessage( message.type ).payload;
        case MessageType::RELAY_HOST:
        case MessageType::RELAY_GUEST:
        case MessageType::LOBBY_PROBE_REPLY:
            return writeBack( Multi::readLobbyKeyMessage( message, message.type ),
                              [&message]( const Multi::LobbyKey & key ) { return Multi::makeLobbyKeyMessage( message.type, key ); } );
        default:
            return {};
        }
    }

    std::optional<std::vector<uint8_t>> parseHex( const std::string_view text )
    {
        if ( text == "-" ) {
            return std::vector<uint8_t>{};
        }

        if ( text.size() % 2 != 0 ) {
            return {};
        }

        std::vector<uint8_t> bytes;
        bytes.reserve( text.size() / 2 );

        for ( size_t i = 0; i < text.size(); i += 2 ) {
            uint8_t byte = 0;
            const auto [end, error] = std::from_chars( text.data() + i, text.data() + i + 2, byte, 16 );
            if ( error != std::errc() || end != text.data() + i + 2 ) {
                return {};
            }

            bytes.push_back( byte );
        }

        return bytes;
    }

    // Lobby messages written by the lobby server (server/lobby/test/vectors.txt) must be read and written back the same by the game,
    // and wrong ones refused by both.
    bool runLobbyProtocolSelfTest( const std::string & vectorsPath )
    {
        const std::vector<uint8_t> fileBytes = readFileBytes( vectorsPath );
        if ( fileBytes.empty() ) {
            return failed( "unable to read '" + vectorsPath + "'" );
        }

        std::vector<std::pair<std::string, Multi::Message>> samples = makeLobbySamples();
        std::vector<std::string> errors;
        size_t readCount = 0;
        size_t refusedCount = 0;

        const std::string text( fileBytes.begin(), fileBytes.end() );
        size_t lineStart = 0;

        while ( lineStart < text.size() ) {
            size_t lineEnd = text.find( '\n', lineStart );
            if ( lineEnd == std::string::npos ) {
                lineEnd = text.size();
            }

            std::string_view line( text.data() + lineStart, lineEnd - lineStart );
            lineStart = lineEnd + 1;

            if ( !line.empty() && line.back() == '\r' ) {
                line.remove_suffix( 1 );
            }

            if ( line.empty() || line.front() == '#' ) {
                continue;
            }

            const size_t firstSpace = line.find( ' ' );
            const size_t secondSpace = line.find( ' ', firstSpace + 1 );
            if ( firstSpace == std::string_view::npos || secondSpace == std::string_view::npos ) {
                errors.emplace_back( "wrong line: " + std::string( line ) );
                continue;
            }

            const std::string name( line.substr( 0, firstSpace ) );
            const std::optional<uint32_t> type = parseNumber<uint32_t>( line.substr( firstSpace + 1, secondSpace - firstSpace - 1 ), 0, 0xFFFFFFFF );
            const std::optional<std::vector<uint8_t>> payload = parseHex( line.substr( secondSpace + 1 ) );
            if ( !type || !payload ) {
                errors.emplace_back( "wrong line: " + name );
                continue;
            }

            const Multi::Message message{ static_cast<Multi::MessageType>( *type ), *payload };
            const std::optional<std::vector<uint8_t>> rewritten = rewriteLobbyMessage( message );

            if ( name.front() == '!' ) {
                ++refusedCount;
                if ( rewritten ) {
                    errors.emplace_back( name.substr( 1 ) + " is accepted" );
                }
                continue;
            }

            ++readCount;
            if ( !rewritten ) {
                errors.emplace_back( name + " is refused" );
                continue;
            }

            if ( *rewritten != *payload ) {
                errors.emplace_back( name + " is not written back the same" );
            }

            const auto sample = std::find_if( samples.begin(), samples.end(), [&name]( const auto & item ) { return item.first == name; } );
            if ( sample == samples.end() ) {
                errors.emplace_back( name + " has no sample in the game" );
                continue;
            }

            if ( sample->second.type != message.type || sample->second.payload != *payload ) {
                errors.emplace_back( name + " is written differently by the game" );
            }

            samples.erase( sample );
        }

        for ( const auto & [name, message] : samples ) {
            errors.emplace_back( name + " is missing in the vectors" );
        }

        const auto check = [&errors]( const bool condition, const std::string & description ) {
            if ( !condition ) {
                errors.emplace_back( description );
            }
        };

        check( Multi::isValidNickname( "\xC3\x89lodie" ), "nickname with an accent is refused" );
        check( !Multi::isValidNickname( "" ), "empty nickname is accepted" );
        check( !Multi::isValidNickname( std::string( 21, 'x' ) ), "nickname of 21 characters is accepted" );
        check( !Multi::isValidNickname( "\xE9lodie" ), "nickname in a code page is accepted" );
        check( Multi::isValidGameName( std::string( 40, 'x' ) ) && !Multi::isValidGameName( std::string( 41, 'x' ) ), "length of game names" );
        check( Multi::isValidInviteCode( "K7Q2MX" ) && !Multi::isValidInviteCode( "K7Q2M0" ) && !Multi::isValidInviteCode( "k7q2mx" )
                   && !Multi::isValidInviteCode( "K7Q2M" ),
               "invite codes" );
        check( !Multi::readLobbyStats( { Multi::MessageType::LOBBY_WELCOME, Multi::makeLobbyStats( { 1, 2, 3 } ).payload } ), "message with a wrong type is accepted" );

        // Texts of the game in its code page, and UTF-8 in lobby messages. "Élodie", "Марк", "東", the euro sign.
        {
            using fheroes2::CodePage;
            const std::string markUtf8 = "\xD0\x9C\xD0\xB0\xD1\x80\xD0\xBA";

            check( Multi::toUtf8( "\xC9lodie", CodePage::CP1252 ) == "\xC3\x89lodie" && Multi::fromUtf8( "\xC3\x89lodie", CodePage::CP1252 ) == "\xC9lodie", "CP1252" );
            check( Multi::toUtf8( "\xCC\xE0\xF0\xEA", CodePage::CP1251 ) == markUtf8 && Multi::fromUtf8( markUtf8, CodePage::CP1251 ) == "\xCC\xE0\xF0\xEA", "CP1251" );
            check( Multi::toUtf8( "\x80", CodePage::CP1252 ) == "\xE2\x82\xAC", "euro sign" );
            check( Multi::fromUtf8( markUtf8, CodePage::CP1252 ) == "????", "letters the fonts cannot draw" );
            check( Multi::fromUtf8( "\xE6\x9D\xB1", CodePage::CP1252 ) == "?" && Multi::fromUtf8( "a\xC3", CodePage::CP1252 ) == "a?",
                   "one question mark per character" );
            check( Multi::toUtf8( "\xE9t\xE9", CodePage::ASCII ) == "?t?" && Multi::fromUtf8( "\xC3\xA9t\xC3\xA9", CodePage::ASCII ) == "?t?", "ASCII" );
            check( Multi::toUtf8( "\x81", CodePage::CP1252 ) == "?", "a character missing in the code page" );

            // Every character of every code page goes there and back.
            for ( const CodePage codePage : { CodePage::CP1250, CodePage::CP1251, CodePage::CP1252, CodePage::CP1253, CodePage::CP1254, CodePage::CP1258,
                                              CodePage::ISO8859_16, CodePage::ISO8859_3 } ) {
                for ( int byte = 0x20; byte < 0x100; ++byte ) {
                    const std::string character( 1, static_cast<char>( byte ) );
                    const std::string utf8 = Multi::toUtf8( character, codePage );
                    if ( utf8 != "?" ) {
                        check( Multi::fromUtf8( utf8, codePage ) == character && Multi::isValidNickname( utf8 ) == ( byte != 0x7F ),
                               "code page " + std::to_string( static_cast<int>( codePage ) ) + ", character " + std::to_string( byte ) );
                    }
                }
            }
        }

        if ( readCount == 0 || refusedCount == 0 ) {
            errors.emplace_back( "no vectors" );
        }

        // Random and damaged messages must be refused or read, never make the game stop.
        {
            const std::vector<std::pair<std::string, Multi::Message>> validMessages = makeLobbySamples();
            uint32_t state = 2463534242U;
            const auto next = [&state]() {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                return state;
            };

            for ( uint32_t i = 0; i < 5000; ++i ) {
                Multi::Message message = validMessages[i % validMessages.size()].second;
                if ( i % 3 == 0 ) {
                    message.payload.resize( next() % 200 );
                    for ( uint8_t & value : message.payload ) {
                        value = static_cast<uint8_t>( next() );
                    }
                }
                else if ( !message.payload.empty() ) {
                    if ( i % 3 == 1 ) {
                        message.payload.resize( next() % message.payload.size() );
                    }
                    else {
                        for ( uint32_t k = 0; k < 3; ++k ) {
                            message.payload[next() % message.payload.size()] = static_cast<uint8_t>( next() );
                        }
                    }
                }

                (void)rewriteLobbyMessage( message );
            }
        }

        if ( !errors.empty() ) {
            std::string details;
            for ( const std::string & error : errors ) {
                details += ( details.empty() ? "" : ", " ) + error;
            }

            return failed( "lobby protocol: " + details );
        }

        return passed( "lobby protocol: " + std::to_string( readCount ) + " messages read and written back the same, " + std::to_string( refusedCount )
                       + " wrong messages refused" );
    }

    std::string toHexText( const uint8_t * data, const size_t size )
    {
        constexpr std::string_view digits{ "0123456789abcdef" };
        std::string text;
        for ( size_t i = 0; i < size; ++i ) {
            text += digits[data[i] >> 4];
            text += digits[data[i] & 0xF];
        }
        return text;
    }

    // A frame as a server writes it. A server never masks its frames, a masked one is for checking that the client refuses it.
    std::vector<uint8_t> makeServerFrame( const uint8_t first, const std::vector<uint8_t> & payload, const bool isMasked = false )
    {
        std::vector<uint8_t> frame{ first };
        const size_t size = payload.size();
        const uint8_t maskBit = isMasked ? 0x80 : 0;

        if ( size < 126 ) {
            frame.push_back( static_cast<uint8_t>( maskBit | size ) );
        }
        else if ( size < 0x10000 ) {
            frame.push_back( maskBit | 126 );
            frame.push_back( static_cast<uint8_t>( size >> 8 ) );
            frame.push_back( static_cast<uint8_t>( size ) );
        }
        else {
            frame.push_back( maskBit | 127 );
            for ( int shift = 56; shift >= 0; shift -= 8 ) {
                frame.push_back( static_cast<uint8_t>( static_cast<uint64_t>( size ) >> shift ) );
            }
        }

        if ( isMasked ) {
            frame.insert( frame.end(), 4, 0 );
        }
        frame.insert( frame.end(), payload.begin(), payload.end() );

        return frame;
    }

    // The opcode and the payload of a frame written by the client, or nothing if it is not masked or not whole.
    std::optional<std::pair<uint8_t, std::vector<uint8_t>>> readClientFrame( const std::vector<uint8_t> & frame )
    {
        if ( frame.size() < 6 || ( frame[1] & 0x80 ) == 0 ) {
            return {};
        }

        uint64_t size = frame[1] & 0x7F;
        size_t offset = 2;
        if ( size == 126 ) {
            size = ( static_cast<uint64_t>( frame[2] ) << 8 ) | frame[3];
            offset = 4;
        }
        else if ( size == 127 ) {
            size = 0;
            for ( size_t i = 2; i < 10; ++i ) {
                size = ( size << 8 ) | frame[i];
            }
            offset = 10;
        }

        if ( frame.size() != offset + 4 + size ) {
            return {};
        }

        std::vector<uint8_t> payload( frame.begin() + static_cast<std::ptrdiff_t>( offset + 4 ), frame.end() );
        for ( size_t i = 0; i < payload.size(); ++i ) {
            payload[i] ^= frame[offset + ( i & 3 )];
        }

        return std::make_pair( static_cast<uint8_t>( frame[0] & 0x0F ), payload );
    }

    // The answer of a server accepting the upgrade request.
    std::vector<uint8_t> makeUpgradeAnswer( const std::vector<uint8_t> & request )
    {
        const std::string text( request.begin(), request.end() );
        const std::string keyHeader = "Sec-WebSocket-Key: ";
        const size_t keyStart = text.find( keyHeader ) + keyHeader.size();
        const std::string key = text.substr( keyStart, text.find( "\r\n", keyStart ) - keyStart );

        const std::string answer = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "
                                   + Multi::computeWebSocketAccept( key ) + "\r\n\r\n";
        return { answer.begin(), answer.end() };
    }

    bool runWebSocketSelfTest()
    {
        std::vector<std::string> errors;
        const auto check = [&errors]( const bool condition, const std::string & description ) {
            if ( !condition ) {
                errors.push_back( description );
            }
        };

        const auto sha1 = []( const std::string & text ) {
            const std::array<uint8_t, 20> digest = Multi::computeSha1( text );
            return toHexText( digest.data(), digest.size() );
        };
        const auto base64 = []( const std::string & text ) { return Multi::encodeBase64( reinterpret_cast<const uint8_t *>( text.data() ), text.size() ); };

        // Examples of FIPS 180, RFC 4648 and RFC 6455.
        check( sha1( "abc" ) == "a9993e364706816aba3e25717850c26c9cd0d89d", "SHA-1 of abc" );
        check( sha1( "" ) == "da39a3ee5e6b4b0d3255bfef95601890afd80709", "SHA-1 of nothing" );
        check( sha1( "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" ) == "84983e441c3bd26ebaae4aa1f95129e5e54670f1", "SHA-1 of two blocks" );
        check( base64( "" ).empty() && base64( "f" ) == "Zg==" && base64( "fo" ) == "Zm8=" && base64( "foo" ) == "Zm9v" && base64( "foobar" ) == "Zm9vYmFy", "base64" );
        check( Multi::computeWebSocketAccept( "dGhlIHNhbXBsZSBub25jZQ==" ) == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", "accept key of the RFC" );

        // Lobby addresses.
        const auto parsed = []( const std::string & text ) {
            const std::optional<Multi::LobbyAddress> address = Multi::parseLobbyAddress( text );
            if ( !address ) {
                return std::string( "-" );
            }
            return ( address->isSecure ? "wss " : "ws " ) + address->host + " " + std::to_string( address->port ) + " " + address->path;
        };

        const std::vector<std::pair<std::string, std::string>> addresses{ { "lobby.example.org", "ws lobby.example.org 47220 /lobby" },
                                                                          { "lobby.example.org:8080", "ws lobby.example.org 8080 /lobby" },
                                                                          { "ws://lobby.example.org", "ws lobby.example.org 80 /lobby" },
                                                                          { "ws://lobby.example.org/other/path", "ws lobby.example.org 80 /other/path" },
                                                                          { "WS://Lobby.Example.org:81/x", "ws Lobby.Example.org 81 /x" },
                                                                          { "wss://lobby.example.org", "wss lobby.example.org 443 /lobby" },
                                                                          { "[2001:db8::1]:9000", "ws 2001:db8::1 9000 /lobby" },
                                                                          { "2001:db8::1", "ws 2001:db8::1 47220 /lobby" },
                                                                          { " 127.0.0.1 ", "ws 127.0.0.1 47220 /lobby" },
                                                                          { "", "-" },
                                                                          { ":80", "-" },
                                                                          { "lobby.example.org:0", "-" },
                                                                          { "lobby.example.org:65536", "-" },
                                                                          { "lobby.example.org:", "-" },
                                                                          { "lob by", "-" },
                                                                          { "[2001:db8::1", "-" },
                                                                          { "[2001:db8::1]x", "-" },
                                                                          { "ws://", "-" },
                                                                          { "lobby.example.org/a b", "-" } };
        for ( const auto & [text, expected] : addresses ) {
            check( parsed( text ) == expected, "lobby address '" + text + "' gives '" + parsed( text ) + "'" );
        }

        // A whole exchange, received byte by byte.
        {
            Multi::WebSocketClient client( "lobby.example.org", 80, "/lobby", 1024 );
            const std::vector<uint8_t> request = client.makeRequest();
            check( std::string( request.begin(), request.end() ).rfind( "GET /lobby HTTP/1.1\r\nHost: lobby.example.org\r\nUpgrade: websocket\r\n", 0 ) == 0,
                   "upgrade request" );

            std::vector<uint8_t> received = makeUpgradeAnswer( request );
            const auto add = [&received]( const std::vector<uint8_t> & frame ) { received.insert( received.end(), frame.begin(), frame.end() ); };
            add( makeServerFrame( 0x82, { 'a', 'b', 'c' } ) );
            add( makeServerFrame( 0x02, { 'd', 'e' } ) );
            add( makeServerFrame( 0x89, { 'p' } ) );
            add( makeServerFrame( 0x80, { 'f' } ) );
            add( makeServerFrame( 0x8A, {} ) );
            add( makeServerFrame( 0x82, std::vector<uint8_t>( 300, 7 ) ) );
            add( makeServerFrame( 0x88, { 0x03, 0xE8 } ) );

            bool isAccepted = true;
            for ( const uint8_t byte : received ) {
                isAccepted = client.receive( &byte, 1 ) && isAccepted;
            }

            check( isAccepted && client.isOpen() && client.error().empty(), "exchange refused: " + client.error() );
            check( client.takeMessage() == std::vector<uint8_t>{ 'a', 'b', 'c' }, "first message" );
            check( client.takeMessage() == std::vector<uint8_t>{ 'd', 'e', 'f' }, "message in fragments" );
            check( client.takeMessage() == std::vector<uint8_t>( 300, 7 ), "message of 300 bytes" );
            check( !client.takeMessage(), "no other message" );
            check( client.takePongs() == 1, "pong" );
            check( client.isClosedByServer() && client.closeCode() == 1000, "close of the server" );

            // The answer to the ping between two fragments, then the answer to the close.
            const std::vector<uint8_t> replies = client.takeReplies();
            check( replies.size() == 7 + 6 + 2, "replies" );
            if ( replies.size() == 7 + 6 + 2 ) {
                check( readClientFrame( { replies.begin(), replies.begin() + 7 } ) == std::make_pair( uint8_t{ 10 }, std::vector<uint8_t>{ 'p' } ), "pong answer" );
                check( readClientFrame( { replies.begin() + 7, replies.end() } ) == std::make_pair( uint8_t{ 8 }, std::vector<uint8_t>{ 0x03, 0xE8 } ), "close answer" );
            }
        }

        // Frames of the client are masked, with sizes on 7, 16 and 64 bits.
        {
            Multi::WebSocketClient client( "127.0.0.1", 47220, "/lobby", 1024 );
            for ( const size_t size : { 0, 125, 126, 65535, 65536 } ) {
                std::vector<uint8_t> data( size );
                for ( size_t i = 0; i < size; ++i ) {
                    data[i] = static_cast<uint8_t>( i * 7 );
                }
                const auto frame = readClientFrame( client.makeMessage( data.data(), data.size() ) );
                check( frame && frame->first == 2 && frame->second == data, "client frame of " + std::to_string( size ) + " bytes" );
            }

            const std::vector<uint8_t> request = client.makeRequest();
            check( std::string( request.begin(), request.end() ).find( "\r\nHost: 127.0.0.1:47220\r\n" ) != std::string::npos, "host and port in the request" );
        }

        // Wrong answers and frames close the connection.
        {
            // The given answer, or the right one followed by the given frames.
            const auto isRefused = []( const std::string & answer, const std::vector<std::vector<uint8_t>> & frames ) {
                Multi::WebSocketClient client( "lobby.example.org", 80, "/lobby", 1024 );
                const std::vector<uint8_t> request = client.makeRequest();

                std::vector<uint8_t> data = answer.empty() ? makeUpgradeAnswer( request ) : std::vector<uint8_t>( answer.begin(), answer.end() );
                for ( const std::vector<uint8_t> & frame : frames ) {
                    data.insert( data.end(), frame.begin(), frame.end() );
                }

                return !client.receive( data.data(), data.size() ) && !client.error().empty();
            };
            const auto text = [&isRefused]( const std::string & answer ) { return isRefused( answer, {} ); };
            const auto withFrames = [&isRefused]( const std::vector<std::vector<uint8_t>> & frames ) { return isRefused( {}, frames ); };

            check( text( "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n" ), "not found is accepted" );
            check( text( "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: abc\r\n\r\n" ),
                   "wrong accept key is accepted" );
            check( text( std::string( 9000, 'a' ) ), "endless answer is accepted" );

            {
                Multi::WebSocketClient client( "lobby.example.org", 80, "/lobby", 1024 );
                std::vector<uint8_t> answer = makeUpgradeAnswer( client.makeRequest() );
                const std::string upgrade = "Upgrade: websocket\r\n";
                const auto start = std::search( answer.begin(), answer.end(), upgrade.begin(), upgrade.end() );
                answer.erase( start, start + static_cast<std::ptrdiff_t>( upgrade.size() ) );
                check( !client.receive( answer.data(), answer.size() ), "answer without upgrade is accepted" );
            }
            check( withFrames( { makeServerFrame( 0x82, { 1 }, true ) } ), "masked server frame is accepted" );
            check( withFrames( { makeServerFrame( 0x81, { 'a' } ) } ), "text frame is accepted" );
            check( withFrames( { { 0x82, 126, 0x07, 0xD0 } } ), "frame bigger than the limit is accepted" );
            check( withFrames( { { 0x82, 127, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } } ), "huge frame is accepted" );
            check( withFrames( { makeServerFrame( 0x09, { 1 } ) } ), "fragmented ping is accepted" );
            check( withFrames( { makeServerFrame( 0x80, { 1 } ) } ), "continuation without a start is accepted" );
            check( withFrames( { makeServerFrame( 0x02, { 1 } ), makeServerFrame( 0x82, { 2 } ) } ), "new message in the middle of another is accepted" );
            check( withFrames( { makeServerFrame( 0xC2, { 1 } ) } ), "reserved bits are accepted" );
            check( withFrames( { makeServerFrame( 0x02, std::vector<uint8_t>( 600 ) ), makeServerFrame( 0x80, std::vector<uint8_t>( 600 ) ) } ),
                   "fragments bigger than the limit are accepted" );
        }

        // Random data never makes the game stop.
        {
            uint32_t state = 2463534242U;
            for ( uint32_t i = 0; i < 2000; ++i ) {
                Multi::WebSocketClient client( "lobby.example.org", 80, "/lobby", 1024 );
                std::vector<uint8_t> data = makeUpgradeAnswer( client.makeRequest() );
                for ( uint32_t k = i % 64; k > 0; --k ) {
                    state ^= state << 13;
                    state ^= state >> 17;
                    state ^= state << 5;
                    data.push_back( static_cast<uint8_t>( state ) );
                }
                (void)client.receive( data.data(), data.size() );
                while ( client.takeMessage() ) {
                    // Only reading.
                }
            }
        }

        if ( !errors.empty() ) {
            std::string details;
            for ( const std::string & error : errors ) {
                details += ( details.empty() ? "" : ", " ) + error;
            }
            return failed( "websocket: " + details );
        }

        return passed( "websocket: SHA-1, addresses, frames, wrong answers and random data" );
    }

    // Connects to a lobby server running on this computer, publishes a room, finds it in the list and closes it. Then wrong
    // addresses must each give a clear reason.
    bool runLobbyClientSelfTest( const std::string & address )
    {
        Multi::LobbyConnection lobby;
        lobby.open( address );

        std::deque<Multi::Message> received;
        const auto waitFor = [&lobby, &received]( const Multi::MessageType type ) -> std::optional<Multi::Message> {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
            while ( std::chrono::steady_clock::now() < deadline ) {
                lobby.update();
                while ( std::optional<Multi::Message> message = lobby.receive() ) {
                    received.push_back( std::move( *message ) );
                }

                const auto found = std::find_if( received.begin(), received.end(), [type]( const Multi::Message & message ) { return message.type == type; } );
                if ( found != received.end() ) {
                    Multi::Message message = std::move( *found );
                    received.erase( found );
                    return message;
                }

                if ( lobby.state() == Multi::LobbyConnection::State::CLOSED ) {
                    return {};
                }
                std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
            }
            return {};
        };

        const Multi::HandshakeInfo local = Multi::getLocalHandshakeInfo();

        // Sent before the connection is open.
        lobby.send( Multi::makeLobbyHello( { Multi::lobbyProtocolVersion, local, "fr", "Self-test" } ) );

        const std::optional<Multi::Message> welcomeMessage = waitFor( Multi::MessageType::LOBBY_WELCOME );
        const std::optional<Multi::LobbyWelcome> welcome = welcomeMessage ? Multi::readLobbyWelcome( *welcomeMessage ) : std::nullopt;
        if ( !welcome || welcome->result != Multi::LobbyWelcomeResult::ACCEPTED ) {
            return failed( "lobby client: no welcome from the lobby: " + lobby.error() );
        }

        Multi::RoomPublish publish;
        publish.isRelayOnly = true;
        publish.gameName = "Self-test";
        publish.game = { "Map", 36, 36, 2, 2, 1, Multi::RoomTurns::ONE_AFTER_ANOTHER, 1 };
        publish.gamePort = Multi::defaultPort;
        lobby.send( Multi::makeRoomPublish( publish ) );

        // A first status at once, a second one when the probe is over (at once too, there is nothing to probe).
        std::optional<Multi::RoomStatus> status;
        while ( !status || !status->reach.isProbeDone ) {
            const std::optional<Multi::Message> statusMessage = waitFor( Multi::MessageType::ROOM_STATUS );
            status = statusMessage ? Multi::readRoomStatus( *statusMessage ) : std::nullopt;
            if ( !status || status->result != Multi::RoomStatusResult::PUBLISHED ) {
                return failed( "lobby client: room not published: " + lobby.error() );
            }
        }

        lobby.send( Multi::makeRoomListSubscribe( true ) );

        bool isListed = false;
        for ( ;; ) {
            const std::optional<Multi::Message> listMessage = waitFor( Multi::MessageType::ROOM_LIST );
            const std::optional<Multi::RoomList> list = listMessage ? Multi::readRoomList( *listMessage ) : std::nullopt;
            if ( !list ) {
                return failed( "lobby client: no list of rooms: " + lobby.error() );
            }

            isListed
                = isListed || std::any_of( list->rooms.begin(), list->rooms.end(), [&status, &local]( const Multi::RoomInfo & room ) {
                      return room.roomId == status->roomId && room.gameName == "Self-test" && room.hostNickname == "Self-test" && room.hostGame.buildId == local.buildId;
                  } );
            if ( list->isLastPart ) {
                break;
            }
        }

        const std::optional<Multi::Message> statsMessage = waitFor( Multi::MessageType::LOBBY_STATS );
        const std::optional<Multi::LobbyStats> stats = statsMessage ? Multi::readLobbyStats( *statsMessage ) : std::nullopt;
        if ( !isListed || !stats || stats->playersOnline == 0 || stats->waitingRooms == 0 ) {
            return failed( "lobby client: the room is not in the list" );
        }

        const auto pingDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
        while ( lobby.delay().count() == 0 && std::chrono::steady_clock::now() < pingDeadline ) {
            lobby.update();
            std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
        }

        lobby.send( Multi::makeEmptyLobbyMessage( Multi::MessageType::ROOM_CLOSE ) );
        const std::optional<Multi::Message> removedMessage = waitFor( Multi::MessageType::ROOM_REMOVED );
        if ( !removedMessage || Multi::readRoomRemoved( *removedMessage ) != status->roomId ) {
            return failed( "lobby client: the room is not removed from the list" );
        }

        const auto delay = lobby.delay();
        lobby.close();
        if ( delay.count() == 0 || lobby.state() != Multi::LobbyConnection::State::CLOSED || !lobby.error().empty() ) {
            return failed( "lobby client: no ping answered or connection not closed cleanly" );
        }

        const std::optional<Multi::LobbyAddress> lobbyAddress = Multi::parseLobbyAddress( address );

        // The probe of the lobby reaches a game hosted on this computer, which answers only with its own key.
        for ( const bool isRightKey : { true, false } ) {
            Multi::NetworkSession host;
            const uint16_t gamePort = static_cast<uint16_t>( lobbyAddress->port + 1 );
            if ( !host.host( gamePort, true ) ) {
                return failed( "lobby client: unable to host for the probe: " + host.error() );
            }

            Multi::RoomPublish probed = publish;
            probed.isRelayOnly = false;
            probed.gamePort = gamePort;
            probed.probeKey = Multi::makeRandomLobbyKey();
            host.setProbeKey( isRightKey ? probed.probeKey : Multi::makeRandomLobbyKey() );

            Multi::PublishedRoom room( address, "Self-test", probed );

            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
            while ( !room.status().reach.isProbeDone && room.state() != Multi::PublishedRoom::State::FAILED && std::chrono::steady_clock::now() < deadline ) {
                room.update();
                host.update();
                std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
            }

            const Multi::RoomReach & reach = room.status().reach;
            if ( !reach.isProbeDone || reach.isIPv4 != isRightKey || host.state() != Multi::NetworkSession::State::LISTENING ) {
                return failed( std::string( "lobby client: the probe with the " ) + ( isRightKey ? "right" : "wrong" ) + " key gives the wrong answer: " + room.error() );
            }
        }

        // Wrong addresses, each with its reason.
        const std::string host = lobbyAddress && lobbyAddress->host.find( ':' ) != std::string::npos ? "[" + lobbyAddress->host + "]"
                                 : lobbyAddress                                                      ? lobbyAddress->host
                                                                                                     : "";
        const std::string otherPath = "ws://" + host + ":" + std::to_string( lobbyAddress ? lobbyAddress->port : 0 ) + "/other";

        const std::vector<std::pair<std::string, std::string>> wrongAddresses{ { "lob by", "wrong lobby address" },
                                                                               { "wss://lobby.example.org", "secure connections to lobbies are not supported yet" },
                                                                               { otherPath, "this address is not a lobby" },
                                                                               { "127.0.0.1:1", "there is no lobby at this address" } };

        const auto describeMismatch = []( const std::string & wrongAddress, const std::string & error, const std::string & expected ) {
            return "lobby client: '" + wrongAddress + "' gives '" + error + "' instead of '" + expected + "'";
        };

        std::string reasons;
        for ( const auto & [wrongAddress, expected] : wrongAddresses ) {
            Multi::LobbyConnection wrong;
            wrong.open( wrongAddress );

            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
            while ( wrong.state() != Multi::LobbyConnection::State::CLOSED && std::chrono::steady_clock::now() < deadline ) {
                wrong.update();
                std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
            }

            if ( wrong.error() != expected ) {
                return failed( describeMismatch( wrongAddress, wrong.error(), expected ) );
            }

            if ( !reasons.empty() ) {
                reasons += ", ";
            }
            reasons += expected;
        }

        return passed( "lobby client: room published, listed and closed, probe answered only with its key, delay " + std::to_string( delay.count() ) + " ms; "
                       + reasons );
    }

    std::optional<bool> runSelfTest( const std::vector<std::string_view> & args )
    {
        const std::string_view command = args[0];

        if ( command == "--multi-selftest-save" && args.size() == 2 ) {
            return runSaveLoadSelfTest( std::string( args[1] ) );
        }

        if ( command == "--multi-selftest-save-parts" && args.size() >= 2 ) {
            return runSavePartsSelfTest( std::vector<std::string>( args.begin() + 1, args.end() ) );
        }

        if ( command == "--multi-selftest-contact" && args.size() >= 2 ) {
            return runContactSelfTest( std::vector<std::string>( args.begin() + 1, args.end() ) );
        }

        if ( command == "--multi-selftest-contact-cases" && args.size() == 2 ) {
            return runContactCasesSelfTest( std::string( args[1] ) );
        }

        if ( command == "--multi-selftest-dialogs" && args.size() == 2 ) {
            return runDialogsSelfTest( std::string( args[1] ) );
        }

        if ( command == "--multi-selftest-port-mapping" && args.size() == 3 ) {
            return runPortMappingSelfTest( std::string( args[1] ), std::string( args[2] ) );
        }

        if ( command == "--multi-selftest-protocol" && args.size() == 1 ) {
            return runProtocolSelfTest();
        }

        if ( command == "--multi-selftest-lobby-protocol" && args.size() == 2 ) {
            return runLobbyProtocolSelfTest( std::string( args[1] ) );
        }

        if ( command == "--multi-selftest-websocket" && args.size() == 1 ) {
            return runWebSocketSelfTest();
        }

        if ( command == "--multi-selftest-lobby-client" && args.size() == 2 ) {
            return runLobbyClientSelfTest( std::string( args[1] ) );
        }

        if ( command == "--multi-selftest-host" && args.size() == 2 ) {
            if ( const std::optional<uint16_t> port = parsePort( args[1] ); port ) {
                return runHostSelfTest( *port );
            }
        }

        if ( command == "--multi-selftest-join" && args.size() == 3 ) {
            if ( const std::optional<uint16_t> port = parsePort( args[2] ); port ) {
                return runJoinSelfTest( std::string( args[1] ), *port );
            }
        }

        if ( command == "--multi-selftest-resume-host" && args.size() == 2 ) {
            if ( const std::optional<uint16_t> port = parsePort( args[1] ); port ) {
                return runResumeHostSelfTest( *port );
            }
        }

        if ( command == "--multi-selftest-resume-join" && args.size() == 3 ) {
            if ( const std::optional<uint16_t> port = parsePort( args[2] ); port ) {
                return runResumeJoinSelfTest( std::string( args[1] ), *port );
            }
        }

        if ( command == "--multi-selftest-routing-host" && args.size() == 3 ) {
            const std::optional<uint16_t> port = parsePort( args[1] );
            const std::optional<uint32_t> guests = parseNumber<uint32_t>( args[2], 1, Multi::maxGuests );
            if ( port && guests ) {
                return runRoutingHostSelfTest( *port, static_cast<Multi::MachineId>( *guests ) );
            }
        }

        if ( command == "--multi-selftest-routing-join" && args.size() == 3 ) {
            if ( const std::optional<uint16_t> port = parsePort( args[2] ); port ) {
                return runRoutingJoinSelfTest( std::string( args[1] ), *port );
            }
        }

        if ( command == "--multi-selftest-game-host" && ( args.size() == 4 || args.size() == 5 ) ) {
            const std::optional<uint16_t> port = parsePort( args[2] );
            const std::optional<uint32_t> days = parseNumber<uint32_t>( args[3], 1, 400 );
            const std::optional<uint32_t> humans = ( args.size() == 5 ) ? parseNumber<uint32_t>( args[4], 2, 6 ) : std::optional<uint32_t>( 2 );
            if ( port && days && humans ) {
                return runGameHostSelfTest( std::string( args[1] ), *port, *days, *humans );
            }
        }

        if ( command == "--multi-selftest-game-join" && args.size() == 3 ) {
            if ( const std::optional<uint16_t> port = parsePort( args[2] ); port ) {
                return runGameJoinSelfTest( std::string( args[1] ), *port );
            }
        }

        if ( command == "--multi-selftest-battle-host" && args.size() == 3 ) {
            if ( const std::optional<uint16_t> port = parsePort( args[2] ); port ) {
                return runBattleHostSelfTest( std::string( args[1] ), *port );
            }
        }

        if ( command == "--multi-selftest-battle-join" && args.size() == 3 ) {
            if ( const std::optional<uint16_t> port = parsePort( args[2] ); port ) {
                return runBattleJoinSelfTest( std::string( args[1] ), *port );
            }
        }

        if ( command == "--multi-selftest-gameover-host" && args.size() == 3 ) {
            if ( const std::optional<uint16_t> port = parsePort( args[2] ); port ) {
                return runGameOverHostSelfTest( std::string( args[1] ), *port );
            }
        }

        if ( command == "--multi-selftest-gameover-join" && args.size() == 3 ) {
            if ( const std::optional<uint16_t> port = parsePort( args[2] ); port ) {
                return runGameOverJoinSelfTest( std::string( args[1] ), *port );
            }
        }

        return {};
    }
}

std::optional<int> Multi::runSelfTestIfRequested( const int argc, const char * const * argv )
{
    if ( argc < 2 || std::string_view( argv[1] ).rfind( "--multi-selftest", 0 ) != 0 ) {
        return {};
    }

#if defined( _WIN32 )
    // A failed assertion must end the self-test, not wait for a click in a message box.
    _set_error_mode( _OUT_TO_STDERR );
    _set_abort_behavior( 0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT );
#endif

    std::vector<std::string_view> args( argv + 1, argv + argc );

    // Options with a value.
    if ( const auto lobbyOption = std::find( args.begin() + 1, args.end(), "--multi-test-lobby" ); lobbyOption != args.end() && lobbyOption + 1 != args.end() ) {
        lobbyTestAddress = std::string( *( lobbyOption + 1 ) );
        args.erase( lobbyOption, lobbyOption + 2 );
    }
    if ( const auto nameOption = std::find( args.begin() + 1, args.end(), "--multi-test-lobby-game" ); nameOption != args.end() && nameOption + 1 != args.end() ) {
        lobbyTestGameName = std::string( *( nameOption + 1 ) );
        args.erase( nameOption, nameOption + 2 );
    }
    if ( const auto guestsOption = std::find( args.begin() + 1, args.end(), "--multi-test-guests" ); guestsOption != args.end() && guestsOption + 1 != args.end() ) {
        if ( const std::optional<uint32_t> guests = parseNumber<uint32_t>( *( guestsOption + 1 ), 1, Multi::maxGuests ); guests ) {
            testGuestCount = *guests;
        }
        args.erase( guestsOption, guestsOption + 2 );
    }

    // Test modes at the end of the command line
    while ( args.size() > 1
            && ( args.back() == "--multi-watch-actions" || args.back() == "--multi-reserve" || args.back() == "--multi-simultaneous"
                 || args.back() == "--multi-test-jumps" || args.back() == "--multi-test-toggle" || args.back() == "--multi-test-attacks"
                 || args.back() == "--multi-test-spectate" || args.back() == "--multi-test-other-sources" || args.back() == "--multi-test-lan"
                 || args.back() == "--multi-test-cuts" || args.back() == "--multi-test-relay" || args.back() == "--multi-test-pick-colors"
                 || args.back() == "--multi-test-absence-window" ) ) {
        if ( args.back() == "--multi-watch-actions" ) {
            Multi::enableActionWatch();
        }
        else if ( args.back() == "--multi-test-jumps" ) {
            Multi::enableTestJumps();
        }
        else if ( args.back() == "--multi-test-toggle" ) {
            Multi::enableTestToggle();
        }
        else if ( args.back() == "--multi-test-attacks" ) {
            Multi::enableTestAttacks();
        }
        else if ( args.back() == "--multi-test-spectate" ) {
            Multi::enableTestSpectate();
            Multi::showEveryBattleForTest();
        }
        else if ( args.back() == "--multi-reserve" ) {
            Multi::enableReservations();
        }
        else if ( args.back() == "--multi-test-other-sources" ) {
            Multi::pretendOtherSourcesForTest();
        }
        else if ( args.back() == "--multi-test-lan" ) {
            isLanTest = true;
        }
        else if ( args.back() == "--multi-test-cuts" ) {
            isCutTest = true;
        }
        else if ( args.back() == "--multi-test-relay" ) {
            isRelayTest = true;
        }
        else if ( args.back() == "--multi-test-pick-colors" ) {
            isPickingColorsTest = true;
        }
        else if ( args.back() == "--multi-test-absence-window" ) {
            Multi::enableTestAbsenceWindow();
        }
        else {
            Multi::setSimultaneousTurns( true );
        }

        args.pop_back();
    }

    const std::optional<bool> result = runSelfTest( args );
    if ( !result ) {
        failed( "invalid command line" );
        return EXIT_FAILURE;
    }

    return *result ? EXIT_SUCCESS : EXIT_FAILURE;
}
