/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2BATTLE_CATAPULT_H
#define H2BATTLE_CATAPULT_H

#include <cstdint>
#include <map>
#include <vector>

#include "battle.h"
#include "math_base.h"

class HeroBase;

namespace Rand
{
    class DeterministicRandomGenerator;
}

namespace Battle
{
    class Catapult
    {
    public:
        explicit Catapult( const HeroBase & hero );
        Catapult( const Catapult & ) = delete;

        Catapult & operator=( const Catapult & ) = delete;

        static const std::vector<CastleDefenseStructure> & getAllowedTargets();
        static CastleDefenseStructure GetTarget( const std::map<CastleDefenseStructure, int> & stateOfCatapultTargets,
                                                 Rand::DeterministicRandomGenerator & randomGenerator );
        static fheroes2::Point GetTargetPosition( const CastleDefenseStructure target, const bool hit );

        uint32_t GetShots() const
        {
            return catShots;
        }

        int GetDamage( Rand::DeterministicRandomGenerator & randomGenerator ) const;
        bool IsNextShotHit( Rand::DeterministicRandomGenerator & randomGenerator ) const;

    private:
        uint32_t catShots;
        uint32_t doubleDamageChance;
        bool canMiss;
    };
}

#endif
