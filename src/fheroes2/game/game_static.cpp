/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include "game_static.h"
#include "mp2.h"
#include "race.h"
#include "skill.h"
#include "skill_static.h"

namespace Skill
{
    stats_t _stats[] = { { "knight",
                           { 1, 1, 1, 1 },
                           { 2, 2, 1, 1 },
                           0,
                           0,
                           { 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
                           10,
                           { 35, 45, 10, 10 },
                           { 25, 25, 25, 25 },
                           { 2, 4, 3, 1, 3, 5, 3, 1, 1, 2, 0, 3, 2, 2 } },
                         { "barbarian",
                           { 1, 1, 1, 1 },
                           { 3, 1, 1, 1 },
                           0,
                           0,
                           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0 },
                           10,
                           { 55, 35, 5, 5 },
                           { 30, 30, 20, 20 },
                           { 3, 3, 2, 1, 2, 3, 3, 2, 1, 3, 0, 4, 4, 1 } },
                         { "sorceress",
                           { 0, 0, 2, 2 },
                           { 0, 0, 2, 3 },
                           1,
                           15,
                           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1 },
                           10,
                           { 10, 10, 30, 50 },
                           { 20, 20, 30, 30 },
                           { 3, 3, 2, 2, 2, 1, 2, 3, 3, 4, 0, 2, 1, 4 } },
                         { "warlock",
                           { 0, 0, 2, 2 },
                           { 0, 0, 3, 2 },
                           1,
                           19,
                           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1 },
                           10,
                           { 10, 10, 50, 30 },
                           { 20, 20, 30, 30 },
                           { 1, 3, 2, 3, 2, 1, 2, 1, 3, 2, 1, 2, 4, 5 } },
                         { "wizard",
                           { 0, 0, 2, 2 },
                           { 0, 1, 2, 2 },
                           1,
                           17,
                           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
                           10,
                           { 10, 10, 40, 40 },
                           { 20, 20, 30, 30 },
                           { 1, 3, 2, 3, 2, 2, 2, 2, 4, 2, 0, 2, 2, 5 } },
                         { "necromancer",
                           { 0, 0, 2, 2 },
                           { 1, 0, 2, 2 },
                           1,
                           10,
                           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1 },
                           10,
                           { 15, 15, 35, 35 },
                           { 25, 25, 25, 25 },
                           { 1, 3, 2, 3, 2, 0, 2, 1, 3, 2, 5, 3, 1, 4 } },
                         { nullptr,
                           { 0, 0, 0, 0 },
                           { 0, 0, 0, 0 },
                           0,
                           0,
                           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                           10,
                           { 0, 0, 0, 0 },
                           { 0, 0, 0, 0 },
                           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } };

    values_t _values[] = {
        { "pathfinding", { 25, 50, 100 } }, { "archery", { 10, 25, 50 } },     { "logistics", { 10, 20, 30 } }, { "scouting", { 1, 2, 3 } },
        { "diplomacy", { 25, 50, 100 } },   { "navigation", { 33, 66, 100 } }, { "leadership", { 1, 2, 3 } },   { "wisdom", { 3, 4, 5 } },
        { "mysticism", { 2, 3, 4 } },       { "luck", { 1, 2, 3 } },           { "ballistics", { 0, 0, 0 } },   { "eagleeye", { 20, 30, 40 } },
        { "necromancy", { 10, 20, 30 } },   { "estates", { 100, 250, 500 } },  { nullptr, { 0, 0, 0 } },
    };

    secondary_t _from_witchs_hut = {
        /* archery */ 1,   /* ballistics */ 1, /* diplomacy */ 1, /* eagleeye */ 1,
        /* estates */ 1,   /* leadership */ 0, /* logistics */ 1, /* luck */ 1,
        /* mysticism */ 1, /* navigation */ 1, /* necromancy*/ 0, /* pathfinding */ 1,
        /* scouting */ 1,  /* wisdom */ 1 };
}

namespace GameStatic
{
    // world
    uint32_t uniq = 0;
}

uint32_t GameStatic::GetLostOnWhirlpoolPercent()
{
    return 50;
}

uint32_t GameStatic::getFogDiscoveryDistance( const FogDiscoveryType type )
{
    switch ( type ) {
    case FogDiscoveryType::CASTLE:
        return 5;
    case FogDiscoveryType::HEROES:
        return 4;
    case FogDiscoveryType::OBSERVATION_TOWER:
        return 20;
    case FogDiscoveryType::MAGI_EYES:
        return 9;
    default:
        break;
    }

    return 0;
}

uint32_t GameStatic::GetGameOverLostDays()
{
    return 7;
}

uint32_t GameStatic::GetHeroesRestoreSpellPointsPerDay()
{
    return 1;
}

uint32_t GameStatic::GetKingdomMaxHeroes()
{
    return 8;
}

uint32_t GameStatic::GetCastleGrownWell()
{
    return 2;
}

uint32_t GameStatic::GetCastleGrownWel2()
{
    return 8;
}

uint32_t GameStatic::GetCastleGrownWeekOf()
{
    return 5;
}

uint32_t GameStatic::GetCastleGrownMonthOf()
{
    return 100;
}

int32_t GameStatic::ObjectVisitedModifiers( const MP2::MapObjectType objectType )
{
    switch ( objectType ) {
    case MP2::OBJ_BUOY:
    case MP2::OBJ_OASIS:
    case MP2::OBJ_WATERING_HOLE:
    case MP2::OBJ_MERMAID:
    case MP2::OBJ_FAERIE_RING:
    case MP2::OBJ_FOUNTAIN:
    case MP2::OBJ_IDOL:
        return 1;
    case MP2::OBJ_TEMPLE:
        return 2;
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_DERELICT_SHIP:
    case MP2::OBJ_SHIPWRECK:
        return -1;
    case MP2::OBJ_PYRAMID:
        return -2;
    default:
        break;
    }

    return 0;
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

    return nullptr;
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

    return nullptr;
}

const Skill::secondary_t * GameStatic::GetSkillForWitchsHut()
{
    return &Skill::_from_witchs_hut;
}

int GameStatic::GetBattleMoatReduceDefense()
{
    return 3;
}

uint32_t GameStatic::getMovementPointBonus( const MP2::MapObjectType objectType )
{
    switch ( objectType ) {
    case MP2::OBJ_OASIS:
        return 800;
    case MP2::OBJ_STABLES:
    case MP2::OBJ_WATERING_HOLE:
        return 400;
    default:
        break;
    }

    return 0;
}
