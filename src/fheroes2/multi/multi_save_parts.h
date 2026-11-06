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
#include <map>
#include <string>
#include <vector>

namespace Multi
{
    // Save files cut into the units of the world, so each unit can be taken from one copy of the game or the other

    // Header as it is. Data uncompressed, with the world, then the settings, the game result and the end marker.
    struct SaveFileData
    {
        std::vector<uint8_t> header;
        std::vector<uint8_t> data;
    };

    // Current save format only. Returns an error message or an empty string.
    std::string readSaveFileData( const std::vector<uint8_t> & fileBytes, SaveFileData & save );

    std::vector<uint8_t> writeSaveFileData( const SaveFileData & save );

    // Sets the current save version while it exists, the readers of the game units depend on it
    class SaveVersionGuard
    {
    public:
        SaveVersionGuard();
        SaveVersionGuard( const SaveVersionGuard & ) = delete;
        SaveVersionGuard & operator=( const SaveVersionGuard & ) = delete;
        ~SaveVersionGuard();

    private:
        const uint16_t _previous;
    };

    struct ByteRange
    {
        size_t begin{ 0 };
        size_t end{ 0 };
    };

    // Position of each unit of the world in the game data. List ranges do not include the count of the list.
    struct WorldParts
    {
        // Map width and height
        ByteRange size;
        // By tile index
        std::vector<ByteRange> tiles;
        // By hero ID
        std::vector<ByteRange> heroes;
        std::vector<ByteRange> castles;
        // By kingdom index
        std::vector<ByteRange> kingdoms;
        std::vector<ByteRange> rumors;
        std::vector<ByteRange> events;
        // With the tile index of the object
        std::vector<ByteRange> capturedObjects;
        ByteRange ultimateArtifact;
        // Day, week, month and the heroes of the win and loss condtions
        ByteRange dateAndConditions;
        // With the UID and the type of the object
        std::vector<ByteRange> mapObjects;
        ByteRange seed;
        // Then the settings and the game result
        size_t worldEnd{ 0 };
    };

    // The game must be loaded on the same map. Kingdoms find their castles and heroes through the world. Returns an error
    // message or an empty string.
    std::string splitWorld( const std::vector<uint8_t> & data, WorldParts & parts );

    // Same bytes if nothing was changed
    std::vector<uint8_t> assembleWorld( const std::vector<uint8_t> & data, const WorldParts & parts );

    // By position in their list. Captured objects by tile index, as the world keeps them sorted (a new one is inserted
    // at its place).
    struct UnitReplacements
    {
        std::map<size_t, std::vector<uint8_t>> tiles;
        std::map<size_t, std::vector<uint8_t>> heroes;
        std::map<size_t, std::vector<uint8_t>> castles;
        std::map<size_t, std::vector<uint8_t>> kingdoms;
        std::map<int32_t, std::vector<uint8_t>> capturedObjects;

        // By UID, sorted as the world writes them. Empty bytes remove the object.
        std::map<uint32_t, std::vector<uint8_t>> mapObjects;
    };

    // Returns empty data if a replced unit does not exist
    std::vector<uint8_t> assembleWorld( const std::vector<uint8_t> & data, const WorldParts & parts, const UnitReplacements & replacements );
}
