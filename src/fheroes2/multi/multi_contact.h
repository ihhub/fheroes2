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

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "color.h"
#include "math_base.h"

namespace Multi
{
    // Tiles of the current map. Invalid indexes (-1 too) are never added.
    class TileSet
    {
    public:
        TileSet() = default;
        explicit TileSet( const int32_t tileCount );

        bool contains( const int32_t index ) const;
        void insert( const int32_t index );
        void insertAll( const TileSet & other );

        // Returns -1 if there is none
        int32_t firstCommonTile( const TileSet & other ) const;

        // Grown by one tile in all 8 directions, within the map borders
        TileSet dilated() const;

        size_t count() const;

        const std::vector<uint8_t> & bits() const
        {
            return _bits;
        }

    private:
        std::vector<uint8_t> _bits;
        int32_t _tileCount{ 0 };
    };

    // What the heroes of one human player can reach today, estimated at the start of the day
    struct PlayerReach
    {
        PlayerColor color{ PlayerColor::NONE };

        // Tiles a hero of this color can stand on or act on today
        TileSet stand;

        // Stand tiles plus the assets of this color and of the computer players it can reach, grown by one tile
        TileSet touch;

        uint32_t rounds{ 0 };
        bool isRoundLimitReached{ false };

        uint32_t heroSources{ 0 };
        uint32_t hireSources{ 0 };
        uint32_t jailSources{ 0 };
        uint32_t castleUnlocks{ 0 };
        uint32_t movementBonuses{ 0 };
    };

    enum class ContactResult : uint8_t
    {
        SIMULTANEOUS,
        CONTACT,
        ROUND_LIMIT,
        NO_PLAYER
    };

    struct ContactVerdict
    {
        ContactResult result{ ContactResult::NO_PLAYER };

        // A tile both sides can touch, and the two players
        int32_t witnessTile{ -1 };
        std::array<PlayerColor, 2> witnessColors{ PlayerColor::NONE, PlayerColor::NONE };

        // One per human player still in the game
        std::vector<PlayerReach> reaches;

        // Allied colors whose touch sets overlap
        PlayerColorsSet alliedOverlaps{ 0 };

        uint32_t milliseconds{ 0 };
    };

    // Checks whether the human players of two different computers can meet or touch the same things today. One set of
    // colors per computer. No change to the game, no random numbers.
    ContactVerdict computeContactVerdict( const std::vector<PlayerColorsSet> & computerColors );

    const char * contactResultName( const ContactResult result );

    // Adds the tiles of a castle, its shadow on the left and where its shipyard puts a boat. They change when it is built or captured.
    void insertCastleFootprint( TileSet & tiles, const fheroes2::Point & center );
}
