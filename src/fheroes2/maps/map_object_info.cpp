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

#include <cassert>
#include <array>

#include "artifact.h"

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
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_ARTIFACT;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_UNKNOWN, 0, fheroes2::Point{ 0, 0 }, MP2::OBJ_ARTIFACT, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // All artifacts before Magic Book have their own images.
        // Magic Book is not present in the ICN resources but it is present in artifact IDs.
        std::vector<uint32_t> imageIndices;

        for ( int artifactId = Artifact::UNKNOWN + 1; artifactId < Artifact::MAGIC_BOOK; ++artifactId ) {
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_ARTIFACT;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, artifactId * 2 - 1, fheroes2::Point{ 0, 0 }, MP2::OBJ_ARTIFACT, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, artifactId * 2, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // TODO: temporary assign Magic Book some values.
        {
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_ARTIFACT;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 1, fheroes2::Point{ 0, 0 }, MP2::OBJ_ARTIFACT, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 0, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Random artifacts.
        {
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_ARTIFACT;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 163, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_ARTIFACT, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 162, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_ARTIFACT;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 167, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_ARTIFACT_MINOR, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 166, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_ARTIFACT;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 169, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_ARTIFACT_MAJOR, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 168, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        {
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_ARTIFACT;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 171, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_ARTIFACT_TREASURE, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, 170, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        for ( int artifactId = Artifact::SPELL_SCROLL; artifactId < Artifact::ARTIFACT_COUNT; ++artifactId ) {
            const uint32_t imageIndex{ 173U + ( static_cast<uint32_t>( artifactId ) - Artifact::SPELL_SCROLL ) * 2 };
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_ARTIFACT;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, imageIndex, fheroes2::Point{ 0, 0 }, MP2::OBJ_ARTIFACT, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNARTI, imageIndex - 1, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateResourceData( std::vector<Maps::ObjectInfo> & objects )
    {
        // Normal resources.
        for ( const uint32_t imageIndex : { 1, 3, 5, 7, 9, 11, 13 } ) {
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_RESOURCE;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, imageIndex, fheroes2::Point{ 0, 0 }, MP2::OBJ_RESOURCE, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, imageIndex - 1, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Genie's Lamp.
        {
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_GENIE_LAMP;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 15, fheroes2::Point{ 0, 0 }, MP2::OBJ_GENIE_LAMP, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 14, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Random resource.
        {
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_GENIE_LAMP;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 17, fheroes2::Point{ 0, 0 }, MP2::OBJ_RANDOM_RESOURCE, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 16, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Treasure chest.
        {
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_GENIE_LAMP;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 19, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREASURE_CHEST, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNRSRC, 18, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateWaterObjectData( std::vector<Maps::ObjectInfo> & objects )
    {
        // TODO: properly populate these objects.

        // Rock.
        {
            Maps::ObjectInfo object;
            object.objectType = MP2::OBJ_ROCK;
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 2, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 1, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.parts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWAT2, 0, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

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
        // The order of objects must be preserved. If you want to add a new object add it to the end of the container.
        populateArtifactData( objectData[static_cast<size_t>( Maps::ObjectGroup::Artifact )] );

        populateResourceData( objectData[static_cast<size_t>( Maps::ObjectGroup::Resource )] );

        populateWaterObjectData( objectData[static_cast<size_t>( Maps::ObjectGroup::Water_Object )] );

        isPopulated = true;
    }
}

namespace Maps
{
    const std::vector<ObjectInfo> & getObjectsByGroup( const ObjectGroup group )
    {
        populateObjectData();

        return objectData[static_cast<size_t>( group )];
    }
}
