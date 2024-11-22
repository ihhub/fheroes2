/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#ifndef H2MAPSGROUND_H
#define H2MAPSGROUND_H

#include <cstdint>

namespace Maps
{
    class Tile;

    namespace Ground
    {
        enum
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

        // Returns index of first ground image in GROUND32.TIL
        uint16_t getTerrainStartImageIndex( const int groundId );
        // Returns ground index by ground image index in GROUND32.TIL
        int getGroundByImageIndex( const uint16_t terrainImageIndex );
        // Returns true if ground image index corresponds to image with transition to other ground.
        bool isTerrainTransitionImage( const uint16_t terrainImageIndex );
        bool doesTerrainImageIndexContainEmbeddedObjects( const uint16_t terrainImageIndex );

        const uint32_t roadPenalty = 75;
        const uint32_t defaultGroundPenalty = 100;

        const uint32_t fastestMovePenalty = roadPenalty;
        const uint32_t slowestMovePenalty = 200;

        const char * String( int groundId );
        uint32_t GetPenalty( const Maps::Tile & tile, uint32_t pathfindingLevel );

        // Returns the random ground image index (used in GROUND32.TIL) for main (without transition) terrain layout.
        uint16_t getRandomTerrainImageIndex( const int groundId, const bool allowEmbeddedObjectsAppearOnTerrain );
    }
}

#endif
