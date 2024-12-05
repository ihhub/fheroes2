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
#include "resource.h"
#include "skill.h"

class Monster;
class Spell;

namespace MP2
{
    enum MapObjectType : uint16_t;
    enum ObjectIcnType : uint8_t;
}

namespace Maps
{
    class Tile;
    struct ObjectPart;

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
    int32_t getMineSpellIdFromTile( const Tile & tile );
    void setMineSpellOnTile( Tile & tile, const int32_t spellId );
    void removeMineSpellFromTile( Tile & tile );

    Funds getDailyIncomeObjectResources( const Tile & tile );

    // Only for objects which always have spell(s): Shrine and Pyramid.
    Spell getSpellFromTile( const Tile & tile );
    void setSpellOnTile( Tile & tile, const int spellId );

    // Only for MP2::OBJ_MONSTER.
    void setMonsterOnTileJoinCondition( Tile & tile, const int32_t condition );
    bool isMonsterOnTileJoinConditionSkip( const Tile & tile );
    bool isMonsterOnTileJoinConditionFree( const Tile & tile );

    int getColorFromBarrierSprite( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex );
    int getColorFromTravellerTentSprite( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex );

    // Only works for action type of objects (ignoring top layer objects parts).
    const ObjectPart * getObjectPartByActionType( const Tile & tile, const MP2::MapObjectType type );

    Monster getMonsterFromTile( const Tile & tile );

    Artifact getArtifactFromTile( const Tile & tile );

    Skill::Secondary getArtifactSecondarySkillRequirement( const Tile & tile );
    ArtifactCaptureCondition getArtifactCaptureCondition( const Tile & tile );
    Funds getArtifactResourceRequirement( const Tile & tile );

    DaemonCaveCaptureBonus getDaemonCaveBonusType( const Tile & tile );
    Funds getDaemonPaymentCondition( const Tile & tile );

    ShipwreckCaptureCondition getShipwreckCaptureCondition( const Tile & tile );

    Funds getTreeOfKnowledgeRequirement( const Tile & tile );

    // Only for MP2::OBJ_WITCHS_HUT.
    Skill::Secondary getSecondarySkillFromWitchsHut( const Tile & tile );

    void setResourceOnTile( Tile & tile, const int resourceType, uint32_t value );

    Funds getFundsFromTile( const Tile & tile );

    Troop getTroopFromTile( const Tile & tile );

    int getColorFromTile( const Tile & tile );
    void setColorOnTile( Tile & tile, const int color );

    bool doesTileContainValuableItems( const Tile & tile );

    void resetObjectMetadata( Tile & tile );

    uint32_t getMonsterCountFromTile( const Tile & tile );
    void setMonsterCountOnTile( Tile & tile, uint32_t count );

    void updateDwellingPopulationOnTile( Tile & tile, const bool isFirstLoad );

    void updateObjectInfoTile( Tile & tile, const bool isFirstLoad );

    void updateMonsterInfoOnTile( Tile & tile );

    void setMonsterOnTile( Tile & tile, const Monster & mons, const uint32_t count );

    std::pair<int, int> getColorRaceFromHeroSprite( const uint32_t heroSpriteIndex );

    // Checks whether the object to be captured is guarded by its own forces
    // (castle has a hero or garrison, dwelling has creatures, etc)
    bool isCaptureObjectProtected( const Tile & tile );

    // Restores an abandoned mine whose main tile is 'tile', turning it into an ordinary mine that brings
    // resources of type 'resource'. This method updates all sprites and sets object types for non-action
    // tiles. The object type for the action tile (i.e. the main tile) remains unchanged and should be
    // updated separately.
    void restoreAbandonedMine( Tile & tile, const int resource );

    void removeMainObjectFromTile( const Tile & tile );

    bool removeObjectFromTileByType( const Tile & tile, const MP2::MapObjectType objectType );

    bool isClearGround( const Tile & tile );

    // Determine the fog direction in the area between min and max positions for given player(s) color code and store it in corresponding tile data.
    void updateFogDirectionsInArea( const fheroes2::Point & minPos, const fheroes2::Point & maxPos, const int32_t color );

    // The functions below are used only in the map Editor.

    void setTerrainOnTiles( const int32_t startTileId, const int32_t endTileId, const int groundId );
    bool updateRoadOnTile( Tile & tile, const bool setRoad );

    bool setObjectOnTile( Tile & tile, const ObjectInfo & info, const bool updateMapPassabilities );

    // Returns UIDs in given area for all objects in the OBJECT and TERRAIN layers.
    // This function does not take into account object parts in the top layer.
    std::set<uint32_t> getObjectUidsInArea( const int32_t startTileId, const int32_t endTileId );
}
