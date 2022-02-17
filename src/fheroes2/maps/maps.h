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

#include <vector>

#include "math_base.h"
#include "mp2.h"
#include "types.h"

#define TILEWIDTH 32

class MapsIndexes : public std::vector<int32_t>
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

    using Indexes = MapsIndexes;

    const char * SizeString( int size );
    const char * GetMinesName( int res );

    int GetDirection( int from, int to );
    int32_t GetDirectionIndex( int32_t from, int vector );
    bool isValidDirection( int32_t from, int vector );

    bool isValidAbsIndex( const int32_t index );
    bool isValidAbsPoint( const int32_t x, const int32_t y );

    fheroes2::Point GetPoint( const int32_t index );

    // Convert maps point to index maps. Returns -1 if x or y is negative.
    int32_t GetIndexFromAbsPoint( const fheroes2::Point & mp );
    int32_t GetIndexFromAbsPoint( const int32_t x, const int32_t y );

    Indexes getAroundIndexes( const int32_t tileIndex, const int32_t maxDistanceFromTile = 1 );

    Indexes ScanAroundObject( const int32_t center, const MP2::MapObjectType objectType );
    Indexes ScanAroundObjectWithDistance( const int32_t center, const uint32_t dist, const MP2::MapObjectType objectType );
    Indexes ScanAroundObject( const int32_t center, const MP2::MapObjectType objectType, const bool ignoreHeroes );
    Indexes GetFreeIndexesAroundTile( const int32_t center );

    // Checks if the tile is guarded by a monster
    bool tileIsUnderProtection( const int32_t tileIndex );
    // Returns a list of indexes of tiles with monsters guarding the given tile
    Indexes getMonstersProtectingTile( const int32_t tileIndex );

    Indexes GetObjectPositions( const MP2::MapObjectType objectType, bool ignoreHeroes );
    Indexes GetObjectPositions( int32_t center, const MP2::MapObjectType objectType, bool ignoreHeroes );

    void ClearFog( const int32_t tileIndex, const int scouteValue, const int playerColor );

    int32_t getFogTileCountToBeRevealed( const int32_t tileIndex, const int scouteValue, const int playerColor );

    // This method should be avoided unless high precision is not important.
    uint32_t GetApproximateDistance( const int32_t pos1, const int32_t pos2 );

    void UpdateCastleSprite( const fheroes2::Point & center, int race, bool isCastle = false, bool isRandom = false );
    void ReplaceRandomCastleObjectId( const fheroes2::Point & );
}

#endif
