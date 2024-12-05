/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2BATTLE_BOARD_H
#define H2BATTLE_BOARD_H

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "battle_cell.h"
#include "math_base.h"

namespace Maps
{
    class Tile;
}

namespace Battle
{
    class Unit;

    inline CellDirection & operator++( CellDirection & d )
    {
        return d = ( CENTER == d ? TOP_LEFT : CellDirection( d << 1 ) );
    }
    inline CellDirection & operator--( CellDirection & d )
    {
        return d = ( TOP_LEFT == d ? CENTER : CellDirection( d >> 1 ) );
    }

    using Indexes = std::vector<int32_t>;

    class Board : public std::vector<Cell>
    {
    public:
        // Width of the battlefield, measured in cells
        static constexpr int widthInCells{ 11 };
        // Height of the battlefield, measured in cells
        static constexpr int heightInCells{ 9 };
        // Total number of cells on the battlefield
        static constexpr int sizeInCells{ widthInCells * heightInCells };

        Board();
        Board( const Board & ) = delete;

        Board & operator=( const Board & ) = delete;

        void removeDeadUnits();

        void SetArea( const fheroes2::Rect & );

        int32_t GetIndexAbsPosition( const fheroes2::Point & ) const;
        std::vector<Unit *> GetNearestTroops( const Unit * startUnit, const std::vector<Unit *> & blackList );

        void SetCobjObjects( const Maps::Tile & tile, std::mt19937 & gen );
        void SetCovrObjects( int icn );

        static std::string GetMoatInfo();

        static Cell * GetCell( const int32_t position, const int dir = CENTER );

        static bool isNearIndexes( const int32_t index1, const int32_t index2 );
        static bool isValidIndex( const int32_t index );
        // Returns true if the given index is considered to be inside the castle from the point of view of castle defense,
        // otherwise returns false. Indexes of destructible walls are considered to be located inside the castle.
        static bool isCastleIndex( const int32_t index );
        static bool isMoatIndex( const int32_t index, const Unit & unit );
        static bool isOutOfWallsIndex( const int32_t index );

        static int GetReflectDirection( const int dir );
        static int GetDirection( const int32_t index1, const int32_t index2 );

        // Returns the distance to the cell with the given index from the given edge of the battlefield along the X axis. The
        // distance from the edges of the battlefield to the cells closest to them is counted as one.
        static uint32_t GetDistanceFromBoardEdgeAlongXAxis( const int32_t index, const bool fromRightEdge );

        // Returns the distance between two cells with the given indexes. If any of the indexes is not valid, then returns 0.
        static uint32_t GetDistance( const int32_t index1, const int32_t index2 );

        // Returns the distance between two given positions. This distance is calculated as the distance between the cells of
        // both positions closest to each other. If any of the positions is not valid, then returns 0.
        static uint32_t GetDistance( const Position & pos1, const Position & pos2 );

        // Returns the distance between the given position and the cell with the given index. The distance is calculated as
        // the distance between the cell with the given index and the cell closest to it, which is part of the given position.
        // If either the position or the index is not valid, then returns 0.
        static uint32_t GetDistance( const Position & pos, const int32_t index );

        static bool isValidDirection( const int32_t index, const int dir );
        static int32_t GetIndexDirection( const int32_t index, const int dir );

        static Indexes GetDistanceIndexes( const int32_t center, const uint32_t radius );
        static Indexes GetDistanceIndexes( const Unit & unit, const uint32_t radius );
        static Indexes GetDistanceIndexes( const Position & pos, const uint32_t radius );

        static Indexes GetAroundIndexes( const int32_t center );
        static Indexes GetAroundIndexes( const Unit & unit );
        static Indexes GetAroundIndexes( const Position & pos );

        static Indexes GetMoveWideIndexes( const int32_t head, const bool reflect );

        static bool isValidMirrorImageIndex( const int32_t index, const Unit * unit );

        // Checks whether a given unit is (in principle) capable of attacking in melee during the current turn from a cell with
        // a given index
        static bool CanAttackFromCell( const Unit & unit, const int32_t from );
        // Checks whether the attacker is able to attack the target in melee during the current turn from a position corresponding
        // to the given index
        static bool CanAttackTargetFromPosition( const Unit & attacker, const Unit & target, const int32_t dst );

    private:
        void SetCobjObject( const int icn, const uint32_t dst );
    };
}

#endif
