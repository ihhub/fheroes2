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

#include "multi_live_battle.h"

#include <chrono>
#include <memory>
#include <optional>
#include <sstream>

#include "castle.h"
#include "color.h"
#include "game_hotkeys.h"
#include "heroes.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "maps.h"
#include "multi_action_watch.h"
#include "multi_battle.h"
#include "multi_connection.h"
#include "multi_game.h"
#include "multi_hero.h"
#include "multi_live.h"
#include "multi_machines.h"
#include "multi_network.h"
#include "multi_notice.h"
#include "multi_protocol.h"
#include "multi_reservation.h"
#include "multi_simultaneous.h"
#include "multi_spectator.h"
#include "players.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_text.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    // The remote player may still be busy with a dialog.
    constexpr std::chrono::seconds battleStartTimeout{ 120 };

    struct LiveBattleState
    {
        uint32_t requestCount{ 0 };

        // Attacker only, until the end of the battle is sent, and the computer of the other player.
        std::optional<uint32_t> agreedRequestId;
        Multi::MachineId agreedMachine{ Multi::hostMachine };

        // In self-tests the AI plans on the map from the start of its turn, which has changed meanwhile. Its heroes stop
        // for the day after the battle.
        std::optional<PlayerColor> stoppedColor;

        bool isTestAttacksEnabled{ false };

        Multi::LiveBattleStats stats;
    };

    LiveBattleState & liveBattleState()
    {
        static LiveBattleState state;
        return state;
    }

    // The lowest bits tell which computer sent the request.
    uint32_t makeRequestId()
    {
        LiveBattleState & state = liveBattleState();
        ++state.requestCount;

        return ( state.requestCount << 3 ) | Multi::getThisMachine();
    }

    // The computer of the player whose hero or castle is attacked.
    std::optional<Multi::MachineId> getTargetMachine( const int32_t targetIndex )
    {
        if ( !Maps::isValidAbsIndex( targetIndex ) ) {
            return {};
        }

        if ( const Heroes * hero = world.getTile( targetIndex ).getHero(); hero != nullptr ) {
            return Multi::getMachineOfColor( hero->GetColor() );
        }

        if ( const Castle * castle = world.getCastleEntrance( Maps::GetPoint( targetIndex ) ); castle != nullptr ) {
            return Multi::getMachineOfColor( castle->GetColor() );
        }

        return {};
    }

    std::string withName( const char * text, const Multi::MachineId machine )
    {
        std::string result = _( text );
        StringReplace( result, "%{name}", Multi::getMachineName( machine ) );
        return result;
    }

    // Self-tests only, see stoppedColor.
    void stopComputerPlayerForTheDay( const PlayerColor color )
    {
        for ( Heroes * hero : world.GetKingdom( color ).GetHeroes() ) {
            hero->ResetMovePoints();
        }
    }

    Multi::NetworkSession * getConnectedSession()
    {
        Multi::NetworkSession * session = Multi::getSession();
        if ( session == nullptr || !session->isAlive() ) {
            return nullptr;
        }

        return session;
    }

    bool isCommonMessage( const Multi::MessageType type )
    {
        switch ( type ) {
        case Multi::MessageType::WORLD_UPDATE:
        case Multi::MessageType::SIMULTANEOUS_TURNS:
        case Multi::MessageType::SPECTATE_START:
        case Multi::MessageType::SPECTATE_ACTIONS:
        case Multi::MessageType::SPECTATE_END:
        case Multi::MessageType::BATTLE_REQUEST:
        case Multi::MessageType::DAY_DONE:
        case Multi::MessageType::DAY_RESUME:
            return true;
        default:
            return false;
        }
    }

    // Read in order with the battle messages. A watched battle needs the world from before it, and what comes after the
    // end of the battle must wait.
    void processCommonMessage( Multi::NetworkSession & session, const Multi::Message & message )
    {
        switch ( message.type ) {
        case Multi::MessageType::WORLD_UPDATE:
            Multi::applyLiveUpdate( message );
            break;
        case Multi::MessageType::SIMULTANEOUS_TURNS:
            Multi::onSimultaneousTurnsMessage( message );
            break;
        case Multi::MessageType::SPECTATE_START:
        case Multi::MessageType::SPECTATE_ACTIONS:
        case Multi::MessageType::SPECTATE_END:
            Multi::onSpectatedMessage( message );
            break;
        case Multi::MessageType::DAY_DONE:
        case Multi::MessageType::DAY_RESUME:
            Multi::onDayReadinessMessage( message );
            break;
        case Multi::MessageType::BATTLE_REQUEST:
            // Another battle is being arranged here, this one is not played now.
            if ( const std::optional<Multi::BattleRequest> request = Multi::readBattleRequest( message ); request ) {
                session.send( message.sender, Multi::makeBattleRequestReply( { request->requestId, false } ) );
                ++liveBattleState().stats.requestsRefusedHere;
                VERBOSE_LOG( "[MULTI] Live battle " << request->requestId << " refused: a battle is already being arranged here" )
            }
            break;
        default:
            break;
        }
    }
}

bool Multi::requestLiveBattle( Heroes & hero, const int32_t targetIndex )
{
    LiveBattleState & state = liveBattleState();

    NetworkSession * session = getConnectedSession();
    const std::optional<MachineId> target = getTargetMachine( targetIndex );
    if ( session == nullptr || !target || *target == getThisMachine() ) {
        return false;
    }

    // Close an earlier agreement that was not used.
    finishLiveBattle();

    // The other computers must know what this computer did so far before the battle.
    sendLiveUpdate();

    const MachineWatch watch = watchMachine( *target );
    const BattleRequest request{ makeRequestId(), static_cast<uint32_t>( hero.GetID() ), targetIndex };
    session->send( *target, makeBattleRequest( request ) );
    ++state.stats.requestsSent;

    VERBOSE_LOG( "[MULTI] Live battle " << request.requestId << " asked to computer " << static_cast<int>( *target ) << ": " << hero.GetName() << " attacks tile "
                                        << targetIndex )

    const bool isPlayerHere = hero.isControlHuman();
    const bool canCallOff = isPlayerHere && !isTestMode();

    std::unique_ptr<MapWait> wait;
    if ( canCallOff ) {
        wait = std::make_unique<MapWait>( withName( System::isHandheldDevice() ? gettext_noop( "Waiting for %{name} (touch the screen to call off the attack)" )
                                                                               : gettext_noop( "Waiting for %{name} (Esc to call off the attack)" ),
                                                    *target ) );
    }

    LocalEvent & le = LocalEvent::Get();
    std::optional<bool> isAccepted;

    const MachineId targetMachine = *target;
    const auto isAwaited = [targetMachine]( const Message & message ) {
        return isCommonMessage( message.type ) || ( message.type == MessageType::BATTLE_REQUEST_REPLY && message.sender == targetMachine );
    };

    while ( !isAccepted ) {
        le.HandleEvents( true, true );
        session->update();
        answerReservations();

        while ( std::optional<Message> message = session->receive( isAwaited ) ) {
            if ( isCommonMessage( message->type ) ) {
                processCommonMessage( *session, *message );
                continue;
            }

            const std::optional<BattleRequestReply> reply = readBattleRequestReply( *message );
            if ( !reply ) {
                isAccepted = false;
                break;
            }

            // Answer to an attack given up before.
            if ( reply->requestId != request.requestId ) {
                continue;
            }

            isAccepted = reply->isAccepted;
            break;
        }

        if ( isAccepted ) {
            break;
        }

        if ( !keepConnection() || !isStillThere( watch ) ) {
            isAccepted = false;
            break;
        }

        if ( canCallOff && ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || ( System::isHandheldDevice() && le.MouseClickLeft() ) ) ) {
            // The other player may already be stopped, so they must be told to continue.
            session->send( targetMachine, makeLiveBattleDone( request.requestId ) );
            ++state.stats.requestsGivenUp;
            VERBOSE_LOG( "[MULTI] Live battle " << request.requestId << " given up" )
            return false;
        }
    }

    wait.reset();
    watchRebaseline();

    // In self-tests the map has changed under the AI playing for this player.
    const bool isStoppedAfterwards = !isPlayerHere && isTestMode();

    if ( !*isAccepted ) {
        if ( isStoppedAfterwards ) {
            stopComputerPlayerForTheDay( hero.GetColor() );
        }

        ++state.stats.requestsRefused;
        VERBOSE_LOG( "[MULTI] Live battle " << request.requestId << " refused" )

        if ( isPlayerHere && !isTestMode() ) {
            showMapNote( withName( gettext_noop( "%{name} is busy. Try again in a moment." ), targetMachine ) );
        }
        return false;
    }

    // The changes of the other player have been applied. Their hero may have left.
    if ( !attacksOtherComputer( hero, targetIndex, getRemoteHumanColors() ) || getTargetMachine( targetIndex ) != targetMachine ) {
        session->send( targetMachine, makeLiveBattleDone( request.requestId ) );
        ++state.stats.requestsGivenUp;
        VERBOSE_LOG( "[MULTI] Live battle " << request.requestId << " given up: the target left" )

        if ( isStoppedAfterwards ) {
            stopComputerPlayerForTheDay( hero.GetColor() );
        }

        if ( isPlayerHere && !isTestMode() ) {
            showMapNote( _( "Your target has moved away." ) );
        }
        return false;
    }

    ++state.stats.requestsAccepted;
    state.agreedRequestId = request.requestId;
    state.agreedMachine = targetMachine;

    if ( isStoppedAfterwards ) {
        state.stoppedColor = hero.GetColor();
    }

    return true;
}

bool Multi::isLiveBattleAgreed()
{
    return liveBattleState().agreedRequestId.has_value();
}

void Multi::finishLiveBattle()
{
    LiveBattleState & state = liveBattleState();
    if ( !state.agreedRequestId ) {
        return;
    }

    const uint32_t requestId = *state.agreedRequestId;
    state.agreedRequestId.reset();

    // The changes made by the battle must be sent before the end.
    sendLiveUpdate();

    if ( NetworkSession * session = getConnectedSession(); session != nullptr ) {
        session->send( state.agreedMachine, makeLiveBattleDone( requestId ) );
    }

    VERBOSE_LOG( "[MULTI] Live battle " << requestId << " over here" )

    if ( state.stoppedColor ) {
        stopComputerPlayerForTheDay( *state.stoppedColor );
        state.stoppedColor.reset();
    }
}

void Multi::answerBattleRequests()
{
    LiveBattleState & state = liveBattleState();

    NetworkSession * session = getConnectedSession();
    if ( session == nullptr ) {
        return;
    }

    while ( std::optional<Message> message = session->receive( MessageType::BATTLE_REQUEST ) ) {
        const std::optional<BattleRequest> request = readBattleRequest( *message );
        if ( !request ) {
            ERROR_LOG( "[MULTI] Invalid battle request received" )
            continue;
        }

        // The other computers must know what this computer did so far before the battle.
        const MachineId attacker = message->sender;
        const MachineWatch watch = watchMachine( attacker );

        sendLiveUpdate();
        session->send( attacker, makeBattleRequestReply( { request->requestId, true } ) );
        ++state.stats.requestsAnswered;

        VERBOSE_LOG( "[MULTI] Live battle " << request->requestId << " accepted: computer " << static_cast<int>( attacker ) << " attacks tile " << request->targetIndex )

        fheroes2::engine().requestAttention();

        auto wait = std::make_unique<MapWait>( withName( gettext_noop( "%{name} is attacking you!" ), attacker ) );

        // What the attacker sends about the battle, read in order with the rest.
        const auto isAwaited = [attacker]( const Message & next ) {
            switch ( next.type ) {
            case MessageType::BATTLE_START:
            case MessageType::BATTLE_END:
            case MessageType::HERO_LEVEL_UP:
            case MessageType::HERO_NOTICE:
            case MessageType::LIVE_BATTLE_DONE:
                return next.sender == attacker;
            default:
                return isCommonMessage( next.type );
            }
        };

        LocalEvent & le = LocalEvent::Get();
        auto start = std::chrono::steady_clock::now();
        bool isBattleStarted = false;

        bool isOver = false;

        while ( !isOver ) {
            le.HandleEvents( true, true );
            session->update();
            answerReservations();

            // Read until the end of the battle on the other computer. What the other player does next must wait.
            while ( std::optional<Message> next = session->receive( isAwaited ) ) {
                if ( isCommonMessage( next->type ) ) {
                    processCommonMessage( *session, *next );
                    continue;
                }

                switch ( next->type ) {
                case MessageType::BATTLE_START:
                    wait.reset();
                    isBattleStarted = true;

                    followNetworkBattle( *next );

                    wait = std::make_unique<MapWait>( withName( gettext_noop( "%{name} is finishing their move..." ), attacker ) );
                    break;
                case MessageType::BATTLE_END:
                    checkNetworkBattleEnd( *next );
                    break;
                case MessageType::HERO_LEVEL_UP:
                    wait.reset();
                    answerHeroLevelUp( *next );
                    break;
                case MessageType::HERO_NOTICE:
                    wait.reset();
                    showHeroNotice( *next );
                    break;
                default:
                    isOver = ( readLiveBattleDone( *next ).value_or( 0 ) == request->requestId );
                    break;
                }

                if ( isOver ) {
                    break;
                }
            }

            if ( isOver || !keepConnection() || !isStillThere( watch ) ) {
                break;
            }

            // The time while a connection was lost does not count.
            if ( session->state() == NetworkSession::State::RECONNECTING || session->state( attacker ) == NetworkSession::State::RECONNECTING ) {
                start = std::chrono::steady_clock::now();
            }

            if ( !isBattleStarted && std::chrono::steady_clock::now() - start > battleStartTimeout ) {
                ERROR_LOG( "[MULTI] Live battle " << request->requestId << ": the battle did not start, the game goes on" )
                break;
            }
        }

        wait.reset();
        watchRebaseline();

        VERBOSE_LOG( "[MULTI] Live battle " << request->requestId << " over, the game goes on here" )

        refreshMap();
    }
}

void Multi::enableTestAttacks()
{
    liveBattleState().isTestAttacksEnabled = true;
}

void Multi::attackForTest( const PlayerColor color )
{
    LiveBattleState & state = liveBattleState();

    if ( !state.isTestAttacksEnabled || !isSimultaneousDay() || ( getLocalHumanColors() & color ) == 0 ) {
        return;
    }

    const uint32_t phase = world.CountDay() % 3;
    if ( ( phase == 1 && isGuest() ) || ( phase == 0 && !isGuest() ) ) {
        return;
    }

    const auto isOnLand = []( const Heroes * hero ) { return !hero->isShipMaster() && hero->inCastle() == nullptr; };

    Heroes * attacker = nullptr;
    for ( Heroes * hero : world.GetKingdom( color ).GetHeroes() ) {
        if ( isOnLand( hero ) ) {
            attacker = hero;
            break;
        }
    }

    // The target changes from day to day, so that every pair of computers fights.
    const PlayerColorsVector others( getRemoteHumanColors() );
    const Heroes * defender = nullptr;

    for ( size_t i = 0; i < others.size(); ++i ) {
        const PlayerColor otherColor = others[( i + world.CountDay() ) % others.size()];
        if ( attacker == nullptr || attacker->isFriends( otherColor ) ) {
            continue;
        }

        for ( const Heroes * hero : world.GetKingdom( otherColor ).GetHeroes() ) {
            if ( isOnLand( hero ) ) {
                defender = hero;
                break;
            }
        }

        if ( defender != nullptr ) {
            break;
        }
    }

    if ( attacker == nullptr || defender == nullptr ) {
        return;
    }

    for ( const int32_t index : Maps::getAroundIndexes( defender->GetIndex() ) ) {
        if ( !Maps::isValidForDimensionDoor( index, false ) ) {
            continue;
        }

        // Jump next to the defender like with Dimension Door.
        if ( !reserveSpellTarget( *attacker, index ) ) {
            return;
        }

        attacker->Scout( index );
        attacker->Move2Dest( index );
        attacker->GetPath().Reset();
        attacker->ActionNewPosition( false );
        sendLiveUpdate( attacker );

        if ( !attacker->isActive() ) {
            return;
        }

        // The turn loop sets the current player just after this.
        Settings::Get().SetCurrentColor( color );

        attacker->calculatePath( defender->GetIndex() );
        attacker->SetMove( true );

        ++state.stats.testAttacks;
        VERBOSE_LOG( "[MULTI] Self-test: " << attacker->GetName() << " attacks " << defender->GetName() << ", path of " << attacker->GetPath().size() << " step(s), "
                                           << attacker->GetMovePoints() << " move points" )

        while ( attacker->isActive() && attacker->isMoveEnabled() ) {
            if ( !attacker->Move( true ) ) {
                break;
            }
        }

        return;
    }
}

const Multi::LiveBattleStats & Multi::getLiveBattleStats()
{
    return liveBattleState().stats;
}

std::string Multi::describeLiveBattles()
{
    const LiveBattleStats & stats = liveBattleState().stats;

    std::ostringstream os;
    os << stats.requestsSent << " asked (" << stats.requestsAccepted << " played, " << stats.requestsRefused << " refused, " << stats.requestsGivenUp << " given up), "
       << stats.requestsAnswered << " answered for the other computer, " << stats.requestsRefusedHere << " refused here";

    if ( stats.testAttacks > 0 ) {
        os << ", " << stats.testAttacks << " test attack(s)";
    }

    return os.str();
}
