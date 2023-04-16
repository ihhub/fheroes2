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
#include "pairs.h"
#include "resource.h"
#include "skill.h"

class Monster;
class Spell;

namespace Maps
{
    class Tiles;

    int32_t getMineSpellIdFromTile( const Tiles & tile );
    void setMineSpellOnTile( Tiles & tile, const int32_t spellId );

    Spell getSpellFromTile( const Tiles & tile );
    void setSpellOnTile( Tiles & tile, const int spellId );

    void setMonsterOnTileJoinCondition( Tiles & tile, const int32_t condition );
    bool isMonsterOnTileJoinConditionSkip( const Tiles & tile );
    bool isMonsterOnTileJoinConditionFree( const Tiles & tile );

    int getColorFromBarrierSprite( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex );
    int getColorFromTravellerTentSprite( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex );

    Monster getMonsterFromTile( const Tiles & tile );

    Artifact getArtifactFromTile( const Tiles & tile );

    void setArtifactOnTile( Tiles & tile, const int artifactId );

    uint32_t getGoldAmountFromTile( const Tiles & tile );

    Skill::Secondary getSecondarySkillFromTile( const Tiles & tile );
    void setSecondarySkillOnTile( Tiles & tile, const int skillId );

    ResourceCount getResourcesFromTile( const Tiles & tile );
    void setResourceOnTile( Tiles & tile, const int resourceType, uint32_t value );

    Funds getFundsFromTile( const Tiles & tile );

    Troop getTroopFromTile( const Tiles & tile );

    int getColorFromTile( const Tiles & tile );
    void setColorOnTile( Tiles & tile, const int color );

    bool doesTileContainValuableItems( const Tiles & tile );

    void resetObjectInfoOnTile( Tiles & tile );

    uint32_t getMonsterCountFromTile( const Tiles & tile );
    void setMonsterCountOnTile( Tiles & tile, uint32_t count );

    void updateMonsterPopulationOnTile( Tiles & tile );

    void updateDwellingPopulationOnTile( Tiles & tile, bool isFirstLoad );

    void updateObjectInfoTile( Tiles & tile, const bool isFirstLoad );

    void updateRandomArtifact( Tiles & tile );

    void updateRandomResource( Tiles & tile );

    void updateRandomMonster( Tiles & tile );

    void updateMonsterInfoOnTile( Tiles & tile );

    void setMonsterOnTile( Tiles & tile, const Monster & mons, const uint32_t count );
}
