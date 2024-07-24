/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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
#include <set>
#include <utility>

#include "army_troop.h"
#include "artifact.h"
#include "math_base.h"
#include "mp2.h"
#include "resource.h"
#include "skill.h"

class Monster;
class Spell;

namespace Maps
{
    class Tiles;

    struct ObjectInfo;

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
        FIGHT_1_CYCLOPS = 9,
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

    // Only for MP2::OBJ_MINE.
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

    uint32_t getMonsterCountFromTile( const Tiles & tile );
    void setMonsterCountOnTile( Tiles & tile, uint32_t count );

    void updateDwellingPopulationOnTile( Tiles & tile, const bool isFirstLoad );

    void updateObjectInfoTile( Tiles & tile, const bool isFirstLoad );

    void updateMonsterInfoOnTile( Tiles & tile );

    void setMonsterOnTile( Tiles & tile, const Monster & mons, const uint32_t count );

    std::pair<int, int> getColorRaceFromHeroSprite( const uint32_t heroSpriteIndex );

    // Checks whether the object to be captured is guarded by its own forces
    // (castle has a hero or garrison, dwelling has creatures, etc)
    bool isCaptureObjectProtected( const Tiles & tile );

    // Restores an abandoned mine whose main tile is 'tile', turning it into an ordinary mine that brings
    // resources of type 'resource'. This method updates all sprites and sets object types for non-action
    // tiles. The object type for the action tile (i.e. the main tile) remains unchanged and should be
    // updated separately.
    void restoreAbandonedMine( Tiles & tile, const int resource );

    void removeMainObjectFromTile( const Tiles & tile );

    bool removeObjectFromTileByType( const Tiles & tile, const MP2::MapObjectType objectType );

    bool isClearGround( const Tiles & tile );

    // Determine the fog direction in the area between min and max positions for given player(s) color code and store it in corresponding tile data.
    void updateFogDirectionsInArea( const fheroes2::Point & minPos, const fheroes2::Point & maxPos, const int32_t color );

    // The functions below are used only in the map Editor.

    void setTerrainOnTiles( const int32_t startTileId, const int32_t endTileId, const int groundId );
    bool updateRoadOnTile( Tiles & tile, const bool setRoad );
    bool updateStreamOnTile( Tiles & tile, const bool setStream );

    // Update the existing streams to connect them to the river delta.
    void updateStreamsToDeltaConnection( const Tiles & tile, const int deltaDirection );

    bool setObjectOnTile( Tiles & tile, const ObjectInfo & info, const bool updateMapPassabilities );

    // Returns UIDs in given area for all objects in the OBJECT and TERRAIN layers.
    std::set<uint32_t> getObjectUidsInArea( const int32_t startTileId, const int32_t endTileId );
}
