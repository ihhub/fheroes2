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

#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <map>
#include <set>
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

    // This map is used for searching object parts based on their ICN information.
    // Since we have a lot of objects it is important to speed up the search even if we take several more KB of memory.
    std::map<std::pair<MP2::ObjectIcnType, uint32_t>, const Maps::ObjectPartInfo *> objectInfoByIcn;

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

        // Grouped mountains have different appearances: Generic, Grass, Snow, Swamp, Lava, Desert, Dirt, Wasteland.
        for ( const MP2::ObjectIcnType type : { MP2::OBJ_ICN_TYPE_MTNMULT, MP2::OBJ_ICN_TYPE_MTNGRAS, MP2::OBJ_ICN_TYPE_MTNSNOW, MP2::OBJ_ICN_TYPE_MTNSWMP,
                                                MP2::OBJ_ICN_TYPE_MTNLAVA, MP2::OBJ_ICN_TYPE_MTNDSRT, MP2::OBJ_ICN_TYPE_MTNDIRT, MP2::OBJ_ICN_TYPE_MTNCRCK } ) {
            // Big mountain from top-left to bottom-right.
            {
                Maps::ObjectInfo object{ MP2::OBJ_MOUNTAINS };
                object.groundLevelParts.emplace_back( type, 14U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 6U, fheroes2::Point{ -2, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 7U, fheroes2::Point{ -1, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 8U, fheroes2::Point{ 0, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 9U, fheroes2::Point{ 1, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 12U, fheroes2::Point{ -2, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 13U, fheroes2::Point{ -1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 15U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 16U, fheroes2::Point{ 2, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 18U, fheroes2::Point{ 0, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 19U, fheroes2::Point{ 1, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 20U, fheroes2::Point{ 2, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );

                object.groundLevelParts.emplace_back( type, 0U, fheroes2::Point{ -3, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, 5U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, 11U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, 17U, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                object.topLevelParts.emplace_back( type, 1U, fheroes2::Point{ -2, -2 }, MP2::OBJ_MOUNTAINS );
                object.topLevelParts.emplace_back( type, 2U, fheroes2::Point{ -1, -2 }, MP2::OBJ_MOUNTAINS );
                object.topLevelParts.emplace_back( type, 3U, fheroes2::Point{ 0, -2 }, MP2::OBJ_MOUNTAINS );
                object.topLevelParts.emplace_back( type, 4U, fheroes2::Point{ 1, -2 }, MP2::OBJ_MOUNTAINS );
                object.topLevelParts.emplace_back( type, 10U, fheroes2::Point{ 2, -1 }, MP2::OBJ_MOUNTAINS );

                objects.emplace_back( std::move( object ) );
            }

            uint32_t icnOffset = 21U;

            // Big mountain from top-right to bottom-left.
            {
                Maps::ObjectInfo object{ MP2::OBJ_MOUNTAINS };
                object.groundLevelParts.emplace_back( type, icnOffset + 14U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 7U, fheroes2::Point{ -1, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 8U, fheroes2::Point{ 0, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 9U, fheroes2::Point{ 1, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 10U, fheroes2::Point{ 2, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 12U, fheroes2::Point{ -2, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 13U, fheroes2::Point{ -1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 15U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 16U, fheroes2::Point{ 2, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 18U, fheroes2::Point{ -2, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 19U, fheroes2::Point{ -1, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 20U, fheroes2::Point{ 0, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );

                object.groundLevelParts.emplace_back( type, icnOffset + 0U, fheroes2::Point{ -2, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 5U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 11U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 17U, fheroes2::Point{ -3, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                object.topLevelParts.emplace_back( type, icnOffset + 1U, fheroes2::Point{ -1, -2 }, MP2::OBJ_MOUNTAINS );
                object.topLevelParts.emplace_back( type, icnOffset + 2U, fheroes2::Point{ 0, -2 }, MP2::OBJ_MOUNTAINS );
                object.topLevelParts.emplace_back( type, icnOffset + 3U, fheroes2::Point{ 1, -2 }, MP2::OBJ_MOUNTAINS );
                object.topLevelParts.emplace_back( type, icnOffset + 4U, fheroes2::Point{ 2, -2 }, MP2::OBJ_MOUNTAINS );
                object.topLevelParts.emplace_back( type, icnOffset + 6U, fheroes2::Point{ -2, -1 }, MP2::OBJ_MOUNTAINS );

                objects.emplace_back( std::move( object ) );
            }

            icnOffset += 21;

            // Dirt and Wasteland has extra mountains placed in ICN between big and small mountains
            if ( type == MP2::OBJ_ICN_TYPE_MTNDIRT || type == MP2::OBJ_ICN_TYPE_MTNCRCK ) {
                // Extra mountain from top-left to bottom-right.
                {
                    Maps::ObjectInfo object{ MP2::OBJ_MOUNTAINS };
                    object.groundLevelParts.emplace_back( type, icnOffset + 8U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 6U, fheroes2::Point{ -2, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 7U, fheroes2::Point{ -1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 12U, fheroes2::Point{ 0, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 13U, fheroes2::Point{ 1, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 14U, fheroes2::Point{ 2, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );

                    object.groundLevelParts.emplace_back( type, icnOffset + 0U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 4U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 5U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 11U, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                    object.topLevelParts.emplace_back( type, icnOffset + 1U, fheroes2::Point{ -2, -1 }, MP2::OBJ_MOUNTAINS );
                    object.topLevelParts.emplace_back( type, icnOffset + 2U, fheroes2::Point{ -1, -1 }, MP2::OBJ_MOUNTAINS );
                    object.topLevelParts.emplace_back( type, icnOffset + 3U, fheroes2::Point{ 0, -1 }, MP2::OBJ_MOUNTAINS );
                    object.topLevelParts.emplace_back( type, icnOffset + 9U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MOUNTAINS );
                    object.topLevelParts.emplace_back( type, icnOffset + 10U, fheroes2::Point{ 2, 0 }, MP2::OBJ_MOUNTAINS );

                    objects.emplace_back( std::move( object ) );
                }

                icnOffset += 15;

                // Extra mountain from top-right to bottom-left.
                {
                    Maps::ObjectInfo object{ MP2::OBJ_MOUNTAINS };
                    object.groundLevelParts.emplace_back( type, icnOffset + 8U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 9U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 10U, fheroes2::Point{ 2, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 12U, fheroes2::Point{ -2, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 13U, fheroes2::Point{ -1, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 14U, fheroes2::Point{ 0, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );

                    object.groundLevelParts.emplace_back( type, icnOffset + 0U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 1U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 5U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 11U, fheroes2::Point{ -3, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                    object.topLevelParts.emplace_back( type, icnOffset + 2U, fheroes2::Point{ 0, -1 }, MP2::OBJ_MOUNTAINS );
                    object.topLevelParts.emplace_back( type, icnOffset + 3U, fheroes2::Point{ 1, -1 }, MP2::OBJ_MOUNTAINS );
                    object.topLevelParts.emplace_back( type, icnOffset + 4U, fheroes2::Point{ 2, -1 }, MP2::OBJ_MOUNTAINS );
                    object.topLevelParts.emplace_back( type, icnOffset + 6U, fheroes2::Point{ -2, 0 }, MP2::OBJ_MOUNTAINS );
                    object.topLevelParts.emplace_back( type, icnOffset + 7U, fheroes2::Point{ -1, 0 }, MP2::OBJ_MOUNTAINS );

                    objects.emplace_back( std::move( object ) );
                }

                icnOffset += 15;
            }

            // Medium mountain from top-left to bottom-right.
            {
                Maps::ObjectInfo object{ MP2::OBJ_MOUNTAINS };
                object.groundLevelParts.emplace_back( type, icnOffset + 5U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 4U, fheroes2::Point{ -1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 6U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 8U, fheroes2::Point{ 0, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 9U, fheroes2::Point{ 1, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );

                object.groundLevelParts.emplace_back( type, icnOffset + 0U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 3U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 7U, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                // Grassy medium mountain should not be passable in the upper row (according to its image)
                if ( type == MP2::OBJ_ICN_TYPE_MTNGRAS ) {
                    object.groundLevelParts.emplace_back( type, icnOffset + 1U, fheroes2::Point{ -1, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 2U, fheroes2::Point{ 0, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                }
                else {
                    object.topLevelParts.emplace_back( type, icnOffset + 1U, fheroes2::Point{ -1, -1 }, MP2::OBJ_MOUNTAINS );
                    object.topLevelParts.emplace_back( type, icnOffset + 2U, fheroes2::Point{ 0, -1 }, MP2::OBJ_MOUNTAINS );
                }

                objects.emplace_back( std::move( object ) );
            }

            icnOffset += 10;

            // Medium mountain from top-left to bottom-right.
            {
                Maps::ObjectInfo object{ MP2::OBJ_MOUNTAINS };
                object.groundLevelParts.emplace_back( type, icnOffset + 5U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 4U, fheroes2::Point{ -1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 6U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 8U, fheroes2::Point{ -1, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 9U, fheroes2::Point{ 0, 1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );

                object.groundLevelParts.emplace_back( type, icnOffset + 0U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 3U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 7U, fheroes2::Point{ -2, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                // Grassy medium mountain should not be passable in the upper row (according to its image)
                if ( type == MP2::OBJ_ICN_TYPE_MTNGRAS ) {
                    object.groundLevelParts.emplace_back( type, icnOffset + 1U, fheroes2::Point{ 0, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                    object.groundLevelParts.emplace_back( type, icnOffset + 2U, fheroes2::Point{ 1, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                }
                else {
                    object.topLevelParts.emplace_back( type, icnOffset + 1U, fheroes2::Point{ 0, -1 }, MP2::OBJ_MOUNTAINS );
                    object.topLevelParts.emplace_back( type, icnOffset + 2U, fheroes2::Point{ 1, -1 }, MP2::OBJ_MOUNTAINS );
                }

                objects.emplace_back( std::move( object ) );
            }

            icnOffset += 10;

            // Small mountain from top-left to bottom-right.
            {
                Maps::ObjectInfo object{ MP2::OBJ_MOUNTAINS };
                object.groundLevelParts.emplace_back( type, icnOffset + 4U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 1U, fheroes2::Point{ -1, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 2U, fheroes2::Point{ 0, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 5U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );

                object.groundLevelParts.emplace_back( type, icnOffset + 0U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 3U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                objects.emplace_back( std::move( object ) );
            }

            icnOffset += 6;

            // Small mountain from top-right to bottom-left.
            {
                Maps::ObjectInfo object{ MP2::OBJ_MOUNTAINS };
                object.groundLevelParts.emplace_back( type, icnOffset + 5U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 1U, fheroes2::Point{ 0, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 2U, fheroes2::Point{ 1, -1 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 4U, fheroes2::Point{ -1, 0 }, MP2::OBJ_MOUNTAINS, Maps::OBJECT_LAYER );

                object.groundLevelParts.emplace_back( type, icnOffset + 0U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, icnOffset + 3U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                objects.emplace_back( std::move( object ) );
            }
        }

        // Single mountains.

        // Grass medium mound.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MOUND };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 77U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUND, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 78U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MOUND, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 76U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Grass small mound.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MOUND };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 149U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUND, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 150U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MOUND, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Big volcano (Lava terrain).
        {
            Maps::ObjectInfo object{ MP2::OBJ_VOLCANO };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 245U, fheroes2::Point{ 0, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 244U, fheroes2::Point{ -1, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 246U, fheroes2::Point{ 1, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 120U, fheroes2::Point{ -4, -2 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 14;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 180U, fheroes2::Point{ -5, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 14;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 195U, fheroes2::Point{ -4, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 14;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 210U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 14;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 225U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 14;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 243U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 0U, fheroes2::Point{ -1, -4 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 14;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 15U, fheroes2::Point{ 0, -4 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 14;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 30U, fheroes2::Point{ 1, -4 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 14;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 45U, fheroes2::Point{ -2, -3 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 14;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 60U, fheroes2::Point{ -1, -3 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 14;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 75U, fheroes2::Point{ 0, -3 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 14;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 90U, fheroes2::Point{ 1, -3 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 14;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 105U, fheroes2::Point{ 2, -3 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 14;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 135U, fheroes2::Point{ -1, -2 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 14;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 150U, fheroes2::Point{ 0, -2 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 14;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 165U, fheroes2::Point{ 1, -2 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 14;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 240U, fheroes2::Point{ -1, -1 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 241U, fheroes2::Point{ 0, -1 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV3, 242U, fheroes2::Point{ 1, -1 }, MP2::OBJ_VOLCANO );

            objects.emplace_back( std::move( object ) );
        }

        // Middle volcano (Lava terrain).
        {
            Maps::ObjectInfo object{ MP2::OBJ_VOLCANO };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 31U, fheroes2::Point{ 0, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 30U, fheroes2::Point{ -1, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 32U, fheroes2::Point{ 1, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 7U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 14U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 28U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 29U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 0U, fheroes2::Point{ 0, -2 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 6;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 21U, fheroes2::Point{ 0, -1 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Small volcano (Lava terrain).
        {
            Maps::ObjectInfo object{ MP2::OBJ_VOLCANO };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 80U, fheroes2::Point{ 0, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 79U, fheroes2::Point{ -1, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 81U, fheroes2::Point{ 1, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 44U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 10;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 55U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 10;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 78U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 33U, fheroes2::Point{ 0, -2 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 10;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 66U, fheroes2::Point{ -1, -1 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAV2, 67U, fheroes2::Point{ 0, -1 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 10;

            objects.emplace_back( std::move( object ) );
        }

        // Smallest volcano (Lava terrain).
        {
            Maps::ObjectInfo object{ MP2::OBJ_VOLCANO };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 76U, fheroes2::Point{ 0, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 77U, fheroes2::Point{ 1, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );

            // TODO: Fit the lack of shadow tile to the left.

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 74U, fheroes2::Point{ 0, -1 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 75U, fheroes2::Point{ 1, -1 }, MP2::OBJ_VOLCANO );

            objects.emplace_back( std::move( object ) );
        }

        // Desert dune.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DUNE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 14U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DUNE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 15U, fheroes2::Point{ 1, 0 }, MP2::OBJ_DUNE, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 13U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Desert mound.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DUNE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 17U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DUNE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 18U, fheroes2::Point{ 1, 0 }, MP2::OBJ_DUNE, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 19U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Desert dune.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DUNE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 21U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DUNE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 20U, fheroes2::Point{ -1, 0 }, MP2::OBJ_DUNE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 22U, fheroes2::Point{ 1, 0 }, MP2::OBJ_DUNE, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 23U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Dirt mound.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MOUND };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 12U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUND, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 13U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MOUND, Maps::OBJECT_LAYER );

            // TODO: Fit the lack of shadow tile to the left.

            objects.emplace_back( std::move( object ) );
        }

        // Dirt mound.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MOUND };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 15U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOUND, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 16U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MOUND, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 14U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateLandscapeRocks( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        // Small and medium rocks, grass terrain, variant 1.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 33U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 34U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 32U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium and very small rocks, grass terrain, variant 2.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 37U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 38U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 36U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 35U, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK );

            objects.emplace_back( std::move( object ) );
        }

        // Single medium rock, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 40U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 39U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Tiny rocks, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 41U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small 1-tile rocks, grass terrain. 4 variants.
        for ( uint32_t count = 0; count < 4; ++count ) {
            const uint32_t offset = 42U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small rocks, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 22U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 21U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium rocks, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 26U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 27U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 25U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 23U, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 24U, fheroes2::Point{ 1, -1 }, MP2::OBJ_ROCK );

            objects.emplace_back( std::move( object ) );
        }

        // Tiny rocks, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 28U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small 1-tile rocks, grass terrain. 2 variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 29U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide small rocks, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 34U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 35U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 33U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium rock, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 37U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 36U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium rock, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 39U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 38U, fheroes2::Point{ -1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide and low mossy rock, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MOSSY_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 139U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOSSY_ROCK, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 138U, fheroes2::Point{ -1, 0 }, MP2::OBJ_MOSSY_ROCK, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium mossy rock, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MOSSY_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 203U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOSSY_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 202U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium rock, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 205U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 204U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Big mossy rock, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MOSSY_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 209U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MOSSY_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 208U, fheroes2::Point{ -1, 0 }, MP2::OBJ_MOSSY_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 207U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 206U, fheroes2::Point{ 0, -1 }, MP2::OBJ_MOSSY_ROCK );

            objects.emplace_back( std::move( object ) );
        }

        // Very-very small rocks (1-tile), swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 210U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide medium rocks, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 92U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 93U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 91U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Big and three medium rocks, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 98U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 99U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 97U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 94U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 95U, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 96U, fheroes2::Point{ 1, -1 }, MP2::OBJ_ROCK );

            objects.emplace_back( std::move( object ) );
        }

        // Medium and small rocks, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 101U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 102U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 100U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium rock, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 104U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 103U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small rocks, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 105U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide medium rock with a tree on it, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 10U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 11U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 9U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 7U, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 8U, fheroes2::Point{ 1, -1 }, MP2::OBJ_ROCK );

            objects.emplace_back( std::move( object ) );
        }

        // Small rock, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 18U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Big rock and two medium around, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 21U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 22U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 20U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 19U, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK );

            objects.emplace_back( std::move( object ) );
        }

        // Wide medium rock, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 24U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 25U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 23U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide big rock, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 30U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 29U, fheroes2::Point{ -1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 28U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 26U, fheroes2::Point{ -1, -1 }, MP2::OBJ_ROCK );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 27U, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK );

            objects.emplace_back( std::move( object ) );
        }

        // Wide low rock, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 31U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 32U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide low rock, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 34U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 35U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 33U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide small rock and a small rock, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 38U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 37U, fheroes2::Point{ -1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 36U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide small rock, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 40U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 41U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 39U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small and medium rocks, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 43U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 42U, fheroes2::Point{ -1, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Tall rocks, wasteland terrain. 4 variants.
        for ( uint32_t count = 0; count < 4; ++count ) {
            const uint32_t offset = 44U + count * 3U;

            Maps::ObjectInfo object{ MP2::OBJ_ROCK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, offset + 2U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, offset + 1U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, offset + 0U, fheroes2::Point{ 0, -1 }, MP2::OBJ_ROCK );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateLandscapeTrees( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        // Forest have different appearances: Deciduous, Evil (dead), Autumn, Fir trees, Jungle, Snowy fir trees.
        for ( const MP2::ObjectIcnType type : { MP2::OBJ_ICN_TYPE_TREDECI, MP2::OBJ_ICN_TYPE_TREEVIL, MP2::OBJ_ICN_TYPE_TREFALL, MP2::OBJ_ICN_TYPE_TREFIR,
                                                MP2::OBJ_ICN_TYPE_TREJNGL, MP2::OBJ_ICN_TYPE_TRESNOW } ) {
            // Big forest from top-left to bottom-right.
            {
                Maps::ObjectInfo object{ MP2::OBJ_TREES };
                object.groundLevelParts.emplace_back( type, 5U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 4U, fheroes2::Point{ -1, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 8U, fheroes2::Point{ 0, 1 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 9U, fheroes2::Point{ 1, 1 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

                object.groundLevelParts.emplace_back( type, 0U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, 3U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, 7U, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                object.topLevelParts.emplace_back( type, 1U, fheroes2::Point{ -1, -1 }, MP2::OBJ_TREES );
                object.topLevelParts.emplace_back( type, 2U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES );
                object.topLevelParts.emplace_back( type, 6U, fheroes2::Point{ 1, 0 }, MP2::OBJ_TREES );

                objects.emplace_back( std::move( object ) );
            }

            // Big forest from top-right to bottom-left.
            {
                Maps::ObjectInfo object{ MP2::OBJ_TREES };
                object.groundLevelParts.emplace_back( type, 15U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 16U, fheroes2::Point{ 1, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 18U, fheroes2::Point{ -1, 1 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 19U, fheroes2::Point{ 0, 1 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

                object.groundLevelParts.emplace_back( type, 10U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, 13U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, 17U, fheroes2::Point{ -2, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                object.topLevelParts.emplace_back( type, 11U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES );
                object.topLevelParts.emplace_back( type, 12U, fheroes2::Point{ 1, -1 }, MP2::OBJ_TREES );
                object.topLevelParts.emplace_back( type, 14U, fheroes2::Point{ -1, 0 }, MP2::OBJ_TREES );

                objects.emplace_back( std::move( object ) );
            }

            // Medium forest from top-left to bottom-right.
            {
                Maps::ObjectInfo object{ MP2::OBJ_TREES };
                object.groundLevelParts.emplace_back( type, 24U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 21U, fheroes2::Point{ -1, -1 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 22U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 25U, fheroes2::Point{ 1, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

                object.groundLevelParts.emplace_back( type, 20U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, 23U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                objects.emplace_back( std::move( object ) );
            }

            // Medium forest from top-right to bottom-left.
            {
                Maps::ObjectInfo object{ MP2::OBJ_TREES };
                object.groundLevelParts.emplace_back( type, 31U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 27U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 28U, fheroes2::Point{ 1, -1 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
                object.groundLevelParts.emplace_back( type, 30U, fheroes2::Point{ -1, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

                object.groundLevelParts.emplace_back( type, 26U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
                object.groundLevelParts.emplace_back( type, 29U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                objects.emplace_back( std::move( object ) );
            }

            // Small forest (2 variants).
            for ( uint32_t count = 0; count < 2; ++count ) {
                const uint32_t offset = count * 2U + 32U;
                Maps::ObjectInfo object{ MP2::OBJ_TREES };
                object.groundLevelParts.emplace_back( type, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

                object.groundLevelParts.emplace_back( type, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

                objects.emplace_back( std::move( object ) );
            }
        }

        // Three trees, grass terrain, variant 1.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 84U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 83U, fheroes2::Point{ -1, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 85U, fheroes2::Point{ 1, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 82U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 79U, fheroes2::Point{ -1, -1 }, MP2::OBJ_TREES );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 80U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 81U, fheroes2::Point{ 1, -1 }, MP2::OBJ_TREES );

            objects.emplace_back( std::move( object ) );
        }

        // Three trees, grass terrain, variant 2.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 89U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 90U, fheroes2::Point{ 1, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 88U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 86U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 87U, fheroes2::Point{ 1, -1 }, MP2::OBJ_TREES );

            objects.emplace_back( std::move( object ) );
        }

        // Single tree, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 93U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 92U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 91U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES );

            objects.emplace_back( std::move( object ) );
        }

        // Two stumps, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_STUMP };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 41U, fheroes2::Point{ 0, 0 }, MP2::OBJ_STUMP, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 40U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Single stump, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_STUMP };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 42U, fheroes2::Point{ 0, 0 }, MP2::OBJ_STUMP, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Dead tree (medium, wide), snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DEAD_TREE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 49U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 50U, fheroes2::Point{ 1, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 48U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 46U, fheroes2::Point{ 0, -1 }, MP2::OBJ_DEAD_TREE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 47U, fheroes2::Point{ 1, -1 }, MP2::OBJ_DEAD_TREE );

            objects.emplace_back( std::move( object ) );
        }

        // Dead tree (tall, wide), snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DEAD_TREE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 56U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 55U, fheroes2::Point{ -1, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 57U, fheroes2::Point{ 1, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 54U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 51U, fheroes2::Point{ -1, -1 }, MP2::OBJ_DEAD_TREE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 52U, fheroes2::Point{ 0, -1 }, MP2::OBJ_DEAD_TREE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 53U, fheroes2::Point{ 1, -1 }, MP2::OBJ_DEAD_TREE );

            objects.emplace_back( std::move( object ) );
        }

        // Dead tree (medium, 1-tile), snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DEAD_TREE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 60U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 59U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 58U, fheroes2::Point{ 0, -1 }, MP2::OBJ_DEAD_TREE );

            objects.emplace_back( std::move( object ) );
        }

        // Dead tree (medium, thinned, wide), snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DEAD_TREE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 64U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 65U, fheroes2::Point{ 1, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 63U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 61U, fheroes2::Point{ 0, -1 }, MP2::OBJ_DEAD_TREE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 62U, fheroes2::Point{ 1, -1 }, MP2::OBJ_DEAD_TREE );

            objects.emplace_back( std::move( object ) );
        }

        // Dead trees (1-tile), snow terrain. 5 variants.
        for ( uint32_t count = 0; count < 5; ++count ) {
            const uint32_t offset = 66U + count * 3U;

            Maps::ObjectInfo object{ MP2::OBJ_DEAD_TREE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, offset + 2U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, offset + 1U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, offset + 0U, fheroes2::Point{ 0, -1 }, MP2::OBJ_DEAD_TREE );

            objects.emplace_back( std::move( object ) );
        }

        // Dead tree (tall in the pool), swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DEAD_TREE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 167U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 166U, fheroes2::Point{ -1, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 163U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 161U, fheroes2::Point{ -1, -2 }, MP2::OBJ_DEAD_TREE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 162U, fheroes2::Point{ 0, -2 }, MP2::OBJ_DEAD_TREE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 164U, fheroes2::Point{ -1, -1 }, MP2::OBJ_DEAD_TREE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 165U, fheroes2::Point{ 0, -1 }, MP2::OBJ_DEAD_TREE );

            objects.emplace_back( std::move( object ) );
        }

        // Dead trees (2-tile), swamp terrain. 2 variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 168U + count * 5U;

            Maps::ObjectInfo object{ MP2::OBJ_DEAD_TREE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 3U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 4U, fheroes2::Point{ 1, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 2U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 0U, fheroes2::Point{ 0, -1 }, MP2::OBJ_DEAD_TREE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 1U, fheroes2::Point{ 1, -1 }, MP2::OBJ_DEAD_TREE );

            objects.emplace_back( std::move( object ) );
        }

        // Two tall palms, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 3U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

            // The next tile contents a shadow and the upper part of the tree. How to render it properly? What layer is better to use?
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 2U, fheroes2::Point{ -1, 0 }, MP2::OBJ_TREES );

            objects.emplace_back( std::move( object ) );
        }

        // Tall palms, desert terrain. 3 variants.
        for ( uint32_t count = 0; count < 3; ++count ) {
            const uint32_t offset = 4U + count * 3U;

            Maps::ObjectInfo object{ MP2::OBJ_TREES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, offset + 2U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

            // The next tile contents a shadow and the upper part of the tree. How to render it properly? What layer is better to use?
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, offset + 1U, fheroes2::Point{ -1, 0 }, MP2::OBJ_TREES );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, offset + 0U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES );

            objects.emplace_back( std::move( object ) );
        }

        // A lonely palm.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 76U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 75U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 74U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES );

            objects.emplace_back( std::move( object ) );
        }

        // Small palms, desert terrain. 3 variants.
        for ( uint32_t count = 0; count < 3; ++count ) {
            const uint32_t offset = 23U + count * 2U;

            Maps::ObjectInfo object{ MP2::OBJ_TREES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Tall single tree, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 118U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 117U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 116U, fheroes2::Point{ 1, -1 }, MP2::OBJ_TREES );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 115U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES );

            // The next tile contents the upper part of the tree. It is passable but should be rendered before the hero on this tile.
            // How to render it properly? What layer is better to use?
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 119U, fheroes2::Point{ 1, 0 }, MP2::OBJ_TREES );

            objects.emplace_back( std::move( object ) );
        }

        // Two tall trees, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 123U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

            // The next tile contents a shadow and the upper part of the tree. How to render it properly? What layer is better to use?
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 122U, fheroes2::Point{ -1, 0 }, MP2::OBJ_TREES );

            // The next tile contents the upper part of the tree. It is passable but should be rendered before the hero on this tile.
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 124U, fheroes2::Point{ 1, 0 }, MP2::OBJ_TREES );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 120U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 121U, fheroes2::Point{ 1, -1 }, MP2::OBJ_TREES );

            objects.emplace_back( std::move( object ) );
        }

        // Two medium trees, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 127U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREES, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 126U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 125U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TREES );

            objects.emplace_back( std::move( object ) );
        }

        // Dead tree, generic.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DEAD_TREE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 2U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 1U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 0U, fheroes2::Point{ 0, -1 }, MP2::OBJ_DEAD_TREE );

            objects.emplace_back( std::move( object ) );
        }

        // Log (dead tree), generic.
        {
            // We use MP2::OBJ_DEAD_TREE instead of MP2::OBJ_NOTHING_SPECIAL used by the original editor.
            Maps::ObjectInfo object{ MP2::OBJ_DEAD_TREE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 4U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DEAD_TREE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 3U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Three stumps, generic.
        {
            // We use MP2::OBJ_STUMP instead of MP2::OBJ_DEAD_TREE used by the original editor.
            Maps::ObjectInfo object{ MP2::OBJ_STUMP };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 16U, fheroes2::Point{ 0, 0 }, MP2::OBJ_STUMP, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Two stumps, generic.
        {
            // We use MP2::OBJ_STUMP instead of MP2::OBJ_DEAD_TREE used by the original editor.
            Maps::ObjectInfo object{ MP2::OBJ_STUMP };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 18U, fheroes2::Point{ 0, 0 }, MP2::OBJ_STUMP, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 17U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Single stump, generic.
        {
            // We use MP2::OBJ_STUMP instead of MP2::OBJ_DEAD_TREE used by the original editor.
            Maps::ObjectInfo object{ MP2::OBJ_STUMP };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 19U, fheroes2::Point{ 0, 0 }, MP2::OBJ_STUMP, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }
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

        // Crack, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 9U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 8U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Crack, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 15U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 10U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 11U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 12U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 13U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 14U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 16U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Crack, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 21U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 17U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 18U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 19U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 20U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 22U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 23U, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 24U, fheroes2::Point{ 0, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 25U, fheroes2::Point{ 1, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small crack, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 28U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 26U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 27U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Big water lake, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATER_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 61U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATER_LAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 54U, fheroes2::Point{ -2, -1 }, MP2::OBJ_WATER_LAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 55U, fheroes2::Point{ -1, -1 }, MP2::OBJ_WATER_LAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 56U, fheroes2::Point{ 0, -1 }, MP2::OBJ_WATER_LAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 57U, fheroes2::Point{ 1, -1 }, MP2::OBJ_WATER_LAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 58U, fheroes2::Point{ 2, -1 }, MP2::OBJ_WATER_LAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 59U, fheroes2::Point{ -2, 0 }, MP2::OBJ_WATER_LAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 60U, fheroes2::Point{ -1, 0 }, MP2::OBJ_WATER_LAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 62U, fheroes2::Point{ 1, 0 }, MP2::OBJ_WATER_LAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 63U, fheroes2::Point{ 2, 0 }, MP2::OBJ_WATER_LAKE, Maps::OBJECT_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 50U, fheroes2::Point{ -1, -2 }, MP2::OBJ_WATER_LAKE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 51U, fheroes2::Point{ 0, -2 }, MP2::OBJ_WATER_LAKE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 52U, fheroes2::Point{ 1, -2 }, MP2::OBJ_WATER_LAKE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 53U, fheroes2::Point{ 2, -2 }, MP2::OBJ_WATER_LAKE );

            objects.emplace_back( std::move( object ) );
        }

        // Medium water lake, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATER_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 71U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 65U, fheroes2::Point{ -2, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 66U, fheroes2::Point{ -1, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 67U, fheroes2::Point{ 0, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 68U, fheroes2::Point{ 1, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 69U, fheroes2::Point{ -2, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 70U, fheroes2::Point{ -1, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 72U, fheroes2::Point{ 1, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small water lake, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATER_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 74U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 73U, fheroes2::Point{ -1, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 75U, fheroes2::Point{ 1, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide shrub, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 96U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 95U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 97U, fheroes2::Point{ 1, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 94U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide shrub, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 100U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 99U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 101U, fheroes2::Point{ 1, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium wide shrub, grass terrain. 3 variants.
        for ( uint32_t count = 0; count < 3; ++count ) {
            const uint32_t offset = 102U + count * 3U;
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, offset + 2U, fheroes2::Point{ 1, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small shrub, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 112U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 111U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Pink flowers, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 64U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide flowers, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 115U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 114U, fheroes2::Point{ -1, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 116U, fheroes2::Point{ 1, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 113U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide flowers (taller), grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 122U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 121U, fheroes2::Point{ -1, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 123U, fheroes2::Point{ 1, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 117U, fheroes2::Point{ -1, -1 }, MP2::OBJ_FLOWERS );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 118U, fheroes2::Point{ 0, -1 }, MP2::OBJ_FLOWERS );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 119U, fheroes2::Point{ 1, -1 }, MP2::OBJ_FLOWERS );

            objects.emplace_back( std::move( object ) );
        }

        // Wide flowers, grass terrain. 2 variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 124U + count * 4U;
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, offset + 2U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, offset + 3U, fheroes2::Point{ 1, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, offset + 1U, fheroes2::Point{ -1, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, offset + 0U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium wide flowers (taller), grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 136U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 135U, fheroes2::Point{ -1, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 134U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 132U, fheroes2::Point{ -1, -1 }, MP2::OBJ_FLOWERS );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 133U, fheroes2::Point{ 0, -1 }, MP2::OBJ_FLOWERS );

            objects.emplace_back( std::move( object ) );
        }

        // Very small flowers, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 137U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Low medium-wide flowers, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 140U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 139U, fheroes2::Point{ -1, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 138U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small (1 tile) flowers, grass terrain. 4 variants.
        for ( uint32_t count = 0; count < 4; ++count ) {
            const uint32_t offset = 141U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Dug hole, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 9U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small cliff, grass terrain. 3 variants.
        for ( uint32_t count = 0; count < 3; ++count ) {
            const uint32_t offset = 10U + count * 2U + ( count == 2 ? 1 : 0 );
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Crack, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Dug hole, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 11U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small cliff, snow terrain. 2 variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 14U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide small cliff, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 19U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 18U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 20U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Very small shrub, snow terrain. 3 variants.
        for ( uint32_t count = 0; count < 3; ++count ) {
            const uint32_t offset = 43U + count * 1U;
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, offset + 0U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Big frozen water lake, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FROZEN_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 87U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 81U, fheroes2::Point{ -2, -1 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 82U, fheroes2::Point{ -1, -1 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 83U, fheroes2::Point{ 0, -1 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 84U, fheroes2::Point{ 1, -1 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 85U, fheroes2::Point{ -2, 0 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 86U, fheroes2::Point{ -1, 0 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 88U, fheroes2::Point{ 1, 0 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 89U, fheroes2::Point{ 2, 0 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium frozen water lake, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FROZEN_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 91U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 90U, fheroes2::Point{ -1, 0 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 92U, fheroes2::Point{ 1, 0 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small frozen water lake, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FROZEN_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 94U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 95U, fheroes2::Point{ 1, 0 }, MP2::OBJ_FROZEN_LAKE, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 93U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Crack, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 30U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 29U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Shrub, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 33U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 32U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 31U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Dug hole, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 86U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Big water lake, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SWAMPY_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 96U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 88U, fheroes2::Point{ -2, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 89U, fheroes2::Point{ -1, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 90U, fheroes2::Point{ 0, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 91U, fheroes2::Point{ 1, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 94U, fheroes2::Point{ -2, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 95U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 97U, fheroes2::Point{ 1, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 98U, fheroes2::Point{ 2, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 101U, fheroes2::Point{ -2, 1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 102U, fheroes2::Point{ -1, 1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 103U, fheroes2::Point{ 0, 1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 104U, fheroes2::Point{ 1, 1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 105U, fheroes2::Point{ 2, 1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 87U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 92U, fheroes2::Point{ 2, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 93U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 99U, fheroes2::Point{ 3, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 100U, fheroes2::Point{ -3, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 106U, fheroes2::Point{ 3, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium (wide) water lake, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SWAMPY_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 113U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 108U, fheroes2::Point{ -1, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 110U, fheroes2::Point{ 1, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 109U, fheroes2::Point{ 0, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 112U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 107U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 111U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 114U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium (tall) water lake, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SWAMPY_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 119U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 115U, fheroes2::Point{ -1, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 116U, fheroes2::Point{ 0, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 118U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 122U, fheroes2::Point{ -1, 1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 123U, fheroes2::Point{ 0, 1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 117U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 120U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 121U, fheroes2::Point{ -2, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 124U, fheroes2::Point{ 1, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Single mandrake, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MANDRAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 126U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MANDRAKE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 125U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium-wide mandrake, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MANDRAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 129U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MANDRAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 128U, fheroes2::Point{ -1, 0 }, MP2::OBJ_MANDRAKE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 127U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Single mandrake, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MANDRAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 131U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MANDRAKE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 130U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide mandrakes, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MANDRAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 134U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MANDRAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 133U, fheroes2::Point{ -1, 0 }, MP2::OBJ_MANDRAKE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 135U, fheroes2::Point{ 1, 0 }, MP2::OBJ_MANDRAKE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 132U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Single small mandrake, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MANDRAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 137U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MANDRAKE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 136U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide swampy water lake, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SWAMPY_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 147U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 142U, fheroes2::Point{ 0, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 143U, fheroes2::Point{ 1, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 144U, fheroes2::Point{ 2, -1 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 145U, fheroes2::Point{ -2, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 146U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 148U, fheroes2::Point{ 1, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 149U, fheroes2::Point{ 2, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small wide swampy water lake, swamp terrain. 3 variants. In original editor is called "Nothing Special" and "Shrub".
        for ( uint32_t count = 0; count < 3; ++count ) {
            const uint32_t offset = 150U + count * 3U;
            Maps::ObjectInfo object{ MP2::OBJ_SWAMPY_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 2U, fheroes2::Point{ 1, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small swampy water lake, swamp terrain. In original editor is called "Nothing Special".
        {
            Maps::ObjectInfo object{ MP2::OBJ_SWAMPY_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 160U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 159U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SWAMPY_LAKE, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide reed, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 181U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 179U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 180U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 182U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 183U, fheroes2::Point{ 2, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 178U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium-wide reed, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 185U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 184U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 186U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small reed, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 187U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Rotten roots, swamp terrain. 2 variants. In original editor is called "Nothing Special".
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 188U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small rotten roots, swamp terrain. 2 variants. In original editor is called "Nothing Special".
        {
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 192U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium-wide shrub, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 194U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 193U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small shrub, swamp terrain. 2variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 195U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium-wide shrub, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 200U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 199U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small shrub, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 201U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Mossy roots, swamp terrain. In original editor is called "Nothing Special".
        {
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 213U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 212U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 211U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Crack, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium crater (wide), lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CRATER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 8U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 2U, fheroes2::Point{ -2, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 3U, fheroes2::Point{ -1, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 4U, fheroes2::Point{ 0, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 5U, fheroes2::Point{ 1, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 6U, fheroes2::Point{ -2, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 7U, fheroes2::Point{ -1, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 9U, fheroes2::Point{ 1, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium crater (high), lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CRATER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 15U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 12U, fheroes2::Point{ -1, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 13U, fheroes2::Point{ 0, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 14U, fheroes2::Point{ -1, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 16U, fheroes2::Point{ -1, 1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 17U, fheroes2::Point{ 0, 1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small lava pool, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LAVAPOOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 20U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 18U, fheroes2::Point{ 0, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 19U, fheroes2::Point{ -1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Passable lava, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 24U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 21U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 22U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 23U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 25U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Dug hole, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 26U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Big lava pool, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LAVAPOOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 36U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 27U, fheroes2::Point{ -3, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 28U, fheroes2::Point{ -2, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 29U, fheroes2::Point{ -1, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 30U, fheroes2::Point{ 0, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 31U, fheroes2::Point{ 1, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 32U, fheroes2::Point{ 2, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 33U, fheroes2::Point{ -3, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 34U, fheroes2::Point{ -2, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 35U, fheroes2::Point{ -1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 37U, fheroes2::Point{ 1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 38U, fheroes2::Point{ 2, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium lava pool, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LAVAPOOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 43U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 39U, fheroes2::Point{ -1, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 40U, fheroes2::Point{ 0, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 41U, fheroes2::Point{ 1, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 42U, fheroes2::Point{ -1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 44U, fheroes2::Point{ 1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium (little lifted) lava pool, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LAVAPOOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 51U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 46U, fheroes2::Point{ -1, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 47U, fheroes2::Point{ 0, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 48U, fheroes2::Point{ 1, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 50U, fheroes2::Point{ -1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 52U, fheroes2::Point{ 1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 49U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small lava streams, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LAVAPOOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 56U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 53U, fheroes2::Point{ -1, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 54U, fheroes2::Point{ 0, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 55U, fheroes2::Point{ -1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small lava streams, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LAVAPOOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 59U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 57U, fheroes2::Point{ 0, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 58U, fheroes2::Point{ -1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small lava streams, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LAVAPOOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 63U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 60U, fheroes2::Point{ -1, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 61U, fheroes2::Point{ 0, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 62U, fheroes2::Point{ -1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium lava streams, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LAVAPOOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 67U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 65U, fheroes2::Point{ 1, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 66U, fheroes2::Point{ -1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 68U, fheroes2::Point{ 1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 64U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium lava streams, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LAVAPOOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 72U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 69U, fheroes2::Point{ -1, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 70U, fheroes2::Point{ 0, -1 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 71U, fheroes2::Point{ -1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 73U, fheroes2::Point{ 1, 0 }, MP2::OBJ_LAVAPOOL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Smoking volcano crater (without the mountain), lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_VOLCANO };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 98U, fheroes2::Point{ 0, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 9;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 88U, fheroes2::Point{ -1, 0 }, MP2::OBJ_VOLCANO, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 9;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 78U, fheroes2::Point{ 0, -1 }, MP2::OBJ_VOLCANO );
            object.topLevelParts.back().animationFrames = 9;

            objects.emplace_back( std::move( object ) );
        }

        // Crack, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Tiny cactus.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 30U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 29U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Tiny cactus, desert terrain. 2 variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 31U + count * 1U;
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, offset + 0U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium cactus, desert terrain. 2 variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 33U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium cactus, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 39U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 38U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 37U, fheroes2::Point{ 0, -1 }, MP2::OBJ_CACTUS );

            objects.emplace_back( std::move( object ) );
        }

        // Small cactus, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 40U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium cactus, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 42U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 41U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium cactus, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 45U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 44U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 43U, fheroes2::Point{ 0, -1 }, MP2::OBJ_CACTUS );

            objects.emplace_back( std::move( object ) );
        }

        // Medium cactus, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 48U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 47U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small cactus, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 49U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium cactus, desert terrain. 2 variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 50U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Dug hole, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 68U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Big hole, desert terrain. In original editor is called "Nothing Special".
        {
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 111U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 110U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 112U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Crack, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 11U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 10U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Crack, dirt terrain. 3 variants.
        for ( uint32_t count = 0; count < 3; ++count ) {
            const uint32_t offset = 17U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_CRATER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Big water lake, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATER_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 35U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 23U, fheroes2::Point{ -4, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 24U, fheroes2::Point{ -3, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 25U, fheroes2::Point{ -2, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 26U, fheroes2::Point{ -1, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 27U, fheroes2::Point{ 0, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 28U, fheroes2::Point{ 1, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 29U, fheroes2::Point{ 2, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 30U, fheroes2::Point{ 3, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 31U, fheroes2::Point{ -4, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 32U, fheroes2::Point{ -3, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 33U, fheroes2::Point{ -2, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 34U, fheroes2::Point{ -1, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 36U, fheroes2::Point{ 1, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 37U, fheroes2::Point{ 2, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 38U, fheroes2::Point{ 3, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 40U, fheroes2::Point{ -3, 1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 41U, fheroes2::Point{ -2, 1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 42U, fheroes2::Point{ -1, 1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 43U, fheroes2::Point{ 0, 1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 44U, fheroes2::Point{ 1, 1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 45U, fheroes2::Point{ 2, 1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 39U, fheroes2::Point{ -4, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 46U, fheroes2::Point{ 3, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Medium water lake, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATER_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 55U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 48U, fheroes2::Point{ -2, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 49U, fheroes2::Point{ -1, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 50U, fheroes2::Point{ 0, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 51U, fheroes2::Point{ 1, -1 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 53U, fheroes2::Point{ -2, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 54U, fheroes2::Point{ -1, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 56U, fheroes2::Point{ 1, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small water lake, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATER_LAKE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 58U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 57U, fheroes2::Point{ -1, 0 }, MP2::OBJ_WATER_LAKE, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide shrub, dirt terrain. 3 variants.
        for ( uint32_t count = 0; count < 3; ++count ) {
            const uint32_t offset = 59U + count * 3U;
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 2U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 1U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 0U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Shrub, dirt terrain. 2 variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 68U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide flowers, dirt terrain. 4 variants.
        for ( uint32_t count = 0; count < 4; ++count ) {
            const uint32_t offset = 72U + count * 3U;
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 2U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 1U, fheroes2::Point{ -1, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 0U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small flowers, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 85U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 84U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Tiny flowers, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 86U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small flowers, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 88U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 87U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Tiny flowers, dirt terrain. 2 variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 89U + count * 1U;
            Maps::ObjectInfo object{ MP2::OBJ_FLOWERS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 0U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FLOWERS, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Tiny meadow, dirt terrain. In original editor is called "Nothing Special".
        {
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 106U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Wide meadow, dirt terrain. 2 variants. In original editor is called "Nothing Special".
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 107U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Grass (high), dirt terrain. In original editor is called "Nothing Special".
        {
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 112U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 111U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Grass, dirt terrain. In original editor is called "Nothing Special".
        {
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 113U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Dug hole, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 140U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Small cliff, dirt terrain. 3 variants.
        for ( uint32_t count = 0; count < 3; ++count ) {
            const uint32_t offset = 141U + count * 2U;
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Crack, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 6U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 5U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Cactus, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 14U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 13U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 12U, fheroes2::Point{ 0, -1 }, MP2::OBJ_CACTUS );

            objects.emplace_back( std::move( object ) );
        }

        // Cactus, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CACTUS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 16U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CACTUS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 15U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Cow scull, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NOTHING_SPECIAL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 17U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NOTHING_SPECIAL, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Shrub, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 57U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 56U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Big crack, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CRATER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 64U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 58U, fheroes2::Point{ -2, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 59U, fheroes2::Point{ -1, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 62U, fheroes2::Point{ -2, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 63U, fheroes2::Point{ -1, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 65U, fheroes2::Point{ 1, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 68U, fheroes2::Point{ 0, 1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 69U, fheroes2::Point{ 1, 1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 60U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 61U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 66U, fheroes2::Point{ -2, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 67U, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Dug hole, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 70U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Shrub, wasteland terrain. 2 variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 71U + count * 1U;
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, offset + 0U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Tar pit, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TAR_PIT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 113U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 10;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 76U, fheroes2::Point{ -2, -1 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 77U, fheroes2::Point{ -1, -1 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 78U, fheroes2::Point{ 0, -1 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 80U, fheroes2::Point{ -3, 0 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 10;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 91U, fheroes2::Point{ -2, 0 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 10;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 102U, fheroes2::Point{ -1, 0 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 10;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 124U, fheroes2::Point{ 1, 0 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 10;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 135U, fheroes2::Point{ 2, 0 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 136U, fheroes2::Point{ -3, 1 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 137U, fheroes2::Point{ -2, 1 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 10;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 148U, fheroes2::Point{ -1, 1 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 10;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 159U, fheroes2::Point{ 0, 1 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 10;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 170U, fheroes2::Point{ 1, 1 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 10;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 181U, fheroes2::Point{ 2, 1 }, MP2::OBJ_TAR_PIT, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Crack, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CRATER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 225U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 221U, fheroes2::Point{ -1, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 222U, fheroes2::Point{ 0, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 223U, fheroes2::Point{ 1, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 224U, fheroes2::Point{ -1, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 226U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Crack (vertical), wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CRATER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 230U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 227U, fheroes2::Point{ -1, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 228U, fheroes2::Point{ 0, -1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 229U, fheroes2::Point{ -1, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 231U, fheroes2::Point{ -1, 1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 232U, fheroes2::Point{ 0, 1 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Crack (horizontal), wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_CRATER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 234U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 233U, fheroes2::Point{ -1, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 235U, fheroes2::Point{ 1, 0 }, MP2::OBJ_CRATER, Maps::BACKGROUND_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Shrub, generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRUB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 64U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 63U, fheroes2::Point{ -1, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 65U, fheroes2::Point{ 1, 0 }, MP2::OBJ_SHRUB, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // River delta (ocean on bottom), generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 2U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 0U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 1U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 3U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 4U, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 5U, fheroes2::Point{ 0, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 6U, fheroes2::Point{ 1, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // River delta (ocean on top), generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 11U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 7U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 8U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 9U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 10U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 12U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 13U, fheroes2::Point{ 0, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // River delta (ocean on right), generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 2U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 1U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 3U, fheroes2::Point{ 0, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 4U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 5U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 6U, fheroes2::Point{ 1, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // River delta (ocean on left), generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_NONE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 11U, fheroes2::Point{ 0, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 7U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 8U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 9U, fheroes2::Point{ -1, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 10U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 12U, fheroes2::Point{ 0, 1 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 218 + 13U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            objects.emplace_back( std::move( object ) );
        }
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
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNTWBA, icnOffset + 7, fheroes2::Point{ 0, 1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
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
        addArtifactObject( Artifact::UNKNOWN, MP2::OBJ_RANDOM_ARTIFACT_TREASURE, 167U );
        addArtifactObject( Artifact::UNKNOWN, MP2::OBJ_RANDOM_ARTIFACT_MINOR, 169U );
        addArtifactObject( Artifact::UNKNOWN, MP2::OBJ_RANDOM_ARTIFACT_MAJOR, 171U );

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
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 74U, fheroes2::Point{ 0, 0 }, MP2::OBJ_RUINS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 73U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_RUINS, Maps::OBJECT_LAYER );

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
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 149U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

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
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 19U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

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
            Maps::ObjectInfo object{ MP2::OBJ_TROLL_BRIDGE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 189U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TROLL_BRIDGE, Maps::BACKGROUND_LAYER );
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
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 152U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CAVE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 151U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_CAVE, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Snow Cave (for Centaurs).
        {
            Maps::ObjectInfo object{ MP2::OBJ_CAVE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 3U, fheroes2::Point{ 0, 0 }, MP2::OBJ_CAVE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 2U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_CAVE, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Barrow Mounds (for Ghosts).
        {
            Maps::ObjectInfo object{ MP2::OBJ_BARROW_MOUNDS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 77U, fheroes2::Point{ 0, 0 }, MP2::OBJ_BARROW_MOUNDS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 76U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_BARROW_MOUNDS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 75U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_BARROW_MOUNDS, Maps::OBJECT_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 74U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_BARROW_MOUNDS );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 73U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_BARROW_MOUNDS );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 72U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Earth Summoning Altar (for Earth Elementals).
        {
            Maps::ObjectInfo object{ MP2::OBJ_EARTH_ALTAR };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 94U, fheroes2::Point{ 0, 0 }, MP2::OBJ_EARTH_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 8;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 103U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_EARTH_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 8;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 85U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_EARTH_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 8;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 84U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 83U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 82U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_EARTH_ALTAR );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 81U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_EARTH_ALTAR );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 80U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_EARTH_ALTAR );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 79U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 78U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Air Summoning Altar (for Air Elementals).
        {
            Maps::ObjectInfo object{ MP2::OBJ_AIR_ALTAR };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 118U, fheroes2::Point{ 0, 0 }, MP2::OBJ_AIR_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 119U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_AIR_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 117U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_AIR_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 116U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 112U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 115U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_AIR_ALTAR );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 114U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_AIR_ALTAR );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 113U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Fire Summoning Altar (for Fire Elementals).
        {
            Maps::ObjectInfo object{ MP2::OBJ_FIRE_ALTAR };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 127U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FIRE_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 128U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_FIRE_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 126U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_FIRE_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 125U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 124U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 123U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_FIRE_ALTAR );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 122U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_FIRE_ALTAR );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 121U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_FIRE_ALTAR );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 120U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Water Summoning Altar (for Water Elementals).
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATER_ALTAR };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 135U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATER_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 136U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_WATER_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 134U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_WATER_ALTAR, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 133U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 132U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_WATER_ALTAR );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 131U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_WATER_ALTAR );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 130U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_WATER_ALTAR );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 129U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateAdventureMines( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        // Mines provide 5 resources: Ore, Sulfur, Crystal, Gems, Gold.
        const uint8_t mineResources = 5;

        // Mines have different appearances: Generic, Grass, Snow, Swamp, Lava, Desert, Dirt, Wasteland.
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

            // The carts with these resources are placed above the main mine tile.
            for ( uint8_t resource = 0; resource < mineResources; ++resource ) {
                object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_EXTRAOVR, resource, fheroes2::Point{ 0, 0 }, MP2::OBJ_MINE, Maps::OBJECT_LAYER );

                // Set the metadata: 0 - resource type, 1 - income per day.
                switch ( resource ) {
                case 0:
                    object.metadata[0] = Resource::ORE;
                    object.metadata[1] = 2;
                    break;
                case 1:
                    object.metadata[0] = Resource::SULFUR;
                    object.metadata[1] = 1;
                    break;
                case 2:
                    object.metadata[0] = Resource::CRYSTAL;
                    object.metadata[1] = 1;
                    break;
                case 3:
                    object.metadata[0] = Resource::GEMS;
                    object.metadata[1] = 1;
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

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_EXTRAOVR, 5, fheroes2::Point{ 0, 0 }, MP2::OBJ_ABANDONED_MINE, Maps::OBJECT_LAYER );

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

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_EXTRAOVR, 5, fheroes2::Point{ 0, 0 }, MP2::OBJ_ABANDONED_MINE, Maps::OBJECT_LAYER );

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

        // Alchemist Lab.
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

        // Alchemist Lab (snow terrain).
        {
            Maps::ObjectInfo object{ MP2::OBJ_ALCHEMIST_LAB };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 150U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ALCHEMIST_LAB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 149U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_ALCHEMIST_LAB, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 148U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 151U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_ALCHEMIST_LAB, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 144U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 145U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_ALCHEMIST_LAB );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 146U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_ALCHEMIST_LAB );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 147U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_ALCHEMIST_LAB );

            // Set resource type and income per day.
            object.metadata[0] = Resource::MERCURY;
            object.metadata[1] = 1;

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateAdventurePowerUps( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        // Artesian Spring.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ARTESIAN_SPRING };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 3U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ARTESIAN_SPRING, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 4U, fheroes2::Point{ 1, 0 }, MP2::OBJ_ARTESIAN_SPRING, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 2U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 1U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NONE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 0U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Watering Hole.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATERING_HOLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 218U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATERING_HOLE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 217U, fheroes2::Point{ -1, 0 }, MP2::OBJ_WATERING_HOLE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 219U, fheroes2::Point{ 1, 0 }, MP2::OBJ_WATERING_HOLE, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 220U, fheroes2::Point{ 2, 0 }, MP2::OBJ_WATERING_HOLE, Maps::BACKGROUND_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 216U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NONE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 215U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 214U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Faerie Ring.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FAERIE_RING };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 129U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FAERIE_RING, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 130U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_FAERIE_RING, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 128U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Faerie Ring.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FAERIE_RING };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 30U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FAERIE_RING, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 31U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_FAERIE_RING, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 29U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Faerie Ring.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FAERIE_RING };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 84U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FAERIE_RING, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 85U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_FAERIE_RING, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 83U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Oasis.
        {
            Maps::ObjectInfo object{ MP2::OBJ_OASIS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 108U, fheroes2::Point{ 0, 0 }, MP2::OBJ_OASIS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 109U, fheroes2::Point{ 1, 0 }, MP2::OBJ_OASIS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 107U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 106U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_OASIS );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 105U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_OASIS );

            objects.emplace_back( std::move( object ) );
        }

        // Fountain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FOUNTAIN };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 15U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FOUNTAIN, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 14U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Magic Well.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MAGIC_WELL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 162U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MAGIC_WELL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 161U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Magic Well.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MAGIC_WELL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 165U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MAGIC_WELL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 164U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Magic Well.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MAGIC_WELL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 194U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MAGIC_WELL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 193U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 192U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Fort.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FORT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 59U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FORT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 58U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_FORT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 57U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 56U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_FORT );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 55U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_FORT );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 54U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 45U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NONE );
            object.topLevelParts.back().animationFrames = 8;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 36U, fheroes2::Point{ -1, -2 }, MP2::OBJ_NONE );
            object.topLevelParts.back().animationFrames = 8;

            objects.emplace_back( std::move( object ) );
        }

        // Gazebo.
        {
            Maps::ObjectInfo object{ MP2::OBJ_GAZEBO };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 62U, fheroes2::Point{ 0, 0 }, MP2::OBJ_GAZEBO, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 61U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 60U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_FORT );

            objects.emplace_back( std::move( object ) );
        }

        // Witch Doctor's Hut.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WITCH_DOCTORS_HUT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 69U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WITCH_DOCTORS_HUT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 68U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 67U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 66U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_FORT );

            objects.emplace_back( std::move( object ) );
        }

        // Mercenary Camp.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MERCENARY_CAMP };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 71U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MERCENARY_CAMP, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 72U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_MERCENARY_CAMP, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 70U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_MERCENARY_CAMP, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Shrine of the First Circle.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRINE_FIRST_CIRCLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 80U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRINE_FIRST_CIRCLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 79U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Shrine of the Second Circle.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRINE_SECOND_CIRCLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 76U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRINE_SECOND_CIRCLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 75U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Shrine of the Third Circle.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SHRINE_THIRD_CIRCLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 78U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SHRINE_THIRD_CIRCLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 77U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Idol.
        {
            Maps::ObjectInfo object{ MP2::OBJ_IDOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 82U, fheroes2::Point{ 0, 0 }, MP2::OBJ_IDOL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 81U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Standing Stones.
        {
            Maps::ObjectInfo object{ MP2::OBJ_STANDING_STONES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 84U, fheroes2::Point{ 0, 0 }, MP2::OBJ_STANDING_STONES, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 85U, fheroes2::Point{ 1, 0 }, MP2::OBJ_STANDING_STONES, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 83U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Temple.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TEMPLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 88U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TEMPLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 89U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_TEMPLE, Maps::OBJECT_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 87U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_FORT );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 86U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_FORT );

            objects.emplace_back( std::move( object ) );
        }

        // Tree of Knowledge.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TREE_OF_KNOWLEDGE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 123U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TREE_OF_KNOWLEDGE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 122U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 121U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 120U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_TREE_OF_KNOWLEDGE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 119U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_TREE_OF_KNOWLEDGE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 118U, fheroes2::Point{ 0, -2 }, MP2::OBJ_NON_ACTION_TREE_OF_KNOWLEDGE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 117U, fheroes2::Point{ -1, -2 }, MP2::OBJ_NONE );

            objects.emplace_back( std::move( object ) );
        }

        // Xanadu.
        {
            Maps::ObjectInfo object{ MP2::OBJ_XANADU };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 81U, fheroes2::Point{ 0, 0 }, MP2::OBJ_XANADU, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 82U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_XANADU, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 74U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_XANADU, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 67U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_XANADU, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 66U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 65U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_XANADU );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 58U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_XANADU );
            object.topLevelParts.back().animationFrames = 6;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 51U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_XANADU );
            object.topLevelParts.back().animationFrames = 6;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 50U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NON_ACTION_XANADU );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 43U, fheroes2::Point{ -3, -1 }, MP2::OBJ_NON_ACTION_XANADU );
            object.topLevelParts.back().animationFrames = 6;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 42U, fheroes2::Point{ 1, -2 }, MP2::OBJ_NONE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 41U, fheroes2::Point{ 0, -2 }, MP2::OBJ_NONE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 34U, fheroes2::Point{ -1, -2 }, MP2::OBJ_NON_ACTION_XANADU );
            object.topLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Arena.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ARENA };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 70U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ARENA, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 71U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_ARENA, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 69U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_ARENA, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 68U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 59U, fheroes2::Point{ -3, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 8;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 50U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_ARENA, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 8;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 49U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_ARENA, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 40U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_ARENA, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 8;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 31U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 8;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 22U, fheroes2::Point{ 1, -2 }, MP2::OBJ_NON_ACTION_ARENA );
            object.topLevelParts.back().animationFrames = 8;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 13U, fheroes2::Point{ 0, -2 }, MP2::OBJ_NON_ACTION_ARENA );
            object.topLevelParts.back().animationFrames = 8;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 4U, fheroes2::Point{ -1, -2 }, MP2::OBJ_NONE );
            object.topLevelParts.back().animationFrames = 8;

            objects.emplace_back( std::move( object ) );
        }
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

        // Boat, direction: right.
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

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 52, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 54, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_MAGELLANS_MAPS );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 53, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_MAGELLANS_MAPS );

            objects.emplace_back( std::move( object ) );
        }

        // Whirlpool. The center point is middle lower tile.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WHIRLPOOL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 218, fheroes2::Point{ 0, 0 }, MP2::OBJ_WHIRLPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 222, fheroes2::Point{ 1, 0 }, MP2::OBJ_WHIRLPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 214, fheroes2::Point{ -1, 0 }, MP2::OBJ_WHIRLPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 206, fheroes2::Point{ 0, -1 }, MP2::OBJ_WHIRLPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 210, fheroes2::Point{ 1, -1 }, MP2::OBJ_WHIRLPOOL, Maps::BACKGROUND_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNWATR, 202, fheroes2::Point{ -1, -1 }, MP2::OBJ_WHIRLPOOL, Maps::BACKGROUND_LAYER );
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

        // Graveyard, generic terrain.
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

        // Graveyard, snow terrain.
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

        // Hill Fort, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_HILL_FORT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 4U, fheroes2::Point{ 0, 0 }, MP2::OBJ_HILL_FORT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 3U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_HILL_FORT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 2U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_HILL_FORT, Maps::OBJECT_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 0U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_HILL_FORT );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 1U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_HILL_FORT );

            objects.emplace_back( std::move( object ) );
        }

        // Hill Fort, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_HILL_FORT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 135U, fheroes2::Point{ 0, 0 }, MP2::OBJ_HILL_FORT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 134U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_HILL_FORT, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 133U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_HILL_FORT, Maps::OBJECT_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 131U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_HILL_FORT );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 132U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_HILL_FORT );

            objects.emplace_back( std::move( object ) );
        }

        // Windmill, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WINDMILL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 55U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WINDMILL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 59U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_WINDMILL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 31U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 35U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 47U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 51U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 23U, fheroes2::Point{ 0, -2 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 27U, fheroes2::Point{ 1, -2 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 39U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 43U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;

            objects.emplace_back( std::move( object ) );
        }

        // Windmill, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WINDMILL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 128U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WINDMILL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 132U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_WINDMILL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 104U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 108U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 120U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 124U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 96U, fheroes2::Point{ 0, -2 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 100U, fheroes2::Point{ 1, -2 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 112U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 116U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;

            objects.emplace_back( std::move( object ) );
        }

        // Windmill, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WINDMILL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 185U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WINDMILL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 189U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_WINDMILL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 3;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 161U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 165U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 177U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 181U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 3;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 153U, fheroes2::Point{ 0, -2 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 157U, fheroes2::Point{ 1, -2 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 169U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 173U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_WINDMILL );
            object.topLevelParts.back().animationFrames = 3;

            objects.emplace_back( std::move( object ) );
        }

        // Oracle, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ORACLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 126U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ORACLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 125U, fheroes2::Point{ -1, 0 }, MP2::OBJ_ORACLE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 124U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 122U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_ORACLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 123U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_ORACLE );

            objects.emplace_back( std::move( object ) );
        }

        // Oracle, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ORACLE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 198U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ORACLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 197U, fheroes2::Point{ -1, 0 }, MP2::OBJ_ORACLE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 196U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 193U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 194U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_ORACLE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 195U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_ORACLE );

            objects.emplace_back( std::move( object ) );
        }

        // Obelisk, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_OBELISK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 129U, fheroes2::Point{ 0, 0 }, MP2::OBJ_OBELISK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 128U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRA2, 127U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_OBELISK );

            objects.emplace_back( std::move( object ) );
        }

        // Obelisk, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_OBELISK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 141U, fheroes2::Point{ 0, 0 }, MP2::OBJ_OBELISK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 140U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 139U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_OBELISK );

            objects.emplace_back( std::move( object ) );
        }

        // Obelisk, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_OBELISK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 216U, fheroes2::Point{ 0, 0 }, MP2::OBJ_OBELISK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 215U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 214U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_OBELISK );

            objects.emplace_back( std::move( object ) );
        }

        // Obelisk, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_OBELISK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 110U, fheroes2::Point{ 0, 0 }, MP2::OBJ_OBELISK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 109U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 108U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_OBELISK );

            objects.emplace_back( std::move( object ) );
        }

        // Obelisk, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_OBELISK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 104U, fheroes2::Point{ 0, 0 }, MP2::OBJ_OBELISK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 103U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 102U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_OBELISK );

            objects.emplace_back( std::move( object ) );
        }

        // Obelisk, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_OBELISK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 238U, fheroes2::Point{ 0, 0 }, MP2::OBJ_OBELISK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 237U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 236U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_OBELISK );

            objects.emplace_back( std::move( object ) );
        }

        // Obelisk, dirt terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_OBELISK };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 201U, fheroes2::Point{ 0, 0 }, MP2::OBJ_OBELISK, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 200U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDIRT, 199U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_OBELISK );

            objects.emplace_back( std::move( object ) );
        }

        // Lean-to, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LEAN_TO };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 13U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LEAN_TO, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 12U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Lean-to, grass terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LEAN_TO };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 154U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LEAN_TO, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNGRAS, 153U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Sign, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SIGN };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 143U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SIGN, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 142U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Sign, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SIGN };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 140U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SIGN, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Sign, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SIGN };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 117U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SIGN, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 116U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Sign, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SIGN };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 119U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SIGN, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 118U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Sign, generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SIGN };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 114U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SIGN, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 113U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Water wheel, snow terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATER_WHEEL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 191U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATER_WHEEL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 177U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_WATER_WHEEL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 184U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_WATER_WHEEL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 161U, fheroes2::Point{ -1, -2 }, MP2::OBJ_NON_ACTION_WATER_WHEEL );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 162U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NON_ACTION_WATER_WHEEL );
            object.topLevelParts.back().animationFrames = 6;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 169U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_WATER_WHEEL );
            object.topLevelParts.back().animationFrames = 6;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 176U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_WATER_WHEEL );

            objects.emplace_back( std::move( object ) );
        }

        // Water wheel, generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WATER_WHEEL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 112U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATER_WHEEL, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 98U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NON_ACTION_WATER_WHEEL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 105U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_WATER_WHEEL, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 82U, fheroes2::Point{ -1, -2 }, MP2::OBJ_NON_ACTION_WATER_WHEEL );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 83U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NON_ACTION_WATER_WHEEL );
            object.topLevelParts.back().animationFrames = 6;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 90U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_WATER_WHEEL );
            object.topLevelParts.back().animationFrames = 6;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 97U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_WATER_WHEEL );

            objects.emplace_back( std::move( object ) );
        }

        // Wagon.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WAGON };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 74, fheroes2::Point{ 0, 0 }, MP2::OBJ_WAGON, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 73, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Witch's Hut, swamp terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_WITCHS_HUT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 22U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WITCHS_HUT, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 14U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 21U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 0U, fheroes2::Point{ 0, -2 }, MP2::OBJ_NON_ACTION_WITCHS_HUT );
            object.topLevelParts.back().animationFrames = 6;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSWMP, 7U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_WITCHS_HUT );
            object.topLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Daemon Cave, lava terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DAEMON_CAVE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 115U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DAEMON_CAVE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 114U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_DAEMON_CAVE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 113U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 111U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_DAEMON_CAVE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNLAVA, 112U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_DAEMON_CAVE );

            objects.emplace_back( std::move( object ) );
        }

        // Daemon Cave, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DAEMON_CAVE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 117U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DAEMON_CAVE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 116U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_DAEMON_CAVE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 115U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 113U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_DAEMON_CAVE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 114U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_DAEMON_CAVE );

            objects.emplace_back( std::move( object ) );
        }
        // Pyramid, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_PYRAMID };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 82U, fheroes2::Point{ 0, 0 }, MP2::OBJ_PYRAMID, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 81U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_PYRAMID, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 80U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 77U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 78U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_PYRAMID );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 79U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_PYRAMID );

            objects.emplace_back( std::move( object ) );
        }

        // Skeleton, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SKELETON };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 84U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SKELETON, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 83U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_SKELETON, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Sphinx, desert terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_SPHINX };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 87U, fheroes2::Point{ 0, 0 }, MP2::OBJ_SPHINX, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 88U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NON_ACTION_SPHINX, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 86U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 85U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_SPHINX );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 131U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_SPHINX );

            objects.emplace_back( std::move( object ) );
        }

        // Trading Post, wasteland terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TRADING_POST };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 213U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TRADING_POST, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 202U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_TRADING_POST, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 10;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 201U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 190U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_TRADING_POST );
            object.topLevelParts.back().animationFrames = 10;

            objects.emplace_back( std::move( object ) );
        }

        // Trading Post, generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_TRADING_POST };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 111U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TRADING_POST, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 104U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_TRADING_POST, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 97U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMULT, 90U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_TRADING_POST );
            object.topLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Lighthouse, generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_LIGHTHOUSE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 73U, fheroes2::Point{ 0, 0 }, MP2::OBJ_LIGHTHOUSE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 60U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 71U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 72U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 59U, fheroes2::Point{ 0, -2 }, MP2::OBJ_NON_ACTION_LIGHTHOUSE );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 61U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_LIGHTHOUSE );
            object.topLevelParts.back().animationFrames = 9;

            objects.emplace_back( std::move( object ) );
        }

        // Stone Liths, generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_STONE_LITHS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 116U, fheroes2::Point{ 0, 0 }, MP2::OBJ_STONE_LITHS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 115U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Stone Liths, generic terrain. 2 variants.
        for ( uint32_t count = 0; count < 2; ++count ) {
            const uint32_t offset = 117U + count * 3U;
            Maps::ObjectInfo object{ MP2::OBJ_STONE_LITHS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, offset + 2U, fheroes2::Point{ 0, 0 }, MP2::OBJ_STONE_LITHS, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, offset + 1U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, offset + 0U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_STONE_LITHS );

            objects.emplace_back( std::move( object ) );
        }

        // Event, generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_EVENT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 163U, fheroes2::Point{ 0, 0 }, MP2::OBJ_EVENT, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Freeman's Foundry, generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_FREEMANS_FOUNDRY };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 188U, fheroes2::Point{ 0, 0 }, MP2::OBJ_FREEMANS_FOUNDRY, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 187U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_FREEMANS_FOUNDRY, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 180U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 166U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_FREEMANS_FOUNDRY );
            object.topLevelParts.back().animationFrames = 6;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 173U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_FREEMANS_FOUNDRY );
            object.topLevelParts.back().animationFrames = 6;

            objects.emplace_back( std::move( object ) );
        }

        // Magic Garden, generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_MAGIC_GARDEN };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 190U, fheroes2::Point{ 0, 0 }, MP2::OBJ_MAGIC_GARDEN, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 6;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 189U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Observation Tower, generic terrain.
        {
            Maps::ObjectInfo object{ MP2::OBJ_OBSERVATION_TOWER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 201U, fheroes2::Point{ 0, 0 }, MP2::OBJ_OBSERVATION_TOWER, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 199U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 200U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 198U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_OBSERVATION_TOWER );

            objects.emplace_back( std::move( object ) );
        }

        // Alchemist's Tower, PoL object.
        {
            Maps::ObjectInfo object{ MP2::OBJ_ALCHEMIST_TOWER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 3U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ALCHEMIST_TOWER, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 1U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 2U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC1, 0U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_ALCHEMIST_TOWER );

            objects.emplace_back( std::move( object ) );
        }

        // Stables, PoL object.
        {
            Maps::ObjectInfo object{ MP2::OBJ_STABLES };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 4U, fheroes2::Point{ 0, 0 }, MP2::OBJ_STABLES, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 3U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_STABLES, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 2U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 0U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_STABLES );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 1U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_STABLES );

            objects.emplace_back( std::move( object ) );
        }

        // Jail, PoL object.
        {
            Maps::ObjectInfo object{ MP2::OBJ_JAIL };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 9U, fheroes2::Point{ 0, 0 }, MP2::OBJ_JAIL, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 7U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 8U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 5U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NON_ACTION_JAIL );
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC2, 6U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_JAIL );

            objects.emplace_back( std::move( object ) );
        }

        // Hut of the Magi, PoL object.
        {
            Maps::ObjectInfo object{ MP2::OBJ_HUT_OF_MAGI };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, 30U, fheroes2::Point{ 0, 0 }, MP2::OBJ_HUT_OF_MAGI, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, 0U, fheroes2::Point{ -2, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 8;
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, 9U, fheroes2::Point{ -1, -1 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, 20U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, 29U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, 31U, fheroes2::Point{ 1, 0 }, MP2::OBJ_NONE, Maps::TERRAIN_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, 10U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_HUT_OF_MAGI );
            object.topLevelParts.back().animationFrames = 8;
            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, 19U, fheroes2::Point{ 1, -1 }, MP2::OBJ_NON_ACTION_HUT_OF_MAGI );

            objects.emplace_back( std::move( object ) );
        }

        // Eye of the Magi, PoL object.
        {
            Maps::ObjectInfo object{ MP2::OBJ_EYE_OF_MAGI };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, 50U, fheroes2::Point{ 0, 0 }, MP2::OBJ_EYE_OF_MAGI, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 8;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, 41U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.back().animationFrames = 8;

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, 32U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_EYE_OF_MAGI );
            object.topLevelParts.back().animationFrames = 8;

            objects.emplace_back( std::move( object ) );
        }

        // Barrier, PoL object. 8 variants: Aqua, Blue, Brown, Gold, Green, Orange, Purple, Red
        for ( uint32_t count = 0; count < 8; ++count ) {
            const uint32_t offset = 59U + count * 6U;
            Maps::ObjectInfo object{ MP2::OBJ_BARRIER };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, offset + 1U, fheroes2::Point{ 0, 0 }, MP2::OBJ_BARRIER, Maps::OBJECT_LAYER );
            object.groundLevelParts.back().animationFrames = 4;

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, offset + 0U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            // Barrier colors start from 1.
            object.metadata[0] = count + 1;

            objects.emplace_back( std::move( object ) );
        }

        // Traveller's Tent, PoL object. 8 variants: Aqua, Blue, Brown, Gold, Green, Orange, Purple, Red
        for ( uint32_t count = 0; count < 8; ++count ) {
            const uint32_t offset = 107U + count * 4U;
            Maps::ObjectInfo object{ MP2::OBJ_TRAVELLER_TENT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, offset + 3U, fheroes2::Point{ 0, 0 }, MP2::OBJ_TRAVELLER_TENT, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, offset + 1U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, offset + 2U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            object.topLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_X_LOC3, offset + 0U, fheroes2::Point{ 0, -1 }, MP2::OBJ_NON_ACTION_TRAVELLER_TENT );

            // Traveller's Tent colors start from 1.
            object.metadata[0] = count + 1;

            objects.emplace_back( std::move( object ) );
        }

        // Graveyard, grass terrain, ugly version - for compatibility.
        {
            Maps::ObjectInfo object{ MP2::OBJ_GRAVEYARD };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 58U, fheroes2::Point{ 0, 0 }, MP2::OBJ_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 57U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNMUL2, 56U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Graveyard, snow terrain, ugly version - for compatibility.
        {
            Maps::ObjectInfo object{ MP2::OBJ_GRAVEYARD };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 160U, fheroes2::Point{ 0, 0 }, MP2::OBJ_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 159U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 158U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Graveyard, desert terrain(?), ugly version - for compatibility.
        {
            Maps::ObjectInfo object{ MP2::OBJ_GRAVEYARD };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 122U, fheroes2::Point{ 0, 0 }, MP2::OBJ_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 121U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_GRAVEYARD, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 120U, fheroes2::Point{ -2, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }
    }

    void populateKingdomHeroes( std::vector<Maps::ObjectInfo> & objects )
    {
        assert( objects.empty() );

        // Only 7 races and 6 colors exist in the game.
        for ( int32_t color = 0; color < 6; ++color ) {
            for ( int32_t race = 0; race < 7; ++race ) {
                Maps::ObjectInfo object{ MP2::OBJ_HERO };
                object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_MINIHERO, color * 7 + race, fheroes2::Point{ 0, 0 }, MP2::OBJ_HERO, Maps::OBJECT_LAYER );
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

        auto addTown = [&objects]( const MP2::MapObjectType mainObjectType, const uint8_t race, const uint8_t townIcnOffset, const uint8_t shadowIcnOffset,
                                   const MP2::ObjectIcnType townIcnType, const MP2::ObjectIcnType shadowIcnType, const bool isCastle ) {
            assert( MP2::isOffGameActionObject( mainObjectType ) );

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

            object.metadata[0] = race;
            object.metadata[1] = ( isCastle ? 1 : 0 );

            objects.emplace_back( std::move( object ) );
        };

        // Castle and town sprites: Knight, Barbarian, Sorceress, Warlock, Wizard, Necromancer.
        // First goes castle, then town, then the next race.
        for ( uint8_t i = 0; i < 6 * 2; ++i ) {
            const uint8_t icnOffset = i * 16;
            const uint8_t race = i / 2;
            const bool isCastle = ( i % 2 ) == 0;

            addTown( MP2::OBJ_CASTLE, race, icnOffset, icnOffset, MP2::OBJ_ICN_TYPE_OBJNTOWN, MP2::OBJ_ICN_TYPE_OBJNTWSH, isCastle );
        }

        // Random castle/town
        addTown( MP2::OBJ_RANDOM_CASTLE, 7, 0, 32, MP2::OBJ_ICN_TYPE_OBJNTWRD, MP2::OBJ_ICN_TYPE_OBJNTWRD, true );
        addTown( MP2::OBJ_RANDOM_TOWN, 7, 16, 16 + 32, MP2::OBJ_ICN_TYPE_OBJNTWRD, MP2::OBJ_ICN_TYPE_OBJNTWRD, false );
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

    void populateExtraBoatDirections( std::vector<Maps::ObjectInfo> & objects )
    {
        // Boat, direction: top.
        {
            Maps::ObjectInfo object{ MP2::OBJ_BOAT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_BOAT32, 0, fheroes2::Point{ 0, 0 }, MP2::OBJ_BOAT, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Boat, direction: top-right.
        {
            Maps::ObjectInfo object{ MP2::OBJ_BOAT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_BOAT32, 9, fheroes2::Point{ 0, 0 }, MP2::OBJ_BOAT, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Boat, direction: bottom-right.
        {
            Maps::ObjectInfo object{ MP2::OBJ_BOAT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_BOAT32, 27, fheroes2::Point{ 0, 0 }, MP2::OBJ_BOAT, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Boat, direction: bottom.
        {
            Maps::ObjectInfo object{ MP2::OBJ_BOAT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_BOAT32, 36, fheroes2::Point{ 0, 0 }, MP2::OBJ_BOAT, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Boat, direction: bottom-left.
        // TODO: Redo direction setting not to use pseudo sprite index that leads to an empty image.
        {
            Maps::ObjectInfo object{ MP2::OBJ_BOAT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_BOAT32, 27 + 128, fheroes2::Point{ 0, 0 }, MP2::OBJ_BOAT, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Boat, direction: left.
        // TODO: Redo direction setting not to use pseudo sprite index that leads to an empty image.
        {
            Maps::ObjectInfo object{ MP2::OBJ_BOAT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_BOAT32, 18 + 128, fheroes2::Point{ 0, 0 }, MP2::OBJ_BOAT, Maps::OBJECT_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Boat, direction: top-left.
        // TODO: Redo direction setting not to use pseudo sprite index that leads to an empty image.
        {
            Maps::ObjectInfo object{ MP2::OBJ_BOAT };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_BOAT32, 9 + 128, fheroes2::Point{ 0, 0 }, MP2::OBJ_BOAT, Maps::OBJECT_LAYER );

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

        populateExtraBoatDirections( objectData[static_cast<size_t>( Maps::ObjectGroup::MAP_EXTRAS )] );

        for ( const auto & objects : objectData ) {
            for ( const auto & objectInfo : objects ) {
                // We accept that there could be duplicates so we don't check if the insertion is successful for the map.

                for ( const auto & info : objectInfo.groundLevelParts ) {
                    objectInfoByIcn.try_emplace( std::make_pair( info.icnType, info.icnIndex ), &info );
                }

                for ( const auto & info : objectInfo.topLevelParts ) {
                    objectInfoByIcn.try_emplace( std::make_pair( info.icnType, info.icnIndex ), &info );
                }
            }
        }

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
                    if ( info.layerType == Maps::SHADOW_LAYER || info.layerType == Maps::TERRAIN_LAYER ) {
                        // Shadow layer never has an object on it.
                        assert( info.objectType == MP2::OBJ_NONE );
                    }
                    else {
                        // All parts should belong to the object.
                        assert( info.objectType == type || info.objectType == secondaryType );
                    }
                }

                if ( objectInfo.groundLevelParts.front().layerType == Maps::TERRAIN_LAYER ) {
                    // Terrain objects must not have top level parts.
                    assert( objectInfo.topLevelParts.empty() );

                    // All parts of the object must be the same layer type.
                    for ( const auto & info : objectInfo.groundLevelParts ) {
                        assert( info.layerType == Maps::TERRAIN_LAYER );
                    }
                }
            }
        }

        // Check that all landscape objects are non-action objects.
        for ( size_t groupType = static_cast<size_t>( Maps::ObjectGroup::ROADS ); groupType <= static_cast<size_t>( Maps::ObjectGroup::LANDSCAPE_FLAGS ); ++groupType ) {
            const auto & objects = objectData[groupType];

            for ( const auto & objectInfo : objects ) {
                assert( !MP2::isOffGameActionObject( objectInfo.objectType ) );
            }
        }

        // Check that all other objects are action objects.
        for ( size_t groupType = static_cast<size_t>( Maps::ObjectGroup::ADVENTURE_ARTIFACTS ); groupType < static_cast<size_t>( Maps::ObjectGroup::GROUP_COUNT );
              ++groupType ) {
            const auto & objects = objectData[groupType];

            for ( const auto & objectInfo : objects ) {
                assert( MP2::isOffGameActionObject( objectInfo.objectType ) );
            }
        }

        // Check that an image part is set to the same layer and object type for all objects.
        std::map<std::pair<MP2::ObjectIcnType, uint32_t>, std::pair<Maps::ObjectLayerType, MP2::MapObjectType>> groundObjectInfoVsObjectType;
        std::set<std::pair<MP2::ObjectIcnType, uint32_t>> topObjectInfo;

        for ( const auto & objects : objectData ) {
            for ( const auto & objectInfo : objects ) {
                for ( const auto & info : objectInfo.groundLevelParts ) {
                    if ( const auto [iter, inserted]
                         = groundObjectInfoVsObjectType.emplace( std::make_pair( info.icnType, info.icnIndex ), std::make_pair( info.layerType, info.objectType ) );
                         !inserted ) {
                        assert( iter->second.first == info.layerType && iter->second.second == info.objectType );
                    }

                    assert( topObjectInfo.find( std::make_pair( info.icnType, info.icnIndex ) ) == topObjectInfo.end() );
                }

                for ( const auto & info : objectInfo.topLevelParts ) {
                    topObjectInfo.emplace( info.icnType, info.icnIndex );

                    assert( groundObjectInfoVsObjectType.find( std::make_pair( info.icnType, info.icnIndex ) ) == groundObjectInfoVsObjectType.end() );
                }
            }
        }
#endif

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

    const ObjectInfo & getObjectInfo( const ObjectGroup group, const int32_t objectIndex )
    {
        const auto & objectInfo = getObjectsByGroup( group );
        if ( objectIndex < 0 || objectIndex >= static_cast<int32_t>( objectInfo.size() ) ) {
            assert( 0 );
            static const ObjectInfo emptyObjectInfo{ MP2::OBJ_NONE };
            return emptyObjectInfo;
        }

        return objectInfo[objectIndex];
    }

    const ObjectPartInfo * getObjectPartByIcn( const MP2::ObjectIcnType icnType, const uint32_t icnIndex )
    {
        populateObjectData();

        auto iter = objectInfoByIcn.find( std::make_pair( icnType, icnIndex ) );
        if ( iter != objectInfoByIcn.end() ) {
            return iter->second;
        }

        // You can reach this code by 3 reasons:
        // - you are passing invalid object information
        // - you updated object properties but didn't do object info migration for save files
        // - you are trying to get info of an object created by an original Editor
        return nullptr;
    }

    MP2::MapObjectType getObjectTypeByIcn( const MP2::ObjectIcnType icnType, const uint32_t icnIndex )
    {
        const ObjectPartInfo * objectPart = getObjectPartByIcn( icnType, icnIndex );
        if ( objectPart != nullptr ) {
            return objectPart->objectType;
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

    std::vector<fheroes2::Point> getGroundLevelUsedTileOffset( const ObjectInfo & info )
    {
        // If this assertion blows up then the object is not formed properly.
        assert( !info.groundLevelParts.empty() );

        std::vector<fheroes2::Point> offsets;
        for ( const auto & objectPart : info.groundLevelParts ) {
            if ( objectPart.layerType != SHADOW_LAYER ) {
                offsets.push_back( objectPart.tileOffset );
            }
        }

        return offsets;
    }
}
