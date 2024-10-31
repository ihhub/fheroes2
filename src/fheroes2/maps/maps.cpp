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

#include "maps.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <ostream>

#include "ai_planner.h"
#include "direction.h"
#include "heroes.h"
#include "kingdom.h"
#include "logging.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "mp2.h"
#include "players.h"
#include "race.h"
#include "resource.h"
#include "translations.h"
#include "world.h"

namespace
{
    Maps::Indexes MapsIndexesFilteredObject( const Maps::Indexes & indexes, const MP2::MapObjectType objectType, const bool ignoreHeroes = true )
    {
        Maps::Indexes result;
        for ( size_t idx = 0; idx < indexes.size(); ++idx ) {
            if ( world.getTile( indexes[idx] ).getMainObjectType( !ignoreHeroes ) == objectType ) {
                result.push_back( indexes[idx] );
            }
        }
        return result;
    }

    Maps::Indexes MapsIndexesObject( const MP2::MapObjectType objectType, const bool ignoreHeroes )
    {
        Maps::Indexes result;
        const int32_t size = static_cast<int32_t>( world.getSize() );
        for ( int32_t idx = 0; idx < size; ++idx ) {
            if ( world.getTile( idx ).getMainObjectType( !ignoreHeroes ) == objectType ) {
                result.push_back( idx );
            }
        }
        return result;
    }
}

struct ComparisonDistance
{
    explicit ComparisonDistance( const int32_t index )
        : centerPoint( Maps::GetPoint( index ) )
    {}

    ComparisonDistance() = delete;

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

const char * Maps::SizeString( int s )
{
    switch ( s ) {
    case SMALL:
        return _( "maps|Small" );
    case MEDIUM:
        return _( "maps|Medium" );
    case LARGE:
        return _( "maps|Large" );
    case XLARGE:
        return _( "maps|Extra Large" );
    default:
        break;
    }

    return _( "maps|Custom Size" );
}

const char * Maps::GetMineName( const int resourceType )
{
    switch ( resourceType ) {
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

int32_t Maps::GetDirectionIndex( const int32_t from, const int direction )
{
    switch ( direction ) {
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

fheroes2::Point Maps::getDirectionPoint( const fheroes2::Point & from, const int direction )
{
    switch ( direction ) {
    case Direction::TOP:
        return { from.x, from.y - 1 };
    case Direction::TOP_RIGHT:
        return { from.x + 1, from.y - 1 };
    case Direction::RIGHT:
        return { from.x + 1, from.y };
    case Direction::BOTTOM_RIGHT:
        return { from.x + 1, from.y + 1 };
    case Direction::BOTTOM:
        return { from.x, from.y + 1 };
    case Direction::BOTTOM_LEFT:
        return { from.x - 1, from.y + 1 };
    case Direction::LEFT:
        return { from.x - 1, from.y };
    case Direction::TOP_LEFT:
        return { from.x - 1, from.y - 1 };
    case Direction::CENTER:
        return from;
    default:
        // This is not a valid direction.
        assert( 0 );
        break;
    }

    // This is equal to index = -1.
    return { -1, 0 };
}

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

Maps::Indexes Maps::getAroundIndexes( const int32_t tileIndex, const int32_t maxDistanceFromTile /* = 1 */ )
{
    Indexes results;

    if ( !isValidAbsIndex( tileIndex ) || maxDistanceFromTile <= 0 ) {
        return results;
    }

    const size_t areaSideSize = maxDistanceFromTile * 2 + 1;
    results.reserve( areaSideSize * areaSideSize - 1 );

    const int32_t worldWidth = world.w();
    const int32_t worldHeight = world.h();

    assert( worldWidth > 0 && worldHeight > 0 );

    const int32_t centerX = tileIndex % worldWidth;
    const int32_t centerY = tileIndex / worldWidth;

    // We avoid getting out of map boundaries.
    const int32_t minTileX = std::max( centerX - maxDistanceFromTile, 0 );
    const int32_t minTileY = std::max( centerY - maxDistanceFromTile, 0 );
    const int32_t maxTileX = std::min( centerX + maxDistanceFromTile + 1, worldWidth );
    const int32_t maxTileY = std::min( centerY + maxDistanceFromTile + 1, worldHeight );

    for ( int32_t tileY = minTileY; tileY < maxTileY; ++tileY ) {
        const int32_t indexOffsetY = tileY * worldWidth;
        const bool isCenterY = ( tileY == centerY );

        for ( int32_t tileX = minTileX; tileX < maxTileX; ++tileX ) {
            // Skip the center tile.
            if ( isCenterY && tileX == centerX ) {
                continue;
            }

            results.push_back( indexOffsetY + tileX );
        }
    }

    return results;
}

MapsIndexes Maps::getVisibleMonstersAroundHero( const Heroes & hero )
{
    const uint32_t dist = hero.GetVisionsDistance();
    MapsIndexes monsters = Maps::ScanAroundObjectWithDistance( hero.GetIndex(), dist, MP2::OBJ_MONSTER );

    const int32_t heroColor = hero.GetColor();
    monsters.erase( std::remove_if( monsters.begin(), monsters.end(), [heroColor]( const int32_t index ) { return world.getTile( index ).isFog( heroColor ); } ),
                    monsters.end() );
    return monsters;
}

void Maps::ClearFog( const int32_t tileIndex, const int scoutingDistance, const int playerColor )
{
    if ( scoutingDistance <= 0 || !Maps::isValidAbsIndex( tileIndex ) ) {
        // Nothing to uncover.
        return;
    }

    const Kingdom & kingdom = world.GetKingdom( playerColor );

    const bool isAIPlayer = kingdom.isControlAI();
    const bool isHumanOrHumanFriend = !isAIPlayer || Players::isFriends( playerColor, Players::HumanColors() );

    const fheroes2::Point center = Maps::GetPoint( tileIndex );
    const int revealRadiusSquared = scoutingDistance * scoutingDistance + 4; // constant factor for "backwards compatibility"
    const int alliedColors = Players::GetPlayerFriends( playerColor );

    const int32_t minY = std::max( center.y - scoutingDistance, 0 );
    const int32_t maxY = std::min( center.y + scoutingDistance, world.h() - 1 );
    assert( minY < maxY );

    const int32_t minX = std::max( center.x - scoutingDistance, 0 );
    const int32_t maxX = std::min( center.x + scoutingDistance, world.w() - 1 );
    assert( minX < maxX );

    fheroes2::Point fogRevealMinPos( world.h(), world.w() );
    fheroes2::Point fogRevealMaxPos( 0, 0 );

    for ( int32_t y = minY; y <= maxY; ++y ) {
        const int32_t dy = y - center.y;

        for ( int32_t x = minX; x <= maxX; ++x ) {
            const int32_t dx = x - center.x;
            if ( revealRadiusSquared >= dx * dx + dy * dy ) {
                Maps::Tile & tile = world.getTile( x, y );
                if ( isAIPlayer && tile.isFog( playerColor ) ) {
                    AI::Planner::Get().revealFog( tile, kingdom );
                }

                if ( tile.isFog( alliedColors ) ) {
                    // Clear fog only if it is not already cleared.
                    tile.ClearFog( alliedColors );

                    if ( isHumanOrHumanFriend ) {
                        // Update fog reveal area points only for human player and his allies.
                        fogRevealMinPos.x = std::min( fogRevealMinPos.x, x );
                        fogRevealMinPos.y = std::min( fogRevealMinPos.y, y );
                        fogRevealMaxPos.x = std::max( fogRevealMaxPos.x, x );
                        fogRevealMaxPos.y = std::max( fogRevealMaxPos.y, y );
                    }
                }
            }
        }
    }

    // Update fog directions only for human player and his allies and only if fog has to be cleared.
    if ( isHumanOrHumanFriend && ( fogRevealMaxPos.x >= fogRevealMinPos.x ) && ( fogRevealMaxPos.y >= fogRevealMinPos.y ) ) {
        // Fog directions should be updated 1 tile outside of the cleared fog.
        fogRevealMinPos -= { 1, 1 };
        fogRevealMaxPos += { 1, 1 };
        Maps::updateFogDirectionsInArea( fogRevealMinPos, fogRevealMaxPos, alliedColors );
    }
}

int32_t Maps::getFogTileCountToBeRevealed( const int32_t tileIndex, const int scoutingDistance, const int playerColor )
{
    if ( scoutingDistance <= 0 || !Maps::isValidAbsIndex( tileIndex ) ) {
        return 0;
    }

    const fheroes2::Point center = Maps::GetPoint( tileIndex );
    const int revealRadiusSquared = scoutingDistance * scoutingDistance + 4; // constant factor for "backwards compatibility"

    const int32_t minY = std::max( center.y - scoutingDistance, 0 );
    const int32_t maxY = std::min( center.y + scoutingDistance, world.h() - 1 );
    assert( minY < maxY );

    const int32_t minX = std::max( center.x - scoutingDistance, 0 );
    const int32_t maxX = std::min( center.x + scoutingDistance, world.w() - 1 );
    assert( minX < maxX );

    int32_t tileCount = 0;

    for ( int32_t y = minY; y <= maxY; ++y ) {
        const int32_t dy = y - center.y;

        for ( int32_t x = minX; x <= maxX; ++x ) {
            const int32_t dx = x - center.x;
            if ( revealRadiusSquared >= dx * dx + dy * dy ) {
                const Maps::Tile & tile = world.getTile( x, y );
                if ( tile.isFog( playerColor ) ) {
                    ++tileCount;
                }
            }
        }
    }

    return tileCount;
}

Maps::Indexes Maps::ScanAroundObject( const int32_t center, const MP2::MapObjectType objectType, const bool ignoreHeroes )
{
    Indexes results = getAroundIndexes( center );
    return MapsIndexesFilteredObject( results, objectType, ignoreHeroes );
}

bool Maps::isValidForDimensionDoor( int32_t targetIndex, bool isWater )
{
    const Maps::Tile & tile = world.getTile( targetIndex );
    return ( tile.GetPassable() & Direction::CENTER ) != 0 && isWater == tile.isWater() && !MP2::isInGameActionObject( tile.getMainObjectType( true ) );
}

Maps::Indexes Maps::ScanAroundObject( const int32_t center, const MP2::MapObjectType objectType )
{
    Indexes results = getAroundIndexes( center );
    return MapsIndexesFilteredObject( results, objectType );
}

Maps::Indexes Maps::ScanAroundObjectWithDistance( const int32_t center, const uint32_t dist, const MP2::MapObjectType objectType )
{
    Indexes results = getAroundIndexes( center, dist );
    std::sort( results.begin(), results.end(), ComparisonDistance( center ) );
    return MapsIndexesFilteredObject( results, objectType );
}

bool Maps::doesObjectExistOnMap( const MP2::MapObjectType objectType )
{
    const int32_t size = static_cast<int32_t>( world.getSize() );
    for ( int32_t idx = 0; idx < size; ++idx ) {
        if ( world.getTile( idx ).getMainObjectType( false ) == objectType ) {
            return true;
        }
    }

    return false;
}

Maps::Indexes Maps::GetObjectPositions( const MP2::MapObjectType objectType )
{
    return MapsIndexesObject( objectType, true );
}

std::vector<std::pair<int32_t, const Maps::ObjectPart *>> Maps::getObjectParts( const MP2::MapObjectType objectType )
{
    std::vector<std::pair<int32_t, const ObjectPart *>> result;
    const int32_t size = static_cast<int32_t>( world.getSize() );
    for ( int32_t idx = 0; idx < size; ++idx ) {
        const Maps::ObjectPart * objectPart = getObjectPartByActionType( world.getTile( idx ), objectType );
        if ( objectPart != nullptr ) {
            result.emplace_back( idx, objectPart );
        }
    }

    return result;
}

Maps::Indexes Maps::GetObjectPositions( int32_t center, const MP2::MapObjectType objectType, bool ignoreHeroes )
{
    Indexes results = MapsIndexesObject( objectType, ignoreHeroes );
    std::sort( results.begin(), results.end(), ComparisonDistance( center ) );
    return results;
}

bool Maps::isTileUnderProtection( const int32_t tileIndex )
{
    return world.getTile( tileIndex ).getMainObjectType() == MP2::OBJ_MONSTER ? true : !getMonstersProtectingTile( tileIndex ).empty();
}

Maps::Indexes Maps::getMonstersProtectingTile( const int32_t tileIndex, const bool checkObjectOnTile /* = true */ )
{
    if ( !isValidAbsIndex( tileIndex ) ) {
        return {};
    }

    Indexes result;

    const Maps::Tile & tile = world.getTile( tileIndex );

    // If a tile contains an object that you can interact with without visiting this tile, then this interaction doesn't trigger a monster attack...
    if ( checkObjectOnTile && MP2::isNeedStayFront( tile.getMainObjectType() ) ) {
        // ... unless the tile itself contains a monster
        if ( tile.getMainObjectType() == MP2::OBJ_MONSTER ) {
            result.push_back( tileIndex );
        }

        return result;
    }

    result.reserve( 9 );

    const int width = world.w();
    const int x = tileIndex % width;
    const int y = tileIndex / width;

    const auto isProtectedBy = [tileIndex, &tile]( const int32_t monsterTileIndex ) {
        const Maps::Tile & monsterTile = world.getTile( monsterTileIndex );

        if ( monsterTile.getMainObjectType() != MP2::OBJ_MONSTER || tile.isWater() != monsterTile.isWater() ) {
            return false;
        }

        const int directionToMonster = Maps::GetDirection( tileIndex, monsterTileIndex );
        const int directionFromMonster = Direction::Reflect( directionToMonster );

        // The tile is directly accessible to the monster
        if ( ( tile.GetPassable() & directionToMonster ) && ( monsterTile.GetPassable() & directionFromMonster ) ) {
            return true;
        }

        // The tile is not directly accessible to the monster, but he can still attack in the diagonal direction if, when the hero moves away from the tile
        // in question in the vertical direction and the monster moves away from his tile in the horizontal direction, they would have to meet
        if ( directionFromMonster == Direction::TOP_LEFT && ( tile.GetPassable() & Direction::BOTTOM ) && ( monsterTile.GetPassable() & Direction::LEFT ) ) {
            return true;
        }
        if ( directionFromMonster == Direction::TOP_RIGHT && ( tile.GetPassable() & Direction::BOTTOM ) && ( monsterTile.GetPassable() & Direction::RIGHT ) ) {
            return true;
        }
        if ( directionFromMonster == Direction::BOTTOM_RIGHT && ( tile.GetPassable() & Direction::TOP ) && ( monsterTile.GetPassable() & Direction::RIGHT ) ) {
            return true;
        }
        if ( directionFromMonster == Direction::BOTTOM_LEFT && ( tile.GetPassable() & Direction::TOP ) && ( monsterTile.GetPassable() & Direction::LEFT ) ) {
            return true;
        }

        return false;
    };

    const auto validateAndAdd = [&result, &isProtectedBy]( const int monsterTileIndex ) {
        if ( isProtectedBy( monsterTileIndex ) ) {
            result.push_back( monsterTileIndex );
        }
    };

    if ( y > 0 ) {
        if ( x > 0 ) {
            validateAndAdd( tileIndex - width - 1 );
        }

        validateAndAdd( tileIndex - width );

        if ( x < width - 1 ) {
            validateAndAdd( tileIndex - width + 1 );
        }
    }

    if ( x > 0 ) {
        validateAndAdd( tileIndex - 1 );
    }

    if ( tile.getMainObjectType() == MP2::OBJ_MONSTER ) {
        result.push_back( tileIndex );
    }

    if ( x < width - 1 ) {
        validateAndAdd( tileIndex + 1 );
    }

    if ( y < world.h() - 1 ) {
        if ( x > 0 ) {
            validateAndAdd( tileIndex + width - 1 );
        }

        validateAndAdd( tileIndex + width );

        if ( x < width - 1 ) {
            validateAndAdd( tileIndex + width + 1 );
        }
    }

    return result;
}

uint32_t Maps::GetApproximateDistance( const int32_t pos1, const int32_t pos2 )
{
    const fheroes2::Point point1( GetPoint( pos1 ) );
    const fheroes2::Point point2( GetPoint( pos2 ) );

    const uint32_t diffX = std::abs( point1.x - point2.x );
    const uint32_t diffY = std::abs( point1.y - point2.y );

    assert( diffX < Maps::XLARGE && diffY < Maps::XLARGE );

    return std::max( diffX, diffY ) + std::min( diffX, diffY ) / 2;
}

uint32_t Maps::GetStraightLineDistance( const int32_t pos1, const int32_t pos2 )
{
    const fheroes2::Point point1( GetPoint( pos1 ) );
    const fheroes2::Point point2( GetPoint( pos2 ) );

    const uint32_t diffX = std::abs( point1.x - point2.x );
    const uint32_t diffY = std::abs( point1.y - point2.y );

    assert( diffX < Maps::XLARGE && diffY < Maps::XLARGE );

    return static_cast<uint32_t>( std::hypot( diffX, diffY ) );
}

void Maps::ReplaceRandomCastleObjectId( const fheroes2::Point & center )
{
    // Reset castle ID
    for ( int32_t y = -3; y < 2; ++y ) {
        for ( int32_t x = -2; x < 3; ++x ) {
            Maps::Tile & tile = world.getTile( center.x + x, center.y + y );

            if ( MP2::OBJ_NON_ACTION_RANDOM_CASTLE == tile.getMainObjectType() || MP2::OBJ_NON_ACTION_RANDOM_TOWN == tile.getMainObjectType() ) {
                tile.setMainObjectType( MP2::OBJ_NON_ACTION_CASTLE );
            }
        }
    }

    // restore center ID
    world.getTile( center.x, center.y ).setMainObjectType( MP2::OBJ_CASTLE );
}

void Maps::UpdateCastleSprite( const fheroes2::Point & center, int race, bool isCastle, bool isRandom )
{
    /*
    Castle/Town object image consists of 42 tile sprites:
    10 base tiles (OBJNTWBA) with 16 shadow tiles on left side (OBJNTWSH) overlaid by 16 town tiles (OBJNTOWN)

    Shadows (OBJNTWSH)  Castle (OBJNTOWN)
                              0
       32 33 34 35      1  2  3  4  5
    36 37 38 39 40      6  7  8  9 10
       41 42 43 44     11 12 13 14 15
          45 46 47
    */

    // correct only RND town and castle
    const Maps::Tile & entranceTile = world.getTile( center.x, center.y );
    const MP2::MapObjectType objectType = entranceTile.getMainObjectType();
    const uint32_t castleID = entranceTile.getMainObjectPart()._uid;

    if ( isRandom && ( objectType != MP2::OBJ_RANDOM_CASTLE && objectType != MP2::OBJ_RANDOM_TOWN ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN,
                   "incorrect object"
                       << ", index: " << GetIndexFromAbsPoint( center.x, center.y ) )
        return;
    }

    uint8_t raceIndex = 0; // Race::KNIGHT
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

    const int castleCoordinates[16][2] = { { 0, -3 }, { -2, -2 }, { -1, -2 }, { 0, -2 }, { 1, -2 }, { 2, -2 }, { -2, -1 }, { -1, -1 },
                                           { 0, -1 }, { 1, -1 },  { 2, -1 },  { -2, 0 }, { -1, 0 }, { 0, 0 },  { 1, 0 },   { 2, 0 } };
    const int shadowCoordinates[16][2] = { { -4, -2 }, { -3, -2 }, { -2, -2 }, { -1, -2 }, { -5, -1 }, { -4, -1 }, { -3, -1 }, { -2, -1 },
                                           { -1, -1 }, { -4, 0 },  { -3, 0 },  { -2, 0 },  { -1, 0 },  { -3, 1 },  { -2, 1 },  { -1, 1 } };

    const uint8_t indexOffset = isCastle ? 0 : 16;

    for ( uint8_t index = 0; index < 16; ++index ) {
        const uint8_t fullTownIndex = index + indexOffset + raceIndex * 32;
        const uint8_t lookupID = isRandom ? index + indexOffset : fullTownIndex;

        const int castleTile = GetIndexFromAbsPoint( center.x + castleCoordinates[index][0], center.y + castleCoordinates[index][1] );
        if ( isValidAbsIndex( castleTile ) ) {
            Tile & tile = world.getTile( castleTile );

            if ( isRandom )
                tile.replaceObject( castleID, MP2::OBJ_ICN_TYPE_OBJNTWRD, MP2::OBJ_ICN_TYPE_OBJNTOWN, lookupID, fullTownIndex ); // OBJNTWRD to OBJNTOWN
            else
                tile.updateObjectImageIndex( castleID, MP2::OBJ_ICN_TYPE_OBJNTOWN, -16 );

            if ( index == 0 ) {
                ObjectPart * part = tile.getTopObjectPart( castleID );
                if ( part && part->icnType == MP2::OBJ_ICN_TYPE_OBJNTWRD ) {
                    part->icnType = MP2::OBJ_ICN_TYPE_OBJNTOWN;
                    part->icnIndex = fullTownIndex - 16;
                }
            }
        }

        const int shadowTileId = GetIndexFromAbsPoint( center.x + shadowCoordinates[index][0], center.y + shadowCoordinates[index][1] );
        if ( isValidAbsIndex( shadowTileId ) ) {
            Maps::Tile & shadowTile = world.getTile( shadowTileId );
            if ( isRandom )
                shadowTile.replaceObject( castleID, MP2::OBJ_ICN_TYPE_OBJNTWRD, MP2::OBJ_ICN_TYPE_OBJNTWSH, lookupID + 32, fullTownIndex );
            else
                shadowTile.updateObjectImageIndex( castleID, MP2::OBJ_ICN_TYPE_OBJNTWSH, -16 );
        }
    }
}
