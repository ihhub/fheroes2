/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2GAMEOVER_H
#define H2GAMEOVER_H

#include <cstdint>
#include <string>

#include "game_mode.h"

class IStreamBase;
class OStreamBase;

namespace GameOver
{
    enum : uint32_t
    {
        COND_NONE = 0x00000000,

        WINS_ALL = 0x00000001,
        WINS_TOWN = 0x00000002,
        WINS_HERO = 0x00000004,
        WINS_ARTIFACT = 0x00000008,
        WINS_SIDE = 0x00000010,
        WINS_GOLD = 0x00000020,

        WINS = WINS_ALL | WINS_TOWN | WINS_HERO | WINS_ARTIFACT | WINS_SIDE | WINS_GOLD,

        LOSS_ALL = 0x00000100,
        LOSS_TOWN = 0x00000200,
        LOSS_HERO = 0x00000400,
        LOSS_TIME = 0x00000800,
        // These loss conditions apply if the enemy player won because of the corresponding win condition
        LOSS_ENEMY_WINS_TOWN = 0x00010000,
        LOSS_ENEMY_WINS_GOLD = 0x00020000,

        LOSS = LOSS_ALL | LOSS_TOWN | LOSS_HERO | LOSS_TIME | LOSS_ENEMY_WINS_TOWN | LOSS_ENEMY_WINS_GOLD,
        LOSS_ENEMY_WINS = LOSS_ENEMY_WINS_TOWN | LOSS_ENEMY_WINS_GOLD
    };

    const char * GetString( uint32_t cond );
    std::string GetActualDescription( uint32_t cond );

    class Result
    {
    public:
        static Result & Get();

        void Reset(); // Resets everything

        // Reset game result to COND_NONE.
        void ResetResult()
        {
            result = GameOver::COND_NONE;
        }

        uint32_t GetResult() const
        {
            return result;
        }

        fheroes2::GameMode checkGameOver();

    private:
        friend OStreamBase & operator<<( OStreamBase & stream, const Result & res );
        friend IStreamBase & operator>>( IStreamBase & stream, Result & res );

        Result();

        int colors;
        uint32_t result;
    };

    OStreamBase & operator<<( OStreamBase & stream, const Result & res );
    IStreamBase & operator>>( IStreamBase & stream, Result & res );
}

#endif
