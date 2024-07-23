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

#include "world.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <memory>
#include <ostream>
#include <set>
#include <tuple>
#include <type_traits>
#include <utility>

#include "ai_planner.h"
#include "artifact.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "castle.h"
#include "color.h"
#include "direction.h"
#include "game.h"
#include "game_io.h"
#include "game_over.h"
#include "gamedefs.h"
#include "ground.h"
#include "heroes.h"
#include "logging.h"
#include "maps_fileinfo.h"
#include "maps_objects.h"
#include "maps_tiles_helper.h"
#include "mp2.h"
#include "pairs.h"
#include "players.h"
#include "race.h"
#include "rand.h"
#include "resource.h"
#include "route.h"
#include "save_format_version.h"
#include "serialize.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "week.h"
#include "world_object_uid.h"

namespace
{
    bool isTileBlockedForSettingMonster( const std::vector<Maps::Tiles> & mapTiles, const int32_t tileId, const int32_t radius, const std::set<int32_t> & excludeTiles )
    {
        const MapsIndexes & indexes = Maps::getAroundIndexes( tileId, radius );

        for ( const int32_t indexId : indexes ) {
            if ( excludeTiles.count( indexId ) > 0 ) {
                return true;
            }

            const Maps::Tiles & indexedTile = mapTiles[indexId];
            if ( indexedTile.isWater() ) {
                continue;
            }

            const MP2::MapObjectType objectType = indexedTile.GetObject( true );
            if ( objectType == MP2::OBJ_CASTLE || objectType == MP2::OBJ_HERO || objectType == MP2::OBJ_MONSTER ) {
                return true;
            }
        }

        return false;
    }

    int32_t findSuitableNeighbouringTile( const std::vector<Maps::Tiles> & mapTiles, const int32_t tileId, const bool allDirections )
    {
        std::vector<int32_t> suitableIds;

        const MapsIndexes & indexes = Maps::getAroundIndexes( tileId );

        for ( const int32_t indexId : indexes ) {
            // If allDirections is false, we should only consider tiles below the current object
            if ( !allDirections && indexId < tileId + world.w() - 2 ) {
                continue;
            }

            const Maps::Tiles & indexedTile = mapTiles[indexId];

            if ( indexedTile.isWater() || !isClearGround( indexedTile ) ) {
                continue;
            }

            // If the candidate tile is a coast tile, it is suitable only if there are other coast tiles nearby
            if ( indexedTile.GetObject( false ) == MP2::OBJ_COAST ) {
                const MapsIndexes coastTiles = Maps::ScanAroundObject( indexId, MP2::OBJ_COAST );

                if ( coastTiles.empty() ) {
                    continue;
                }
            }

            suitableIds.emplace_back( indexId );
        }

        if ( suitableIds.empty() ) {
            return -1;
        }

        return Rand::Get( suitableIds );
    }

    int32_t getNeighbouringEmptyTileCount( const std::vector<Maps::Tiles> & mapTiles, const int32_t tileId )
    {
        int32_t count = 0;

        const MapsIndexes & indexes = Maps::getAroundIndexes( tileId );

        for ( const int32_t indexId : indexes ) {
            const Maps::Tiles & indexedTile = mapTiles[indexId];
            if ( indexedTile.isWater() || !isClearGround( indexedTile ) ) {
                continue;
            }

            ++count;
        }

        return count;
    }
}

MapObjects::~MapObjects()
{
    clear();
}

void MapObjects::clear()
{
    for ( iterator it = begin(); it != end(); ++it )
        delete ( *it ).second;
    std::map<uint32_t, MapObjectSimple *>::clear();
}

void MapObjects::add( MapObjectSimple * obj )
{
    if ( obj ) {
        std::map<uint32_t, MapObjectSimple *> & currentMap = *this;
        if ( currentMap[obj->GetUID()] )
            delete currentMap[obj->GetUID()];
        currentMap[obj->GetUID()] = obj;
    }
}

MapObjectSimple * MapObjects::get( uint32_t uid )
{
    iterator it = find( uid );
    return it != end() ? ( *it ).second : nullptr;
}

std::list<MapObjectSimple *> MapObjects::get( const fheroes2::Point & pos )
{
    std::list<MapObjectSimple *> res;
    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it ).second && ( *it ).second->isPosition( pos ) )
            res.push_back( ( *it ).second );
    return res;
}

void MapObjects::remove( uint32_t uid )
{
    iterator it = find( uid );
    if ( it != end() )
        delete ( *it ).second;
    erase( it );
}

CapturedObject & CapturedObjects::Get( int32_t index )
{
    std::map<int32_t, CapturedObject> & my = *this;
    return my[index];
}

void CapturedObjects::SetColor( int32_t index, int col )
{
    Get( index ).SetColor( col );
}

void CapturedObjects::Set( int32_t index, int obj, int col )
{
    CapturedObject & co = Get( index );

    if ( co.GetColor() != col && co.guardians.isValid() )
        co.guardians.Reset();

    co.Set( obj, col );
}

uint32_t CapturedObjects::GetCount( int obj, int col ) const
{
    uint32_t result = 0;

    const ObjectColor objcol( obj, col );

    for ( const_iterator it = begin(); it != end(); ++it ) {
        if ( objcol == ( *it ).second.objcol )
            ++result;
    }

    return result;
}

uint32_t CapturedObjects::GetCountMines( const int resourceType, const int ownerColor ) const
{
    uint32_t count = 0;
    const ObjectColor correctObject( MP2::OBJ_MINE, ownerColor );

    for ( const auto & [tileIndex, objectInfo] : *this ) {
        if ( correctObject == objectInfo.objcol ) {
            const int32_t mineResource = Maps::getDailyIncomeObjectResources( world.GetTiles( tileIndex ) ).getFirstValidResource().first;
            if ( resourceType == mineResource ) {
                ++count;
            }
        }
    }

    return count;
}

int CapturedObjects::GetColor( int32_t index ) const
{
    const_iterator it = find( index );
    return it != end() ? ( *it ).second.GetColor() : Color::NONE;
}

void CapturedObjects::ClearFog( int colors )
{
    // clear abroad objects
    for ( const_iterator it = begin(); it != end(); ++it ) {
        const ObjectColor & objcol = ( *it ).second.objcol;

        if ( objcol.isColor( colors ) ) {
            int scoutingDistance = 0;

            switch ( objcol.first ) {
            case MP2::OBJ_MINE:
            case MP2::OBJ_ALCHEMIST_LAB:
            case MP2::OBJ_SAWMILL:
                scoutingDistance = 2;
                break;

            default:
                break;
            }

            if ( scoutingDistance )
                Maps::ClearFog( ( *it ).first, scoutingDistance, colors );
        }
    }
}

void CapturedObjects::ResetColor( int color )
{
    for ( iterator it = begin(); it != end(); ++it ) {
        ObjectColor & objcol = ( *it ).second.objcol;

        if ( objcol.isColor( color ) ) {
            const MP2::MapObjectType objectType = static_cast<MP2::MapObjectType>( objcol.first );

            objcol.second = objectType == MP2::OBJ_CASTLE ? Color::UNUSED : Color::NONE;
            world.GetTiles( it->first ).setOwnershipFlag( objectType, objcol.second );
        }
    }
}

World & world = World::Get();

World & World::Get()
{
    static World insideWorld;

    return insideWorld;
}

void World::Defaults()
{
    // playing kingdom
    vec_kingdoms.Init();

    // Map seed is random and persisted on saves
    // this has to be generated before initializing heroes, as campaign-specific heroes start at a higher level and thus have to simulate level ups
    _seed = Rand::Get( std::numeric_limits<uint32_t>::max() );

    // initialize all heroes
    vec_heroes.Init();

    vec_castles.Init();
}

void World::Reset()
{
    width = 0;
    height = 0;

    // maps tiles
    vec_tiles.clear();

    // kingdoms
    vec_kingdoms.clear();

    // event day
    vec_eventsday.clear();

    // rumors
    _customRumors.clear();

    // castles
    vec_castles.Clear();

    // heroes
    vec_heroes.clear();

    // extra
    map_captureobj.clear();
    map_objects.clear();

    ultimate_artifact.Reset();

    day = 0;
    week = 0;
    month = 0;

    heroIdAsWinCondition = Heroes::UNKNOWN;
    heroIdAsLossCondition = Heroes::UNKNOWN;

    _seed = 0;
}

void World::generateBattleOnlyMap()
{
    const std::vector<int> terrainTypes{ Maps::Ground::DESERT, Maps::Ground::SNOW, Maps::Ground::SWAMP, Maps::Ground::WASTELAND, Maps::Ground::BEACH,
                                         Maps::Ground::LAVA,   Maps::Ground::DIRT, Maps::Ground::GRASS, Maps::Ground::WATER };

    Reset();

    width = 2;
    height = 2;

    Maps::FileInfo fi;

    fi.width = static_cast<uint16_t>( width );
    fi.height = static_cast<uint16_t>( height );

    Settings & conf = Settings::Get();

    if ( conf.isPriceOfLoyaltySupported() ) {
        fi.version = GameVersion::PRICE_OF_LOYALTY;
    }

    conf.SetCurrentFileInfo( std::move( fi ) );

    Defaults();

    vec_tiles.resize( static_cast<size_t>( width ) * height );

    const int groundType = Rand::Get( terrainTypes );

    for ( size_t i = 0; i < vec_tiles.size(); ++i ) {
        vec_tiles[i] = {};

        vec_tiles[i].setIndex( static_cast<int32_t>( i ) );
        vec_tiles[i].setTerrain( Maps::Ground::getTerrainStartImageIndex( groundType ), false, false );
    }
}

void World::generateForEditor( const int32_t size )
{
    assert( size > 0 );

    Reset();

    width = size;
    height = size;

    Maps::FileInfo fi;

    fi.width = static_cast<uint16_t>( width );
    fi.height = static_cast<uint16_t>( height );

    Settings & conf = Settings::Get();
    assert( conf.isPriceOfLoyaltySupported() );

    fi.version = GameVersion::PRICE_OF_LOYALTY;

    conf.SetCurrentFileInfo( std::move( fi ) );

    Defaults();

    vec_tiles.resize( static_cast<size_t>( width ) * height );

    // init all tiles
    for ( size_t i = 0; i < vec_tiles.size(); ++i ) {
        vec_tiles[i] = {};

        vec_tiles[i].setIndex( static_cast<int32_t>( i ) );

        const uint8_t terrainFlag = static_cast<uint8_t>( Rand::Get( 0, 3 ) );
        vec_tiles[i].setTerrain( static_cast<uint16_t>( Rand::Get( 16, 19 ) ), terrainFlag & 1, terrainFlag & 2 );
    }
}

const Castle * World::getCastleEntrance( const fheroes2::Point & tilePosition ) const
{
    if ( !isValidCastleEntrance( tilePosition ) ) {
        return nullptr;
    }

    return vec_castles.Get( tilePosition );
}

Castle * World::getCastleEntrance( const fheroes2::Point & tilePosition )
{
    if ( !isValidCastleEntrance( tilePosition ) ) {
        return nullptr;
    }

    return vec_castles.Get( tilePosition );
}

bool World::isValidCastleEntrance( const fheroes2::Point & tilePosition ) const
{
    return Maps::isValidAbsPoint( tilePosition.x, tilePosition.y ) && ( GetTiles( tilePosition.x, tilePosition.y ).GetObject( false ) == MP2::OBJ_CASTLE );
}

Heroes * World::GetHeroForHire( const int race, const int heroIDToIgnore /* = Heroes::UNKNOWN */ ) const
{
    return vec_heroes.GetHeroForHire( race, heroIDToIgnore );
}

Heroes * World::FromJailHeroes( int32_t index )
{
    return vec_heroes.FromJail( index );
}

Heroes * World::GetHero( const Castle & castle ) const
{
    return vec_heroes.Get( castle.GetCenter() );
}

int World::GetDay() const
{
    return LastDay() ? DAYOFWEEK : day % DAYOFWEEK;
}

int World::GetWeek() const
{
    return LastWeek() ? WEEKOFMONTH : week % WEEKOFMONTH;
}

bool World::BeginWeek() const
{
    return 1 == ( day % DAYOFWEEK );
}

bool World::BeginMonth() const
{
    return 1 == ( week % WEEKOFMONTH ) && BeginWeek();
}

bool World::LastDay() const
{
    return ( 0 == ( day % DAYOFWEEK ) );
}

bool World::FirstWeek() const
{
    return ( 1 == ( week % WEEKOFMONTH ) );
}

bool World::LastWeek() const
{
    return ( 0 == ( week % WEEKOFMONTH ) );
}

const Week & World::GetWeekType() const
{
    static auto cachedWeekDependencies = std::make_tuple( week, GetWeekSeed() );
    static Week cachedWeek = Week::RandomWeek( FirstWeek(), GetWeekSeed() );

    const auto currentWeekDependencies = std::make_tuple( week, GetWeekSeed() );

    if ( cachedWeekDependencies != currentWeekDependencies ) {
        cachedWeekDependencies = currentWeekDependencies;
        cachedWeek = Week::RandomWeek( FirstWeek(), GetWeekSeed() );
    }

    return cachedWeek;
}

void World::NewDay()
{
    ++day;

    if ( BeginWeek() ) {
        ++week;

        if ( BeginMonth() ) {
            ++month;
        }
    }

    // first the routine of the new month
    if ( BeginMonth() ) {
        NewMonth();

        vec_kingdoms.NewMonth();
        vec_castles.NewMonth();
        vec_heroes.NewMonth();
    }

    // then the routine of the new week
    if ( BeginWeek() ) {
        NewWeek();

        vec_kingdoms.NewWeek();
        vec_castles.NewWeek();
        vec_heroes.NewWeek();
    }

    // and finally the routine of the new day
    vec_kingdoms.NewDay();
    vec_castles.NewDay();
    vec_heroes.NewDay();

    // remove deprecated events
    assert( day > 0 );

    vec_eventsday.remove_if( [this]( const EventDate & v ) { return v.isDeprecated( day - 1 ); } );
}

void World::NewWeek()
{
    // update objects
    if ( week > 1 ) {
        for ( Maps::Tiles & tile : vec_tiles ) {
            if ( MP2::isWeekLife( tile.GetObject( false ) ) || tile.GetObject() == MP2::OBJ_MONSTER ) {
                updateObjectInfoTile( tile, false );
            }
        }
    }

    // Reset RECRUIT mode for all heroes at once
    vec_heroes.ResetModes( Heroes::RECRUIT );

    // Reset recruits in all kingdoms at once
    std::set<Heroes *> remainingRecruits = vec_kingdoms.resetRecruits();

    // Restore the RECRUIT mode for the remaining recruits
    for ( Heroes * hero : remainingRecruits ) {
        assert( hero != nullptr );

        hero->SetModes( Heroes::RECRUIT );
    }
}

void World::NewMonth()
{
    if ( month > 1 && GetWeekType().GetType() == WeekName::MONSTERS ) {
        MonthOfMonstersAction( Monster( GetWeekType().GetMonster() ) );
    }
}

void World::MonthOfMonstersAction( const Monster & mons )
{
    if ( !mons.isValid() ) {
        return;
    }

    // Find all tiles which are useful for monsters such as resources, artifacts, mines, other capture objects. Exclude heroes, monsters and castles.
    std::vector<int32_t> primaryTargetTiles;

    // Sometimes monsters appear on roads so find all road tiles.
    std::vector<int32_t> secondaryTargetTiles;

    // Lastly monster occasionally appear on empty tiles.
    std::vector<int32_t> tetriaryTargetTiles;

    std::set<int32_t> excludeTiles;

    for ( const Maps::Tiles & tile : vec_tiles ) {
        if ( tile.isWater() ) {
            // Monsters are not placed on water.
            continue;
        }

        const int32_t tileId = tile.GetIndex();
        const MP2::MapObjectType objectType = tile.GetObject( true );

        if ( objectType == MP2::OBJ_CASTLE || objectType == MP2::OBJ_HERO || objectType == MP2::OBJ_MONSTER ) {
            excludeTiles.emplace( tileId );
            continue;
        }

        if ( MP2::isInGameActionObject( objectType ) ) {
            if ( isTileBlockedForSettingMonster( vec_tiles, tileId, 3, excludeTiles ) ) {
                continue;
            }

            const int32_t tileToSet = findSuitableNeighbouringTile( vec_tiles, tileId, ( tile.GetPassable() == DIRECTION_ALL ) );
            if ( tileToSet >= 0 ) {
                primaryTargetTiles.emplace_back( tileToSet );
                excludeTiles.emplace( tileId );
            }
        }
        else if ( tile.isRoad() ) {
            if ( isTileBlockedForSettingMonster( vec_tiles, tileId, 4, excludeTiles ) ) {
                continue;
            }

            if ( getNeighbouringEmptyTileCount( vec_tiles, tileId ) < 2 ) {
                continue;
            }

            const int32_t tileToSet = findSuitableNeighbouringTile( vec_tiles, tileId, true );
            if ( tileToSet >= 0 ) {
                secondaryTargetTiles.emplace_back( tileToSet );
                excludeTiles.emplace( tileId );
            }
        }
        else if ( isClearGround( tile ) ) {
            if ( isTileBlockedForSettingMonster( vec_tiles, tileId, 4, excludeTiles ) ) {
                continue;
            }

            if ( getNeighbouringEmptyTileCount( vec_tiles, tileId ) < 4 ) {
                continue;
            }

            const int32_t tileToSet = findSuitableNeighbouringTile( vec_tiles, tileId, true );
            if ( tileToSet >= 0 ) {
                tetriaryTargetTiles.emplace_back( tileToSet );
                excludeTiles.emplace( tileId );
            }
        }
    }

    // Shuffle all found tile IDs.
    Rand::Shuffle( primaryTargetTiles );
    Rand::Shuffle( secondaryTargetTiles );
    Rand::Shuffle( tetriaryTargetTiles );

    // Calculate the number of monsters to be placed.
    uint32_t monstersToBePlaced = static_cast<uint32_t>( primaryTargetTiles.size() / 3 );
    const uint32_t mapMinimum = static_cast<uint32_t>( vec_tiles.size() / 360 );

    if ( monstersToBePlaced < mapMinimum ) {
        monstersToBePlaced = mapMinimum;
    }
    else {
        monstersToBePlaced = Rand::GetWithSeed( monstersToBePlaced * 75 / 100, monstersToBePlaced * 125 / 100, _seed );
    }

    // 85% of positions are for primary targets
    // 10% of positions are for roads
    // 5% of positions are for empty tiles
    uint32_t primaryTileCount = monstersToBePlaced * 85 / 100;
    if ( primaryTileCount > primaryTargetTiles.size() ) {
        primaryTileCount = static_cast<uint32_t>( primaryTargetTiles.size() );
    }

    for ( uint32_t i = 0; i < primaryTileCount; ++i ) {
        setMonsterOnTile( vec_tiles[primaryTargetTiles[i]], mons, 0 /* random */ );
    }

    uint32_t secondaryTileCount = monstersToBePlaced * 10 / 100;
    if ( secondaryTileCount > secondaryTargetTiles.size() ) {
        secondaryTileCount = static_cast<uint32_t>( secondaryTargetTiles.size() );
    }

    for ( uint32_t i = 0; i < secondaryTileCount; ++i ) {
        setMonsterOnTile( vec_tiles[secondaryTargetTiles[i]], mons, 0 /* random */ );
    }

    uint32_t tetriaryTileCount = monstersToBePlaced * 5 / 100;
    if ( tetriaryTileCount > tetriaryTargetTiles.size() ) {
        tetriaryTileCount = static_cast<uint32_t>( tetriaryTargetTiles.size() );
    }

    for ( uint32_t i = 0; i < tetriaryTileCount; ++i ) {
        setMonsterOnTile( vec_tiles[tetriaryTargetTiles[i]], mons, 0 /* random */ );
    }
}

std::string World::getCurrentRumor() const
{
    const uint32_t standardRumorCount = 10;
    const uint32_t totalRumorCount = static_cast<uint32_t>( _customRumors.size() ) + standardRumorCount;
    const uint32_t chosenRumorId = Rand::GetWithSeed( 0, totalRumorCount - 1, GetWeekSeed() );

    switch ( chosenRumorId ) {
    case 0: {
        std::string rumor( _( "The ultimate artifact is really the %{name}." ) );
        StringReplace( rumor, "%{name}", ultimate_artifact.GetName() );
        return rumor;
    }
    case 1: {
        std::string rumor( _( "The ultimate artifact may be found in the %{name} regions of the world." ) );
        const int32_t artifactIndex = ultimate_artifact.getPosition();
        const fheroes2::Point artifactPos = Maps::GetPoint( artifactIndex );

        if ( height / 3 > artifactPos.y ) {
            if ( width / 3 > artifactPos.x ) {
                StringReplace( rumor, "%{name}", _( "north-west" ) );
            }
            else if ( 2 * width / 3 > artifactPos.x ) {
                StringReplace( rumor, "%{name}", _( "north" ) );
            }
            else {
                StringReplace( rumor, "%{name}", _( "north-east" ) );
            }
        }
        else if ( 2 * height / 3 > artifactPos.y ) {
            if ( width / 3 > artifactPos.x ) {
                StringReplace( rumor, "%{name}", _( "west" ) );
            }
            else if ( 2 * width / 3 > artifactPos.x ) {
                StringReplace( rumor, "%{name}", _( "center" ) );
            }
            else {
                StringReplace( rumor, "%{name}", _( "east" ) );
            }
        }
        else if ( width / 3 > artifactPos.x ) {
            StringReplace( rumor, "%{name}", _( "south-west" ) );
        }
        else if ( 2 * width / 3 > artifactPos.x ) {
            StringReplace( rumor, "%{name}", _( "south" ) );
        }
        else {
            StringReplace( rumor, "%{name}", _( "south-east" ) );
        }
        return rumor;
    }
    case 2:
        return _( "The truth is out there." );
    case 3:
        return _( "The dark side is stronger." );
    case 4:
        return _( "The end of the world is near." );
    case 5:
        return _( "The bones of Lord Slayer are buried in the foundation of the arena." );
    case 6:
        return _( "A Black Dragon will take out a Titan any day of the week." );
    case 7:
        return _( "He told her: Yada yada yada... and then she said: Blah, blah, blah..." );
    case 8:
        return _( "An unknown force is being resurrected..." );
    case 9:
        return _( "Check the newest version of the game at\nhttps://github.com/ihhub/\nfheroes2/releases" );
    default:
        break;
    }

    assert( chosenRumorId >= standardRumorCount && chosenRumorId < totalRumorCount );
    return _customRumors[chosenRumorId - standardRumorCount];
}

MapsIndexes World::GetTeleportEndPoints( const int32_t index ) const
{
    MapsIndexes result;

    const Maps::Tiles & entranceTile = GetTiles( index );

    if ( entranceTile.GetObject( false ) != MP2::OBJ_STONE_LITHS ) {
        return result;
    }

    // The type of destination stone liths must match the type of the source stone liths.
    for ( const int32_t teleportIndex : _allTeleports.at( entranceTile.GetObjectSpriteIndex() ) ) {
        const Maps::Tiles & teleportTile = GetTiles( teleportIndex );

        if ( teleportIndex == index || teleportTile.getHero() != nullptr || teleportTile.isWater() != entranceTile.isWater() ) {
            continue;
        }

        result.push_back( teleportIndex );
    }

    return result;
}

int32_t World::NextTeleport( const int32_t index ) const
{
    const MapsIndexes teleports = GetTeleportEndPoints( index );
    if ( teleports.empty() ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "not found" )
        return index;
    }

    return Rand::Get( teleports );
}

MapsIndexes World::GetWhirlpoolEndPoints( const int32_t index ) const
{
    MapsIndexes result;

    const Maps::Tiles & entranceTile = GetTiles( index );

    if ( entranceTile.GetObject( false ) != MP2::OBJ_WHIRLPOOL ) {
        return result;
    }

    // The exit point from the destination whirlpool must match the entry point in the source whirlpool.
    for ( const int32_t whirlpoolIndex : _allWhirlpools.at( entranceTile.GetObjectSpriteIndex() ) ) {
        const Maps::Tiles & whirlpoolTile = GetTiles( whirlpoolIndex );

        if ( whirlpoolTile.GetObjectUID() == entranceTile.GetObjectUID() || whirlpoolTile.getHero() != nullptr ) {
            continue;
        }

        result.push_back( whirlpoolIndex );
    }

    return result;
}

int32_t World::NextWhirlpool( const int32_t index ) const
{
    const MapsIndexes whilrpools = GetWhirlpoolEndPoints( index );
    if ( whilrpools.empty() ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "not found" )
        return index;
    }

    return Rand::Get( whilrpools );
}

uint32_t World::CountCapturedObject( int obj, int col ) const
{
    return map_captureobj.GetCount( obj, col );
}

uint32_t World::CountCapturedMines( int type, int color ) const
{
    switch ( type ) {
    case Resource::WOOD:
        return CountCapturedObject( MP2::OBJ_SAWMILL, color );
    case Resource::MERCURY:
        return CountCapturedObject( MP2::OBJ_ALCHEMIST_LAB, color );
    default:
        break;
    }

    return map_captureobj.GetCountMines( type, color );
}

void World::CaptureObject( int32_t index, int color )
{
    const MP2::MapObjectType objectType = GetTiles( index ).GetObject( false );
    map_captureobj.Set( index, objectType, color );

    Castle * castle = getCastleEntrance( Maps::GetPoint( index ) );
    if ( castle && castle->GetColor() != color ) {
        castle->ChangeColor( color );
    }

    if ( color & ( Color::ALL | Color::UNUSED ) ) {
        GetTiles( index ).setOwnershipFlag( objectType, color );
    }
}

int World::ColorCapturedObject( int32_t index ) const
{
    return map_captureobj.GetColor( index );
}

CapturedObject & World::GetCapturedObject( int32_t index )
{
    return map_captureobj.Get( index );
}

void World::ResetCapturedObjects( int color )
{
    map_captureobj.ResetColor( color );
}

void World::ClearFog( int colors )
{
    colors = Players::GetPlayerFriends( colors );

    // clear abroad castles
    vec_castles.Scout( colors );

    // clear abroad heroes
    vec_heroes.Scout( colors );

    map_captureobj.ClearFog( colors );
}

const UltimateArtifact & World::GetUltimateArtifact() const
{
    return ultimate_artifact;
}

bool World::DiggingForUltimateArtifact( const fheroes2::Point & center )
{
    Maps::Tiles & tile = GetTiles( center.x, center.y );

    // Get digging hole sprite.
    MP2::ObjectIcnType objectIcnType = MP2::OBJ_ICN_TYPE_UNKNOWN;
    uint8_t imageIndex = 0;

    if ( !MP2::getDiggingHoleSprite( tile.GetGround(), objectIcnType, imageIndex ) ) {
        // Are you sure that you can dig here?
        assert( 0 );

        return false;
    }

    tile.pushBottomLayerAddon( Maps::TilesAddon( Maps::BACKGROUND_LAYER, Maps::getNewObjectUID(), objectIcnType, imageIndex ) );

    if ( ultimate_artifact.isPosition( tile.GetIndex() ) && !ultimate_artifact.isFound() ) {
        ultimate_artifact.markAsFound();
        return true;
    }

    return false;
}

void World::AddEventDate( const EventDate & event )
{
    vec_eventsday.push_back( event );
}

EventsDate World::GetEventsDate( int color ) const
{
    EventsDate res;

    for ( EventsDate::const_iterator it = vec_eventsday.begin(); it != vec_eventsday.end(); ++it )
        if ( ( *it ).isAllow( color, day ) )
            res.push_back( *it );

    return res;
}

std::string World::DateString() const
{
    std::string output( "month: " );
    output += std::to_string( GetMonth() );
    output += ", week: ";
    output += std::to_string( GetWeek() );
    output += ", day: ";
    output += std::to_string( GetDay() );

    return output;
}

uint32_t World::CountObeliskOnMaps()
{
    const size_t res = std::count_if( vec_tiles.begin(), vec_tiles.end(), []( const Maps::Tiles & tile ) { return MP2::OBJ_OBELISK == tile.GetObject( false ); } );
    return res > 0 ? static_cast<uint32_t>( res ) : 6;
}

void World::ActionForMagellanMaps( int color )
{
    const Kingdom & kingdom = world.GetKingdom( color );
    const bool isAIPlayer = kingdom.isControlAI();

    const int alliedColors = Players::GetPlayerFriends( color );

    for ( Maps::Tiles & tile : vec_tiles ) {
        if ( tile.isWater() ) {
            if ( isAIPlayer && tile.isFog( color ) ) {
                AI::Planner::Get().revealFog( tile, kingdom );
            }

            tile.ClearFog( alliedColors );
        }
    }
}

MapEvent * World::GetMapEvent( const fheroes2::Point & pos )
{
    std::list<MapObjectSimple *> res = map_objects.get( pos );
    return !res.empty() ? static_cast<MapEvent *>( res.front() ) : nullptr;
}

MapObjectSimple * World::GetMapObject( uint32_t uid )
{
    return uid ? map_objects.get( uid ) : nullptr;
}

void World::RemoveMapObject( const MapObjectSimple * obj )
{
    if ( obj )
        map_objects.remove( obj->GetUID() );
}

const Heroes * World::GetHeroesCondWins() const
{
    return ( ( Settings::Get().getCurrentMapInfo().ConditionWins() & GameOver::WINS_HERO ) != 0 ) ? GetHeroes( heroIdAsWinCondition ) : nullptr;
}

const Heroes * World::GetHeroesCondLoss() const
{
    return ( ( Settings::Get().getCurrentMapInfo().ConditionLoss() & GameOver::LOSS_HERO ) != 0 ) ? GetHeroes( heroIdAsLossCondition ) : nullptr;
}

bool World::KingdomIsWins( const Kingdom & kingdom, const uint32_t wins ) const
{
#ifndef NDEBUG
#if defined( WITH_DEBUG )
    const Player * kingdomPlayer = Players::Get( kingdom.GetColor() );
    assert( kingdomPlayer != nullptr );

    const bool isKingdomInAIAutoControlMode = kingdomPlayer->isAIAutoControlMode();
#else
    const bool isKingdomInAIAutoControlMode = false;
#endif // WITH_DEBUG
#endif // !NDEBUG

    const Maps::FileInfo & mapInfo = Settings::Get().getCurrentMapInfo();

    switch ( wins ) {
    case GameOver::WINS_ALL:
        // This method should be called with this condition only for a human-controlled kingdom
        assert( kingdom.isControlHuman() || isKingdomInAIAutoControlMode );

        return kingdom.GetColor() == vec_kingdoms.GetNotLossColors();

    case GameOver::WINS_TOWN: {
        const Castle * town = getCastleEntrance( mapInfo.WinsMapsPositionObject() );
        return ( kingdom.isControlHuman() || mapInfo.WinsCompAlsoWins() ) && ( town && town->GetColor() == kingdom.GetColor() );
    }

    case GameOver::WINS_HERO: {
        // This method should be called with this condition only for a human-controlled kingdom
        assert( kingdom.isControlHuman() || isKingdomInAIAutoControlMode );

        if ( heroIdAsWinCondition == Heroes::UNKNOWN ) {
            return false;
        }

        const Heroes * hero = GetHeroesCondWins();
        assert( hero != nullptr );

        // The hero in question should either be available for hire or be hired by a human-controlled kingdom
        return ( hero->isAvailableForHire() || GetKingdom( hero->GetColor() ).isControlHuman() );
    }

    case GameOver::WINS_ARTIFACT: {
        // This method should be called with this condition only for a human-controlled kingdom
        assert( kingdom.isControlHuman() || isKingdomInAIAutoControlMode );

        const VecHeroes & heroes = kingdom.GetHeroes();
        if ( mapInfo.WinsFindUltimateArtifact() ) {
            return std::any_of( heroes.begin(), heroes.end(), []( const Heroes * hero ) { return hero->HasUltimateArtifact(); } );
        }
        else {
            const Artifact art = mapInfo.WinsFindArtifactID();
            return std::any_of( heroes.begin(), heroes.end(), [&art]( const Heroes * hero ) { return hero->hasArtifact( art ); } );
        }
    }

    case GameOver::WINS_SIDE:
        // This method should be called with this condition only for a human-controlled kingdom
        assert( kingdom.isControlHuman() || isKingdomInAIAutoControlMode );

        return !( Game::GetActualKingdomColors() & ~Players::GetPlayerFriends( kingdom.GetColor() ) );

    case GameOver::WINS_GOLD:
        return ( ( kingdom.isControlHuman() || mapInfo.WinsCompAlsoWins() ) && 0 < kingdom.GetFunds().Get( Resource::GOLD )
                 && static_cast<uint32_t>( kingdom.GetFunds().Get( Resource::GOLD ) ) >= mapInfo.getWinningGoldAccumulationValue() );

    default:
        break;
    }

    return false;
}

bool World::KingdomIsLoss( const Kingdom & kingdom, const uint32_t loss ) const
{
#ifndef NDEBUG
#if defined( WITH_DEBUG )
    const Player * kingdomPlayer = Players::Get( kingdom.GetColor() );
    assert( kingdomPlayer != nullptr );

    const bool isKingdomInAIAutoControlMode = kingdomPlayer->isAIAutoControlMode();
#else
    const bool isKingdomInAIAutoControlMode = false;
#endif // WITH_DEBUG
#endif // !NDEBUG

    // This method should only be called for a human-controlled kingdom
    assert( kingdom.isControlHuman() || isKingdomInAIAutoControlMode );

    const Settings & conf = Settings::Get();

    switch ( loss ) {
    case GameOver::LOSS_ALL:
        return kingdom.isLoss();

    case GameOver::LOSS_TOWN: {
        const Castle * town = getCastleEntrance( conf.getCurrentMapInfo().LossMapsPositionObject() );
        return ( town && town->GetColor() != kingdom.GetColor() );
    }

    case GameOver::LOSS_HERO: {
        if ( heroIdAsLossCondition == Heroes::UNKNOWN ) {
            return false;
        }

        const Heroes * hero = GetHeroesCondLoss();
        assert( hero != nullptr );

        // The hero in question should either be available for hire...
        if ( hero->isAvailableForHire() ) {
            return true;
        }

#if defined( WITH_DEBUG )
        const Player * heroPlayer = Players::Get( hero->GetColor() );
        assert( heroPlayer != nullptr );

        const bool isHeroInAIAutoControlMode = heroPlayer->isAIAutoControlMode();
#else
        const bool isHeroInAIAutoControlMode = false;
#endif

        // .. or be hired by an AI-controlled kingdom
        if ( GetKingdom( hero->GetColor() ).isControlAI() && !isHeroInAIAutoControlMode ) {
            // Exception for campaign: hero is not considered lost if he is hired by a friendly AI-controlled kingdom
            if ( conf.isCampaignGameType() && Players::isFriends( kingdom.GetColor(), hero->GetColor() ) ) {
                return false;
            }

            return true;
        }

        return false;
    }

    case GameOver::LOSS_TIME:
        return ( CountDay() > conf.getCurrentMapInfo().LossCountDays() );

    default:
        break;
    }

    return false;
}

uint32_t World::CheckKingdomWins( const Kingdom & kingdom ) const
{
#ifndef NDEBUG
#if defined( WITH_DEBUG )
    const Player * kingdomPlayer = Players::Get( kingdom.GetColor() );
    assert( kingdomPlayer != nullptr );

    const bool isKingdomInAIAutoControlMode = kingdomPlayer->isAIAutoControlMode();
#else
    const bool isKingdomInAIAutoControlMode = false;
#endif // WITH_DEBUG
#endif // !NDEBUG

    // This method should only be called for a human-controlled kingdom
    assert( kingdom.isControlHuman() || isKingdomInAIAutoControlMode );

    const Settings & conf = Settings::Get();

    if ( conf.isCampaignGameType() ) {
        const Campaign::ScenarioVictoryCondition victoryCondition = Campaign::getCurrentScenarioVictoryCondition();
        if ( victoryCondition == Campaign::ScenarioVictoryCondition::CAPTURE_DRAGON_CITY ) {
            const bool visited = kingdom.isVisited( MP2::OBJ_DRAGON_CITY );
            if ( visited ) {
                return GameOver::WINS_SIDE;
            }

            return GameOver::COND_NONE;
        }
    }

    const std::array<uint32_t, 6> wins
        = { GameOver::WINS_ALL, GameOver::WINS_TOWN, GameOver::WINS_HERO, GameOver::WINS_ARTIFACT, GameOver::WINS_SIDE, GameOver::WINS_GOLD };

    for ( const uint32_t cond : wins ) {
        if ( ( ( conf.getCurrentMapInfo().ConditionWins() & cond ) == cond ) && KingdomIsWins( kingdom, cond ) ) {
            return cond;
        }
    }

    return GameOver::COND_NONE;
}

uint32_t World::CheckKingdomLoss( const Kingdom & kingdom ) const
{
#ifndef NDEBUG
#if defined( WITH_DEBUG )
    const Player * kingdomPlayer = Players::Get( kingdom.GetColor() );
    assert( kingdomPlayer != nullptr );

    const bool isKingdomInAIAutoControlMode = kingdomPlayer->isAIAutoControlMode();
#else
    const bool isKingdomInAIAutoControlMode = false;
#endif // WITH_DEBUG
#endif // !NDEBUG

    // This method should only be called for a human-controlled kingdom
    assert( kingdom.isControlHuman() || isKingdomInAIAutoControlMode );

    const Settings & conf = Settings::Get();

    // First of all, check if the other players have not completed WINS_TOWN or WINS_GOLD yet
    const std::array<std::pair<uint32_t, uint32_t>, 4> enemy_wins = { std::make_pair<uint32_t, uint32_t>( GameOver::WINS_TOWN, GameOver::LOSS_ENEMY_WINS_TOWN ),
                                                                      std::make_pair<uint32_t, uint32_t>( GameOver::WINS_GOLD, GameOver::LOSS_ENEMY_WINS_GOLD ) };

    for ( const auto & item : enemy_wins ) {
        if ( conf.getCurrentMapInfo().ConditionWins() & item.first ) {
            const int color = vec_kingdoms.FindWins( item.first );

            if ( color && color != kingdom.GetColor() ) {
                return item.second;
            }
        }
    }

    if ( conf.isCampaignGameType() ) {
        const Campaign::ScenarioLossCondition lossCondition = Campaign::getCurrentScenarioLossCondition();
        if ( lossCondition == Campaign::ScenarioLossCondition::LOSE_ALL_SORCERESS_VILLAGES ) {
            const VecCastles & castles = kingdom.GetCastles();
            bool hasSorceressVillage = false;

            for ( size_t i = 0; i < castles.size(); ++i ) {
                if ( castles[i]->isCastle() || castles[i]->GetRace() != Race::SORC )
                    continue;

                hasSorceressVillage = true;
                break;
            }

            if ( !hasSorceressVillage )
                return GameOver::LOSS_ALL;
        }
    }

    const std::array<uint32_t, 4> loss = { GameOver::LOSS_ALL, GameOver::LOSS_TOWN, GameOver::LOSS_HERO, GameOver::LOSS_TIME };

    for ( const uint32_t cond : loss ) {
        if ( ( ( conf.getCurrentMapInfo().ConditionLoss() & cond ) == cond ) && KingdomIsLoss( kingdom, cond ) ) {
            return cond;
        }
    }

    return GameOver::COND_NONE;
}

uint32_t World::getDistance( const Heroes & hero, int targetIndex )
{
    _pathfinder.reEvaluateIfNeeded( hero );
    return _pathfinder.getDistance( targetIndex );
}

std::list<Route::Step> World::getPath( const Heroes & hero, int targetIndex )
{
    _pathfinder.reEvaluateIfNeeded( hero );
    return _pathfinder.buildPath( targetIndex );
}

void World::resetPathfinder()
{
    _pathfinder.reset();
    AI::Planner::Get().resetPathfinder();
}

void World::updatePassabilities()
{
    for ( Maps::Tiles & tile : vec_tiles ) {
        // If tile is empty then update tile's object type if needed.
        if ( tile.isSameMainObject( MP2::OBJ_NONE ) ) {
            tile.updateObjectType();
        }

        tile.setInitialPassability();
    }

    // Once the original passabilities are set we know all neighbours. Now we have to update passabilities based on neighbours.
    for ( Maps::Tiles & tile : vec_tiles ) {
        tile.updatePassability();
    }
}

void World::PostLoad( const bool setTilePassabilities )
{
    if ( setTilePassabilities ) {
        updatePassabilities();
    }

    // Cache all tiles that that contain stone liths of a certain type (depending on object sprite index).
    _allTeleports.clear();

    for ( const int32_t index : Maps::GetObjectPositions( MP2::OBJ_STONE_LITHS ) ) {
        _allTeleports[GetTiles( index ).GetObjectSpriteIndex()].push_back( index );
    }

    // Cache all tiles that contain a certain part of the whirlpool (depending on object sprite index).
    _allWhirlpools.clear();

    for ( const int32_t index : Maps::GetObjectPositions( MP2::OBJ_WHIRLPOOL ) ) {
        _allWhirlpools[GetTiles( index ).GetObjectSpriteIndex()].push_back( index );
    }

    resetPathfinder();
    ComputeStaticAnalysis();
}

uint32_t World::GetMapSeed() const
{
    return _seed;
}

uint32_t World::GetWeekSeed() const
{
    uint32_t weekSeed = _seed;

    fheroes2::hashCombine( weekSeed, week );

    return weekSeed;
}

bool World::isAnyKingdomVisited( const MP2::MapObjectType objectType, const int32_t dstIndex ) const
{
    const Colors colors( Game::GetKingdomColors() );
    for ( const int color : colors ) {
        const Kingdom & kingdom = GetKingdom( color );
        if ( kingdom.isVisited( dstIndex, objectType ) ) {
            return true;
        }
    }
    return false;
}

StreamBase & operator<<( StreamBase & msg, const CapturedObject & obj )
{
    return msg << obj.objcol << obj.guardians;
}

StreamBase & operator>>( StreamBase & msg, CapturedObject & obj )
{
    return msg >> obj.objcol >> obj.guardians;
}

StreamBase & operator<<( StreamBase & msg, const MapObjects & objs )
{
    msg << static_cast<uint32_t>( objs.size() );
    for ( MapObjects::const_iterator it = objs.begin(); it != objs.end(); ++it )
        if ( ( *it ).second ) {
            const MapObjectSimple & obj = *( *it ).second;
            msg << ( *it ).first << obj.GetType();

            switch ( obj.GetType() ) {
            case MP2::OBJ_EVENT:
                msg << static_cast<const MapEvent &>( obj );
                break;

            case MP2::OBJ_SPHINX:
                msg << static_cast<const MapSphinx &>( obj );
                break;

            case MP2::OBJ_SIGN:
                msg << static_cast<const MapSign &>( obj );
                break;

            default:
                msg << obj;
                break;
            }
        }

    return msg;
}

StreamBase & operator>>( StreamBase & msg, MapObjects & objs )
{
    uint32_t size = 0;
    msg >> size;

    objs.clear();

    for ( uint32_t ii = 0; ii < size; ++ii ) {
        int32_t index;
        int type;
        msg >> index >> type;

        switch ( type ) {
        case MP2::OBJ_EVENT: {
            MapEvent * ptr = new MapEvent();
            msg >> *ptr;
            objs[index] = ptr;
            break;
        }

        case MP2::OBJ_SPHINX: {
            MapSphinx * ptr = new MapSphinx();
            msg >> *ptr;
            objs[index] = ptr;
            break;
        }

        case MP2::OBJ_SIGN: {
            MapSign * ptr = new MapSign();
            msg >> *ptr;
            objs[index] = ptr;
            break;
        }

        default: {
            MapObjectSimple * ptr = new MapObjectSimple();
            msg >> *ptr;
            objs[index] = ptr;
            break;
        }
        }
    }

    return msg;
}

StreamBase & operator<<( StreamBase & msg, const World & w )
{
    return msg << w.width << w.height << w.vec_tiles << w.vec_heroes << w.vec_castles << w.vec_kingdoms << w._customRumors << w.vec_eventsday << w.map_captureobj
               << w.ultimate_artifact << w.day << w.week << w.month << w.heroIdAsWinCondition << w.heroIdAsLossCondition << w.map_objects << w._seed;
}

StreamBase & operator>>( StreamBase & msg, World & w )
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1010_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1010_RELEASE ) {
        uint16_t width = 0;
        uint16_t height = 0;

        msg >> width >> height;
        w.width = width;
        w.height = height;
    }
    else {
        msg >> w.width >> w.height;
    }

    msg >> w.vec_tiles >> w.vec_heroes >> w.vec_castles >> w.vec_kingdoms >> w._customRumors >> w.vec_eventsday >> w.map_captureobj >> w.ultimate_artifact >> w.day
        >> w.week >> w.month >> w.heroIdAsWinCondition >> w.heroIdAsLossCondition;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1010_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1010_RELEASE ) {
        ++w.heroIdAsWinCondition;
        ++w.heroIdAsLossCondition;
    }

    msg >> w.map_objects >> w._seed;

    w.PostLoad( false );

    return msg;
}

void EventDate::LoadFromMP2( const std::vector<uint8_t> & data )
{
    assert( data.size() >= MP2::MP2_EVENT_STRUCTURE_MIN_SIZE );

    assert( data[0] == 0 );

    assert( data[42] == 1 );

    // Structure containing information about a timed global event.
    //
    // - uint8_t (1 byte)
    //     Always 0 as an indicator that this indeed a timed global object.
    //
    // - int32_t (4 bytes)
    //     The amount of Wood to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Mercury to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Ore to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Sulfur to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Crystals to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Gems to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Gold to be given. Can be negative.
    //
    // - uint16_t (2 bytes)
    //     Possible artifacts to be given. Not in use.
    //
    // - uint16_t (2 bytes)
    //     A flag whether the event is applicable for AI players as well.
    //
    // - uint16_t (2 bytes)
    //     The first day of occurrence of the event.
    //
    // - uint16_t (2 bytes)
    //     Period in days when the event repeats. 0 means that the event never repeats.
    //
    // - unused 5 bytes
    //    Always 0.
    //
    // - unused 1 byte
    //    Always 1.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Blue player receives the event.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Green player receives the event.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Red player receives the event.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Yellow player receives the event.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Orange player receives the event.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Purple player receives the event.
    //
    // - string
    //    Null terminated string containing the event text.

    StreamBuf dataStream( data );

    dataStream.skip( 1 );

    // Get the amount of resources.
    resource.wood = static_cast<int32_t>( dataStream.getLE32() );
    resource.mercury = static_cast<int32_t>( dataStream.getLE32() );
    resource.ore = static_cast<int32_t>( dataStream.getLE32() );
    resource.sulfur = static_cast<int32_t>( dataStream.getLE32() );
    resource.crystal = static_cast<int32_t>( dataStream.getLE32() );
    resource.gems = static_cast<int32_t>( dataStream.getLE32() );
    resource.gold = static_cast<int32_t>( dataStream.getLE32() );

    dataStream.skip( 2 );

    // The event applies to AI players as well.
    isApplicableForAIPlayers = ( dataStream.getLE16() != 0 );

    // Get the first day of occurrence.
    firstOccurrenceDay = dataStream.getLE16();

    // Get the repeat period.
    repeatPeriodInDays = dataStream.getLE16();

    dataStream.skip( 6 );

    colors = 0;

    if ( dataStream.get() ) {
        colors |= Color::BLUE;
    }

    if ( dataStream.get() ) {
        colors |= Color::GREEN;
    }

    if ( dataStream.get() ) {
        colors |= Color::RED;
    }

    if ( dataStream.get() ) {
        colors |= Color::YELLOW;
    }

    if ( dataStream.get() ) {
        colors |= Color::ORANGE;
    }

    if ( dataStream.get() ) {
        colors |= Color::PURPLE;
    }

    message = dataStream.toString();

    DEBUG_LOG( DBG_GAME, DBG_INFO, "A timed event which occurs at day " << firstOccurrenceDay << " contains a message: " << message )
}

bool EventDate::isAllow( const int col, const uint32_t date ) const
{
    if ( ( col & colors ) == 0 ) {
        // This player color is not allowed for the event.
        return false;
    }

    if ( firstOccurrenceDay > date ) {
        // The date has not come.
        return false;
    }

    if ( firstOccurrenceDay == date ) {
        return true;
    }

    if ( repeatPeriodInDays == 0 ) {
        // This is not the same date and the event does not repeat.
        return false;
    }

    return ( ( date - firstOccurrenceDay ) % repeatPeriodInDays ) == 0;
}

StreamBase & operator<<( StreamBase & msg, const EventDate & obj )
{
    return msg << obj.resource << obj.isApplicableForAIPlayers << obj.firstOccurrenceDay << obj.repeatPeriodInDays << obj.colors << obj.message << obj.title;
}

StreamBase & operator>>( StreamBase & msg, EventDate & obj )
{
    return msg >> obj.resource >> obj.isApplicableForAIPlayers >> obj.firstOccurrenceDay >> obj.repeatPeriodInDays >> obj.colors >> obj.message >> obj.title;
}
