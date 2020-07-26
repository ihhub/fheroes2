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

#include <algorithm>

#include "difficulty.h"
#include "game.h"
#include "icn.h"
#include "kingdom.h"
#include "maps.h"
#include "maps_tiles.h"
#include "race.h"
#include "settings.h"
#include "world.h"

struct ComparsionDistance
{
    ComparsionDistance( const s32 & index )
        : center( index )
    {}

    bool operator()( const s32 & index1, const s32 & index2 ) const
    {
        return Maps::GetApproximateDistance( center, index1 ) < Maps::GetApproximateDistance( center, index2 );
    }

    s32 center;
};

Maps::IndexesDistance::IndexesDistance( s32 from, s32 center, u32 dist, int sort )
{
    Assign( from, GetAroundIndexes( center, dist, sort ), sort );
}

Maps::IndexesDistance::IndexesDistance( s32 from, const Indexes & indexes, int sort )
{
    Assign( from, indexes, sort );
}

void Maps::IndexesDistance::Assign( s32 from, const Indexes & indexes, int sort )
{
    reserve( indexes.size() );

    for ( Indexes::const_iterator it = indexes.begin(); it != indexes.end(); ++it )
        push_back( IndexDistance( *it, Maps::GetApproximateDistance( from, *it ) ) );

    if ( 1 == sort )
        std::sort( begin(), end(), IndexDistance::Shortest );
    else if ( 2 == sort )
        std::sort( begin(), end(), IndexDistance::Longest );
}

bool TileIsObject( s32 index, int obj )
{
    return obj == world.GetTiles( index ).GetObject();
}

bool TileIsObjects( s32 index, const u8 * objs )
{
    while ( objs && *objs ) {
        if ( *objs == world.GetTiles( index ).GetObject() )
            return true;
        ++objs;
    }
    return false;
}

Maps::Indexes & MapsIndexesFilteredObjects( Maps::Indexes & indexes, const u8 * objs )
{
    indexes.resize(
        std::distance( indexes.begin(), std::remove_if( indexes.begin(), indexes.end(), std::not1( std::bind2nd( std::ptr_fun( &TileIsObjects ), objs ) ) ) ) );

    return indexes;
}

Maps::Indexes & MapsIndexesFilteredObject( Maps::Indexes & indexes, int obj )
{
    indexes.resize( std::distance( indexes.begin(), std::remove_if( indexes.begin(), indexes.end(), std::not1( std::bind2nd( std::ptr_fun( &TileIsObject ), obj ) ) ) ) );
    return indexes;
}

const char * Maps::SizeString( int s )
{
    const char * mapsize[] = {"Unknown", _( "maps|Small" ), _( "maps|Medium" ), _( "maps|Large" ), _( "maps|Extra Large" ), _( "maps|Custom Size" )};

    switch ( s ) {
    case SMALL:
        return mapsize[1];
    case MEDIUM:
        return mapsize[2];
    case LARGE:
        return mapsize[3];
    case XLARGE:
        return mapsize[4];
    default:
        break;
    }

    return mapsize[5];
}

const char * Maps::GetMinesName( int type )
{
    switch ( type ) {
    case Resource::ORE:
        return _( "Ore Mine" );
    case Resource::SULFUR:
        return _( "Sulfur Mine" );
    case Resource::CRYSTAL:
        return _( "Crystal Mine" );
    case Resource::GEMS:
        return _( "Gems Mine" );
    case Resource::GOLD:
        return _( "Gold Mine" );
    default:
        break;
    }

    return _( "Mine" );
}

s32 Maps::GetDirectionIndex( s32 from, int vector )
{
    switch ( vector ) {
    case Direction::TOP:
        return from - world.w();
    case Direction::TOP_RIGHT:
        return from - world.w() + 1;
    case Direction::RIGHT:
        return from + 1;
    case Direction::BOTTOM_RIGHT:
        return from + world.w() + 1;
    case Direction::BOTTOM:
        return from + world.w();
    case Direction::BOTTOM_LEFT:
        return from + world.w() - 1;
    case Direction::LEFT:
        return from - 1;
    case Direction::TOP_LEFT:
        return from - world.w() - 1;
    default:
        break;
    }

    return -1;
}

// check bound
bool Maps::isValidDirection( s32 from, int vector )
{
    switch ( vector ) {
    case Direction::TOP:
        return ( from >= world.w() );
    case Direction::RIGHT:
        return ( ( from % world.w() ) < ( world.w() - 1 ) );
    case Direction::BOTTOM:
        return ( from < world.w() * ( world.h() - 1 ) );
    case Direction::LEFT:
        return ( from % world.w() );

    case Direction::TOP_RIGHT:
        return isValidDirection( from, Direction::TOP ) && isValidDirection( from, Direction::RIGHT );

    case Direction::BOTTOM_RIGHT:
        return isValidDirection( from, Direction::BOTTOM ) && isValidDirection( from, Direction::RIGHT );

    case Direction::BOTTOM_LEFT:
        return isValidDirection( from, Direction::BOTTOM ) && isValidDirection( from, Direction::LEFT );

    case Direction::TOP_LEFT:
        return isValidDirection( from, Direction::TOP ) && isValidDirection( from, Direction::LEFT );

    default:
        break;
    }

    return false;
}

Point Maps::GetPoint( s32 index )
{
    return Point( index % world.w(), index / world.w() );
}

bool Maps::isValidAbsPoint( const Point & pt )
{
    return isValidAbsPoint( pt.x, pt.y );
}

bool Maps::isValidAbsIndex( s32 ii )
{
    return 0 <= ii && ii < world.w() * world.h();
}

bool Maps::isValidAbsPoint( s32 x, s32 y )
{
    return 0 <= x && world.w() > x && 0 <= y && world.h() > y;
}

/* convert maps point to index maps */
s32 Maps::GetIndexFromAbsPoint( const Point & mp )
{
    return GetIndexFromAbsPoint( mp.x, mp.y );
}

s32 Maps::GetIndexFromAbsPoint( s32 px, s32 py )
{
    const s32 res = py * world.w() + px;

    if ( px < 0 || py < 0 ) {
        VERBOSE( "Maps::GetIndexFromAbsPoint: error coods, "
                 << "x: " << px << ", y: " << py );
        return -1;
    }

    return res;
}

Maps::Indexes Maps::GetAllIndexes( void )
{
    Indexes result;
    result.assign( world.w() * world.h(), 0 );

    for ( Indexes::iterator it = result.begin(); it != result.end(); ++it )
        *it = std::distance( result.begin(), it );
    return result;
}

Maps::Indexes Maps::GetAroundIndexes( s32 center )
{
    Indexes result;
    result.reserve( 8 );

    if ( isValidAbsIndex( center ) ) {
        const Directions directions = Direction::All();

        for ( Directions::const_iterator it = directions.begin(); it != directions.end(); ++it )
            if ( isValidDirection( center, *it ) )
                result.push_back( GetDirectionIndex( center, *it ) );
    }

    return result;
}

Maps::Indexes Maps::GetAroundIndexes( s32 center, int dist, bool sort )
{
    Indexes results;
    results.reserve( dist * 12 );

    const Point cp = GetPoint( center );

    for ( s32 xx = cp.x - dist; xx <= cp.x + dist; ++xx )
        for ( s32 yy = cp.y - dist; yy <= cp.y + dist; ++yy ) {
            if ( isValidAbsPoint( xx, yy ) && ( xx != cp.x || yy != cp.y ) )
                results.push_back( GetIndexFromAbsPoint( xx, yy ) );
        }

    if ( sort )
        std::sort( results.begin(), results.end(), ComparsionDistance( center ) );

    return results;
}

Maps::Indexes Maps::GetDistanceIndexes( s32 center, int dist )
{
    Indexes results;
    results.reserve( dist * 6 );

    const Point cp = GetPoint( center );

    for ( s32 xx = cp.x - dist; xx <= cp.x + dist; ++xx ) {
        if ( isValidAbsPoint( xx, cp.y - dist ) )
            results.push_back( GetIndexFromAbsPoint( xx, cp.y - dist ) );
        if ( isValidAbsPoint( xx, cp.y + dist ) )
            results.push_back( GetIndexFromAbsPoint( xx, cp.y + dist ) );
    }

    for ( s32 yy = cp.y - dist + 1; yy < cp.y + dist; ++yy ) {
        if ( isValidAbsPoint( cp.x - dist, yy ) )
            results.push_back( GetIndexFromAbsPoint( cp.x - dist, yy ) );
        if ( isValidAbsPoint( cp.x + dist, yy ) )
            results.push_back( GetIndexFromAbsPoint( cp.x + dist, yy ) );
    }

    return results;
}

void Maps::ClearFog( s32 index, int scoute, int color )
{
    if ( 0 != scoute && isValidAbsIndex( index ) ) {
        const Point center = GetPoint( index );
        const Settings & conf = Settings::Get();

        // AI advantage
        if ( world.GetKingdom( color ).isControlAI() ) {
            switch ( conf.GameDifficulty() ) {
            case Difficulty::NORMAL:
                scoute += 2;
                break;
            case Difficulty::HARD:
                scoute += 3;
                break;
            case Difficulty::EXPERT:
                scoute += 4;
                break;
            case Difficulty::IMPOSSIBLE:
                scoute += 6;
                break;
            default:
                break;
            }
        }

        int colors = conf.ExtUnionsAllowViewMaps() ? Players::GetPlayerFriends( color ) : color;

        const int revealRadiusSquared = scoute * scoute + 4; // constant factor for "backwards compatibility"
        for ( s32 y = center.y - scoute; y <= center.y + scoute; ++y ) {
            for ( s32 x = center.x - scoute; x <= center.x + scoute; ++x ) {
                const s32 dx = x - center.x;
                const s32 dy = y - center.y;
                if ( isValidAbsPoint( x, y ) && revealRadiusSquared >= dx * dx + dy * dy )
                    world.GetTiles( GetIndexFromAbsPoint( x, y ) ).ClearFog( colors );
            }
        }
    }
}

Maps::Indexes Maps::ScanAroundObjects( s32 center, const u8 * objs )
{
    Indexes results = Maps::GetAroundIndexes( center );
    return MapsIndexesFilteredObjects( results, objs );
}

Maps::Indexes Maps::ScanAroundObject( s32 center, int obj )
{
    Maps::Indexes results = Maps::GetAroundIndexes( center );
    return MapsIndexesFilteredObject( results, obj );
}

Maps::Indexes Maps::ScanAroundObject( s32 center, u32 dist, int obj )
{
    Indexes results = Maps::GetAroundIndexes( center, dist, true );
    return MapsIndexesFilteredObject( results, obj );
}

Maps::Indexes Maps::ScanAroundObjects( s32 center, u32 dist, const u8 * objs )
{
    Indexes results = Maps::GetAroundIndexes( center, dist, true );
    return MapsIndexesFilteredObjects( results, objs );
}

Maps::Indexes Maps::GetObjectPositions( int obj, bool check_hero )
{
    Maps::Indexes results = GetAllIndexes();
    MapsIndexesFilteredObject( results, obj );

    if ( check_hero && obj != MP2::OBJ_HEROES ) {
        const Indexes & v = GetObjectPositions( MP2::OBJ_HEROES, false );
        for ( Indexes::const_iterator it = v.begin(); it != v.end(); ++it ) {
            const Heroes * hero = world.GetHeroes( GetPoint( *it ) );
            if ( hero && obj == hero->GetMapsObject() )
                results.push_back( *it );
        }
    }

    return results;
}

Maps::Indexes Maps::GetObjectPositions( s32 center, int obj, bool check_hero )
{
    Indexes results = Maps::GetObjectPositions( obj, check_hero );
    std::sort( results.begin(), results.end(), ComparsionDistance( center ) );
    return results;
}

Maps::Indexes Maps::GetObjectsPositions( const u8 * objs )
{
    Indexes results = GetAllIndexes();
    return MapsIndexesFilteredObjects( results, objs );
}

bool MapsTileIsUnderProtection( s32 from, s32 index ) /* from: center, index: monster */
{
    bool result = false;
    const Maps::Tiles & tile1 = world.GetTiles( from );
    const Maps::Tiles & tile2 = world.GetTiles( index );

    if ( tile1.isWater() == tile2.isWater() ) {
        /* if monster can attack to */
        result = ( tile2.GetPassable() & Direction::Get( index, from ) ) && ( tile1.GetPassable() & Direction::Get( from, index ) );

        if ( !result ) {
            /* h2 specific monster attack: BOTTOM_LEFT impassable! */
            if ( Direction::BOTTOM_LEFT == Direction::Get( index, from ) && ( Direction::LEFT & tile2.GetPassable() ) && ( Direction::TOP & tile1.GetPassable() ) )
                result = true;
            else
                /* h2 specific monster attack: BOTTOM_RIGHT impassable! */
                if ( Direction::BOTTOM_RIGHT == Direction::Get( index, from ) && ( Direction::RIGHT & tile2.GetPassable() ) && ( Direction::TOP & tile1.GetPassable() ) )
                result = true;
        }
    }

    return result;
}

bool Maps::IsNearTiles( s32 index1, s32 index2 )
{
    return DIRECTION_ALL & Direction::Get( index1, index2 );
}

bool Maps::TileIsUnderProtection( s32 center )
{
    return MP2::OBJ_MONSTER == world.GetTiles( center ).GetObject() ? true : GetTilesUnderProtection( center ).size();
}

Maps::Indexes Maps::GetTilesUnderProtection( s32 center )
{
    Indexes indexes = Maps::ScanAroundObject( center, MP2::OBJ_MONSTER );

    indexes.resize( std::distance( indexes.begin(),
                                   std::remove_if( indexes.begin(), indexes.end(), std::not1( std::bind1st( std::ptr_fun( &MapsTileIsUnderProtection ), center ) ) ) ) );

    if ( MP2::OBJ_MONSTER == world.GetTiles( center ).GetObject() )
        indexes.push_back( center );

    return indexes;
}

u32 Maps::GetApproximateDistance( s32 index1, s32 index2 )
{
    const Size sz( GetPoint( index1 ) - GetPoint( index2 ) );
    // diagonal move costs 1.5 as much
    return std::max( sz.w, sz.h ) + std::min( sz.w, sz.h ) / 2;
}

void Maps::MinimizeAreaForCastle( const Point & center )
{
    // reset castle ID
    for ( s32 yy = -3; yy < 2; ++yy )
        for ( s32 xx = -2; xx < 3; ++xx ) {
            Maps::Tiles & tile = world.GetTiles( center.x + xx, center.y + yy );

            if ( MP2::OBJN_RNDCASTLE == tile.GetObject() || MP2::OBJN_RNDTOWN == tile.GetObject() || MP2::OBJN_CASTLE == tile.GetObject() )
                tile.SetObject( MP2::OBJ_ZERO );
        }

    // set minimum area castle ID
    for ( s32 yy = -1; yy < 1; ++yy )
        for ( s32 xx = -2; xx < 3; ++xx ) {
            Maps::Tiles & tile = world.GetTiles( center.x + xx, center.y + yy );

            // skip angle
            if ( yy == -1 && ( xx == -2 || xx == 2 ) )
                continue;

            tile.SetObject( MP2::OBJN_CASTLE );
        }

    // restore center ID
    world.GetTiles( center.x, center.y ).SetObject( MP2::OBJ_CASTLE );
}

void Maps::UpdateCastleSprite( const Point & center, int race, bool isCastle, bool isRandom )
{
    /*
    Castle/Town object image consists of 42 tile sprites:
    10 base tiles (OBJNTWBA) with 16 shadow tiles on left side (OBJNTWSH) overlayed by 16 town tiles (OBJNTOWN)

    Shadows (OBJNTWSH)  Castle (OBJNTOWN)
                              0
       32 33 34 35      1  2  3  4  5
    36 37 38 39 40      6  7  8  9 10
       41 42 43 44     11 12 13 14 15
          45 46 47
    */

    // correct only RND town and castle
    const int entranceObject = world.GetTiles( center.x, center.y ).GetObject();

    if ( isRandom && ( entranceObject != MP2::OBJ_RNDCASTLE && entranceObject != MP2::OBJ_RNDTOWN ) ) {
        DEBUG( DBG_GAME, DBG_WARN,
               "incorrect object"
                   << ", index: " << GetIndexFromAbsPoint( center.x, center.y ) );
        return;
    }

    const int castleICN = isRandom ? ICN::OBJNTWRD : ICN::OBJNTOWN;
    const int shadowICN = isRandom ? ICN::OBJNTWRD : ICN::OBJNTWSH;

    int raceIndex = 0;
    switch ( race ) {
    case Race::BARB:
        raceIndex = 1;
        break;
    case Race::SORC:
        raceIndex = 2;
        break;
    case Race::WRLK:
        raceIndex = 3;
        break;
    case Race::WZRD:
        raceIndex = 4;
        break;
    case Race::NECR:
        raceIndex = 5;
        break;
    default:
        break;
    }

    for ( int index = 0; index < 16; ++index ) {
        const int fullTownIndex = index + ( isCastle ? 0 : 16 ) + raceIndex * 32;
        const int lookupID = isRandom ? index + ( isCastle ? 0 : 16 ) : fullTownIndex;

        static const int castleCoordinates[16][2]
            = {{0, -3}, {-2, -2}, {-1, -2}, {0, -2}, {1, -2}, {2, -2}, {-2, -1}, {-1, -1}, {0, -1}, {1, -1}, {2, -1}, {-2, 0}, {-1, 0}, {0, 0}, {1, 0}, {2, 0}};
        static const int shadowCoordinates[16][2] = {{-4, -2}, {-3, -2}, {-2, -2}, {-1, -2}, {-5, -1}, {-4, -1}, {-3, -1}, {-2, -1},
                                                     {-1, -1}, {-4, 0},  {-3, 0},  {-2, 0},  {-1, 0},  {-3, -1}, {-2, -1}, {-1, -1}};

        const int castleTile = GetIndexFromAbsPoint( center.x + castleCoordinates[index][0], center.y + castleCoordinates[index][1] );
        if ( isValidAbsIndex( castleTile ) ) {
            Maps::TilesAddon * addon = world.GetTiles( castleTile ).FindAddonICN( castleICN, -1, lookupID );

            if ( addon ) {
                if ( isRandom ) {
                    addon->object -= 12; // OBJNTWRD to OBJNTOWN
                    addon->index = fullTownIndex;
                }
                else {
                    addon->index -= 16;
                }
            }
        }

        const int shadowTile = GetIndexFromAbsPoint( center.x + shadowCoordinates[index][0], center.y + shadowCoordinates[index][1] );
        if ( isValidAbsIndex( shadowTile ) ) {
            Maps::TilesAddon * addon = world.GetTiles( shadowTile ).FindAddonICN( shadowICN, -1, isRandom ? lookupID + 32 : lookupID );

            if ( addon ) {
                if ( isRandom ) {
                    addon->object -= 4; // OBJNTWRD to OBJNTWSH
                    addon->index = fullTownIndex;
                }
                else {
                    addon->index -= 16;
                }
            }
        }
    }
}

int Maps::TileIsCoast( s32 center, int filter )
{
    int result = 0;
    const Directions directions = Direction::All();

    for ( Directions::const_iterator it = directions.begin(); it != directions.end(); ++it )
        if ( ( *it & filter ) && isValidDirection( center, *it ) && world.GetTiles( GetDirectionIndex( center, *it ) ).isWater() )
            result |= *it;

    return result;
}

StreamBase & operator>>( StreamBase & sb, IndexObject & st )
{
    return sb >> st.first >> st.second;
}

StreamBase & operator>>( StreamBase & sb, IndexDistance & st )
{
    return sb >> st.first >> st.second;
}

StreamBase & operator>>( StreamBase & sb, ObjectColor & st )
{
    return sb >> st.first >> st.second;
}

StreamBase & operator>>( StreamBase & sb, ResourceCount & st )
{
    return sb >> st.first >> st.second;
}
