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

        (void)objects;
    }

    void populateAdventureMines( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        (void)objects;
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

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 10, fheroes2::Point{ 1, -2 }, MP2::OBJ_NON_ACTION_DERELICT_SHIP, Maps::SHADOW_LAYER );

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

        // Castle and town sprites: Knight, Barbarian, Sorceress, Warlock, Wizard, Necromancer.
        for ( uint8_t race = 0; race < 6 * 2; ++race ) {
            const uint8_t icnOffset = race * 16;
            Maps::ObjectInfo object{ MP2::OBJ_CASTLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 13, fheroes2::Point{ 0, 0 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset, fheroes2::Point{ 0, -3 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 1, fheroes2::Point{ -2, -2 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 2, fheroes2::Point{ -1, -2 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 3, fheroes2::Point{ 0, -2 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 4, fheroes2::Point{ 1, -2 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 5, fheroes2::Point{ 2, -2 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 6, fheroes2::Point{ -2, -1 }, MP2::OBJ_CASTLE );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 7, fheroes2::Point{ -1, -1 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 8, fheroes2::Point{ 0, -1 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 9, fheroes2::Point{ 1, -1 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 10, fheroes2::Point{ 2, -1 }, MP2::OBJ_CASTLE );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 11, fheroes2::Point{ -2, 0 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 12, fheroes2::Point{ -1, 0 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 14, fheroes2::Point{ 1, 0 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTOWN, icnOffset + 15, fheroes2::Point{ 2, 0 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );

            // Castle/town shadow.

            Maps::ObjectInfo townSadow{ MP2::OBJ_NONE };
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset, fheroes2::Point{ -4, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 1, fheroes2::Point{ -3, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 2, fheroes2::Point{ -2, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 3, fheroes2::Point{ -1, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 4, fheroes2::Point{ -5, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 5, fheroes2::Point{ -4, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 6, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 7, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 8, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 9, fheroes2::Point{ -4, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 10, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 11, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 12, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 13, fheroes2::Point{ -3, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 14, fheroes2::Point{ -2, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWSH, icnOffset + 15, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( townSadow ) );
        }

        // Random castle/town
        for ( uint8_t isTown = 0; isTown < 2; ++isTown ) {
            const uint8_t icnOffset = isTown * 16;
            Maps::ObjectInfo object{ MP2::OBJ_CASTLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 13, fheroes2::Point{ 0, 0 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 0, fheroes2::Point{ 0, -3 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 1, fheroes2::Point{ -2, -2 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 2, fheroes2::Point{ -1, -2 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 3, fheroes2::Point{ 0, -2 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 4, fheroes2::Point{ 1, -2 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 5, fheroes2::Point{ 2, -2 }, MP2::OBJ_CASTLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 6, fheroes2::Point{ -2, -1 }, MP2::OBJ_CASTLE );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 7, fheroes2::Point{ -1, -1 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 8, fheroes2::Point{ 0, -1 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 9, fheroes2::Point{ 1, -1 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 10, fheroes2::Point{ 2, -1 }, MP2::OBJ_CASTLE );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 11, fheroes2::Point{ -2, 0 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 12, fheroes2::Point{ -1, 0 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 14, fheroes2::Point{ 1, 0 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 15, fheroes2::Point{ 2, 0 }, MP2::OBJ_CASTLE, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );

            // Castle/town shadow.

            Maps::ObjectInfo townSadow{ MP2::OBJ_NONE };
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 0 + 32, fheroes2::Point{ -4, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 1 + 32, fheroes2::Point{ -3, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 2 + 32, fheroes2::Point{ -2, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 3 + 32, fheroes2::Point{ -1, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 4 + 32, fheroes2::Point{ -5, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 5 + 32, fheroes2::Point{ -4, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 6 + 32, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 7 + 32, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 8 + 32, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 9 + 32, fheroes2::Point{ -4, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 10 + 32, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 11 + 32, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 12 + 32, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 13 + 32, fheroes2::Point{ -3, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 14 + 32, fheroes2::Point{ -2, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            townSadow.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWRD, icnOffset + 15 + 32, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( townSadow ) );
        }

        // Castle/town basement: grass, snow, swamp, lava, desert, dirt, wasteland, beach.
        for ( uint8_t basement = 0; basement < 8; ++basement ) {
            const uint8_t icnOffset = basement * 10;
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 2, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 1, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 3, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 4, fheroes2::Point{ 2, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 5, fheroes2::Point{ -2, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 6, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 7, fheroes2::Point{ 0, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 8, fheroes2::Point{ 1, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 9, fheroes2::Point{ 2, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Owner color flags: blue, green, red, yellow, orange, purple, white (neutral).
        for ( uint8_t color = 0; color < 7; ++color ) {
            const uint8_t icnOffset = color * 2;
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_FLAG32, icnOffset, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_FLAG32, icnOffset + 1, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
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
                assert( !objectInfo.groundLevelParts.empty() );

                assert( objectInfo.groundLevelParts.front().objectType == objectInfo.objectType );
            }
        }

        // Check that all landscape objects are non-action objects.
        for ( size_t groupType = static_cast<size_t>( Maps::ObjectGroup::ROADS ); groupType <= static_cast<size_t>( Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS );
              ++groupType ) {
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
                if ( groupType == static_cast<size_t>( Maps::ObjectGroup::KINGDOM_TOWNS ) ) {
                    const MP2::ObjectIcnType icnType = objectInfo.groundLevelParts.front().icnType;
                    if ( icnType == MP2::OBJ_ICN_TYPE_OBJNTWSH || icnType == MP2::OBJ_ICN_TYPE_OBJNTWBA || icnType == MP2::OBJ_ICN_TYPE_OBJNTWRD
                         || icnType == MP2::OBJ_ICN_TYPE_FLAG32 ) {
                        // Town/castle objects contain separate shadow, basement and flags that are not action objects.
                        continue;
                    }
                }
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
                    assert( 0 );
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

    int townTypeCalculation( const int townColor, const int townRace, const bool isCastle )
    {
        // NOTICE: This calculation should be consistent with the number of KINGDOM_TOWNS objects.
        return ( townColor * 7 + townRace ) * 2 + ( isCastle ? 0 : 1 );
    }

    bool isCastleByTownType( const int townType )
    {
        // NOTICE: This calculation should be consistent with the number of town types (town/castle) KINGDOM_TOWNS objects.
        return ( townType % 2 ) == 0;
    }

    int getRaceByTownType( const int townType )
    {
        // NOTICE: This calculation should be consistent with the number of races in KINGDOM_TOWNS objects.
        return ( townType / 2 ) % 7;
    }

    int getColorByTownType( const int townType )
    {
        // NOTICE: This calculation should be consistent with the number of color flags in KINGDOM_TOWNS objects.
        return townType / 14;
    }

    size_t getTownObjectOffset( const int townType )
    {
        // NOTICE: This calculation should be consistent with KINGDOM_TOWNS objects position in vector.
        return getRaceByTownType( townType ) * 4 + ( isCastleByTownType( townType ) ? 0 : 2 );
    }

    size_t getTownFlagObjectOffset( const int townType )
    {
        // NOTICE: This calculation should be consistent with KINGDOM_TOWNS objects position in vector.
        return 36 + getColorByTownType( townType );
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
