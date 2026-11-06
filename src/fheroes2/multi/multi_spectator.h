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

class Castle;

namespace Battle
{
    class Actions;
    class Arena;
    class Unit;
}

namespace Multi
{
    struct Message;

    // Battles of an ally on the other computer can be watched while waiting, left and joined again (the missed part is
    // replayed without being shown). Nothing is sent back and the changes made here are undone afterwards.

    // Can be called from any waiting loop
    void onSpectatedMessage( const Message & message );
    void processSpectatedMessages();

    bool isSpectatedBattleAvailable();

    // Until the battle ends or the player leaves it
    void watchSpectatedBattle();

    void receiveWatchedActions( Battle::Arena & arena, const Battle::Unit & unit, Battle::Actions & actions );

    // A battle which is already over cannot be played again from a changed world
    void onWorldChangedForSpectator( const bool isGameLoaded );

    // Castle of the local player or of an ally, only to look at, while the other player is playing
    void visitCastle( Castle & castle );

    // Nothing can be done in the castle dialog then
    bool isCastleVisit();

    // Returns the usual status bar text, or why nothing can be done, when the cursor is not over any item
    std::string getCastleStatus( const std::string & status );

    // In self-tests battles are watched as soon as possible, joined in the middle, or left and joined again
    void enableTestSpectate();
    void spectateForTest();

    struct SpectatorStats
    {
        uint32_t battlesOffered{ 0 };
        uint32_t battlesWatched{ 0 };
        uint32_t battlesLeft{ 0 };
        uint32_t battlesWatchedToTheEnd{ 0 };
        uint32_t sameEnds{ 0 };
        uint32_t differentEnds{ 0 };
        uint32_t castlesVisited{ 0 };
    };

    const SpectatorStats & getSpectatorStats();
    std::string describeSpectator();
}
