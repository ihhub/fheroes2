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

#include "gamedefs.h"
#include "direction.h"

#define TILEWIDTH 32

struct Point;
class Castle;
class IndexDistance;

class MapsIndexes : public std::vector<s32>
{
};


namespace Maps
{
    enum mapsize_t
    {
	ZERO	= 0,
        SMALL	= 36,
        MEDIUM	= 72,
        LARGE	= 108,
        XLARGE	= 144
    };

    typedef MapsIndexes Indexes;

    class IndexesDistance : public std::vector<IndexDistance>
    {
	void Assign(s32, const Indexes &, int sort /* 0: none, 1: short first, 2: long first */);

    public:
	IndexesDistance(s32, const Indexes &, int sort = 0);
	IndexesDistance(s32, s32, u32 dist, int sort = 0);
    };

    const char* SizeString(int size);
    const char* GetMinesName(int res);

    s32 GetDirectionIndex(s32, int direct);
    bool isValidDirection(s32, int direct);

    bool isValidAbsIndex(s32);
    bool isValidAbsPoint(const Point & pt);
    bool isValidAbsPoint(s32 x, s32 y);

    Point GetPoint(s32);

    s32 GetIndexFromAbsPoint(const Point & mp);
    s32 GetIndexFromAbsPoint(s32 px, s32 py);

    Indexes GetAllIndexes(void);
    Indexes GetAroundIndexes(s32);
    Indexes GetAroundIndexes(s32, int dist, bool sort = false); // sorting distance
    Indexes GetDistanceIndexes(s32 center, int dist);

    Indexes ScanAroundObject(s32, int obj);
    Indexes ScanAroundObjects(s32, const u8* objs);
    Indexes ScanAroundObject(s32, u32 dist, int obj);
    Indexes ScanAroundObjects(s32, u32 dist, const u8* objs);

    Indexes	GetTilesUnderProtection(s32);
    bool	TileIsUnderProtection(s32);
    bool	IsNearTiles(s32, s32);

    Indexes GetObjectPositions(int obj, bool check_hero);
    Indexes GetObjectPositions(s32, int obj, bool check_hero);
    Indexes GetObjectsPositions(const u8* objs);

    int TileIsCoast(s32, int direct = DIRECTION_ALL);

    void ClearFog(s32, int scoute, int color);
    u32 GetApproximateDistance(s32, s32);

    void UpdateRNDSpriteForCastle(const Point &, int race, bool castle);
    void UpdateSpritesFromTownToCastle(const Point &);
    void MinimizeAreaForCastle(const Point &);
}

#endif
