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

#include "multi_world_changes.h"

#include <algorithm>
#include <map>
#include <optional>
#include <sstream>

#include "kingdom.h"
#include "maps_tiles.h"
#include "serialize.h"
#include "world.h"

namespace
{
    // Done once. Two tiles which differ only by their fog are written and the only different byte is the fog
    std::optional<size_t> findFogOffset()
    {
        const Maps::Tile withFog;

        Maps::Tile withoutFog;
        withoutFog.ClearFog( Color::allPlayerColors() );

        RWStreamBuf first;
        first.setBigendian( true );
        first << withFog;

        RWStreamBuf second;
        second.setBigendian( true );
        second << withoutFog;

        if ( first.size() != second.size() ) {
            return {};
        }

        std::optional<size_t> offset;

        for ( size_t i = 0; i < first.size(); ++i ) {
            if ( first.data()[i] == second.data()[i] ) {
                continue;
            }

            if ( offset ) {
                return {};
            }

            offset = i;
        }

        return offset;
    }

    bool isSameRange( const std::vector<uint8_t> & first, const Multi::ByteRange & firstRange, const std::vector<uint8_t> & second, const Multi::ByteRange & secondRange )
    {
        const size_t size = firstRange.end - firstRange.begin;

        return size == secondRange.end - secondRange.begin
               && std::equal( first.begin() + static_cast<std::ptrdiff_t>( firstRange.begin ), first.begin() + static_cast<std::ptrdiff_t>( firstRange.end ),
                              second.begin() + static_cast<std::ptrdiff_t>( secondRange.begin ) );
    }

    // Same order on both sides, matched by index
    void compareList( const std::vector<uint8_t> & first, const std::vector<Multi::ByteRange> & firstRanges, const std::vector<uint8_t> & second,
                      const std::vector<Multi::ByteRange> & secondRanges, std::vector<int32_t> & changed, bool & isLayoutChanged )
    {
        if ( firstRanges.size() != secondRanges.size() ) {
            isLayoutChanged = true;
        }

        const size_t count = std::min( firstRanges.size(), secondRanges.size() );
        for ( size_t i = 0; i < count; ++i ) {
            if ( !isSameRange( first, firstRanges[i], second, secondRanges[i] ) ) {
                changed.push_back( static_cast<int32_t>( i ) );
            }
        }
    }

    bool isListChanged( const std::vector<uint8_t> & first, const std::vector<Multi::ByteRange> & firstRanges, const std::vector<uint8_t> & second,
                        const std::vector<Multi::ByteRange> & secondRanges )
    {
        if ( firstRanges.size() != secondRanges.size() ) {
            return true;
        }

        for ( size_t i = 0; i < firstRanges.size(); ++i ) {
            if ( !isSameRange( first, firstRanges[i], second, secondRanges[i] ) ) {
                return true;
            }
        }

        return false;
    }

    // 32-bit big-endian key at the start of the unit
    uint32_t readKey( const std::vector<uint8_t> & data, const Multi::ByteRange & range )
    {
        if ( range.end - range.begin < 4 ) {
            return 0;
        }

        const uint8_t * bytes = data.data() + range.begin;
        return ( static_cast<uint32_t>( bytes[0] ) << 24 ) | ( static_cast<uint32_t>( bytes[1] ) << 16 ) | ( static_cast<uint32_t>( bytes[2] ) << 8 ) | bytes[3];
    }

    template <typename Key>
    void compareKeyedList( const std::vector<uint8_t> & first, const std::vector<Multi::ByteRange> & firstRanges, const std::vector<uint8_t> & second,
                           const std::vector<Multi::ByteRange> & secondRanges, std::vector<Key> & changed )
    {
        std::map<uint32_t, const Multi::ByteRange *> before;
        for ( const Multi::ByteRange & range : firstRanges ) {
            before.emplace( readKey( first, range ), &range );
        }

        std::map<uint32_t, const Multi::ByteRange *> after;
        for ( const Multi::ByteRange & range : secondRanges ) {
            after.emplace( readKey( second, range ), &range );
        }

        for ( const auto & [key, range] : before ) {
            const auto iter = after.find( key );
            if ( iter == after.end() || !isSameRange( first, *range, second, *iter->second ) ) {
                changed.push_back( static_cast<Key>( key ) );
            }
        }

        for ( const auto & [key, range] : after ) {
            if ( before.find( key ) == before.end() ) {
                changed.push_back( static_cast<Key>( key ) );
            }
        }
    }
}

std::string Multi::describeKingdomDifference( const WorldSnapshot & first, const WorldSnapshot & second, const int32_t kingdomIndex )
{
    const auto kingdomBytes = [kingdomIndex]( const WorldSnapshot & snapshot ) {
        const std::vector<ByteRange> & kingdoms = snapshot.parts().kingdoms;
        if ( kingdomIndex < 0 || static_cast<size_t>( kingdomIndex ) >= kingdoms.size() ) {
            return std::vector<uint8_t>();
        }

        const ByteRange & range = kingdoms[kingdomIndex];
        return std::vector<uint8_t>( snapshot.data().begin() + static_cast<std::ptrdiff_t>( range.begin ),
                                     snapshot.data().begin() + static_cast<std::ptrdiff_t>( range.end ) );
    };

    const SaveVersionGuard versionGuard;

    const auto describe = []( const std::vector<uint8_t> & bytes ) {
        ROStreamBuf stream( bytes );
        stream.setBigendian( true );

        Kingdom kingdom;
        stream >> kingdom;

        std::ostringstream text;
        text << Color::String( kingdom.GetColor() ) << ", " << kingdom.GetFunds().String() << ", " << kingdom.GetHeroes().size() << " heroes, "
             << kingdom.GetCastles().size() << " castles, lost town days " << kingdom.GetLostTownDays() << ", " << bytes.size() << " bytes";
        return text.str();
    };

    const std::vector<uint8_t> firstBytes = kingdomBytes( first );
    const std::vector<uint8_t> secondBytes = kingdomBytes( second );
    const auto difference = std::mismatch( firstBytes.begin(), firstBytes.end(), secondBytes.begin(), secondBytes.end() );

    std::ostringstream os;
    os << describe( firstBytes ) << " / " << describe( secondBytes ) << ", first difference at byte " << ( difference.first - firstBytes.begin() );
    return os.str();
}

const std::optional<size_t> & Multi::getTileFogOffset()
{
    static const std::optional<size_t> offset = findFogOffset();
    return offset;
}

std::string Multi::WorldSnapshot::capture()
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << World::Get();

    if ( stream.fail() ) {
        _data.clear();
        return "unable to write the world";
    }

    _data.assign( stream.data(), stream.data() + stream.size() );

    if ( std::string error = splitWorld( _data, _parts ); !error.empty() ) {
        _data.clear();
        return error;
    }

    return {};
}

std::string Multi::WorldSnapshot::assign( std::vector<uint8_t> data )
{
    _data = std::move( data );

    if ( std::string error = splitWorld( _data, _parts ); !error.empty() ) {
        _data.clear();
        return error;
    }

    return {};
}

bool Multi::WorldChanges::empty() const
{
    return tiles.empty() && fogTiles.empty() && heroes.empty() && castles.empty() && kingdoms.empty() && capturedObjects.empty() && mapObjects.empty() && !isRumorsChanged
           && !isEventsChanged && !isUltimateArtifactChanged && !isDateChanged && !isSeedChanged && !isLayoutChanged;
}

Multi::WorldChanges Multi::compareSnapshots( const WorldSnapshot & before, const WorldSnapshot & after )
{
    WorldChanges changes;

    const std::vector<uint8_t> & first = before.data();
    const std::vector<uint8_t> & second = after.data();
    const WorldParts & firstParts = before.parts();
    const WorldParts & secondParts = after.parts();

    if ( !before.isValid() || !after.isValid() || firstParts.tiles.size() != secondParts.tiles.size()
         || !isSameRange( first, firstParts.size, second, secondParts.size ) ) {
        changes.isLayoutChanged = true;
        return changes;
    }

    const std::optional<size_t> & fogOffset = getTileFogOffset();

    for ( size_t i = 0; i < firstParts.tiles.size(); ++i ) {
        const ByteRange & firstRange = firstParts.tiles[i];
        const ByteRange & secondRange = secondParts.tiles[i];

        if ( isSameRange( first, firstRange, second, secondRange ) ) {
            continue;
        }

        const size_t size = firstRange.end - firstRange.begin;
        bool isFogOnly = false;

        if ( fogOffset && size == secondRange.end - secondRange.begin && *fogOffset < size ) {
            isFogOnly = true;

            for ( size_t byte = 0; byte < size; ++byte ) {
                if ( byte != *fogOffset && first[firstRange.begin + byte] != second[secondRange.begin + byte] ) {
                    isFogOnly = false;
                    break;
                }
            }

            changes.fogColors |= static_cast<PlayerColorsSet>( first[firstRange.begin + *fogOffset] ^ second[secondRange.begin + *fogOffset] );
        }

        if ( isFogOnly ) {
            changes.fogTiles.push_back( static_cast<int32_t>( i ) );
        }
        else {
            changes.tiles.push_back( static_cast<int32_t>( i ) );
        }
    }

    compareList( first, firstParts.heroes, second, secondParts.heroes, changes.heroes, changes.isLayoutChanged );
    compareList( first, firstParts.castles, second, secondParts.castles, changes.castles, changes.isLayoutChanged );
    compareList( first, firstParts.kingdoms, second, secondParts.kingdoms, changes.kingdoms, changes.isLayoutChanged );

    compareKeyedList( first, firstParts.capturedObjects, second, secondParts.capturedObjects, changes.capturedObjects );
    compareKeyedList( first, firstParts.mapObjects, second, secondParts.mapObjects, changes.mapObjects );

    changes.isRumorsChanged = isListChanged( first, firstParts.rumors, second, secondParts.rumors );
    changes.isEventsChanged = isListChanged( first, firstParts.events, second, secondParts.events );
    changes.isUltimateArtifactChanged = !isSameRange( first, firstParts.ultimateArtifact, second, secondParts.ultimateArtifact );
    changes.isDateChanged = !isSameRange( first, firstParts.dateAndConditions, second, secondParts.dateAndConditions );
    changes.isSeedChanged = !isSameRange( first, firstParts.seed, second, secondParts.seed );

    return changes;
}
