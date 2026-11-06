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

#include "multi_live.h"

#include <algorithm>
#include <list>
#include <map>
#include <optional>
#include <sstream>
#include <vector>

#include "castle.h"
#include "color.h"
#include "heroes.h"
#include "kingdom.h"
#include "logging.h"
#include "maps.h"
#include "maps_objects.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"
#include "multi_action_watch.h"
#include "multi_fingerprint.h"
#include "multi_game.h"
#include "multi_live_battle.h"
#include "multi_machines.h"
#include "multi_network.h"
#include "multi_protocol.h"
#include "multi_save_parts.h"
#include "multi_simultaneous.h"
#include "multi_spectator.h"
#include "multi_world_changes.h"
#include "players.h"
#include "route.h"
#include "serialize.h"
#include "settings.h"
#include "world.h"

namespace
{
    struct LiveState
    {
        // The local world as last sent, plus the units received since then. What differs from it is not sent yet.
        Multi::WorldSnapshot sentWorld;

        // The world of the computer which plays, used while only one computer plays. Each update must give its exact hash.
        Multi::WorldSnapshot otherWorld;

        // Set if the updates of the other computer can be applied locally.
        bool isInSync{ false };

        uint32_t sequence{ 0 };

        Multi::LiveStats stats;
    };

    LiveState & liveState()
    {
        static LiveState state;
        return state;
    }

    std::vector<uint8_t> getUnitBytes( const Multi::WorldSnapshot & snapshot, const Multi::ByteRange & range )
    {
        return { snapshot.data().begin() + static_cast<std::ptrdiff_t>( range.begin ), snapshot.data().begin() + static_cast<std::ptrdiff_t>( range.end ) };
    }

    uint32_t readKey( const std::vector<uint8_t> & data )
    {
        if ( data.size() < 4 ) {
            return 0;
        }

        return ( static_cast<uint32_t>( data[0] ) << 24 ) | ( static_cast<uint32_t>( data[1] ) << 16 ) | ( static_cast<uint32_t>( data[2] ) << 8 ) | data[3];
    }

    template <typename Unit>
    bool readWholeUnit( const std::vector<uint8_t> & data, Unit & unit )
    {
        ROStreamBuf stream( data );
        stream.setBigendian( true );
        stream >> unit;

        return !stream.fail() && stream.size() == 0;
    }

    // Returns the tile index of the center of the castle written in these bytes, or -1.
    int32_t getCastleKey( const std::vector<uint8_t> & data )
    {
        const Multi::SaveVersionGuard versionGuard;

        Castle castle;
        if ( !readWholeUnit( data, castle ) ) {
            return -1;
        }

        return Maps::GetIndexFromAbsPoint( castle.GetCenter() );
    }

    Kingdom & getKingdomByIndex( const int32_t index )
    {
        // The kingdom of neutral objects comes after the six players.
        return world.GetKingdom( index < maxNumOfPlayers ? Color::IndexToColor( index ) : PlayerColor::NONE );
    }

    std::vector<uint8_t> writeTile( const Maps::Tile & tile )
    {
        RWStreamBuf stream;
        stream.setBigendian( true );
        stream << tile;

        return { stream.data(), stream.data() + stream.size() };
    }

    uint8_t getFogByte( const Maps::Tile & tile )
    {
        const std::optional<size_t> & offset = Multi::getTileFogOffset();
        const std::vector<uint8_t> bytes = writeTile( tile );

        return ( offset && *offset < bytes.size() ) ? bytes[*offset] : 0;
    }

    // An event, a sphinx or a sign is written as in save files, with its uid, its type and then the object.
    template <typename Object>
    bool readMapObject( ROStreamBuf & stream, MapBaseObject * existing )
    {
        Object * object = dynamic_cast<Object *>( existing );
        if ( object == nullptr ) {
            return false;
        }

        stream >> *object;
        return true;
    }

    bool readMapObjectInto( const std::vector<uint8_t> & data, MapBaseObject * object )
    {
        ROStreamBuf stream( data );
        stream.setBigendian( true );

        uint32_t uid = 0;
        MP2::MapObjectType type = MP2::OBJ_NONE;
        stream >> uid >> type;

        bool isRead = false;
        switch ( type ) {
        case MP2::OBJ_EVENT:
            isRead = readMapObject<MapEvent>( stream, object );
            break;
        case MP2::OBJ_SPHINX:
            isRead = readMapObject<MapSphinx>( stream, object );
            break;
        case MP2::OBJ_SIGN:
            isRead = readMapObject<MapSign>( stream, object );
            break;
        default:
            break;
        }

        return isRead && !stream.fail() && stream.size() == 0;
    }

    template <typename Object>
    bool isValidMapObject( ROStreamBuf & stream, const MapBaseObject * existing )
    {
        Object object;
        stream >> object;
        return dynamic_cast<const Object *>( existing ) != nullptr;
    }

    // Reads the unit into a temporary object. Nothing is applied unless all units are valid.
    bool isValidUnit( const Multi::WorldUnit & unit )
    {
        switch ( unit.kind ) {
        case Multi::WorldUnit::Kind::TILE: {
            Maps::Tile tile;
            return Maps::isValidAbsIndex( unit.key ) && readWholeUnit( unit.data, tile ) && tile.GetIndex() == unit.key;
        }
        case Multi::WorldUnit::Kind::HERO: {
            Heroes hero;
            return Heroes::isValidId( unit.key ) && world.GetHeroes( unit.key ) != nullptr && readWholeUnit( unit.data, hero ) && hero.GetID() == unit.key;
        }
        case Multi::WorldUnit::Kind::CASTLE: {
            Castle castle;
            return Maps::isValidAbsIndex( unit.key ) && readWholeUnit( unit.data, castle ) && Maps::GetIndexFromAbsPoint( castle.GetCenter() ) == unit.key
                   && world.getCastle( castle.GetCenter() ) != nullptr;
        }
        case Multi::WorldUnit::Kind::KINGDOM: {
            Kingdom kingdom;
            return unit.key >= 0 && unit.key <= maxNumOfPlayers && readWholeUnit( unit.data, kingdom );
        }
        case Multi::WorldUnit::Kind::CAPTURED_OBJECT: {
            ROStreamBuf stream( unit.data );
            stream.setBigendian( true );

            int32_t tileIndex = -1;
            CapturedObject object;
            stream >> tileIndex >> object;

            return !stream.fail() && stream.size() == 0 && tileIndex == unit.key && Maps::isValidAbsIndex( tileIndex );
        }
        case Multi::WorldUnit::Kind::MAP_OBJECT: {
            // Map objects are never added during a game, only changed or removed.
            const MapBaseObject * existing = world.GetMapObject( static_cast<uint32_t>( unit.key ) );
            if ( existing == nullptr || unit.data.empty() ) {
                return existing != nullptr;
            }

            ROStreamBuf stream( unit.data );
            stream.setBigendian( true );

            uint32_t uid = 0;
            MP2::MapObjectType type = MP2::OBJ_NONE;
            stream >> uid >> type;

            bool isValid = false;
            switch ( type ) {
            case MP2::OBJ_EVENT:
                isValid = isValidMapObject<MapEvent>( stream, existing );
                break;
            case MP2::OBJ_SPHINX:
                isValid = isValidMapObject<MapSphinx>( stream, existing );
                break;
            case MP2::OBJ_SIGN:
                isValid = isValidMapObject<MapSign>( stream, existing );
                break;
            default:
                break;
            }

            return isValid && !stream.fail() && stream.size() == 0 && uid == static_cast<uint32_t>( unit.key );
        }
        case Multi::WorldUnit::Kind::FOG:
            return Maps::isValidAbsIndex( unit.key ) && unit.data.size() == 1;
        default:
            break;
        }

        return false;
    }

    void applyUnit( const Multi::WorldUnit & unit )
    {
        ROStreamBuf stream( unit.data );
        stream.setBigendian( true );

        switch ( unit.kind ) {
        case Multi::WorldUnit::Kind::TILE: {
            // The fog only clears during a day, so what is clear on either computer stays clear.
            Maps::Tile & tile = world.getTile( unit.key );
            const uint8_t localFog = getFogByte( tile );

            stream >> tile;

            if ( const uint8_t clearedHere = static_cast<uint8_t>( getFogByte( tile ) & ~localFog ); clearedHere != 0 ) {
                tile.ClearFog( clearedHere );
            }
            break;
        }
        case Multi::WorldUnit::Kind::HERO:
            stream >> *world.GetHeroes( unit.key );
            break;
        case Multi::WorldUnit::Kind::CASTLE:
            stream >> *world.getCastle( Maps::GetPoint( unit.key ) );
            break;
        case Multi::WorldUnit::Kind::KINGDOM:
            stream >> getKingdomByIndex( unit.key );
            break;
        case Multi::WorldUnit::Kind::CAPTURED_OBJECT: {
            int32_t tileIndex = -1;
            stream >> tileIndex >> world.GetCapturedObject( unit.key );
            break;
        }
        case Multi::WorldUnit::Kind::MAP_OBJECT: {
            MapBaseObject * object = world.GetMapObject( static_cast<uint32_t>( unit.key ) );
            if ( unit.data.empty() ) {
                world.RemoveMapObject( object );
            }
            else {
                readMapObjectInto( unit.data, object );
            }
            break;
        }
        case Multi::WorldUnit::Kind::FOG: {
            Maps::Tile & tile = world.getTile( unit.key );
            if ( const uint8_t clearedThere = static_cast<uint8_t>( getFogByte( tile ) & ~unit.data[0] ); clearedThere != 0 ) {
                tile.ClearFog( clearedThere );
            }
            break;
        }
        default:
            break;
        }
    }

    // The fog of a tile is ignored because it is merged.
    bool isAppliedAsReceived( const Multi::WorldUnit & unit )
    {
        RWStreamBuf stream;
        stream.setBigendian( true );

        switch ( unit.kind ) {
        case Multi::WorldUnit::Kind::TILE: {
            std::vector<uint8_t> bytes = writeTile( world.getTile( unit.key ) );
            std::vector<uint8_t> received = unit.data;

            if ( const std::optional<size_t> & offset = Multi::getTileFogOffset(); offset && *offset < bytes.size() && *offset < received.size() ) {
                bytes[*offset] = 0;
                received[*offset] = 0;
            }

            return bytes == received;
        }
        case Multi::WorldUnit::Kind::HERO:
            stream << *world.GetHeroes( unit.key );
            break;
        case Multi::WorldUnit::Kind::CASTLE:
            stream << *world.getCastle( Maps::GetPoint( unit.key ) );
            break;
        case Multi::WorldUnit::Kind::KINGDOM:
            stream << getKingdomByIndex( unit.key );
            break;
        case Multi::WorldUnit::Kind::CAPTURED_OBJECT:
            stream << unit.key << world.GetCapturedObject( unit.key );
            break;
        case Multi::WorldUnit::Kind::MAP_OBJECT:
            return unit.data.empty() == ( world.GetMapObject( static_cast<uint32_t>( unit.key ) ) == nullptr );
        case Multi::WorldUnit::Kind::FOG:
            return true;
        default:
            return false;
        }

        return std::equal( stream.data(), stream.data() + stream.size(), unit.data.begin(), unit.data.end() );
    }

    // Returns the world with these units applied to it, or an empty vector if they do not fit in it.
    std::vector<uint8_t> applyToSnapshot( const Multi::WorldSnapshot & snapshot, const std::vector<Multi::WorldUnit> & units )
    {
        const Multi::WorldParts & parts = snapshot.parts();

        // Castles are identified by the tile of their center.
        std::map<int32_t, size_t> castleIndexes;
        for ( size_t i = 0; i < parts.castles.size(); ++i ) {
            castleIndexes.emplace( getCastleKey( getUnitBytes( snapshot, parts.castles[i] ) ), i );
        }

        Multi::UnitReplacements replacements;

        for ( const Multi::WorldUnit & unit : units ) {
            switch ( unit.kind ) {
            case Multi::WorldUnit::Kind::TILE:
                replacements.tiles[static_cast<size_t>( unit.key )] = unit.data;
                break;
            case Multi::WorldUnit::Kind::HERO:
                replacements.heroes[static_cast<size_t>( unit.key )] = unit.data;
                break;
            case Multi::WorldUnit::Kind::CASTLE: {
                const auto iter = castleIndexes.find( unit.key );
                if ( iter == castleIndexes.end() ) {
                    return {};
                }
                replacements.castles[iter->second] = unit.data;
                break;
            }
            case Multi::WorldUnit::Kind::KINGDOM:
                replacements.kingdoms[static_cast<size_t>( unit.key )] = unit.data;
                break;
            case Multi::WorldUnit::Kind::CAPTURED_OBJECT:
                replacements.capturedObjects[unit.key] = unit.data;
                break;
            case Multi::WorldUnit::Kind::MAP_OBJECT:
                replacements.mapObjects[static_cast<uint32_t>( unit.key )] = unit.data;
                break;
            case Multi::WorldUnit::Kind::FOG: {
                const std::optional<size_t> & offset = Multi::getTileFogOffset();
                if ( !offset || static_cast<size_t>( unit.key ) >= parts.tiles.size() ) {
                    return {};
                }

                auto iter = replacements.tiles.find( static_cast<size_t>( unit.key ) );
                if ( iter == replacements.tiles.end() ) {
                    iter = replacements.tiles.emplace( static_cast<size_t>( unit.key ), getUnitBytes( snapshot, parts.tiles[unit.key] ) ).first;
                }
                if ( *offset >= iter->second.size() ) {
                    return {};
                }

                iter->second[*offset] = unit.data[0];
                break;
            }
            default:
                return {};
            }
        }

        return Multi::assembleWorld( snapshot.data(), parts, replacements );
    }

    bool applyToSnapshot( Multi::WorldSnapshot & snapshot, const std::vector<Multi::WorldUnit> & units, uint64_t * hash )
    {
        std::vector<uint8_t> data = applyToSnapshot( snapshot, units );
        if ( data.empty() ) {
            return false;
        }

        if ( hash != nullptr ) {
            *hash = Multi::computeFnv1aHash( data.data(), data.size() );
        }

        return snapshot.assign( std::move( data ) ).empty();
    }

    // Returns false if some change cannot be sent as units.
    bool addUnits( const Multi::WorldSnapshot & before, const Multi::WorldSnapshot & after, const Multi::WorldChanges & changes, Multi::WorldUpdate & update )
    {
        if ( changes.isLayoutChanged || changes.isRumorsChanged || changes.isEventsChanged || changes.isUltimateArtifactChanged || changes.isDateChanged
             || changes.isSeedChanged ) {
            return false;
        }

        const Multi::WorldParts & parts = after.parts();

        for ( const int32_t index : changes.tiles ) {
            update.units.push_back( { Multi::WorldUnit::Kind::TILE, index, getUnitBytes( after, parts.tiles[index] ) } );
        }

        // Only the fog is sent here. The rest of the tile may be changed by the other computer meanwhile.
        const std::optional<size_t> & fogOffset = Multi::getTileFogOffset();
        for ( const int32_t index : changes.fogTiles ) {
            const Multi::ByteRange & range = parts.tiles[index];
            if ( !fogOffset || range.begin + *fogOffset >= range.end ) {
                return false;
            }

            update.units.push_back( { Multi::WorldUnit::Kind::FOG, index, { after.data()[range.begin + *fogOffset] } } );
        }

        for ( const int32_t id : changes.heroes ) {
            update.units.push_back( { Multi::WorldUnit::Kind::HERO, id, getUnitBytes( after, parts.heroes[id] ) } );
        }

        for ( const int32_t index : changes.castles ) {
            std::vector<uint8_t> data = getUnitBytes( after, parts.castles[index] );
            const int32_t key = getCastleKey( data );
            if ( key == -1 ) {
                return false;
            }

            update.units.push_back( { Multi::WorldUnit::Kind::CASTLE, key, std::move( data ) } );
        }

        for ( const int32_t index : changes.kingdoms ) {
            update.units.push_back( { Multi::WorldUnit::Kind::KINGDOM, index, getUnitBytes( after, parts.kingdoms[index] ) } );
        }

        // Captured objects are never removed from the world, only changed or added.
        if ( !changes.capturedObjects.empty() ) {
            if ( before.parts().capturedObjects.size() > parts.capturedObjects.size() ) {
                return false;
            }

            for ( const Multi::ByteRange & range : parts.capturedObjects ) {
                std::vector<uint8_t> data = getUnitBytes( after, range );
                const int32_t key = static_cast<int32_t>( readKey( data ) );

                if ( std::find( changes.capturedObjects.begin(), changes.capturedObjects.end(), key ) != changes.capturedObjects.end() ) {
                    update.units.push_back( { Multi::WorldUnit::Kind::CAPTURED_OBJECT, key, std::move( data ) } );
                }
            }
        }

        // Map objects can be changed or removed. An object that is not there anymore is sent without data.
        for ( const uint32_t uid : changes.mapObjects ) {
            std::vector<uint8_t> data;
            for ( const Multi::ByteRange & range : parts.mapObjects ) {
                std::vector<uint8_t> bytes = getUnitBytes( after, range );
                if ( readKey( bytes ) == uid ) {
                    data = std::move( bytes );
                    break;
                }
            }

            update.units.push_back( { Multi::WorldUnit::Kind::MAP_OBJECT, static_cast<int32_t>( uid ), std::move( data ) } );
        }

        return true;
    }
}

void Multi::resetLiveBaseline()
{
    LiveState & state = liveState();

    if ( const std::string error = state.sentWorld.capture(); !error.empty() ) {
        ERROR_LOG( "[MULTI] Live view stopped: " << error )
        state.isInSync = false;
        state.otherWorld = {};
        return;
    }

    state.otherWorld = state.sentWorld;
    state.isInSync = true;
}

void Multi::onHeroStep( const Heroes & hero, const int32_t targetIndex )
{
    watchHeroStep( hero, targetIndex );
}

void Multi::onHeroStepDone( const Heroes & hero )
{
    watchHeroStepDone( hero );
    sendLiveUpdate( &hero );
    finishLiveBattle();

    // When every computer plays, the messages of the other ones are processed at each step.
    if ( isSimultaneousDay() ) {
        processDayMessages( hero.isControlHuman() );
    }
}

void Multi::onTurnEnd( const PlayerColor color )
{
    watchTurnEnd( color );
    sendLiveUpdate();
    finishLiveBattle();
}

void Multi::sendLiveUpdate( const Heroes * hero /* = nullptr */ )
{
    LiveState & state = liveState();

    NetworkSession * session = getSession();
    if ( session == nullptr || !session->isAlive() || !state.sentWorld.isValid() ) {
        return;
    }

    WorldSnapshot now;
    if ( const std::string error = now.capture(); !error.empty() ) {
        ERROR_LOG( "[MULTI] Live view stopped: " << error )
        state.sentWorld = {};
        return;
    }

    const WorldChanges changes = compareSnapshots( state.sentWorld, now );
    if ( changes.empty() ) {
        return;
    }

    // When every computer plays, the kingdom of a player of another computer changes only there, except in a battle
    // arranged with that computer.
    if ( isSimultaneousDay() && ( isGuest() || !areOtherDaysDone() ) && !isLiveBattleAgreed() ) {
        for ( const int32_t index : changes.kingdoms ) {
            const PlayerColor color = getKingdomByIndex( index ).GetColor();
            if ( ( getRemoteHumanColors() & color ) != 0 ) {
                ERROR_LOG( "[MULTI] The kingdom of " << Color::String( color ) << ", played on another computer, changed here"
                                                     << ( hero ? " after a step of " + hero->GetName() : std::string() ) << ": "
                                                     << describeKingdomDifference( state.sentWorld, now, index ) )
            }
        }
    }

    WorldUpdate update;
    update.sequence = ++state.sequence;
    update.worldHash = computeFnv1aHash( now.data().data(), now.data().size() );

    if ( !addUnits( state.sentWorld, now, changes, update ) ) {
        update.units.clear();
        update.isIncomplete = true;
        ++state.stats.incompleteSent;

        VERBOSE_LOG( "[MULTI] Live update " << update.sequence << ": changes that cannot be sent as units, the other computer waits for the next whole game" )
    }

    const Message message = makeWorldUpdate( update );
    session->sendToMachines( getOtherMachines(), message );

    ++state.stats.updatesSent;
    state.stats.bytesSent += message.payload.size();

    state.sentWorld = std::move( now );
}

bool Multi::applyLiveUpdate( const Message & message )
{
    LiveState & state = liveState();

    const std::optional<WorldUpdate> update = readWorldUpdate( message );
    if ( !update ) {
        ++state.stats.updatesRefused;
        state.isInSync = false;
        ERROR_LOG( "[MULTI] Invalid live update received" )
        return false;
    }

    if ( !state.isInSync ) {
        ++state.stats.updatesIgnored;
        return false;
    }

    if ( update->isIncomplete ) {
        ++state.stats.updatesIgnored;
        state.isInSync = false;
        return false;
    }

    const auto stopFollowing = [&state]() {
        state.isInSync = false;
        state.otherWorld = {};
    };

    const SaveVersionGuard versionGuard;

    for ( const WorldUnit & unit : update->units ) {
        if ( !isValidUnit( unit ) ) {
            ++state.stats.updatesRefused;
            stopFollowing();
            ERROR_LOG( "[MULTI] Live update " << update->sequence << " refused: invalid unit " << static_cast<int>( unit.kind ) << " " << unit.key )
            return false;
        }
    }

    // If only the other computer plays, its world with these units must be exactly the one known here (local routes do
    // not count). When both play, the worlds are compared in the evening.
    if ( !isSimultaneousDay() && state.otherWorld.isValid() ) {
        uint64_t hash = 0;
        if ( !applyToSnapshot( state.otherWorld, update->units, &hash ) || hash != update->worldHash ) {
            ++state.stats.hashMismatches;
            stopFollowing();
            ERROR_LOG( "[MULTI] Live update " << update->sequence << " of computer " << static_cast<int>( message.sender ) << ": its world is not the one known here" )
            return false;
        }
    }

    for ( const WorldUnit & unit : update->units ) {
        applyUnit( unit );
    }

    world.resetPathfinder();
    resetLostFocus();
    onWorldChangedForSpectator( false );

    // What was received must not be sent back.
    if ( state.sentWorld.isValid() && !applyToSnapshot( state.sentWorld, update->units, nullptr ) ) {
        state.sentWorld = {};
    }

    // Each local unit must now be exactly the same as the received one.
    for ( const WorldUnit & unit : update->units ) {
        if ( !isAppliedAsReceived( unit ) ) {
            ++state.stats.hashMismatches;
            stopFollowing();
            ERROR_LOG( "[MULTI] Live update " << update->sequence << ": unit " << static_cast<int>( unit.kind ) << " " << unit.key << " differs once applied" )
            return false;
        }
    }

    ++state.stats.updatesApplied;

    // In self-tests the local player clicks on their heroes meanwhile. Their routes change here only.
    if ( isTestMode() && !isSimultaneousDay() ) {
        for ( const PlayerColor color : PlayerColorsVector( getLocalHumanColors() ) ) {
            for ( Heroes * hero : world.GetKingdom( color ).GetHeroes() ) {
                hero->ShowPath( !hero->GetPath().isShow() );
                break;
            }
        }
    }

    return true;
}

void Multi::resetLostFocus()
{
    for ( Player * player : Settings::Get().GetPlayers().getVector() ) {
        Focus & focus = player->GetFocus();

        const Heroes * hero = focus.GetHeroes();
        const Castle * castle = focus.GetCastle();

        const bool isLost = ( focus.first != FOCUS_UNSEL && focus.second == nullptr )
                            || ( hero != nullptr && ( hero->GetColor() != player->GetColor() || !Maps::isValidAbsIndex( hero->GetIndex() ) ) )
                            || ( castle != nullptr && castle->GetColor() != player->GetColor() );
        if ( isLost ) {
            focus.Reset();
        }
    }
}

Multi::WorldRollback::WorldRollback()
{
    _isValid = _before.capture().empty();
}

bool Multi::WorldRollback::restore()
{
    WorldSnapshot now;
    if ( !_isValid || !now.capture().empty() ) {
        ERROR_LOG( "[MULTI] Unable to put the world back as it was" )
        return false;
    }

    const WorldChanges changes = compareSnapshots( now, _before );
    if ( changes.empty() ) {
        return true;
    }

    WorldUpdate update;
    if ( !addUnits( now, _before, changes, update ) ) {
        ERROR_LOG( "[MULTI] Unable to put the world back as it was: changes that cannot be undone" )
        return false;
    }

    const SaveVersionGuard versionGuard;

    for ( const WorldUnit & unit : update.units ) {
        if ( !isValidUnit( unit ) ) {
            ERROR_LOG( "[MULTI] Unable to put the world back as it was: invalid unit " << static_cast<int>( unit.kind ) << " " << unit.key )
            return false;
        }
    }

    for ( const WorldUnit & unit : update.units ) {
        applyUnit( unit );
    }

    world.resetPathfinder();
    resetLostFocus();

    VERBOSE_LOG( "[MULTI] " << update.units.size() << " part(s) of the world put back as they were" )

    return true;
}

uint64_t Multi::computeWorldHash()
{
    WorldSnapshot snapshot;
    if ( !snapshot.capture().empty() ) {
        return 0;
    }

    return computeFnv1aHash( snapshot.data().data(), snapshot.data().size() );
}

uint64_t Multi::computeWorldHashWithoutRoutes()
{
    struct SavedRoute
    {
        Heroes * hero{ nullptr };
        std::vector<Route::Step> steps;
        bool isShown{ false };
    };

    std::vector<SavedRoute> routes;

    for ( int32_t id = Heroes::UNKNOWN + 1; id < Heroes::HEROES_COUNT; ++id ) {
        Heroes * hero = world.GetHeroes( id );
        if ( hero == nullptr ) {
            continue;
        }

        Route::Path & path = hero->GetPath();
        routes.push_back( { hero, std::vector<Route::Step>( path.begin(), path.end() ), path.isShow() } );

        path.Reset();
        path.Hide();
    }

    const uint64_t hash = computeWorldHash();

    for ( const SavedRoute & route : routes ) {
        Route::Path & path = route.hero->GetPath();
        path.setPath( { route.steps.begin(), route.steps.end() } );

        if ( route.isShown ) {
            path.Show();
        }
        else {
            path.Hide();
        }
    }

    return hash;
}

const Multi::LiveStats & Multi::getLiveStats()
{
    return liveState().stats;
}

std::string Multi::describeLiveStats()
{
    const LiveStats & stats = liveState().stats;

    std::ostringstream os;
    os << stats.updatesSent << " update(s) sent (" << stats.bytesSent << " bytes, " << stats.incompleteSent << " incomplete), " << stats.updatesApplied << " applied, "
       << stats.updatesIgnored << " ignored, " << stats.updatesRefused << " refused, " << stats.hashMismatches << " hash mismatch(es)";

    return os.str();
}
