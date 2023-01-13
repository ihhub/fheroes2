/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#define ARENAW 11
#define ARENAH 9
#define ARENASIZE ARENAW * ARENAH

namespace Maps
{
    class Tiles;
}

namespace Battle
{
    class Unit;

    inline direction_t & operator++( direction_t & d )
    {
        return d = ( CENTER == d ? TOP_LEFT : direction_t( d << 1 ) );
    }
    inline direction_t & operator--( direction_t & d )
    {
        return d = ( TOP_LEFT == d ? CENTER : direction_t( d >> 1 ) );
    }

    using Indexes = std::vector<int32_t>;

    class Board : public std::vector<Cell>
    {
    public:
        Board();
        Board( const Board & ) = delete;

        Board & operator=( const Board & ) = delete;

        void Reset();

        void SetArea( const fheroes2::Rect & );

        int32_t GetIndexAbsPosition( const fheroes2::Point & ) const;
        std::vector<Unit *> GetNearestTroops( const Unit * startUnit, const std::vector<Unit *> & blackList );

        void SetEnemyQuality( const Unit & ) const;
        void SetPositionQuality( const Unit & ) const;

        void SetCobjObjects( const Maps::Tiles & tile, std::mt19937 & gen );
        void SetCovrObjects( int icn );

        static std::string GetMoatInfo();

        static Cell * GetCell( int32_t position, int dir = CENTER );
        static bool isNearIndexes( int32_t, int32_t );
        static bool isValidIndex( int32_t );
        static bool isCastleIndex( int32_t );
        static bool isMoatIndex( int32_t index, const Unit & b );
        static bool isBridgeIndex( int32_t index, const Unit & b );
        static bool isOutOfWallsIndex( int32_t );
        static bool IsLeftDirection( const int32_t startCellId, const int32_t endCellId, const bool prevLeftDirection );
        static bool isNegativeDistance( int32_t index1, int32_t index2 );
        static int DistanceFromOriginX( int32_t index, bool reflect );
        static int GetReflectDirection( int );
        static int GetDirection( int32_t, int32_t );
        static int32_t DoubleCellAttackValue( const Unit & attacker, const Unit & target, const int32_t from, const int32_t targetCell );
        static int32_t OptimalAttackTarget( const Unit & attacker, const Unit & target, const int32_t from );
        static int32_t OptimalAttackValue( const Unit & attacker, const Unit & target, const int32_t from );
        static uint32_t GetDistance( int32_t, int32_t );
        static bool isValidDirection( int32_t, int );
        static int32_t GetIndexDirection( int32_t, int );
        static Indexes GetDistanceIndexes( int32_t, uint32_t );
        static Indexes GetAroundIndexes( int32_t center, int32_t ignore = -1 );
        static Indexes GetAroundIndexes( const Unit & unit );
        static Indexes GetAroundIndexes( const Position & position );
        static Indexes GetMoveWideIndexes( int32_t, bool reflect );
        static bool isValidMirrorImageIndex( const int32_t index, const Unit * unit );

        // Checks that the current unit (to which the current pathfinder graph relates) is able (in principle)
        // to attack during the current turn from the cell with the given index
        static bool CanAttackFromCell( const Unit & currentUnit, const int32_t from );
        // Checks that the current unit (to which the current pathfinder graph relates) is able to attack the
        // target during the current turn from the position which corresponds to the given index
        static bool CanAttackTargetFromPosition( const Unit & currentUnit, const Unit & target, const int32_t dst );

        static Indexes GetAdjacentEnemies( const Unit & unit );

    private:
        void SetCobjObject( const int icn, const int32_t dst );
    };
}

#endif
