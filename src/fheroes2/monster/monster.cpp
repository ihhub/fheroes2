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

#include <cmath>

#include "castle.h"
#include "difficulty.h"
#include "game.h"
#include "game_static.h"
#include "icn.h"
#include "luck.h"
#include "monster.h"
#include "morale.h"
#include "race.h"
#include "rand.h"
#include "save_format_version.h"
#include "settings.h"
#include "speed.h"
#include "translations.h"

namespace
{
    struct monstats_t
    {
        u8 attack;
        u8 defense;
        u8 damageMin;
        u8 damageMax;
        u16 hp;
        u8 speed;
        u8 grown;
        u8 shots;
        const char * name;
        const char * multiname;
        cost_t cost;
    };
}

StreamBase & operator>>( StreamBase & msg, monstats_t & obj )
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE_095_RELEASE, "Remove this function as it's not supported from 0.9.5" );
    return msg >> obj.attack >> obj.defense >> obj.damageMin >> obj.damageMax >> obj.hp >> obj.speed >> obj.grown >> obj.shots >> obj.cost;
}

StreamBase & operator>>( StreamBase & msg, const MonsterStaticData & /*obj*/ )
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE_095_RELEASE, "Remove this function as it's not supported from 0.9.5" );

    u32 monsters_size;
    msg >> monsters_size;

    monstats_t dummyMonster = { 0, 0, 0, 0, 0, Speed::VERYSLOW, 0, 0, "", "", { 0, 0, 0, 0, 0, 0, 0 } };

    for ( u32 ii = 0; ii < monsters_size; ++ii )
        msg >> dummyMonster;
    return msg;
}

uint32_t Monster::GetMissileICN( uint32_t monsterID )
{
    switch ( monsterID ) {
    case Monster::ARCHER:
        return ICN::ARCH_MSL;
    case Monster::RANGER:
        return ICN::ARCH_MSL;
    case Monster::ORC:
        return ICN::ORC__MSL;
    case Monster::ORC_CHIEF:
        return ICN::ORC__MSL;
    case Monster::TROLL:
        return ICN::TROLLMSL;
    case Monster::WAR_TROLL:
        return ICN::TROLL2MSL;
    case Monster::ELF:
        return ICN::ELF__MSL;
    case Monster::GRAND_ELF:
        return ICN::ELF__MSL;
    case Monster::DRUID:
        return ICN::DRUIDMSL;
    case Monster::GREATER_DRUID:
        return ICN::DRUIDMSL;
    case Monster::CENTAUR:
        // Doesn't have own missile file, game falls back to ELF__MSL
        return ICN::ELF__MSL;
    case Monster::HALFLING:
        return ICN::HALFLMSL;
    case Monster::TITAN:
        return ICN::TITANMSL;
    case Monster::LICH:
        return ICN::LICH_MSL;
    case Monster::POWER_LICH:
        return ICN::LICH_MSL;

    default:
        break;
    }

    return ICN::UNKNOWN;
}

Monster::Monster( const int m )
    : id( UNKNOWN )
{
    if ( m <= WATER_ELEMENT ) {
        id = m;
    }
    else if ( MONSTER_RND1 == m )
        id = Rand( LevelType::LEVEL_1 ).GetID();
    else if ( MONSTER_RND2 == m )
        id = Rand( LevelType::LEVEL_2 ).GetID();
    else if ( MONSTER_RND3 == m )
        id = Rand( LevelType::LEVEL_3 ).GetID();
    else if ( MONSTER_RND4 == m )
        id = Rand( LevelType::LEVEL_4 ).GetID();
    else if ( MONSTER_RND == m )
        id = Rand( LevelType::LEVEL_ANY ).GetID();
}

Monster::Monster( const Spell & sp )
    : id( UNKNOWN )
{
    switch ( sp() ) {
    case Spell::SETEGUARDIAN:
    case Spell::SUMMONEELEMENT:
        id = EARTH_ELEMENT;
        break;

    case Spell::SETAGUARDIAN:
    case Spell::SUMMONAELEMENT:
        id = AIR_ELEMENT;
        break;

    case Spell::SETFGUARDIAN:
    case Spell::SUMMONFELEMENT:
        id = FIRE_ELEMENT;
        break;

    case Spell::SETWGUARDIAN:
    case Spell::SUMMONWELEMENT:
        id = WATER_ELEMENT;
        break;

    case Spell::HAUNT:
        id = GHOST;
        break;
    default:
        break;
    }
}

Monster::Monster( int race, u32 dw )
    : id( UNKNOWN )
{
    id = FromDwelling( race, dw ).id;
}

bool Monster::isValid( void ) const
{
    return id != UNKNOWN;
}

bool Monster::operator==( const Monster & m ) const
{
    return id == m.id;
}

bool Monster::operator!=( const Monster & m ) const
{
    return id != m.id;
}

void Monster::Upgrade( void )
{
    id = GetUpgrade().id;
}

u32 Monster::GetAttack() const
{
    return fheroes2::getMonsterData( id ).battleStats.attack;
}

u32 Monster::GetDefense() const
{
    return fheroes2::getMonsterData( id ).battleStats.defense;
}

int Monster::GetColor() const
{
    return Color::NONE;
}

int Monster::GetMorale() const
{
    return Morale::NORMAL;
}

int Monster::GetLuck() const
{
    return Luck::NORMAL;
}

int Monster::GetRace() const
{
    return fheroes2::getMonsterData( id ).generalStats.race;
}

u32 Monster::GetDamageMin() const
{
    return fheroes2::getMonsterData( id ).battleStats.damageMin;
}

u32 Monster::GetDamageMax() const
{
    return fheroes2::getMonsterData( id ).battleStats.damageMax;
}

u32 Monster::GetShots() const
{
    return fheroes2::getMonsterData( id ).battleStats.shots;
}

u32 Monster::GetHitPoints() const
{
    return fheroes2::getMonsterData( id ).battleStats.hp;
}

u32 Monster::GetSpeed() const
{
    return fheroes2::getMonsterData( id ).battleStats.speed;
}

u32 Monster::GetGrown() const
{
    return fheroes2::getMonsterData( id ).generalStats.baseGrowth;
}

// Get universal heuristic of Monster type regardless of context; both combat and strategic value
// Doesn't account for situational special bonuses such as spell immunity
double Monster::GetMonsterStrength( int attack, int defense ) const
{
    const fheroes2::MonsterBattleStats & battleStats = fheroes2::getMonsterData( id ).battleStats;

    // If no modified values were provided then re-calculate
    // GetAttack and GetDefense will call overloaded versions accounting for Hero bonuses
    if ( attack == -1 )
        attack = battleStats.attack;

    if ( defense == -1 )
        defense = battleStats.defense;

    const double attackDefense = 1.0 + attack * 0.1 + defense * 0.05;
    const double effectiveHP = battleStats.hp * ( ignoreRetaliation() ? 1.4 : 1 );

    double damagePotential = ( battleStats.damageMin + battleStats.damageMax ) / 2.0;

    if ( isTwiceAttack() ) {
        // Melee attacker will lose potential on second attack after retaliation
        damagePotential *= ( isArchers() || ignoreRetaliation() ) ? 2 : 1.75;
    }
    if ( isAbilityPresent( fheroes2::MonsterAbilityType::DOUBLE_DAMAGE_TO_UNDEAD ) )
        damagePotential *= 1.15; // 15% of all Monsters are Undead, deals double dmg
    if ( isDoubleCellAttack() )
        damagePotential *= 1.2;
    if ( isAbilityPresent( fheroes2::MonsterAbilityType::ALWAYS_RETALIATE ) )
        damagePotential *= 1.25;
    if ( isAbilityPresent( fheroes2::MonsterAbilityType::ALL_ADJACENT_CELL_MELEE_ATTACK ) || isAbilityPresent( fheroes2::MonsterAbilityType::AREA_SHOT ) )
        damagePotential *= 1.3;

    double monsterSpecial = 1.0;
    if ( isArchers() ) {
        monsterSpecial += isAbilityPresent( fheroes2::MonsterAbilityType::NO_MELEE_PENALTY ) ? 0.5 : 0.4;
    }
    if ( isFlying() ) {
        monsterSpecial += 0.3;
    }

    switch ( id ) {
    case Monster::UNICORN:
    case Monster::CYCLOPS:
    case Monster::MEDUSA:
        // 20% to Blind, Paralyze and Petrify
        monsterSpecial += 0.2;
        break;
    case Monster::VAMPIRE_LORD:
        // Lifesteal
        monsterSpecial += 0.3;
        break;
    case Monster::GENIE:
        // Genie's ability to half enemy troops
        monsterSpecial += 1;
        break;
    case Monster::GHOST:
        // Ghost's ability to increase the numbers
        monsterSpecial += 2;
        break;
    }

    // Higher speed gives initiative advantage/first attack. Remap speed value to -0.2...+0.15, AVERAGE is 0
    // Punish slow speeds more as unit won't participate in first rounds and slows down strategic army
    const int speedDiff = battleStats.speed - Speed::AVERAGE;
    monsterSpecial += ( speedDiff < 0 ) ? speedDiff * 0.1 : speedDiff * 0.05;

    // Additonal HP and Damage effectiveness diminishes with every combat round; strictly x4 HP == x2 unit count
    return sqrt( damagePotential * effectiveHP ) * attackDefense * monsterSpecial;
}

u32 Monster::GetRNDSize( bool skip_factor ) const
{
    if ( !isValid() )
        return 0;

    const uint32_t defaultArmySizePerLevel[7] = {0, 50, 30, 25, 25, 12, 8};
    uint32_t result = 0;

    // Check for outliers
    switch ( id ) {
    case PEASANT:
        result = 80;
        break;
    case ROGUE:
        result = 40;
        break;
    case PIKEMAN:
    case VETERAN_PIKEMAN:
    case WOLF:
    case ELF:
    case GRAND_ELF:
        result = 30;
        break;
    case GARGOYLE:
        result = 25;
        break;
    case GHOST:
    case MEDUSA:
        result = 20;
        break;
    case MINOTAUR:
    case MINOTAUR_KING:
    case ROC:
    case VAMPIRE:
    case VAMPIRE_LORD:
    case UNICORN:
        result = 16;
        break;
    case CAVALRY:
    case CHAMPION:
        result = 18;
        break;
    case PALADIN:
    case CRUSADER:
    case CYCLOPS:
    case PHOENIX:
        result = 12;
        break;
    default:
        // for most units default range is okay
        result = defaultArmySizePerLevel[GetMonsterLevel()];
        break;
    }

    if ( !skip_factor && Settings::Get().ExtWorldNeutralArmyDifficultyScaling() ) {
        uint32_t factor = 100;

        switch ( Game::getDifficulty() ) {
        case Difficulty::EASY:
            factor = 80;
            break;
        case Difficulty::NORMAL:
            factor = 100;
            break;
        case Difficulty::HARD:
            factor = 130;
            break;
        case Difficulty::EXPERT:
            factor = 160;
            break;
        case Difficulty::IMPOSSIBLE:
            factor = 190;
            break;
        default:
            break;
        }

        result = ( result * factor / 100 );
        // force minimal
        if ( result == 0 )
            result = 1;
    }

    return ( result > 1 ) ? Rand::Get( result / 2, result ) : 1;
}

bool Monster::isElemental() const
{
    return isAbilityPresent( fheroes2::MonsterAbilityType::ELEMENTAL );
}

bool Monster::isUndead() const
{
    return isAbilityPresent( fheroes2::MonsterAbilityType::UNDEAD );
}

bool Monster::isAbilityPresent( const fheroes2::MonsterAbilityType abilityType ) const
{
    const std::set<fheroes2::MonsterAbility> & abilities = fheroes2::getMonsterData( id ).battleStats.abilities;

    return abilities.find( fheroes2::MonsterAbility( abilityType ) ) != abilities.end();
}

bool Monster::isFlying() const
{
    return isAbilityPresent( fheroes2::MonsterAbilityType::FLYING );
}

bool Monster::isWide() const
{
    return isAbilityPresent( fheroes2::MonsterAbilityType::DOUBLE_HEX_SIZE );
}

bool Monster::isArchers() const
{
    return GetShots() > 0;
}

bool Monster::isAllowUpgrade() const
{
    return id != GetUpgrade().id;
}

bool Monster::ignoreRetaliation() const
{
    return isAbilityPresent( fheroes2::MonsterAbilityType::NO_ENEMY_RETALIATION );
}

bool Monster::isDragons() const
{
    return isAbilityPresent( fheroes2::MonsterAbilityType::DRAGON );
}

bool Monster::isTwiceAttack() const
{
    return isAbilityPresent( fheroes2::MonsterAbilityType::DOUBLE_MELEE_ATTACK ) || isAbilityPresent( fheroes2::MonsterAbilityType::DOUBLE_SHOOTING );
}

bool Monster::isRegenerating() const
{
    return isAbilityPresent( fheroes2::MonsterAbilityType::HP_REGENERATION );
}

bool Monster::isDoubleCellAttack() const
{
    return isAbilityPresent( fheroes2::MonsterAbilityType::TWO_CELL_MELEE_ATTACK );
}

bool Monster::isAffectedByMorale() const
{
    return !( isUndead() || isElemental() );
}

Monster Monster::GetDowngrade() const
{
    switch ( id ) {
    case RANGER:
        return Monster( ARCHER );
    case VETERAN_PIKEMAN:
        return Monster( PIKEMAN );
    case MASTER_SWORDSMAN:
        return Monster( SWORDSMAN );
    case CHAMPION:
        return Monster( CAVALRY );
    case CRUSADER:
        return Monster( PALADIN );
    case ORC_CHIEF:
        return Monster( ORC );
    case OGRE_LORD:
        return Monster( OGRE );
    case WAR_TROLL:
        return Monster( TROLL );
    case BATTLE_DWARF:
        return Monster( DWARF );
    case GRAND_ELF:
        return Monster( ELF );
    case GREATER_DRUID:
        return Monster( DRUID );
    case MUTANT_ZOMBIE:
        return Monster( ZOMBIE );
    case ROYAL_MUMMY:
        return Monster( MUMMY );
    case VAMPIRE_LORD:
        return Monster( VAMPIRE );
    case POWER_LICH:
        return Monster( LICH );
    case MINOTAUR_KING:
        return Monster( MINOTAUR );
    case RED_DRAGON:
        return Monster( GREEN_DRAGON );
    case BLACK_DRAGON:
        return Monster( RED_DRAGON );
    case STEEL_GOLEM:
        return Monster( IRON_GOLEM );
    case ARCHMAGE:
        return Monster( MAGE );
    case TITAN:
        return Monster( GIANT );

    default:
        break;
    }

    return Monster( id );
}

Monster Monster::GetUpgrade( void ) const
{
    switch ( id ) {
    case ARCHER:
        return Monster( RANGER );
    case PIKEMAN:
        return Monster( VETERAN_PIKEMAN );
    case SWORDSMAN:
        return Monster( MASTER_SWORDSMAN );
    case CAVALRY:
        return Monster( CHAMPION );
    case PALADIN:
        return Monster( CRUSADER );
    case ORC:
        return Monster( ORC_CHIEF );
    case OGRE:
        return Monster( OGRE_LORD );
    case TROLL:
        return Monster( WAR_TROLL );
    case DWARF:
        return Monster( BATTLE_DWARF );
    case ELF:
        return Monster( GRAND_ELF );
    case DRUID:
        return Monster( GREATER_DRUID );
    case ZOMBIE:
        return Monster( MUTANT_ZOMBIE );
    case MUMMY:
        return Monster( ROYAL_MUMMY );
    case VAMPIRE:
        return Monster( VAMPIRE_LORD );
    case LICH:
        return Monster( POWER_LICH );
    case MINOTAUR:
        return Monster( MINOTAUR_KING );
    case GREEN_DRAGON:
        return Monster( RED_DRAGON );
    case RED_DRAGON:
        return Monster( BLACK_DRAGON );
    case IRON_GOLEM:
        return Monster( STEEL_GOLEM );
    case MAGE:
        return Monster( ARCHMAGE );
    case GIANT:
        return Monster( TITAN );

    default:
        break;
    }

    return Monster( id );
}

Monster Monster::FromDwelling( int race, u32 dwelling )
{
    switch ( dwelling ) {
    case DWELLING_MONSTER1:
        switch ( race ) {
        case Race::KNGT:
            return Monster( PEASANT );
        case Race::BARB:
            return Monster( GOBLIN );
        case Race::SORC:
            return Monster( SPRITE );
        case Race::WRLK:
            return Monster( CENTAUR );
        case Race::WZRD:
            return Monster( HALFLING );
        case Race::NECR:
            return Monster( SKELETON );
        default:
            break;
        }
        break;

    case DWELLING_MONSTER2:
        switch ( race ) {
        case Race::KNGT:
            return Monster( ARCHER );
        case Race::BARB:
            return Monster( ORC );
        case Race::SORC:
            return Monster( DWARF );
        case Race::WRLK:
            return Monster( GARGOYLE );
        case Race::WZRD:
            return Monster( BOAR );
        case Race::NECR:
            return Monster( ZOMBIE );
        default:
            break;
        }
        break;

    case DWELLING_UPGRADE2:
        switch ( race ) {
        case Race::KNGT:
            return Monster( RANGER );
        case Race::BARB:
            return Monster( ORC_CHIEF );
        case Race::SORC:
            return Monster( BATTLE_DWARF );
        case Race::WRLK:
            return Monster( GARGOYLE );
        case Race::WZRD:
            return Monster( BOAR );
        case Race::NECR:
            return Monster( MUTANT_ZOMBIE );
        default:
            break;
        }
        break;

    case DWELLING_MONSTER3:
        switch ( race ) {
        case Race::KNGT:
            return Monster( PIKEMAN );
        case Race::BARB:
            return Monster( WOLF );
        case Race::SORC:
            return Monster( ELF );
        case Race::WRLK:
            return Monster( GRIFFIN );
        case Race::WZRD:
            return Monster( IRON_GOLEM );
        case Race::NECR:
            return Monster( MUMMY );
        default:
            break;
        }
        break;

    case DWELLING_UPGRADE3:
        switch ( race ) {
        case Race::KNGT:
            return Monster( VETERAN_PIKEMAN );
        case Race::BARB:
            return Monster( WOLF );
        case Race::SORC:
            return Monster( GRAND_ELF );
        case Race::WRLK:
            return Monster( GRIFFIN );
        case Race::WZRD:
            return Monster( STEEL_GOLEM );
        case Race::NECR:
            return Monster( ROYAL_MUMMY );
        default:
            break;
        }
        break;

    case DWELLING_MONSTER4:
        switch ( race ) {
        case Race::KNGT:
            return Monster( SWORDSMAN );
        case Race::BARB:
            return Monster( OGRE );
        case Race::SORC:
            return Monster( DRUID );
        case Race::WRLK:
            return Monster( MINOTAUR );
        case Race::WZRD:
            return Monster( ROC );
        case Race::NECR:
            return Monster( VAMPIRE );
        default:
            break;
        }
        break;

    case DWELLING_UPGRADE4:
        switch ( race ) {
        case Race::KNGT:
            return Monster( MASTER_SWORDSMAN );
        case Race::BARB:
            return Monster( OGRE_LORD );
        case Race::SORC:
            return Monster( GREATER_DRUID );
        case Race::WRLK:
            return Monster( MINOTAUR_KING );
        case Race::WZRD:
            return Monster( ROC );
        case Race::NECR:
            return Monster( VAMPIRE_LORD );
        default:
            break;
        }
        break;

    case DWELLING_MONSTER5:
        switch ( race ) {
        case Race::KNGT:
            return Monster( CAVALRY );
        case Race::BARB:
            return Monster( TROLL );
        case Race::SORC:
            return Monster( UNICORN );
        case Race::WRLK:
            return Monster( HYDRA );
        case Race::WZRD:
            return Monster( MAGE );
        case Race::NECR:
            return Monster( LICH );
        default:
            break;
        }
        break;

    case DWELLING_UPGRADE5:
        switch ( race ) {
        case Race::KNGT:
            return Monster( CHAMPION );
        case Race::BARB:
            return Monster( WAR_TROLL );
        case Race::SORC:
            return Monster( UNICORN );
        case Race::WRLK:
            return Monster( HYDRA );
        case Race::WZRD:
            return Monster( ARCHMAGE );
        case Race::NECR:
            return Monster( POWER_LICH );
        default:
            break;
        }
        break;

    case DWELLING_MONSTER6:
        switch ( race ) {
        case Race::KNGT:
            return Monster( PALADIN );
        case Race::BARB:
            return Monster( CYCLOPS );
        case Race::SORC:
            return Monster( PHOENIX );
        case Race::WRLK:
            return Monster( GREEN_DRAGON );
        case Race::WZRD:
            return Monster( GIANT );
        case Race::NECR:
            return Monster( BONE_DRAGON );
        default:
            break;
        }
        break;

    case DWELLING_UPGRADE6:
        switch ( race ) {
        case Race::KNGT:
            return Monster( CRUSADER );
        case Race::BARB:
            return Monster( CYCLOPS );
        case Race::SORC:
            return Monster( PHOENIX );
        case Race::WRLK:
            return Monster( RED_DRAGON );
        case Race::WZRD:
            return Monster( TITAN );
        case Race::NECR:
            return Monster( BONE_DRAGON );
        default:
            break;
        }
        break;

    case DWELLING_UPGRADE7:
        switch ( race ) {
        case Race::KNGT:
            return Monster( CRUSADER );
        case Race::BARB:
            return Monster( CYCLOPS );
        case Race::SORC:
            return Monster( PHOENIX );
        case Race::WRLK:
            return Monster( BLACK_DRAGON );
        case Race::WZRD:
            return Monster( TITAN );
        case Race::NECR:
            return Monster( BONE_DRAGON );
        default:
            break;
        }
        break;

    default:
        break;
    }

    return Monster( UNKNOWN );
}

Monster Monster::Rand( const LevelType type )
{
    if ( type == LevelType::LEVEL_ANY )
        return Monster( Rand::Get( PEASANT, WATER_ELEMENT ) );
    static std::vector<Monster> monstersVec[static_cast<int>( LevelType::LEVEL_4 )];
    if ( monstersVec[0].empty() ) {
        for ( uint32_t i = PEASANT; i <= WATER_ELEMENT; ++i ) {
            const Monster monster( i );
            if ( monster.GetRandomUnitLevel() > LevelType::LEVEL_ANY )
                monstersVec[static_cast<int>( monster.GetRandomUnitLevel() ) - 1].push_back( monster );
        }
    }
    return Rand::Get( monstersVec[static_cast<int>( type ) - 1] );
}

u32 Monster::Rand4WeekOf( void )
{
    switch ( Rand::Get( 1, 47 ) ) {
    case 1:
        return PEASANT;
    case 2:
        return ARCHER;
    case 3:
        return RANGER;
    case 4:
        return PIKEMAN;
    case 5:
        return VETERAN_PIKEMAN;
    case 6:
        return SWORDSMAN;
    case 7:
        return MASTER_SWORDSMAN;
    case 8:
        return CAVALRY;
    case 9:
        return CHAMPION;
    case 10:
        return GOBLIN;
    case 11:
        return ORC;
    case 12:
        return ORC_CHIEF;
    case 13:
        return WOLF;
    case 14:
        return OGRE;
    case 15:
        return OGRE_LORD;
    case 16:
        return TROLL;
    case 17:
        return WAR_TROLL;
    case 18:
        return SPRITE;
    case 19:
        return DWARF;
    case 20:
        return BATTLE_DWARF;
    case 21:
        return ELF;
    case 22:
        return GRAND_ELF;
    case 23:
        return DRUID;
    case 24:
        return GREATER_DRUID;
    case 25:
        return UNICORN;
    case 26:
        return CENTAUR;
    case 27:
        return GARGOYLE;
    case 28:
        return GRIFFIN;
    case 29:
        return MINOTAUR;
    case 30:
        return MINOTAUR_KING;
    case 31:
        return HYDRA;
    case 32:
        return HALFLING;
    case 33:
        return BOAR;
    case 34:
        return IRON_GOLEM;
    case 35:
        return STEEL_GOLEM;
    case 36:
        return ROC;
    case 37:
        return MAGE;
    case 38:
        return ARCHMAGE;
    case 39:
        return SKELETON;
    case 40:
        return ZOMBIE;
    case 41:
        return MUTANT_ZOMBIE;
    case 42:
        return MUMMY;
    case 43:
        return ROYAL_MUMMY;
    case 44:
        return VAMPIRE;
    case 45:
        return VAMPIRE_LORD;
    case 46:
        return LICH;
    case 47:
        return POWER_LICH;
    default:
        break;
    }
    return UNKNOWN;
}

u32 Monster::Rand4MonthOf( void )
{
    switch ( Rand::Get( 1, 30 ) ) {
    case 1:
        return PEASANT;
    case 2:
        return ARCHER;
    case 3:
        return PIKEMAN;
    case 4:
        return SWORDSMAN;
    case 5:
        return CAVALRY;
    case 6:
        return GOBLIN;
    case 7:
        return ORC;
    case 8:
        return WOLF;
    case 9:
        return OGRE;
    case 10:
        return TROLL;
    case 11:
        return SPRITE;
    case 12:
        return DWARF;
    case 13:
        return ELF;
    case 14:
        return DRUID;
    case 15:
        return UNICORN;
    case 16:
        return CENTAUR;
    case 17:
        return GARGOYLE;
    case 18:
        return GRIFFIN;
    case 19:
        return MINOTAUR;
    case 20:
        return HYDRA;
    case 21:
        return HALFLING;
    case 22:
        return BOAR;
    case 23:
        return IRON_GOLEM;
    case 24:
        return ROC;
    case 25:
        return MAGE;
    case 26:
        return SKELETON;
    case 27:
        return ZOMBIE;
    case 28:
        return MUMMY;
    case 29:
        return VAMPIRE;
    case 30:
        return LICH;
    default:
        break;
    }
    return UNKNOWN;
}

int Monster::GetMonsterLevel() const
{
    return fheroes2::getMonsterData( id ).generalStats.level;
}

Monster::LevelType Monster::GetRandomUnitLevel() const
{
    switch ( id ) {
    case PEASANT:
    case ARCHER:
    case GOBLIN:
    case ORC:
    case SPRITE:
    case CENTAUR:
    case HALFLING:
    case SKELETON:
    case ZOMBIE:
    case ROGUE:
    case MONSTER_RND1:
        return LevelType::LEVEL_1;

    case RANGER:
    case PIKEMAN:
    case VETERAN_PIKEMAN:
    case ORC_CHIEF:
    case WOLF:
    case DWARF:
    case BATTLE_DWARF:
    case ELF:
    case GRAND_ELF:
    case GARGOYLE:
    case BOAR:
    case IRON_GOLEM:
    case MUTANT_ZOMBIE:
    case MUMMY:
    case NOMAD:
    case MONSTER_RND2:
        return LevelType::LEVEL_2;

    case SWORDSMAN:
    case MASTER_SWORDSMAN:
    case CAVALRY:
    case CHAMPION:
    case OGRE:
    case OGRE_LORD:
    case TROLL:
    case WAR_TROLL:
    case DRUID:
    case GREATER_DRUID:
    case GRIFFIN:
    case MINOTAUR:
    case MINOTAUR_KING:
    case STEEL_GOLEM:
    case ROC:
    case MAGE:
    case ARCHMAGE:
    case ROYAL_MUMMY:
    case VAMPIRE:
    case VAMPIRE_LORD:
    case LICH:
    case GHOST:
    case MEDUSA:
    case EARTH_ELEMENT:
    case AIR_ELEMENT:
    case FIRE_ELEMENT:
    case WATER_ELEMENT:
    case MONSTER_RND3:
        return LevelType::LEVEL_3;

    case PALADIN:
    case CRUSADER:
    case CYCLOPS:
    case UNICORN:
    case PHOENIX:
    case HYDRA:
    case GREEN_DRAGON:
    case RED_DRAGON:
    case BLACK_DRAGON:
    case GIANT:
    case TITAN:
    case POWER_LICH:
    case BONE_DRAGON:
    case GENIE:
    case MONSTER_RND4:
        return LevelType::LEVEL_4;

    case MONSTER_RND:
        switch ( Rand::Get( 0, 3 ) ) {
        default:
            return LevelType::LEVEL_1;
        case 1:
            return LevelType::LEVEL_2;
        case 2:
            return LevelType::LEVEL_3;
        case 3:
            return LevelType::LEVEL_4;
        }
        break;

    default:
        break;
    }

    return LevelType::LEVEL_ANY;
}

u32 Monster::GetDwelling( void ) const
{
    switch ( id ) {
    case PEASANT:
    case GOBLIN:
    case SPRITE:
    case CENTAUR:
    case HALFLING:
    case SKELETON:
        return DWELLING_MONSTER1;

    case ARCHER:
    case ORC:
    case ZOMBIE:
    case DWARF:
    case GARGOYLE:
    case BOAR:
        return DWELLING_MONSTER2;

    case RANGER:
    case ORC_CHIEF:
    case BATTLE_DWARF:
    case MUTANT_ZOMBIE:
        return DWELLING_UPGRADE2;

    case PIKEMAN:
    case WOLF:
    case ELF:
    case IRON_GOLEM:
    case MUMMY:
    case GRIFFIN:
        return DWELLING_MONSTER3;

    case VETERAN_PIKEMAN:
    case GRAND_ELF:
    case STEEL_GOLEM:
    case ROYAL_MUMMY:
        return DWELLING_UPGRADE3;

    case SWORDSMAN:
    case OGRE:
    case DRUID:
    case MINOTAUR:
    case ROC:
    case VAMPIRE:
        return DWELLING_MONSTER4;

    case MASTER_SWORDSMAN:
    case OGRE_LORD:
    case GREATER_DRUID:
    case MINOTAUR_KING:
    case VAMPIRE_LORD:
        return DWELLING_UPGRADE4;

    case CAVALRY:
    case TROLL:
    case MAGE:
    case LICH:
    case UNICORN:
    case HYDRA:
        return DWELLING_MONSTER5;

    case CHAMPION:
    case WAR_TROLL:
    case ARCHMAGE:
    case POWER_LICH:
        return DWELLING_UPGRADE5;

    case PALADIN:
    case CYCLOPS:
    case PHOENIX:
    case GREEN_DRAGON:
    case GIANT:
    case BONE_DRAGON:
        return DWELLING_MONSTER6;

    case CRUSADER:
    case RED_DRAGON:
    case TITAN:
        return DWELLING_UPGRADE6;

    case BLACK_DRAGON:
        return DWELLING_UPGRADE7;

    default:
        break;
    }

    return 0;
}

const char * Monster::GetName( void ) const
{
    return _( fheroes2::getMonsterData( id ).generalStats.name );
}

const char * Monster::GetMultiName( void ) const
{
    return _( fheroes2::getMonsterData( id ).generalStats.pluralName );
}

const char * Monster::GetPluralName( u32 count ) const
{
    const fheroes2::MonsterGeneralStats & generalStats = fheroes2::getMonsterData( id ).generalStats;
    return _n( generalStats.name, generalStats.pluralName, count );
}

u32 Monster::GetSpriteIndex( void ) const
{
    return UNKNOWN < id ? id - 1 : 0;
}

int Monster::ICNMonh( void ) const
{
    return id >= PEASANT && id <= WATER_ELEMENT ? ICN::MONH0000 + id - PEASANT : ICN::UNKNOWN;
}

payment_t Monster::GetCost( void ) const
{
    return payment_t( fheroes2::getMonsterData( id ).generalStats.cost );
}

payment_t Monster::GetUpgradeCost( void ) const
{
    const Monster upgr = GetUpgrade();
    const payment_t pay = id != upgr.id ? upgr.GetCost() - GetCost() : GetCost();

    return pay;
}

u32 Monster::GetCountFromHitPoints( const Monster & mons, u32 hp )
{
    if ( hp ) {
        const u32 hp1 = mons.GetHitPoints();
        const u32 count = hp / hp1;
        return ( count * hp1 ) < hp ? count + 1 : count;
    }

    return 0;
}

int Monster::GetMonsterSprite() const
{
    return fheroes2::getMonsterData( id ).icnId;
}

MonsterStaticData & MonsterStaticData::Get( void )
{
    static MonsterStaticData mgds;
    return mgds;
}
