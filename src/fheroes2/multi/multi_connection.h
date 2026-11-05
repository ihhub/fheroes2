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

namespace Multi
{
    enum class WaitKind
    {
        // Turn or day of the other player. If the other computer joins again, it gets the game and the wait goes on.
        MAP,
        // Answer or battle orders. If the other computer joins again, the answer will never come and the AI plays instead.
        EXCHANGE
    };

    // Called by every loop waiting for the other computer. Returns false once the network game is over.
    // A short cut is not shown, a longer one pauses the game with a window.
    bool keepConnection( const WaitKind kind = WaitKind::EXCHANGE );

    // Shows the window of a lost connection. Used by self-tests only.
    void showReconnectionWindowForTest( const bool isHostSide, const bool isRetrying );

    // The host of a self-test shows the window of the absent players, as for a player, instead of going on by itself.
    void enableTestAbsenceWindow();
}
