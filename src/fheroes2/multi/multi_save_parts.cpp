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

#include "multi_save_parts.h"

#include <exception>
#include <string>

#include "artifact_ultimate.h"
#include "castle.h"
#include "game_io.h"
#include "heroes.h"
#include "kingdom.h"
#include "maps_fileinfo.h"
#include "maps_objects.h"
#include "maps_tiles.h"
#include "mp2.h"
#include "save_format_version.h"
#include "serialize.h"
#include "world.h"
#include "zzlib.h"

// The units below mirror World::operator<< and the save file layout of Game::Save().
static_assert( CURRENT_FORMAT_VERSION == FORMAT_VERSION_1180_RELEASE, "The save format changed: check multi_save_parts" );

namespace
{
    constexpr uint16_t saveFileMagicNumber{ 0xFF03 };

    // The largest map takes only a few MB
    constexpr uint32_t maxDataSize{ 64 * 1024 * 1024 };

    constexpr int32_t maxMapSize{ 1024 };

    // Into a temporary object, only the position is kept
    template <typename Unit>
    bool readUnit( ROStreamBuf & stream, std::vector<Multi::ByteRange> & ranges )
    {
        const size_t begin = stream.tell();

        Unit unit;
        stream >> unit;

        ranges.push_back( { begin, stream.tell() } );

        return !stream.fail();
    }

    // A unit takes at least one byte, a count larger than the stream size means broken data
    bool readCount( ROStreamBuf & stream, uint32_t & count )
    {
        count = stream.get32();

        return !stream.fail() && count <= stream.size();
    }

    void putRange( RWStreamBuf & stream, const std::vector<uint8_t> & data, const Multi::ByteRange & range )
    {
        stream.putRaw( data.data() + range.begin, range.end - range.begin );
    }

    void putList( RWStreamBuf & stream, const std::vector<uint8_t> & data, const std::vector<Multi::ByteRange> & ranges )
    {
        stream.put32( static_cast<uint32_t>( ranges.size() ) );

        for ( const Multi::ByteRange & range : ranges ) {
            putRange( stream, data, range );
        }
    }

    bool putList( RWStreamBuf & stream, const std::vector<uint8_t> & data, const std::vector<Multi::ByteRange> & ranges,
                  const std::map<size_t, std::vector<uint8_t>> & replacements )
    {
        if ( !replacements.empty() && replacements.rbegin()->first >= ranges.size() ) {
            return false;
        }

        stream.put32( static_cast<uint32_t>( ranges.size() ) );

        for ( size_t i = 0; i < ranges.size(); ++i ) {
            if ( const auto iter = replacements.find( i ); iter != replacements.end() ) {
                stream.putRaw( iter->second.data(), iter->second.size() );
            }
            else {
                putRange( stream, data, ranges[i] );
            }
        }

        return true;
    }

    int32_t readTileIndexKey( const std::vector<uint8_t> & data, const Multi::ByteRange & range )
    {
        if ( range.end - range.begin < 4 ) {
            return -1;
        }

        const uint8_t * bytes = data.data() + range.begin;
        return static_cast<int32_t>( ( static_cast<uint32_t>( bytes[0] ) << 24 ) | ( static_cast<uint32_t>( bytes[1] ) << 16 )
                                     | ( static_cast<uint32_t>( bytes[2] ) << 8 ) | bytes[3] );
    }

    std::string splitWorldUnits( ROStreamBuf & stream, Multi::WorldParts & parts )
    {
        int32_t width = 0;
        int32_t height = 0;

        parts.size.begin = stream.tell();
        stream >> width >> height;
        parts.size.end = stream.tell();

        if ( stream.fail() || width <= 0 || height <= 0 || width > maxMapSize || height > maxMapSize ) {
            return "invalid map size";
        }

        uint32_t count = 0;

        if ( !readCount( stream, count ) || count != static_cast<uint32_t>( width * height ) ) {
            return "invalid number of tiles";
        }
        if ( world.w() != width || world.h() != height ) {
            return "the game loaded is on another map";
        }

        for ( uint32_t i = 0; i < count; ++i ) {
            if ( !readUnit<Maps::Tile>( stream, parts.tiles ) ) {
                return "invalid tile";
            }
        }

        if ( !readCount( stream, count ) ) {
            return "invalid number of heroes";
        }
        for ( uint32_t i = 0; i < count; ++i ) {
            if ( !readUnit<Heroes>( stream, parts.heroes ) ) {
                return "invalid hero";
            }
        }

        if ( !readCount( stream, count ) ) {
            return "invalid number of castles";
        }
        for ( uint32_t i = 0; i < count; ++i ) {
            if ( !readUnit<Castle>( stream, parts.castles ) ) {
                return "invalid castle";
            }
        }

        if ( !readCount( stream, count ) || count != maxNumOfPlayers + 1 ) {
            return "invalid number of kingdoms";
        }
        for ( uint32_t i = 0; i < count; ++i ) {
            if ( !readUnit<Kingdom>( stream, parts.kingdoms ) ) {
                return "invalid kingdom";
            }
        }

        if ( !readCount( stream, count ) ) {
            return "invalid number of rumors";
        }
        for ( uint32_t i = 0; i < count; ++i ) {
            if ( !readUnit<std::string>( stream, parts.rumors ) ) {
                return "invalid rumor";
            }
        }

        if ( !readCount( stream, count ) ) {
            return "invalid number of events";
        }
        for ( uint32_t i = 0; i < count; ++i ) {
            if ( !readUnit<EventDate>( stream, parts.events ) ) {
                return "invalid event";
            }
        }

        if ( !readCount( stream, count ) ) {
            return "invalid number of captured objects";
        }
        for ( uint32_t i = 0; i < count; ++i ) {
            const size_t begin = stream.tell();

            int32_t tileIndex = -1;
            CapturedObject object;
            stream >> tileIndex >> object;

            parts.capturedObjects.push_back( { begin, stream.tell() } );

            if ( stream.fail() || tileIndex < 0 || tileIndex >= width * height ) {
                return "invalid captured object";
            }
        }

        parts.ultimateArtifact.begin = stream.tell();
        {
            UltimateArtifact ultimateArtifact;
            stream >> ultimateArtifact;
        }
        parts.ultimateArtifact.end = stream.tell();

        parts.dateAndConditions.begin = stream.tell();
        {
            uint32_t day = 0;
            uint32_t week = 0;
            uint32_t month = 0;
            int32_t winHeroId = 0;
            int32_t lossHeroId = 0;
            stream >> day >> week >> month >> winHeroId >> lossHeroId;
        }
        parts.dateAndConditions.end = stream.tell();

        if ( stream.fail() ) {
            return "invalid date";
        }

        if ( !readCount( stream, count ) ) {
            return "invalid number of map objects";
        }
        for ( uint32_t i = 0; i < count; ++i ) {
            const size_t begin = stream.tell();

            uint32_t uid = 0;
            MP2::MapObjectType type = MP2::OBJ_NONE;
            stream >> uid >> type;

            switch ( type ) {
            case MP2::OBJ_EVENT: {
                MapEvent object;
                stream >> object;
                break;
            }
            case MP2::OBJ_SPHINX: {
                MapSphinx object;
                stream >> object;
                break;
            }
            case MP2::OBJ_SIGN: {
                MapSign object;
                stream >> object;
                break;
            }
            default:
                return "invalid map object";
            }

            parts.mapObjects.push_back( { begin, stream.tell() } );

            if ( stream.fail() ) {
                return "invalid map object";
            }
        }

        parts.seed.begin = stream.tell();
        {
            uint32_t seed = 0;
            stream >> seed;
        }
        parts.seed.end = stream.tell();

        if ( stream.fail() ) {
            return "invalid seed";
        }

        parts.worldEnd = stream.tell();

        return {};
    }
}

std::string Multi::readSaveFileData( const std::vector<uint8_t> & fileBytes, SaveFileData & save )
{
    const Multi::SaveVersionGuard versionGuard;

    ROStreamBuf stream( fileBytes );
    stream.setBigendian( true );

    uint16_t magicNumber = 0;
    uint32_t versionSize = 0;
    stream >> magicNumber >> versionSize;

    if ( stream.fail() || magicNumber != saveFileMagicNumber || versionSize > 32 || versionSize > stream.size() ) {
        return "not a save file";
    }

    stream.skip( versionSize );

    uint16_t version = 0;
    stream >> version;

    if ( stream.fail() || version != CURRENT_FORMAT_VERSION ) {
        return "unsupported save format";
    }

    // The header contains requirements, map information and game type.
    try {
        uint16_t requirements = 0;
        Maps::FileInfo fileInfo;
        int32_t gameType = 0;
        stream >> requirements >> fileInfo >> gameType;
    }
    catch ( const std::exception & ) {
        return "invalid save header";
    }

    if ( stream.fail() ) {
        return "invalid save header";
    }

    const size_t headerSize = stream.tell();

    // Sizes checked before anything is allocated
    const uint32_t rawSize = stream.get32();
    const uint32_t zipSize = stream.get32();

    if ( stream.fail() || rawSize == 0 || rawSize > maxDataSize || zipSize > stream.size() ) {
        return "invalid save data";
    }

    stream.seek( headerSize );

    RWStreamBuf data;
    data.setBigendian( true );

    if ( !Compression::unzipStream( stream, data ) ) {
        return "invalid save data";
    }

    save.header.assign( fileBytes.begin(), fileBytes.begin() + static_cast<std::ptrdiff_t>( headerSize ) );
    save.data.assign( data.data(), data.data() + data.size() );

    return {};
}

std::vector<uint8_t> Multi::writeSaveFileData( const SaveFileData & save )
{
    RWStreamBuf data;
    data.setBigendian( true );
    data.putRaw( save.data.data(), save.data.size() );

    RWStreamBuf file;
    file.setBigendian( true );
    file.putRaw( save.header.data(), save.header.size() );

    if ( !Compression::zipStreamBuf( data, file ) ) {
        return {};
    }

    return { file.data(), file.data() + file.size() };
}

std::string Multi::splitWorld( const std::vector<uint8_t> & data, WorldParts & parts )
{
    const Multi::SaveVersionGuard versionGuard;

    parts = {};

    ROStreamBuf stream( data );
    stream.setBigendian( true );

    // Broken data can still ask for an absurd amount of memory inside a unit
    try {
        if ( std::string error = splitWorldUnits( stream, parts ); !error.empty() ) {
            parts = {};
            return error;
        }
    }
    catch ( const std::exception & ) {
        parts = {};
        return "invalid world data";
    }

    return {};
}

std::vector<uint8_t> Multi::assembleWorld( const std::vector<uint8_t> & data, const WorldParts & parts )
{
    RWStreamBuf stream;
    stream.setBigendian( true );

    putRange( stream, data, parts.size );
    putList( stream, data, parts.tiles );
    putList( stream, data, parts.heroes );
    putList( stream, data, parts.castles );
    putList( stream, data, parts.kingdoms );
    putList( stream, data, parts.rumors );
    putList( stream, data, parts.events );
    putList( stream, data, parts.capturedObjects );
    putRange( stream, data, parts.ultimateArtifact );
    putRange( stream, data, parts.dateAndConditions );
    putList( stream, data, parts.mapObjects );
    putRange( stream, data, parts.seed );

    stream.putRaw( data.data() + parts.worldEnd, data.size() - parts.worldEnd );

    return { stream.data(), stream.data() + stream.size() };
}

std::vector<uint8_t> Multi::assembleWorld( const std::vector<uint8_t> & data, const WorldParts & parts, const UnitReplacements & replacements )
{
    RWStreamBuf stream;
    stream.setBigendian( true );

    putRange( stream, data, parts.size );

    if ( !putList( stream, data, parts.tiles, replacements.tiles ) || !putList( stream, data, parts.heroes, replacements.heroes )
         || !putList( stream, data, parts.castles, replacements.castles ) || !putList( stream, data, parts.kingdoms, replacements.kingdoms ) ) {
        return {};
    }

    putList( stream, data, parts.rumors );
    putList( stream, data, parts.events );

    std::map<int32_t, std::vector<uint8_t>> capturedObjects;
    for ( const ByteRange & range : parts.capturedObjects ) {
        capturedObjects.emplace( readTileIndexKey( data, range ), std::vector<uint8_t>( data.begin() + static_cast<std::ptrdiff_t>( range.begin ),
                                                                                        data.begin() + static_cast<std::ptrdiff_t>( range.end ) ) );
    }
    for ( const auto & [key, bytes] : replacements.capturedObjects ) {
        capturedObjects[key] = bytes;
    }

    stream.put32( static_cast<uint32_t>( capturedObjects.size() ) );
    for ( const auto & [key, bytes] : capturedObjects ) {
        stream.putRaw( bytes.data(), bytes.size() );
    }

    putRange( stream, data, parts.ultimateArtifact );
    putRange( stream, data, parts.dateAndConditions );

    if ( replacements.mapObjects.empty() ) {
        putList( stream, data, parts.mapObjects );
    }
    else {
        std::map<uint32_t, std::vector<uint8_t>> mapObjects;
        for ( const ByteRange & range : parts.mapObjects ) {
            mapObjects.emplace( static_cast<uint32_t>( readTileIndexKey( data, range ) ),
                                std::vector<uint8_t>( data.begin() + static_cast<std::ptrdiff_t>( range.begin ),
                                                      data.begin() + static_cast<std::ptrdiff_t>( range.end ) ) );
        }
        for ( const auto & [uid, bytes] : replacements.mapObjects ) {
            if ( bytes.empty() ) {
                mapObjects.erase( uid );
            }
            else {
                mapObjects[uid] = bytes;
            }
        }

        stream.put32( static_cast<uint32_t>( mapObjects.size() ) );
        for ( const auto & [uid, bytes] : mapObjects ) {
            stream.putRaw( bytes.data(), bytes.size() );
        }
    }

    putRange( stream, data, parts.seed );

    stream.putRaw( data.data() + parts.worldEnd, data.size() - parts.worldEnd );

    return { stream.data(), stream.data() + stream.size() };
}

Multi::SaveVersionGuard::SaveVersionGuard()
    : _previous( Game::GetVersionOfCurrentSaveFile() )
{
    Game::SetVersionOfCurrentSaveFile( CURRENT_FORMAT_VERSION );
}

Multi::SaveVersionGuard::~SaveVersionGuard()
{
    Game::SetVersionOfCurrentSaveFile( _previous );
}
