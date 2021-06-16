/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "players.h"

namespace Interface
{
    struct PlayerInfo
    {
        PlayerInfo()
            : player( nullptr )
        {}

        Player * player;
        fheroes2::Rect rect1; // opponent
        fheroes2::Rect rect2; // class
        fheroes2::Rect rect3; // change
    };

    struct PlayersInfo : std::vector<PlayerInfo>
    {
        PlayersInfo( bool /* show name */, bool /* show race */, bool /* show swap button */ );

        void UpdateInfo( Players &, const fheroes2::Point & opponents, const fheroes2::Point & classes );
        bool SwapPlayers( Player & player1, Player & player2 ) const;

        Player * GetFromOpponentClick( const fheroes2::Point & pt );
        Player * GetFromOpponentNameClick( const fheroes2::Point & pt );
        Player * GetFromOpponentChangeClick( const fheroes2::Point & pt );
        Player * GetFromClassClick( const fheroes2::Point & pt );

        void RedrawInfo( bool show_play_info = false ) const;
        void resetSelection();
        bool QueueEventProcessing( void );

        bool show_name;
        bool show_race;
        bool show_swap;
        Player * currentSelectedPlayer;
    };
}
