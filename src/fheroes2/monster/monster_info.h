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

#ifndef H2MONSTER_INFO_H
#define H2MONSTER_INFO_H

#include "resource.h"

#include <set>
#include <string>

namespace fheroes2
{

    enum class MonsterAbilityType : int
    {
        NONE,
        DOUBLE_SHOOTING,
        DOUBLE_HEX_SIZE,
        DOUBLE_MELEE_ATTACK,
        DOUBLE_DAMAGE_TO_UNDEAD,
        MAGIC_IMMUNITY,
        IMMUNE_TO_CERTAIN_SPELL,
        SPELL_DAMAGE_REDUCTION,
        SPELL_CASTER,
        HP_REGENERATION,
        DOUBLE_CELL_MELEE_ATTACK,
        FLYING,
        ALWAYS_RETALIATE,
        ALL_AROUND_CELL_MELEE_ATTACK,
        NO_MELEE_PENALTY,
        DRAGON,
        UNDEAD,
        NO_ENEMY_RETALIATION,
        HP_DRAIN,
        AREA_SHOT,
        MORAL_DECREMENT,
        ENEMY_HALFING,
        SOUL_EATER,
        NEUTRAL_MORAL
    };

    enum class MonsterWeaknessType : int
    {
        NONE,
        EXTRA_DAMAGE_FROM_SPELL
    };

    struct MonsterAbility
    {
        MonsterAbility()
            : type( MonsterAbilityType::NONE )
            , percentage( 0 )
            , value( 0 )
        {
        }

        explicit MonsterAbility( const MonsterAbilityType type_ )
            : type( type_ )
            , percentage( 0 )
            , value( 0 )
        {}

        explicit MonsterAbility( const MonsterAbilityType type_, const uint32_t percentage_, const uint32_t value_ )
            : type( type_ )
            , percentage( percentage_ )
            , value( value_ )
        {}

        bool operator <( const MonsterAbility & another ) const
        {
            return type < another.type;
        }

        MonsterAbilityType type;

        uint32_t percentage;

        uint32_t value;
    };

    struct MonsterWeakness
    {
        MonsterWeakness()
            : type( MonsterWeaknessType::NONE )
            , percentage( 0 )
            , value( 0 )
        {
        }

        explicit MonsterWeakness( const MonsterWeaknessType type_ )
            : type( type_ )
            , percentage( 0 )
            , value( 0 )
        {}

        explicit MonsterWeakness( const MonsterWeaknessType type_, const uint32_t percentage_, const uint32_t value_ )
            : type( type_ )
            , percentage( percentage_ )
            , value( value_ )
        {}

        bool operator <( const MonsterWeakness & another ) const
        {
            return type < another.type;
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

        std::set<MonsterAbility> abilities;
        std::set<MonsterWeakness> weaknesses;
    };

    struct MonsterGeneralStats
    {
        const char * name;
        const char * multiName;

        uint32_t baseGrowth;
        uint32_t race;
        uint32_t level;

        cost_t cost;
    };

    struct MonsterSound
    {
        int attack;
        int death;
        int movement;
        int wince;
    };

    struct MonsterData
    {
        MonsterData() = delete;

        MonsterData( const int icnId_, const MonsterSound & sounds_, const MonsterBattleStats & battleStats_, const MonsterGeneralStats & generalStats_ )
            : icnId( icnId_ )
            , sounds( sounds_ )
            , battleStats( battleStats_ )
            , generalStats( generalStats_ )
        {}

        int icnId;

        MonsterSound sounds;

        MonsterBattleStats battleStats;

        MonsterGeneralStats generalStats;
    };

    const MonsterData & getMonsterData( const int monsterId );

    std::string getMonsterAbilityDescription( const MonsterAbility & ability );
    std::string getMonsterWeaknessDescription( const MonsterWeakness & weakness );

    std::string getMonsterDescription( const int monsterId );
}
#endif
