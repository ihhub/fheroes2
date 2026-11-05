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
    // Self-tests only. Changes of each step must be inside the zone it would have reserved, changes between two steps
    // (castles, tavern) must belong to the player. The game is not changed.
    struct ActionWatchStats
    {
        uint32_t checks{ 0 };
        uint64_t totalMicroseconds{ 0 };
        uint32_t maxMicroseconds{ 0 };

        uint32_t steps{ 0 };

        // Changed units of one step, as they would be sent
        uint64_t totalChangedBytes{ 0 };
        uint32_t maxChangedBytes{ 0 };

        // Steps refused for a battle with a human player of the other computer
        uint32_t battlesWithOtherComputer{ 0 };

        // The computer player can be defeated
        uint32_t computerPlayerReservations{ 0 };

        // Free heroes offered for hire in a tavern between two steps
        uint32_t tavernChanges{ 0 };

        // Travel spells, reserved at their destination
        uint32_t spellMoves{ 0 };

        // Changes outside of the zone which would have been reserved
        uint32_t holes{ 0 };
    };

    void enableActionWatch();
    bool isActionWatchEnabled();

    void watchTurnBegin( const PlayerColor color );
    void watchTurnEnd( const PlayerColor color );

    void watchHeroStep( const Heroes & hero, const int32_t targetIndex );
    void watchHeroStepDone( const Heroes & hero );

    // Changes of the other computer applied in the middle of a turn (after a battle with it)
    void watchRebaseline();

    const ActionWatchStats & getActionWatchStats();
    std::string describeActionWatch();
}
