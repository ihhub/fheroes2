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
#include <numeric>

#include "ai.h"
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
    ComparsionDistance( const int32_t index )
        : center( index )
    {}

    bool operator()( const int32_t index1, const int32_t index2 ) const
    {
        return Maps::GetApproximateDistance( center, index1 ) < Maps::GetApproximateDistance( center, index2 );
    }

    int32_t center;
};

Maps::Indexes MapsIndexesFilteredObject( const Maps::Indexes & indexes, const int obj, const bool ignoreHeroes = true )
{
    Maps::Indexes result;
    for ( size_t idx = 0; idx < indexes.size(); ++idx ) {
        if ( world.GetTiles( indexes[idx] ).GetObject( ignoreHeroes ) == obj ) {
            result.push_back( indexes[idx] );
        }
    }
    return result;
}

Maps::Indexes MapsIndexesObject( const int obj, const bool ignoreHeroes = true )
{
    Maps::Indexes result;
    const int32_t size = static_cast<int32_t>( world.getSize() );
    for ( int32_t idx = 0; idx < size; ++idx ) {
        if ( world.GetTiles( idx ).GetObject( ignoreHeroes ) == obj ) {
            result.push_back( idx );
        }
    }
    return result;
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

int Maps::GetDirection( int from, int to )
{
    if ( from == to )
        return Direction::CENTER;

    const int diff = to - from;
    const int width = world.w();

    if ( diff == ( -width - 1 ) ) {
        return Direction::TOP_LEFT;
    }
    else if ( diff == -width ) {
        return Direction::TOP;
    }
    else if ( diff == ( -width + 1 ) ) {
        return Direction::TOP_RIGHT;
    }
    else if ( diff == -1 ) {
        return Direction::LEFT;
    }
    else if ( diff == 1 ) {
        return Direction::RIGHT;
    }
    else if ( diff == width - 1 ) {
        return Direction::BOTTOM_LEFT;
    }
    else if ( diff == width ) {
        return Direction::BOTTOM;
    }
    else if ( diff == width + 1 ) {
        return Direction::BOTTOM_RIGHT;
    }

    return Direction::UNKNOWN;
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
    const int32_t width = world.w();

    switch ( vector ) {
    case Direction::TOP:
        return ( from >= width );
    case Direction::RIGHT:
        return ( ( from % width ) < ( width - 1 ) );
    case Direction::BOTTOM:
        return ( from < width * ( world.h() - 1 ) );
    case Direction::LEFT:
        return ( from % width );

    case Direction::TOP_RIGHT:
        return ( from >= width ) && ( ( from % width ) < ( width - 1 ) );

    case Direction::BOTTOM_RIGHT:
        return ( from < width * ( world.h() - 1 ) ) && ( ( from % width ) < ( width - 1 ) );

    case Direction::BOTTOM_LEFT:
        return ( from < width * ( world.h() - 1 ) ) && ( from % width );

    case Direction::TOP_LEFT:
        return ( from >= width ) && ( from % width );

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

Maps::Indexes Maps::GetAroundIndexes( s32 center )
{
    Indexes result;
    if ( !isValidAbsIndex( center ) )
        return result;

    result.reserve( 8 );
    const int width = world.w();
    const int x = center % width;
    const int y = center / width;

    if ( y > 0 ) {
        if ( x > 0 )
            result.push_back( center - width - 1 );

        result.push_back( center - width );

        if ( x < width - 1 )
            result.push_back( center - width + 1 );
    }

    if ( x > 0 )
        result.push_back( center - 1 );
    if ( x < width - 1 )
        result.push_back( center + 1 );

    if ( y < world.h() - 1 ) {
        if ( x > 0 )
            result.push_back( center + width - 1 );

        result.push_back( center + width );

        if ( x < width - 1 )
            result.push_back( center + width + 1 );
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
        const bool isAIPlayer = world.GetKingdom( color ).isControlAI();
        if ( isAIPlayer ) {
            scoute += Difficulty::GetScoutingBonus( conf.GameDifficulty() );
        }

        const int alliedColors = Players::GetPlayerFriends( color );

        const int revealRadiusSquared = scoute * scoute + 4; // constant factor for "backwards compatibility"
        for ( s32 y = center.y - scoute; y <= center.y + scoute; ++y ) {
            for ( s32 x = center.x - scoute; x <= center.x + scoute; ++x ) {
                const s32 dx = x - center.x;
                const s32 dy = y - center.y;
                if ( isValidAbsPoint( x, y ) && revealRadiusSquared >= dx * dx + dy * dy ) {
                    Maps::Tiles & tile = world.GetTiles( GetIndexFromAbsPoint( x, y ) );
                    if ( isAIPlayer && tile.isFog( color ) )
                        AI::Get().revealFog( tile );

                    tile.ClearFog( alliedColors );
                }
            }
        }
    }
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

Maps::Indexes Maps::GetObjectPositions( int obj, bool ignoreHeroes )
{
    return MapsIndexesObject( obj, ignoreHeroes );
}

Maps::Indexes Maps::GetObjectPositions( s32 center, int obj, bool ignoreHeroes )
{
    Indexes results = MapsIndexesObject( obj, ignoreHeroes );
    std::sort( results.begin(), results.end(), ComparsionDistance( center ) );
    return results;
}

Maps::Indexes Maps::GetObjectsPositions( const std::vector<u8> & objs )
{
    if ( objs.size() == 1 ) {
        return MapsIndexesObject( objs[0], true );
    }

    Maps::Indexes result;
    if ( objs.empty() )
        return result;

    const int32_t size = static_cast<int32_t>( world.getSize() );
    for ( int32_t idx = 0; idx < size; ++idx ) {
        const int objectID = world.GetTiles( idx ).GetObject( true );

        for ( const uint8_t obj : objs ) {
            if ( obj == objectID ) {
                result.push_back( idx );
                break;
            }
        }
    }
    return result;
}

bool MapsTileIsUnderProtection( s32 from, s32 index ) /* from: center, index: monster */
{
    bool result = false;
    const Maps::Tiles & tile1 = world.GetTiles( from );
    const Maps::Tiles & tile2 = world.GetTiles( index );

    if ( !MP2::isPickupObject( tile1.GetObject() ) && tile2.GetObject() == MP2::OBJ_MONSTER && tile1.isWater() == tile2.isWater() ) {
        const int monsterDirection = Maps::GetDirection( index, from );
        /* if monster can attack to */
        result = ( tile2.GetPassable() & monsterDirection ) && ( tile1.GetPassable() & Maps::GetDirection( from, index ) );

        if ( !result ) {
            /* h2 specific monster attack: BOTTOM_LEFT impassable! */
            if ( Direction::BOTTOM_LEFT == monsterDirection && ( Direction::LEFT & tile2.GetPassable() ) && ( Direction::TOP & tile1.GetPassable() ) )
                result = true;
            else
                /* h2 specific monster attack: BOTTOM_RIGHT impassable! */
                if ( Direction::BOTTOM_RIGHT == monsterDirection && ( Direction::RIGHT & tile2.GetPassable() ) && ( Direction::TOP & tile1.GetPassable() ) )
                result = true;
        }
    }

    return result;
}

bool Maps::IsNearTiles( s32 index1, s32 index2 )
{
    return DIRECTION_ALL & Maps::GetDirection( index1, index2 );
}

bool Maps::TileIsUnderProtection( s32 center )
{
    return MP2::OBJ_MONSTER == world.GetTiles( center ).GetObject() ? true : GetTilesUnderProtection( center ).size();
}

Maps::Indexes Maps::GetTilesUnderProtection( s32 center )
{
    Indexes result;
    if ( !isValidAbsIndex( center ) )
        return result;

    result.reserve( 9 );
    const int width = world.w();
    const int x = center % width;
    const int y = center / width;

    auto validateAndInsert = [&result, &center]( const int index ) {
        if ( MapsTileIsUnderProtection( center, index ) )
            result.push_back( index );
    };

    if ( y > 0 ) {
        if ( x > 0 )
            validateAndInsert( center - width - 1 );

        validateAndInsert( center - width );

        if ( x < width - 1 )
            validateAndInsert( center - width + 1 );
    }

    if ( x > 0 )
        validateAndInsert( center - 1 );
    if ( MP2::OBJ_MONSTER == world.GetTiles( center ).GetObject() )
        result.push_back( center );
    if ( x < width - 1 )
        validateAndInsert( center + 1 );

    if ( y < world.h() - 1 ) {
        if ( x > 0 )
            validateAndInsert( center + width - 1 );

        validateAndInsert( center + width );

        if ( x < width - 1 )
            validateAndInsert( center + width + 1 );
    }

    return result;
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
    const Maps::Tiles & entranceTile = world.GetTiles( center.x, center.y );
    const int entranceObject = entranceTile.GetObject();
    const uint32_t castleID = entranceTile.GetObjectUID();

    if ( isRandom && ( entranceObject != MP2::OBJ_RNDCASTLE && entranceObject != MP2::OBJ_RNDTOWN ) ) {
        DEBUG( DBG_GAME, DBG_WARN,
               "incorrect object"
                   << ", index: " << GetIndexFromAbsPoint( center.x, center.y ) );
        return;
    }

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
        static const int shadowCoordinates[16][2]
            = {{-4, -2}, {-3, -2}, {-2, -2}, {-1, -2}, {-5, -1}, {-4, -1}, {-3, -1}, {-2, -1}, {-1, -1}, {-4, 0}, {-3, 0}, {-2, 0}, {-1, 0}, {-3, 1}, {-2, 1}, {-1, 1}};

        const int castleTile = GetIndexFromAbsPoint( center.x + castleCoordinates[index][0], center.y + castleCoordinates[index][1] );
        if ( isValidAbsIndex( castleTile ) ) {
            Tiles & tile = world.GetTiles( castleTile );

            if ( isRandom )
                tile.ReplaceObjectSprite( castleID, 38, 35 * 4, lookupID, fullTownIndex ); // OBJNTWRD to OBJNTOWN
            else
                tile.UpdateObjectSprite( castleID, 35, 35 * 4, -16 ); // no change in tileset

            if ( index == 0 ) {
                TilesAddon * addon = tile.FindAddonLevel2( castleID );
                if ( addon && MP2::GetICNObject( addon->object ) == ICN::OBJNTWRD ) {
                    addon->object -= 12;
                    addon->index = fullTownIndex - 16;
                }
            }
        }

        const int shadowTile = GetIndexFromAbsPoint( center.x + shadowCoordinates[index][0], center.y + shadowCoordinates[index][1] );
        if ( isValidAbsIndex( shadowTile ) ) {
            if ( isRandom )
                world.GetTiles( shadowTile ).ReplaceObjectSprite( castleID, 38, 37 * 4, lookupID + 32, fullTownIndex ); // OBJNTWRD to OBJNTWSH
            else
                world.GetTiles( shadowTile ).UpdateObjectSprite( castleID, 37, 37 * 4, -16 ); // no change in tileset
        }
    }
}

int Maps::TileIsCoast( s32 center, int filter )
{
    int result = 0;
    const Directions & directions = Direction::All();

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
