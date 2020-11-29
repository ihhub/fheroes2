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

#include "battle_animation.h"
#include "gamedefs.h"
#include "monster_info.h"
#include "payment.h"

class Spell;

class Monster
{
public:
    enum
    {
        JOIN_CONDITION_SKIP = 0,
        JOIN_CONDITION_MONEY = 1,
        JOIN_CONDITION_FREE = 2,
        JOIN_CONDITION_FORCE = 3
    };

    enum level_t
    {
        LEVEL0,
        LEVEL1,
        LEVEL2,
        LEVEL3,
        LEVEL4
    };

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

    struct animframe_t
    {
        int start;
        int count;
    };

    struct monstersprite_t
    {
        int icn_file;
        animframe_t frm_static;
        animframe_t frm_idle;
        animframe_t frm_move;
        animframe_t frm_fly1;
        animframe_t frm_fly2;
        animframe_t frm_fly3;
        animframe_t frm_shot0;
        animframe_t frm_shot1;
        animframe_t frm_shot2;
        animframe_t frm_shot3;
        animframe_t frm_attk0;
        animframe_t frm_attk1;
        animframe_t frm_attk2;
        animframe_t frm_attk3;
        animframe_t frm_wnce;
        animframe_t frm_kill;
        int m82_attk;
        int m82_kill;
        int m82_move;
        int m82_wnce;
    };

    Monster( int = UNKNOWN );
    Monster( const Spell & );
    Monster( int race, u32 dw );
    virtual ~Monster() {}

    bool operator<( const Monster & ) const;
    bool operator==( const Monster & ) const;
    bool operator!=( const Monster & ) const;

    int GetID( void ) const
    {
        return id;
    }

    void Upgrade( void );
    Monster GetUpgrade( void ) const;
    Monster GetDowngrade( void ) const;

    virtual u32 GetAttack( void ) const;
    virtual u32 GetDefense( void ) const;
    virtual int GetColor( void ) const;
    virtual int GetMorale( void ) const;
    virtual int GetLuck( void ) const;
    virtual int GetRace( void ) const;

    u32 GetDamageMin( void ) const;
    u32 GetDamageMax( void ) const;
    u32 GetShots( void ) const;
    u32 GetHitPoints( void ) const;
    u32 GetSpeed( void ) const;
    u32 GetGrown( void ) const;
    int GetMonsterLevel() const;
    int GetRandomUnitLevel( void ) const;
    u32 GetRNDSize( bool skip ) const;

    const char * GetName( void ) const;
    const char * GetMultiName( void ) const;
    const char * GetPluralName( u32 ) const;

    bool isValid( void ) const;
    bool isElemental( void ) const;
    bool isUndead( void ) const;
    bool isFlying( void ) const;
    bool isWide( void ) const;
    bool isArchers( void ) const;
    bool isAllowUpgrade( void ) const;
    bool isTwiceAttack( void ) const;
    bool isRegenerating( void ) const;
    bool isDoubleCellAttack( void ) const;
    bool isMultiCellAttack( void ) const;
    bool isAlwaysRetaliating( void ) const;
    bool ignoreRetaliation( void ) const;
    bool isDragons( void ) const;
    bool isAffectedByMorale( void ) const;
    bool isAlive( void ) const;
    bool hasMeleePenalty() const;

    double GetMonsterStrength( int attack = -1, int defense = -1 ) const;
    int ICNMonh( void ) const;

    u32 GetSpriteIndex( void ) const;
    payment_t GetCost( void ) const;
    payment_t GetUpgradeCost( void ) const;
    u32 GetDwelling( void ) const;

    const monstersprite_t & GetMonsterSprite() const;

    static Monster Rand( level_t = LEVEL0 );
    static u32 Rand4WeekOf( void );
    static u32 Rand4MonthOf( void );

    static u32 GetCountFromHitPoints( const Monster &, u32 );

    static void UpdateStats( const std::string & );
    static float GetUpgradeRatio( void );

    static uint32_t GetICNByMonsterID( uint32_t monsterID );
    static uint32_t GetMissileICN( uint32_t monsterID );

protected:
    static Monster FromDwelling( int race, u32 dw );

    int id;
};

class RandomMonsterAnimation
{
public:
    explicit RandomMonsterAnimation( const Monster & monster = Monster() );

    void increment();

    int icnFile() const;
    int frameId() const;
    int offset() const;

    void reset(); // reset to static animation

private:
    AnimationReference _reference;
    int _icnID;
    std::vector<int> _validMoves;
    std::list<int> _frameSet;
    std::list<int> _offsetSet;
    int _frameId;
    int _frameOffset;
    bool _isFlyer;

    void _pushFrames( Monster_Info::ANIMATION_TYPE type );
    void _addValidMove( Monster_Info::ANIMATION_TYPE type );
    void _updateFrameInfo();
};

struct MonsterStaticData
{
    // wrapper for stream
    static MonsterStaticData & Get( void );
};

StreamBase & operator<<( StreamBase &, const Monster & );
StreamBase & operator>>( StreamBase &, Monster & );

StreamBase & operator<<( StreamBase &, const MonsterStaticData & );
StreamBase & operator>>( StreamBase &, MonsterStaticData & );

#endif
