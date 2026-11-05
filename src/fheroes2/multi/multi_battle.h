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

#include <cstdint>
#include <vector>

#include "color.h"
#include "game_mode.h"
#include "multi_protocol.h"

class Army;

namespace Battle
{
    class Actions;
    class Arena;
    class Unit;
    struct Result;
}

namespace Multi
{
    // Same battle on the computer whose player attacks (the leader) and on the computer of the other army, with the same
    // seed. Each one sends the orders of its own units (the leader also those of AI units) so both battles stay identical.
    // If that computer is gone the AI plays its units. Computers of allies may watch, the leader sends them every order.

    // Returns true if the battle has to be shown. Without a player of another computer it may be offered to allies.
    bool startNetworkBattleIfNeeded( Army & attackingArmy, Army & defendingArmy, const int32_t tileIndex, const uint32_t seed, const bool isShown );

    void endNetworkBattle( const Battle::Arena & arena, const Battle::Result & result );

    bool isNetworkBattle();

    // Battles watched by an ally do not count
    bool isBattleWithOtherComputer();

    // In self-tests battels that can be watched are shown on the computer which plays them, as for a real player
    void showEveryBattleForTest();

    // Watching a battle of another computer, all orders come from there
    void beginWatchedBattle( const uint32_t battleId );
    void endWatchedBattle();

    // Returns false if any of the orders is invalid.
    bool toBattleActions( const std::vector<BattleCommandData> & commands, Battle::Actions & actions );

    // Returns the fingerprint of the battle result and of the remaining units of both armies.
    uint64_t computeBattleResultFingerprint( const Battle::Arena & arena, const Battle::Result & result );

    enum class BattleTurnSource
    {
        AI,
        HUMAN,
        // The orders come from another computer.
        REMOTE
    };

    // isAIControlled: whether the arena would let the AI play this unit in a normal game
    BattleTurnSource getBattleTurnSource( const Battle::Unit & unit, const bool isAIControlled );

    void receiveBattleActions( Battle::Arena & arena, const Battle::Unit & unit, Battle::Actions & actions );

    // Only orders a unit can give during its turn, with the expected values
    bool isValidBattleCommand( const BattleCommandData & data );

    // Quick combat becomes auto combat for the local player only
    void sendBattleActions( const Battle::Unit & unit, Battle::Actions & actions );

    // Auto combat can be toggled only for the player's own units
    bool canControlBattleColor( const PlayerColor color );

    // Each computer shows the summary from the side of its own player, as if the other player was not human
    void adjustBattleSummarySides( const PlayerColor attackerColor, const PlayerColor defenderColor, bool & isAttackerHuman, bool & isDefenderHuman );

    // Plays here the battle started by another computer. Returns the next mode of the main loop.
    fheroes2::GameMode followNetworkBattle( const Message & message );

    // Compares with the result sent by the leader
    void checkNetworkBattleEnd( const Message & message );

    // Commanders included. Must match on both computers before a battle during a simultaneous day.
    uint64_t computeArmiesFingerprint( const Army & attackingArmy, const Army & defendingArmy );

    struct NetworkBattleStats
    {
        uint32_t battlesLed{ 0 };
        uint32_t battlesFollowed{ 0 };
        uint32_t ordersSent{ 0 };
        uint32_t ordersReceived{ 0 };
        uint32_t fingerprintMatches{ 0 };
        uint32_t fingerprintMismatches{ 0 };
    };

    const NetworkBattleStats & getBattleStats();
}
