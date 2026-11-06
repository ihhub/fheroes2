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

#include "multi_spectator.h"

#include <algorithm>
#include <deque>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>

#include "army.h"
#include "battle.h"
#include "battle_arena.h"
#include "battle_command.h"
#include "battle_interface.h"
#include "battle_troop.h"
#include "castle.h"
#include "game_hotkeys.h"
#include "heroes.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "multi_battle.h"
#include "multi_connection.h"
#include "multi_fingerprint.h"
#include "multi_game.h"
#include "multi_live.h"
#include "multi_network.h"
#include "multi_protocol.h"
#include "multi_reservation.h"
#include "rand.h"
#include "settings.h"
#include "system.h"
#include "translations.h"
#include "world.h"

namespace
{
    struct SpectatorState
    {
        // Last battle of another computer which can be watched, the computer leading it, and its orders so far
        std::optional<Multi::BattleStart> start;
        Multi::MachineWatch leader;
        std::vector<Multi::BattleActions> turns;
        bool isOver{ false };
        bool isWatchedToTheEnd{ false };
        std::optional<uint64_t> leaderFingerprint;
        std::optional<uint64_t> localFingerprint;

        // While the battle is shown
        bool isWatching{ false };
        size_t nextTurn{ 0 };
        bool isLeaving{ false };
        // Messages of other battles, in their order
        std::deque<Multi::Message> laterMessages;

        bool isTestEnabled{ false };
        uint32_t testBattleCount{ 0 };
        std::optional<size_t> testLeaveTurn;
        // In self-tests, orders considered as received so far. The next ones arrive one by one
        std::optional<size_t> testReceivedTurns;

        bool isVisitingCastle{ false };

        Multi::SpectatorStats stats;
    };

    SpectatorState & spectatorState()
    {
        static SpectatorState state;
        return state;
    }

    size_t receivedTurns()
    {
        const SpectatorState & state = spectatorState();
        return state.testReceivedTurns ? std::min( *state.testReceivedTurns, state.turns.size() ) : state.turns.size();
    }

    void forgetBattle()
    {
        SpectatorState & state = spectatorState();
        state.start.reset();
        state.turns.clear();
        state.isOver = false;
        state.isWatchedToTheEnd = false;
        state.leaderFingerprint.reset();
        state.localFingerprint.reset();
    }

    void compareEnds()
    {
        SpectatorState & state = spectatorState();
        if ( !state.leaderFingerprint || !state.localFingerprint ) {
            return;
        }

        if ( *state.leaderFingerprint == *state.localFingerprint ) {
            ++state.stats.sameEnds;
            VERBOSE_LOG( "[MULTI] The battle watched here ended the same way" )
        }
        else {
            ++state.stats.differentEnds;
            ERROR_LOG( "[MULTI] Battle " << ( state.start ? state.start->battleId : 0 ) << " watched here ended differently (defender "
                                         << ( state.start ? static_cast<int>( state.start->defender ) : -1 ) << ", " << state.turns.size() << " orders)" )
        }

        state.localFingerprint.reset();
    }

    std::optional<uint32_t> getBattleId( const Multi::Message & message )
    {
        const std::optional<Multi::Message> battleMessage = Multi::readSpectatedMessage( message );
        if ( !battleMessage ) {
            return {};
        }

        switch ( message.type ) {
        case Multi::MessageType::SPECTATE_START:
            if ( const std::optional<Multi::BattleStart> start = Multi::readBattleStart( *battleMessage ); start ) {
                return start->battleId;
            }
            break;
        case Multi::MessageType::SPECTATE_ACTIONS:
            if ( const std::optional<Multi::BattleActions> actions = Multi::readBattleActions( *battleMessage ); actions ) {
                return actions->battleId;
            }
            break;
        case Multi::MessageType::SPECTATE_END:
            if ( const std::optional<Multi::BattleEnd> end = Multi::readBattleEnd( *battleMessage ); end ) {
                return end->battleId;
            }
            break;
        default:
            break;
        }

        return {};
    }

    // Returns true when the battle known here has just ended.
    bool applySpectatedMessage( const Multi::Message & message )
    {
        SpectatorState & state = spectatorState();

        // Another battle, of another computer, waits for the end of the one watched
        if ( state.isWatching && state.start && getBattleId( message ) != state.start->battleId ) {
            state.laterMessages.push_back( message );
            return false;
        }

        const std::optional<Multi::Message> battleMessage = Multi::readSpectatedMessage( message );
        if ( !battleMessage ) {
            return false;
        }

        switch ( message.type ) {
        case Multi::MessageType::SPECTATE_START: {
            const std::optional<Multi::BattleStart> start = Multi::readBattleStart( *battleMessage );
            if ( !start || !start->saveData.empty() ) {
                ERROR_LOG( "[MULTI] Invalid watched battle received" )
                return false;
            }

            forgetBattle();
            state.start = start;
            state.leader = Multi::watchMachine( message.sender );
            ++state.stats.battlesOffered;

            VERBOSE_LOG( "[MULTI] Battle " << start->battleId << " of an ally can be watched" )
            return false;
        }
        case Multi::MessageType::SPECTATE_ACTIONS: {
            const std::optional<Multi::BattleActions> actions = Multi::readBattleActions( *battleMessage );
            if ( actions && state.start && actions->battleId == state.start->battleId && !state.isOver ) {
                state.turns.push_back( *actions );
            }
            return false;
        }
        case Multi::MessageType::SPECTATE_END: {
            const std::optional<Multi::BattleEnd> end = Multi::readBattleEnd( *battleMessage );
            if ( !end || !state.start || end->battleId != state.start->battleId ) {
                return false;
            }

            state.isOver = true;
            state.leaderFingerprint = end->fingerprint;
            compareEnds();
            return true;
        }
        default:
            return false;
        }
    }
}

void Multi::onSpectatedMessage( const Message & message )
{
    if ( !applySpectatedMessage( message ) ) {
        return;
    }

    // In self-tests the battle is watched as soon as it ends, before its world changes arrive. It is watched whole, joined in
    // the middle, or left and joined again, in turn.
    SpectatorState & state = spectatorState();
    if ( !state.isTestEnabled || state.isWatching ) {
        return;
    }

    const size_t half = state.turns.size() / 2;

    switch ( state.testBattleCount++ % 3 ) {
    case 0:
        watchSpectatedBattle();
        break;
    case 1:
        state.testReceivedTurns = half;
        watchSpectatedBattle();
        break;
    default:
        state.testReceivedTurns = half;
        state.testLeaveTurn = half + 2;
        watchSpectatedBattle();

        state.testReceivedTurns = half + 4;
        state.testLeaveTurn.reset();
        if ( state.start ) {
            watchSpectatedBattle();
        }
        break;
    }

    state.testReceivedTurns.reset();
    state.testLeaveTurn.reset();
}

void Multi::processSpectatedMessages()
{
    NetworkSession * session = getSession();
    if ( session == nullptr ) {
        return;
    }

    // Read in their order. The end of a battle comes before the start of the next one
    while ( std::optional<Message> message = session->receive( { MessageType::SPECTATE_START, MessageType::SPECTATE_ACTIONS, MessageType::SPECTATE_END } ) ) {
        applySpectatedMessage( *message );
    }
}

bool Multi::isSpectatedBattleAvailable()
{
    // A battle not shown on the other computer is resolved at once, nothing to watch
    const SpectatorState & state = spectatorState();
    return state.start.has_value() && state.start->isShown && !state.isWatching && !state.isOver && !state.isWatchedToTheEnd;
}

void Multi::watchSpectatedBattle()
{
    SpectatorState & state = spectatorState();
    if ( !isSpectatedBattleAvailable() && ( !state.isTestEnabled || !state.start || state.isWatching ) ) {
        return;
    }

    const BattleStart start = *state.start;

    Heroes * attackingHero = Heroes::isValidId( static_cast<int>( start.attackerHeroId ) ) ? world.GetHeroes( static_cast<int>( start.attackerHeroId ) ) : nullptr;
    Army * defendingArmy = nullptr;
    std::unique_ptr<Army> monsters;

    if ( Maps::isValidAbsIndex( start.tileIndex ) ) {
        switch ( start.defender ) {
        case BattleStart::Defender::CASTLE:
            if ( Castle * castle = world.getCastleEntrance( Maps::GetPoint( start.tileIndex ) ); castle != nullptr ) {
                defendingArmy = &castle->GetActualArmy();
            }
            break;
        case BattleStart::Defender::HERO:
            if ( Heroes * hero = Heroes::isValidId( static_cast<int>( start.defenderHeroId ) ) ? world.GetHeroes( static_cast<int>( start.defenderHeroId ) ) : nullptr;
                 hero != nullptr ) {
                defendingArmy = &hero->GetArmy();
            }
            break;
        case BattleStart::Defender::MONSTERS:
            // Army( tile ) asserts on a tile without monsters
            if ( const Maps::Tile & tile = world.getTile( start.tileIndex ); tile.getMainObjectType( false ) == start.defenderObject ) {
                monsters = std::make_unique<Army>( tile );
                defendingArmy = monsters.get();
            }
            break;
        default:
            break;
        }
    }

    if ( attackingHero == nullptr || defendingArmy == nullptr || computeArmiesFingerprint( attackingHero->GetArmy(), *defendingArmy ) != start.fingerprint ) {
        ERROR_LOG( "[MULTI] Battle " << start.battleId << " cannot be watched: the armies are not the same here" )
        forgetBattle();
        return;
    }

    VERBOSE_LOG( "[MULTI] Watching battle " << start.battleId << ", " << state.turns.size() << " order(s) to catch up with" )

    ++state.stats.battlesWatched;

    // Undo what the watched battle changes (spell points...)
    WorldRollback rollback;

    state.isWatching = true;
    state.nextTurn = 0;
    state.isLeaving = false;

    beginWatchedBattle( start.battleId );

    {
        Rand::PCG32 randomGenerator( start.seed );
        // Self-tests also replay battles not shown on the other computer
        Battle::Arena arena( attackingHero->GetArmy(), *defendingArmy, start.tileIndex, start.isShown, randomGenerator );

        // The missed part is played at once, hidden
        if ( receivedTurns() > 0 ) {
            arena.hideInterface();
        }

        while ( arena.BattleValid() && !state.isLeaving ) {
            arena.Turns();
        }

        if ( !state.isLeaving ) {
            const Battle::Result result = arena.GetResult();
            state.localFingerprint = computeBattleResultFingerprint( arena, result );
            ++state.stats.battlesWatchedToTheEnd;
        }

        arena.FadeArena( false );
    }

    endWatchedBattle();
    state.isWatching = false;

    rollback.restore();

    if ( state.isLeaving ) {
        ++state.stats.battlesLeft;
        VERBOSE_LOG( "[MULTI] Battle " << start.battleId << " left" )
    }
    else {
        compareEnds();

        // If the other end has not arrived yet, keep the battle to compare both ends
        if ( state.isOver ) {
            forgetBattle();
        }
        else {
            state.isWatchedToTheEnd = true;
        }
    }

    while ( !state.isWatching && !state.laterMessages.empty() ) {
        const Message message = std::move( state.laterMessages.front() );
        state.laterMessages.pop_front();
        applySpectatedMessage( message );
    }

    refreshMap();
}

void Multi::receiveWatchedActions( Battle::Arena & arena, const Battle::Unit & unit, Battle::Actions & actions )
{
    SpectatorState & state = spectatorState();

    LocalEvent & le = LocalEvent::Get();

    while ( true ) {
        // All missed orders are played, show the battle from now on
        if ( state.nextTurn >= receivedTurns() ) {
            arena.showInterface();
        }

        if ( Battle::Interface * battleInterface = Battle::Arena::GetInterface(); battleInterface != nullptr ) {
            battleInterface->setStatus( System::isHandheldDevice() ? _( "Battle of your ally (touch to leave)" ) : _( "Battle of your ally (Esc to leave)" ), true );
        }

        if ( !state.isLeaving ) {
            le.HandleEvents( false );

            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || ( System::isHandheldDevice() && le.MouseClickLeft() )
                 || ( state.testLeaveTurn && state.nextTurn >= *state.testLeaveTurn ) ) {
                state.isLeaving = true;
            }
        }

        // Skip the rest of the round, the battle is left at its end
        if ( state.isLeaving ) {
            actions.emplace_back( Battle::CommandType::SKIP, std::vector<int>{ static_cast<int>( unit.GetUID() ) } );
            return;
        }

        if ( state.nextTurn < receivedTurns() ) {
            const BattleActions & turn = state.turns[state.nextTurn];
            ++state.nextTurn;

            if ( !toBattleActions( turn.commands, actions ) ) {
                ERROR_LOG( "[MULTI] Invalid orders in the watched battle" )
                state.isLeaving = true;
                continue;
            }

            return;
        }

        // In self-tests the next order arrives now.
        if ( state.testReceivedTurns && *state.testReceivedTurns < state.turns.size() ) {
            le.HandleEvents( true, true );
            ++*state.testReceivedTurns;
            continue;
        }

        // The battle is over there but not here, the player resolved it at once
        if ( state.isOver && state.nextTurn >= state.turns.size() ) {
            VERBOSE_LOG( "[MULTI] The watched battle ended without its last orders" )
            state.isLeaving = true;
            continue;
        }

        NetworkSession * session = getSession();
        if ( session == nullptr || !keepConnection() || !isStillThere( state.leader ) ) {
            state.isLeaving = true;
            continue;
        }

        le.HandleEvents( true, true );
        session->update();
        processSpectatedMessages();
        answerReservations();
    }
}

void Multi::onWorldChangedForSpectator( const bool isGameLoaded )
{
    const SpectatorState & state = spectatorState();

    // While the battle is watched, nothing is applied.
    if ( state.isWatching ) {
        return;
    }

    if ( isGameLoaded || state.isOver ) {
        forgetBattle();
    }
}

void Multi::visitCastle( Castle & castle )
{
    SpectatorState & state = spectatorState();
    Settings & conf = Settings::Get();

    if ( !castle.isFriends( conf.CurrentColor() ) ) {
        return;
    }

    ++state.stats.castlesVisited;
    VERBOSE_LOG( "[MULTI] Looking at the castle " << castle.GetName() )

    // Nothing can be done in the castle. Undo anything that changes anyway.
    WorldRollback rollback;

    const PlayerColor currentColor = conf.CurrentColor();
    conf.SetCurrentColor( castle.GetColor() );
    state.isVisitingCastle = true;

    castle.OpenDialog( false, false, true, true );

    state.isVisitingCastle = false;
    conf.SetCurrentColor( currentColor );

    rollback.restore();
    refreshMap();
}

void Multi::enableTestSpectate()
{
    spectatorState().isTestEnabled = true;
}

void Multi::spectateForTest()
{
    if ( spectatorState().isTestEnabled && isSpectatedBattleAvailable() ) {
        watchSpectatedBattle();
    }
}

bool Multi::isCastleVisit()
{
    return spectatorState().isVisitingCastle;
}

std::string Multi::getCastleStatus( const std::string & status )
{
    return isCastleVisit() ? _( "Look only, while the others are playing" ) : status;
}

const Multi::SpectatorStats & Multi::getSpectatorStats()
{
    return spectatorState().stats;
}

std::string Multi::describeSpectator()
{
    const SpectatorStats & stats = spectatorState().stats;

    std::ostringstream os;
    os << stats.battlesOffered << " battle(s) offered, " << stats.battlesWatched << " watch(es), " << stats.battlesLeft << " left, " << stats.battlesWatchedToTheEnd
       << " watched to the end (" << stats.sameEnds << " same end(s), " << stats.differentEnds << " different), " << stats.castlesVisited << " castle(s) visited";

    return os.str();
}
