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
#include <optional>

#include "color.h"
#include "game_mode.h"
#include "multi_protocol.h"

namespace Multi
{
    // checkGameOver() runs on the computer which plays the turn. Each computer shows the end of the game to its own
    // players, so every victory, defeat or vanquished player is sent to the other ones.

    // Always true outside network games, never in self-tests
    bool isResultShownHere( const PlayerColor color );

    // Nothing should wait for a click in self-tests
    bool isGameEndShown();

    bool hasPlayerInGame( const PlayerColorsSet colors );

    // The game may still go on
    void onPlayerVanquished( const PlayerColor color );

    // isGameOver: the game ends for everyone
    void onPlayerLoss( const PlayerColor color, const uint32_t result, const bool isGameOver );

    // Returns a mode if the usual victory is not shown here (winner on another computer, or self-tests).
    std::optional<fheroes2::GameMode> onPlayerVictory( const PlayerColor color, const uint32_t result );

    fheroes2::GameMode showGameResult( const Message & message );

    // No player left here: a guest leaves, the host gives the game to the last computer with players.
    fheroes2::GameMode leaveWithoutPlayer();

    // The host has no player left but goes on hosting the game for the other computers.
    void keepHostingWithoutPlayer();

    // No other computer has players left, or the host handed the game over: the game goes on here alone.
    void goOnAlone();

    struct GameOverStats
    {
        uint32_t resultsSent{ 0 };
        uint32_t resultsReceived{ 0 };
        bool isGameOver{ false };
        // No player left here
        bool isOutOfGame{ false };
        // No player left on the other computer
        bool isPlayingAlone{ false };
    };

    const GameOverStats & getGameOverStats();
}
