/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2026                                             *
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

#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "army_troop.h"
#include "artifact_ultimate.h"
#include "castle.h"
#include "color.h"
#include "game_string.h"
#include "heroes.h"
#include "kingdom.h"
#include "maps.h"
#include "maps_objects.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "monster.h"
#include "pairs.h"
#include "resource.h"
#include "world_pathfinding.h"
#include "world_regions.h"

class IStreamBase;
class OStreamBase;

struct Week;

namespace MP2
{
    enum MapObjectType : uint16_t;
}

namespace Route
{
    class Step;
}

// Number of days in the game week
inline constexpr uint32_t numOfDaysPerWeek{ 7 };
// Number of weeks in the game month
inline constexpr uint32_t numOfWeeksPerMonth{ 4 };

class MapObjects
{
public:
    MapObjects() = default;
    MapObjects( const MapObjects & other ) = delete;
    MapObjects( MapObjects && other ) = delete;

    ~MapObjects() = default;

    MapObjects & operator=( const MapObjects & other ) = delete;
    MapObjects & operator=( MapObjects && other ) = delete;

    void clear()
    {
        _objects.clear();
    }

    template <typename T, std::enable_if_t<std::is_base_of_v<MapBaseObject, T>, bool> = true>
    void add( std::unique_ptr<T> && obj )
    {
        if ( !obj ) {
            return;
        }

        if ( const auto [iter, inserted] = _objects.try_emplace( obj->GetUID(), std::move( obj ) ); !inserted ) {
            iter->second = std::move( obj );
        }
    }

    void remove( const uint32_t uid )
    {
        _objects.erase( uid );
    }

    MapBaseObject * get( const uint32_t uid ) const;
    std::list<MapBaseObject *> get( const fheroes2::Point & pos ) const;

private:
    friend OStreamBase & operator<<( OStreamBase & stream, const MapObjects & objs );
    friend IStreamBase & operator>>( IStreamBase & stream, MapObjects & objs );

    std::map<uint32_t, std::unique_ptr<MapBaseObject>> _objects;
};

struct CapturedObject
{
    ObjectColor objCol;
    Troop guardians;

    CapturedObject() = default;

    PlayerColor GetColor() const
    {
        return objCol.second;
    }

    Troop & GetTroop()
    {
        return guardians;
    }

    void Set( const MP2::MapObjectType object, const PlayerColor color )
    {
        objCol = { object, color };
    }

    void SetColor( const PlayerColor color )
    {
        objCol.second = color;
    }
};

struct CapturedObjects final : std::map<int32_t, CapturedObject>
{
    CapturedObjects() = default;

    void Set( const int32_t index, const MP2::MapObjectType obj, const PlayerColor color );

    void SetColor( const int32_t index, const PlayerColor color )
    {
        Get( index ).SetColor( color );
    }

    void ResetColor( const PlayerColor color );

    void ClearFog( const PlayerColorsSet colors ) const;

    CapturedObject & Get( const int32_t index )
    {
        return operator[]( index );
    }

    PlayerColor GetColor( const int32_t index ) const;

    uint32_t GetCount( const MP2::MapObjectType objectType, const PlayerColor ownerColor ) const;
    uint32_t GetCountMines( const int resourceType, const PlayerColor ownerColor ) const;
};

struct EventDate
{
    void LoadFromMP2( const std::vector<uint8_t> & data );

    bool isAllow( const PlayerColor color, const uint32_t date ) const;

    bool isDeprecated( const uint32_t date ) const
    {
        return date > firstOccurrenceDay && repeatPeriodInDays == 0;
    }

    Funds resource;
    uint32_t firstOccurrenceDay{ 0 };
    uint32_t repeatPeriodInDays{ 0 };
    PlayerColorsSet colors{ 0 };
    bool isApplicableForAIPlayers{ false };
    std::string message;

    std::string title;
};

OStreamBase & operator<<( OStreamBase & stream, const EventDate & obj );
IStreamBase & operator>>( IStreamBase & stream, EventDate & obj );

using EventsDate = std::list<EventDate>;

class World final : protected fheroes2::Size
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
    void generateBattleOnlyMap( const int32_t groundType );

    void setUniformTerrain( const int32_t groundType );

    // Generates a map without initializing tiles.
    // WARNING: call this method only when reading a map from a file
    void generateUninitializedMap( const int32_t size );

    static World & Get();

    int32_t w() const
    {
        return width;
    }

    int32_t h() const
    {
        return height;
    }

    const Maps::Tile & getTile( const int32_t x, const int32_t y ) const
    {
#ifdef WITH_DEBUG
        return vec_tiles.at( y * width + x );
#else
        return vec_tiles[y * width + x];
#endif
    }

    Maps::Tile & getTile( const int32_t x, const int32_t y )
    {
#ifdef WITH_DEBUG
        return vec_tiles.at( y * width + x );
#else
        return vec_tiles[y * width + x];
#endif
    }

    const Maps::Tile & getTile( const int32_t tileId ) const
    {
#ifdef WITH_DEBUG
        return vec_tiles.at( tileId );
#else
        return vec_tiles[tileId];
#endif
    }

    Maps::Tile & getTile( const int32_t tileId )
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

    Kingdom & GetKingdom( PlayerColor color )
    {
        return vec_kingdoms.GetKingdom( color );
    }

    const Kingdom & GetKingdom( PlayerColor color ) const
    {
        return vec_kingdoms.GetKingdom( color );
    }

    void addCastle( int32_t index, uint8_t race, PlayerColor color )
    {
        auto castle = std::make_unique<Castle>( index % width, index / width, race );
        castle->SetColor( color );

        vec_castles.AddCastle( std::move( castle ) );
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

    size_t getCastleCount() const
    {
        return vec_castles.Size();
    }

    void removeCastle( const fheroes2::Point & tilePosition )
    {
        vec_castles.removeCastle( tilePosition );
    }

    // Get castle based on its entrance tile. If the tile is not castle's entrance return nullptr.
    const Castle * getCastleEntrance( const fheroes2::Point & tilePosition ) const;
    Castle * getCastleEntrance( const fheroes2::Point & tilePosition );

    const Heroes * GetHeroes( const int id ) const
    {
        return vec_heroes.Get( id );
    }

    Heroes * GetHeroes( const int id )
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

    Heroes * FromJailHeroes( const int32_t tileIndex )
    {
        return vec_heroes.FromJail( tileIndex );
    }

    Heroes * GetHeroForHire( const int race, const int heroIDToIgnore = Heroes::UNKNOWN ) const
    {
        return vec_heroes.GetHeroForHire( race, heroIDToIgnore );
    }

    const Heroes * GetHeroesCondWins() const;
    const Heroes * GetHeroesCondLoss() const;

    Heroes * GetHero( const Castle & castle ) const
    {
        return vec_heroes.Get( castle.GetCenter() );
    }

    const UltimateArtifact & GetUltimateArtifact() const
    {
        return _ultimateArtifact;
    }

    bool DiggingForUltimateArtifact( const fheroes2::Point & center );

    // overall number of cells of the world map: width * height
    size_t getSize() const
    {
        return vec_tiles.size();
    }

    uint32_t GetDay() const
    {
        return LastDay() ? numOfDaysPerWeek : _day % numOfDaysPerWeek;
    }

    uint32_t GetWeek() const
    {
        return LastWeek() ? numOfWeeksPerMonth : _week % numOfWeeksPerMonth;
    }

    uint32_t GetMonth() const
    {
        return _month;
    }

    uint32_t CountDay() const
    {
        return _day;
    }

    uint32_t CountWeek() const
    {
        return _week;
    }

    bool BeginWeek() const
    {
        return ( ( _day % numOfDaysPerWeek ) == 1 );
    }

    bool BeginMonth() const
    {
        return FirstWeek() && BeginWeek();
    }

    bool LastDay() const
    {
        return ( ( _day % numOfDaysPerWeek ) == 0 );
    }

    bool FirstWeek() const
    {
        return ( ( _week % numOfWeeksPerMonth ) == 1 );
    }

    bool LastWeek() const
    {
        return ( ( _week % numOfWeeksPerMonth ) == 0 );
    }

    const Week & GetWeekType() const;
    std::string DateString() const;

    void NewDay();
    void NewWeek();
    void NewMonth();

    fheroes2::LocalizedString getCurrentRumor() const;

    int32_t NextTeleport( const int32_t index ) const;
    MapsIndexes GetTeleportEndPoints( const int32_t index ) const;

    int32_t NextWhirlpool( const int32_t index ) const;
    MapsIndexes GetWhirlpoolEndPoints( const int32_t index ) const;

    void CaptureObject( const int32_t index, const PlayerColor color );

    uint32_t CountCapturedObject( const MP2::MapObjectType obj, const PlayerColor color ) const
    {
        return map_captureobj.GetCount( obj, color );
    }

    uint32_t CountCapturedMines( const int type, const PlayerColor color ) const;
    uint32_t CountObeliskOnMaps();

    PlayerColor ColorCapturedObject( const int32_t index ) const
    {
        return map_captureobj.GetColor( index );
    }

    void ResetCapturedObjects( const PlayerColor color )
    {
        map_captureobj.ResetColor( color );
    }

    CapturedObject & GetCapturedObject( const int32_t index )
    {
        return map_captureobj.Get( index );
    }

    void ActionForMagellanMaps( const PlayerColor color );
    void ClearFog( PlayerColor color ) const;

    bool KingdomIsWins( const Kingdom & kingdom, const uint32_t wins ) const;
    bool KingdomIsLoss( const Kingdom & kingdom, const uint32_t loss ) const;

    uint32_t CheckKingdomWins( const Kingdom & kingdom ) const;
    uint32_t CheckKingdomLoss( const Kingdom & kingdom ) const;

    void AddEventDate( const EventDate & event )
    {
        vec_eventsday.push_back( event );
    }

    EventsDate GetEventsDate( const PlayerColor color ) const;

    MapEvent * GetMapEvent( const fheroes2::Point & pos );

    MapBaseObject * GetMapObject( const uint32_t uid )
    {
        return uid ? map_objects.get( uid ) : nullptr;
    }

    void RemoveMapObject( const MapBaseObject * obj );
    const MapRegion & getRegion( size_t id ) const;
    size_t getRegionCount() const;

    uint8_t getWaterPercentage() const
    {
        return _waterPercentage;
    }

    double getLandRoughness() const
    {
        return _landRoughness;
    }

    uint32_t getDistance( const Heroes & hero, int targetIndex );
    std::list<Route::Step> getPath( const Heroes & hero, int targetIndex );
    void resetPathfinder();

    void ComputeStaticAnalysis();

    uint32_t GetMapSeed() const
    {
        return _seed;
    }

    uint32_t GetWeekSeed() const;

    bool isAnyKingdomVisited( const MP2::MapObjectType objectType, const int32_t dstIndex ) const;

    void updatePassabilities();

    const std::vector<int32_t> & getAllEyeOfMagiPositions() const
    {
        return _allEyeOfMagi;
    }

    // Update French language-specific characters in all strings to match CP1252.
    // Call this method only when loading maps made with original French editor.
    void fixFrenchCharactersInStrings();

private:
    World() = default;

    void Defaults();
    void Reset();
    void _monthOfMonstersAction( const Monster & mons );

    bool ProcessNewMP2Map( const std::string & filename, const bool checkPoLObjects );

    bool _processNewResurrectionMap( const std::string & filename );

    void PostLoad( const bool setTilePassabilities, const bool updateUidCounterToMaximum );

    bool updateTileMetadata( Maps::Tile & tile, const MP2::MapObjectType objectType, const bool checkPoLObjects );

    bool isValidCastleEntrance( const fheroes2::Point & tilePosition ) const;

    void setUltimateArtifact();

    void tryAddDebugHero();

    bool setHeroIdsForMapConditions();

    friend class Radar;
    friend OStreamBase & operator<<( OStreamBase & stream, const World & w );
    friend IStreamBase & operator>>( IStreamBase & stream, World & w );

    std::vector<Maps::Tile> vec_tiles;
    AllHeroes vec_heroes;
    AllCastles vec_castles;
    Kingdoms vec_kingdoms;
    std::vector<std::string> _customRumors;
    EventsDate vec_eventsday;

    // index, object, color
    CapturedObjects map_captureobj;

    UltimateArtifact _ultimateArtifact;

    uint32_t _day = 0;
    uint32_t _week = 0;
    uint32_t _month = 0;

    int32_t heroIdAsWinCondition = Heroes::UNKNOWN;
    int32_t heroIdAsLossCondition = Heroes::UNKNOWN;

    MapObjects map_objects;

    uint32_t _seed{ 0 }; // Map seed

    // The following fields are not serialized

    std::map<uint8_t, Maps::Indexes> _allTeleports; // All indexes of tiles that contain stone liths of a certain type (sprite index)
    std::map<uint8_t, Maps::Indexes> _allWhirlpools; // All indexes of tiles that contain a certain part (sprite index) of the whirlpool
    std::vector<int32_t> _allEyeOfMagi;

    uint8_t _waterPercentage{ 0 };
    double _landRoughness{ 1.0 };
    std::vector<MapRegion> _regions;
    PlayerWorldPathfinder _pathfinder;
};

OStreamBase & operator<<( OStreamBase & stream, const CapturedObject & obj );
IStreamBase & operator>>( IStreamBase & stream, CapturedObject & obj );

extern World & world;
