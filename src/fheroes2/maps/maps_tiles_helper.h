/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#include "army_troop.h"
#include "artifact.h"
#include "mp2.h"
#include "resource.h"
#include "skill.h"

class Monster;
class Spell;

namespace Maps
{
    class Tiles;

    // ATTENTION: If you add any new enumeration make sure that value 0 corresponds to empty / visited object
    //            so we don't need to write special logic in resetObjectInfoOnTile().

    enum class ArtifactCaptureCondition : uint32_t
    {
        NO_CONDITIONS = 0,
        PAY_2000_GOLD = 1,
        PAY_2500_GOLD_AND_3_RESOURCES = 2,
        PAY_3000_GOLD_AND_5_RESOURCES = 3,
        HAVE_WISDOM_SKILL = 4,
        HAVE_LEADERSHIP_SKILL = 5,
        FIGHT_50_ROGUES = 6,
        FIGHT_1_GENIE = 7,
        FIGHT_1_PALADIN = 8,
        FIGHT_1_CYCLOP = 9,
        FIGHT_1_PHOENIX = 10,
        FIGHT_1_GREEN_DRAGON = 11,
        FIGHT_1_TITAN = 12,
        FIGHT_1_BONE_DRAGON = 13,
        CONTAINS_SPELL = 15
    };

    enum class DaemonCaveCaptureBonus : uint32_t
    {
        EMPTY = 0,
        GET_1000_EXPERIENCE = 1,
        GET_1000_EXPERIENCE_AND_2500_GOLD = 2,
        GET_1000_EXPERIENCE_AND_ARTIFACT = 3,
        PAY_2500_GOLD = 4
    };

    enum class ShipwreckCaptureCondition : uint32_t
    {
        EMPTY = 0,
        FIGHT_10_GHOSTS_AND_GET_1000_GOLD = 1,
        FIGHT_15_GHOSTS_AND_GET_2000_GOLD = 2,
        FIGHT_25_GHOSTS_AND_GET_5000_GOLD = 3,
        FIGHT_50_GHOSTS_AND_GET_2000_GOLD_WITH_ARTIFACT = 4
    };

    // Only for MP2::OBJ_MINES.
    int32_t getMineSpellIdFromTile( const Tiles & tile );
    void setMineSpellOnTile( Tiles & tile, const int32_t spellId );
    void removeMineSpellFromTile( Tiles & tile );

    Funds getDailyIncomeObjectResources( const Tiles & tile );

    // Only for objects which always have spell(s): Shrine and Pyramid.
    Spell getSpellFromTile( const Tiles & tile );
    void setSpellOnTile( Tiles & tile, const int spellId );

    // Only for MP2::OBJ_MONSTER.
    void setMonsterOnTileJoinCondition( Tiles & tile, const int32_t condition );
    bool isMonsterOnTileJoinConditionSkip( const Tiles & tile );
    bool isMonsterOnTileJoinConditionFree( const Tiles & tile );

    int getColorFromBarrierSprite( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex );
    int getColorFromTravellerTentSprite( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex );

    Monster getMonsterFromTile( const Tiles & tile );

    Artifact getArtifactFromTile( const Tiles & tile );

    Skill::Secondary getArtifactSecondarySkillRequirement( const Tiles & tile );
    ArtifactCaptureCondition getArtifactCaptureCondition( const Tiles & tile );
    Funds getArtifactResourceRequirement( const Tiles & tile );

    DaemonCaveCaptureBonus getDaemonCaveBonusType( const Tiles & tile );
    Funds getDaemonPaymentCondition( const Tiles & tile );

    ShipwreckCaptureCondition getShipwreckCaptureCondition( const Tiles & tile );

    Funds getTreeOfKnowledgeRequirement( const Tiles & tile );

    // Only for MP2::OBJ_WITCHS_HUT.
    Skill::Secondary getSecondarySkillFromWitchsHut( const Tiles & tile );

    void setResourceOnTile( Tiles & tile, const int resourceType, uint32_t value );

    Funds getFundsFromTile( const Tiles & tile );

    Troop getTroopFromTile( const Tiles & tile );

    int getColorFromTile( const Tiles & tile );
    void setColorOnTile( Tiles & tile, const int color );

    bool doesTileContainValuableItems( const Tiles & tile );

    void resetObjectMetadata( Tiles & tile );

    void resetObjectInfoOnTile( Tiles & tile );

    uint32_t getMonsterCountFromTile( const Tiles & tile );
    void setMonsterCountOnTile( Tiles & tile, uint32_t count );

    void updateDwellingPopulationOnTile( Tiles & tile, const bool isFirstLoad );

    void updateObjectInfoTile( Tiles & tile, const bool isFirstLoad );

    void updateMonsterInfoOnTile( Tiles & tile );

    void setMonsterOnTile( Tiles & tile, const Monster & mons, const uint32_t count );
}
