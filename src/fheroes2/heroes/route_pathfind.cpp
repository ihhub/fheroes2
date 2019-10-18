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

#include <map>
#include "maps.h"
#include "ai.h"
#include "world.h"
#include "direction.h"
#include "settings.h"
#include "heroes.h"
#include "ground.h"
#include "route.h"

struct cell_t
{
    cell_t() : cost_g(MAXU16), cost_t(MAXU16), cost_d(MAXU16), passbl(0), open(1), direct(Direction::CENTER), parent(-1) {}

    u16		cost_g;	// ground
    u16		cost_t; // total
    u16		cost_d; // distance
    u16		passbl;
    u16		open;	// bool
    u16		direct;
    s32		parent;
};

int GetCurrentLength(std::map<s32, cell_t> & list, s32 from)
{
    int res = 0;
    while(0 <= list[from].parent)
    {
	from = list[from].parent;
	++res;
    }
    return res;
}

bool CheckMonsterProtectionAndNotDst(const s32 & to, const s32 & dst)
{
    const MapsIndexes & monsters = Maps::GetTilesUnderProtection(to);
    return monsters.size() && monsters.end() == std::find(monsters.begin(), monsters.end(), dst);
}

bool PassableToTile(const Heroes & hero, const Maps::Tiles & toTile, int direct, s32 dst)
{
    // check end point
    if(toTile.GetIndex() == dst)
    {
	// fix toTilePassable with action object
	if(MP2::isPickupObject(toTile.GetObject()))
	    return true;

	// check direct to object
	if(MP2::isActionObject(toTile.GetObject(false), hero.isShipMaster()))
	    return Direction::Reflect(direct) & toTile.GetPassable();

	if(MP2::OBJ_HEROES == toTile.GetObject())
	    return toTile.isPassable(NULL, Direction::Reflect(direct), (hero.isControlAI() ? AI::HeroesSkipFog() : false));
    }

    // check to tile direct
    if(! toTile.isPassable(&hero, Direction::Reflect(direct), (hero.isControlAI() ? AI::HeroesSkipFog() : false)))
	return false;

    if(toTile.GetIndex() != dst)
    {
	if(MP2::isPickupObject(toTile.GetObject()) ||
	    MP2::isActionObject(toTile.GetObject(false), hero.isShipMaster()))
	    return false;

	// check hero/monster on route
	switch(toTile.GetObject())
	{
	    case MP2::OBJ_HEROES:
	    case MP2::OBJ_MONSTER:
		return false;

	    default: break;
	}

	// check monster protection
	if(CheckMonsterProtectionAndNotDst(toTile.GetIndex(), dst))
	    return false;
    }

    return true;
}

bool PassableFromToTile(const Heroes & hero, s32 from, const s32 & to, int direct, s32 dst)
{
    const Maps::Tiles & fromTile = world.GetTiles(from);
    const Maps::Tiles & toTile = world.GetTiles(to);

    // check start point
    if(hero.GetIndex() == from)
    {
	if(MP2::isActionObject(fromTile.GetObject(false), hero.isShipMaster()))
	{
	    // check direct from object
	    if(! (direct & fromTile.GetPassable()))
		return false;
	}
	else
	{
	    // check from tile direct
	    if(! fromTile.isPassable(&hero, direct, (hero.isControlAI() ? AI::HeroesSkipFog() : false)))
		return false;
	}
    }
    else
    {
	if(MP2::isActionObject(fromTile.GetObject(), hero.isShipMaster()))
	{
	    // check direct from object
	    if(! (direct & fromTile.GetPassable()))
		return false;
	}
	else
	{
	    // check from tile direct
	    if(! fromTile.isPassable(&hero, direct, (hero.isControlAI() ? AI::HeroesSkipFog() : false)))
		return false;
	}
    }

    if(fromTile.isWater() && !toTile.isWater())
    {
	switch(toTile.GetObject())
	{
	    case MP2::OBJ_BOAT:
            case MP2::OBJ_MONSTER:
            case MP2::OBJ_HEROES:
                return false;

	    case MP2::OBJ_COAST:
		return toTile.GetIndex() == dst;

	    default: break;
	}
    }
    else
    if(!fromTile.isWater() && toTile.isWater())
    {
	switch(toTile.GetObject())
	{
	    case MP2::OBJ_BOAT:
                return true;

            case MP2::OBJ_HEROES:
		return toTile.GetIndex() == dst;

	    default: break;
	}
    }

    // check corner water/coast
    if(hero.isShipMaster() && 
	(direct & (Direction::TOP_LEFT | Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM_LEFT)))
    {
	switch(direct)
	{
	    case Direction::TOP_LEFT:
		if((Maps::isValidDirection(from, Direction::TOP) &&
		    ! world.GetTiles(Maps::GetDirectionIndex(from, Direction::TOP)).isWater()) ||
		   (Maps::isValidDirection(from, Direction::LEFT) &&
		    ! world.GetTiles(Maps::GetDirectionIndex(from, Direction::LEFT)).isWater()))
		    return false;
		break;

	    case Direction::TOP_RIGHT:
		if((Maps::isValidDirection(from, Direction::TOP) &&
		    ! world.GetTiles(Maps::GetDirectionIndex(from, Direction::TOP)).isWater()) ||
		   (Maps::isValidDirection(from, Direction::RIGHT) &&
		    ! world.GetTiles(Maps::GetDirectionIndex(from, Direction::RIGHT)).isWater()))
		    return false;
		break;

	    case Direction::BOTTOM_RIGHT:
		if((Maps::isValidDirection(from, Direction::BOTTOM) &&
		    ! world.GetTiles(Maps::GetDirectionIndex(from, Direction::BOTTOM)).isWater()) ||
		   (Maps::isValidDirection(from, Direction::RIGHT) &&
		    ! world.GetTiles(Maps::GetDirectionIndex(from, Direction::RIGHT)).isWater()))
		    return false;
		break;

	    case Direction::BOTTOM_LEFT:
		if((Maps::isValidDirection(from, Direction::BOTTOM) &&
		    ! world.GetTiles(Maps::GetDirectionIndex(from, Direction::BOTTOM)).isWater()) ||
		   (Maps::isValidDirection(from, Direction::LEFT) &&
		    ! world.GetTiles(Maps::GetDirectionIndex(from, Direction::LEFT)).isWater()))
		    return false;
		break;

	    default: break;
	}
    }

    return PassableToTile(hero, toTile, direct, dst);
}

u32 GetPenaltyFromTo(s32 from, s32 to, int direct, int pathfinding)
{
    const u32 cost1 = Maps::Ground::GetPenalty(from, direct, pathfinding); // penalty: for [cur] out
    const u32 cost2 = Maps::Ground::GetPenalty(to, Direction::Reflect(direct), pathfinding); // penalty: for [tmp] in
    return (cost1 + cost2) >> 1;
}

bool Route::Path::Find(s32 to, int limit)
{
    const int pathfinding = hero->GetLevelSkill(Skill::Secondary::PATHFINDING);
    const s32 from = hero->GetIndex();

    s32 cur = from;
    s32 alt = 0;
    s32 tmp = 0;
    std::map<s32, cell_t> list;
    std::map<s32, cell_t>::iterator it1 = list.begin();
    std::map<s32, cell_t>::iterator it2 = list.end();

    list[cur].cost_g = 0;
    list[cur].cost_t = 0;
    list[cur].parent = -1;
    list[cur].open   = 0;

    const Directions directions = Direction::All();
    clear();

    while(cur != to)
    {
	LocalEvent::Get().HandleEvents(false);
    	for(Directions::const_iterator
	    it = directions.begin(); it != directions.end(); ++it)
	{
    	    if(Maps::isValidDirection(cur, *it))
	    {
		tmp = Maps::GetDirectionIndex(cur, *it);

		if(list[tmp].open)
		{
		    const u32 costg = GetPenaltyFromTo(cur, tmp, *it, pathfinding);

		    // new
		    if(-1 == list[tmp].parent)
		    {
			if((list[cur].passbl & *it) ||
			   PassableFromToTile(*hero, cur, tmp, *it, to))
			{
			    list[cur].passbl |= *it;

			    list[tmp].direct = *it;
	    		    list[tmp].cost_g = costg;
			    list[tmp].parent = cur;
			    list[tmp].open   = 1;
			    list[tmp].cost_d = 50 * Maps::GetApproximateDistance(tmp, to);
	    		    list[tmp].cost_t = list[cur].cost_t + costg;
			}
		    }
		    // check alt
		    else
		    {
			if(list[tmp].cost_t > list[cur].cost_t + costg &&
			   ((list[cur].passbl & *it) || PassableFromToTile(*hero, cur, tmp, *it, to)))
			{
			    list[cur].passbl |= *it;

			    list[tmp].direct = *it;
			    list[tmp].parent = cur;
			    list[tmp].cost_g = costg;
			    list[tmp].cost_t = list[cur].cost_t + costg;
			}
		    }
    		}
	    }
	}

	list[cur].open = 0;

	it1 = list.begin();
	alt = -1;
	tmp = MAXU16;

	DEBUG(DBG_OTHER, DBG_TRACE, "route, from: " << cur);

	// find minimal cost
	for(; it1 != it2; ++it1) if((*it1).second.open)
	{
	    const cell_t & cell2 = (*it1).second;
#ifdef WITH_DEBUG
	    if(IS_DEBUG(DBG_OTHER, DBG_TRACE) && cell2.cost_g != MAXU16)
	    {
		int direct = Direction::Get(cur, (*it1).first);

		if(Direction::UNKNOWN != direct)
		{
		    VERBOSE("\t\tdirect: " << Direction::String(direct) <<
			    ", index: " << (*it1).first <<
			    ", cost g: " << cell2.cost_g <<
			    ", cost t: " << cell2.cost_t <<
			    ", cost d: " << cell2.cost_d);
		}
	    }
#endif

	    if(cell2.cost_t + cell2.cost_d < tmp)
	    {
    		tmp = cell2.cost_t + cell2.cost_d;
    		alt = (*it1).first;
	    }
	}

	// not found, and exception
	if(MAXU16 == tmp || -1 == alt) break;
#ifdef WITH_DEBUG
	else
	DEBUG(DBG_OTHER, DBG_TRACE, "select: " << alt);
#endif
	cur = alt;

	if(0 < limit && GetCurrentLength(list, cur) > limit) break;
    }

    // save path
    if(cur == to)
    {
	while(cur != from)
	{
	    push_front(Route::Step(list[cur].parent, list[cur].direct, list[cur].cost_g));
    	    cur = list[cur].parent;
	}
    }
    else
    {
	DEBUG(DBG_OTHER, DBG_TRACE, "not found" << ", from:" << from << ", to: " << to);
    }

    return !empty();
}
