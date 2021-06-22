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

#include "ai.h"
#include "difficulty.h"
#include "game.h"
#include "icn.h"
#include "kingdom.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "race.h"
#include "translations.h"
#include "world.h"

namespace
{
    std::vector<int32_t> getTileToClearIndicies( const int32_t tileIndex, int scouteValue, const int playerColor )
    {
        std::vector<int32_t> indicies;

        if ( scouteValue <= 0 || !Maps::isValidAbsIndex( tileIndex ) ) {
            return indicies;
        }

        const fheroes2::Point center = Maps::GetPoint( tileIndex );

        // AI is cheating!
        const bool isAIPlayer = world.GetKingdom( playerColor ).isControlAI();
        if ( isAIPlayer ) {
            scouteValue += Difficulty::GetScoutingBonus( Game::getDifficulty() );
        }

        const int revealRadiusSquared = scouteValue * scouteValue + 4; // constant factor for "backwards compatibility"
        for ( int32_t y = center.y - scouteValue; y <= center.y + scouteValue; ++y ) {
            if ( y < 0 || y >= world.h() )
                continue;

            for ( int32_t x = center.x - scouteValue; x <= center.x + scouteValue; ++x ) {
                if ( x < 0 || x >= world.w() )
                    continue;

                const int32_t dx = x - center.x;
                const int32_t dy = y - center.y;
                if ( revealRadiusSquared >= dx * dx + dy * dy ) {
                    indicies.emplace_back( Maps::GetIndexFromAbsPoint( x, y ) );
                }
            }
        }

        return indicies;
    }
}

struct ComparsionDistance
{
    explicit ComparsionDistance( const int32_t index )
        : centerPoint( Maps::GetPoint( index ) )
    {}

    ComparsionDistance() = delete;

    bool operator()( const int32_t index1, const int32_t index2 ) const
    {
        const fheroes2::Point point1( Maps::GetPoint( index1 ) );
        const fheroes2::Point point2( Maps::GetPoint( index2 ) );

        const int32_t diffX1 = std::abs( centerPoint.x - point1.x );
        const int32_t diffY1 = std::abs( centerPoint.y - point1.y );
        const int32_t diffX2 = std::abs( centerPoint.x - point2.x );
        const int32_t diffY2 = std::abs( centerPoint.y - point2.y );

        return ( diffX1 * diffX1 + diffY1 * diffY1 ) < ( diffX2 * diffX2 + diffY2 * diffY2 );
    }

    const fheroes2::Point centerPoint;
};

Maps::Indexes Maps::MapsIndexesFilteredObject( const Maps::Indexes & indexes, const int obj, const bool ignoreHeroes /* = true */ )
{
    Maps::Indexes result;
    for ( size_t idx = 0; idx < indexes.size(); ++idx ) {
        if ( world.GetTiles( indexes[idx] ).GetObject( !ignoreHeroes ) == obj ) {
            result.push_back( indexes[idx] );
        }
    }
    return result;
}

Maps::Indexes Maps::MapsIndexesObject( const int obj, const bool ignoreHeroes /* = true */ )
{
    Maps::Indexes result;
    const int32_t size = static_cast<int32_t>( world.getSize() );
    for ( int32_t idx = 0; idx < size; ++idx ) {
        if ( world.GetTiles( idx ).GetObject( !ignoreHeroes ) == obj ) {
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

int32_t Maps::GetDirectionIndex( int32_t from, int vector )
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
bool Maps::isValidDirection( int32_t from, int vector )
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
        return ( from % width ) != 0;

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

fheroes2::Point Maps::GetPoint( const int32_t index )
{
    return fheroes2::Point( index % world.w(), index / world.w() );
}

bool Maps::isValidAbsIndex( const int32_t index )
{
    return 0 <= index && index < world.w() * world.h();
}

bool Maps::isValidAbsPoint( const int32_t x, const int32_t y )
{
    return 0 <= x && world.w() > x && 0 <= y && world.h() > y;
}

int32_t Maps::GetIndexFromAbsPoint( const fheroes2::Point & mp )
{
    if ( mp.x < 0 || mp.y < 0 ) {
        return -1;
    }

    return mp.y * world.w() + mp.x;
}

int32_t Maps::GetIndexFromAbsPoint( const int32_t x, const int32_t y )
{
    if ( x < 0 || y < 0 ) {
        return -1;
    }

    return y * world.w() + x;
}

Maps::Indexes Maps::GetAroundIndexes( int32_t center )
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

Maps::Indexes Maps::GetAroundIndexes( const int32_t tileIndex, const int32_t maxDistanceFromTile, bool sortTiles )
{
    Indexes results;
    results.reserve( maxDistanceFromTile * 12 );

    const int32_t width = world.w();
    const int32_t size = world.h() * width;

    for ( int32_t y = -maxDistanceFromTile; y <= maxDistanceFromTile; ++y ) {
        for ( int32_t x = -maxDistanceFromTile; x <= maxDistanceFromTile; ++x ) {
            const int32_t tileId = tileIndex + y * width + x;

            if ( tileId >= 0 && tileId < size && tileId != tileIndex ) {
                results.push_back( tileId );
            }
        }
    }

    if ( sortTiles ) {
        std::sort( results.begin(), results.end(), ComparsionDistance( tileIndex ) );
    }

    return results;
}

void Maps::ClearFog( const int32_t tileIndex, const int scouteValue, const int playerColor )
{
    const std::vector<int32_t> tileIndicies = getTileToClearIndicies( tileIndex, scouteValue, playerColor );
    if ( tileIndicies.empty() ) {
        // Nothing to uncover.
        return;
    }

    const bool isAIPlayer = world.GetKingdom( playerColor ).isControlAI();
    const int alliedColors = Players::GetPlayerFriends( playerColor );

    for ( const int32_t index : tileIndicies ) {
        Maps::Tiles & tile = world.GetTiles( index );
        if ( isAIPlayer && tile.isFog( playerColor ) ) {
            AI::Get().revealFog( tile );
        }

        tile.ClearFog( alliedColors );
    }
}

int32_t Maps::getFogTileCountToBeRevealed( const int32_t tileIndex, const int scouteValue, const int playerColor )
{
    const std::vector<int32_t> tileIndicies = getTileToClearIndicies( tileIndex, scouteValue, playerColor );

    int32_t tileCount = 0;

    for ( const int32_t index : tileIndicies ) {
        const Maps::Tiles & tile = world.GetTiles( index );
        if ( tile.isFog( playerColor ) ) {
            ++tileCount;
        }
    }

    return tileCount;
}

Maps::Indexes Maps::ScanAroundObject( const int32_t center, const int obj, const bool ignoreHeroes )
{
    Maps::Indexes results = Maps::GetAroundIndexes( center );
    return MapsIndexesFilteredObject( results, obj, ignoreHeroes );
}

Maps::Indexes Maps::ScanAroundObject( const int32_t center, const int obj )
{
    Maps::Indexes results = Maps::GetAroundIndexes( center );
    return MapsIndexesFilteredObject( results, obj );
}

Maps::Indexes Maps::ScanAroundObjectWithDistance( const int32_t center, const uint32_t dist, const int obj )
{
    Indexes results = Maps::GetAroundIndexes( center, dist, true );
    return MapsIndexesFilteredObject( results, obj );
}

Maps::Indexes Maps::GetObjectPositions( int obj, bool ignoreHeroes )
{
    return MapsIndexesObject( obj, ignoreHeroes );
}

Maps::Indexes Maps::GetObjectPositions( int32_t center, int obj, bool ignoreHeroes )
{
    Indexes results = MapsIndexesObject( obj, ignoreHeroes );
    std::sort( results.begin(), results.end(), ComparsionDistance( center ) );
    return results;
}

Maps::Indexes Maps::GetObjectsPositions( const std::vector<uint8_t> & objs )
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

bool MapsTileIsUnderProtection( int32_t from, int32_t index ) /* from: center, index: monster */
{
    const Maps::Tiles & tile1 = world.GetTiles( from );
    const Maps::Tiles & tile2 = world.GetTiles( index );

    if ( !MP2::isPickupObject( tile1.GetObject() ) && tile2.GetObject() == MP2::OBJ_MONSTER && tile1.isWater() == tile2.isWater() ) {
        const int monsterDirection = Maps::GetDirection( index, from );
        // if monster can attack to
        if ( ( tile2.GetPassable() & monsterDirection ) && ( tile1.GetPassable() & Maps::GetDirection( from, index ) ) )
            return true;

        // h2 specific monster attack: BOTTOM_LEFT impassable!
        if ( Direction::BOTTOM_LEFT == monsterDirection && ( Direction::LEFT & tile2.GetPassable() ) && ( Direction::TOP & tile1.GetPassable() ) )
            return true;

        // h2 specific monster attack: BOTTOM_RIGHT impassable!
        if ( Direction::BOTTOM_RIGHT == monsterDirection && ( Direction::RIGHT & tile2.GetPassable() ) && ( Direction::TOP & tile1.GetPassable() ) )
            return true;
    }

    return false;
}

bool Maps::TileIsUnderProtection( int32_t center )
{
    return MP2::OBJ_MONSTER == world.GetTiles( center ).GetObject() ? true : !GetTilesUnderProtection( center ).empty();
}

Maps::Indexes Maps::GetTilesUnderProtection( int32_t center )
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

uint32_t Maps::GetApproximateDistance( const int32_t pos1, const int32_t pos2 )
{
    const fheroes2::Point point1( GetPoint( pos1 ) );
    const fheroes2::Point point2( GetPoint( pos2 ) );

    const fheroes2::Size sz( std::abs( point1.x - point2.x ), std::abs( point1.y - point2.y ) );
    // diagonal move costs 1.5 as much
    return std::max( sz.width, sz.height ) + std::min( sz.width, sz.height ) / 2;
}

void Maps::MinimizeAreaForCastle( const fheroes2::Point & center )
{
    // Reset castle ID
    for ( int32_t y = -3; y < 2; ++y ) {
        for ( int32_t x = -2; x < 3; ++x ) {
            Maps::Tiles & tile = world.GetTiles( center.x + x, center.y + y );

            if ( MP2::OBJN_RNDCASTLE == tile.GetObject() || MP2::OBJN_RNDTOWN == tile.GetObject() || MP2::OBJN_CASTLE == tile.GetObject() )
                tile.setAsEmpty();
        }
    }

    // set minimum area castle ID
    for ( int32_t y = -1; y < 1; ++y ) {
        for ( int32_t x = -2; x < 3; ++x ) {
            Maps::Tiles & tile = world.GetTiles( center.x + x, center.y + y );

            // skip angle
            if ( y == -1 && ( x == -2 || x == 2 ) )
                continue;

            tile.SetObject( MP2::OBJN_CASTLE );
        }
    }

    // restore center ID
    world.GetTiles( center.x, center.y ).SetObject( MP2::OBJ_CASTLE );
}

void Maps::UpdateCastleSprite( const fheroes2::Point & center, int race, bool isCastle, bool isRandom )
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
        DEBUG_LOG( DBG_GAME, DBG_WARN,
                   "incorrect object"
                       << ", index: " << GetIndexFromAbsPoint( center.x, center.y ) );
        return;
    }

    int raceIndex = 0; // Race::KNIGHT
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

int Maps::TileIsCoast( int32_t center, int filter )
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

StreamBase & operator>>( StreamBase & sb, ObjectColor & st )
{
    return sb >> st.first >> st.second;
}

StreamBase & operator>>( StreamBase & sb, ResourceCount & st )
{
    return sb >> st.first >> st.second;
}
