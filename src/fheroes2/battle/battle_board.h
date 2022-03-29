/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2019 - 2022                                             *
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

#include <random>

#include "battle_cell.h"

#define ARENAW 11
#define ARENAH 9
#define ARENASIZE ARENAW * ARENAH

namespace Maps
{
    class Tiles;
}

namespace Battle
{
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

        void Reset( void );

        void SetArea( const fheroes2::Rect & );

        s32 GetIndexAbsPosition( const fheroes2::Point & ) const;
        std::vector<Unit *> GetNearestTroops( const Unit * startUnit, const std::vector<Unit *> & blackList );
        Indexes GetPath( const Unit & unit, const Position & destination, const bool debug = true ) const;

        void SetEnemyQuality( const Unit & ) const;
        void SetPositionQuality( const Unit & ) const;
        void SetScanPassability( const Unit & );

        void SetCobjObjects( const Maps::Tiles & tile, std::mt19937 & gen );
        void SetCovrObjects( int icn );

        static std::string GetMoatInfo( void );

        static Cell * GetCell( s32 position, int dir = CENTER );
        static bool isNearIndexes( s32, s32 );
        static bool isValidIndex( s32 );
        static bool isCastleIndex( s32 );
        static bool isMoatIndex( s32 index, const Unit & b );
        static bool isBridgeIndex( s32 index, const Unit & b );
        static bool isOutOfWallsIndex( s32 );
        static bool IsLeftDirection( const int32_t startCellId, const int32_t endCellId, const bool prevLeftDirection );
        static bool isNegativeDistance( s32 index1, s32 index2 );
        static int DistanceFromOriginX( int32_t index, bool reflect );
        static int GetReflectDirection( int );
        static int GetDirection( s32, s32 );
        static int32_t DoubleCellAttackValue( const Unit & attacker, const Unit & target, const int32_t from, const int32_t targetCell );
        static int32_t OptimalAttackTarget( const Unit & attacker, const Unit & target, const int32_t from );
        static int32_t OptimalAttackValue( const Unit & attacker, const Unit & target, const int32_t from );
        static uint32_t GetDistance( s32, s32 );
        static bool isValidDirection( s32, int );
        static s32 GetIndexDirection( s32, int );
        static Indexes GetDistanceIndexes( s32, u32 );
        static Indexes GetAroundIndexes( s32 center, s32 ignore = -1 );
        static Indexes GetAroundIndexes( const Unit & unit );
        static Indexes GetAroundIndexes( const Position & position );
        static Indexes GetMoveWideIndexes( s32, bool reflect );
        static bool isValidMirrorImageIndex( s32, const Unit * );

        // Checks that the current unit (to which the current passability information relates) is able (in principle)
        // to attack from the cell with the given index
        static bool CanAttackUnitFromCell( const Unit & currentUnit, const int32_t from );
        // Checks that the current unit (to which the current passability information relates) is able to attack the
        // target from the position which corresponds to the given index
        static bool CanAttackUnitFromPosition( const Unit & currentUnit, const Unit & target, const int32_t dst );

        static Indexes GetAdjacentEnemies( const Unit & unit );

    private:
        void SetCobjObject( const int icn, const int32_t dst );

        bool GetPathForUnit( const Unit & unit, const Position & destination, const uint32_t remainingSteps, const int32_t currentCellId,
                             std::vector<bool> & visitedCells, Indexes & result ) const;
        bool GetPathForWideUnit( const Unit & unit, const Position & destination, const uint32_t remainingSteps, const int32_t currentHeadCellId,
                                 const int32_t prevHeadCellId, std::vector<bool> & visitedCells, Indexes & result ) const;
        void StraightenPathForUnit( const int32_t currentCellId, Indexes & path ) const;
    };
}

#endif
