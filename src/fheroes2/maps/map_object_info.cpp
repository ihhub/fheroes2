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

namespace
{
    // This is the main container that holds information about all Adventure Map objects in the game.
    //
    // All object information is based on The Price of Loyalty expansion of the original game since
    // the fheroes2 Editor requires to have resources from the expansion.
    std::array<std::vector<Maps::ObjectInfo>, static_cast<size_t>( Maps::ObjectGroup::Group_Count )> objectData;

    void populateArtifactData( std::vector<Maps::ObjectInfo> & objects )
    {
        // Put an unknown artifact.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_UNKNOWN, 0, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // All artifacts before Magic Book have their own images.
        // Magic Book is not present in the ICN resources but it is present in artifact IDs.
        std::vector<uint32_t> imageIndices;

        for ( int artifactId = Artifact::UNKNOWN + 1; artifactId < Artifact::MAGIC_BOOK; ++artifactId ) {
            Maps::ObjectInfo object{ MP2::OBJ_ARTIFACT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, artifactId * 2 - 1, fheroes2::Point{ 0, 0 }, MP2::OBJ_ARTIFACT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, artifactId * 2, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.metadata[0] = artifactId;

            objects.emplace_back( std::move( object ) );
        }

        // TODO: temporary assign Magic Book some values.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ARTIFACT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 1, fheroes2::Point{ 0, 0 }, MP2::OBJ_ARTIFACT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 0, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.metadata[0] = Artifact::MAGIC_BOOK;

            objects.emplace_back( std::move( object ) );
        }

        // Random artifacts.
        {
            Maps::ObjectInfo object{ MP2::OBJ_RANDOM_ARTIFACT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 163, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_ARTIFACT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 162, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_RANDOM_ARTIFACT_MINOR };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 167, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_ARTIFACT_MINOR, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 166, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_RANDOM_ARTIFACT_MAJOR };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 169, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_ARTIFACT_MAJOR, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 168, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object{ MP2::OBJ_RANDOM_ARTIFACT_TREASURE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 171, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_ARTIFACT_TREASURE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 170, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        for ( int artifactId = Artifact::SPELL_SCROLL; artifactId < Artifact::ARTIFACT_COUNT; ++artifactId ) {
            const uint32_t imageIndex{ 173U + ( static_cast<uint32_t>( artifactId ) - Artifact::SPELL_SCROLL ) * 2 };
            Maps::ObjectInfo object{ MP2::OBJ_ARTIFACT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, imageIndex, fheroes2::Point{ 0, 0 }, MP2::OBJ_ARTIFACT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, imageIndex - 1, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.metadata[0] = artifactId;

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

    void populateResourceData( std::vector<Maps::ObjectInfo> & objects )
    {
        // Normal resources.
        for ( const uint32_t imageIndex : { 1, 3, 5, 7, 9, 11, 13 } ) {
            Maps::ObjectInfo object{ MP2::OBJ_RESOURCE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, imageIndex, fheroes2::Point{ 0, 0 }, MP2::OBJ_RESOURCE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, imageIndex - 1, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
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
            Maps::ObjectInfo object{ MP2::OBJ_GENIE_LAMP };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 17, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_RESOURCE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 16, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Treasure chest.
        {
            Maps::ObjectInfo object{ MP2::OBJ_GENIE_LAMP };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 19, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREASURE_CHEST, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 18, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateOceanObjectData( std::vector<Maps::ObjectInfo> & objects )
    {
        // TODO: properly populate these objects.

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

        // Rock.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 2, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 1, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 0, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

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
        populateResourceData( objectData[static_cast<size_t>( Maps::ObjectGroup::Resource )] );
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
}
