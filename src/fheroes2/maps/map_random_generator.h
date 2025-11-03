/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
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

namespace Maps::Map_Format
{
    struct MapFormat;
}

namespace Maps::Random_Generator
{
    enum class Layout : uint8_t
    {
        MIRRORED, // Multi-player map with equal distances and distribution of resources
        BALANCED, // Standard option; some players might have an advantaged position
        ISLANDS, // Water-based map that requires castles on the coast to make it navigable
        PYRAMID, // Map with AI-only players that get resource advantages
        QUEST // Single-player only map with non-standard winning condition
    };

    enum class ResourceDensity : uint8_t
    {
        SCARCE, // 2-tier resources (Crystals, Gems, etc) are harder to come by
        NORMAL, // 2-tier resources are common, gold mines are rare
        ABUNDANT, // Even neutral regions have mines, rare treasures are common

        // Put all new entries above this line.
        ITEM_COUNT
    };

    enum class MonsterStrength : uint8_t
    {
        WEAK, // Fairly open map, some mines might be unguarded
        NORMAL, // Most monsters are MON2, mines and treasures are protected
        STRONG, // Higher tier monsters on average, even wood and ore are protected
        DEADLY // Neutral monster stacks are significantly larger, pre-determined Tier 6 used
    };

    struct Configuration final
    {
        int32_t playerCount{ 2 };
        int32_t waterPercentage{ 0 };
        int32_t seed{ 0 };

        Layout mapLayout{ Layout::MIRRORED };
        ResourceDensity resourceDensity{ ResourceDensity::NORMAL };
        MonsterStrength monsterStrength{ MonsterStrength::NORMAL };
    };

    bool generateMap( Map_Format::MapFormat & mapFormat, const Configuration & config, const int32_t width, const int32_t height );
}
