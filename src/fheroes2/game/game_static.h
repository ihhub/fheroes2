/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2GAMESTATIC_H
#define H2GAMESTATIC_H

#include <cstdint>
#include <vector>

namespace MP2
{
    enum MapObjectType : uint16_t;
}

namespace Skill
{
    struct FactionProperties;
    struct SecondarySkillValuesPerLevel;
}

class Heroes;

namespace GameStatic
{
    enum class FogDiscoveryType : int32_t
    {
        CASTLE,
        HEROES,
        OBSERVATION_TOWER,
        MAGI_EYES
    };

    uint32_t GetLostOnWhirlpoolPercent();
    uint32_t GetGameOverLostDays();
    uint32_t getFogDiscoveryDistance( const FogDiscoveryType type );

    uint32_t GetKingdomMaxHeroes();

    uint32_t GetCastleGrownWell();
    uint32_t GetCastleGrownWel2();
    uint32_t GetCastleGrownWeekOf();
    uint32_t GetCastleGrownMonthOf();

    uint32_t GetHeroesRestoreSpellPointsPerDay();

    int32_t ObjectVisitedModifiers( const MP2::MapObjectType objectType );

    int GetBattleMoatReduceDefense();
    // Returns the percentage penalty for the damage dealt by shooters firing at targets protected by castle walls.
    uint32_t getCastleWallRangedPenalty();

    const Skill::FactionProperties * GetFactionProperties( const int race );
    const Skill::SecondarySkillValuesPerLevel * GetSecondarySkillValuesPerLevel( const int skill );

    const std::vector<int32_t> & getSecondarySkillsForWitchsHut();

    uint32_t getMovementPointBonus( const MP2::MapObjectType objectType );

    bool isHeroWorthyToVisitXanadu( const Heroes & hero );
}

#endif
