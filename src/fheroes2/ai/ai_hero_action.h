/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

class Heroes;
class Spell;

namespace AI
{
    void HeroesAction( Heroes & hero, const int32_t dst_index );
    void HeroesMove( Heroes & hero );

    // Makes it so that the 'hero' casts the Dimension Door spell to the 'targetIndex'
    void HeroesCastDimensionDoor( Heroes & hero, const int32_t targetIndex );

    // Makes it so that the 'hero' casts the Summon Boat spell, summoning the boat at the 'boatDestinationIndex'.
    // Returns the index of the tile on which the boat was located before the summoning. It's the caller's
    // responsibility to make sure that 'hero' may cast this spell and there is a summonable boat on the map
    // before calling this function.
    int32_t HeroesCastSummonBoat( Heroes & hero, const int32_t boatDestinationIndex );

    bool HeroesCastAdventureSpell( Heroes & hero, const Spell & spell );
}
