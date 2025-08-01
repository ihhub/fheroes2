/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2025                                             *
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
#include <string>
#include <utility>
#include <vector>

#include "resource.h"

namespace fheroes2
{
    // Spell power value, based on which the effect of the monsters' built-in spells is calculated
    inline constexpr int spellPowerForBuiltinMonsterSpells{ 3 };

    enum class MonsterAbilityType : int
    {
        // Basic abilities (usually not shown in the unit description).
        NONE,
        DOUBLE_HEX_SIZE,
        FLYING,
        DRAGON,
        EARTH_CREATURE,
        AIR_CREATURE,
        FIRE_CREATURE,
        WATER_CREATURE,
        // Advanced abilities (shown in the unit description).
        UNDEAD,
        ELEMENTAL,
        DOUBLE_SHOOTING,
        DOUBLE_MELEE_ATTACK,
        DOUBLE_DAMAGE_TO_UNDEAD,
        MAGIC_RESISTANCE,
        MIND_SPELL_IMMUNITY,
        ELEMENTAL_SPELL_IMMUNITY,
        FIRE_SPELL_IMMUNITY,
        COLD_SPELL_IMMUNITY,
        IMMUNE_TO_CERTAIN_SPELL,
        ELEMENTAL_SPELL_DAMAGE_REDUCTION,
        CERTAIN_SPELL_DAMAGE_REDUCTION,
        SPELL_CASTER,
        HP_REGENERATION,
        TWO_CELL_MELEE_ATTACK,
        UNLIMITED_RETALIATION,
        ALL_ADJACENT_CELL_MELEE_ATTACK,
        NO_MELEE_PENALTY,
        NO_ENEMY_RETALIATION,
        HP_DRAIN,
        AREA_SHOT,
        MORAL_DECREMENT,
        ENEMY_HALVING,
        SOUL_EATER
    };

    enum class MonsterWeaknessType : int
    {
        // Basic weaknesses (usually not shown in the unit description).
        NONE,
        DOUBLE_DAMAGE_FROM_EARTH_CREATURES,
        DOUBLE_DAMAGE_FROM_AIR_CREATURES,
        DOUBLE_DAMAGE_FROM_FIRE_CREATURES,
        DOUBLE_DAMAGE_FROM_WATER_CREATURES,
        // Advanced weaknesses (shown in the unit description).
        DOUBLE_DAMAGE_FROM_FIRE_SPELLS,
        DOUBLE_DAMAGE_FROM_COLD_SPELLS,
        EXTRA_DAMAGE_FROM_CERTAIN_SPELL
    };

    struct MonsterAbility
    {
        explicit MonsterAbility( const MonsterAbilityType type_ )
            : type( type_ )
            , percentage( 0 )
            , value( 0 )
        {
            // Do nothing.
        }

        MonsterAbility( const MonsterAbilityType type_, const uint32_t percentage_, const uint32_t value_ )
            : type( type_ )
            , percentage( percentage_ )
            , value( value_ )
        {
            // Do nothing.
        }

        bool operator==( const MonsterAbilityType anotherType ) const
        {
            return type == anotherType;
        }

        bool operator==( const std::pair<MonsterAbilityType, uint32_t> & typeValuePair ) const
        {
            return type == typeValuePair.first && value == typeValuePair.second;
        }

        MonsterAbilityType type;

        uint32_t percentage;

        uint32_t value;
    };

    struct MonsterWeakness
    {
        explicit MonsterWeakness( const MonsterWeaknessType type_ )
            : type( type_ )
            , percentage( 0 )
            , value( 0 )
        {
            // Do nothing.
        }

        explicit MonsterWeakness( const MonsterWeaknessType type_, const uint32_t percentage_, const uint32_t value_ )
            : type( type_ )
            , percentage( percentage_ )
            , value( value_ )
        {
            // Do nothing.
        }

        bool operator<( const MonsterWeakness & another ) const
        {
            return type < another.type || ( type == another.type && value < another.value );
        }

        bool operator==( const MonsterWeaknessType anotherType ) const
        {
            return type == anotherType;
        }

        bool operator==( const std::pair<MonsterWeaknessType, uint32_t> & typeValuePair ) const
        {
            return type == typeValuePair.first && value == typeValuePair.second;
        }

        MonsterWeaknessType type;

        uint32_t percentage;

        uint32_t value;
    };

    struct MonsterBattleStats
    {
        uint32_t attack;
        uint32_t defense;
        uint32_t damageMin;
        uint32_t damageMax;
        uint32_t hp;
        uint32_t speed;
        uint32_t shots;

        double monsterBaseStrength;

        std::vector<MonsterAbility> abilities;
        std::vector<MonsterWeakness> weaknesses;
    };

    struct MonsterGeneralStats
    {
        const char * untranslatedName;
        const char * untranslatedPluralName;

        uint32_t baseGrowth;
        uint32_t race;
        uint32_t level;

        Cost cost;
    };

    struct MonsterSound
    {
        int meleeAttack;
        int death;
        int movement;
        int wince;
        int rangeAttack;
        int takeoff;
        int landing;
        int explosion;
    };

    struct MonsterData
    {
        MonsterData( const int icnId_, const char * binFileName_, const MonsterSound & sounds_, const MonsterBattleStats & battleStats_,
                     const MonsterGeneralStats & generalStats_ )
            : icnId( icnId_ )
            , binFileName( binFileName_ )
            , sounds( sounds_ )
            , battleStats( battleStats_ )
            , generalStats( generalStats_ )
        {
            // Do nothing.
        }

        int icnId;

        const char * binFileName;

        MonsterSound sounds;

        MonsterBattleStats battleStats;

        MonsterGeneralStats generalStats;
    };

    const MonsterData & getMonsterData( const int monsterId );

    std::string getMonsterAbilityDescription( const MonsterAbility & ability, const bool ignoreBasicAbilities );
    std::string getMonsterWeaknessDescription( const MonsterWeakness & weakness, const bool ignoreBasicWeaknesses );

    std::string getMonsterDescription( const int monsterId ); // To be utilized in future.

    std::vector<std::string> getMonsterPropertiesDescription( const int monsterId );

    uint32_t getSpellResistance( const int monsterId, const int spellId );

    bool isAbilityPresent( const std::vector<MonsterAbility> & abilities, const MonsterAbilityType abilityType );
    bool isWeaknessPresent( const std::vector<MonsterWeakness> & weaknesses, const MonsterWeaknessType weaknessType );
}
