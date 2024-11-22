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

#pragma once

#include <cstdint>

#include "monster_info.h"
#include "resource.h"

class Spell;

class Monster
{
public:
    enum
    {
        JOIN_CONDITION_UNSET = -1,
        JOIN_CONDITION_SKIP = 0,
        JOIN_CONDITION_MONEY = 1,
        JOIN_CONDITION_FREE = 2
    };

    enum class LevelType : int
    {
        LEVEL_ANY = 0,
        LEVEL_1,
        LEVEL_2,
        LEVEL_3,
        LEVEL_4
    };

    enum MonsterType : int32_t
    {
        UNKNOWN,

        PEASANT,
        ARCHER,
        RANGER,
        PIKEMAN,
        VETERAN_PIKEMAN,
        SWORDSMAN,
        MASTER_SWORDSMAN,
        CAVALRY,
        CHAMPION,
        PALADIN,
        CRUSADER,
        GOBLIN,
        ORC,
        ORC_CHIEF,
        WOLF,
        OGRE,
        OGRE_LORD,
        TROLL,
        WAR_TROLL,
        CYCLOPS,
        SPRITE,
        DWARF,
        BATTLE_DWARF,
        ELF,
        GRAND_ELF,
        DRUID,
        GREATER_DRUID,
        UNICORN,
        PHOENIX,
        CENTAUR,
        GARGOYLE,
        GRIFFIN,
        MINOTAUR,
        MINOTAUR_KING,
        HYDRA,
        GREEN_DRAGON,
        RED_DRAGON,
        BLACK_DRAGON,
        HALFLING,
        BOAR,
        IRON_GOLEM,
        STEEL_GOLEM,
        ROC,
        MAGE,
        ARCHMAGE,
        GIANT,
        TITAN,
        SKELETON,
        ZOMBIE,
        MUTANT_ZOMBIE,
        MUMMY,
        ROYAL_MUMMY,
        VAMPIRE,
        VAMPIRE_LORD,
        LICH,
        POWER_LICH,
        BONE_DRAGON,

        ROGUE,
        NOMAD,
        GHOST,
        GENIE,
        MEDUSA,
        EARTH_ELEMENT,
        AIR_ELEMENT,
        FIRE_ELEMENT,
        WATER_ELEMENT,

        // Editor-related monsters.
        RANDOM_MONSTER,
        RANDOM_MONSTER_LEVEL_1,
        RANDOM_MONSTER_LEVEL_2,
        RANDOM_MONSTER_LEVEL_3,
        RANDOM_MONSTER_LEVEL_4,

        // IMPORTANT! Put all new monsters just above this line.
        MONSTER_COUNT
    };

    Monster( const int m = UNKNOWN )
        : id( m )
    {
        // Do nothing.
    }

    explicit Monster( const Spell & sp );
    Monster( const int race, const uint32_t dw );

    Monster( const Monster & ) = default;
    Monster( Monster && ) = default;

    virtual ~Monster() = default;

    Monster & operator=( const Monster & ) = default;
    Monster & operator=( Monster && ) = default;

    bool operator==( const Monster & m ) const
    {
        return id == m.id;
    }

    bool operator!=( const Monster & m ) const
    {
        return id != m.id;
    }

    int GetID() const
    {
        return id;
    }

    void Upgrade()
    {
        id = GetUpgrade().id;
    }

    Monster GetUpgrade() const;
    Monster GetDowngrade() const;

    virtual uint32_t GetAttack() const;
    virtual uint32_t GetDefense() const;
    virtual int GetMorale() const;
    virtual int GetLuck() const;
    virtual int GetRace() const;

    uint32_t GetDamageMin() const
    {
        return fheroes2::getMonsterData( id ).battleStats.damageMin;
    }

    uint32_t GetDamageMax() const
    {
        return fheroes2::getMonsterData( id ).battleStats.damageMax;
    }

    virtual uint32_t GetShots() const;

    uint32_t GetHitPoints() const
    {
        return fheroes2::getMonsterData( id ).battleStats.hp;
    }

    uint32_t GetSpeed() const
    {
        return fheroes2::getMonsterData( id ).battleStats.speed;
    }

    uint32_t GetGrown() const
    {
        return fheroes2::getMonsterData( id ).generalStats.baseGrowth;
    }

    int GetMonsterLevel() const
    {
        return fheroes2::getMonsterData( id ).generalStats.level;
    }

    LevelType GetRandomUnitLevel() const;
    uint32_t GetRNDSize() const;

    const char * GetName() const;
    const char * GetMultiName() const;
    const char * GetPluralName( uint32_t ) const;
    static const char * getRandomRaceMonstersName( const uint32_t building );

    bool isValid() const
    {
        return id != UNKNOWN && id < MONSTER_COUNT && !isRandomMonster();
    }

    bool isRandomMonster() const
    {
        return ( id >= RANDOM_MONSTER && id <= RANDOM_MONSTER_LEVEL_4 );
    }

    bool isElemental() const
    {
        return isAbilityPresent( fheroes2::MonsterAbilityType::ELEMENTAL );
    }

    bool isUndead() const
    {
        return isAbilityPresent( fheroes2::MonsterAbilityType::UNDEAD );
    }

    bool isFlying() const
    {
        return isAbilityPresent( fheroes2::MonsterAbilityType::FLYING );
    }

    bool isWide() const
    {
        return isAbilityPresent( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
    }

    bool isArchers() const
    {
        return GetShots() > 0;
    }

    bool isAllowUpgrade() const
    {
        return id != GetUpgrade().id;
    }

    bool isRegenerating() const
    {
        return isAbilityPresent( fheroes2::MonsterAbilityType::HP_REGENERATION );
    }

    bool isDoubleCellAttack() const
    {
        return isAbilityPresent( fheroes2::MonsterAbilityType::TWO_CELL_MELEE_ATTACK );
    }

    bool isAllAdjacentCellsAttack() const
    {
        return isAbilityPresent( fheroes2::MonsterAbilityType::ALL_ADJACENT_CELL_MELEE_ATTACK );
    }

    bool isIgnoringRetaliation() const
    {
        return isAbilityPresent( fheroes2::MonsterAbilityType::NO_ENEMY_RETALIATION );
    }

    bool isDragons() const
    {
        return isAbilityPresent( fheroes2::MonsterAbilityType::DRAGON );
    }

    bool isAffectedByMorale() const
    {
        // TODO: possible optimization: run through all abilities once.
        return !( isUndead() || isElemental() );
    }

    bool isAbilityPresent( const fheroes2::MonsterAbilityType abilityType ) const;

    double GetMonsterStrength( int attack = -1, int defense = -1 ) const;

    int ICNMonh() const;

    uint32_t GetSpriteIndex() const
    {
        return UNKNOWN < id ? id - 1 : 0;
    }

    Funds GetCost() const
    {
        return Funds( fheroes2::getMonsterData( id ).generalStats.cost );
    }

    uint32_t GetDwelling() const;

    int GetMonsterSprite() const
    {
        return fheroes2::getMonsterData( id ).icnId;
    }

    static Monster Rand( const LevelType type );

    static uint32_t GetCountFromHitPoints( const Monster & mons, const uint32_t hp );

    static uint32_t GetMissileICN( uint32_t monsterID );

protected:
    // Returns the cost of an upgrade if a monster has an upgrade. Otherwise returns no resources.
    Funds GetUpgradeCost() const;

    static Monster FromDwelling( int race, uint32_t dw );

    int id;
};
