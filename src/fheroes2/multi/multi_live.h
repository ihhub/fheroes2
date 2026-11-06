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
#include "multi_world_changes.h"

class Heroes;

namespace Multi
{
    struct Message;

    // In the live view each computer sends the world units changed by what is played to every other one, which applies them.

    struct LiveStats
    {
        uint32_t updatesSent{ 0 };
        uint64_t bytesSent{ 0 };
        uint32_t incompleteSent{ 0 };

        uint32_t updatesApplied{ 0 };
        uint32_t updatesIgnored{ 0 };
        uint32_t updatesRefused{ 0 };
        uint32_t hashMismatches{ 0 };
    };

    // Called when a whole game was sent or loaded. Its sender and receivers then have the same world.
    void resetLiveBaseline();

    // Called before and after each step of a hero of any player.
    void onHeroStep( const Heroes & hero, const int32_t targetIndex );
    void onHeroStepDone( const Heroes & hero );

    // Called at the end of the turn of any player.
    void onTurnEnd( const PlayerColor color );

    // Sends what changed since the last update to every other computer.
    void sendLiveUpdate( const Heroes * hero = nullptr );

    // Returns false if the update could not be applied. Later updates are then ignored until the next whole game.
    bool applyLiveUpdate( const Message & message );

    // Resets the focus on a hero or a castle lost by an update or missing from a received game.
    // Saving a game with such a focus would crash.
    void resetLostFocus();

    // Undoes later what is not part of the game, like a watched battle or a castle visited while the remote player plays.
    class WorldRollback
    {
    public:
        WorldRollback();

        // Returns false if it could not be done.
        bool restore();

    private:
        WorldSnapshot _before;
        bool _isValid{ false };
    };

    // Returns the hash of the local world as it is written in save files.
    uint64_t computeWorldHash();

    // Leaves out the routes of the heroes. Each computer plans the routes of its own heroes.
    uint64_t computeWorldHashWithoutRoutes();

    const LiveStats & getLiveStats();
    std::string describeLiveStats();
}
