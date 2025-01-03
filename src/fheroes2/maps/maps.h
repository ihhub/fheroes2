/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "math_base.h"

class Heroes;

namespace MP2
{
    enum MapObjectType : uint16_t;
}

using MapsIndexes = std::vector<int32_t>;

namespace Maps
{
    struct ObjectPart;

    enum MapSize : int
    {
        ZERO = 0,
        SMALL = 36,
        MEDIUM = 72,
        LARGE = 108,
        XLARGE = 144
    };

    using Indexes = MapsIndexes;

    const char * SizeString( int size );
    const char * GetMineName( const int resourceType );

    int GetDirection( int from, int to );
    int32_t GetDirectionIndex( const int32_t from, const int direction );
    // Returns the nearest point on the map to the current tile, located in the given direction vector.
    fheroes2::Point getDirectionPoint( const fheroes2::Point & from, const int direction );
    bool isValidDirection( int32_t from, int vector );

    bool isValidAbsIndex( const int32_t index );
    bool isValidAbsPoint( const int32_t x, const int32_t y );

    fheroes2::Point GetPoint( const int32_t index );

    // Convert maps point to index maps. Returns -1 if x or y is negative.
    int32_t GetIndexFromAbsPoint( const fheroes2::Point & mp );
    int32_t GetIndexFromAbsPoint( const int32_t x, const int32_t y );

    Indexes getAroundIndexes( const int32_t tileIndex, const int32_t maxDistanceFromTile = 1 );

    MapsIndexes getVisibleMonstersAroundHero( const Heroes & hero );

    Indexes ScanAroundObject( const int32_t center, const MP2::MapObjectType objectType );
    Indexes ScanAroundObjectWithDistance( const int32_t center, const uint32_t dist, const MP2::MapObjectType objectType );
    Indexes ScanAroundObject( const int32_t center, const MP2::MapObjectType objectType, const bool ignoreHeroes );

    bool isValidForDimensionDoor( int32_t targetIndex, bool isWater );
    // Checks if the tile is guarded by a monster
    bool isTileUnderProtection( const int32_t tileIndex );
    // Returns a list of indexes of tiles with monsters guarding the specified tile. If the 'checkObjectOnTile' parameter
    // is set to true, then an additional check is performed to see if it is possible to interact with an object on this
    // tile without triggering a monster attack.
    Indexes getMonstersProtectingTile( const int32_t tileIndex, const bool checkObjectOnTile = true );

    // This function always ignores heroes.
    bool doesObjectExistOnMap( const MP2::MapObjectType objectType );

    // This function always ignores heroes.
    Indexes GetObjectPositions( const MP2::MapObjectType objectType );

    // This is a very slow function by performance. Use it only while loading a map.
    std::vector<std::pair<int32_t, const ObjectPart *>> getObjectParts( const MP2::MapObjectType objectType );

    Indexes GetObjectPositions( int32_t center, const MP2::MapObjectType objectType, bool ignoreHeroes );

    void ClearFog( const int32_t tileIndex, const int scoutingDistance, const int playerColor );
    int32_t getFogTileCountToBeRevealed( const int32_t tileIndex, const int scoutingDistance, const int playerColor );

    // Returns the approximate distance between two tiles with given indexes. This distance is calculated as the number of
    // tiles (truncated to the nearest smaller integer value) that would need to be traversed in a straight direction to
    // overcome the distance that corresponds to the distance that the hero would have to cover when moving between tiles
    // with given indexes using the usual rules and penalties.
    //
    // For example, in order to navigate to a tile B, which is located seven tiles below and five to the right of tile A,
    // the hero should move two tiles down and five tiles right down diagonally (diagonal movement costs 50% more), which
    // corresponds to 2 + 5 * 1.5 = 9.5 (9 after truncation) tiles traversed in the straight direction.
    //
    // This function should be avoided unless high precision is not important.
    uint32_t GetApproximateDistance( const int32_t pos1, const int32_t pos2 );

    // Returns the straight line distance between two tiles with given indexes. This distance is calculated as the number
    // of tiles (truncated to the nearest smaller integer value) that would need to be traversed in a straight direction
    // to overcome the distance that corresponds to the straight line distance between the tiles with given indexes.
    //
    // For example, in order to navigate to a tile B, which is located seven tiles below and five to the right of tile A,
    // a straight line distance should be covered, which corresponds to sqrt( 5^2 + 7^2 ) = 8.6 (8 after truncation) tiles
    // traversed in the straight direction.
    //
    // This function is not suitable for accurate calculation of the distance expressed in movement points.
    uint32_t GetStraightLineDistance( const int32_t pos1, const int32_t pos2 );

    void UpdateCastleSprite( const fheroes2::Point & center, int race, bool isCastle = false, bool isRandom = false );
    void ReplaceRandomCastleObjectId( const fheroes2::Point & );
}

#endif
