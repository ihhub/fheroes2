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
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 14U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DUNE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 15U, fheroes2::Point{ 1, 0 }, MP2::OBJ_DUNE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 13U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Desert mound.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DUNE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 17U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DUNE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 18U, fheroes2::Point{ 1, 0 }, MP2::OBJ_DUNE, Maps::OBJECT_LAYER );

            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 19U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NONE, Maps::SHADOW_LAYER );

            objects.emplace_back( std::move( object ) );
        }

        // Desert dune.
        {
            Maps::ObjectInfo object{ MP2::OBJ_DUNE };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 21U, fheroes2::Point{ 0, 0 }, MP2::OBJ_DUNE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 20U, fheroes2::Point{ -1, 0 }, MP2::OBJ_DUNE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 22U, fheroes2::Point{ 1, 0 }, MP2::OBJ_DUNE, Maps::OBJECT_LAYER );

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
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNSNOW, 28U, fheroes2::Point{ 0, 0 }, MP2::OBJ_ROCK, Maps::OBJECT_LAYER );

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
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 218U, fheroes2::Point{ 0, 0 }, MP2::OBJ_WATERING_HOLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 217U, fheroes2::Point{ -1, 0 }, MP2::OBJ_WATERING_HOLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 219U, fheroes2::Point{ 1, 0 }, MP2::OBJ_WATERING_HOLE, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNCRCK, 220U, fheroes2::Point{ 2, 0 }, MP2::OBJ_WATERING_HOLE, Maps::OBJECT_LAYER );

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

        // Oasis.
        {
            Maps::ObjectInfo object{ MP2::OBJ_OASIS };
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 108U, fheroes2::Point{ 0, 0 }, MP2::OBJ_OASIS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 109U, fheroes2::Point{ 1, 0 }, MP2::OBJ_OASIS, Maps::OBJECT_LAYER );
            object.groundLevelParts.emplace_back( MP2::OBJ_ICN_TYPE_OBJNDSRT, 107U, fheroes2::Point{ -1, 0 }, MP2::OBJ_NON_ACTION_OASIS, Maps::OBJECT_LAYER );

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
