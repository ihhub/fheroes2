/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#ifndef H2MAPS_H
#define H2MAPS_H

#include "direction.h"
#include "gamedefs.h"

#define TILEWIDTH 32

struct Point;

class MapsIndexes : public std::vector<s32>
{};

namespace Maps
{
    enum mapsize_t
    {
        ZERO = 0,
        SMALL = 36,
        MEDIUM = 72,
        LARGE = 108,
        XLARGE = 144
    };

    typedef MapsIndexes Indexes;

    const char * SizeString( int size );
    const char * GetMinesName( int res );

    int GetDirection( int from, int to );
    s32 GetDirectionIndex( s32, int direct );
    bool isValidDirection( s32, int direct );

    bool isValidAbsIndex( s32 );
    bool isValidAbsPoint( const Point & pt );
    bool isValidAbsPoint( s32 x, s32 y );

    Point GetPoint( s32 );

    s32 GetIndexFromAbsPoint( const Point & mp );
    s32 GetIndexFromAbsPoint( s32 px, s32 py );

    Indexes GetAroundIndexes( s32 );
    Indexes GetAroundIndexes( s32, int dist, bool sort = false ); // sorting distance
    Indexes GetDistanceIndexes( s32 center, int dist );

    Indexes ScanAroundObject( s32, int obj );
    Indexes ScanAroundObject( s32, u32 dist, int obj );

    Indexes GetTilesUnderProtection( s32 );
    bool TileIsUnderProtection( s32 );
    bool IsNearTiles( s32, s32 );

    Indexes GetObjectPositions( int obj, bool ignoreHeroes );
    Indexes GetObjectPositions( s32, int obj, bool ignoreHeroes );
    Indexes GetObjectsPositions( const std::vector<u8> & objs );

    int TileIsCoast( s32, int direct = DIRECTION_ALL );

    void ClearFog( s32, int scoute, int color );
    u32 GetApproximateDistance( s32, s32 );

    void UpdateCastleSprite( const Point & center, int race, bool isCastle = false, bool isRandom = false );
    void MinimizeAreaForCastle( const Point & );
}

#endif
