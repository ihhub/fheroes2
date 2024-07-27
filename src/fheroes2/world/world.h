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
#ifndef H2WORLD_H
#define H2WORLD_H

#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "army_troop.h"
#include "artifact_ultimate.h"
#include "castle.h"
#include "heroes.h"
#include "kingdom.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "monster.h"
#include "mp2.h"
#include "pairs.h"
#include "resource.h"
#include "world_pathfinding.h"
#include "world_regions.h"

class MapObjectSimple;
class StreamBase;

struct MapEvent;
struct Week;

namespace Route
{
    class Step;
}

struct MapObjects : public std::map<uint32_t, MapObjectSimple *>
{
    MapObjects() = default;
    MapObjects( const MapObjects & other ) = delete;
    MapObjects( MapObjects && other ) = delete;

    ~MapObjects();

    MapObjects & operator=( const MapObjects & other ) = delete;
    MapObjects & operator=( MapObjects && other ) = delete;

    void clear();
    void add( MapObjectSimple * );
    std::list<MapObjectSimple *> get( const fheroes2::Point & );
    MapObjectSimple * get( uint32_t uid );
    void remove( uint32_t uid );
};

struct CapturedObject
{
    ObjectColor objcol;
    Troop guardians;

    CapturedObject() = default;

    int GetColor() const
    {
        return objcol.second;
    }
    Troop & GetTroop()
    {
        return guardians;
    }

    void Set( int obj, int col )
    {
        objcol = ObjectColor( obj, col );
    }
    void SetColor( int col )
    {
        objcol.second = col;
    }
};

struct CapturedObjects : std::map<int32_t, CapturedObject>
{
    void Set( int32_t, int, int );
    void SetColor( int32_t, int );
    void ClearFog( int );
    void ResetColor( int );

    CapturedObject & Get( int32_t );

    uint32_t GetCount( int, int ) const;
    uint32_t GetCountMines( int, int ) const;
    int GetColor( int32_t ) const;
};

struct EventDate
{
    void LoadFromMP2( const std::vector<uint8_t> & data );

    bool isAllow( const int color, const uint32_t date ) const;

    bool isDeprecated( const uint32_t date ) const
    {
        return date > firstOccurrenceDay && repeatPeriodInDays == 0;
    }

    Funds resource;
    uint32_t firstOccurrenceDay{ 0 };
    uint32_t repeatPeriodInDays{ 0 };
    int colors{ 0 };
    bool isApplicableForAIPlayers{ false };
    std::string message;

    std::string title;
};

StreamBase & operator<<( StreamBase &, const EventDate & );
StreamBase & operator>>( StreamBase &, EventDate & );

using EventsDate = std::list<EventDate>;

class World : protected fheroes2::Size
{
public:
    World( const World & other ) = delete;
    World( World && other ) = delete;

    ~World()
    {
        Reset();
    }

    World & operator=( const World & other ) = delete;
    World & operator=( World && other ) = delete;

    bool LoadMapMP2( const std::string & filename, const bool isOriginalMp2File );

    bool loadResurrectionMap( const std::string & filename );

    // Generate 2x2 map for Battle Only mode.
    void generateBattleOnlyMap();

    void generateForEditor( const int32_t size );

    static World & Get();

    int32_t w() const
    {
        return width;
    }

    int32_t h() const
    {
        return height;
    }

    const Maps::Tiles & GetTiles( const int32_t x, const int32_t y ) const
    {
#ifdef WITH_DEBUG
        return vec_tiles.at( y * width + x );
#else
        return vec_tiles[y * width + x];
#endif
    }

    Maps::Tiles & GetTiles( const int32_t x, const int32_t y )
    {
#ifdef WITH_DEBUG
        return vec_tiles.at( y * width + x );
#else
        return vec_tiles[y * width + x];
#endif
    }

    const Maps::Tiles & GetTiles( const int32_t tileId ) const
    {
#ifdef WITH_DEBUG
        return vec_tiles.at( tileId );
#else
        return vec_tiles[tileId];
#endif
    }

    Maps::Tiles & GetTiles( const int32_t tileId )
    {
#ifdef WITH_DEBUG
        return vec_tiles.at( tileId );
#else
        return vec_tiles[tileId];
#endif
    }

    void InitKingdoms()
    {
        vec_kingdoms.Init();
    }

    Kingdom & GetKingdom( int color )
    {
        return vec_kingdoms.GetKingdom( color );
    }

    const Kingdom & GetKingdom( int color ) const
    {
        return vec_kingdoms.GetKingdom( color );
    }

    void addCastle( int32_t index, uint8_t race, uint8_t color )
    {
        Castle * castle = new Castle( index % width, index / width, race );
        castle->SetColor( color );
        vec_castles.AddCastle( castle );
    }

    // Get castle based on its tile. If the tile is not a part of a castle return nullptr.
    const Castle * getCastle( const fheroes2::Point & tilePosition ) const
    {
        return vec_castles.Get( tilePosition );
    }

    Castle * getCastle( const fheroes2::Point & tilePosition )
    {
        return vec_castles.Get( tilePosition );
    }

    // Get castle based on its entrance tile. If the tile is not castle's entrance return nullptr.
    const Castle * getCastleEntrance( const fheroes2::Point & tilePosition ) const;
    Castle * getCastleEntrance( const fheroes2::Point & tilePosition );

    const Heroes * GetHeroes( int id ) const
    {
        return vec_heroes.Get( id );
    }

    Heroes * GetHeroes( int id )
    {
        return vec_heroes.Get( id );
    }

    const Heroes * GetHeroes( const fheroes2::Point & center ) const
    {
        return vec_heroes.Get( center );
    }

    Heroes * GetHeroes( const fheroes2::Point & center )
    {
        return vec_heroes.Get( center );
    }

    Heroes * FromJailHeroes( int32_t );
    Heroes * GetHeroForHire( const int race, const int heroIDToIgnore = Heroes::UNKNOWN ) const;

    const Heroes * GetHeroesCondWins() const;
    const Heroes * GetHeroesCondLoss() const;

    Heroes * GetHero( const Castle & ) const;

    const UltimateArtifact & GetUltimateArtifact() const;
    bool DiggingForUltimateArtifact( const fheroes2::Point & );

    // overall number of cells of the world map: width * height
    size_t getSize() const
    {
        return vec_tiles.size();
    }

    int GetDay() const;
    int GetWeek() const;

    uint32_t GetMonth() const
    {
        return month;
    }

    uint32_t CountDay() const
    {
        return day;
    }

    uint32_t CountWeek() const
    {
        return week;
    }

    bool BeginWeek() const;
    bool BeginMonth() const;
    bool LastDay() const;
    bool FirstWeek() const;
    bool LastWeek() const;
    const Week & GetWeekType() const;
    std::string DateString() const;

    void NewDay();
    void NewWeek();
    void NewMonth();

    std::string getCurrentRumor() const;

    int32_t NextTeleport( const int32_t index ) const;
    MapsIndexes GetTeleportEndPoints( const int32_t index ) const;

    int32_t NextWhirlpool( const int32_t index ) const;
    MapsIndexes GetWhirlpoolEndPoints( const int32_t index ) const;

    void CaptureObject( int32_t, int col );
    uint32_t CountCapturedObject( int obj, int col ) const;
    uint32_t CountCapturedMines( int type, int col ) const;
    uint32_t CountObeliskOnMaps();
    int ColorCapturedObject( int32_t ) const;
    void ResetCapturedObjects( int );
    CapturedObject & GetCapturedObject( int32_t );

    void ActionForMagellanMaps( int color );
    void ClearFog( int color );

    bool KingdomIsWins( const Kingdom & kingdom, const uint32_t wins ) const;
    bool KingdomIsLoss( const Kingdom & kingdom, const uint32_t loss ) const;

    uint32_t CheckKingdomWins( const Kingdom & kingdom ) const;
    uint32_t CheckKingdomLoss( const Kingdom & kingdom ) const;

    void AddEventDate( const EventDate & );
    EventsDate GetEventsDate( int color ) const;

    MapEvent * GetMapEvent( const fheroes2::Point & );
    MapObjectSimple * GetMapObject( uint32_t uid );
    void RemoveMapObject( const MapObjectSimple * );
    const MapRegion & getRegion( size_t id ) const;
    size_t getRegionCount() const;

    uint32_t getDistance( const Heroes & hero, int targetIndex );
    std::list<Route::Step> getPath( const Heroes & hero, int targetIndex );
    void resetPathfinder();

    void ComputeStaticAnalysis();

    uint32_t GetMapSeed() const;
    uint32_t GetWeekSeed() const;

    bool isAnyKingdomVisited( const MP2::MapObjectType objectType, const int32_t dstIndex ) const;

    void updatePassabilities();

private:
    World() = default;

    void Defaults();
    void Reset();
    void MonthOfMonstersAction( const Monster & );
    bool ProcessNewMP2Map( const std::string & filename, const bool checkPoLObjects );
    void PostLoad( const bool setTilePassabilities );

    bool updateTileMetadata( Maps::Tiles & tile, const MP2::MapObjectType objectType, const bool checkPoLObjects );

    bool isValidCastleEntrance( const fheroes2::Point & tilePosition ) const;

    void setUltimateArtifact( const int32_t tileId, const int32_t radius );

    void addDebugHero();

    void setHeroIdsForMapConditions();

    friend class Radar;
    friend StreamBase & operator<<( StreamBase &, const World & );
    friend StreamBase & operator>>( StreamBase &, World & );

    std::vector<Maps::Tiles> vec_tiles;
    AllHeroes vec_heroes;
    AllCastles vec_castles;
    Kingdoms vec_kingdoms;
    std::vector<std::string> _customRumors;
    EventsDate vec_eventsday;

    // index, object, color
    CapturedObjects map_captureobj;

    UltimateArtifact ultimate_artifact;

    uint32_t day = 0;
    uint32_t week = 0;
    uint32_t month = 0;

    int heroIdAsWinCondition = Heroes::UNKNOWN;
    int heroIdAsLossCondition = Heroes::UNKNOWN;

    MapObjects map_objects;

    uint32_t _seed{ 0 }; // Map seed

    // The following fields are not serialized

    std::map<uint8_t, Maps::Indexes> _allTeleports; // All indexes of tiles that contain stone liths of a certain type (sprite index)
    std::map<uint8_t, Maps::Indexes> _allWhirlpools; // All indexes of tiles that contain a certain part (sprite index) of the whirlpool

    std::vector<MapRegion> _regions;
    PlayerWorldPathfinder _pathfinder;
};

StreamBase & operator<<( StreamBase &, const CapturedObject & );
StreamBase & operator>>( StreamBase &, CapturedObject & );

StreamBase & operator<<( StreamBase &, const World & );
StreamBase & operator>>( StreamBase &, World & );

StreamBase & operator<<( StreamBase &, const MapObjects & );
StreamBase & operator>>( StreamBase &, MapObjects & );

extern World & world;

#endif
