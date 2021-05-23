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
#include "math_base.h"

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

    bool isValidAbsIndex( int32_t ii );
    bool isValidAbsPoint( int32_t x, int32_t y );

    fheroes2::Point GetPoint( const int32_t index );

    int32_t GetIndexFromAbsPoint( const fheroes2::Point & mp );
    int32_t GetIndexFromAbsPoint( int32_t px, int32_t py );

    Indexes GetAroundIndexes( int32_t center );
    Indexes GetAroundIndexes( int32_t center, int dist, bool sort = false ); // sorting distance

    Indexes ScanAroundObject( int32_t center, int obj );
    Indexes ScanAroundObject( int32_t center, uint32_t dist, int obj );
    Indexes FindBoatTiles( int32_t center );

    Indexes GetTilesUnderProtection( int32_t center );
    bool TileIsUnderProtection( int32_t center );

    Indexes GetObjectPositions( int obj, bool ignoreHeroes );
    Indexes GetObjectPositions( int32_t center, int obj, bool ignoreHeroes );
    Indexes GetObjectsPositions( const std::vector<uint8_t> & objs );

    int TileIsCoast( int32_t center, int direct = DIRECTION_ALL );

    void ClearFog( int32_t index, int scoute, int color );
    uint32_t GetApproximateDistance( int32_t index1, int32_t index2 );

    void UpdateCastleSprite( const fheroes2::Point & center, int race, bool isCastle = false, bool isRandom = false );
    void MinimizeAreaForCastle( const fheroes2::Point & );
}

#endif
