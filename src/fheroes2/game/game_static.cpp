/***************************************************************************
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cstring>

#include "difficulty.h"
#include "game.h"
#include "game_static.h"
#include "mp2.h"
#include "race.h"
#include "resource.h"
#include "settings.h"
#include "skill.h"
#include "skill_static.h"

namespace Skill
{
    stats_t _stats[] = {{"knight",
                         {1, 1, 1, 1},
                         {2, 2, 1, 1},
                         0,
                         0,
                         {0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                         10,
                         {35, 45, 10, 10},
                         {25, 25, 25, 25},
                         {2, 4, 3, 1, 3, 5, 3, 1, 1, 2, 0, 3, 2, 2}},
                        {"barbarian",
                         {1, 1, 1, 1},
                         {3, 1, 1, 1},
                         0,
                         0,
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0},
                         10,
                         {55, 35, 5, 5},
                         {30, 30, 20, 20},
                         {3, 3, 2, 1, 2, 3, 3, 2, 1, 3, 0, 4, 4, 1}},
                        {"sorceress",
                         {0, 0, 2, 2},
                         {0, 0, 2, 3},
                         1,
                         15,
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1},
                         10,
                         {10, 10, 30, 50},
                         {20, 20, 30, 30},
                         {3, 3, 2, 2, 2, 1, 2, 3, 3, 4, 0, 2, 1, 4}},
                        {"warlock",
                         {0, 0, 2, 2},
                         {0, 0, 3, 2},
                         1,
                         19,
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1},
                         10,
                         {10, 10, 50, 30},
                         {20, 20, 30, 30},
                         {1, 3, 2, 3, 2, 1, 2, 1, 3, 2, 1, 2, 4, 5}},
                        {"wizard",
                         {0, 0, 2, 2},
                         {0, 1, 2, 2},
                         1,
                         17,
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
                         10,
                         {10, 10, 40, 40},
                         {20, 20, 30, 30},
                         {1, 3, 2, 3, 2, 2, 2, 2, 4, 2, 0, 2, 2, 5}},
                        {"necromancer",
                         {0, 0, 2, 2},
                         {1, 0, 2, 2},
                         1,
                         10,
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
                         10,
                         {15, 15, 35, 35},
                         {25, 25, 25, 25},
                         {1, 3, 2, 3, 2, 0, 2, 1, 3, 2, 5, 3, 1, 4}},
                        {NULL,
                         {0, 0, 0, 0},
                         {0, 0, 0, 0},
                         0,
                         0,
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                         10,
                         {0, 0, 0, 0},
                         {0, 0, 0, 0},
                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}};

    values_t _values[] = {
        {"pathfinding", {25, 50, 100}}, {"archery", {10, 25, 50}},  {"logistics", {10, 20, 30}},  {"scouting", {1, 2, 3}},      {"diplomacy", {25, 50, 100}},
        {"navigation", {33, 66, 100}},  {"leadership", {1, 2, 3}},  {"wisdom", {3, 4, 5}},        {"mysticism", {2, 3, 4}},     {"luck", {1, 2, 3}},
        {"ballistics", {0, 0, 0}},      {"eagleeye", {20, 30, 40}}, {"necromancy", {10, 20, 30}}, {"estates", {100, 250, 500}}, {NULL, {0, 0, 0}},
    };

    secondary_t _from_witchs_hut = {
        /* archery */ 1,   /* ballistics */ 1, /* diplomacy */ 1, /* eagleeye */ 1,
        /* estates */ 1,   /* leadership */ 0, /* logistics */ 1, /* luck */ 1,
        /* mysticism */ 1, /* navigation */ 1, /* necromancy*/ 0, /* pathfinding */ 1,
        /* scouting */ 1,  /* wisdom */ 1};

    StreamBase & operator<<( StreamBase & msg, const level_t & obj )
    {
        return msg << obj.basic << obj.advanced << obj.expert;
    }

    StreamBase & operator>>( StreamBase & msg, level_t & obj )
    {
        return msg >> obj.basic >> obj.advanced >> obj.expert;
    }

    StreamBase & operator<<( StreamBase & msg, const primary_t & obj )
    {
        return msg << obj.attack << obj.defense << obj.power << obj.knowledge;
    }

    StreamBase & operator>>( StreamBase & msg, primary_t & obj )
    {
        return msg >> obj.attack >> obj.defense >> obj.power >> obj.knowledge;
    }

    StreamBase & operator<<( StreamBase & msg, const secondary_t & obj )
    {
        return msg << obj.archery << obj.ballistics << obj.diplomacy << obj.eagleeye << obj.estates << obj.leadership << obj.logistics << obj.luck << obj.mysticism
                   << obj.navigation << obj.necromancy << obj.pathfinding << obj.scouting << obj.wisdom;
    }

    StreamBase & operator>>( StreamBase & msg, secondary_t & obj )
    {
        return msg >> obj.archery >> obj.ballistics >> obj.diplomacy >> obj.eagleeye >> obj.estates >> obj.leadership >> obj.logistics >> obj.luck >> obj.mysticism
               >> obj.navigation >> obj.necromancy >> obj.pathfinding >> obj.scouting >> obj.wisdom;
    }

    StreamBase & operator<<( StreamBase & msg, const stats_t & obj )
    {
        return msg << obj.captain_primary << obj.initial_primary << obj.initial_book << obj.initial_spell << obj.initial_secondary << obj.over_level
                   << obj.mature_primary_under << obj.mature_primary_over << obj.mature_secondary;
    }

    StreamBase & operator>>( StreamBase & msg, stats_t & obj )
    {
        return msg >> obj.captain_primary >> obj.initial_primary >> obj.initial_book >> obj.initial_spell >> obj.initial_secondary >> obj.over_level
               >> obj.mature_primary_under >> obj.mature_primary_over >> obj.mature_secondary;
    }

    StreamBase & operator<<( StreamBase & msg, const values_t & obj )
    {
        return msg << obj.values;
    }

    StreamBase & operator>>( StreamBase & msg, values_t & obj )
    {
        return msg >> obj.values;
    }

#ifdef WITH_XML
    void LoadPrimarySection( const TiXmlElement * xml, primary_t & skill )
    {
        if ( xml ) {
            int value;
            xml->Attribute( "attack", &value );
            skill.attack = value;
            xml->Attribute( "defense", &value );
            skill.defense = value;
            xml->Attribute( "power", &value );
            skill.power = value;
            xml->Attribute( "knowledge", &value );
            skill.knowledge = value;
        }
    }

    void LoadSecondarySection( const TiXmlElement * xml, secondary_t & sec )
    {
        if ( xml ) {
            int value;
            xml->Attribute( "archery", &value );
            sec.archery = value;
            xml->Attribute( "ballistics", &value );
            sec.ballistics = value;
            xml->Attribute( "diplomacy", &value );
            sec.diplomacy = value;
            xml->Attribute( "eagleeye", &value );
            sec.eagleeye = value;
            xml->Attribute( "estates", &value );
            sec.estates = value;
            xml->Attribute( "leadership", &value );
            sec.leadership = value;
            xml->Attribute( "logistics", &value );
            sec.logistics = value;
            xml->Attribute( "luck", &value );
            sec.luck = value;
            xml->Attribute( "mysticism", &value );
            sec.mysticism = value;
            xml->Attribute( "navigation", &value );
            sec.navigation = value;
            xml->Attribute( "necromancy", &value );
            sec.necromancy = value;
            xml->Attribute( "pathfinding", &value );
            sec.pathfinding = value;
            xml->Attribute( "scouting", &value );
            sec.scouting = value;
            xml->Attribute( "wisdom", &value );
            sec.wisdom = value;
        }
    }
#endif
}

namespace GameStatic
{
    u8 whirlpool_lost_percent = 50;

    /* town, castle, heroes, artifact_telescope, object_observation_tower, object_magi_eyes */
    u8 overview_distance[] = {4, 5, 4, 1, 20, 9};

    u8 gameover_lost_days = 7;

    // kingdom
    u8 kingdom_max_heroes = 8;
    cost_t kingdom_starting_resource[] = {{10000, 30, 10, 30, 10, 10, 10},
                                          {7500, 20, 5, 20, 5, 5, 5},
                                          {5000, 10, 2, 10, 2, 2, 2},
                                          {2500, 5, 0, 5, 0, 0, 0},
                                          {0, 0, 0, 0, 0, 0, 0},
                                          // ai resource
                                          {10000, 30, 10, 30, 10, 10, 10}};

    // castle
    u8 castle_grown_well = 2;
    u8 castle_grown_wel2 = 8;
    u8 castle_grown_week_of = 5;
    u8 castle_grown_month_of = 100;

    u8 mageguild_restore_spell_points_day[] = {20, 40, 60, 80, 100};

    // heroes
    u8 heroes_spell_points_day = 1;

    // spells
    u16 spell_dd_distance = 0;
    u16 spell_dd_sp = 0;
    u16 spell_dd_hp = 0;

    // monsters
    float monster_upgrade_ratio = 1.0;

    // visit objects mod:	OBJ_BUOY, OBJ_OASIS, OBJ_WATERINGHOLE, OBJ_TEMPLE, OBJ_GRAVEYARD, OBJ_DERELICTSHIP,
    //			        OBJ_SHIPWRECK, OBJ_MERMAID, OBJ_FAERIERING, OBJ_FOUNTAIN, OBJ_IDOL, OBJ_PYRAMID
    s8 objects_mod[] = {1, 1, 1, 2, -1, -1, -1, 1, 1, 1, 1, -2};

    // world
    u32 uniq = 0;
}

StreamBase & GameStatic::operator<<( StreamBase & msg, const Data & /*obj*/ )
{
    msg << whirlpool_lost_percent << kingdom_max_heroes << castle_grown_well << castle_grown_wel2 << castle_grown_week_of << castle_grown_month_of
        << heroes_spell_points_day << gameover_lost_days << spell_dd_distance << spell_dd_sp << spell_dd_hp;

    u8 array_size = ARRAY_COUNT( overview_distance );
    msg << array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg << overview_distance[ii];

    array_size = ARRAY_COUNT( kingdom_starting_resource );
    msg << array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg << kingdom_starting_resource[ii];

    array_size = ARRAY_COUNT( mageguild_restore_spell_points_day );
    msg << array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg << mageguild_restore_spell_points_day[ii];

    array_size = ARRAY_COUNT( objects_mod );
    msg << array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg << objects_mod[ii];

    msg << monster_upgrade_ratio << uniq;

    // skill statics
    array_size = ARRAY_COUNT( Skill::_stats );
    msg << array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg << Skill::_stats[ii];

    array_size = ARRAY_COUNT( Skill::_values );
    msg << array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg << Skill::_values[ii];

    msg << Skill::_from_witchs_hut;

    return msg;
}

StreamBase & GameStatic::operator>>( StreamBase & msg, Data & /*obj*/ )
{
    msg >> whirlpool_lost_percent >> kingdom_max_heroes >> castle_grown_well >> castle_grown_wel2 >> castle_grown_week_of >> castle_grown_month_of
        >> heroes_spell_points_day >> gameover_lost_days >> spell_dd_distance >> spell_dd_sp >> spell_dd_hp;

    u8 array_size = 0;

    msg >> array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg >> overview_distance[ii];

    msg >> array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg >> kingdom_starting_resource[ii];

    msg >> array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg >> mageguild_restore_spell_points_day[ii];

    msg >> array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg >> objects_mod[ii];

    msg >> monster_upgrade_ratio >> uniq;

    msg >> array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg >> Skill::_stats[ii];

    msg >> array_size;
    for ( u32 ii = 0; ii < array_size; ++ii )
        msg >> Skill::_values[ii];

    msg >> Skill::_from_witchs_hut;

    return msg;
}

float GameStatic::GetMonsterUpgradeRatio( void )
{
    return monster_upgrade_ratio;
}

u32 GameStatic::GetLostOnWhirlpoolPercent( void )
{
    return whirlpool_lost_percent;
}

u32 GameStatic::GetOverViewDistance( u32 d )
{
    return d >= ARRAY_COUNT( overview_distance ) ? 0 : overview_distance[d];
}

u32 GameStatic::GetGameOverLostDays( void )
{
    return gameover_lost_days;
}

u32 GameStatic::GetHeroesRestoreSpellPointsPerDay( void )
{
    return heroes_spell_points_day;
}

u32 GameStatic::GetMageGuildRestoreSpellPointsPercentDay( int level )
{
    return level && level < 6 ? mageguild_restore_spell_points_day[level - 1] : 0;
}

u32 GameStatic::GetKingdomMaxHeroes( void )
{
    return kingdom_max_heroes;
}

u32 GameStatic::GetCastleGrownWell( void )
{
    return castle_grown_well;
}

u32 GameStatic::GetCastleGrownWel2( void )
{
    return castle_grown_wel2;
}

u32 GameStatic::GetCastleGrownWeekOf( void )
{
    return castle_grown_week_of;
}

u32 GameStatic::GetCastleGrownMonthOf( void )
{
    return castle_grown_month_of;
}

s32 GameStatic::ObjectVisitedModifiers( int obj )
{
    switch ( obj ) {
    case MP2::OBJ_BUOY:
        return objects_mod[0];
    case MP2::OBJ_OASIS:
        return objects_mod[1];
    case MP2::OBJ_WATERINGHOLE:
        return objects_mod[2];
    case MP2::OBJ_TEMPLE:
        return objects_mod[3];
    case MP2::OBJ_GRAVEYARD:
        return objects_mod[4];
    case MP2::OBJ_DERELICTSHIP:
        return objects_mod[5];
    case MP2::OBJ_SHIPWRECK:
        return objects_mod[6];
    case MP2::OBJ_MERMAID:
        return objects_mod[7];
    case MP2::OBJ_FAERIERING:
        return objects_mod[8];
    case MP2::OBJ_FOUNTAIN:
        return objects_mod[9];
    case MP2::OBJ_IDOL:
        return objects_mod[10];
    case MP2::OBJ_PYRAMID:
        return objects_mod[11];
    default:
        break;
    }

    return 0;
}

u32 GameStatic::Spell_DD_Distance( void )
{
    return spell_dd_distance;
}

u32 GameStatic::Spell_DD_SP( void )
{
    return spell_dd_sp;
}

u32 GameStatic::Spell_DD_HP( void )
{
    return spell_dd_hp;
}

void GameStatic::SetSpell_DD_Distance( int v )
{
    spell_dd_distance = v;
}

void GameStatic::SetSpell_DD_SP( int v )
{
    spell_dd_sp = v;
}

void GameStatic::SetSpell_DD_HP( int v )
{
    spell_dd_hp = v;
}

const Skill::stats_t * GameStatic::GetSkillStats( int race )
{
    switch ( race ) {
    case Race::KNGT:
        return &Skill::_stats[0];
    case Race::BARB:
        return &Skill::_stats[1];
    case Race::SORC:
        return &Skill::_stats[2];
    case Race::WRLK:
        return &Skill::_stats[3];
    case Race::WZRD:
        return &Skill::_stats[4];
    case Race::NECR:
        return &Skill::_stats[5];
    default:
        break;
    }

    return NULL;
}

const Skill::values_t * GameStatic::GetSkillValues( int type )
{
    switch ( type ) {
    case Skill::Secondary::PATHFINDING:
        return &Skill::_values[0];
    case Skill::Secondary::ARCHERY:
        return &Skill::_values[1];
    case Skill::Secondary::LOGISTICS:
        return &Skill::_values[2];
    case Skill::Secondary::SCOUTING:
        return &Skill::_values[3];
    case Skill::Secondary::DIPLOMACY:
        return &Skill::_values[4];
    case Skill::Secondary::NAVIGATION:
        return &Skill::_values[5];
    case Skill::Secondary::LEADERSHIP:
        return &Skill::_values[6];
    case Skill::Secondary::WISDOM:
        return &Skill::_values[7];
    case Skill::Secondary::MYSTICISM:
        return &Skill::_values[8];
    case Skill::Secondary::LUCK:
        return &Skill::_values[9];
    case Skill::Secondary::BALLISTICS:
        return &Skill::_values[10];
    case Skill::Secondary::EAGLEEYE:
        return &Skill::_values[11];
    case Skill::Secondary::NECROMANCY:
        return &Skill::_values[12];
    case Skill::Secondary::ESTATES:
        return &Skill::_values[13];
    default:
        break;
    }

    return NULL;
}

const Skill::secondary_t * GameStatic::GetSkillForWitchsHut( void )
{
    return &Skill::_from_witchs_hut;
}

/*
 */

#ifdef WITH_XML
void Game::CastleUpdateGrowth( const TiXmlElement * xml )
{
    if ( xml ) {
        int value;
        xml->Attribute( "well", &value );
        GameStatic::castle_grown_well = value > 255 ? 255 : value;

        xml->Attribute( "wel2", &value );
        GameStatic::castle_grown_wel2 = value > 255 ? 255 : value;

        xml->Attribute( "week_of", &value );
        GameStatic::castle_grown_week_of = value > 255 ? 255 : value;

        xml->Attribute( "month_of", &value );
        GameStatic::castle_grown_month_of = value > 255 ? 255 : value;
    }
}

void Game::KingdomUpdateStartingResource( const TiXmlElement * xml )
{
    if ( xml ) {
        const TiXmlElement * xml_difficult;
        const char * ai_always = xml->Attribute( "ai_always" );
        const char * level[] = {"easy", "normal", "hard", "expert", "impossible", NULL};

        for ( u32 ii = 0; ii < 5; ++ii ) {
            if ( NULL != ( xml_difficult = xml->FirstChildElement( level[ii] ) ) ) {
                LoadCostFromXMLElement( GameStatic::kingdom_starting_resource[ii], *xml_difficult );
                if ( ai_always && 0 == std::strcmp( ai_always, level[ii] ) )
                    LoadCostFromXMLElement( GameStatic::kingdom_starting_resource[5], *xml_difficult );
            }
        }
    }
}

void Game::KingdomUpdateStatic( const TiXmlElement * xml )
{
    if ( xml ) {
        int value;
        xml->Attribute( "max_heroes", &value );
        GameStatic::kingdom_max_heroes = value;
    }
}

void Game::HeroesUpdateStatic( const TiXmlElement * xml )
{
    if ( xml ) {
        int value;
        xml->Attribute( "spell_points_per_day", &value );
        if ( value < 11 )
            GameStatic::heroes_spell_points_day = value;
    }
}

void Game::GameOverUpdateStatic( const TiXmlElement * xml )
{
    if ( xml ) {
        int value;
        xml->Attribute( "lost_towns_days", &value );
        GameStatic::gameover_lost_days = value;
    }
}

void Game::OverViewUpdateStatic( const TiXmlElement * xml )
{
    if ( xml ) {
        int value;
        xml->Attribute( "town", &value );
        if ( value )
            GameStatic::overview_distance[0] = value;

        xml->Attribute( "castle", &value );
        if ( value )
            GameStatic::overview_distance[1] = value;

        xml->Attribute( "heroes", &value );
        if ( value )
            GameStatic::overview_distance[2] = value;

        xml->Attribute( "artifact_telescope", &value );
        if ( value )
            GameStatic::overview_distance[3] = value;

        xml->Attribute( "object_observation_tower", &value );
        if ( value )
            GameStatic::overview_distance[4] = value;

        xml->Attribute( "object_magi_eyes", &value );
        if ( value )
            GameStatic::overview_distance[5] = value;
    }
}

void Game::WhirlpoolUpdateStatic( const TiXmlElement * xml )
{
    if ( xml ) {
        int value;
        xml->Attribute( "percent", &value );
        GameStatic::whirlpool_lost_percent = 0 < value && value < 90 ? value : 50;
    }
}

void Game::MonsterUpdateStatic( const TiXmlElement * xml )
{
    if ( xml ) {
        double res;
        xml->Attribute( "rate", &res );
        GameStatic::monster_upgrade_ratio = static_cast<float>( res );
    }
}

void Game::SkillUpdateStatic( const TiXmlElement * xml )
{
    if ( xml ) {
        const TiXmlElement * xml_captain = xml->FirstChildElement( "captain" );
        const TiXmlElement * xml_initial = xml->FirstChildElement( "initial" );
        const TiXmlElement * xml_maturity = xml->FirstChildElement( "maturity" );
        const TiXmlElement * xml_secondary = xml_maturity ? xml_maturity->FirstChildElement( "secondary" ) : NULL;
        const TiXmlElement * xml_primary = xml_maturity ? xml_maturity->FirstChildElement( "primary" ) : NULL;
        const TiXmlElement * xml_under = xml_primary ? xml_primary->FirstChildElement( "under" ) : NULL;
        const TiXmlElement * xml_over = xml_primary ? xml_primary->FirstChildElement( "over" ) : NULL;
        Skill::stats_t * ptr = &Skill::_stats[0];
        int value;

        while ( ptr->id ) {
            const TiXmlElement * initial_race = xml_initial ? xml_initial->FirstChildElement( ptr->id ) : NULL;

            if ( initial_race ) {
                LoadPrimarySection( initial_race, ptr->initial_primary );
                LoadSecondarySection( initial_race, ptr->initial_secondary );

                initial_race->Attribute( "book", &value );
                ptr->initial_book = value;
                initial_race->Attribute( "spell", &value );
                ptr->initial_spell = value;
            }

            const TiXmlElement * captain_race = xml_captain ? xml_captain->FirstChildElement( ptr->id ) : NULL;
            if ( captain_race )
                LoadPrimarySection( captain_race, ptr->captain_primary );

            const TiXmlElement * under_race = xml_under ? xml_under->FirstChildElement( ptr->id ) : NULL;
            if ( under_race )
                LoadPrimarySection( under_race, ptr->mature_primary_under );

            const TiXmlElement * over_race = xml_over ? xml_over->FirstChildElement( ptr->id ) : NULL;
            if ( over_race ) {
                LoadPrimarySection( over_race, ptr->mature_primary_over );
                over_race->Attribute( "level", &value );
                if ( value )
                    ptr->over_level = value;
            }

            const TiXmlElement * secondary_race = xml_secondary ? xml_secondary->FirstChildElement( ptr->id ) : NULL;
            if ( secondary_race )
                LoadSecondarySection( secondary_race, ptr->mature_secondary );

            ++ptr;
        }

        xml_secondary = xml->FirstChildElement( "secondary" );
        if ( xml_secondary ) {
            Skill::values_t * ptr2 = &Skill::_values[0];

            while ( ptr2->id ) {
                const TiXmlElement * xml_sec = xml_secondary->FirstChildElement( ptr2->id );

                if ( xml_sec ) {
                    xml_sec->Attribute( "basic", &value );
                    ptr2->values.basic = value;
                    xml_sec->Attribute( "advanced", &value );
                    ptr2->values.advanced = value;
                    xml_sec->Attribute( "expert", &value );
                    ptr2->values.expert = value;
                }

                ++ptr2;
            }
        }

        const TiXmlElement * xml_witch_huts = xml->FirstChildElement( "witch_huts" );

        if ( xml_witch_huts )
            LoadSecondarySection( xml_witch_huts, Skill::_from_witchs_hut );
    }
}

void Skill::UpdateStats( const std::string & spec )
{
    TiXmlDocument doc;

    if ( doc.LoadFile( spec.c_str() ) )
        Game::SkillUpdateStatic( doc.FirstChildElement( "skills" ) );
}

#else
void Skill::UpdateStats( const std::string & ) {}
#endif

GameStatic::Data & GameStatic::Data::Get( void )
{
    static Data gds;
    return gds;
}

int GameStatic::GetBattleMoatReduceDefense( void )
{
    return 3;
}
