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

#include "ai.h"
#include "direction.h"
#include "ground.h"
#include "heroes.h"
#include "maps.h"
#include "route.h"
#include "settings.h"
#include "world.h"

struct cell_t
{
    cell_t()
        : cost_g( MAXU16 )
        , cost_t( MAXU16 )
        , cost_d( MAXU16 )
        , passbl( 0 )
        , open( 1 )
        , direct( Direction::CENTER )
        , parent( -1 )
    {}

    u16 cost_g; // ground
    u16 cost_t; // total
    u16 cost_d; // distance
    u16 passbl;
    u16 open; // bool
    u16 direct;
    s32 parent;
};

int GetCurrentLength( std::map<s32, cell_t> & list, s32 from )
{
    int res = 0;
    while ( 0 <= list[from].parent ) {
        from = list[from].parent;
        ++res;
    }
    return res;
}

bool CheckMonsterProtectionAndNotDst( const s32 & to, const s32 & dst )
{
    const MapsIndexes & monsters = Maps::GetTilesUnderProtection( to );
    return monsters.size() && monsters.end() == std::find( monsters.begin(), monsters.end(), dst );
}

bool PassableToTile( const Maps::Tiles & toTile, int direct, s32 dst, bool fromWater )
{
    const int object = toTile.GetObject();

    // check end point
    if ( toTile.GetIndex() == dst ) {
        // fix toTilePassable with action object
        if ( MP2::isPickupObject( object ) )
            return true;

        // check direct to object
        if ( MP2::isActionObject( toTile.GetObject( false ), fromWater ) )
            return direct & toTile.GetPassable();

        if ( MP2::OBJ_HEROES == object )
            return toTile.isPassable( direct, fromWater, false );
    }

    // check to tile direct
    if ( !toTile.isPassable( direct, fromWater, false ) )
        return false;

    if ( toTile.GetIndex() != dst ) {
        if ( MP2::isPickupObject( object ) || MP2::isActionObject( object, fromWater ) )
            return false;

        // check hero/monster on route
        switch ( object ) {
        case MP2::OBJ_HEROES:
        case MP2::OBJ_MONSTER:
            return false;

        default:
            break;
        }

        // check monster protection
        if ( CheckMonsterProtectionAndNotDst( toTile.GetIndex(), dst ) )
            return false;
    }

    return true;
}

bool PassableFromToTile( s32 from, s32 to, int direct, s32 dst, bool fromWater )
{
    const Maps::Tiles & fromTile = world.GetTiles( from );
    const Maps::Tiles & toTile = world.GetTiles( to );
    const int directionReflect = Direction::Reflect( direct );

    if ( !fromTile.isPassable( direct, fromWater, false ) )
        return false;

    if ( fromTile.isWater() && !toTile.isWater() ) {
        switch ( toTile.GetObject() ) {
        case MP2::OBJ_BOAT:
        case MP2::OBJ_MONSTER:
        case MP2::OBJ_HEROES:
            return false;

        case MP2::OBJ_COAST:
            return to == dst;

        default:
            break;
        }
    }
    else if ( !fromTile.isWater() && toTile.isWater() ) {
        switch ( toTile.GetObject() ) {
        case MP2::OBJ_BOAT:
            return to == dst;

        case MP2::OBJ_HEROES:
            return to == dst;

        default:
            break;
        }
    }

    // check corner water/coast
    if ( fromWater ) {
        switch ( direct ) {
        case Direction::TOP_LEFT:
            if ( !world.GetTiles( Maps::GetDirectionIndex( from, Direction::TOP ) ).isWater()
                 || !world.GetTiles( Maps::GetDirectionIndex( from, Direction::LEFT ) ).isWater() )
                return false;
            break;

        case Direction::TOP_RIGHT:
            if ( !world.GetTiles( Maps::GetDirectionIndex( from, Direction::TOP ) ).isWater()
                 || !world.GetTiles( Maps::GetDirectionIndex( from, Direction::RIGHT ) ).isWater() )
                return false;
            break;

        case Direction::BOTTOM_RIGHT:
            if ( !world.GetTiles( Maps::GetDirectionIndex( from, Direction::BOTTOM ) ).isWater()
                 || !world.GetTiles( Maps::GetDirectionIndex( from, Direction::RIGHT ) ).isWater() )
                return false;
            break;

        case Direction::BOTTOM_LEFT:
            if ( !world.GetTiles( Maps::GetDirectionIndex( from, Direction::BOTTOM ) ).isWater()
                 || !world.GetTiles( Maps::GetDirectionIndex( from, Direction::LEFT ) ).isWater() )
                return false;
            break;

        default:
            break;
        }
    }

    return PassableToTile( toTile, directionReflect, dst, fromWater );
}

uint32_t GetPenaltyFromTo( int from, int to, int direction, uint32_t pathfinding )
{
    const Maps::Tiles & tileTo = world.GetTiles( to );
    uint32_t penalty = ( world.GetTiles( from ).isRoad( direction ) || tileTo.isRoad( Direction::Reflect( direction ) ) )
                           ? Maps::Ground::roadPenalty
                           : Maps::Ground::GetPenalty( tileTo, pathfinding );

    // diagonal move costs 50% extra
    if ( direction & ( Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM_LEFT | Direction::TOP_LEFT ) )
        penalty = penalty * 3 / 2;

    return penalty;
}

uint32_t Route::Path::Find( int32_t from, int32_t to, bool fromWater /* false */, int limit /* -1 */, int pathfinding /* NONE */ )
{
    uint32_t pathCost = 0;

    s32 cur = from;
    s32 alt = 0;
    s32 tmp = 0;
    std::map<s32, cell_t> list;
    std::map<s32, cell_t>::iterator it1 = list.begin();
    std::map<s32, cell_t>::iterator it2 = list.end();

    list[cur].cost_g = 0;
    list[cur].cost_t = 0;
    list[cur].parent = -1;
    list[cur].open = 0;

    const Directions directions = Direction::All();
    clear();

    while ( cur != to ) {
        LocalEvent::Get().HandleEvents( false );
        for ( Directions::const_iterator it = directions.begin(); it != directions.end(); ++it ) {
            if ( Maps::isValidDirection( cur, *it ) ) {
                tmp = Maps::GetDirectionIndex( cur, *it );

                if ( list[tmp].open ) {
                    const u32 costg = GetPenaltyFromTo( cur, tmp, *it, pathfinding );

                    // new
                    if ( -1 == list[tmp].parent ) {
                        if ( ( list[cur].passbl & *it ) || PassableFromToTile( cur, tmp, *it, to, fromWater ) ) {
                            list[cur].passbl |= *it;

                            list[tmp].direct = *it;
                            list[tmp].cost_g = costg;
                            list[tmp].parent = cur;
                            list[tmp].open = 1;
                            list[tmp].cost_d = 75 * Maps::GetApproximateDistance( tmp, to );
                            list[tmp].cost_t = list[cur].cost_t + costg;
                        }
                    }
                    // check alt
                    else {
                        if ( list[tmp].cost_t > list[cur].cost_t + costg && ( ( list[cur].passbl & *it ) || PassableFromToTile( cur, tmp, *it, to, fromWater ) ) ) {
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

        DEBUG( DBG_OTHER, DBG_TRACE, "route, from: " << cur );

        // find minimal cost
        for ( ; it1 != it2; ++it1 )
            if ( ( *it1 ).second.open ) {
                const cell_t & cell2 = ( *it1 ).second;
#ifdef WITH_DEBUG
                if ( IS_DEBUG( DBG_OTHER, DBG_TRACE ) && cell2.cost_g != MAXU16 ) {
                    int direct = Direction::Get( cur, ( *it1 ).first );

                    if ( Direction::UNKNOWN != direct ) {
                        VERBOSE( "\t\tdirect: " << Direction::String( direct ) << ", index: " << ( *it1 ).first << ", cost g: " << cell2.cost_g
                                                << ", cost t: " << cell2.cost_t << ", cost d: " << cell2.cost_d );
                    }
                }
#endif

                if ( cell2.cost_t + cell2.cost_d < tmp ) {
                    tmp = cell2.cost_t + cell2.cost_d;
                    alt = ( *it1 ).first;
                }
            }

        // not found, and exception
        if ( MAXU16 == tmp || -1 == alt )
            break;
#ifdef WITH_DEBUG
        else
            DEBUG( DBG_OTHER, DBG_TRACE, "select: " << alt );
#endif
        cur = alt;

        if ( 0 < limit && GetCurrentLength( list, cur ) > limit )
            break;
    }

    // save path
    if ( cur == to ) {
        while ( cur != from ) {
            push_front( Route::Step( list[cur].parent, list[cur].direct, list[cur].cost_g ) );
            pathCost += list[cur].cost_g;
            cur = list[cur].parent;
        }
    }
    else {
        DEBUG( DBG_OTHER, DBG_TRACE,
               "not found"
                   << ", from:" << from << ", to: " << to );
    }

    return pathCost;
}
