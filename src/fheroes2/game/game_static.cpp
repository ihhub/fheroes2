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

#include "game_static.h"

#include <array>

#include "heroes.h"
#include "mp2.h"
#include "race.h"
#include "skill.h"
#include "skill_static.h"

namespace
{
    const std::array<Skill::FactionProperties, 7> factionProperties = { { { "knight",
                                                                            { 1, 1, 1, 1 },
                                                                            { 2, 2, 1, 1 },
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
                                                                            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0 },
                                                                            10,
                                                                            { 55, 35, 5, 5 },
                                                                            { 30, 30, 20, 20 },
                                                                            { 3, 3, 2, 1, 2, 3, 3, 2, 1, 3, 0, 4, 4, 1 } },
                                                                          { "sorceress",
                                                                            { 0, 0, 2, 2 },
                                                                            { 0, 0, 2, 3 },
                                                                            15,
                                                                            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1 },
                                                                            10,
                                                                            { 10, 10, 30, 50 },
                                                                            { 20, 20, 30, 30 },
                                                                            { 3, 3, 2, 2, 2, 1, 2, 3, 3, 4, 0, 2, 1, 4 } },
                                                                          { "warlock",
                                                                            { 0, 0, 2, 2 },
                                                                            { 0, 0, 3, 2 },
                                                                            19,
                                                                            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1 },
                                                                            10,
                                                                            { 10, 10, 50, 30 },
                                                                            { 20, 20, 30, 30 },
                                                                            { 1, 3, 2, 3, 2, 1, 2, 1, 3, 2, 1, 2, 4, 5 } },
                                                                          { "wizard",
                                                                            { 0, 0, 2, 2 },
                                                                            { 0, 1, 2, 2 },
                                                                            17,
                                                                            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
                                                                            10,
                                                                            { 10, 10, 40, 40 },
                                                                            { 20, 20, 30, 30 },
                                                                            { 1, 3, 2, 3, 2, 2, 2, 2, 4, 2, 0, 2, 2, 5 } },
                                                                          { "necromancer",
                                                                            { 0, 0, 2, 2 },
                                                                            { 1, 0, 2, 2 },
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
                                                                            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                                                                            10,
                                                                            { 0, 0, 0, 0 },
                                                                            { 0, 0, 0, 0 },
                                                                            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } } };

    const std::array<Skill::SecondarySkillValuesPerLevel, 15> secondarySkillValuesPerLevel = { {
        { "pathfinding", { 25, 50, 100 } },
        { "archery", { 10, 25, 50 } },
        { "logistics", { 10, 20, 30 } },
        { "scouting", { 1, 2, 3 } },
        { "diplomacy", { 25, 50, 100 } },
        { "navigation", { 33, 66, 100 } },
        { "leadership", { 1, 2, 3 } },
        { "wisdom", { 3, 4, 5 } },
        { "mysticism", { 1, 2, 3 } },
        { "luck", { 1, 2, 3 } },
        { "ballistics", { 0, 0, 0 } },
        { "eagleeye", { 20, 30, 40 } },
        { "necromancy", { 10, 20, 30 } },
        { "estates", { 100, 250, 500 } },
        { nullptr, { 0, 0, 0 } },
    } };
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

const Skill::FactionProperties * GameStatic::GetFactionProperties( const int race )
{
    switch ( race ) {
    case Race::KNGT:
        return &factionProperties[0];
    case Race::BARB:
        return &factionProperties[1];
    case Race::SORC:
        return &factionProperties[2];
    case Race::WRLK:
        return &factionProperties[3];
    case Race::WZRD:
        return &factionProperties[4];
    case Race::NECR:
        return &factionProperties[5];
    default:
        break;
    }

    return nullptr;
}

const Skill::SecondarySkillValuesPerLevel * GameStatic::GetSecondarySkillValuesPerLevel( const int skill )
{
    switch ( skill ) {
    case Skill::Secondary::PATHFINDING:
        return &secondarySkillValuesPerLevel[0];
    case Skill::Secondary::ARCHERY:
        return &secondarySkillValuesPerLevel[1];
    case Skill::Secondary::LOGISTICS:
        return &secondarySkillValuesPerLevel[2];
    case Skill::Secondary::SCOUTING:
        return &secondarySkillValuesPerLevel[3];
    case Skill::Secondary::DIPLOMACY:
        return &secondarySkillValuesPerLevel[4];
    case Skill::Secondary::NAVIGATION:
        return &secondarySkillValuesPerLevel[5];
    case Skill::Secondary::LEADERSHIP:
        return &secondarySkillValuesPerLevel[6];
    case Skill::Secondary::WISDOM:
        return &secondarySkillValuesPerLevel[7];
    case Skill::Secondary::MYSTICISM:
        return &secondarySkillValuesPerLevel[8];
    case Skill::Secondary::LUCK:
        return &secondarySkillValuesPerLevel[9];
    case Skill::Secondary::BALLISTICS:
        return &secondarySkillValuesPerLevel[10];
    case Skill::Secondary::EAGLE_EYE:
        return &secondarySkillValuesPerLevel[11];
    case Skill::Secondary::NECROMANCY:
        return &secondarySkillValuesPerLevel[12];
    case Skill::Secondary::ESTATES:
        return &secondarySkillValuesPerLevel[13];
    default:
        break;
    }

    return nullptr;
}

const std::vector<int32_t> & GameStatic::getSecondarySkillsForWitchsHut()
{
    // Every skill except Leadership and Necromancy.
    static const std::vector<int32_t> skills{ Skill::Secondary::PATHFINDING, Skill::Secondary::ARCHERY,    Skill::Secondary::LOGISTICS, Skill::Secondary::SCOUTING,
                                              Skill::Secondary::DIPLOMACY,   Skill::Secondary::NAVIGATION, Skill::Secondary::WISDOM,    Skill::Secondary::MYSTICISM,
                                              Skill::Secondary::LUCK,        Skill::Secondary::BALLISTICS, Skill::Secondary::EAGLE_EYE, Skill::Secondary::ESTATES };

    return skills;
}

int GameStatic::GetBattleMoatReduceDefense()
{
    return 3;
}

uint32_t GameStatic::getCastleWallRangedPenalty()
{
    return 50;
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

bool GameStatic::isHeroWorthyToVisitXanadu( const Heroes & hero )
{
    const uint32_t heroLevel = hero.GetLevel();
    if ( heroLevel > 9 ) {
        return true;
    }

    const uint32_t diplomacyLevel = hero.GetLevelSkill( Skill::Secondary::DIPLOMACY );
    return ( diplomacyLevel == Skill::Level::BASIC && heroLevel > 7 ) || ( diplomacyLevel == Skill::Level::ADVANCED && heroLevel > 5 )
           || ( diplomacyLevel == Skill::Level::EXPERT && heroLevel > 3 );
}
