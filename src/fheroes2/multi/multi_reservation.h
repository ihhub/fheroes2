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
#include <vector>

#include "color.h"
#include "multi_contact.h"

class Heroes;

namespace Multi
{
    // What an action may change. It is reserved with the host before the action and held until the end of the day.
    struct ReservationZone
    {
        // No fog of war, it only clears during a day and is merged separately.
        TileSet tiles;
        std::vector<int32_t> tileList;

        // Players with a hero or a castle next to the hero (possible battle). Everything they captured is reserved too
        // if the battle could vanquish them.
        PlayerColorsSet kingdoms{ 0 };

        // A battle with a human player of another computer is arranged with that computer, not reserved.
        bool isBattleWithOtherComputer{ false };
    };

    // Returns the zone of a step of the hero to the next tile.
    ReservationZone getStepZone( const Heroes & hero, const int32_t targetIndex, const PlayerColorsSet otherComputerColors );

    // Returns true if the step attacks a hero or a castle of a non allied player of another computer.
    bool attacksOtherComputer( const Heroes & hero, const int32_t targetIndex, const PlayerColorsSet otherComputerColors );

    struct ReservationStats
    {
        uint32_t stepsReserved{ 0 };
        uint32_t stepsRefused{ 0 };
        uint32_t battlesRefused{ 0 };

        // Guest only.
        uint32_t requestsSent{ 0 };
        uint64_t totalWaitMilliseconds{ 0 };
        uint32_t maxWaitMilliseconds{ 0 };

        // Host only, for the requests of the guests.
        uint32_t requestsGranted{ 0 };
        uint32_t requestsRefused{ 0 };

        // Jumps like Dimension Door made in self-tests.
        uint32_t testJumps{ 0 };
        uint32_t testJumpsRefused{ 0 };
    };

    // Self-tests can also enable reservations during classic turns.
    void enableReservations();
    bool areReservationsEnabled();

    // Each computer holds the castles of its players for the day. It builds there and buys boats.
    void reserveCastlesForTheDay();

    // Reserves what the next step touches. Returns false if another computer got there first, and the hero stops.
    bool onHeroMove( Heroes & hero, const bool isJump );

    // Reserves the area around the hero and the target of a travel spell or a spell on a mine.
    // Returns false if refused. The spell is then not cast.
    bool reserveSpellTarget( const Heroes & hero, const int32_t targetIndex );

    // In self-tests the first hero of the player jumps a few tiles away like with Dimension Door.
    void enableTestJumps();
    void jumpForTest( const PlayerColor color );

    // Called by the host whenever events are processed.
    void answerReservations();

    const ReservationStats & getReservationStats();
    std::string describeReservations();
}
