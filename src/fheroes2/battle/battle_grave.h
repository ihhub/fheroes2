/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2BATTLE_GRAVEYARD_H
#define H2BATTLE_GRAVEYARD_H

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

#include "battle_board.h"

namespace Battle
{
    class Unit;

    using TroopUIDs = std::vector<uint32_t>;

    class Graveyard : public std::map<int32_t, TroopUIDs>
    {
    public:
        Graveyard() = default;
        Graveyard( const Graveyard & ) = delete;

        ~Graveyard() = default;

        Graveyard & operator=( const Graveyard & ) = delete;

        Indexes GetClosedCells() const;
        void AddTroop( const Unit & );
        void RemoveTroop( const Unit & );
        uint32_t GetLastTroopUID( int32_t ) const;
        TroopUIDs GetTroopUIDs( const int32_t hexIndex ) const;
    };
}

#endif
