/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

#include "battle_board.h"
#include "battle_troop.h"

namespace Battle
{
    struct Grave
    {
        explicit Grave( const Unit & unit )
            : uid( unit.GetUID() )
        {}

        uint32_t uid;
    };

    using Graves = std::vector<Grave>;

    class Graveyard : public std::map<int32_t, Graves>
    {
    public:
        Graveyard() = default;
        Graveyard( const Graveyard & ) = delete;

        ~Graveyard() = default;

        Graveyard & operator=( const Graveyard & ) = delete;

        Indexes getOccupiedCells() const;

        void addUnit( const Unit & unit );
        void removeUnit( const Unit & unit );

        std::optional<uint32_t> getUIDOfLastUnit( const int32_t index ) const;

        Graves getGraves( const int32_t index ) const;
    };
}
