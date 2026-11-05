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
#include <string>

#include "color.h"

class Heroes;

namespace Multi
{
    // Battles between players of both computers during simultaneous days. The remote player is stopped at a safe moment,
    // the battle is played live and they continue once its changes are applied.

    // Called before the step of the attack. Returns false if both playres attack each other at once or the other one left.
    bool requestLiveBattle( Heroes & hero, const int32_t targetIndex );

    // Returns true if the battle about to start was agreed with the other computer. No game is sent then.
    bool isLiveBattleAgreed();

    // Called after the step of the attack and at the end of each turn. The remote player continues once the changes are sent.
    void finishLiveBattle();

    // Called on the computer of the defender when its player can be stopped.
    void answerBattleRequests();

    // In self-tests a hero jumps next to a hero of the other computer and attacks it. Day 1 of 3 the host attacks, day 2
    // both attack at once (neither battle is played), day 3 the guest.
    void enableTestAttacks();
    void attackForTest( const PlayerColor color );

    struct LiveBattleStats
    {
        // On the computer of the attacker.
        uint32_t requestsSent{ 0 };
        uint32_t requestsAccepted{ 0 };
        uint32_t requestsRefused{ 0 };
        uint32_t requestsGivenUp{ 0 };

        // On the computer of the defender.
        uint32_t requestsAnswered{ 0 };
        uint32_t requestsRefusedHere{ 0 };

        // Self-tests only.
        uint32_t testAttacks{ 0 };
    };

    const LiveBattleStats & getLiveBattleStats();
    std::string describeLiveBattles();
}
