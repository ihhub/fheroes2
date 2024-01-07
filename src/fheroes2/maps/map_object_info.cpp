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

#include "map_object_info.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <map>
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
    std::array<std::vector<Maps::ObjectInfo>, static_cast<size_t>( Maps::ObjectGroup::GROUP_COUNT )> objectData;

    std::map<std::pair<MP2::ObjectIcnType, uint32_t>, std::pair<Maps::ObjectGroup, uint32_t>> icnVsObjectInfo;

    void populateRoads( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        for ( uint32_t i = 0; i < 32; ++i ) {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_ROAD, i, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateStreams( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        for ( uint32_t i = 0; i < 13; ++i ) {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_STREAM, i, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateLandscapeMountains( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        (void)objects;
    }

    void populateLandscapeRocks( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        (void)objects;
    }

    void populateLandscapeTrees( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        (void)objects;
    }

    void populateLandscapeWater( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        // Rock with seagulls.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 182, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 183, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 166, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 118, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 15;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 150, fheroes2::Point{ 1, -1 }, MP2::OBJ_ROCK );
            object.topLevelParts.back().animationFrames = 15;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 134, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK );
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
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 0, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK );

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

    void populateLandscapeMiscellaneous( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        (void)objects;
    }

    void populateLandscapeTownBasements( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        // Castle/town basement in the next order: grass, snow, swamp, lava, desert, dirt, wasteland, beach.
        for ( uint8_t basement = 0; basement < 8; ++basement ) {
            const uint8_t icnOffset = basement * 10;
            Maps::ObjectInfo object{ MP2::OBJ_NON_ACTION_CASTLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 2, fheroes2::Point{ 0, 0 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 0, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 1, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 3, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 4, fheroes2::Point{ 2, 0 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 5, fheroes2::Point{ -2, 1 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 6, fheroes2::Point{ -1, 1 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 7, fheroes2::Point{ 0, 1 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 8, fheroes2::Point{ 1, 1 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 9, fheroes2::Point{ 2, 1 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateLandscapeFlags( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        // Castle flags located one tile to the left and right from the castle entrance: blue, green, red, yellow, orange, purple, gray (neutral).
        for ( uint8_t color = 0; color < 7; ++color ) {
            const uint8_t icnOffset = color * 2;

            {
                // Left flag.
                Maps::ObjectInfo object{ MP2::OBJ_NON_ACTION_CASTLE };
                object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_FLAG32, icnOffset, fheroes2::Point{ 0, 0 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );
                object.metadata[0] = color;

                objects.emplace_back( std::move( object ) );
            }

            {
                // Right flag.
                Maps::ObjectInfo object{ MP2::OBJ_NON_ACTION_CASTLE };
                object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_FLAG32, icnOffset + 1, fheroes2::Point{ 0, 0 }, MP2::OBJ_NON_ACTION_CASTLE, Maps::OBJECT_LAYER );
                object.metadata[0] = color;

                objects.emplace_back( std::move( object ) );
            }
        }

        // TODO: Add flags for other capture-able objects.
    }

    void populateAdventureArtifacts( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        // All artifacts before Magic Book have their own images.
        // Magic Book is not present in the ICN resources but it is present in artifact IDs.

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

    void populateAdventureDwellings( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        // Peasant Hut.
        {
            Maps::ObjectInfo object{ MP2::OBJ_PEASANT_HUT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 35U, fheroes2::Point{ 0, 0 }, MP2::OBJ_PEASANT_HUT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 25U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 9;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 15U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 9;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 5U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_PEASANT_HUT );
            object.topLevelParts.back().animationFrames = 9;

            objects.emplace_back( std::move( object ) );
        }

        // Ruins (for Medusa).
        {
            Maps::ObjectInfo object{ MP2::OBJ_RUINS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 73U, fheroes2::Point{ 0, 0 }, MP2::OBJ_RUINS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 74U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_RUINS, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Tree House (for Sprites).
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREE_HOUSE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 114U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREE_HOUSE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 113U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 112U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_TREE_HOUSE );

            objects.emplace_back( std::move( object ) );
        }

        // Watch Tower (for Orcs).
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATCH_TOWER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 116U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATCH_TOWER, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 115U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Watch Tower (for Orcs).
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATCH_TOWER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 138U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATCH_TOWER, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 137U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 136U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_WATCH_TOWER );

            objects.emplace_back( std::move( object ) );
        }

        // Halfling Hole.
        {
            Maps::ObjectInfo object{ MP2::OBJ_HALFLING_HOLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 138U, fheroes2::Point{ 0, 0 }, MP2::OBJ_HALFLING_HOLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 139U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_HALFLING_HOLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 137U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_HALFLING_HOLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 136U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Halfling Hole.
        {
            Maps::ObjectInfo object{ MP2::OBJ_HALFLING_HOLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 7U, fheroes2::Point{ 0, 0 }, MP2::OBJ_HALFLING_HOLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 8U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_HALFLING_HOLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 6U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_HALFLING_HOLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 5U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Tree City (for Sprites).
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREE_CITY };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 151U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREE_CITY, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 152U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_TREE_CITY, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 150U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 148U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_TREE_CITY );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 147U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_TREE_CITY );

            objects.emplace_back( std::move( object ) );
        }

        // Tree City (for Sprites).
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREE_CITY };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 21U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREE_CITY, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 22U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_TREE_CITY, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 20U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 18U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_TREE_CITY );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 17U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_TREE_CITY );

            objects.emplace_back( std::move( object ) );
        }

        // Desert Tent (for Nomads).
        {
            Maps::ObjectInfo object{ MP2::OBJ_DESERT_TENT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 73U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DESERT_TENT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 72U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_DESERT_TENT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 71U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 70U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_DESERT_TENT );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 69U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_DESERT_TENT );

            objects.emplace_back( std::move( object ) );
        }

        // City of the Dead (for Liches).
        {
            Maps::ObjectInfo object{ MP2::OBJ_CITY_OF_DEAD };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 96U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CITY_OF_DEAD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 94U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_CITY_OF_DEAD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 95U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_CITY_OF_DEAD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 97U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_CITY_OF_DEAD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 98U, fheroes2::Point{ 2, 0 }, MP2::OBJ_NON_ACTION_CITY_OF_DEAD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 89U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NON_ACTION_CITY_OF_DEAD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 90U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_CITY_OF_DEAD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 91U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_CITY_OF_DEAD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 92U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_CITY_OF_DEAD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 93U, fheroes2::Point{ 2, -1 }, MP2::OBJ_NON_ACTION_CITY_OF_DEAD, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Excavation (for Skeletons).
        {
            Maps::ObjectInfo object{ MP2::OBJ_EXCAVATION };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 101U, fheroes2::Point{ 0, 0 }, MP2::OBJ_EXCAVATION, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 100U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_EXCAVATION, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 99U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_EXCAVATION, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Troll's Bridge.
        {
            // This is the only action object who's parts should be on background layer.
            Maps::ObjectInfo object{ MP2::OBJ_TROLL_BRIDGE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 189U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TROLL_BRIDGE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 188U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_TROLL_BRIDGE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 187U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_TROLL_BRIDGE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 186U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NON_ACTION_TROLL_BRIDGE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 185U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_TROLL_BRIDGE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 184U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_TROLL_BRIDGE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 183U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NON_ACTION_TROLL_BRIDGE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 182U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NON_ACTION_TROLL_BRIDGE, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Archer's House.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ARCHER_HOUSE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 84U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ARCHER_HOUSE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 77U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 70U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 63U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_ARCHER_HOUSE );

            objects.emplace_back( std::move( object ) );
        }

        // Goblin's Hut.
        {
            Maps::ObjectInfo object{ MP2::OBJ_GOBLIN_HUT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 92U, fheroes2::Point{ 0, 0 }, MP2::OBJ_GOBLIN_HUT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 91U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Dwarf's Cottage.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DWARF_COTTAGE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 114U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DWARF_COTTAGE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 107U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 100U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 93U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_DWARF_COTTAGE );
            object.topLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Dragon City.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DRAGON_CITY };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 54U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DRAGON_CITY, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 51U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NON_ACTION_DRAGON_CITY, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 52U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_DRAGON_CITY, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 53U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_DRAGON_CITY, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 55U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_DRAGON_CITY, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 46U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_DRAGON_CITY, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 49U, fheroes2::Point{ -5, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 50U, fheroes2::Point{ -4, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 42U, fheroes2::Point{ -5, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 43U, fheroes2::Point{ -4, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 44U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NON_ACTION_DRAGON_CITY );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 45U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NON_ACTION_DRAGON_CITY );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 47U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_DRAGON_CITY );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 48U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_DRAGON_CITY );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 37U, fheroes2::Point{ -3, -2 }, MP2::OBJ_NON_ACTION_DRAGON_CITY );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 38U, fheroes2::Point{ -2, -2 }, MP2::OBJ_NON_ACTION_DRAGON_CITY );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 39U, fheroes2::Point{ -1, -2 }, MP2::OBJ_NON_ACTION_DRAGON_CITY );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 40U, fheroes2::Point{ 0, -2 }, MP2::OBJ_NON_ACTION_DRAGON_CITY );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 41U, fheroes2::Point{ 1, -2 }, MP2::OBJ_NON_ACTION_DRAGON_CITY );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 34U, fheroes2::Point{ -2, -3 }, MP2::OBJ_NONE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 35U, fheroes2::Point{ -1, -3 }, MP2::OBJ_NON_ACTION_DRAGON_CITY );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 36U, fheroes2::Point{ 0, -3 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Graveyard (for Zombies).
        {
            Maps::ObjectInfo object{ MP2::OBJ_GRAVEYARD };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 58U, fheroes2::Point{ 0, 0 }, MP2::OBJ_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 57U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 56U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Graveyard (for Zombies).
        {
            Maps::ObjectInfo object{ MP2::OBJ_GRAVEYARD };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 160U, fheroes2::Point{ 0, 0 }, MP2::OBJ_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 159U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 158U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Graveyard (for Zombies).
        {
            Maps::ObjectInfo object{ MP2::OBJ_GRAVEYARD };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 122U, fheroes2::Point{ 0, 0 }, MP2::OBJ_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 121U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 120U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Graveyard (for Zombies).
        {
            Maps::ObjectInfo object{ MP2::OBJ_GRAVEYARD };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 208U, fheroes2::Point{ 0, 0 }, MP2::OBJ_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 209U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 207U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 206U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 205U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_GRAVEYARD );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 204U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_GRAVEYARD );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 203U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_GRAVEYARD );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 202U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Graveyard (for Zombies).
        {
            Maps::ObjectInfo object{ MP2::OBJ_GRAVEYARD };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 209U, fheroes2::Point{ 0, 0 }, MP2::OBJ_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 210U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 208U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 207U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 206U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_GRAVEYARD );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 205U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_GRAVEYARD );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 204U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_GRAVEYARD );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 203U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Wagon Camp (for Rogues).
        {
            Maps::ObjectInfo object{ MP2::OBJ_WAGON_CAMP };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 129U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WAGON_CAMP, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 136U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_WAGON_CAMP, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 128U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_WAGON_CAMP, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 127U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 126U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_WAGON_CAMP );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 125U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_WAGON_CAMP );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 124U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_WAGON_CAMP );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 123U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Cave (for Centaurs).
        {
            Maps::ObjectInfo object{ MP2::OBJ_CAVE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 3U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CAVE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 2U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_CAVE, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateAdventureMines( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        // Mines provides 5 resources: Ore, Sulfur, Crystal, Gems, Gold.
        const uint8_t mineResources = 5;

        // Mines have different appearance: Generic, Grass, Snow, Swamp, Lava, Desert, Dirt, Wasteland.
        for ( const auto & [type, offset] :
              { std::make_pair( MP2::OBJ_ICN_TYPE_MTNMULT, 74U ), std::make_pair( MP2::OBJ_ICN_TYPE_MTNGRAS, 74U ), std::make_pair( MP2::OBJ_ICN_TYPE_MTNSNOW, 74U ),
                std::make_pair( MP2::OBJ_ICN_TYPE_MTNSWMP, 74U ), std::make_pair( MP2::OBJ_ICN_TYPE_MTNLAVA, 74U ), std::make_pair( MP2::OBJ_ICN_TYPE_MTNDSRT, 74U ),
                std::make_pair( MP2::OBJ_ICN_TYPE_MTNDIRT, 104U ), std::make_pair( MP2::OBJ_ICN_TYPE_MTNCRCK, 104U ) } ) {
            Maps::ObjectInfo object{ MP2::OBJ_MINE };
            object.groundLevelParts.emplace_back( type, offset + 8U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 4U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 7U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 9U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 0U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 1U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 5U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 6U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.topLevelParts.emplace_back( type, offset + 2U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_MINE );
            object.topLevelParts.emplace_back( type, offset + 3U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_MINE );

            // The cars with these resources are placed above the main mine tile.
            for ( uint8_t resource = 0; resource < mineResources; ++resource ) {
                object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_EXTRAOVR, resource, fheroes2::Point{ 0, 0 }, MP2::OBJ_MINE, Maps::OBJECT_LAYER );

                // Income per day for all mines except the gold mine.
                object.metadata[1] = 1;

                switch ( resource ) {
                case 0:
                    object.metadata[0] = Resource::ORE;
                    break;
                case 1:
                    object.metadata[0] = Resource::SULFUR;
                    break;
                case 2:
                    object.metadata[0] = Resource::CRYSTAL;
                    break;
                case 3:
                    object.metadata[0] = Resource::GEMS;
                    break;
                case 4:
                    object.metadata[0] = Resource::GOLD;
                    object.metadata[1] = 1000;
                    break;
                default:
                    // Have you added a new mine resource?! Update the logic above!
                    assert( 0 );
                    break;
                }

                if ( resource < mineResources - 1 ) {
                    objects.emplace_back( object );
                    // Remove the resource from the mine to add a new one in the next loop.
                    object.groundLevelParts.pop_back();
                }
            }

            // The last resource with the current mine appearance type.
            objects.emplace_back( std::move( object ) );
        }

        // Abandoned mines are only for Grass and Dirt and they have different tiles number.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ABANDONED_MINE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 6U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ABANDONED_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 3U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_ABANDONED_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 5U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_ABANDONED_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 7U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_ABANDONED_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 0U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 4U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 1U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_ABANDONED_MINE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 2U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_ABANDONED_MINE );

            objects.emplace_back( std::move( object ) );
        }
        {
            Maps::ObjectInfo object{ MP2::OBJ_ABANDONED_MINE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 8U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ABANDONED_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 4U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_ABANDONED_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 7U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_ABANDONED_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 9U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_ABANDONED_MINE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 0U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 1U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 5U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 6U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 2U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_ABANDONED_MINE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 3U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_ABANDONED_MINE );

            objects.emplace_back( std::move( object ) );
        }

        // Sawmills for different terrains: Grass/Swamp, Snow, Lava, Desert, Dirt, Wasteland.
        for ( const auto & [type, offset] : { std::make_pair( MP2::OBJ_ICN_TYPE_OBJNMUL2, 210U ), std::make_pair( MP2::OBJ_ICN_TYPE_OBJNSNOW, 195U ),
                                              std::make_pair( MP2::OBJ_ICN_TYPE_OBJNLAVA, 118U ), std::make_pair( MP2::OBJ_ICN_TYPE_OBJNDSRT, 123U ),
                                              std::make_pair( MP2::OBJ_ICN_TYPE_OBJNMUL2, 74U ), std::make_pair( MP2::OBJ_ICN_TYPE_OBJNCRCK, 239U ) } ) {
            Maps::ObjectInfo object{ MP2::OBJ_SAWMILL };
            object.groundLevelParts.emplace_back( type, offset + 6U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SAWMILL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 2U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_SAWMILL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 3U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_SAWMILL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 4U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_SAWMILL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 5U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_SAWMILL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( type, offset + 7U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_SAWMILL, Maps::OBJECT_LAYER );
            object.topLevelParts.emplace_back( type, offset + 0U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NON_ACTION_SAWMILL );
            object.topLevelParts.emplace_back( type, offset + 1U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_SAWMILL );

            // Set resource type and income per day.
            object.metadata[0] = Resource::WOOD;
            object.metadata[1] = 2;

            objects.emplace_back( std::move( object ) );
        }

        // Alchemist Lab (one for all terrains).
        {
            Maps::ObjectInfo object{ MP2::OBJ_ALCHEMIST_LAB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 26U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ALCHEMIST_LAB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 25U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_ALCHEMIST_LAB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 27U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_ALCHEMIST_LAB, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 24U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 20U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 21U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_ALCHEMIST_LAB );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 22U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_ALCHEMIST_LAB );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 23U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_ALCHEMIST_LAB );

            // Set resource type and income per day.
            object.metadata[0] = Resource::MERCURY;
            object.metadata[1] = 1;

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateAdventurePowerUps( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        (void)objects;
    }

    void populateAdventureTreasures( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

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

        // Campfire.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CAMPFIRE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 131, fheroes2::Point{ 0, 0 }, MP2::OBJ_CAMPFIRE, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 124, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Campfire on show.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CAMPFIRE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 4, fheroes2::Point{ 0, 0 }, MP2::OBJ_CAMPFIRE, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            // The original resources do not have a part of shadow for this object so we use the same shadow from another campfire.
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 54, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Campfire in desert.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CAMPFIRE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 61, fheroes2::Point{ 0, 0 }, MP2::OBJ_CAMPFIRE, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 54, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateAdventureWater( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

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

        // Boat.
        {
            Maps::ObjectInfo object{ MP2::OBJ_BOAT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_BOAT32, 18, fheroes2::Point{ 0, 0 }, MP2::OBJ_BOAT, Maps::OBJECT_LAYER );

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

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 54, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_MAGELLANS_MAPS );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 53, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_MAGELLANS_MAPS );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 52, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE );

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

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 233, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_SHIPWRECK );
            object.topLevelParts.back().animationFrames = 6;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 226, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_SHIPWRECK );
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

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 10, fheroes2::Point{ 1, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 11, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_DERELICT_SHIP );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 3, fheroes2::Point{ 0, -2 }, MP2::OBJ_NON_ACTION_DERELICT_SHIP );

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

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 19, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_MERMAID );
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

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 56, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_SIRENS );
            object.topLevelParts.back().animationFrames = 8;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 65, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_SIRENS );
            object.topLevelParts.back().animationFrames = 8;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 74, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_SIRENS );
            object.topLevelParts.back().animationFrames = 8;

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateAdventureMiscellaneous( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        (void)objects;
    }

    void populateKingdomHeroes( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

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

    void populateKingdomTowns( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        auto addTown = [&objects]( const MP2::MapObjectType mainObjectType, const uint8_t townIcnOffset, const uint8_t shadowIcnOffset,
                                   const MP2::ObjectIcnType townIcnType, const MP2::ObjectIcnType shadowIcnType ) {
            assert( MP2::isActionObject( mainObjectType ) );

            const MP2::MapObjectType secondaryObjectType( static_cast<MP2::MapObjectType>( mainObjectType - MP2::OBJ_ACTION_OBJECT_TYPE ) );

            Maps::ObjectInfo object{ mainObjectType };
            object.groundLevelParts.emplace_back( townIcnType, townIcnOffset + 13, fheroes2::Point{ 0, 0 }, mainObjectType, Maps::OBJECT_LAYER );
            // Castle/town shadow.
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 13, fheroes2::Point{ -3, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 14, fheroes2::Point{ -2, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 15, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 9, fheroes2::Point{ -4, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 10, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 11, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 12, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 4, fheroes2::Point{ -5, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 5, fheroes2::Point{ -4, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 6, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 7, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 8, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 0, fheroes2::Point{ -4, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 1, fheroes2::Point{ -3, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 2, fheroes2::Point{ -2, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( shadowIcnType, shadowIcnOffset + 3, fheroes2::Point{ -1, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            // Castle/town main sprite.
            object.groundLevelParts.emplace_back( townIcnType, townIcnOffset + 11, fheroes2::Point{ -2, 0 }, secondaryObjectType, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( townIcnType, townIcnOffset + 12, fheroes2::Point{ -1, 0 }, secondaryObjectType, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( townIcnType, townIcnOffset + 14, fheroes2::Point{ 1, 0 }, secondaryObjectType, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( townIcnType, townIcnOffset + 15, fheroes2::Point{ 2, 0 }, secondaryObjectType, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( townIcnType, townIcnOffset + 7, fheroes2::Point{ -1, -1 }, secondaryObjectType, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( townIcnType, townIcnOffset + 8, fheroes2::Point{ 0, -1 }, secondaryObjectType, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( townIcnType, townIcnOffset + 9, fheroes2::Point{ 1, -1 }, secondaryObjectType, Maps::OBJECT_LAYER );
            object.topLevelParts.emplace_back( townIcnType, townIcnOffset + 6, fheroes2::Point{ -2, -1 }, secondaryObjectType );
            object.topLevelParts.emplace_back( townIcnType, townIcnOffset + 10, fheroes2::Point{ 2, -1 }, secondaryObjectType );
            object.topLevelParts.emplace_back( townIcnType, townIcnOffset + 1, fheroes2::Point{ -2, -2 }, secondaryObjectType );
            object.topLevelParts.emplace_back( townIcnType, townIcnOffset + 2, fheroes2::Point{ -1, -2 }, secondaryObjectType );
            object.topLevelParts.emplace_back( townIcnType, townIcnOffset + 3, fheroes2::Point{ 0, -2 }, secondaryObjectType );
            object.topLevelParts.emplace_back( townIcnType, townIcnOffset + 4, fheroes2::Point{ 1, -2 }, secondaryObjectType );
            object.topLevelParts.emplace_back( townIcnType, townIcnOffset + 5, fheroes2::Point{ 2, -2 }, secondaryObjectType );
            object.topLevelParts.emplace_back( townIcnType, townIcnOffset + 0, fheroes2::Point{ 0, -3 }, secondaryObjectType );

            objects.emplace_back( std::move( object ) );
        };

        // Castle and town sprites: Knight, Barbarian, Sorceress, Warlock, Wizard, Necromancer.
        // First goes castle, then town, then the next race.
        for ( uint8_t race = 0; race < 6 * 2; ++race ) {
            const uint8_t icnOffset = race * 16;

            addTown( MP2::OBJ_CASTLE, icnOffset, icnOffset, MP2::OBJ_ICN_TYPE_OBJNTOWN, MP2::OBJ_ICN_TYPE_OBJNTWSH );
        }

        // Random castle/town
        for ( uint8_t i = 0; i < 2; ++i ) {
            const uint8_t icnOffset = i * 16;
            const MP2::MapObjectType mainObjectType = ( i == 0 ) ? MP2::OBJ_RANDOM_CASTLE : MP2::OBJ_RANDOM_TOWN;

            addTown( mainObjectType, icnOffset, icnOffset + 32, MP2::OBJ_ICN_TYPE_OBJNTWRD, MP2::OBJ_ICN_TYPE_OBJNTWRD );
        }
    }

    void populateMonsters( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

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

    void populateObjectData()
    {
        static bool isPopulated = false;
        if ( isPopulated ) {
            // The object container has been populated. No need to do anything else.
            return;
        }

        // IMPORTANT!!!
        // The order of objects must be preserved. If you want to add a new object, add it to the end of the corresponding container.
        populateRoads( objectData[static_cast<size_t>( Maps::ObjectGroup::ROADS )] );
        populateStreams( objectData[static_cast<size_t>( Maps::ObjectGroup::STREAMS )] );

        populateLandscapeMountains( objectData[static_cast<size_t>( Maps::ObjectGroup::LANDSCAPE_MOUNTAINS )] );
        populateLandscapeRocks( objectData[static_cast<size_t>( Maps::ObjectGroup::LANDSCAPE_ROCKS )] );
        populateLandscapeTrees( objectData[static_cast<size_t>( Maps::ObjectGroup::LANDSCAPE_TREES )] );
        populateLandscapeWater( objectData[static_cast<size_t>( Maps::ObjectGroup::LANDSCAPE_WATER )] );
        populateLandscapeMiscellaneous( objectData[static_cast<size_t>( Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS )] );

        // These are the extra objects used with the others and never used alone.
        populateLandscapeTownBasements( objectData[static_cast<size_t>( Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS )] );
        populateLandscapeFlags( objectData[static_cast<size_t>( Maps::ObjectGroup::LANDSCAPE_FLAGS )] );

        populateAdventureArtifacts( objectData[static_cast<size_t>( Maps::ObjectGroup::ADVENTURE_ARTIFACTS )] );
        populateAdventureDwellings( objectData[static_cast<size_t>( Maps::ObjectGroup::ADVENTURE_DWELLINGS )] );
        populateAdventureMines( objectData[static_cast<size_t>( Maps::ObjectGroup::ADVENTURE_MINES )] );
        populateAdventurePowerUps( objectData[static_cast<size_t>( Maps::ObjectGroup::ADVENTURE_POWER_UPS )] );
        populateAdventureTreasures( objectData[static_cast<size_t>( Maps::ObjectGroup::ADVENTURE_TREASURES )] );
        populateAdventureWater( objectData[static_cast<size_t>( Maps::ObjectGroup::ADVENTURE_WATER )] );
        populateAdventureMiscellaneous( objectData[static_cast<size_t>( Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS )] );

        populateKingdomHeroes( objectData[static_cast<size_t>( Maps::ObjectGroup::KINGDOM_HEROES )] );
        populateKingdomTowns( objectData[static_cast<size_t>( Maps::ObjectGroup::KINGDOM_TOWNS )] );

        populateMonsters( objectData[static_cast<size_t>( Maps::ObjectGroup::MONSTERS )] );

#if defined( WITH_DEBUG )
        // It is important to check that all data is accurately generated.
        for ( const auto & objects : objectData ) {
            for ( const auto & objectInfo : objects ) {
                // An object must have at least one ground level part.
                assert( !objectInfo.groundLevelParts.empty() );

                // And this part must have the same type as the object.
                assert( objectInfo.groundLevelParts.front().objectType == objectInfo.objectType );

                const MP2::MapObjectType type = objectInfo.objectType;
                MP2::MapObjectType secondaryType = type;
                if ( ( secondaryType & MP2::OBJ_ACTION_OBJECT_TYPE ) == MP2::OBJ_ACTION_OBJECT_TYPE ) {
                    secondaryType = static_cast<MP2::MapObjectType>( secondaryType & ~MP2::OBJ_ACTION_OBJECT_TYPE );
                }

                for ( const auto & info : objectInfo.groundLevelParts ) {
                    if ( info.layerType == Maps::SHADOW_LAYER ) {
                        // Shadow layer never has an object on it.
                        assert( info.objectType == MP2::OBJ_NONE );
                    }
                    else {
                        // All parts should belong to the object.
                        assert( info.objectType == type || info.objectType == secondaryType );
                    }
                }
            }
        }

        // Check that all landscape objects are non-action objects.
        for ( size_t groupType = static_cast<size_t>( Maps::ObjectGroup::ROADS ); groupType <= static_cast<size_t>( Maps::ObjectGroup::LANDSCAPE_FLAGS ); ++groupType ) {
            const auto & objects = objectData[groupType];

            for ( const auto & objectInfo : objects ) {
                assert( !MP2::isActionObject( objectInfo.objectType ) );
            }
        }

        // Check that all other objects are action objects.
        for ( size_t groupType = static_cast<size_t>( Maps::ObjectGroup::ADVENTURE_ARTIFACTS ); groupType < static_cast<size_t>( Maps::ObjectGroup::GROUP_COUNT );
              ++groupType ) {
            const auto & objects = objectData[groupType];

            for ( const auto & objectInfo : objects ) {
                assert( MP2::isActionObject( objectInfo.objectType ) );
            }
        }
#endif

        // For game's map loading and saving we need to keep another cached container.
        // This container also serves as verification that all objects use unique object info as their main object part.
        for ( size_t groupType = 0; groupType < objectData.size(); ++groupType ) {
            for ( size_t objectId = 0; objectId < objectData[groupType].size(); ++objectId ) {
                const auto & frontPart = objectData[groupType][objectId].groundLevelParts.front();
                auto [it, inserted] = icnVsObjectInfo.emplace( std::make_pair( frontPart.icnType, frontPart.icnIndex ),
                                                               std::make_pair( static_cast<Maps::ObjectGroup>( groupType ), static_cast<uint32_t>( objectId ) ) );
                if ( !inserted ) {
                    // You use the same object part for more than one object. Check your code!
                    // But mines with one appearance (terrain type) differ one from other only by the resource car.
                    assert( objectData[groupType][objectId].objectType == MP2::OBJ_MINE );
                }
            }
        }

        isPopulated = true;
    }
}

namespace Maps
{
    const std::vector<ObjectInfo> & getObjectsByGroup( const ObjectGroup group )
    {
        assert( group != ObjectGroup::GROUP_COUNT );

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

    bool getObjectInfo( const MP2::ObjectIcnType icnType, const uint32_t icnIndex, ObjectGroup & group, uint32_t & index )
    {
        if ( icnType == MP2::OBJ_ICN_TYPE_UNKNOWN ) {
            // No object exist. Nothing to do.
            return false;
        }

        populateObjectData();

        auto iter = icnVsObjectInfo.find( std::make_pair( icnType, icnIndex ) );
        if ( iter != icnVsObjectInfo.end() ) {
            group = iter->second.first;
            index = iter->second.second;
            return true;
        }

        return false;
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
