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

#include "multi_game.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <exception>
#include <map>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "castle.h"
#include "dialog.h"
#include "exception.h"
#include "game.h"
#include "game_exit.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_io.h"
#include "game_mainmenu_ui.h"
#include "heroes.h"
#include "interface_gamearea.h"
#include "interface_icons.h"
#include "interface_radar.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "maps_fileinfo.h"
#include "multi_action_watch.h"
#include "multi_android.h"
#include "multi_battle.h"
#include "multi_connection.h"
#include "multi_crash_report.h"
#include "multi_fingerprint.h"
#include "multi_game_over.h"
#include "multi_hero.h"
#include "multi_live.h"
#include "multi_live_battle.h"
#include "multi_lobby.h"
#include "multi_lobby_client.h"
#include "multi_machines.h"
#include "multi_network.h"
#include "multi_notice.h"
#include "multi_port_mapping.h"
#include "multi_reservation.h"
#include "multi_settings.h"
#include "multi_simultaneous.h"
#include "multi_spectator.h"
#include "multi_text.h"
#include "multi_waiting_room.h"
#include "multi_waiting_screen.h"
#include "players.h"
#include "save_format_version.h"
#include "screen.h"
#include "serialize.h"
#include "settings.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    enum class Role
    {
        NONE,
        HOST,
        GUEST
    };

    // A computer is paused for shorter cuts only when something waits for it.
    constexpr std::chrono::seconds absenceDelay{ 2 };

    struct NetworkGame
    {
        Role role{ Role::NONE };
        std::unique_ptr<Multi::NetworkSession> session;

        // The local player whose kingdom is shown while the other players play.
        PlayerColor localColor{ PlayerColor::NONE };
        bool isObserving{ false };

        // The player whose turn was last sent here as a game to look at.
        PlayerColor playingColor{ PlayerColor::NONE };

        std::optional<Multi::TurnStage> resumeStage;

        // Host only. The player whose classic turn is played on a guest.
        PlayerColor otherTurnColor{ PlayerColor::NONE };

        // Host only. A game loaded from a save file: the turn loop has not started yet.
        bool isLoadedAtStart{ false };

        // Host only. The keys the guests gave in their handshake, to know them when they come back after a restart.
        std::map<Multi::MachineId, uint64_t> machineKeys;

        // Host only. Guests which came back and must get the game.
        Multi::MachineSet resyncPending{ 0 };

        // On the guest, the session with the restarted host. Used once the game leaves the current one.
        std::unique_ptr<Multi::NetworkSession> nextSession;

        // Port opened on the internet box of the host until the game ends.
        std::unique_ptr<Multi::PortMapping> portMapping;

        // Room of the host on a lobby, kept until the game ends.
        std::unique_ptr<Multi::PublishedRoom> publishedRoom;

        Multi::NetworkGameTestOptions testOptions;
        Multi::NetworkGameStats stats;
    };

    NetworkGame & networkGame()
    {
        static NetworkGame game;
        return game;
    }

    const char * stageName( const Multi::TurnStage stage )
    {
        switch ( stage ) {
        case Multi::TurnStage::IN_PROGRESS:
            return "in progress";
        case Multi::TurnStage::NOT_STARTED:
            return "not started";
        case Multi::TurnStage::ENDED:
            return "ended";
        default:
            break;
        }

        return "unknown";
    }

    // The last games sent and received, to compare them when fingerprints differ. Use the autosave to resume a game.
    std::string getTurnFilePath( const char * name )
    {
        return System::concatPath( Game::GetSaveDir(), std::string( name ) + Game::GetSaveFileExtension( Game::TYPE_HOTSEAT ) );
    }

    // The guest keeps no copy of the whole game on its disk, what its fog of war hides included. Self-tests keep them.
    void removeGuestTurnFile( const std::string & filePath )
    {
        if ( Multi::isGuest() && !Multi::isTestMode() ) {
            System::Unlink( filePath );
        }
    }

    // Only during the turn of a local player, not while the AI players play.
    bool canFocusHere()
    {
        const Player * player = Players::Get( Settings::Get().CurrentColor() );
        return player != nullptr && ( Multi::getLocalHumanColors() & player->GetColor() ) != 0
               && ( player->isControlHuman() || ( player->isControlAI() && player->isAIAutoControlMode() ) );
    }

    void refreshObservedMap()
    {
        Interface::AdventureMap & adventureMap = Interface::AdventureMap::Get();

        Interface::GameArea::updateMapFogDirections();
        adventureMap.getRadar().Build();
        adventureMap.GetIconsPanel().resetIcons( ICON_ANY );

        // The hero or castle in focus may have been taken, or the hero defeated.
        const PlayerColor color = Settings::Get().CurrentColor();
        Heroes * hero = Interface::GetFocusHeroes();
        Castle * castle = Interface::GetFocusCastle();

        if ( canFocusHere() ) {
            if ( ( hero != nullptr && hero->GetColor() != color ) || ( castle != nullptr && castle->GetColor() != color ) ) {
                adventureMap.ResetFocus( GameFocus::FIRSTHERO, false );
            }
            else {
                adventureMap.updateFocus();
            }
        }
        else if ( hero != nullptr && hero->GetColor() == color ) {
            // While the other players play, the selection stays: a second click on it opens the hero or the castle.
            adventureMap.GetIconsPanel().select( hero );
        }
        else if ( castle != nullptr && castle->GetColor() == color ) {
            adventureMap.GetIconsPanel().select( castle );
        }
        adventureMap.setRedraw( Interface::REDRAW_ALL );
    }

    // Set when the local player left while waiting, until the game can go back to the main menu.
    // Not in NetworkGame since the network game ends at once.
    std::optional<fheroes2::GameMode> pendingLeave;

    fheroes2::GameMode onHostLeft()
    {
        NetworkGame & game = networkGame();

        VERBOSE_LOG( "[MULTI] The host has left the game." )

        game.stats.isEndedByHost = true;
        const bool showDialog = !game.testOptions.isEnabled;

        Multi::endGame();

        if ( showDialog ) {
            fheroes2::showStandardTextMessage( _( "Network game" ), _( "The host has left the game." ), Dialog::OK );
        }

        return fheroes2::GameMode::MAIN_MENU;
    }

    // Errors are shown to the player, except in self-tests.
    fheroes2::GameMode stopOnError( const std::string & reason )
    {
        // Not an error if the host left on purpose.
        if ( const NetworkGame & game = networkGame(); game.role == Role::GUEST && game.session && game.session->state() == Multi::NetworkSession::State::CLOSED
                                                       && ( game.session->receiveAnywhere( Multi::MessageType::GAME_END ) || game.session->isClosedByOther() ) ) {
            return onHostLeft();
        }

        ERROR_LOG( "[MULTI] Network game stopped: " << reason )

        const bool showDialog = !networkGame().testOptions.isEnabled;

        Multi::endGame();

        if ( showDialog ) {
            Multi::showNetworkError( _( "The network game has been stopped:" ), reason );
        }

        return fheroes2::GameMode::MAIN_MENU;
    }

    // The AI plays the colors the host plays for players who left, and in self-tests every human player. Not kept in
    // save files, so done again after each load.
    void applyAIControl()
    {
        const NetworkGame & game = networkGame();

        if ( game.testOptions.isEnabled ) {
            // Cache the human players before the AI takes them over, otherwise the game thinks they have all lost.
            Players::HumanColors();
        }

        const PlayerColorsSet seated = Multi::getSeatedColors();
        const PlayerColorsSet playedHere = Multi::getLocalHumanColors();

        for ( Player * player : Settings::Get().GetPlayers().getVector() ) {
            if ( ( player->GetControl() & CONTROL_HUMAN ) == 0 && !player->isAIAutoControlMode() ) {
                continue;
            }

            // In self-tests the players of the other computers too, otherwise their battles or level ups open dialogs here.
            const PlayerColor color = player->GetColor();
            const bool isAI = game.testOptions.isEnabled || ( game.role == Role::HOST && ( playedHere & color ) != 0 && ( seated & color ) == 0 );

            if ( isAI && !player->isAIAutoControlMode() ) {
                player->setAIAutoControlMode( true );
            }
            else if ( !isAI && player->isAIAutoControlMode() ) {
                player->setAIAutoControlMode( false );
                player->commitAIAutoControlMode();
            }
        }
    }

    // The first color of these players in the turn order which is still in the game, or the first one at all.
    PlayerColor getFirstColor( const PlayerColorsSet colors )
    {
        PlayerColor first = PlayerColor::NONE;

        for ( const PlayerColor color : PlayerColorsVector( colors ) ) {
            if ( world.GetKingdom( color ).isPlay() ) {
                return color;
            }
            if ( first == PlayerColor::NONE ) {
                first = color;
            }
        }

        return first;
    }

    // Who gets a whole game, and how.
    struct GameReceiver
    {
        Multi::MachineId machine{ Multi::hostMachine };
        Multi::MessageType type{ Multi::MessageType::TURN_STATE };
        PlayerColor color{ PlayerColor::NONE };
    };

    // Sends the game saved once to each receiver. Returns an error message, or an empty string on success.
    std::string sendGame( const std::vector<GameReceiver> & receivers, const Multi::TurnStage stage, const bool isSenderOut = false )
    {
        const NetworkGame & game = networkGame();

        // The computers which do not get the game must know the same world as the ones which do.
        Multi::sendLiveUpdate();

        Multi::TurnState state;
        state.stage = stage;
        state.isSenderOut = isSenderOut;

        if ( std::string error = Multi::saveGameData( state.saveData, state.fingerprint ); !error.empty() ) {
            return error;
        }

        Multi::StateFingerprint fingerprint;
        fingerprint.hash = state.fingerprint;

        for ( const GameReceiver & receiver : receivers ) {
            state.color = static_cast<uint8_t>( receiver.color );
            state.receiverColors = static_cast<uint8_t>( Multi::getColorsPlayedBy( receiver.machine ) );

            const char * what = ( receiver.type == Multi::MessageType::GAME_SNAPSHOT ) ? "Snapshot for "
                                : ( receiver.type == Multi::MessageType::DAY_START )   ? "Day of "
                                                                                       : "Turn of ";
            VERBOSE_LOG( "[MULTI] " << what << Color::String( receiver.color ) << " sent to computer " << static_cast<int>( receiver.machine ) << " ("
                                    << stageName( stage ) << "), fingerprint " << fingerprint.hashString() << ", " << state.saveData.size() << " bytes" )

            game.session->send( receiver.machine, Multi::makeTurnState( state, receiver.type ) );
        }

        Multi::resetLiveBaseline();

        return {};
    }

    // Host only. Every guest still in the game gets the game to look at, except the ones given.
    std::vector<GameReceiver> getSnapshotReceivers( const Multi::MachineSet excluded )
    {
        std::vector<GameReceiver> receivers;

        for ( const Multi::MachineInfo & info : Multi::getMachineTable().machines ) {
            if ( info.id == Multi::hostMachine || info.state == Multi::MachineState::GONE || ( excluded & Multi::machineBit( info.id ) ) != 0 ) {
                continue;
            }

            const PlayerColorsSet colors = info.playedColors != 0 ? info.playedColors : info.ownedColors;
            receivers.push_back( { info.id, Multi::MessageType::GAME_SNAPSHOT, getFirstColor( colors ) } );
        }

        return receivers;
    }

    // Host only. Each guest gets the table with its own number.
    void sendMachineTables()
    {
        const NetworkGame & game = networkGame();
        if ( game.role != Role::HOST || !game.session ) {
            return;
        }

        Multi::MachineTable table = Multi::getMachineTable();

        for ( const Multi::MachineInfo & info : Multi::getMachineTable().machines ) {
            if ( info.id != Multi::hostMachine && info.state != Multi::MachineState::GONE ) {
                table.receiver = info.id;
                game.session->send( info.id, Multi::makeMachineTable( table ) );
            }
        }

        VERBOSE_LOG( "[MULTI] Computers of the game: " << Multi::describeMachines() )
    }

    // A few words for the players here when another computer comes or goes. Nothing on a guest which has no game yet.
    void showMachineChange( const Multi::MachineInfo & before, const Multi::MachineInfo & after )
    {
        const bool hasGame = !Multi::isGuest() || networkGame().playingColor != PlayerColor::NONE;
        if ( before.state == after.state || Multi::isTestMode() || after.id == Multi::getThisMachine() || !hasGame ) {
            return;
        }

        const char * text = nullptr;
        switch ( after.state ) {
        case Multi::MachineState::PRESENT:
            text = gettext_noop( "%{name} is back." );
            break;
        case Multi::MachineState::LOST:
            text = gettext_noop( "The connection with %{name} is cut." );
            break;
        case Multi::MachineState::LEFT:
            text = gettext_noop( "%{name} has left the game." );
            break;
        case Multi::MachineState::GONE:
            if ( Multi::hasPlayerInGame( after.ownedColors ) ) {
                text = gettext_noop( "The computer plays for %{name} from now on." );
            }
            break;
        default:
            break;
        }

        if ( text == nullptr ) {
            return;
        }

        std::string note = _( text );
        StringReplace( note, "%{name}", Multi::getMachineName( after.id ) );
        Multi::showMapNote( note );
    }

    // Guest only.
    void onMachineTable( const Multi::Message & message )
    {
        std::optional<Multi::MachineTable> table = Multi::readMachineTable( message );
        if ( !table || table->receiver == Multi::hostMachine ) {
            ERROR_LOG( "[MULTI] Invalid table of computers received" )
            return;
        }

        const Multi::MachineTable before = Multi::setMachineTable( *table );

        for ( const Multi::MachineInfo & info : table->machines ) {
            const auto previous
                = std::find_if( before.machines.begin(), before.machines.end(), [&info]( const Multi::MachineInfo & other ) { return other.id == info.id; } );
            if ( previous != before.machines.end() ) {
                showMachineChange( *previous, info );
            }
        }

        VERBOSE_LOG( "[MULTI] Computers of the game: " << Multi::describeMachines() )
    }

    // Shows the last kingdom played here, or another one still in the game.
    void refreshLocalColor()
    {
        NetworkGame & game = networkGame();
        const PlayerColorsSet colors = Multi::getSeatedColors() != 0 ? Multi::getSeatedColors() : Multi::getLocalHumanColors();

        if ( ( colors & game.localColor ) == 0 || !Multi::hasPlayerInGame( static_cast<PlayerColorsSet>( game.localColor ) ) ) {
            game.localColor = getFirstColor( colors );
        }
    }

    // Loads a turn to play (TURN_STATE), a game to look at (GAME_SNAPSHOT) or the start of a simultaneous day (DAY_START).
    fheroes2::GameMode loadReceivedGame( const Multi::Message & message )
    {
        NetworkGame & game = networkGame();

        const bool isSnapshot = ( message.type == Multi::MessageType::GAME_SNAPSHOT );
        const bool isDayStart = ( message.type == Multi::MessageType::DAY_START );

        const std::optional<Multi::TurnState> state = Multi::readTurnState( message );
        if ( !state ) {
            return stopOnError( gettext_noop( "invalid turn data received" ) );
        }

        if ( game.role == Role::HOST ) {
            const PlayerColor color = static_cast<PlayerColor>( state->color );
            const bool isExpected = !isSnapshot && !isDayStart && state->stage == Multi::TurnStage::ENDED && color == game.otherTurnColor
                                    && Multi::getMachineOfColor( color ) == message.sender;
            if ( !isExpected ) {
                ERROR_LOG( "[MULTI] Unexpected game from computer " << static_cast<int>( message.sender ) << " ignored" )
                return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
            }
        }
        else {
            if ( !isSnapshot && state->stage == Multi::TurnStage::ENDED ) {
                return stopOnError( std::string( gettext_noop( "unexpected turn stage" ) ) + ": " + stageName( state->stage ) );
            }
            if ( ( state->receiverColors & state->color ) == 0 && !isSnapshot ) {
                return stopOnError( gettext_noop( "the turn sent by the host is inconsistent" ) );
            }
        }

        if ( const std::string error = Multi::loadGameData( state->saveData ); !error.empty() ) {
            return stopOnError( error );
        }

        const Settings & conf = Settings::Get();
        const PlayerColor color = static_cast<PlayerColor>( state->color );
        const Player * player = Players::Get( color );

        // In self-tests the AI already plays the human players.
        const bool isHuman = player != nullptr && ( player->isControlHuman() || player->isAIAutoControlMode() );
        if ( !isHuman || ( !isSnapshot && conf.CurrentColor() != color ) ) {
            return stopOnError( gettext_noop( "the turn sent by the host is inconsistent" ) );
        }

        const Multi::StateFingerprint fingerprint = Multi::computeStateFingerprint();
        if ( fingerprint.hash != state->fingerprint ) {
            ++game.stats.fingerprintMismatches;

            Multi::StateFingerprint expected;
            expected.hash = state->fingerprint;

            ERROR_LOG( "[MULTI] Fingerprint mismatch for the turn of " << Color::String( color ) << ": " << fingerprint.hashString() << " here, " << expected.hashString()
                                                                       << " for the sender" )
        }

        Multi::resetLiveBaseline();

        if ( game.role == Role::HOST ) {
            game.otherTurnColor = PlayerColor::NONE;
        }
        else if ( !isSnapshot ) {
            // A snapshot keeps the kingdom shown here.
            game.localColor = color;
        }

        if ( isSnapshot ) {
            VERBOSE_LOG( "[MULTI] Snapshot for " << Color::String( color ) << " received, fingerprint " << fingerprint.hashString() )

            ++game.stats.snapshotsReceived;
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        }

        VERBOSE_LOG( "[MULTI] Turn of " << Color::String( color ) << " received (" << stageName( state->stage ) << "), fingerprint " << fingerprint.hashString() )

        ++game.stats.turnsReceived;

        // The host has no player left and hands the game over for good.
        if ( state->isSenderOut && game.role == Role::GUEST ) {
            Multi::goOnAlone();
            Multi::endGame();
        }

        // A guest without players left gives its last turn back and leaves: its colors are out of the game.
        if ( state->isSenderOut && game.role == Role::HOST ) {
            VERBOSE_LOG( "[MULTI] Computer " << static_cast<int>( message.sender ) << " has no player left" )
            Multi::setMachineState( message.sender, Multi::MachineState::GONE );
            sendMachineTables();
        }

        // This must be set after endGame() because it resets the stage.
        game.resumeStage = state->stage;

        if ( isDayStart ) {
            Multi::beginSimultaneousDay();
        }
        else {
            Multi::endSimultaneousDay();
        }

        return fheroes2::GameMode::START_GAME;
    }

    // Returns true for the messages a guest without a game can accept.
    bool isGameMessageForNewcomer( const Multi::Message & message )
    {
        switch ( message.type ) {
        case Multi::MessageType::TURN_STATE:
        case Multi::MessageType::GAME_SNAPSHOT:
        case Multi::MessageType::DAY_START:
        case Multi::MessageType::GAME_END:
        case Multi::MessageType::SIMULTANEOUS_TURNS:
        case Multi::MessageType::MACHINES:
            return true;
        case Multi::MessageType::BATTLE_START: {
            // Outside of a simultaneous day, a battle comes with the game saved right before it.
            const std::optional<Multi::BattleStart> start = Multi::readBattleStart( message );
            return start && !start->saveData.empty();
        }
        default:
            return false;
        }
    }

    // Returns true for the messages after which the local player has something to do or to read.
    bool isForPlayerHere( const Multi::MessageType type )
    {
        switch ( type ) {
        case Multi::MessageType::TURN_STATE:
        case Multi::MessageType::BATTLE_START:
        case Multi::MessageType::HERO_LEVEL_UP:
        case Multi::MessageType::HERO_NOTICE:
        case Multi::MessageType::GAME_RESULT:
        case Multi::MessageType::GAME_END:
            return true;
        default:
            return false;
        }
    }

    fheroes2::GameMode onMessage( const Multi::Message & message )
    {
        const NetworkGame & game = networkGame();

        if ( isForPlayerHere( message.type ) && ( message.type != Multi::MessageType::GAME_END || game.role == Role::GUEST ) ) {
            fheroes2::engine().requestAttention();
        }

        switch ( message.type ) {
        case Multi::MessageType::TURN_STATE:
        case Multi::MessageType::GAME_SNAPSHOT:
        case Multi::MessageType::DAY_START:
            return loadReceivedGame( message );
        case Multi::MessageType::DAY_DONE:
        case Multi::MessageType::DAY_RESUME:
        case Multi::MessageType::DAY_RESUMED:
        case Multi::MessageType::DAY_LOCKED:
        case Multi::MessageType::DAY_CLOSED:
            Multi::onDayReadinessMessage( message );
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::SPECTATE_START:
        case Multi::MessageType::SPECTATE_ACTIONS:
        case Multi::MessageType::SPECTATE_END:
            Multi::onSpectatedMessage( message );
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::DAY_END:
            if ( const std::optional<Multi::DayEnd> dayEnd = Multi::readDayEnd( message ); dayEnd ) {
                Multi::checkDayEnd( *dayEnd );
            }
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::BATTLE_START:
            return Multi::followNetworkBattle( message );
        case Multi::MessageType::BATTLE_END:
            Multi::checkNetworkBattleEnd( message );
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::HERO_LEVEL_UP:
            Multi::answerHeroLevelUp( message );
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::HERO_NOTICE:
            Multi::showHeroNotice( message );
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::GAME_RESULT:
            return Multi::showGameResult( message );
        case Multi::MessageType::WORLD_UPDATE:
            Multi::applyLiveUpdate( message );
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::RESERVE:
            game.session->putBack( message );
            Multi::answerReservations();
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::BATTLE_REQUEST:
            game.session->putBack( message );
            Multi::answerBattleRequests();
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::SIMULTANEOUS_TURNS:
            Multi::onSimultaneousTurnsMessage( message );
            Multi::showSimultaneousTurnsNotice();
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::MACHINES:
            if ( game.role == Role::GUEST ) {
                onMachineTable( message );
            }
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::BATTLE_ACTIONS:
        case Multi::MessageType::HERO_LEVEL_UP_CHOICE:
        case Multi::MessageType::RESERVE_REPLY:
        case Multi::MessageType::BATTLE_REQUEST_REPLY:
        case Multi::MessageType::LIVE_BATTLE_DONE:
            // Late answers to a battle, choice or request given up here, or the end of an attack which came too late.
            VERBOSE_LOG( "[MULTI] Late message " << static_cast<uint32_t>( message.type ) << " ignored" )
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        case Multi::MessageType::GAME_END:
            // A guest which leaves is followed by updateGuests().
            if ( game.role == Role::HOST ) {
                VERBOSE_LOG( "[MULTI] Computer " << static_cast<int>( message.sender ) << " leaves the game" )
                return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
            }
            return onHostLeft();
        default:
            break;
        }

        if ( game.session && Multi::handleLateWaitingRoomMessage( *game.session, message ) ) {
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        }

        return stopOnError( std::string( gettext_noop( "unexpected message" ) ) + ": " + std::to_string( static_cast<uint32_t>( message.type ) ) );
    }

    // Internet box of the host, shown while it waits for the other players.
    struct BoxStatus
    {
        Multi::PortMapping::State state{ Multi::PortMapping::State::WORKING };
        bool isBehindAnotherNetwork{ false };
        std::string externalAddress;
        std::string error;
    };

    std::string makeAddressJoinText( const std::optional<BoxStatus> & box, const std::vector<std::string> & addresses, const std::string & addressV6,
                                     const uint16_t port )
    {
        if ( !box ) {
            return _( "The other games of this computer join this one." );
        }

        std::string text;

        switch ( box->state ) {
        case Multi::PortMapping::State::OPEN:
            if ( box->isBehindAnotherNetwork ) {
                text += _( "This connection cannot be reached from the internet. Use Tailscale." );
            }
            else if ( box->externalAddress.empty() ) {
                text += _( "The internet box is open." );
            }
            else {
                text += std::string( _( "Internet address to give to the other players:" ) ) + '\n' + box->externalAddress;
            }
            break;
        case Multi::PortMapping::State::FAILED: {
            std::string advice
                = _( "Unable to open the internet box (%{reason}). Enable UPnP in the box, forward port %{port} (TCP) to this computer, or use Tailscale." );
            StringReplace( advice, "%{reason}", Multi::translateError( box->error ) );
            StringReplace( advice, "%{port}", port );
            text += advice;
            break;
        }
        default:
            text += _( "Opening the internet box..." );
            break;
        }

        // Without a box to open, but many boxes turn IPv6 connections away.
        if ( !addressV6.empty() ) {
            text += std::string( "\n\n" ) + _( "IPv6 address, if the box lets it through:" ) + '\n' + addressV6;
        }

        // The first ones are enough, the Tailscale address comes first.
        if ( !addresses.empty() ) {
            text += std::string( "\n\n" ) + _( "Same network or Tailscale:" );
            for ( size_t i = 0; i < addresses.size() && i < 3; ++i ) {
                text += '\n' + addresses[i];
            }
        }

        return text;
    }

    std::unique_ptr<Multi::NetworkSession> openHostSession( const uint16_t port, const bool localOnly, const Multi::LobbyKey & probeKey, std::string & error )
    {
        auto session = std::make_unique<Multi::NetworkSession>();
        if ( !session->host( port, localOnly ) ) {
            error = session->error();
            return {};
        }

        session->setProbeKey( probeKey );
        VERBOSE_LOG( "[MULTI] Hosting on port " << port << ( localOnly ? ", local connections only" : "" ) )
        return session;
    }

    // The name shown in the list of the lobby and in the waiting room: the one chosen by the host, or the name of the map.
    std::string makeGameName( const std::string & chosenName )
    {
        std::string name = Multi::cutUtf8( chosenName.empty() ? Multi::makeRoomGameOfCurrentGame( 0 ).mapName : chosenName, 40 );
        if ( !Multi::isValidGameName( name ) ) {
            name = Multi::cutUtf8( Multi::toUtf8( _( "Network game" ) ), 40 );
        }
        return name;
    }

    // The game the host publishes on a lobby.
    Multi::RoomPublish makeLobbyRoom( const Multi::LobbyHosting & lobby, const uint16_t port, const Multi::LobbyKey & probeKey, const int freeColors,
                                      const Multi::PortMapping * portMapping )
    {
        Multi::RoomPublish room;
        room.visibility = lobby.isPublic ? Multi::RoomVisibility::PUBLIC : Multi::RoomVisibility::INVITE_ONLY;
        room.isRelayOnly = lobby.isRelayOnly;
        room.game = Multi::makeRoomGameOfCurrentGame( static_cast<uint8_t>( freeColors ) );
        room.gamePort = port;
        room.probeKey = probeKey;

        room.gameName = makeGameName( lobby.gameName );

        if ( lobby.isRelayOnly ) {
            return room;
        }

        if ( const std::string addressV6 = Multi::getInternetAddressV6(); !addressV6.empty() ) {
            room.publicAddresses.push_back( addressV6 );
        }
        if ( portMapping != nullptr && portMapping->state() == Multi::PortMapping::State::OPEN && !portMapping->isBehindAnotherNetwork()
             && !portMapping->externalAddress().empty() ) {
            room.publicAddresses.push_back( portMapping->externalAddress() );
        }

        room.localAddresses = Multi::getLocalAddresses();
        room.localAddresses.resize( std::min<size_t>( room.localAddresses.size(), 8 ) );

        return room;
    }

    Multi::JoinHelp makeLobbyJoinHelp( const Multi::PublishedRoom * room, const Multi::LobbyHosting & lobby )
    {
        if ( room == nullptr ) {
            return { {}, _( "Opening the internet box..." ) };
        }

        if ( room->state() != Multi::PublishedRoom::State::PUBLISHED ) {
            if ( room->error().empty() ) {
                return { {}, _( "Connecting to the lobby..." ) };
            }

            std::string text = _( "The lobby cannot be reached: %{reason}. The game tries again." );
            StringReplace( text, "%{reason}", Multi::translateError( room->error() ) );
            return { {}, text };
        }

        const std::string & lobbyName = room->lobby().lobbyName;
        std::string text = lobby.isPublic ? _( "Your game is in the list of the lobby %{lobby}." ) : _( "Your game is on the lobby %{lobby}, for invited players only." );
        StringReplace( text, "%{lobby}", lobbyName.empty() ? room->lobbyAddress() : Multi::fromUtf8( lobbyName ) );
        text += "\n\n";

        const Multi::RoomReach & reach = room->status().reach;
        if ( !room->isConnected() ) {
            text += _( "The connection to the lobby was lost. The game connects again." );
        }
        else if ( !reach.isProbeDone ) {
            text += _( "The lobby checks whether other players can reach you directly..." );
        }
        else if ( reach.isIPv4 || reach.isIPv6 ) {
            text += _( "Other players join you directly." );
        }
        else if ( reach.isRelayAvailable ) {
            text += _( "Other players join you through the relay of the lobby." );
        }
        else {
            text += _( "Other players cannot reach you: your computer cannot be reached directly, and this lobby has no relay." );
        }

        return { room->status().inviteCode, text };
    }

    // The human players changed before the start: the game must know them anew, the events of the map too. A loaded game
    // is saved and loaded again, a new one starts again from its map. Returns an error message, or an empty string on success.
    std::string reloadWithPlayers()
    {
        Settings & conf = Settings::Get();

        if ( conf.LoadedGameVersion() ) {
            std::vector<uint8_t> data;
            uint64_t fingerprint = 0;
            if ( std::string error = Multi::saveGameData( data, fingerprint ); !error.empty() ) {
                return error;
            }
            return Multi::loadGameData( data );
        }

        conf.GetPlayers().SetStartGame();

        const Maps::FileInfo & mapInfo = conf.getCurrentMapInfo();
        const bool isLoaded = ( mapInfo.version == GameVersion::RESURRECTION ) ? world.loadResurrectionMap( mapInfo.filename )
                                                                               : world.LoadMapMP2( mapInfo.filename, mapInfo.version == GameVersion::SUCCESSION_WARS );

        return isLoaded ? std::string() : std::string( gettext_noop( "the map is corrupted or doesn't exist" ) );
    }

    // Host only. A computer which connected during the game: one of the guests coming back after a restart, recognised by
    // its key or its nickname.
    void takeComebacks()
    {
        NetworkGame & game = networkGame();

        while ( const std::optional<Multi::NetworkSession::Newcomer> newcomer = game.session->takeNewcomer() ) {
            std::string refusal;
            const std::optional<Multi::HandshakeInfo> handshake = Multi::readNewcomerHandshake( *game.session, newcomer->machine, newcomer->firstMessage, refusal );
            if ( !handshake ) {
                ERROR_LOG( "[MULTI] A computer could not come back: " << refusal )
                continue;
            }

            const auto isPlace = [&game, &handshake]( const Multi::MachineInfo & info, const bool byKey ) {
                if ( info.id == Multi::hostMachine || info.state == Multi::MachineState::PRESENT || !Multi::hasPlayerInGame( info.ownedColors ) ) {
                    return false;
                }
                if ( byKey ) {
                    const auto key = game.machineKeys.find( info.id );
                    return handshake->machineKey != 0 && key != game.machineKeys.end() && key->second == handshake->machineKey;
                }
                return !handshake->nickname.empty() && info.nickname == handshake->nickname;
            };

            const std::vector<Multi::MachineInfo> & machines = Multi::getMachineTable().machines;
            auto place = std::find_if( machines.begin(), machines.end(), [&isPlace]( const Multi::MachineInfo & info ) { return isPlace( info, true ); } );
            if ( place == machines.end() ) {
                place = std::find_if( machines.begin(), machines.end(), [&isPlace]( const Multi::MachineInfo & info ) { return isPlace( info, false ); } );
            }

            if ( place == machines.end() ) {
                VERBOSE_LOG( "[MULTI] A computer which is not in this game was refused" )
                Multi::refuseNewcomer( *game.session, newcomer->machine, Multi::HandshakeResult::GAME_STARTED );
                continue;
            }

            const Multi::MachineId machine = place->id;
            Multi::acceptNewcomer( *game.session, newcomer->machine, machine );
            game.machineKeys[machine] = handshake->machineKey;

            Multi::setMachineState( machine, Multi::MachineState::PRESENT );
            Multi::addMachineComeback( machine );
            game.resyncPending |= Multi::machineBit( machine );

            VERBOSE_LOG( "[MULTI] Computer " << static_cast<int>( machine ) << " came back after a restart" )

            if ( !game.testOptions.isEnabled ) {
                std::string note = _( "%{name} is back." );
                StringReplace( note, "%{name}", Multi::getMachineName( machine ) );
                Multi::showMapNote( note );
            }

            sendMachineTables();
        }
    }

    // Host only. Returns true if the state of a guest changed.
    bool followGuestConnections()
    {
        const NetworkGame & game = networkGame();
        bool isChanged = false;

        for ( const Multi::MachineInfo & info : std::vector<Multi::MachineInfo>( Multi::getMachineTable().machines ) ) {
            if ( info.id == Multi::hostMachine || info.state == Multi::MachineState::GONE ) {
                continue;
            }

            Multi::MachineState state = info.state;

            switch ( game.session->state( info.id ) ) {
            case Multi::NetworkSession::State::CONNECTED:
                state = Multi::MachineState::PRESENT;
                break;
            case Multi::NetworkSession::State::RECONNECTING:
                if ( game.session->lostFor( info.id ) >= absenceDelay ) {
                    state = Multi::MachineState::LOST;
                }
                break;
            default:
                state = game.session->isClosedByOther( info.id ) ? Multi::MachineState::LEFT : Multi::MachineState::LOST;
                break;
            }

            // Without players there is nothing to wait for.
            if ( state != Multi::MachineState::PRESENT && !Multi::hasPlayerInGame( info.ownedColors ) ) {
                state = Multi::MachineState::GONE;
                game.session->closeLink( info.id );
            }

            if ( Multi::setMachineState( info.id, state ) ) {
                VERBOSE_LOG( "[MULTI] Computer " << static_cast<int>( info.id ) << " is now " << Multi::describeMachines() )

                Multi::MachineInfo after = info;
                after.state = state;
                showMachineChange( info, after );
                isChanged = true;
            }
        }

        return isChanged;
    }
}

void Multi::startHostGame( std::unique_ptr<NetworkSession> session, const std::vector<RoomGuest> & guests, const NetworkGameTestOptions testOptions )
{
    NetworkGame & game = networkGame();
    assert( game.role == Role::NONE );

    game = {};
    game.role = Role::HOST;

    installCrashReport();
    setNetworkGameRunning( true );

    // The guests get their answers from any screen of the host, and the lobby hears from it.
    LocalEvent::Get().setEventProcessingHook( []() {
        updateGuests();
        answerReservations();
        updateLobby();
    } );

    resetSimultaneousStats();

    game.session = std::move( session );
    game.testOptions = testOptions;

    // The host plays the human colors no guest took.
    MachineTable table;
    table.receiver = hostMachine;

    PlayerColorsSet guestColors = 0;
    for ( const RoomGuest & guest : guests ) {
        guestColors |= guest.colors;
    }

    MachineInfo host;
    host.nickname = loadNetworkSettings().nickname;
    host.ownedColors = static_cast<PlayerColorsSet>( Players::HumanColors() & ~guestColors );
    host.playedColors = host.ownedColors;
    table.machines.push_back( host );

    for ( const RoomGuest & guest : guests ) {
        MachineInfo info;
        info.id = guest.machine;
        info.nickname = guest.nickname;
        info.ownedColors = guest.colors;
        info.playedColors = guest.colors;
        table.machines.push_back( info );

        game.machineKeys[guest.machine] = guest.machineKey;
    }

    std::sort( table.machines.begin(), table.machines.end(), []( const MachineInfo & first, const MachineInfo & second ) { return first.id < second.id; } );
    setMachineTable( std::move( table ) );

    refreshLocalColor();

    // For a loaded game the turn loop has to find the current player.
    if ( Settings::Get().LoadedGameVersion() ) {
        game.resumeStage = TurnStage::IN_PROGRESS;
        game.isLoadedAtStart = true;
    }

    applyAIControl();

    sendSimultaneousTurns();
    sendMachineTables();
}

void Multi::startGuestGame( std::unique_ptr<NetworkSession> session, const NetworkGameTestOptions testOptions )
{
    NetworkGame & game = networkGame();
    assert( game.role == Role::NONE );

    game = {};
    game.role = Role::GUEST;

    installCrashReport();
    setNetworkGameRunning( true );

    game.session = std::move( session );
    game.testOptions = testOptions;

    // The changes of the other computers are known on any screen.
    LocalEvent::Get().setEventProcessingHook( []() {
        const NetworkGame & current = networkGame();
        if ( current.session ) {
            while ( const std::optional<Message> message = current.session->receive( MessageType::MACHINES ) ) {
                onMachineTable( *message );
            }
        }
    } );
}

void Multi::endGame()
{
    NetworkGame & game = networkGame();

    if ( game.role == Role::HOST && getSimultaneousStats().daysMeasured > 0 ) {
        VERBOSE_LOG( "[MULTI] Contact summary: " << describeSimultaneousStats() )
    }

    if ( game.role != Role::NONE && ( getLiveStats().updatesSent > 0 || getLiveStats().updatesApplied > 0 ) ) {
        VERBOSE_LOG( "[MULTI] Live view summary: " << describeLiveStats() )
    }

    if ( game.role != Role::NONE && isActionWatchEnabled() ) {
        VERBOSE_LOG( "[MULTI] Action watch summary: " << describeActionWatch() )
    }

    // Even during a reconnection. The other computers get a few seconds for it, unless gone for a while: a short cut, or a
    // tablet catching up with a battle, makes the answer late.
    if ( game.session && game.session->isAlive() ) {
        const std::vector<MachineId> machines = ( game.role == Role::HOST ) ? game.session->guests() : std::vector<MachineId>{ hostMachine };
        MachineSet receivers = 0;

        for ( const MachineId machine : machines ) {
            if ( game.session->isAlive( machine ) && game.session->lostFor( machine ) < std::chrono::seconds( 10 ) ) {
                receivers |= machineBit( machine );
            }
        }

        if ( receivers != 0 ) {
            game.session->sendToMachines( receivers, { MessageType::GAME_END, {} } );

            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
            while ( game.session->undeliveredBytes() > 0 && game.session->isAlive() && std::chrono::steady_clock::now() < deadline ) {
                game.session->update();
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            }
        }
    }

    if ( game.role != Role::NONE && ( getReservationStats().stepsReserved > 0 || getReservationStats().requestsGranted > 0 ) ) {
        VERBOSE_LOG( "[MULTI] Reservation summary: " << describeReservations() )
    }

    if ( game.role != Role::NONE && getSimultaneousDayStats().days > 0 ) {
        VERBOSE_LOG( "[MULTI] Simultaneous turns summary: " << describeSimultaneousDays() )
    }

    if ( game.role != Role::NONE && getSpectatorStats().battlesOffered + getSpectatorStats().castlesVisited > 0 ) {
        VERBOSE_LOG( "[MULTI] Spectator summary: " << describeSpectator() )
    }

    if ( game.role != Role::NONE && ( getLiveBattleStats().requestsSent > 0 || getLiveBattleStats().requestsAnswered > 0 ) ) {
        VERBOSE_LOG( "[MULTI] Live battle summary: " << describeLiveBattles() )
    }

    endSimultaneousDay();

    LocalEvent::Get().setEventProcessingHook( {} );
    setNetworkGameRunning( false );

    game.role = Role::NONE;
    game.session.reset();
    game.localColor = PlayerColor::NONE;
    game.playingColor = PlayerColor::NONE;
    game.isObserving = false;
    game.resumeStage.reset();
    game.otherTurnColor = PlayerColor::NONE;
    game.isLoadedAtStart = false;
    game.machineKeys.clear();
    game.resyncPending = 0;
    game.nextSession.reset();
    game.portMapping.reset();
    game.publishedRoom.reset();

    clearMachineTable();
}

void Multi::updateLobby()
{
    const NetworkGame & game = networkGame();
    if ( game.publishedRoom && game.session ) {
        game.publishedRoom->update();
        listenOnRelayOfRoom( *game.publishedRoom, *game.session );
    }
}

void Multi::keepPublishedRoom( std::unique_ptr<PublishedRoom> room )
{
    NetworkGame & game = networkGame();
    assert( game.role == Role::HOST );

    // Not in the list any more.
    room->setState( RoomState::IN_GAME, makeRoomGameOfCurrentGame( 0 ) );
    game.publishedRoom = std::move( room );
}

bool Multi::isNetworkGame()
{
    return networkGame().role != Role::NONE;
}

bool Multi::isGuest()
{
    return networkGame().role == Role::GUEST;
}

bool Multi::isRemoteColor( const PlayerColor color )
{
    const std::optional<MachineId> machine = getMachineOfColor( color );
    return networkGame().role == Role::HOST && machine && *machine != hostMachine;
}

bool Multi::isObservingOtherPlayers()
{
    return networkGame().isObserving;
}

PlayerColor Multi::getPlayingColorShown( const PlayerColor currentColor )
{
    if ( !isObservingOtherPlayers() ) {
        return currentColor;
    }

    const NetworkGame & game = networkGame();

    // In turns one after the other, the player whose turn it is.
    const PlayerColor playing = ( game.role == Role::HOST ) ? game.otherTurnColor : game.playingColor;
    if ( !isSimultaneousDay() && playing != PlayerColor::NONE && ( getLocalHumanColors() & playing ) == 0 && world.GetKingdom( playing ).isPlay() ) {
        return playing;
    }

    for ( const PlayerColor color : PlayerColorsVector( getRemoteHumanColors() ) ) {
        if ( world.GetKingdom( color ).isPlay() ) {
            return color;
        }
    }

    return currentColor;
}

bool Multi::announceTurnOnMap( const PlayerColor color )
{
    if ( !isNetworkGame() ) {
        return false;
    }

    // Players at the same computer may be different people, the map is hidden between their turns.
    int playersHere = 0;
    for ( const PlayerColor seatedColor : PlayerColorsVector( getSeatedColors() ) ) {
        if ( world.GetKingdom( seatedColor ).isPlay() ) {
            ++playersHere;
        }
    }

    if ( playersHere > 1 ) {
        return false;
    }

    std::string text = _( "%{color} player's turn." );
    const Player * player = Players::Get( color );
    StringReplace( text, "%{color}", player ? player->GetName() : Color::String( color ) );
    showMapNote( text );

    return true;
}

Multi::NetworkSession * Multi::getSession()
{
    return networkGame().session.get();
}

bool Multi::isTestMode()
{
    return networkGame().testOptions.isEnabled;
}

PlayerColorsSet Multi::getLocalHumanColors()
{
    return isNetworkGame() ? getColorsPlayedBy( getThisMachine() ) : PlayerColorsSet{ 0 };
}

PlayerColorsSet Multi::getRemoteHumanColors()
{
    return static_cast<PlayerColorsSet>( getAllHumanColors() & ~getLocalHumanColors() );
}

std::string Multi::saveGameData( std::vector<uint8_t> & data, uint64_t & fingerprint )
{
    const std::string filePath = getTurnFilePath( "multi_sent" );
    if ( !Game::Save( filePath, true ) ) {
        return gettext_noop( "unable to save the game" );
    }

    fingerprint = computeStateFingerprint().hash;

    StreamFile file;
    if ( !file.open( filePath, "rb" ) ) {
        return std::string( gettext_noop( "unable to read the file" ) ) + ": " + filePath;
    }

    data = file.getRaw( 0 );
    file.close();
    removeGuestTurnFile( filePath );

    if ( data.empty() ) {
        return std::string( gettext_noop( "unable to read the file" ) ) + ": " + filePath;
    }

    return {};
}

std::string Multi::loadGameData( const std::vector<uint8_t> & data )
{
    // Game::Load() trusts the sizes it reads. Every computer runs the same game, the format must be the current one.
    {
        ROStreamBuf stream( data );
        stream.setBigendian( true );

        uint16_t magicNumber = 0;
        uint32_t versionSize = 0;
        stream >> magicNumber >> versionSize;

        const bool isValidStart = !stream.fail() && magicNumber == 0xFF03 && versionSize <= 32 && versionSize < stream.size();
        uint16_t version = 0;

        if ( isValidStart ) {
            stream.skip( versionSize );
            stream >> version;
        }

        if ( !isValidStart || stream.fail() || version != CURRENT_FORMAT_VERSION ) {
            return gettext_noop( "invalid game received" );
        }
    }

    const std::string filePath = getTurnFilePath( "multi_received" );

    {
        StreamFile file;
        if ( !file.open( filePath, "wb" ) ) {
            return std::string( gettext_noop( "unable to write the file" ) ) + ": " + filePath;
        }

        file.putRaw( data.data(), data.size() );
    }

    // Game::Load() only accepts a save file of the current game type.
    Settings::Get().SetGameType( Game::TYPE_HOTSEAT );

    // A broken file can still ask for absurd amounts of memory.
    bool isLoaded = false;
    try {
        isLoaded = ( Game::Load( filePath ) != fheroes2::GameMode::CANCEL );
    }
    catch ( const std::exception & ex ) {
        ERROR_LOG( "[MULTI] Unable to load the game sent by another computer: " << ex.what() )
    }

    removeGuestTurnFile( filePath );

    if ( !isLoaded ) {
        return gettext_noop( "unable to load the game sent by another computer" );
    }

    resetLostFocus();
    onWorldChangedForSpectator( true );

    // Not kept in the save file.
    applyAIControl();

    if ( isGuest() ) {
        networkGame().playingColor = Settings::Get().CurrentColor();
        refreshLocalColor();
    }

    return {};
}

const Multi::NetworkGameStats & Multi::getStats()
{
    return networkGame().stats;
}

bool Multi::isResumingTurn()
{
    return networkGame().resumeStage.has_value();
}

Multi::TurnStage Multi::finishResumingTurn()
{
    NetworkGame & game = networkGame();
    assert( game.resumeStage.has_value() );

    const TurnStage stage = game.resumeStage.value_or( TurnStage::IN_PROGRESS );
    game.resumeStage.reset();

    return stage;
}

fheroes2::GameMode Multi::handOverTurn( const PlayerColor color, const TurnStage stage )
{
    NetworkGame & game = networkGame();
    assert( game.role != Role::NONE && game.session );

    Settings::Get().SetCurrentColor( color );

    std::vector<GameReceiver> receivers;
    bool isOut = !hasPlayerInGame( getLocalHumanColors() );

    if ( game.role == Role::HOST ) {
        const MachineId machine = getMachineOfColor( color ).value_or( hostMachine );
        game.otherTurnColor = color;

        // No player left here and nobody else to host: that computer gets the game for good.
        const std::vector<MachineId> guestsWithPlayers = getGuestsWithPlayers();
        isOut = isOut && guestsWithPlayers.size() == 1 && guestsWithPlayers.front() == machine;

        receivers.push_back( { machine, MessageType::TURN_STATE, color } );

        if ( !isOut ) {
            const std::vector<GameReceiver> snapshots = getSnapshotReceivers( machineBit( machine ) );
            receivers.insert( receivers.end(), snapshots.begin(), snapshots.end() );
        }
    }
    else {
        receivers.push_back( { hostMachine, MessageType::TURN_STATE, color } );
    }

    if ( const std::string error = sendGame( receivers, stage, isOut ); !error.empty() ) {
        return stopOnError( error );
    }

    ++game.stats.turnsSent;

    if ( isOut ) {
        return leaveWithoutPlayer();
    }

    return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
}

void Multi::startSimultaneousDay()
{
    const NetworkGame & game = networkGame();
    if ( game.role != Role::HOST ) {
        return;
    }

    // Who plays what is settled for the whole day.
    if ( updatePlayedColors() ) {
        applyAIControl();
        sendMachineTables();
    }

    // Each guest starts with its first player in the turn order.
    std::vector<GameReceiver> receivers;
    for ( const MachineId machine : getGuestsWithPlayers() ) {
        receivers.push_back( { machine, MessageType::DAY_START, getFirstColor( getColorsPlayedBy( machine ) ) } );
    }

    const PlayerColor currentColor = Settings::Get().CurrentColor();

    for ( const GameReceiver & receiver : receivers ) {
        Settings::Get().SetCurrentColor( receiver.color );
        const std::string error = sendGame( { receiver }, TurnStage::NOT_STARTED );

        if ( !error.empty() ) {
            Settings::Get().SetCurrentColor( currentColor );
            ERROR_LOG( "[MULTI] Unable to start a day played at the same time: " << error )
            return;
        }
    }

    Settings::Get().SetCurrentColor( currentColor );
    beginSimultaneousDay();
}

bool Multi::isFirstTurnOfLoadedGame()
{
    NetworkGame & game = networkGame();
    return std::exchange( game.isLoadedAtStart, false );
}

void Multi::onTurnBoundary()
{
    if ( networkGame().role == Role::HOST && !isSimultaneousDay() && updatePlayedColors() ) {
        applyAIControl();
        sendMachineTables();
    }
}

void Multi::onTurnStart( const PlayerColor color )
{
    watchTurnBegin( color );

    // Meanwhile the guests look at the game.
    if ( networkGame().role == Role::HOST && ( getLocalHumanColors() & color ) != 0 && !isSimultaneousDay() ) {
        // The game is saved as a regular save of the turn which starts on this computer.
        Settings::Get().SetCurrentColor( color );

        if ( const std::vector<GameReceiver> receivers = getSnapshotReceivers( 0 ); !receivers.empty() ) {
            if ( const std::string error = sendGame( receivers, TurnStage::NOT_STARTED ); !error.empty() ) {
                ERROR_LOG( "[MULTI] Unable to send a snapshot of the game: " << error )
            }
        }
    }

    // Only once we know if the day is simultaneous.
    jumpForTest( color );
    toggleForTest();
    attackForTest( color );
}

bool Multi::isTestDayLimitReached()
{
    const NetworkGame & game = networkGame();

    return game.testOptions.isEnabled && game.testOptions.lastDay != 0 && world.CountDay() >= game.testOptions.lastDay;
}

void Multi::sendToMachine( const MachineId machine, const Message & message )
{
    const NetworkGame & game = networkGame();

    // While the connection is lost, the message is kept until it is back.
    if ( game.session && game.session->isAlive() ) {
        game.session->send( machine, message );
    }
}

void Multi::sendToOthers( const Message & message )
{
    const NetworkGame & game = networkGame();

    if ( game.session && game.session->isAlive() ) {
        game.session->sendToMachines( getOtherMachines(), message );
    }
}

void Multi::processDayMessages( const bool isApplyingUpdates /* = true */ )
{
    const NetworkGame & game = networkGame();
    if ( !game.session ) {
        return;
    }

    game.session->update();
    answerReservations();

    if ( !isApplyingUpdates ) {
        return;
    }

    // Read in the order of arrival. A watched battle needs the world from before it, its changes come after its end.
    bool isChanged = false;
    while ( std::optional<Message> message
            = game.session->receive( { MessageType::WORLD_UPDATE, MessageType::SIMULTANEOUS_TURNS, MessageType::DAY_DONE, MessageType::DAY_RESUME,
                                       MessageType::DAY_RESUMED, MessageType::DAY_LOCKED, MessageType::DAY_CLOSED, MessageType::SPECTATE_START,
                                       MessageType::SPECTATE_ACTIONS, MessageType::SPECTATE_END, MessageType::MACHINES } ) ) {
        switch ( message->type ) {
        case MessageType::WORLD_UPDATE:
            isChanged = applyLiveUpdate( *message ) || isChanged;
            break;
        case MessageType::SIMULTANEOUS_TURNS:
            onSimultaneousTurnsMessage( *message );
            break;
        case MessageType::SPECTATE_START:
        case MessageType::SPECTATE_ACTIONS:
        case MessageType::SPECTATE_END:
            onSpectatedMessage( *message );
            break;
        case MessageType::MACHINES:
            if ( game.role == Role::GUEST ) {
                onMachineTable( *message );
            }
            break;
        default:
            onDayReadinessMessage( *message );
            break;
        }
    }

    if ( isChanged ) {
        Interface::AdventureMap & adventureMap = Interface::AdventureMap::Get();

        // The hero in focus may have been lost meanwhile.
        if ( canFocusHere() && Interface::GetFocusType() == GameFocus::UNSEL ) {
            adventureMap.ResetFocus( GameFocus::FIRSTHERO, false );
        }

        Interface::GameArea::updateMapFogDirections();
        adventureMap.getRadar().Build();
        adventureMap.setRedraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );
    }
}

void Multi::updateGuests()
{
    const NetworkGame & game = networkGame();
    if ( game.role != Role::HOST || !game.session ) {
        return;
    }

    game.session->update();
    takeComebacks();

    if ( followGuestConnections() ) {
        sendMachineTables();
    }
}

bool Multi::hasAbsentGuests()
{
    const std::vector<MachineInfo> & machines = getMachineTable().machines;
    return std::any_of( machines.begin(), machines.end(),
                        []( const MachineInfo & info ) { return info.state == MachineState::LOST || info.state == MachineState::LEFT; } );
}

void Multi::abandonAbsentGuests()
{
    const NetworkGame & game = networkGame();
    if ( game.role != Role::HOST || !game.session ) {
        return;
    }

    for ( const MachineInfo & info : std::vector<MachineInfo>( getMachineTable().machines ) ) {
        if ( info.state != MachineState::LOST && info.state != MachineState::LEFT ) {
            continue;
        }

        VERBOSE_LOG( "[MULTI] The game goes on without computer " << static_cast<int>( info.id ) << ", the AI plays its players from their next turn" )

        // If it comes back, it joins as a new computer and gets its place back.
        game.session->closeLink( info.id );
        setMachineState( info.id, MachineState::GONE );
    }

    sendMachineTables();
}

void Multi::resyncReturningGuests()
{
    NetworkGame & game = networkGame();
    if ( game.role != Role::HOST || !game.session || game.resyncPending == 0 ) {
        return;
    }

    const MachineSet pending = game.resyncPending;
    game.resyncPending = 0;

    Settings & conf = Settings::Get();
    const PlayerColor currentColor = conf.CurrentColor();

    for ( MachineId machine = hostMachine + 1; machine <= maxMachineId; ++machine ) {
        if ( ( pending & machineBit( machine ) ) == 0 || !game.session->isAlive( machine ) ) {
            continue;
        }

        // Goes first, the day sent next may be simultaneous.
        game.session->send( machine, makeSimultaneousTurns( { areSimultaneousTurnsEnabled(), false } ) );

        const PlayerColorsSet colors = getColorsPlayedBy( machine );
        std::string error;

        if ( isSimultaneousDay() && isDayLocked() ) {
            // Its day is over, it has nothing more to send today.
            setOtherDayClosed( machine );
        }

        if ( isSimultaneousDay() && !isDayLocked() && hasPlayerInGame( colors ) ) {
            // Its players play their day again from this point.
            restartOtherDay( machine );

            const PlayerColor color = getFirstColor( colors );
            conf.SetCurrentColor( color );
            error = sendGame( { { machine, MessageType::DAY_START, color } }, TurnStage::IN_PROGRESS );
            conf.SetCurrentColor( currentColor );
        }
        else if ( game.otherTurnColor != PlayerColor::NONE && ( colors & game.otherTurnColor ) != 0 ) {
            // Its turn goes on from the last state known by the host.
            conf.SetCurrentColor( game.otherTurnColor );
            error = sendGame( { { machine, MessageType::TURN_STATE, game.otherTurnColor } }, TurnStage::IN_PROGRESS );
            conf.SetCurrentColor( currentColor );
        }
        else {
            // Another player is playing, it only looks at the game.
            const MachineInfo * info = findMachine( machine );
            error = sendGame( { { machine, MessageType::GAME_SNAPSHOT, getFirstColor( info != nullptr ? info->ownedColors : colors ) } }, TurnStage::IN_PROGRESS );
        }

        if ( !error.empty() ) {
            ERROR_LOG( "[MULTI] Unable to send the game to computer " << static_cast<int>( machine ) << ": " << error )
            continue;
        }

        VERBOSE_LOG( "[MULTI] The game was sent to computer " << static_cast<int>( machine ) << ", which came back" )
    }
}

void Multi::waitForOtherDaysClosed()
{
    const NetworkGame & game = networkGame();
    if ( game.role != Role::HOST || !game.session ) {
        return;
    }

    LocalEvent & le = LocalEvent::Get();
    const auto start = std::chrono::steady_clock::now();
    std::unique_ptr<MapWait> wait;

    while ( !areOtherDaysClosed() ) {
        if ( !keepConnection( WaitKind::MAP ) ) {
            return;
        }

        le.HandleEvents( true, true );
        processDayMessages();

        // Usually at once. It can take a while when another game is in the background.
        if ( !wait && std::chrono::steady_clock::now() - start > std::chrono::seconds( 2 ) ) {
            wait = std::make_unique<MapWait>( _( "Waiting for the other players..." ) );
        }
    }
}

uint8_t Multi::getColorsToClaim()
{
    const PlayerColorsSet seated = getSeatedColors();
    return seated != 0 ? seated : getLocalHumanColors();
}

void Multi::replaceSessionAfterHostRestart( std::unique_ptr<NetworkSession> session )
{
    NetworkGame & game = networkGame();

    game.nextSession = std::move( session );

    VERBOSE_LOG( "[MULTI] Joined the restarted game of the host, waiting for the game it sends" )

    pendingLeave = fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
}

bool Multi::isSessionReplaced()
{
    return networkGame().nextSession != nullptr;
}

fheroes2::GameMode Multi::onAdventureMapFrame()
{
    if ( const fheroes2::GameMode mode = leaveIfNetworkGameOver(); mode != fheroes2::GameMode::CANCEL ) {
        return mode;
    }

    if ( !isSimultaneousDay() ) {
        return fheroes2::GameMode::CANCEL;
    }

    processDayMessages();
    showSimultaneousTurnsNotice();

    // Battle requests are not answered while a hero of this player is moving.
    const Heroes * hero = Interface::GetFocusHeroes();
    if ( hero == nullptr || !hero->isMoveEnabled() ) {
        answerBattleRequests();
    }

    return fheroes2::GameMode::CANCEL;
}

fheroes2::GameMode Multi::leaveIfNetworkGameOver()
{
    if ( pendingLeave ) {
        const fheroes2::GameMode mode = *pendingLeave;
        pendingLeave.reset();
        return mode;
    }

    const NetworkGame & game = networkGame();
    if ( game.role == Role::NONE || !game.session ) {
        return fheroes2::GameMode::CANCEL;
    }

    if ( game.role == Role::GUEST ) {
        if ( game.session->isAlive() ) {
            return fheroes2::GameMode::CANCEL;
        }

        return stopOnError( std::string( gettext_noop( "the connection with the host was lost" ) ) + ": " + game.session->error() );
    }

    // Self-tests stop after their last day, also once the AI plays for every guest.
    if ( game.testOptions.lastDay != 0 && world.CountDay() > game.testOptions.lastDay ) {
        return fheroes2::GameMode::MAIN_MENU;
    }

    // Between two turns a computer which came back can get the game.
    updateGuests();
    resyncReturningGuests();

    // Nobody else can come back: the game goes on here alone.
    bool hasGuestPlayers = false;
    for ( const MachineInfo & info : getMachineTable().machines ) {
        hasGuestPlayers = hasGuestPlayers || ( info.id != hostMachine && hasPlayerInGame( info.ownedColors ) );
    }

    if ( !hasGuestPlayers ) {
        goOnAlone();
        endGame();
    }

    return fheroes2::GameMode::CANCEL;
}

void Multi::refreshMap()
{
    refreshObservedMap();
}

void Multi::observeUntil( const std::function<bool()> & isWaiting )
{
    NetworkGame & game = networkGame();
    if ( !game.session ) {
        return;
    }

    game.isObserving = true;

    const fheroes2::GameMode mode = Interface::AdventureMap::Get().observeOtherPlayers( [&isWaiting]() {
        processDayMessages();
        answerBattleRequests();

        return keepConnection( WaitKind::MAP ) && isWaiting();
    } );

    game.isObserving = false;

    if ( mode == fheroes2::GameMode::QUIT_GAME ) {
        endGame();
        throw fheroes2::UserRequestedApplicationClosure();
    }

    // The player left on purpose. Back to the main menu once the turn can end.
    if ( mode == fheroes2::GameMode::MAIN_MENU ) {
        endGame();
        pendingLeave = fheroes2::GameMode::MAIN_MENU;
    }
}

void Multi::waitForOtherDays()
{
    NetworkGame & game = networkGame();
    if ( !game.session ) {
        return;
    }

    Settings::Get().SetCurrentColor( game.localColor );
    game.isObserving = true;

    const fheroes2::GameMode mode = Interface::AdventureMap::Get().observeOtherPlayers( []() {
        processDayMessages();
        showSimultaneousTurnsNotice();
        answerBattleRequests();

        return !areOtherDaysDone() && keepConnection( WaitKind::MAP );
    } );

    game.isObserving = false;

    if ( areOtherDaysDone() ) {
        return;
    }

    if ( mode == fheroes2::GameMode::QUIT_GAME ) {
        endGame();
        throw fheroes2::UserRequestedApplicationClosure();
    }

    // The player left on purpose. A closed connection is handled by leaveIfNetworkGameOver().
    if ( mode != fheroes2::GameMode::CANCEL ) {
        endGame();
        pendingLeave = fheroes2::GameMode::MAIN_MENU;
    }
}

namespace
{
    // Host only. The computer playing a classic turn left: the turn ends as it is known here, the AI plays its players
    // from their next turn.
    bool isTurnOfGoneComputer()
    {
        const NetworkGame & game = networkGame();
        if ( game.role != Role::HOST || game.otherTurnColor == PlayerColor::NONE ) {
            return false;
        }

        const std::optional<Multi::MachineId> machine = Multi::getMachineOfColor( game.otherTurnColor );
        const Multi::MachineInfo * info = machine ? Multi::findMachine( *machine ) : nullptr;
        return info == nullptr || info->state == Multi::MachineState::GONE;
    }

    fheroes2::GameMode takeTurnBack()
    {
        NetworkGame & game = networkGame();
        const PlayerColor color = game.otherTurnColor;

        VERBOSE_LOG( "[MULTI] The computer playing the turn of " << Color::String( color ) << " is gone, the turn ends here" )

        // Loaded again as if the turn came back, from what is known of it here.
        Settings::Get().SetCurrentColor( color );

        std::vector<uint8_t> data;
        uint64_t fingerprint = 0;
        std::string error = Multi::saveGameData( data, fingerprint );
        if ( error.empty() ) {
            error = Multi::loadGameData( data );
        }
        if ( !error.empty() ) {
            return stopOnError( error );
        }

        Multi::resetLiveBaseline();
        game.otherTurnColor = PlayerColor::NONE;
        game.resumeStage = Multi::TurnStage::ENDED;
        Multi::endSimultaneousDay();

        return fheroes2::GameMode::START_GAME;
    }
}

fheroes2::GameMode Multi::waitForTurn()
{
    NetworkGame & game = networkGame();
    if ( game.role == Role::NONE || !game.session ) {
        assert( 0 );
        return fheroes2::GameMode::MAIN_MENU;
    }

    // The restarted host takes over, nothing uses the previous session any more.
    if ( game.nextSession ) {
        game.session = std::move( game.nextSession );
        pendingLeave.reset();
        endSimultaneousDay();
    }

    if ( isTurnOfGoneComputer() ) {
        return takeTurnBack();
    }

    std::optional<Message> message;

    // The guest has no game to look at until the first turn arrives.
    if ( game.localColor != PlayerColor::NONE && ( game.role == Role::HOST || game.playingColor != PlayerColor::NONE ) ) {
        Settings::Get().SetCurrentColor( game.localColor );

        game.isObserving = true;

        const fheroes2::GameMode mode = Interface::AdventureMap::Get().observeOtherPlayers( [&game, &message]() {
            game.session->update();
            message = game.session->receive();

            // The actions of the other players are shown as they happen, and their reservations are answered at once. A guest
            // which leaves is followed by updateGuests().
            while ( message
                    && ( message->type == MessageType::WORLD_UPDATE || message->type == MessageType::RESERVE || message->type == MessageType::DAY_END
                         || message->type == MessageType::BATTLE_REQUEST || message->type == MessageType::SPECTATE_START || message->type == MessageType::SPECTATE_ACTIONS
                         || message->type == MessageType::SPECTATE_END || message->type == MessageType::MACHINES
                         || ( message->type == MessageType::GAME_END && game.role == Role::HOST ) ) ) {
                if ( message->type == MessageType::RESERVE ) {
                    game.session->putBack( std::move( *message ) );
                    answerReservations();
                }
                else if ( message->type == MessageType::BATTLE_REQUEST ) {
                    game.session->putBack( std::move( *message ) );
                    answerBattleRequests();
                }
                else if ( message->type == MessageType::SPECTATE_START || message->type == MessageType::SPECTATE_ACTIONS || message->type == MessageType::SPECTATE_END ) {
                    onSpectatedMessage( *message );
                }
                else if ( message->type == MessageType::DAY_END ) {
                    if ( const std::optional<DayEnd> dayEnd = readDayEnd( *message ); dayEnd ) {
                        checkDayEnd( *dayEnd );
                    }
                }
                else if ( message->type == MessageType::MACHINES ) {
                    if ( game.role == Role::GUEST ) {
                        onMachineTable( *message );
                    }
                }
                else if ( message->type == MessageType::WORLD_UPDATE ) {
                    if ( applyLiveUpdate( *message ) ) {
                        refreshObservedMap();
                    }
                }

                message = game.session->receive();
            }

            return !message && keepConnection( WaitKind::MAP ) && !isTurnOfGoneComputer();
        } );

        game.isObserving = false;

        if ( mode != fheroes2::GameMode::CANCEL ) {
            endGame();
            return mode;
        }
    }
    else {
        WaitingScreen screen( _( "Waiting for the host..." ) );

        // Ignore messages about a game we do not have yet, like the end of a battle we were in before joining again.
        while ( true ) {
            message = waitForMessage( *game.session, [&screen]() { return screen.keepWaiting() && keepConnection( WaitKind::MAP ); } );

            if ( message && message->type == MessageType::MACHINES ) {
                onMachineTable( *message );
                continue;
            }

            if ( !message || isGameMessageForNewcomer( *message ) ) {
                break;
            }

            VERBOSE_LOG( "[MULTI] Message " << static_cast<uint32_t>( message->type ) << " left aside before the first game" )
        }

        if ( !message && screen.isLeftByPlayer() ) {
            endGame();
            return screen.modeAfterLeaving();
        }
    }

    if ( pendingLeave ) {
        const fheroes2::GameMode mode = *pendingLeave;
        pendingLeave.reset();
        return mode;
    }

    if ( message ) {
        return onMessage( *message );
    }

    if ( isTurnOfGoneComputer() ) {
        return takeTurnBack();
    }

    if ( game.role == Role::HOST && game.session->isAlive() ) {
        return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
    }

    return stopOnError( std::string( gettext_noop( "the connection with the host was lost" ) ) + ": " + game.session->error() );
}

Multi::MachineWatch Multi::watchMachine( const MachineId machine )
{
    const MachineInfo * info = findMachine( machine );
    return { machine, info != nullptr ? info->comebacks : 0U };
}

bool Multi::isStillThere( const MachineWatch & watch )
{
    const MachineInfo * info = findMachine( watch.machine );
    return info != nullptr && info->state != MachineState::GONE && info->comebacks == watch.comebacks;
}

fheroes2::GameMode Multi::hostGame( const std::string & filePath, const uint16_t port, const bool localOnly )
{
    // Game::Load() only accepts a save file of the current game type, and shows its own error messages.
    Settings::Get().SetGameType( Game::TYPE_HOTSEAT );
    if ( Game::Load( filePath ) == fheroes2::GameMode::CANCEL ) {
        return fheroes2::GameMode::MAIN_MENU;
    }

    return hostCurrentGame( port, localOnly );
}

fheroes2::GameMode Multi::hostCurrentGame( const uint16_t port, const bool localOnly, const std::optional<LobbyHosting> & lobby )
{
    const WaitingRoomInfo info = makeWaitingRoomInfo( makeGameName( lobby ? lobby->gameName : std::string() ) );

    if ( Color::Count( info.humanColors ) < 2 ) {
        showNetworkError( _( "Unable to start the network game:" ), gettext_noop( "this game needs at least two human players still in the game" ) );
        return fheroes2::GameMode::MAIN_MENU;
    }

    // For a game over the internet we ask the box to let the other players in. Not for a game only through a relay.
    std::unique_ptr<PortMapping> portMapping;
    if ( !localOnly && !( lobby && lobby->isRelayOnly ) ) {
        portMapping = std::make_unique<PortMapping>();
        portMapping->start( port );
    }

    const std::vector<std::string> addresses = localOnly ? std::vector<std::string>() : getLocalAddresses();
    const std::string addressV6 = localOnly ? std::string() : getInternetAddressV6();

    // On a lobby, the game is published once the box is open or gave up: the lobby checks the addresses at once.
    std::unique_ptr<PublishedRoom> room;
    const LobbyKey probeKey = makeRandomLobbyKey();
    const auto hostingStart = std::chrono::steady_clock::now();
    int freeColors = Color::Count( info.humanColors ) - 1;

    HostRoomHooks hooks;

    hooks.openSession = [port, localOnly, &probeKey]( std::string & error ) { return openHostSession( port, localOnly, probeKey, error ); };

    hooks.update = [&]( NetworkSession & session, JoinHelp & help, std::string & error ) {
        if ( portMapping ) {
            portMapping->update();
        }

        std::optional<BoxStatus> box;
        if ( portMapping ) {
            box = BoxStatus{ portMapping->state(), portMapping->isBehindAnotherNetwork(), portMapping->externalAddress(), portMapping->error() };
        }

        if ( !lobby ) {
            help = { {}, makeAddressJoinText( box, addresses, addressV6, port ) };
            return true;
        }

        const bool isBoxReady = !portMapping || portMapping->state() == PortMapping::State::OPEN || portMapping->state() == PortMapping::State::FAILED
                                || std::chrono::steady_clock::now() - hostingStart > std::chrono::seconds( 10 );
        if ( !room && isBoxReady ) {
            room = std::make_unique<PublishedRoom>( lobby->lobbyAddress, lobby->nickname, makeLobbyRoom( *lobby, port, probeKey, freeColors, portMapping.get() ) );
            room->setState( freeColors > 0 ? RoomState::WAITING_FOR_PLAYERS : RoomState::IN_GAME, makeRoomGameOfCurrentGame( static_cast<uint8_t>( freeColors ) ) );
        }

        if ( room ) {
            room->update();
            if ( room->state() == PublishedRoom::State::FAILED ) {
                error = room->error();
                return false;
            }
            listenOnRelayOfRoom( *room, session );
        }

        help = makeLobbyJoinHelp( room.get(), *lobby );
        return true;
    };

    // In the list of the lobby as long as some colors are free.
    hooks.onPlayersChange = [&room, &freeColors]( const int colors ) {
        freeColors = colors;
        if ( room ) {
            room->setState( colors > 0 ? RoomState::WAITING_FOR_PLAYERS : RoomState::IN_GAME, makeRoomGameOfCurrentGame( static_cast<uint8_t>( colors ) ) );
        }
    };

    std::unique_ptr<NetworkSession> session;
    RoomPlayers players;
    std::string error;

    switch ( runHostWaitingRoom( info, hooks, session, players, error ) ) {
    case WaitingRoomEnd::STARTED:
        break;
    case WaitingRoomEnd::QUIT_GAME:
        return fheroes2::GameMode::QUIT_GAME;
    case WaitingRoomEnd::FAILED:
        if ( room && room->state() == PublishedRoom::State::FAILED ) {
            ERROR_LOG( "[MULTI] The lobby refused the game: " << error )
            showNetworkError( _( "The lobby refused the game:" ), error );
        }
        else {
            ERROR_LOG( "[MULTI] " << error )
            showNetworkError( _( "Unable to start the network game:" ), error );
        }
        return fheroes2::GameMode::MAIN_MENU;
    default:
        return fheroes2::GameMode::MAIN_MENU;
    }

    // Human colors nobody took are played by the computer.
    uint8_t takenColors = players.hostColors;
    for ( const RoomGuest & guest : players.guests ) {
        takenColors |= guest.colors;
    }

    if ( const auto freeHumanColors = static_cast<uint8_t>( info.humanColors & ~takenColors ); freeHumanColors != 0 ) {
        VERBOSE_LOG( "[MULTI] Human colors nobody took are played by the computer: " << describeColors( freeHumanColors ) )

        for ( Player * player : Settings::Get().GetPlayers().getVector() ) {
            if ( ( freeHumanColors & static_cast<uint8_t>( player->GetColor() ) ) != 0 ) {
                player->setHandicapStatus( Player::HandicapStatus::NONE );
                player->SetControl( CONTROL_AI );
            }
        }

        if ( const std::string reloadError = reloadWithPlayers(); !reloadError.empty() ) {
            ERROR_LOG( "[MULTI] " << reloadError )
            session->close();
            showNetworkError( _( "Unable to start the network game:" ), reloadError );
            return fheroes2::GameMode::MAIN_MENU;
        }
    }

    fheroes2::engine().requestAttention();
    startHostGame( std::move( session ), players.guests );
    networkGame().portMapping = std::move( portMapping );
    if ( room ) {
        keepPublishedRoom( std::move( room ) );
    }

    return fheroes2::GameMode::START_GAME;
}

fheroes2::GameMode Multi::joinGame( const std::string & address, const uint16_t port )
{
    VERBOSE_LOG( "[MULTI] Joining " << address << ":" << port )

    WaitingScreen screen( _( "Connecting to the host..." ) );

    std::unique_ptr<NetworkSession> session;

    while ( true ) {
        session = std::make_unique<NetworkSession>();

        const std::string error = connectToHost( *session, address, port, screen.callback() );
        if ( error.empty() ) {
            break;
        }

        if ( screen.isLeftByPlayer() ) {
            return screen.modeAfterLeaving();
        }

        if ( session->isAddressUnknown() ) {
            ERROR_LOG( "[MULTI] " << error )
            showNetworkError( _( "Unable to join the network game:" ), error );
            return fheroes2::GameMode::MULTI_NETWORK_MENU;
        }

        // The host may not be ready yet. Try again until the player gives up.
        VERBOSE_LOG( "[MULTI] " << error << ", trying again" )

        const auto retryTime = std::chrono::steady_clock::now() + std::chrono::seconds( 1 );
        while ( std::chrono::steady_clock::now() < retryTime ) {
            if ( !screen.keepWaiting() ) {
                return screen.modeAfterLeaving();
            }
        }
    }

    if ( const std::string error = checkHostHandshake( *session, screen.callback() ); !error.empty() ) {
        if ( screen.isLeftByPlayer() ) {
            return screen.modeAfterLeaving();
        }

        ERROR_LOG( "[MULTI] " << error )
        showNetworkError( _( "Unable to join the network game:" ), error );
        return fheroes2::GameMode::MAIN_MENU;
    }

    std::string error;
    switch ( runGuestWaitingRoom( *session, error ) ) {
    case WaitingRoomEnd::STARTED:
        startGuestGame( std::move( session ) );
        return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
    case WaitingRoomEnd::QUIT_GAME:
        return fheroes2::GameMode::QUIT_GAME;
    case WaitingRoomEnd::FAILED:
        ERROR_LOG( "[MULTI] " << error )
        showNetworkError( _( "The network game has been stopped:" ), error );
        return fheroes2::GameMode::MULTI_NETWORK_MENU;
    default:
        return fheroes2::GameMode::MULTI_NETWORK_MENU;
    }
}

void Multi::applyTestAutoPlay()
{
    applyAIControl();
}

void Multi::showHostWaitingScreenForTest( const HostWaitingCase waitingCase )
{
    // Made up addresses, the real ones are never shown here.
    const std::vector<std::string> addresses{ "100.101.102.103", "192.168.1.20" };
    const std::string addressV6{ "2001:db8:4c2:1d00::25" };

    std::optional<BoxStatus> box;
    switch ( waitingCase ) {
    case HostWaitingCase::OPENING:
        box = BoxStatus{};
        break;
    case HostWaitingCase::OPEN:
        box = BoxStatus{ PortMapping::State::OPEN, false, "203.0.113.25", {} };
        break;
    case HostWaitingCase::BEHIND_ANOTHER_NETWORK:
        box = BoxStatus{ PortMapping::State::OPEN, true, "203.0.113.25", {} };
        break;
    case HostWaitingCase::FAILED:
        box = BoxStatus{ PortMapping::State::FAILED, false, {}, "UPnP is off or missing in the box" };
        break;
    default:
        // A local game does not use the box.
        break;
    }

    JoinHelp help{ {}, makeAddressJoinText( box, box ? addresses : std::vector<std::string>(), box ? addressV6 : std::string(), defaultPort ) };

    const bool isOnLobby = ( waitingCase == HostWaitingCase::LOBBY || waitingCase == HostWaitingCase::GUEST_IN );
    if ( isOnLobby ) {
        help.inviteCode = "K7Q2MX";
        help.text = _( "Your game is in the list of the lobby %{lobby}." );
        StringReplace( help.text, "%{lobby}", "fheroes2" );
        help.text += std::string( "\n\n" ) + _( "Other players join you directly." );
    }

    showWaitingRoomForTest( makeWaitingRoomInfo( makeGameName( isOnLobby ? "Duel" : std::string() ) ), help, waitingCase == HostWaitingCase::GUEST_IN );
}
