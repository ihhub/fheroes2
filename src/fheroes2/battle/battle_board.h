/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include <functional>

#include "battle.h"
#include "battle_cell.h"

#define ARENAW 11
#define ARENAH 9
#define ARENASIZE ARENAW * ARENAH

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

    typedef std::vector<s32> Indexes;

    class Board : public std::vector<Cell>
    {
    public:
        Board();

        void Reset( void );

        Rect GetArea( void ) const;
        void SetArea( const Rect & );

        s32 GetIndexAbsPosition( const Point & ) const;
        Indexes GetPassableQualityPositions( const Unit & b );
        Indexes GetNearestTroopIndexes( s32, const Indexes * ) const;
        Indexes GetAStarPath( const Unit & unit, const Position & destination, const bool debug = true ) const;

        void SetEnemyQuality( const Unit & );
        void SetPositionQuality( const Unit & );
        void SetScanPassability( const Unit & );

        void SetCobjObjects( const Maps::Tiles & );
        void SetCobjObject( int icn, s32 );
        void SetCovrObjects( int icn );

        static std::string GetMoatInfo( void );

        static Cell * GetCell( s32, int = CENTER );
        static bool isNearIndexes( s32, s32 );
        static bool isValidIndex( s32 );
        static bool isCastleIndex( s32 );
        static bool isMoatIndex( s32 );
        static bool isBridgeIndex( s32 );
        static bool isImpassableIndex( s32 );
        static bool isOutOfWallsIndex( s32 );
        static bool isReflectDirection( int );
        static bool isNegativeDistance( s32 index1, s32 index2 );
        static int GetReflectDirection( int );
        static int GetDirection( s32, s32 );
        static s32 GetDistance( s32, s32 );
        static bool isValidDirection( s32, int );
        static s32 GetIndexDirection( s32, int );
        static Indexes GetDistanceIndexes( s32, u32 );
        static Indexes GetAroundIndexes( s32 center, s32 ignore = -1 );
        static Indexes GetAroundIndexes( const Unit & );
        static Indexes GetMoveWideIndexes( s32, bool reflect );
        static bool isValidMirrorImageIndex( s32, const Unit * );

        static Indexes GetAdjacentEnemies( const Unit & unit );
    };
}

#endif
