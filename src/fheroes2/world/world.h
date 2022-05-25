/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

class MapObjectSimple;
class ActionSimple;
struct MapEvent;

struct ListActions : public std::list<ActionSimple *>
{
    ListActions() = default;
    ListActions( const ListActions & other ) = default;
    ListActions( ListActions && other ) = delete;

    ~ListActions();

    ListActions & operator=( const ListActions & other ) = delete;
    ListActions & operator=( ListActions && other ) = delete;

    void clear( void );
};

struct MapObjects : public std::map<uint32_t, MapObjectSimple *>
{
    MapObjects() = default;
    MapObjects( const MapObjects & other ) = delete;
    MapObjects( MapObjects && other ) = delete;

    ~MapObjects();

    MapObjects & operator=( const MapObjects & other ) = delete;
    MapObjects & operator=( MapObjects && other ) = delete;

    void clear( void );
    void add( MapObjectSimple * );
    std::list<MapObjectSimple *> get( const fheroes2::Point & );
    MapObjectSimple * get( uint32_t uid );
    void remove( uint32_t uid );
};

using MapActions = std::map<int32_t, ListActions>;

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

struct CapturedObjects : std::map<int32_t, CapturedObject>
{
    void Set( int32_t, int, int );
    void SetColor( int32_t, int );
    void ClearFog( int );
    void ResetColor( int );

    CapturedObject & Get( int32_t );

    void tributeCapturedObjects( const int playerColorId, const int objectType, Funds & funds, int & objectCount );

    uint32_t GetCount( int, int ) const;
    uint32_t GetCountMines( int, int ) const;
    int GetColor( int32_t ) const;
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

    bool isAllow( int color, uint32_t date ) const;
    bool isDeprecated( uint32_t date ) const;

    Funds resource;
    uint32_t first;
    uint32_t subsequent;
    int colors;
    bool computer;
    std::string message;

    std::string title;
};

StreamBase & operator<<( StreamBase &, const EventDate & );
StreamBase & operator>>( StreamBase &, EventDate & );

using EventsDate = std::list<EventDate>;
using MapsTiles = std::vector<Maps::Tiles>;

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

    Heroes * FromJailHeroes( int32_t );
    Heroes * GetFreemanHeroes( const int race, const int heroIDToIgnore = Heroes::UNKNOWN ) const;

    const Heroes * GetHeroesCondWins() const;
    const Heroes * GetHeroesCondLoss() const;

    CastleHeroes GetHeroes( const Castle & ) const;

    const UltimateArtifact & GetUltimateArtifact( void ) const;
    bool DiggingForUltimateArtifact( const fheroes2::Point & );

    // overall number of cells of the world map: width * height
    size_t getSize() const;
    int GetDay( void ) const;
    int GetWeek( void ) const;

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

    bool BeginWeek( void ) const;
    bool BeginMonth( void ) const;
    bool LastDay( void ) const;
    bool FirstWeek() const;
    bool LastWeek() const;
    const Week & GetWeekType() const;
    std::string DateString( void ) const;

    void NewDay( void );
    void NewWeek( void );
    void NewMonth( void );

    std::string getCurrentRumor() const;

    int32_t NextTeleport( const int32_t index ) const;
    MapsIndexes GetTeleportEndPoints( const int32_t index ) const;

    int32_t NextWhirlpool( const int32_t index ) const;
    MapsIndexes GetWhirlpoolEndPoints( const int32_t index ) const;

    void CaptureObject( int32_t, int col );
    uint32_t CountCapturedObject( int obj, int col ) const;
    uint32_t CountCapturedMines( int type, int col ) const;
    uint32_t CountObeliskOnMaps( void );
    int ColorCapturedObject( int32_t ) const;
    void ResetCapturedObjects( int );
    CapturedObject & GetCapturedObject( int32_t );
    ListActions * GetListActions( int32_t );

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
    static uint32_t GetUniq( void );

    uint32_t GetMapSeed() const;
    uint32_t GetWeekSeed() const;

    bool isAnyKingdomVisited( const MP2::MapObjectType objectType, const int32_t dstIndex ) const;

private:
    World() = default;

    void Defaults( void );
    void Reset( void );
    void MonthOfMonstersAction( const Monster & );
    void ProcessNewMap();
    void PostLoad( const bool setTilePassabilities );

    bool isValidCastleEntrance( const fheroes2::Point & tilePosition ) const;

    friend class Radar;
    friend StreamBase & operator<<( StreamBase &, const World & );
    friend StreamBase & operator>>( StreamBase &, World & );

    MapsTiles vec_tiles;
    AllHeroes vec_heroes;
    AllCastles vec_castles;
    Kingdoms vec_kingdoms;
    std::vector<std::string> _rumors;
    EventsDate vec_eventsday;

    // index, object, color
    CapturedObjects map_captureobj;

    UltimateArtifact ultimate_artifact;

    uint32_t day = 0;
    uint32_t week = 0;
    uint32_t month = 0;

    int heroes_cond_wins = Heroes::UNKNOWN;
    int heroes_cond_loss = Heroes::UNKNOWN;

    MapActions map_actions;
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

StreamBase & operator<<( StreamBase &, const ListActions & );
StreamBase & operator>>( StreamBase &, ListActions & );

StreamBase & operator<<( StreamBase &, const MapObjects & );
StreamBase & operator>>( StreamBase &, MapObjects & );

extern World & world;

#endif
