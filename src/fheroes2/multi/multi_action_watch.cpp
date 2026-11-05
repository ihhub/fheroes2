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

#include "multi_action_watch.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <sstream>
#include <vector>

#include "castle.h"
#include "heroes.h"
#include "kingdom.h"
#include "logging.h"
#include "maps.h"
#include "maps_objects.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"
#include "multi_contact.h"
#include "multi_game.h"
#include "multi_reservation.h"
#include "multi_world_changes.h"
#include "serialize.h"
#include "world.h"

namespace
{
    struct WatchState
    {
        bool isEnabled{ false };
        Multi::ActionWatchStats stats;

        Multi::WorldSnapshot last;
        PlayerColor color{ PlayerColor::NONE };

        // Set between the checks before and after the step
        std::optional<Multi::ReservationZone> stepZone;
        int32_t target{ -1 };
    };

    WatchState & watchState()
    {
        static WatchState state;
        return state;
    }

    bool isWatchedColor( const PlayerColor color )
    {
        return watchState().isEnabled && Multi::isNetworkGame() && ( Multi::getLocalHumanColors() & color ) != 0;
    }

    std::string describeTile( const int32_t index )
    {
        const fheroes2::Point point = Maps::GetPoint( index );

        std::ostringstream os;
        os << "[" << point.x << ", " << point.y << "] " << MP2::StringObject( world.getTile( index ).getMainObjectType( false ) );
        return os.str();
    }

    template <typename Unit>
    void readUnit( const Multi::WorldSnapshot & snapshot, const std::vector<Multi::ByteRange> & ranges, const size_t index, Unit & unit )
    {
        if ( index >= ranges.size() ) {
            return;
        }

        const std::vector<uint8_t> data( snapshot.data().begin() + static_cast<std::ptrdiff_t>( ranges[index].begin ),
                                         snapshot.data().begin() + static_cast<std::ptrdiff_t>( ranges[index].end ) );

        const Multi::SaveVersionGuard versionGuard;

        ROStreamBuf stream( data );
        stream.setBigendian( true );
        stream >> unit;
    }

    // Color and tile
    std::pair<PlayerColor, int32_t> getHeroInfo( const Multi::WorldSnapshot & snapshot, const int32_t id )
    {
        Heroes hero;
        readUnit( snapshot, snapshot.parts().heroes, static_cast<size_t>( id ), hero );
        return { hero.GetColor(), hero.GetIndex() };
    }

    // From the previous snapshot if the object was removed
    int32_t getMapObjectIndex( const Multi::WorldSnapshot & before, const uint32_t uid )
    {
        if ( const MapBaseObject * object = world.GetMapObject( uid ); object != nullptr ) {
            return object->GetIndex();
        }

        const Multi::SaveVersionGuard versionGuard;

        for ( const Multi::ByteRange & range : before.parts().mapObjects ) {
            const std::vector<uint8_t> data( before.data().begin() + static_cast<std::ptrdiff_t>( range.begin ),
                                             before.data().begin() + static_cast<std::ptrdiff_t>( range.end ) );

            ROStreamBuf stream( data );
            stream.setBigendian( true );

            uint32_t objectUid = 0;
            MP2::MapObjectType type = MP2::OBJ_NONE;
            stream >> objectUid >> type;

            if ( objectUid != uid ) {
                continue;
            }

            switch ( type ) {
            case MP2::OBJ_EVENT: {
                MapEvent object;
                stream >> object;
                return object.GetIndex();
            }
            case MP2::OBJ_SPHINX: {
                MapSphinx object;
                stream >> object;
                return object.GetIndex();
            }
            case MP2::OBJ_SIGN: {
                MapSign object;
                stream >> object;
                return object.GetIndex();
            }
            default:
                return -1;
            }
        }

        return -1;
    }

    // Color and tile of the center
    std::pair<PlayerColor, int32_t> getCastleInfo( const Multi::WorldSnapshot & snapshot, const int32_t index )
    {
        Castle castle;
        readUnit( snapshot, snapshot.parts().castles, static_cast<size_t>( index ), castle );
        return { castle.GetColor(), Maps::GetIndexFromAbsPoint( castle.GetCenter() ) };
    }

    // Between two steps a player can build in its castles and buy boats
    Multi::TileSet getCastleTiles( const PlayerColor color )
    {
        Multi::TileSet tiles( static_cast<int32_t>( world.getSize() ) );

        for ( const Castle * castle : world.GetKingdom( color ).GetCastles() ) {
            Multi::insertCastleFootprint( tiles, castle->GetCenter() );
        }

        return tiles;
    }

    void insertAround( Multi::TileSet & tiles, const int32_t index )
    {
        if ( !Maps::isValidAbsIndex( index ) ) {
            return;
        }

        const fheroes2::Point center = Maps::GetPoint( index );
        for ( int32_t dy = -1; dy <= 1; ++dy ) {
            for ( int32_t dx = -1; dx <= 1; ++dx ) {
                if ( Maps::isValidAbsPoint( center.x + dx, center.y + dy ) ) {
                    tiles.insert( Maps::GetIndexFromAbsPoint( center.x + dx, center.y + dy ) );
                }
            }
        }
    }

    // Against what the step would have reserved, or without a step against what belongs to the player
    void checkChanges( const Multi::WorldSnapshot & before, const Multi::WorldSnapshot & after, const Multi::WorldChanges & changes )
    {
        WatchState & state = watchState();
        Multi::ActionWatchStats & stats = state.stats;

        const PlayerColor color = state.color;
        const Multi::ReservationZone * zone = state.stepZone ? &*state.stepZone : nullptr;

        if ( zone != nullptr && zone->isBattleWithOtherComputer ) {
            // Arranged with the other computer, its player is stopped meanwhile
            ++stats.battlesWithOtherComputer;
            return;
        }

        const auto isReservedKingdom = [zone, color]( const PlayerColor owner ) { return owner == color || ( zone != nullptr && ( zone->kingdoms & owner ) != 0 ); };

        std::ostringstream holes;
        uint32_t holeCount = 0;

        const auto addHole = [&holes, &holeCount]( const std::string & text ) {
            if ( ++holeCount <= 4 ) {
                holes << " " << text << ";";
            }
        };

        // Without a step, look around its castles and where its heroes jumped with a travel spell
        Multi::TileSet ownTiles = ( zone == nullptr ) ? getCastleTiles( color ) : Multi::TileSet();
        if ( zone == nullptr ) {
            for ( const int32_t id : changes.heroes ) {
                const auto [colorBefore, indexBefore] = getHeroInfo( before, id );
                const auto [colorAfter, indexAfter] = getHeroInfo( after, id );

                if ( colorBefore == color && colorAfter == color && indexBefore != indexAfter ) {
                    ++stats.spellMoves;
                    insertAround( ownTiles, indexBefore );
                    insertAround( ownTiles, indexAfter );
                }
            }
        }

        const Multi::TileSet & allowedTiles = ( zone != nullptr ) ? zone->tiles : ownTiles;

        uint32_t bytes = static_cast<uint32_t>( changes.fogTiles.size() ) * 4;
        const auto addBytes = [&bytes]( const std::vector<Multi::ByteRange> & ranges, const int32_t index ) {
            if ( index >= 0 && static_cast<size_t>( index ) < ranges.size() ) {
                bytes += static_cast<uint32_t>( ranges[index].end - ranges[index].begin );
            }
        };

        for ( const int32_t index : changes.tiles ) {
            addBytes( after.parts().tiles, index );

            if ( !allowedTiles.contains( index ) ) {
                addHole( "tile " + describeTile( index ) );
            }
        }

        for ( const int32_t id : changes.heroes ) {
            addBytes( after.parts().heroes, id );

            const PlayerColor colorBefore = getHeroInfo( before, id ).first;
            const PlayerColor colorAfter = getHeroInfo( after, id ).first;

            if ( zone == nullptr && colorBefore == PlayerColor::NONE && colorAfter == PlayerColor::NONE ) {
                ++stats.tavernChanges;
            }
            else if ( !isReservedKingdom( colorBefore ) && !isReservedKingdom( colorAfter ) && ( zone == nullptr || colorBefore != PlayerColor::NONE ) ) {
                addHole( "hero " + std::to_string( id ) + " of " + Color::String( colorBefore ) + " now " + Color::String( colorAfter ) );
            }
        }

        for ( const int32_t index : changes.castles ) {
            addBytes( after.parts().castles, index );

            const auto [colorBefore, center] = getCastleInfo( before, index );
            const PlayerColor colorAfter = getCastleInfo( after, index ).first;
            const bool isNeutralTaken = ( zone != nullptr && colorBefore == PlayerColor::NONE && zone->tiles.contains( center ) );

            if ( !isReservedKingdom( colorBefore ) && !isReservedKingdom( colorAfter ) && !isNeutralTaken ) {
                addHole( "castle " + describeTile( center ) + " of " + Color::String( colorBefore ) + " now " + Color::String( colorAfter ) );
            }
        }

        for ( const int32_t index : changes.kingdoms ) {
            addBytes( after.parts().kingdoms, index );

            const PlayerColor owner = ( index < 6 ) ? Color::IndexToColor( index ) : PlayerColor::NONE;
            if ( !isReservedKingdom( owner ) ) {
                addHole( "kingdom of " + Color::String( owner ) );
            }
        }

        for ( const int32_t index : changes.capturedObjects ) {
            bytes += 16;

            if ( !allowedTiles.contains( index ) ) {
                addHole( "captured object " + describeTile( index ) );
            }
        }

        // Events, sphinxes and signs are visited on their tile.
        for ( const uint32_t uid : changes.mapObjects ) {
            const int32_t index = getMapObjectIndex( before, uid );
            if ( !allowedTiles.contains( index ) ) {
                addHole( "map object " + std::to_string( uid ) + ( Maps::isValidAbsIndex( index ) ? " " + describeTile( index ) : std::string() ) );
            }
        }

        if ( changes.isRumorsChanged || changes.isEventsChanged || changes.isUltimateArtifactChanged || changes.isDateChanged || changes.isSeedChanged
             || changes.isLayoutChanged ) {
            addHole( std::string( "global:" ) + ( changes.isRumorsChanged ? " rumors" : "" ) + ( changes.isEventsChanged ? " events" : "" )
                     + ( changes.isUltimateArtifactChanged ? " ultimate artifact" : "" ) + ( changes.isDateChanged ? " date" : "" )
                     + ( changes.isSeedChanged ? " seed" : "" ) + ( changes.isLayoutChanged ? " layout" : "" ) );
        }

        if ( zone != nullptr ) {
            ++stats.steps;
            stats.totalChangedBytes += bytes;
            stats.maxChangedBytes = std::max( stats.maxChangedBytes, bytes );

            if ( zone->kingdoms != 0 ) {
                ++stats.computerPlayerReservations;
            }
        }

        if ( holeCount > 0 ) {
            stats.holes += holeCount;

            const std::string what = ( zone != nullptr ) ? "step to " + describeTile( state.target ) : std::string( "between steps" );
            VERBOSE_LOG( "[MULTI] Action watch hole, " << Color::String( color ) << ", " << what << ":" << holes.str() )
        }
    }

    void check( const PlayerColor color )
    {
        WatchState & state = watchState();

        const auto start = std::chrono::steady_clock::now();

        Multi::WorldSnapshot now;
        if ( const std::string error = now.capture(); !error.empty() ) {
            ERROR_LOG( "[MULTI] Action watch stopped: " << error )
            state.isEnabled = false;
            return;
        }

        if ( state.last.isValid() && state.color == color ) {
            const Multi::WorldChanges changes = Multi::compareSnapshots( state.last, now );
            if ( !changes.empty() || state.stepZone ) {
                checkChanges( state.last, now, changes );
            }
        }

        const uint32_t microseconds = static_cast<uint32_t>( std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::steady_clock::now() - start ).count() );
        ++state.stats.checks;
        state.stats.totalMicroseconds += microseconds;
        state.stats.maxMicroseconds = std::max( state.stats.maxMicroseconds, microseconds );

        state.last = std::move( now );
        state.color = color;
        state.stepZone.reset();
        state.target = -1;
    }
}

void Multi::enableActionWatch()
{
    watchState().isEnabled = true;
}

bool Multi::isActionWatchEnabled()
{
    return watchState().isEnabled;
}

void Multi::watchTurnBegin( const PlayerColor color )
{
    if ( !isWatchedColor( color ) ) {
        return;
    }

    WatchState & state = watchState();
    state.last = {};
    state.color = PlayerColor::NONE;

    check( color );
}

void Multi::watchTurnEnd( const PlayerColor color )
{
    if ( !isWatchedColor( color ) ) {
        return;
    }

    check( color );

    WatchState & state = watchState();
    state.last = {};
    state.color = PlayerColor::NONE;
}

void Multi::watchHeroStep( const Heroes & hero, const int32_t targetIndex )
{
    if ( !isWatchedColor( hero.GetColor() ) ) {
        return;
    }

    // Changes since the last check were made between two steps
    check( hero.GetColor() );

    WatchState & state = watchState();
    state.stepZone = getStepZone( hero, targetIndex, getRemoteHumanColors() );
    state.target = targetIndex;
}

void Multi::watchHeroStepDone( const Heroes & hero )
{
    const WatchState & state = watchState();
    if ( !state.stepZone || state.color != hero.GetColor() ) {
        return;
    }

    check( hero.GetColor() );
}

void Multi::watchRebaseline()
{
    WatchState & state = watchState();
    if ( !state.isEnabled || !state.last.isValid() ) {
        return;
    }

    if ( const std::string error = state.last.capture(); !error.empty() ) {
        ERROR_LOG( "[MULTI] Action watch stopped: " << error )
        state.isEnabled = false;
    }
}

const Multi::ActionWatchStats & Multi::getActionWatchStats()
{
    return watchState().stats;
}

std::string Multi::describeActionWatch()
{
    const ActionWatchStats & stats = watchState().stats;

    std::ostringstream os;
    os << stats.steps << " step(s), " << stats.checks << " check(s), " << ( stats.checks > 0 ? stats.totalMicroseconds / stats.checks / 1000 : 0 ) << " ms on average, "
       << stats.maxMicroseconds / 1000 << " ms at most, changed units " << ( stats.steps > 0 ? stats.totalChangedBytes / stats.steps : 0 )
       << " bytes per step on average, " << stats.maxChangedBytes << " at most; " << stats.battlesWithOtherComputer << " battle(s) with the other computer, "
       << stats.computerPlayerReservations << " step(s) reserving a computer player, " << stats.tavernChanges << " tavern change(s), " << stats.spellMoves
       << " spell move(s), " << stats.holes << " hole(s)";

    return os.str();
}
