/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2026                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

namespace Maps
{
    class Tile;

    namespace Ground
    {
        enum : int32_t
        {
            UNKNOWN = 0x0000,
            DESERT = 0x0001,
            SNOW = 0x0002,
            SWAMP = 0x0004,
            WASTELAND = 0x0008,
            BEACH = 0x0010,
            LAVA = 0x0020,
            DIRT = 0x0040,
            GRASS = 0x0080,
            WATER = 0x0100,
            ALL = DESERT | SNOW | SWAMP | WASTELAND | BEACH | LAVA | DIRT | GRASS
        };

        // These are start image indecies from TIL::GROUND32 image set.
        enum GroundImageStartIndex : uint16_t
        {
            WATER_START_IMAGE_INDEX = 0U,
            GRASS_START_IMAGE_INDEX = 30U,
            SNOW_START_IMAGE_INDEX = 92U,
            SWAMP_START_IMAGE_INDEX = 146U,
            LAVA_START_IMAGE_INDEX = 208U,
            DESERT_START_IMAGE_INDEX = 262U,
            DIRT_START_IMAGE_INDEX = 321U,
            WASTELAND_START_IMAGE_INDEX = 361U,
            BEACH_START_IMAGE_INDEX = 415U,
            MAX_IMAGE_INDEX = 432U
        };

        // Returns index of first ground image in GROUND32.TIL
        uint16_t getTerrainStartImageIndex( const int32_t groundId );
        // Returns ground index by ground image index in GROUND32.TIL
        int32_t getGroundByImageIndex( const uint16_t terrainImageIndex );
        // Returns true if ground image index corresponds to image with transition to other ground.
        bool isTerrainTransitionImage( const uint16_t terrainImageIndex );
        bool doesTerrainImageIndexContainEmbeddedObjects( const uint16_t terrainImageIndex );

        constexpr uint32_t roadPenalty{ 75 };
        constexpr uint32_t defaultGroundPenalty{ 100 };

        constexpr uint32_t fastestMovePenalty{ roadPenalty };
        constexpr uint32_t slowestMovePenalty{ 200 };

        const char * String( const int32_t groundId );
        uint32_t GetPenalty( const Maps::Tile & tile, const uint32_t pathfindingLevel );

        // Returns the random ground image index (used in GROUND32.TIL) for main (without transition) terrain layout.
        uint16_t getRandomTerrainImageIndex( const int32_t groundId, const bool allowEmbeddedObjectsAppearOnTerrain );
    }
}
