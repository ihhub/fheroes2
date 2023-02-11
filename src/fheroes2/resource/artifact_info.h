/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2023                                             *
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

// TODO: this header is redundant here, but detected as required by IWYU with older compilers
// IWYU pragma: no_include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace fheroes2
{
    enum class ArtifactBonusType : int32_t
    {
        NONE,

        // These bonuses are cumulative means each copy of an artifact having it will add to the total value.
        KNOWLEDGE_SKILL,
        ATTACK_SKILL,
        DEFENCE_SKILL,
        SPELL_POWER_SKILL,
        GOLD_INCOME,
        WOOD_INCOME,
        MERCURY_INCOME,
        ORE_INCOME,
        SULFUR_INCOME,
        CRYSTAL_INCOME,
        GEMS_INCOME,

        // These bonuses are unique per artifact type but cumulative across multiple artifact types.
        MORALE,
        LUCK,
        SEA_BATTLE_MORALE_BOOST,
        SEA_BATTLE_LUCK_BOOST,
        LAND_MOBILITY,
        SEA_MOBILITY,
        SPELL_POINTS_DAILY_GENERATION,
        EVERY_COMBAT_SPELL_DURATION,
        EXTRA_CATAPULT_SHOTS,
        AREA_REVEAL_DISTANCE,
        ADD_SPELL,
        NECROMANCY_SKILL,

        // These bonuses are unique per artifact type but the effect is multiplied with different artifact types.
        SURRENDER_COST_REDUCTION_PERCENT,

        CURSE_SPELL_COST_REDUCTION_PERCENT,
        BLESS_SPELL_COST_REDUCTION_PERCENT,
        SUMMONING_SPELL_COST_REDUCTION_PERCENT,
        MIND_INFLUENCE_SPELL_COST_REDUCTION_PERCENT,

        COLD_SPELL_DAMAGE_REDUCTION_PERCENT,
        FIRE_SPELL_DAMAGE_REDUCTION_PERCENT,
        LIGHTNING_SPELL_DAMAGE_REDUCTION_PERCENT,
        ELEMENTAL_SPELL_DAMAGE_REDUCTION_PERCENT,

        HYPNOTIZE_SPELL_EXTRA_EFFECTIVENESS_PERCENT,
        COLD_SPELL_EXTRA_EFFECTIVENESS_PERCENT,
        FIRE_SPELL_EXTRA_EFFECTIVENESS_PERCENT,
        LIGHTNING_SPELL_EXTRA_EFFECTIVENESS_PERCENT,
        RESURRECT_SPELL_EXTRA_EFFECTIVENESS_PERCENT,
        SUMMONING_SPELL_EXTRA_EFFECTIVENESS_PERCENT,

        // All bonuses below are unique per hero. They do not require any extra values. Only one artifact with such bonus will be used.
        CURSE_SPELL_IMMUNITY,
        HYPNOTIZE_SPELL_IMMUNITY,
        DEATH_SPELL_IMMUNITY,
        BERSERK_SPELL_IMMUNITY,
        BLIND_SPELL_IMMUNITY,
        PARALYZE_SPELL_IMMUNITY,
        HOLY_SPELL_IMMUNITY,
        DISPEL_SPELL_IMMUNITY,

        ENDLESS_AMMUNITION,
        NO_SHOOTING_PENALTY,
        VIEW_MONSTER_INFORMATION,
        DISABLE_ALL_SPELL_COMBAT_CASTING,
        MAXIMUM_MORALE,
        MAXIMUM_LUCK
    };

    enum class ArtifactCurseType : int32_t
    {
        // These curses are cumulative means each copy of an artifact having it will add to the total value.
        GOLD_PENALTY,
        SPELL_POWER_SKILL,

        // These curses are unique per artifact type but cumulative across multiple artifact types.
        MORALE,

        // These curses are unique per artifact type but the effect is multiplied with different artifact types.
        FIRE_SPELL_EXTRA_DAMAGE_PERCENT,
        COLD_SPELL_EXTRA_DAMAGE_PERCENT,

        // All curses below are unique per hero. They do not require any extra values. Only one artifact with such curse will be used.
        NO_JOINING_ARMIES,
        UNDEAD_MORALE_PENALTY
    };

    struct ArtifactBonus
    {
        explicit ArtifactBonus( const ArtifactBonusType type_ )
            : type( type_ )
            , value( 0 )
        {
            // Do nothing.
        }

        ArtifactBonus( const ArtifactBonusType type_, const int32_t value_ )
            : type( type_ )
            , value( value_ )
        {
            // Do nothing.
        }

        bool operator==( const ArtifactBonus & another ) const
        {
            return type == another.type;
        }

        ArtifactBonusType type;

        int32_t value;
    };

    struct ArtifactCurse
    {
        explicit ArtifactCurse( const ArtifactCurseType type_ )
            : type( type_ )
            , value( 0 )
        {
            // Do nothing.
        }

        ArtifactCurse( const ArtifactCurseType type_, const int32_t value_ )
            : type( type_ )
            , value( value_ )
        {
            // Do nothing.
        }

        bool operator==( const ArtifactCurse & another ) const
        {
            return type == another.type;
        }

        ArtifactCurseType type;

        int32_t value;
    };

    // This is the only bonus type which is cumulative even for several copies of the same artifact.
    bool isBonusCumulative( const ArtifactBonusType bonus );

    bool isBonusMultiplied( const ArtifactBonusType bonus );

    bool isBonusUnique( const ArtifactBonusType bonus );

    // This is the only curse type which is cumulative even for several copies of the same artifact.
    bool isCurseCumulative( const ArtifactCurseType curse );

    bool isCurseMultiplied( const ArtifactCurseType curse );

    bool isCurseUnique( const ArtifactCurseType curse );

    struct ArtifactData
    {
        const char * name;

        // Do not use this member directly. Use getDescription() method.
        const char * baseDescription;

        const char * discoveryEventDescription;

        std::vector<ArtifactBonus> bonuses;
        std::vector<ArtifactCurse> curses;

        std::string getDescription( const int extraParameter ) const;
    };

    const ArtifactData & getArtifactData( const int artifactId );

    std::string getArtifactDescription( const int artifactId );
}
