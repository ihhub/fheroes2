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
#include "game_mode.h"
#include "multi_protocol.h"

class Heroes;

namespace Multi
{
    struct DayEnd;

    // Human players of every computer play their day at the same time. AI players play afterwards, on the host.
    void setSimultaneousTurns( const bool isEnabled );
    bool areSimultaneousTurnsEnabled();

    // Takes effect the next day. The host remembers the setting for the next games.
    void changeSimultaneousTurns( const bool isEnabled );

    // Called by the host when the game starts.
    void sendSimultaneousTurns();

    // Can be called from any waiting loop. The local player is told about a change by showSimultaneousTurnsNotice().
    void onSimultaneousTurnsMessage( const Message & message );
    void showSimultaneousTurnsNotice();

    // In self-tests the guest turns simultaneous turns off on day 3 and on again on day 6.
    void enableTestToggle();
    void toggleForTest();

    // A player tried to attack a player of another computer. The next day is played in classic turns.
    // A guest tells the host at the end of the day.
    void askClassicDay();

    // Called by the host in the morning. Clears the request.
    bool takeClassicDayRequest();

    bool isSimultaneousDay();

    void beginSimultaneousDay();
    void endSimultaneousDay();

    // Host only. The players of every guest have finished their day (gone ones are not waited for).
    bool areOtherDaysDone();

    // If all players of this computer are done, the player waits on the map and can resume their day.
    // Returns false if the turn continues.
    bool confirmEndOfTurn( const PlayerColor color );

    // Returns true on the host once the day can no longer be resumed and AI players are playing.
    bool isDayLocked();

    // Host only. Every guest has finished its turn and sent its last changes of the day (DAY_CLOSED).
    bool areOtherDaysClosed();
    void setOtherDayClosed( const MachineId machine );

    // Host only. The guest came back after a restart. Its players play their day again from the game they are sent.
    void restartOtherDay( const MachineId machine );

    // The player can resume their day with the end of turn button or the R key.
    bool canResumeDay();
    void askDayResume();
    void offerDayResume();

    // Empty unless a battle of an ally can be watched.
    std::string getObserverBannerText();

    // Processes DAY_DONE, DAY_RESUME, DAY_RESUMED, DAY_LOCKED and DAY_CLOSED. Can be called from any waiting loop.
    void onDayReadinessMessage( const Message & message );
    void processDayReadinessMessages();

    // On the host the day starts with its first human player, players of the guests are skipped and AI players wait for the
    // end of every guest's day. On a guest only its own players play.
    bool isTurnSkippedToday( const PlayerColor color );

    // Returns true if the turn goes back to the host at the end of a guest player's turn. Not during a simultaneous day.
    bool isTurnHandedOverAtEnd();

    // Called once every player of the turn loop has palyed.
    fheroes2::GameMode onAllPlayersDone( const fheroes2::GameMode mode );

    // Guest only. Checks at the end of the day that its world is the same as the host's.
    void checkDayEnd( const DayEnd & dayEnd );

    // Visits marked for allies change kingdoms which may be played on another computer.
    // During simultaneous days they are put off until the evening.
    bool isAlliedVisitPutOff( const Heroes & hero, const int32_t tileIndex );

    // Each computer offers only its share of the free heroes, otherwise the same hero could be hired twice.
    bool isHeroForHireHere( const int heroId );

    // For travel spells and spells changing a mine a human player reserves what they touch, the AI does not cast them
    // during simultaneous days.
    bool canCastAdventureSpell( const Heroes & hero, const int spellId );

    struct SimultaneousDayStats
    {
        uint32_t days{ 0 };
        uint32_t classicDays{ 0 };
        uint32_t waitsForOtherDay{ 0 };
        uint32_t dayEndsChecked{ 0 };
        uint32_t dayEndMismatches{ 0 };
    };

    // Returns what differs between the local world and the given one, for the log.
    std::string describeWorldDifferences( const std::vector<uint8_t> & otherWorld );

    const SimultaneousDayStats & getSimultaneousDayStats();
    std::string describeSimultaneousDays();

    // Days that could have been played at the same time according to the contact check. Only a measurement.
    struct SimultaneousStats
    {
        uint32_t daysMeasured{ 0 };

        // Number of days without any possible contact.
        uint32_t simultaneousDays{ 0 };

        // The same count when the day after a contact is also played in classic turns.
        uint32_t simultaneousDaysWithDelay{ 0 };

        uint32_t contactDays{ 0 };
        uint32_t roundLimitDays{ 0 };

        // Days starting with a player who wins or loses (played in classic turns).
        uint32_t conditionDays{ 0 };

        uint32_t alliedOverlapDays{ 0 };
        uint32_t maxMilliseconds{ 0 };
    };

    // Called by the host at the start of every new day of a network game.
    void onNewDay( const bool isLoadedFromSave );

    const SimultaneousStats & getSimultaneousStats();
    void resetSimultaneousStats();

    std::string describeSimultaneousStats();
}
