/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2022                                             *
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

#include <vector>

#include "math_base.h"

class Player;
class Players;

namespace Interface
{
    struct PlayerInfo
    {
        Player * player{ nullptr };
        fheroes2::Rect playerTypeRoi;
        fheroes2::Rect classRoi;
        fheroes2::Rect nameRoi;
        fheroes2::Rect handicapRoi;
    };

    struct PlayersInfo : std::vector<PlayerInfo>
    {
        void UpdateInfo( Players &, const fheroes2::Point & playerTypeOffset, const fheroes2::Point & classOffset );
        bool SwapPlayers( Player & player1, Player & player2 ) const;

        Player * GetFromOpponentClick( const fheroes2::Point & pt );
        Player * GetFromOpponentNameClick( const fheroes2::Point & pt );
        Player * GetFromClassClick( const fheroes2::Point & pt );
        Player * getPlayerFromHandicapRoi( const fheroes2::Point & point );

        // displayInGameInfo: use colored and grayscale race icons to distinguish between players who are still in the game and vanquished players respectively
        void RedrawInfo( const bool displayInGameInfo ) const;

        void resetSelection()
        {
            currentSelectedPlayer = nullptr;
        }

        bool QueueEventProcessing();

        bool readOnlyEventProcessing();

        Player * currentSelectedPlayer{ nullptr };
    };
}
