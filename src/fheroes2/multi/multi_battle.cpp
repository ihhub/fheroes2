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

#include "multi_battle.h"

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "ai_battle.h"
#include "army.h"
#include "battle.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_board.h"
#include "battle_command.h"
#include "battle_interface.h"
#include "battle_troop.h"
#include "castle.h"
#include "color.h"
#include "exception.h"
#include "game_exit.h"
#include "heroes.h"
#include "heroes_base.h"
#include "localevent.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "multi_connection.h"
#include "multi_fingerprint.h"
#include "multi_game.h"
#include "multi_live.h"
#include "multi_live_battle.h"
#include "multi_machines.h"
#include "multi_network.h"
#include "multi_simultaneous.h"
#include "multi_spectator.h"
#include "players.h"
#include "rand.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

namespace
{
    struct NetworkBattle
    {
        bool isActive{ false };
        bool isLeader{ false };

        // The computer of the other army: the follower for the leader, the leader for the follower. None for a battle
        // only watched by allies.
        std::optional<Multi::MachineId> peer;
        Multi::MachineWatch peerWatch;

        // Leader only. Computers of allies which get every order.
        Multi::MachineSet spectators{ 0 };

        // The other computer is gone or could not join. The AI plays its units from then on.
        bool isRemoteLost{ false };

        // A battle of another computer is watched here
        bool isSpectator{ false };

        // ID of the battle being played or of the last battle followed here.
        uint32_t battleId{ 0 };

        // Follower only. Compared once both are known.
        std::optional<uint64_t> localFingerprint;
        std::optional<uint64_t> leaderFingerprint;
    };

    NetworkBattle & networkBattle()
    {
        static NetworkBattle battle;
        return battle;
    }

    // The lowest bits tell which computer leads, so IDs of different computers never overlap
    uint32_t makeBattleId()
    {
        static uint32_t battleCount = 0;
        ++battleCount;

        return ( battleCount << 3 ) | Multi::getThisMachine();
    }

    Multi::NetworkBattleStats & battleStats()
    {
        static Multi::NetworkBattleStats stats;
        return stats;
    }

    void loseRemote( const std::string & reason )
    {
        NetworkBattle & battle = networkBattle();

        if ( !battle.isRemoteLost ) {
            ERROR_LOG( "[MULTI] The AI plays the units of the other player until the end of the battle: " << reason )
            battle.isRemoteLost = true;
        }
    }

    Multi::NetworkSession * getConnectedSession()
    {
        Multi::NetworkSession * session = Multi::getSession();

        if ( session == nullptr || session->state() == Multi::NetworkSession::State::CLOSED ) {
            return nullptr;
        }

        return session;
    }

    // To the other computer taking part, and the same orders to the allies watching.
    void sendToPeer( const Multi::Message & message )
    {
        const NetworkBattle & battle = networkBattle();
        if ( Multi::NetworkSession * session = getConnectedSession(); session != nullptr && battle.peer ) {
            session->send( *battle.peer, message );
        }
    }

    void sendToSpectators( const Multi::Message & message )
    {
        const NetworkBattle & battle = networkBattle();
        if ( Multi::NetworkSession * session = getConnectedSession(); session != nullptr && battle.spectators != 0 ) {
            session->sendToMachines( battle.spectators, Multi::makeSpectatedMessage( message ) );
        }
    }

    // Battle reslut and remaining units of both armies
    uint64_t computeBattleFingerprint( const Battle::Arena & arena, const Battle::Result & result )
    {
        uint64_t hash = Multi::fnv1aOffsetBasis;

        const auto add = [&hash]( const uint32_t value ) {
            const std::array<uint8_t, 4> bytes{ static_cast<uint8_t>( value >> 24 ), static_cast<uint8_t>( value >> 16 ), static_cast<uint8_t>( value >> 8 ),
                                                static_cast<uint8_t>( value ) };
            hash = Multi::computeFnv1aHash( bytes.data(), bytes.size(), hash );
        };

        add( result.attacker );
        add( result.defender );

        for ( const Battle::Force * force : { &arena.getAttackingForce(), &arena.getDefendingForce() } ) {
            for ( const Battle::Unit * unit : *force ) {
                add( unit->GetUID() );
                add( unit->GetCount() );
                add( unit->GetHitPoints() );
            }
        }

        return hash;
    }

    void compareFingerprints()
    {
        NetworkBattle & battle = networkBattle();

        if ( !battle.localFingerprint || !battle.leaderFingerprint ) {
            return;
        }

        Multi::StateFingerprint local;
        local.hash = *battle.localFingerprint;
        Multi::StateFingerprint leader;
        leader.hash = *battle.leaderFingerprint;

        if ( local.hash == leader.hash ) {
            ++battleStats().fingerprintMatches;
            VERBOSE_LOG( "[MULTI] Both battles ended the same way: " << local.hashString() )
        }
        else {
            ++battleStats().fingerprintMismatches;
            ERROR_LOG( "[MULTI] The battles ended differently: " << local.hashString() << " here, " << leader.hashString() << " for the leader" )
        }

        battle.localFingerprint.reset();
        battle.leaderFingerprint.reset();
    }

    std::vector<Multi::BattleCommandData> toCommandData( const Battle::Actions & actions )
    {
        std::vector<Multi::BattleCommandData> commands;
        commands.reserve( actions.size() );

        for ( const Battle::Command & command : actions ) {
            Multi::BattleCommandData & data = commands.emplace_back();
            data.type = static_cast<uint32_t>( command.GetType() );
            data.values.assign( command.begin(), command.end() );
        }

        return commands;
    }

    bool toActions( const std::vector<Multi::BattleCommandData> & commands, Battle::Actions & actions )
    {
        for ( const Multi::BattleCommandData & data : commands ) {
            if ( !Multi::isValidBattleCommand( data ) ) {
                return false;
            }
        }

        for ( const Multi::BattleCommandData & data : commands ) {
            actions.emplace_back( static_cast<Battle::CommandType>( data.type ), std::vector<int>( data.values.begin(), data.values.end() ) );
        }

        return true;
    }

    // The AI plays the units of the other player from now on. The other player is told.
    void abandonNetworkBattle( const std::string & reason )
    {
        loseRemote( reason );

        Multi::BattleEnd end;
        end.battleId = networkBattle().battleId;
        sendToPeer( Multi::makeBattleEnd( end ) );
    }

    // Computers playing an ally of one of both armies may watch, never this one or the other army's computer.
    Multi::MachineSet getAllyComputers( const PlayerColor attackingColor, const PlayerColor defendingColor, const std::optional<Multi::MachineId> peer )
    {
        PlayerColorsSet allies = 0;
        for ( const PlayerColor color : { attackingColor, defendingColor } ) {
            if ( ( Color::allPlayerColors() & color ) != 0 ) {
                allies |= static_cast<PlayerColorsSet>( Players::GetPlayerFriends( color ) & ~static_cast<PlayerColorsSet>( color ) );
            }
        }

        Multi::MachineSet machines = Multi::getMachinesOfColors( allies );
        if ( peer ) {
            machines &= static_cast<Multi::MachineSet>( ~Multi::machineBit( *peer ) );
        }

        return static_cast<Multi::MachineSet>( machines & Multi::getOtherMachines() );
    }

    // The heroes, castle or monsters of the battle. Returns false if they cannot be told apart.
    bool describeArmies( Army & attackingArmy, Army & defendingArmy, const int32_t tileIndex, Multi::BattleStart & start, const bool areMonstersAllowed )
    {
        const Heroes * attackingHero = dynamic_cast<const Heroes *>( attackingArmy.GetCommander() );
        if ( attackingHero == nullptr || &attackingHero->GetArmy() != &attackingArmy ) {
            return false;
        }

        start.attackerHeroId = static_cast<uint32_t>( attackingHero->GetID() );
        start.tileIndex = tileIndex;

        const Castle * castle = world.getCastleEntrance( Maps::GetPoint( tileIndex ) );
        const Heroes * defendingHero = dynamic_cast<const Heroes *>( defendingArmy.GetCommander() );

        if ( castle != nullptr && &castle->GetActualArmy() == &defendingArmy ) {
            start.defender = Multi::BattleStart::Defender::CASTLE;
        }
        else if ( defendingHero != nullptr && &defendingHero->GetArmy() == &defendingArmy ) {
            start.defender = Multi::BattleStart::Defender::HERO;
            start.defenderHeroId = static_cast<uint32_t>( defendingHero->GetID() );
        }
        else if ( areMonstersAllowed && defendingArmy.GetCommander() == nullptr ) {
            start.defender = Multi::BattleStart::Defender::MONSTERS;
            start.defenderObject = static_cast<uint16_t>( world.getTile( tileIndex ).getMainObjectType( false ) );
        }
        else {
            return false;
        }

        return true;
    }
}

// The arena does not check the commands it gets. Values are stored in reverse order of the command's parameters.
bool Multi::isValidBattleCommand( const BattleCommandData & data )
{
    const size_t count = data.values.size();

    if ( data.type > static_cast<uint32_t>( Battle::CommandType::QUICK_COMBAT ) ) {
        return false;
    }

    switch ( static_cast<Battle::CommandType>( data.type ) ) {
    case Battle::CommandType::MOVE:
        return count == 2 && Battle::Board::isValidIndex( data.values[0] );
    case Battle::CommandType::ATTACK:
        return count == 5;
    case Battle::CommandType::SPELLCAST:
        if ( count == 0 ) {
            return false;
        }
        if ( data.values.back() == Spell::TELEPORT ) {
            return count == 3 && Battle::Board::isValidIndex( data.values[0] ) && Battle::Board::isValidIndex( data.values[1] );
        }
        return count == 2;
    case Battle::CommandType::SKIP:
        return count == 1;
    case Battle::CommandType::RETREAT:
    case Battle::CommandType::SURRENDER:
        return count == 0;
    case Battle::CommandType::TOGGLE_AUTO_COMBAT: {
        // Only for a player of another computer, canControlBattleColor() does not allow anyting else there
        if ( count != 1 ) {
            return false;
        }
        const PlayerColorsSet colors = static_cast<PlayerColorsSet>( data.values[0] );
        return Color::Count( colors ) == 1 && ( getRemoteHumanColors() & colors ) == colors;
    }
    default:
        // Morale, catapult and towers are decided by the arena on every computer. Quick combat is sent as auto combat.
        break;
    }

    return false;
}

namespace
{
    // In self-tests human players are played by the AI and their battles are not shown without this
    bool isEveryBattleShownForTest{ false };

    // Allies watch from the same start, with the armies to check it. Returns false if there is nobody to tell.
    bool offerToAllies( Army & attackingArmy, Army & defendingArmy, const int32_t tileIndex, const uint32_t seed, const bool isShown )
    {
        NetworkBattle & battle = networkBattle();

        battle.spectators = getAllyComputers( attackingArmy.GetColor(), defendingArmy.GetColor(), battle.peer );
        if ( battle.spectators == 0 || getConnectedSession() == nullptr ) {
            battle.spectators = 0;
            return false;
        }

        Multi::BattleStart start;
        start.battleId = battle.battleId;
        start.seed = seed;
        start.isShown = isShown || isEveryBattleShownForTest;

        if ( !describeArmies( attackingArmy, defendingArmy, tileIndex, start, true ) ) {
            battle.spectators = 0;
            return false;
        }

        // Both games are the same before the battle, the armies are enough
        Multi::sendLiveUpdate();
        start.fingerprint = Multi::computeArmiesFingerprint( attackingArmy, defendingArmy );

        sendToSpectators( Multi::makeBattleStart( start ) );

        VERBOSE_LOG( "[MULTI] Battle " << battle.battleId << " on tile " << tileIndex << " can be watched by allies" )

        return true;
    }
}

bool Multi::startNetworkBattleIfNeeded( Army & attackingArmy, Army & defendingArmy, const int32_t tileIndex, const uint32_t seed, const bool isShown )
{
    if ( !isNetworkGame() ) {
        return false;
    }

    NetworkBattle & battle = networkBattle();
    battle = {};
    battle.battleId = makeBattleId();

    // The other army may be played on another computer.
    const PlayerColorsSet remoteColors = getRemoteHumanColors() & ( attackingArmy.GetColor() | defendingArmy.GetColor() );
    if ( remoteColors != 0 ) {
        const PlayerColor remoteColor = ( remoteColors & defendingArmy.GetColor() ) != 0 ? defendingArmy.GetColor() : attackingArmy.GetColor();
        battle.peer = getMachineOfColor( remoteColor );
        if ( battle.peer ) {
            battle.peerWatch = watchMachine( *battle.peer );
        }
    }

    // Only watched: the battle goes on as usual here, quick combat included.
    if ( !battle.peer ) {
        const bool isOffered = offerToAllies( attackingArmy, defendingArmy, tileIndex, seed, isShown );
        if ( !isOffered ) {
            battle = {};
            return false;
        }

        battle.isActive = true;
        battle.isLeader = true;

        // Shown for self-tests only.
        return isEveryBattleShownForTest && !isShown;
    }

    battle.isActive = true;
    battle.isLeader = true;

    ++battleStats().battlesLed;

    // Nothing is sent before BATTLE_START. If the battle cannot be shared the AI plays the other player's units here.
    BattleStart start;
    start.battleId = battle.battleId;
    start.seed = seed;
    start.followerColors = getColorsPlayedBy( *battle.peer );

    if ( !describeArmies( attackingArmy, defendingArmy, tileIndex, start, false ) ) {
        loseRemote( "unknown armies" );
        return true;
    }

    if ( getConnectedSession() == nullptr ) {
        loseRemote( "no connection" );
        return true;
    }

    offerToAllies( attackingArmy, defendingArmy, tileIndex, seed, isShown );

    // On a simultaneous day the other player is stopped and both games are the same. The armies are enought.
    if ( isLiveBattleAgreed() ) {
        sendLiveUpdate();

        start.fingerprint = computeArmiesFingerprint( attackingArmy, defendingArmy );

        VERBOSE_LOG( "[MULTI] Network battle " << battle.battleId << " with computer " << static_cast<int>( *battle.peer ) << " started on tile " << tileIndex
                                               << " during the day, seed " << seed )

        sendToPeer( makeBattleStart( start ) );
        return true;
    }

    // The other computers must know the world the battle starts from.
    sendLiveUpdate();

    if ( const std::string error = saveGameData( start.saveData, start.fingerprint ); !error.empty() ) {
        loseRemote( error );
        return true;
    }

    VERBOSE_LOG( "[MULTI] Network battle " << battle.battleId << " with computer " << static_cast<int>( *battle.peer ) << " started on tile " << tileIndex << ", seed "
                                           << seed )

    sendToPeer( makeBattleStart( start ) );
    resetLiveBaseline();

    return true;
}

void Multi::endNetworkBattle( const Battle::Arena & arena, const Battle::Result & result )
{
    NetworkBattle & battle = networkBattle();

    if ( !battle.isActive || !battle.isLeader ) {
        return;
    }

    BattleEnd end;
    end.battleId = battle.battleId;
    end.fingerprint = computeBattleFingerprint( arena, result );

    sendToSpectators( makeBattleEnd( end ) );

    if ( battle.peer && !battle.isRemoteLost ) {
        sendToPeer( makeBattleEnd( end ) );
    }

    battle = {};
}

bool Multi::isNetworkBattle()
{
    return networkBattle().isActive;
}

void Multi::showEveryBattleForTest()
{
    isEveryBattleShownForTest = true;
}

bool Multi::isBattleWithOtherComputer()
{
    const NetworkBattle & battle = networkBattle();
    return battle.isActive && battle.peer.has_value() && !battle.isSpectator;
}

void Multi::beginWatchedBattle( const uint32_t battleId )
{
    NetworkBattle & battle = networkBattle();
    battle = {};
    battle.isActive = true;
    battle.isSpectator = true;
    battle.battleId = battleId;
}

void Multi::endWatchedBattle()
{
    networkBattle() = {};
}

bool Multi::toBattleActions( const std::vector<BattleCommandData> & commands, Battle::Actions & actions )
{
    return toActions( commands, actions );
}

uint64_t Multi::computeBattleResultFingerprint( const Battle::Arena & arena, const Battle::Result & result )
{
    return computeBattleFingerprint( arena, result );
}

Multi::BattleTurnSource Multi::getBattleTurnSource( const Battle::Unit & unit, const bool isAIControlled )
{
    const NetworkBattle & battle = networkBattle();

    if ( battle.isSpectator ) {
        return BattleTurnSource::REMOTE;
    }

    if ( !battle.isActive || !battle.peer ) {
        return isAIControlled ? BattleTurnSource::AI : BattleTurnSource::HUMAN;
    }

    // Human units are always played on their player's computer, so the player can take control back at any time
    const PlayerColor color = unit.GetCurrentColor();

    if ( ( getLocalHumanColors() & color ) != 0 ) {
        return ( isAIControlled || isTestMode() ) ? BattleTurnSource::AI : BattleTurnSource::HUMAN;
    }

    if ( ( getRemoteHumanColors() & color ) != 0 ) {
        return battle.isRemoteLost ? BattleTurnSource::AI : BattleTurnSource::REMOTE;
    }

    // Units of AI players are played by the leader.
    return ( battle.isLeader || battle.isRemoteLost ) ? BattleTurnSource::AI : BattleTurnSource::REMOTE;
}

void Multi::receiveBattleActions( Battle::Arena & arena, const Battle::Unit & unit, Battle::Actions & actions )
{
    NetworkBattle & battle = networkBattle();

    if ( battle.isSpectator ) {
        receiveWatchedActions( arena, unit, actions );
        return;
    }

    Battle::Interface * battleInterface = Battle::Arena::GetInterface();

    if ( battleInterface != nullptr ) {
        std::string status = _( "Waiting for %{name}..." );
        StringReplace( status, "%{name}", getMachineName( battle.peer.value_or( hostMachine ) ) );
        battleInterface->setStatus( status, true );
        battleInterface->Redraw();
    }

    LocalEvent & le = LocalEvent::Get();

    // Orders already received are played even if the connection is closed. Anything else waits until the battle is over.
    // A new turn or another battle from the other computer or from the host means the battle was left there.
    const MachineId peer = battle.peer.value_or( hostMachine );
    const auto isForBattle = [peer]( const Message & message ) {
        switch ( message.type ) {
        case MessageType::BATTLE_ACTIONS:
        case MessageType::BATTLE_END:
            return message.sender == peer;
        case MessageType::SPECTATE_START:
        case MessageType::SPECTATE_ACTIONS:
        case MessageType::SPECTATE_END:
        case MessageType::SIMULTANEOUS_TURNS:
            return true;
        case MessageType::GAME_END:
        case MessageType::TURN_STATE:
        case MessageType::GAME_SNAPSHOT:
        case MessageType::DAY_START:
        case MessageType::DAY_END:
        case MessageType::BATTLE_START:
            return message.sender == peer || message.sender == hostMachine;
        default:
            return false;
        }
    };

    while ( !battle.isRemoteLost ) {
        if ( !le.HandleEvents( true, true ) ) {
            if ( Game::processExitEvent() == fheroes2::GameMode::QUIT_GAME ) {
                abandonNetworkBattle( "the player left the game" );
                endGame();
                throw fheroes2::UserRequestedApplicationClosure{};
            }

            if ( battleInterface != nullptr ) {
                battleInterface->Redraw();
            }
        }

        NetworkSession * session = getSession();
        if ( session == nullptr ) {
            loseRemote( "connection lost" );
            break;
        }

        session->update();

        const std::optional<Message> message = session->receive( isForBattle );
        if ( !message ) {
            if ( !keepConnection() || !isStillThere( battle.peerWatch ) ) {
                loseRemote( "connection lost" );
                break;
            }

            continue;
        }

        switch ( message->type ) {
        case MessageType::BATTLE_ACTIONS: {
            const std::optional<BattleActions> received = readBattleActions( *message );
            if ( received && received->battleId != battle.battleId ) {
                VERBOSE_LOG( "[MULTI] Orders of battle " << received->battleId << " ignored" )
                continue;
            }

            if ( received && toActions( received->commands, actions ) ) {
                ++battleStats().ordersReceived;

                // The allies watching get the orders of the other army too, in their order.
                if ( battle.isLeader ) {
                    sendToSpectators( makeBattleActions( *received ) );
                }
                return;
            }

            abandonNetworkBattle( "invalid orders received" );
            break;
        }
        case MessageType::BATTLE_END: {
            const std::optional<BattleEnd> end = readBattleEnd( *message );
            if ( end && end->battleId != battle.battleId ) {
                VERBOSE_LOG( "[MULTI] End of battle " << end->battleId << " ignored" )
                continue;
            }

            // The leader's battle is over while this one goes on, or the follower gave up.
            if ( !battle.isLeader && end && end->fingerprint != 0 ) {
                battle.leaderFingerprint = end->fingerprint;
            }
            loseRemote( "the other battle is over" );
            break;
        }
        case MessageType::GAME_END:
            // The main loop tells the player
            loseRemote( "the other player has left the game" );
            session->putBack( *message );
            break;
        case MessageType::SPECTATE_START:
        case MessageType::SPECTATE_ACTIONS:
        case MessageType::SPECTATE_END:
            onSpectatedMessage( *message );
            continue;
        case MessageType::SIMULTANEOUS_TURNS:
            // The player is told after the battle
            onSimultaneousTurnsMessage( *message );
            continue;
        default:
            // The other computer has left this battle. The message is processed after it.
            session->putBack( *message );
            abandonNetworkBattle( "unexpected message " + std::to_string( static_cast<uint32_t>( message->type ) ) );
            break;
        }
    }

    AI::BattlePlanner::Get().BattleTurn( arena, unit, actions );
}

void Multi::sendBattleActions( const Battle::Unit & unit, Battle::Actions & actions )
{
    const NetworkBattle & battle = networkBattle();

    if ( !battle.isActive || battle.isSpectator ) {
        return;
    }

    // A battle only watched by allies goes on as usual here, quick combat included
    if ( !battle.peer ) {
        BattleActions sent;
        sent.battleId = battle.battleId;
        sent.commands = toCommandData( actions );
        sendToSpectators( makeBattleActions( sent ) );
        return;
    }

    if ( battle.isRemoteLost ) {
        return;
    }

    for ( Battle::Command & command : actions ) {
        if ( command.GetType() == Battle::CommandType::QUICK_COMBAT ) {
            command = Battle::Command( Battle::Command::TOGGLE_AUTO_COMBAT, static_cast<int>( static_cast<PlayerColorsSet>( unit.GetCurrentColor() ) ) );
        }
    }

    // A closed connection is detected later, after the orders already received have been playd
    BattleActions sent;
    sent.battleId = battle.battleId;
    sent.commands = toCommandData( actions );

    sendToPeer( makeBattleActions( sent ) );
    ++battleStats().ordersSent;

    if ( battle.isLeader ) {
        sendToSpectators( makeBattleActions( sent ) );
    }
}

bool Multi::canControlBattleColor( const PlayerColor color )
{
    const NetworkBattle & battle = networkBattle();
    return !battle.isActive || ( !battle.peer && !battle.isSpectator ) || ( !battle.isSpectator && ( getLocalHumanColors() & color ) != 0 );
}

void Multi::adjustBattleSummarySides( const PlayerColor attackerColor, const PlayerColor defenderColor, bool & isAttackerHuman, bool & isDefenderHuman )
{
    if ( !isNetworkGame() ) {
        return;
    }

    const bool isAttackerHere = isAttackerHuman && ( getLocalHumanColors() & attackerColor ) != 0;
    const bool isDefenderHere = isDefenderHuman && ( getLocalHumanColors() & defenderColor ) != 0;

    if ( isAttackerHere != isDefenderHere ) {
        isAttackerHuman = isAttackerHere;
        isDefenderHuman = isDefenderHere;
    }
}

fheroes2::GameMode Multi::followNetworkBattle( const Message & message )
{
    NetworkBattle & battle = networkBattle();
    battle = {};

    const std::optional<BattleStart> start = readBattleStart( message );
    if ( !start ) {
        ERROR_LOG( "[MULTI] Invalid battle start received" )
        return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
    }

    battle.battleId = start->battleId;
    battle.peer = message.sender;
    battle.peerWatch = watchMachine( message.sender );

    // Later messages of the leader about this battle are ignored
    const auto giveUp = []( const std::string & reason ) {
        abandonNetworkBattle( reason );
        return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
    };

    // No game is sent on a simultaneous day. The local game is the same as the leader's
    const bool isLive = start->saveData.empty();

    if ( !isLive ) {
        if ( const std::string error = loadGameData( start->saveData ); !error.empty() ) {
            return giveUp( error );
        }

        resetLiveBaseline();

        if ( computeStateFingerprint().hash != start->fingerprint ) {
            ERROR_LOG( "[MULTI] The game sent for the battle has a different fingerprint" )
        }
    }

    Heroes * attackingHero = world.GetHeroes( static_cast<int>( start->attackerHeroId ) );
    Army * defendingArmy = nullptr;

    if ( start->defender == BattleStart::Defender::CASTLE ) {
        if ( Maps::isValidAbsIndex( start->tileIndex ) ) {
            if ( Castle * castle = world.getCastleEntrance( Maps::GetPoint( start->tileIndex ) ); castle != nullptr ) {
                defendingArmy = &castle->GetActualArmy();
            }
        }
    }
    else if ( Heroes * defendingHero = world.GetHeroes( static_cast<int>( start->defenderHeroId ) ); defendingHero != nullptr ) {
        defendingArmy = &defendingHero->GetArmy();
    }

    if ( attackingHero == nullptr || defendingArmy == nullptr || !Maps::isValidAbsIndex( start->tileIndex ) ) {
        return giveUp( "the armies of the battle are not found" );
    }

    // Two different players, one of them on this computer
    const PlayerColor attackingColor = attackingHero->GetColor();
    const PlayerColor defendingColor = defendingArmy->GetColor();

    if ( attackingColor == defendingColor || ( getLocalHumanColors() & ( attackingColor | defendingColor ) ) == 0 ) {
        return giveUp( "invalid battle start received" );
    }

    if ( isLive && computeArmiesFingerprint( attackingHero->GetArmy(), *defendingArmy ) != start->fingerprint ) {
        return giveUp( "the armies of the battle are not the same here" );
    }

    VERBOSE_LOG( "[MULTI] Following the network battle " << battle.battleId << " of computer " << static_cast<int>( message.sender ) << " on tile " << start->tileIndex
                                                         << ", seed " << start->seed )

    battle.isActive = true;
    ++battleStats().battlesFollowed;

    {
        Rand::PCG32 randomGenerator( start->seed );
        Battle::Arena arena( attackingHero->GetArmy(), *defendingArmy, start->tileIndex, true, randomGenerator );

        while ( arena.BattleValid() ) {
            arena.Turns();
        }

        const Battle::Result result = arena.GetResult();
        battle.localFingerprint = computeBattleFingerprint( arena, result );

        arena.FadeArena( false );

        if ( !isTestMode() ) {
            arena.DialogBattleSummary( result, {}, false );
        }
    }

    battle.isActive = false;
    compareFingerprints();

    return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
}

void Multi::checkNetworkBattleEnd( const Message & message )
{
    NetworkBattle & battle = networkBattle();

    const std::optional<BattleEnd> end = readBattleEnd( message );
    if ( !end ) {
        ERROR_LOG( "[MULTI] Invalid battle end received" )
        return;
    }

    // Only the last battle followed here, and only once
    if ( end->battleId != battle.battleId || !battle.localFingerprint ) {
        VERBOSE_LOG( "[MULTI] End of battle " << end->battleId << " ignored" )
        return;
    }

    if ( end->fingerprint == 0 ) {
        VERBOSE_LOG( "[MULTI] The leader gave battle " << end->battleId << " up" )
        battle.localFingerprint.reset();
        return;
    }

    battle.leaderFingerprint = end->fingerprint;
    compareFingerprints();
}

uint64_t Multi::computeArmiesFingerprint( const Army & attackingArmy, const Army & defendingArmy )
{
    uint64_t hash = fnv1aOffsetBasis;

    const auto add = [&hash]( const int32_t value ) {
        const uint32_t bits = static_cast<uint32_t>( value );
        const std::array<uint8_t, 4> bytes{ static_cast<uint8_t>( bits >> 24 ), static_cast<uint8_t>( bits >> 16 ), static_cast<uint8_t>( bits >> 8 ),
                                            static_cast<uint8_t>( bits ) };
        hash = computeFnv1aHash( bytes.data(), bytes.size(), hash );
    };

    for ( const Army * army : { &attackingArmy, &defendingArmy } ) {
        add( static_cast<int32_t>( army->GetColor() ) );

        for ( size_t i = 0; i < army->Size(); ++i ) {
            const Troop * troop = army->GetTroop( i );
            add( troop != nullptr ? troop->GetID() : -1 );
            add( troop != nullptr ? static_cast<int32_t>( troop->GetCount() ) : 0 );
        }

        const HeroBase * commander = army->GetCommander();
        if ( commander == nullptr ) {
            add( -1 );
            continue;
        }

        add( commander->GetAttack() );
        add( commander->GetDefense() );
        add( commander->GetPower() );
        add( commander->GetKnowledge() );
        add( commander->GetMorale() );
        add( commander->GetLuck() );
        add( static_cast<int32_t>( commander->GetSpellPoints() ) );
    }

    return hash;
}

const Multi::NetworkBattleStats & Multi::getBattleStats()
{
    return battleStats();
}
