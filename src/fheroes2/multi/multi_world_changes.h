/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "color.h"
#include "multi_save_parts.h"

namespace Multi
{
    // The world cut unit by unit, to find out what an action has changed
    class WorldSnapshot
    {
    public:
        // Returns an error message or an empty string
        std::string capture();

        // A world written by this version of the game on the current map. Returns an error message or an empty string.
        std::string assign( std::vector<uint8_t> data );

        bool isValid() const
        {
            return !_data.empty();
        }

        const std::vector<uint8_t> & data() const
        {
            return _data;
        }

        const WorldParts & parts() const
        {
            return _parts;
        }

    private:
        std::vector<uint8_t> _data;
        WorldParts _parts;
    };

    struct WorldChanges
    {
        // Not counting the fog of war
        std::vector<int32_t> tiles;

        // Tiles where only the fog has changed
        std::vector<int32_t> fogTiles;
        PlayerColorsSet fogColors{ 0 };

        // By hero ID, castle index and kingdom index
        std::vector<int32_t> heroes;
        std::vector<int32_t> castles;
        std::vector<int32_t> kingdoms;

        // Added, removed or changed, by tile index
        std::vector<int32_t> capturedObjects;

        // Events, sphinxes and signs added, removed or changed, by UID
        std::vector<uint32_t> mapObjects;

        bool isRumorsChanged{ false };
        bool isEventsChanged{ false };
        bool isUltimateArtifactChanged{ false };
        bool isDateChanged{ false };
        bool isSeedChanged{ false };

        // The two snapshots do not even have the same units
        bool isLayoutChanged{ false };

        bool empty() const;
    };

    WorldChanges compareSnapshots( const WorldSnapshot & before, const WorldSnapshot & after );

    // Funds, heroes, castles, size and the first different byte
    std::string describeKingdomDifference( const WorldSnapshot & first, const WorldSnapshot & second, const int32_t kingdomIndex );

    // Offset of the fog of war byte in the bytes of a tile, if found
    const std::optional<size_t> & getTileFogOffset();
}
