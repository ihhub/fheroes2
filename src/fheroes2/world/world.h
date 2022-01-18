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
#ifndef H2WORLD_H
#define H2WORLD_H

#include <map>
#include <string>
#include <vector>

#include "artifact_ultimate.h"
#include "castle_heroes.h"
#include "kingdom.h"
#include "maps.h"
#include "maps_tiles.h"
#include "week.h"
#include "world_pathfinding.h"
#include "world_regions.h"

class Recruits;
class MapObjectSimple;
class ActionSimple;
struct MapEvent;

struct ListActions : public std::list<ActionSimple *>
{
    ListActions() = default;
    ListActions( const ListActions & other ) = default;
    ListActions & operator=( const ListActions & other ) = delete;
    ListActions( const ListActions && other ) = delete;
    ListActions & operator=( const ListActions && other ) = delete;
    ~ListActions();
    void clear( void );
};

struct MapObjects : public std::map<u32, MapObjectSimple *>
{
    MapObjects() = default;
    MapObjects( const MapObjects & other ) = delete;
    MapObjects & operator=( const MapObjects & other ) = delete;
    MapObjects( const MapObjects && other ) = delete;
    MapObjects & operator=( const MapObjects && other ) = delete;
    ~MapObjects();
    void clear( void );
    void add( MapObjectSimple * );
    std::list<MapObjectSimple *> get( const fheroes2::Point & );
    MapObjectSimple * get( u32 uid );
    void remove( u32 uid );
};

using MapActions = std::map<s32, ListActions>;

struct CapturedObject
{
    ObjectColor objcol;
    Troop guardians;
    int split;

    CapturedObject()
        : split( 1 )
    {}

    int GetSplit( void ) const
    {
        return split;
    }
    int GetColor( void ) const
    {
        return objcol.second;
    }
    Troop & GetTroop( void )
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
    void SetSplit( int spl )
    {
        split = spl;
    }
};

struct CapturedObjects : std::map<s32, CapturedObject>
{
    void Set( s32, int, int );
    void SetColor( s32, int );
    void ClearFog( int );
    void ResetColor( int );

    CapturedObject & Get( s32 );

    void tributeCapturedObjects( const int playerColorId, const int objectType, Funds & funds, int & objectCount );

    u32 GetCount( int, int ) const;
    u32 GetCountMines( int, int ) const;
    int GetColor( s32 ) const;
};

struct EventDate
{
    EventDate()
        : first( 0 )
        , subsequent( 0 )
        , colors( 0 )
        , computer( false )
    {}

    void LoadFromMP2( StreamBuf );

    bool isAllow( int color, u32 date ) const;
    bool isDeprecated( u32 date ) const;

    Funds resource;
    u32 first;
    u32 subsequent;
    int colors;
    bool computer;
    std::string message;

    std::string title;
};

StreamBase & operator<<( StreamBase &, const EventDate & );
StreamBase & operator>>( StreamBase &, EventDate & );

using Rumors = std::list<std::string>;
using EventsDate = std::list<EventDate>;
using MapsTiles = std::vector<Maps::Tiles>;

class World : protected fheroes2::Size
{
public:
    World( const World & other ) = delete;
    World & operator=( const World & other ) = delete;
    World( const World && other ) = delete;
    World & operator=( const World && other ) = delete;
    ~World()
    {
        Reset();
    }

    bool LoadMapMP2( const std::string & );

    void NewMaps( int32_t, int32_t );

    static World & Get( void );

    int32_t w() const
    {
        return width;
    }

    int32_t h() const
    {
        return height;
    }

    const Maps::Tiles & GetTiles( const int32_t x, const int32_t y ) const;
    Maps::Tiles & GetTiles( const int32_t x, const int32_t y );
    const Maps::Tiles & GetTiles( const int32_t tileId ) const;
    Maps::Tiles & GetTiles( const int32_t tileId );

    void InitKingdoms( void );

    Kingdom & GetKingdom( int color )
    {
        return vec_kingdoms.GetKingdom( color );
    }

    const Kingdom & GetKingdom( int color ) const
    {
        return vec_kingdoms.GetKingdom( color );
    }

    // Get castle based on its tile. If the tile is not a part of a castle return nullptr.
    const Castle * getCastle( const fheroes2::Point & tilePosition ) const;
    Castle * getCastle( const fheroes2::Point & tilePosition );

    // Get castle based on its entrance tile. If the tile is not castle's entrance return nullptr.
    const Castle * getCastleEntrance( const fheroes2::Point & tilePosition ) const;
    Castle * getCastleEntrance( const fheroes2::Point & tilePosition );

    const Heroes * GetHeroes( int id ) const;
    Heroes * GetHeroes( int id );

    const Heroes * GetHeroes( const fheroes2::Point & ) const;
    Heroes * GetHeroes( const fheroes2::Point & );

    Heroes * FromJailHeroes( s32 );
    Heroes * GetFreemanHeroes( int race = 0 ) const;

    const Heroes * GetHeroesCondWins( void ) const;
    const Heroes * GetHeroesCondLoss( void ) const;

    CastleHeroes GetHeroes( const Castle & ) const;

    const UltimateArtifact & GetUltimateArtifact( void ) const;
    bool DiggingForUltimateArtifact( const fheroes2::Point & );

    // overall number of cells of the world map: width * height
    size_t getSize() const;
    int GetDay( void ) const;
    int GetWeek( void ) const;
    int GetMonth( void ) const;
    u32 CountDay( void ) const;
    u32 CountWeek( void ) const;
    bool BeginWeek( void ) const;
    bool BeginMonth( void ) const;
    bool LastDay( void ) const;
    bool LastWeek( void ) const;
    const Week & GetWeekType( void ) const;
    std::string DateString( void ) const;

    void NewDay( void );
    void NewWeek( void );
    void NewMonth( void );

    const std::string & GetRumors( void );

    int32_t NextTeleport( const int32_t index ) const;
    MapsIndexes GetTeleportEndPoints( const int32_t index ) const;

    int32_t NextWhirlpool( const int32_t index ) const;
    MapsIndexes GetWhirlpoolEndPoints( const int32_t index ) const;

    void CaptureObject( s32, int col );
    u32 CountCapturedObject( int obj, int col ) const;
    u32 CountCapturedMines( int type, int col ) const;
    u32 CountObeliskOnMaps( void );
    int ColorCapturedObject( s32 ) const;
    void ResetCapturedObjects( int );
    CapturedObject & GetCapturedObject( s32 );
    ListActions * GetListActions( s32 );

    void ActionForMagellanMaps( int color );
    void ClearFog( int color );

    uint32_t CheckKingdomWins( const Kingdom & ) const;
    bool KingdomIsWins( const Kingdom &, uint32_t wins ) const;
    uint32_t CheckKingdomLoss( const Kingdom & ) const;
    bool KingdomIsLoss( const Kingdom &, uint32_t loss ) const;

    void AddEventDate( const EventDate & );
    EventsDate GetEventsDate( int color ) const;

    MapEvent * GetMapEvent( const fheroes2::Point & );
    MapObjectSimple * GetMapObject( u32 uid );
    void RemoveMapObject( const MapObjectSimple * );
    const MapRegion & getRegion( size_t id ) const;
    size_t getRegionCount() const;

    uint32_t getDistance( const Heroes & hero, int targetIndex );
    std::list<Route::Step> getPath( const Heroes & hero, int targetIndex );
    void resetPathfinder();

    void ComputeStaticAnalysis();
    static u32 GetUniq( void );

    uint32_t GetMapSeed() const;

    bool isAnyKingdomVisited( const MP2::MapObjectType objectType, const int32_t dstIndex ) const;

private:
    World()
        : fheroes2::Size( 0, 0 )
        , _rumor( nullptr )
        , _seed( 0 )
    {}

    void Defaults( void );
    void Reset( void );
    void MonthOfMonstersAction( const Monster & );
    void ProcessNewMap();
    void PostLoad( const bool setTilePassabilities );
    void pickRumor();

    bool isValidCastleEntrance( const fheroes2::Point & tilePosition ) const;

    friend class Radar;
    friend StreamBase & operator<<( StreamBase &, const World & );
    friend StreamBase & operator>>( StreamBase &, World & );

    MapsTiles vec_tiles;
    AllHeroes vec_heroes;
    AllCastles vec_castles;
    Kingdoms vec_kingdoms;
    Rumors vec_rumors;
    const std::string * _rumor;
    EventsDate vec_eventsday;

    // index, object, color
    CapturedObjects map_captureobj;

    UltimateArtifact ultimate_artifact;

    uint32_t day = 0;
    uint32_t week = 0;
    uint32_t month = 0;

    Week week_current;
    Week week_next;

    int heroes_cond_wins = Heroes::UNKNOWN;
    int heroes_cond_loss = Heroes::UNKNOWN;

    MapActions map_actions;
    MapObjects map_objects;

    // This data isn't serialized
    std::map<uint8_t, Maps::Indexes> _allTeleports; // All indexes of tiles that contain stone liths of a certain type (sprite index)
    std::map<uint8_t, Maps::Indexes> _allWhirlpools; // All indexes of tiles that contain a certain part (sprite index) of the whirlpool
    std::vector<MapRegion> _regions;
    PlayerWorldPathfinder _pathfinder;

    uint32_t _seed{ 0 }; // global seed for the map
    size_t _weekSeed{ 0 }; // global seed for the map, for this week
};

StreamBase & operator<<( StreamBase &, const CapturedObject & );
StreamBase & operator>>( StreamBase &, CapturedObject & );
StreamBase & operator<<( StreamBase &, const World & );
StreamBase & operator>>( StreamBase &, World & );

StreamBase & operator<<( StreamBase &, const ListActions & );
StreamBase & operator>>( StreamBase &, ListActions & );

StreamBase & operator<<( StreamBase &, const MapObjects & );
StreamBase & operator>>( StreamBase &, MapObjects & );

extern World & world;

#endif
