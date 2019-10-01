/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#ifndef H2MONSTER_H
#define H2MONSTER_H

#include <string>
#include "payment.h"
#include "gamedefs.h"

class Spell;

class Monster
{
public:
    enum { JOIN_CONDITION_SKIP  = 0, JOIN_CONDITION_MONEY = 1, JOIN_CONDITION_FREE  = 2, JOIN_CONDITION_FORCE = 3 };

    enum level_t { LEVEL0, LEVEL1, LEVEL2, LEVEL3, LEVEL4 };

    enum monster_t
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

	MONSTER_RND1,
	MONSTER_RND2,
	MONSTER_RND3,
	MONSTER_RND4,
	MONSTER_RND
    };

    static std::map<monster_t, level_t> monsterLevel;

    Monster(int = UNKNOWN);
    Monster(const Spell &);
    Monster(int race, u32 dw);
    virtual ~Monster(){}

    bool operator< (const Monster &) const;
    bool operator== (const Monster &) const;
    bool operator!= (const Monster &) const;

    int operator() (void) const;
    int GetID(void) const;

    void Upgrade(void);
    Monster GetUpgrade(void) const;
    Monster GetDowngrade(void) const;

    virtual u32 GetAttack(void) const;
    virtual u32 GetDefense(void) const;
    virtual int GetColor(void) const;
    virtual int GetMorale(void) const;
    virtual int GetLuck(void) const;
    virtual int GetRace(void) const;

    u32  GetDamageMin(void) const;
    u32  GetDamageMax(void) const;
    u32  GetShots(void) const;
    u32  GetHitPoints(void) const;
    u32  GetSpeed(void) const;
    u32  GetGrown(void) const;
    u32 GetRNDSize(bool skip) const;

    const char* GetName(void) const;
    const char* GetMultiName(void) const;
    const char* GetPluralName(u32) const;

    bool isValid(void) const;
    bool isElemental(void) const;
    bool isUndead(void) const;
    bool isFly(void) const;
    bool isWide(void) const;
    bool isArchers(void) const;
    bool isAllowUpgrade(void) const;
    bool isTwiceAttack(void) const;
    bool isResurectLife(void) const;
    bool isDoubleCellAttack(void) const;
    bool isMultiCellAttack(void) const;
    bool isAlwayResponse(void) const;
    bool isHideAttack(void) const;
    bool isDragons(void) const;
    bool isAffectedByMorale(void) const;
    bool isAlive(void) const;

    int		ICNMonh(void) const;

    u32		GetSpriteIndex(void) const;
    payment_t	GetCost(void) const;
    payment_t	GetUpgradeCost(void) const;
    u32		GetDwelling(void) const;

    static Monster Rand(level_t = LEVEL0);
    static u32 Rand4WeekOf(void);
    static u32 Rand4MonthOf(void);

    static u32 GetCountFromHitPoints(const Monster &, u32);

    static void UpdateStats(const std::string &);
    static float GetUpgradeRatio(void);
    static level_t GetLevel(const monster_t);

protected:
    static Monster FromDwelling(int race, u32 dw);

    int id;
private:
    static std::map<monster_t, level_t> InitializeMonsterLevels() {
        std::map<monster_t, level_t> theMap;

        theMap[UNKNOWN] = LEVEL0;

        theMap[PEASANT] = LEVEL1;
        theMap[ARCHER] = LEVEL1;
        theMap[GOBLIN] = LEVEL1;
        theMap[ORC] = LEVEL1;
        theMap[SPRITE] = LEVEL1;
        theMap[CENTAUR] = LEVEL1;
        theMap[HALFLING] = LEVEL1;
        theMap[SKELETON] = LEVEL1;
        theMap[ZOMBIE] = LEVEL1;
        theMap[ROGUE] = LEVEL1;
        theMap[MONSTER_RND1] = LEVEL1;

        theMap[RANGER] = LEVEL2;
        theMap[PIKEMAN] = LEVEL2;
        theMap[VETERAN_PIKEMAN] = LEVEL2;
        theMap[ORC_CHIEF] = LEVEL2;
        theMap[WOLF] = LEVEL2;
        theMap[DWARF] = LEVEL2;
        theMap[BATTLE_DWARF] = LEVEL2;
        theMap[ELF] = LEVEL2;
        theMap[GRAND_ELF] = LEVEL2;
        theMap[GARGOYLE] = LEVEL2;
        theMap[BOAR] = LEVEL2;
        theMap[IRON_GOLEM] = LEVEL2;
        theMap[MUTANT_ZOMBIE] = LEVEL2;
        theMap[MUMMY] = LEVEL2;
        theMap[NOMAD] = LEVEL2;
        theMap[MONSTER_RND2] = LEVEL2;

        theMap[SWORDSMAN] = LEVEL3;
        theMap[MASTER_SWORDSMAN] = LEVEL3;
        theMap[CAVALRY] = LEVEL3;
        theMap[CHAMPION] = LEVEL3;
        theMap[OGRE] = LEVEL3;
        theMap[OGRE_LORD] = LEVEL3;
        theMap[TROLL] = LEVEL3;
        theMap[WAR_TROLL] = LEVEL3;
        theMap[DRUID] = LEVEL3;
        theMap[GREATER_DRUID] = LEVEL3;
        theMap[GRIFFIN] = LEVEL3;
        theMap[MINOTAUR] = LEVEL3;
        theMap[MINOTAUR_KING] = LEVEL3;
        theMap[STEEL_GOLEM] = LEVEL3;
        theMap[ROC] = LEVEL3;
        theMap[MAGE] = LEVEL3;
        theMap[ARCHMAGE] = LEVEL3;
        theMap[ROYAL_MUMMY] = LEVEL3;
        theMap[VAMPIRE] = LEVEL3;
        theMap[VAMPIRE_LORD] = LEVEL3;
        theMap[LICH] = LEVEL3;
        theMap[GHOST] = LEVEL3;
        theMap[MEDUSA] = LEVEL3;
        theMap[EARTH_ELEMENT] = LEVEL3;
        theMap[AIR_ELEMENT] = LEVEL3;
        theMap[FIRE_ELEMENT] = LEVEL3;
        theMap[WATER_ELEMENT] = LEVEL3;
        theMap[MONSTER_RND3] = LEVEL3;

        theMap[PALADIN] = LEVEL4;
        theMap[CRUSADER] = LEVEL4;
        theMap[CYCLOPS] = LEVEL4;
        theMap[UNICORN] = LEVEL4;
        theMap[PHOENIX] = LEVEL4;
        theMap[HYDRA] = LEVEL4;
        theMap[GREEN_DRAGON] = LEVEL4;
        theMap[RED_DRAGON] = LEVEL4;
        theMap[BLACK_DRAGON] = LEVEL4;
        theMap[GIANT] = LEVEL4;
        theMap[TITAN] = LEVEL4;
        theMap[POWER_LICH] = LEVEL4;
        theMap[BONE_DRAGON] = LEVEL4;
        theMap[GENIE] = LEVEL4;
        theMap[MONSTER_RND4] = LEVEL4;

        return theMap;
    }
};

struct MonsterStaticData
{
    // wrapper for stream
    static MonsterStaticData & Get(void);
};

StreamBase & operator<< (StreamBase &, const Monster &);
StreamBase & operator>> (StreamBase &, Monster &);

StreamBase & operator<< (StreamBase &, const MonsterStaticData &);
StreamBase & operator>> (StreamBase &, MonsterStaticData &);

#endif
