/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "multi_contact.h"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <map>
#include <utility>

#include "castle.h"
#include "game_static.h"
#include "ground.h"
#include "heroes.h"
#include "kingdom.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "mp2.h"
#include "players.h"
#include "world.h"
#include "world_pathfinding.h"

namespace
{
    // Rounds of growth (prisoners, castles, movement bonuses). If it still grows after that, there is no simultaneous day.
    constexpr uint32_t maxRounds{ 4 };

    // A movement bonus can be visited again after a battle
    constexpr uint32_t maxBattlesPerBonus{ 3 };

    // Map objects which matter for the contact, found in one scan
    struct MapScan
    {
        std::vector<int32_t> castles;
        std::vector<int32_t> jails;
        std::vector<int32_t> monsters;
        std::vector<std::pair<int32_t, uint32_t>> movementBonuses;
        std::vector<std::pair<int32_t, PlayerColor>> capturedObjects;

        // Stone liths and whirlpools of the same kind lead to each other.
        std::vector<std::vector<int32_t>> teleportGroups;
        std::vector<int32_t> teleportGroupOfTile;
    };

    // Stone liths and whirlpools lead to the others with the same image, on land or on water.
    struct TeleportKind
    {
        MP2::MapObjectType type;
        uint8_t icnIndex;
        bool isWater;

        bool operator<( const TeleportKind & other ) const
        {
            if ( type != other.type ) {
                return type < other.type;
            }
            if ( icnIndex != other.icnIndex ) {
                return icnIndex < other.icnIndex;
            }
            return isWater < other.isWater;
        }
    };

    MapScan scanMap()
    {
        MapScan scan;

        const int32_t tileCount = static_cast<int32_t>( world.getSize() );
        scan.teleportGroupOfTile.assign( tileCount, -1 );

        std::map<TeleportKind, int32_t> groupIds;

        for ( int32_t index = 0; index < tileCount; ++index ) {
            const Maps::Tile & tile = world.getTile( index );
            const MP2::MapObjectType type = tile.getMainObjectType( false );

            switch ( type ) {
            case MP2::OBJ_CASTLE:
                if ( world.getCastleEntrance( Maps::GetPoint( index ) ) != nullptr ) {
                    scan.castles.push_back( index );
                }
                break;
            case MP2::OBJ_JAIL:
                scan.jails.push_back( index );
                break;
            case MP2::OBJ_MONSTER:
                scan.monsters.push_back( index );
                break;
            case MP2::OBJ_STONE_LITHS:
            case MP2::OBJ_WHIRLPOOL: {
                const Maps::ObjectPart * part = Maps::getObjectPartByActionType( tile, type );
                if ( part == nullptr ) {
                    break;
                }

                const auto [group, isNew]
                    = groupIds.try_emplace( TeleportKind{ type, part->icnIndex, tile.isWater() }, static_cast<int32_t>( scan.teleportGroups.size() ) );
                if ( isNew ) {
                    scan.teleportGroups.emplace_back();
                }

                scan.teleportGroups[group->second].push_back( index );
                scan.teleportGroupOfTile[index] = group->second;
                break;
            }
            default:
                if ( const uint32_t bonus = GameStatic::getMovementPointBonus( type ); bonus > 0 ) {
                    scan.movementBonuses.emplace_back( index, bonus );
                }
                break;
            }

            if ( MP2::isCaptureObject( type ) ) {
                if ( const PlayerColor color = world.ColorCapturedObject( index ); color != PlayerColor::NONE ) {
                    scan.capturedObjects.emplace_back( index, color );
                }
            }
        }

        return scan;
    }

    // A hero, or a hero which could be hired or freed today
    struct Source
    {
        int32_t index{ -1 };
        uint32_t movePoints{ 0 };
        uint32_t maxMovePoints{ 0 };

        Multi::TileSet reach;
        bool isExplored{ false };

        std::vector<int32_t> bonusesTaken;
    };

    // Wider than the real rules. Fog, zones of control, heroes and monsters never stop the hero, teleports lead to every
    // exit and Pathfinding is always expert.
    class ContactPathfinder final : public WorldPathfinder
    {
    public:
        explicit ContactPathfinder( const MapScan & scan )
            : _scan( scan )
        {}

        void explore( Source & source )
        {
            reset();

            _pathStart = source.index;
            _remainingMovePoints = source.movePoints;
            _maxMovePoints = std::max( { source.maxMovePoints, source.movePoints, 1U } );
            _isOnWater = world.getTile( source.index ).isWater();

            processWorldMap();

            source.reach = Multi::TileSet( static_cast<int32_t>( _cache.size() ) );

            for ( size_t i = 0; i < _cache.size(); ++i ) {
                const int32_t index = static_cast<int32_t>( i );

                if ( index == _pathStart || ( _cache[i]._from != -1 && _cache[i]._cost <= source.movePoints ) ) {
                    source.reach.insert( index );
                }
            }

            source.isExplored = true;
        }

    private:
        void processCurrentNode( std::vector<int> & nodesToExplore, const int currentNodeIdx ) override
        {
            if ( currentNodeIdx != _pathStart ) {
                const WorldNode & node = _cache[currentNodeIdx];

                if ( node._cost > _remainingMovePoints || !_isCrossable( currentNodeIdx ) ) {
                    return;
                }

                if ( const int32_t group = _scan.teleportGroupOfTile[currentNodeIdx]; group >= 0 ) {
                    for ( const int32_t exitIndex : _scan.teleportGroups[group] ) {
                        if ( exitIndex == currentNodeIdx || exitIndex == _pathStart ) {
                            continue;
                        }

                        WorldNode & exitNode = _cache[exitIndex];
                        if ( exitNode._from == -1 || exitNode._cost > node._cost ) {
                            exitNode.update( currentNodeIdx, node._cost, node._remainingMovePoints );
                            nodesToExplore.push_back( exitIndex );
                        }
                    }
                }
            }

            checkAdjacentNodes( nodesToExplore, currentNodeIdx );
        }

        uint32_t getMaxMovePoints( const bool /* onWater */ ) const override
        {
            return _maxMovePoints;
        }

        // The move ends at a boat, at a shore to land on and at objects visited from the next tile
        bool _isCrossable( const int32_t index ) const
        {
            const Maps::Tile & tile = world.getTile( index );
            if ( tile.isWater() != _isOnWater ) {
                return false;
            }

            const MP2::MapObjectType type = tile.getMainObjectType();
            if ( type == MP2::OBJ_HERO || type == MP2::OBJ_MONSTER || type == MP2::OBJ_CASTLE || MP2::isPickupObject( type ) ) {
                return true;
            }

            return !MP2::isNeedStayFront( type );
        }

        const MapScan & _scan;
        uint32_t _maxMovePoints{ 0 };
        bool _isOnWater{ false };
    };

    bool canHireHeroes( const Castle & castle )
    {
        return castle.isCastle() || !castle.isBuildingDisabled( BUILD_CASTLE );
    }

    // For a hero hired today
    uint32_t getHireMovePoints()
    {
        uint32_t result = 0;

        for ( int32_t id = Heroes::UNKNOWN + 1; id < Heroes::HEROES_COUNT; ++id ) {
            const Heroes * hero = world.GetHeroes( id );
            if ( hero == nullptr || !hero->isAvailableForHire() ) {
                continue;
            }

            result = std::max( result, hero->Modes( Heroes::SAVEMP ) ? hero->GetMovePoints() : hero->GetMaxMovePoints( false ) );
        }

        return result;
    }

    // Heroes, castles and captured objects
    Multi::TileSet getAssets( const Kingdom & kingdom, const MapScan & scan, const int32_t tileCount )
    {
        Multi::TileSet assets( tileCount );

        for ( const Heroes * hero : kingdom.GetHeroes() ) {
            assets.insert( hero->GetIndex() );
        }

        for ( const Castle * castle : kingdom.GetCastles() ) {
            Multi::insertCastleFootprint( assets, castle->GetCenter() );
        }

        for ( const auto & [index, color] : scan.capturedObjects ) {
            if ( color == kingdom.GetColor() ) {
                assets.insert( index );
            }
        }

        return assets;
    }

    // Monsters close enough to fight them and come back to the bonus with some profit
    uint32_t countBattlesForBonus( const int32_t bonusIndex, const uint32_t bonus, const MapScan & scan )
    {
        const fheroes2::Point bonusPoint = Maps::GetPoint( bonusIndex );
        uint32_t count = 0;

        for ( const int32_t monsterIndex : scan.monsters ) {
            const fheroes2::Point monsterPoint = Maps::GetPoint( monsterIndex );
            const uint32_t steps = static_cast<uint32_t>( std::max( std::abs( bonusPoint.x - monsterPoint.x ), std::abs( bonusPoint.y - monsterPoint.y ) ) );

            if ( steps * Maps::Ground::roadPenalty <= bonus / 2 && ++count == maxBattlesPerBonus ) {
                break;
            }
        }

        return count;
    }

    Multi::PlayerReach computeReach( const PlayerColor color, const MapScan & scan, ContactPathfinder & pathfinder, const uint32_t hireMovePoints )
    {
        const int32_t tileCount = static_cast<int32_t>( world.getSize() );
        const Kingdom & kingdom = world.GetKingdom( color );

        Multi::PlayerReach reach;
        reach.color = color;

        std::vector<Source> sources;
        Multi::TileSet unlocked( tileCount );

        const auto addHirePoint = [&sources, hireMovePoints]( const int32_t index ) {
            if ( hireMovePoints > 0 ) {
                sources.push_back( { index, hireMovePoints, hireMovePoints, {}, false, {} } );
            }
        };

        for ( const Heroes * hero : kingdom.GetHeroes() ) {
            if ( Maps::isValidAbsIndex( hero->GetIndex() ) ) {
                sources.push_back( { hero->GetIndex(), hero->GetMovePoints(), hero->GetMaxMovePoints(), {}, false, {} } );
                ++reach.heroSources;
            }
        }

        for ( const Castle * castle : kingdom.GetCastles() ) {
            unlocked.insert( castle->GetIndex() );

            if ( canHireHeroes( *castle ) ) {
                addHirePoint( castle->GetIndex() );
                ++reach.hireSources;
            }
        }

        for ( uint32_t round = 1; round <= maxRounds; ++round ) {
            reach.rounds = round;
            reach.stand = Multi::TileSet( tileCount );

            for ( Source & source : sources ) {
                if ( !source.isExplored ) {
                    pathfinder.explore( source );
                }

                reach.stand.insertAll( source.reach );
            }

            bool isGrowing = false;

            // A captured castle hires heroes too.
            for ( const int32_t index : scan.castles ) {
                if ( unlocked.contains( index ) || !reach.stand.contains( index ) ) {
                    continue;
                }

                unlocked.insert( index );

                const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );
                if ( castle != nullptr && canHireHeroes( *castle ) ) {
                    addHirePoint( index );
                    ++reach.castleUnlocks;
                    isGrowing = true;
                }
            }

            for ( const int32_t index : scan.jails ) {
                if ( unlocked.contains( index ) || !reach.stand.contains( index ) ) {
                    continue;
                }

                unlocked.insert( index );

                if ( const Heroes * prisoner = world.FromJailHeroes( index ); prisoner != nullptr ) {
                    const uint32_t maxMovePoints = prisoner->GetMaxMovePoints( false );
                    sources.push_back( { index, std::max( prisoner->GetMovePoints(), maxMovePoints ), maxMovePoints, {}, false, {} } );
                    ++reach.jailSources;
                    isGrowing = true;
                }
            }

            for ( const auto & [index, bonus] : scan.movementBonuses ) {
                for ( Source & source : sources ) {
                    if ( !source.isExplored || !source.reach.contains( index )
                         || std::find( source.bonusesTaken.begin(), source.bonusesTaken.end(), index ) != source.bonusesTaken.end() ) {
                        continue;
                    }

                    source.bonusesTaken.push_back( index );
                    source.movePoints += bonus * ( 1 + countBattlesForBonus( index, bonus, scan ) );
                    source.isExplored = false;

                    ++reach.movementBonuses;
                    isGrowing = true;
                }
            }

            if ( !isGrowing ) {
                break;
            }

            if ( round == maxRounds ) {
                reach.isRoundLimitReached = true;
            }
        }

        return reach;
    }
}

Multi::TileSet::TileSet( const int32_t tileCount )
    : _bits( static_cast<size_t>( std::max( tileCount, 0 ) + 7 ) / 8, 0 )
    , _tileCount( std::max( tileCount, 0 ) )
{}

bool Multi::TileSet::contains( const int32_t index ) const
{
    return index >= 0 && index < _tileCount && ( ( _bits[index >> 3] >> ( index & 7 ) ) & 1 ) != 0;
}

void Multi::TileSet::insert( const int32_t index )
{
    if ( index >= 0 && index < _tileCount ) {
        _bits[index >> 3] |= static_cast<uint8_t>( 1 << ( index & 7 ) );
    }
}

void Multi::TileSet::insertAll( const TileSet & other )
{
    assert( other._tileCount == _tileCount );

    const size_t size = std::min( _bits.size(), other._bits.size() );
    for ( size_t i = 0; i < size; ++i ) {
        _bits[i] |= other._bits[i];
    }
}

int32_t Multi::TileSet::firstCommonTile( const TileSet & other ) const
{
    const size_t size = std::min( _bits.size(), other._bits.size() );

    for ( size_t i = 0; i < size; ++i ) {
        const uint8_t common = _bits[i] & other._bits[i];
        if ( common == 0 ) {
            continue;
        }

        for ( int32_t bit = 0; bit < 8; ++bit ) {
            if ( ( common >> bit ) & 1 ) {
                return static_cast<int32_t>( i * 8 ) + bit;
            }
        }
    }

    return -1;
}

Multi::TileSet Multi::TileSet::dilated() const
{
    TileSet result( _tileCount );

    for ( int32_t index = 0; index < _tileCount; ++index ) {
        if ( !contains( index ) ) {
            continue;
        }

        const fheroes2::Point point = Maps::GetPoint( index );

        for ( int32_t dy = -1; dy <= 1; ++dy ) {
            for ( int32_t dx = -1; dx <= 1; ++dx ) {
                if ( Maps::isValidAbsPoint( point.x + dx, point.y + dy ) ) {
                    result.insert( Maps::GetIndexFromAbsPoint( point.x + dx, point.y + dy ) );
                }
            }
        }
    }

    return result;
}

size_t Multi::TileSet::count() const
{
    size_t result = 0;

    for ( const uint8_t byte : _bits ) {
        result += std::bitset<8>( byte ).count();
    }

    return result;
}

Multi::ContactVerdict Multi::computeContactVerdict( const std::vector<PlayerColorsSet> & computerColors )
{
    const auto start = std::chrono::steady_clock::now();

    ContactVerdict verdict;

    const auto colorsInPlay = []( const PlayerColorsSet colors ) {
        std::vector<PlayerColor> result;

        for ( const PlayerColor color : PlayerColorsVector( colors ) ) {
            if ( world.GetKingdom( color ).isPlay() ) {
                result.push_back( color );
            }
        }

        return result;
    };

    std::vector<std::vector<PlayerColor>> players;
    PlayerColorsSet humanColors = 0;

    for ( const PlayerColorsSet colors : computerColors ) {
        if ( std::vector<PlayerColor> inPlay = colorsInPlay( colors ); !inPlay.empty() ) {
            players.push_back( std::move( inPlay ) );
            humanColors |= colors;
        }
    }

    if ( players.size() < 2 ) {
        return verdict;
    }

    const int32_t tileCount = static_cast<int32_t>( world.getSize() );
    const MapScan scan = scanMap();
    const uint32_t hireMovePoints = getHireMovePoints();

    // Defeating the last hero or castle of a computer player changes things all over the map.
    std::vector<TileSet> computerAssets;
    for ( const PlayerColor color : colorsInPlay( static_cast<PlayerColorsSet>( Color::allPlayerColors() & ~humanColors ) ) ) {
        computerAssets.push_back( getAssets( world.GetKingdom( color ), scan, tileCount ) );
    }

    ContactPathfinder pathfinder( scan );

    for ( const PlayerColor color : PlayerColorsVector( humanColors ) ) {
        if ( !world.GetKingdom( color ).isPlay() ) {
            continue;
        }

        PlayerReach reach = computeReach( color, scan, pathfinder, hireMovePoints );

        TileSet touch = reach.stand;
        touch.insertAll( getAssets( world.GetKingdom( color ), scan, tileCount ) );

        for ( const int32_t index : scan.castles ) {
            if ( reach.stand.contains( index ) ) {
                insertCastleFootprint( touch, Maps::GetPoint( index ) );
            }
        }

        for ( const TileSet & assets : computerAssets ) {
            if ( reach.stand.firstCommonTile( assets ) != -1 ) {
                touch.insertAll( assets );
            }
        }

        reach.touch = touch.dilated();

        verdict.reaches.push_back( std::move( reach ) );
    }

    const auto findReach = [&verdict]( const PlayerColor color ) -> const PlayerReach & {
        const auto iter = std::find_if( verdict.reaches.begin(), verdict.reaches.end(), [color]( const PlayerReach & reach ) { return reach.color == color; } );
        assert( iter != verdict.reaches.end() );

        return *iter;
    };

    verdict.result = ContactResult::SIMULTANEOUS;

    for ( const PlayerReach & reach : verdict.reaches ) {
        if ( reach.isRoundLimitReached ) {
            verdict.result = ContactResult::ROUND_LIMIT;
            verdict.witnessColors[0] = reach.color;
        }
    }

    // Every pair of players of two different computers.
    for ( size_t first = 0; first < players.size(); ++first ) {
        for ( size_t second = first + 1; second < players.size(); ++second ) {
            for ( const PlayerColor firstColor : players[first] ) {
                for ( const PlayerColor secondColor : players[second] ) {
                    const int32_t tile = findReach( firstColor ).touch.firstCommonTile( findReach( secondColor ).touch );
                    if ( tile == -1 ) {
                        continue;
                    }

                    if ( Players::isFriends( firstColor, static_cast<PlayerColorsSet>( secondColor ) ) ) {
                        verdict.alliedOverlaps |= static_cast<PlayerColorsSet>( firstColor ) | static_cast<PlayerColorsSet>( secondColor );
                    }

                    if ( verdict.result == ContactResult::SIMULTANEOUS ) {
                        verdict.result = ContactResult::CONTACT;
                        verdict.witnessTile = tile;
                        verdict.witnessColors[0] = firstColor;
                        verdict.witnessColors[1] = secondColor;
                    }
                }
            }
        }
    }

    verdict.milliseconds = static_cast<uint32_t>( std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - start ).count() );

    return verdict;
}

void Multi::insertCastleFootprint( TileSet & tiles, const fheroes2::Point & center )
{
    for ( int32_t dy = -3; dy <= 2; ++dy ) {
        for ( int32_t dx = -5; dx <= 2; ++dx ) {
            if ( Maps::isValidAbsPoint( center.x + dx, center.y + dy ) ) {
                tiles.insert( Maps::GetIndexFromAbsPoint( center.x + dx, center.y + dy ) );
            }
        }
    }
}

const char * Multi::contactResultName( const ContactResult result )
{
    switch ( result ) {
    case ContactResult::SIMULTANEOUS:
        return "simultaneous";
    case ContactResult::CONTACT:
        return "contact";
    case ContactResult::ROUND_LIMIT:
        return "round limit";
    case ContactResult::NO_PLAYER:
        return "no player";
    default:
        break;
    }

    return "unknown";
}
