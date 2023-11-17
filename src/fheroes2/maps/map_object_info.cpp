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

#include "map_object_info.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <utility>

#include "artifact.h"
#include "monster.h"
#include "resource.h"

namespace
{
    // This is the main container that holds information about all Adventure Map objects in the game.
    //
    // All object information is based on The Price of Loyalty expansion of the original game since
    // the fheroes2 Editor requires to have resources from the expansion.
    std::array<std::vector<Maps::ObjectInfo>, static_cast<size_t>( Maps::ObjectGroup::Group_Count )> objectData;

    void populateArtifactData( std::vector<Maps::ObjectInfo> & objects )
    {
        // All artifacts before Magic Book have their own images.
        // Magic Book is not present in the ICN resources but it is present in artifact IDs.
        std::vector<uint32_t> imageIndices;

        auto addArtifactObject = [&objects]( const uint32_t artifactId, const MP2::MapObjectType ObjectType, const uint32_t mainIcnIndex ) {
            Maps::ObjectInfo object{ ObjectType };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, mainIcnIndex, fheroes2::Point{ 0, 0 }, ObjectType, Maps::OBJECT_LAYER );
            object.metadata[0] = artifactId;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, mainIcnIndex - 1U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        };

        for ( uint32_t artifactId = Artifact::ARCANE_NECKLACE; artifactId < Artifact::MAGIC_BOOK; ++artifactId ) {
            addArtifactObject( artifactId, MP2::OBJ_ARTIFACT, artifactId * 2U - 1U );
        }

        // Assign temporary sprites for the Magic Book.
        addArtifactObject( Artifact::MAGIC_BOOK, MP2::OBJ_ARTIFACT, 207U );

        for ( uint32_t artifactId = Artifact::SPELL_SCROLL; artifactId < Artifact::ARTIFACT_COUNT; ++artifactId ) {
            addArtifactObject( artifactId, MP2::OBJ_ARTIFACT, artifactId * 2U - 1U );
        }

        // Random artifacts.
        addArtifactObject( Artifact::UNKNOWN, MP2::OBJ_RANDOM_ARTIFACT, 163U );
        addArtifactObject( Artifact::UNKNOWN, MP2::OBJ_RANDOM_ARTIFACT_MINOR, 167U );
        addArtifactObject( Artifact::UNKNOWN, MP2::OBJ_RANDOM_ARTIFACT_MAJOR, 169U );
        addArtifactObject( Artifact::UNKNOWN, MP2::OBJ_RANDOM_ARTIFACT_TREASURE, 171U );

        // The random Ultimate artifact does not have a shadow in original assets.
        // We temporary use an empty shadow image from the Spell Scroll artifact for this case.
        {
            Maps::ObjectInfo object{ MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 164U, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 172U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateHeroData( std::vector<Maps::ObjectInfo> & objects )
    {
        // Only 7 races and 6 colors exist in the game.
        for ( int32_t color = 0; color < 6; ++color ) {
            for ( int32_t race = 0; race < 7; ++race ) {
                Maps::ObjectInfo object{ MP2::OBJ_HEROES };
                object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_MINIHERO, color * 7 + race, fheroes2::Point{ 0, 0 }, MP2::OBJ_HEROES, Maps::OBJECT_LAYER );
                object.metadata[0] = color;

                if ( race == 6 ) {
                    // We need to set as a random race, not multi.
                    ++race;
                }

                object.metadata[1] = race;

                objects.emplace_back( std::move( object ) );
            }
        }
    }

    void populateMonsterData( std::vector<Maps::ObjectInfo> & objects )
    {
        // Monsters are "unique" objects in terms of their ICN resources.
        // The Editor uses ICN::MON32 while the Adventure Map renderer uses modified ICN::MINIMON resources.
        for ( int32_t monsterId = Monster::PEASANT; monsterId <= Monster::WATER_ELEMENT; ++monsterId ) {
            Maps::ObjectInfo object{ MP2::OBJ_MONSTER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_MONS32, monsterId - 1, fheroes2::Point{ 0, 0 }, MP2::OBJ_MONSTER, Maps::OBJECT_LAYER );
            object.metadata[0] = monsterId;

            objects.emplace_back( std::move( object ) );
        }

        // Random monsters.
        {
            Maps::ObjectInfo object{ MP2::OBJ_RANDOM_MONSTER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_MONS32, Monster::RANDOM_MONSTER - 1, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_MONSTER,
                                                  Maps::OBJECT_LAYER );
            object.metadata[0] = Monster::RANDOM_MONSTER;

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_RANDOM_MONSTER_WEAK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_MONS32, Monster::RANDOM_MONSTER_LEVEL_1 - 1, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_MONSTER_WEAK,
                                                  Maps::OBJECT_LAYER );
            object.metadata[0] = Monster::RANDOM_MONSTER_LEVEL_1;

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_RANDOM_MONSTER_MEDIUM };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_MONS32, Monster::RANDOM_MONSTER_LEVEL_2 - 1, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_MONSTER_MEDIUM,
                                                  Maps::OBJECT_LAYER );
            object.metadata[0] = Monster::RANDOM_MONSTER_LEVEL_2;

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_RANDOM_MONSTER_STRONG };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_MONS32, Monster::RANDOM_MONSTER_LEVEL_3 - 1, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_MONSTER_STRONG,
                                                  Maps::OBJECT_LAYER );
            object.metadata[0] = Monster::RANDOM_MONSTER_LEVEL_3;

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_RANDOM_MONSTER_VERY_STRONG };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_MONS32, Monster::RANDOM_MONSTER_LEVEL_4 - 1, fheroes2::Point{ 0, 0 },
                                                  MP2::OBJ_RANDOM_MONSTER_VERY_STRONG, Maps::OBJECT_LAYER );
            object.metadata[0] = Monster::RANDOM_MONSTER_LEVEL_4;

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateTreasureData( std::vector<Maps::ObjectInfo> & objects )
    {
        // Normal resources.
        int32_t resourceImageIndex = 1;
        for ( const uint32_t resource : { Resource::WOOD, Resource::MERCURY, Resource::ORE, Resource::SULFUR, Resource::CRYSTAL, Resource::GEMS, Resource::GOLD } ) {
            Maps::ObjectInfo object{ MP2::OBJ_RESOURCE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, resourceImageIndex, fheroes2::Point{ 0, 0 }, MP2::OBJ_RESOURCE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, resourceImageIndex - 1, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.metadata[0] = resource;

            objects.emplace_back( std::move( object ) );

            resourceImageIndex += 2;
        }

        // Genie's Lamp.
        {
            Maps::ObjectInfo object{ MP2::OBJ_GENIE_LAMP };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 15, fheroes2::Point{ 0, 0 }, MP2::OBJ_GENIE_LAMP, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 14, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Random resource.
        {
            Maps::ObjectInfo object{ MP2::OBJ_RANDOM_RESOURCE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 17, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_RESOURCE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 16, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Treasure chest.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREASURE_CHEST };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 19, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREASURE_CHEST, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 18, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateOceanObjectData( std::vector<Maps::ObjectInfo> & objects )
    {
        // Bottle.
        {
            Maps::ObjectInfo object{ MP2::OBJ_BOTTLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 0, fheroes2::Point{ 0, 0 }, MP2::OBJ_BOTTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 11;

            objects.emplace_back( std::move( object ) );
        }

        // Sea chest.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SEA_CHEST };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 19, fheroes2::Point{ 0, 0 }, MP2::OBJ_SEA_CHEST, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 12, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Flotsam.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FLOTSAM };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 45, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOTSAM, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 38, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Buoy.
        {
            Maps::ObjectInfo object{ MP2::OBJ_BUOY };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 195, fheroes2::Point{ 0, 0 }, MP2::OBJ_BUOY, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 188, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Shipwreck survivor.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHIPWRECK_SURVIVOR };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 111, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHIPWRECK_SURVIVOR, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Magellan's Maps.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MAGELLANS_MAPS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 62, fheroes2::Point{ 0, 0 }, MP2::OBJ_MAGELLANS_MAPS, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 69, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_MAGELLANS_MAPS, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 55, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 54, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_MAGELLANS_MAPS, Maps::OBJECT_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 53, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_MAGELLANS_MAPS, Maps::OBJECT_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 52, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Rock with seagulls.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 182, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 183, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 166, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 118, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 15;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 150, fheroes2::Point{ 1, -1 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.topLevelParts.back().animationFrames = 15;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 134, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.topLevelParts.back().animationFrames = 15;

            objects.emplace_back( std::move( object ) );
        }

        // Rock.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 185, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Rock.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 186, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 187, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Rock.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 2, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 1, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 0, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Whirlpool. The center point is middle lower tile.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WHIRLPOOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 218, fheroes2::Point{ 0, 0 }, MP2::OBJ_WHIRLPOOL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 222, fheroes2::Point{ 1, 0 }, MP2::OBJ_WHIRLPOOL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 214, fheroes2::Point{ -1, 0 }, MP2::OBJ_WHIRLPOOL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 206, fheroes2::Point{ 0, -1 }, MP2::OBJ_WHIRLPOOL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 210, fheroes2::Point{ 1, -1 }, MP2::OBJ_WHIRLPOOL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 202, fheroes2::Point{ -1, -1 }, MP2::OBJ_WHIRLPOOL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;

            objects.emplace_back( std::move( object ) );
        }

        // Shipwreck.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHIPWRECK };

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 241, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHIPWRECK, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 248, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_SHIPWRECK, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 240, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 233, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_SHIPWRECK, Maps::OBJECT_LAYER );
            object.topLevelParts.back().animationFrames = 6;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 226, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_SHIPWRECK, Maps::OBJECT_LAYER );
            object.topLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Derelict Ship.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DERELICT_SHIP };

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 21, fheroes2::Point{ 0, 0 }, MP2::OBJ_DERELICT_SHIP, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 20, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_DERELICT_SHIP, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 22, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_DERELICT_SHIP, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 19, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_DERELICT_SHIP, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 12, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_DERELICT_SHIP, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 10, fheroes2::Point{ 1, -2 }, MP2::OBJ_NON_ACTION_DERELICT_SHIP, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 11, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_DERELICT_SHIP, Maps::OBJECT_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 3, fheroes2::Point{ 0, -2 }, MP2::OBJ_NON_ACTION_DERELICT_SHIP, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Aquatic plants. Terrain object.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 83, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 76, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 90, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Aquatic plants. Terrain object.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 97, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 104, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Mermaid.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MERMAID };

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 37, fheroes2::Point{ 0, 0 }, MP2::OBJ_MERMAID, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 8;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 46, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_MERMAID, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 28, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_MERMAID, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 8;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 10, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 8;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 19, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_MERMAID, Maps::OBJECT_LAYER );
            object.topLevelParts.back().animationFrames = 8;

            objects.emplace_back( std::move( object ) );
        }

        // Sirens.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SIRENS };

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 101, fheroes2::Point{ 0, 0 }, MP2::OBJ_SIRENS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 102, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_SIRENS, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 8;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 92, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_SIRENS, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 8;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 83, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 8;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 47, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 8;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 56, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_SIRENS, Maps::OBJECT_LAYER );
            object.topLevelParts.back().animationFrames = 8;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 65, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_SIRENS, Maps::OBJECT_LAYER );
            object.topLevelParts.back().animationFrames = 8;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 74, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_SIRENS, Maps::OBJECT_LAYER );
            object.topLevelParts.back().animationFrames = 8;

            objects.emplace_back( std::move( object ) );
        }

        // Reefs.
        {
            Maps::ObjectInfo object{ MP2::OBJ_REEFS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 113, fheroes2::Point{ 0, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 114, fheroes2::Point{ 1, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 115, fheroes2::Point{ 2, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 111, fheroes2::Point{ 1, -1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 112, fheroes2::Point{ 2, -1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 116, fheroes2::Point{ 0, 1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 117, fheroes2::Point{ 1, 1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_REEFS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 120, fheroes2::Point{ 0, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 121, fheroes2::Point{ 1, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 122, fheroes2::Point{ 2, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 118, fheroes2::Point{ 0, -1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 119, fheroes2::Point{ 1, -1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 123, fheroes2::Point{ 1, 1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 124, fheroes2::Point{ 2, 1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_REEFS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 125, fheroes2::Point{ 0, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 126, fheroes2::Point{ 1, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 127, fheroes2::Point{ 0, 1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 128, fheroes2::Point{ 1, 1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_REEFS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 129, fheroes2::Point{ 0, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 130, fheroes2::Point{ -1, 1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 131, fheroes2::Point{ 0, 1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_REEFS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 132, fheroes2::Point{ 0, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 133, fheroes2::Point{ 1, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_REEFS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 134, fheroes2::Point{ 0, 0 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 135, fheroes2::Point{ 0, 1 }, MP2::OBJ_REEFS, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateObjectData()
    {
        static bool isPopulated = false;
        if ( isPopulated ) {
            // The object container has been populated. No need to do anything else.
            return;
        }

        // IMPORTANT!!!
        // The order of objects must be preserved. If you want to add a new object, add it to the end of the corresponding container.
        populateArtifactData( objectData[static_cast<size_t>( Maps::ObjectGroup::Artifact )] );
        populateHeroData( objectData[static_cast<size_t>( Maps::ObjectGroup::Hero )] );
        populateMonsterData( objectData[static_cast<size_t>( Maps::ObjectGroup::Monster )] );
        populateTreasureData( objectData[static_cast<size_t>( Maps::ObjectGroup::Treasure )] );
        populateOceanObjectData( objectData[static_cast<size_t>( Maps::ObjectGroup::Ocean_Object )] );

        isPopulated = true;
    }
}

namespace Maps
{
    const std::vector<ObjectInfo> & getObjectsByGroup( const ObjectGroup group )
    {
        assert( group != ObjectGroup::Group_Count );

        populateObjectData();

        return objectData[static_cast<size_t>( group )];
    }

    MP2::MapObjectType getObjectTypeByIcn( const MP2::ObjectIcnType icnType, const uint32_t icnIndex )
    {
        populateObjectData();

        for ( const auto & group : objectData ) {
            for ( const auto & object : group ) {
                assert( !object.groundLevelParts.empty() );
                const auto & info = object.groundLevelParts.front();

                if ( info.icnType == icnType && info.icnIndex == icnIndex ) {
                    return info.objectType;
                }
            }
        }

        return MP2::OBJ_NONE;
    }

    std::vector<fheroes2::Point> getGroundLevelOccupiedTileOffset( const ObjectInfo & info )
    {
        // If this assertion blows up then the object is not formed properly.
        assert( !info.groundLevelParts.empty() );

        std::vector<fheroes2::Point> offsets;
        for ( const auto & objectPart : info.groundLevelParts ) {
            if ( objectPart.layerType == OBJECT_LAYER || objectPart.layerType == BACKGROUND_LAYER ) {
                offsets.push_back( objectPart.tileOffset );
            }
        }

        return offsets;
    }
}
