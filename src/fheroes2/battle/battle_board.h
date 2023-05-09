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

        static Cell * GetCell( const int32_t position, const int dir = CENTER );
        static bool isNearIndexes( const int32_t index1, const int32_t index2 );
        static bool isValidIndex( const int32_t index );
        static bool isCastleIndex( const int32_t index );
        static bool isMoatIndex( const int32_t index, const Unit & unit );
        static bool isOutOfWallsIndex( const int32_t index );
        static bool isNegativeDistance( const int32_t index1, const int32_t index2 );
        static int DistanceFromOriginX( const int32_t index, const bool reflect );
        static int GetReflectDirection( const int dir );
        static int GetDirection( const int32_t index1, const int32_t index2 );
        static int32_t DoubleCellAttackValue( const Unit & attacker, const Unit & target, const int32_t from, const int32_t targetCell );
        static int32_t OptimalAttackTarget( const Unit & attacker, const Unit & target, const int32_t from );
        static int32_t OptimalAttackValue( const Unit & attacker, const Unit & target, const int32_t from );
        static uint32_t GetDistance( int32_t, int32_t );
        static bool isValidDirection( const int32_t index, const int dir );
        static int32_t GetIndexDirection( const int32_t index, const int dir );
        static Indexes GetDistanceIndexes( const int32_t center, const uint32_t radius );
        static Indexes GetAroundIndexes( const int32_t center );
        static Indexes GetAroundIndexes( const Unit & unit );
        static Indexes GetAroundIndexes( const Position & position );
        static Indexes GetMoveWideIndexes( const int32_t head, const bool reflect );
        static bool isValidMirrorImageIndex( const int32_t index, const Unit * unit );

        // Checks whether a given unit is (in principle) capable of attacking during the current turn from a cell with a given index
        static bool CanAttackFromCell( const Unit & unit, const int32_t from );
        // Checks whether this attacker is able to attack the target during the current turn from a position corresponding to a given index
        static bool CanAttackTargetFromPosition( const Unit & attacker, const Unit & target, const int32_t dst );

        static Indexes GetAdjacentEnemies( const Unit & unit );

    private:
        void SetCobjObject( const int icn, const uint32_t dst );
    };
}

#endif
