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

#include "multi_fingerprint.h"

#include <iomanip>
#include <sstream>

#include "game_over.h"
#include "logging.h"
#include "multi_game.h"
#include "serialize.h"
#include "world.h"

uint64_t Multi::computeFnv1aHash( const uint8_t * data, const size_t size, uint64_t hash /* = fnv1aOffsetBasis */ )
{
    for ( size_t i = 0; i < size; ++i ) {
        hash ^= data[i];
        hash *= 0x100000001B3ULL;
    }

    return hash;
}

std::string Multi::StateFingerprint::hashString() const
{
    std::ostringstream os;
    os << std::hex << std::setw( 16 ) << std::setfill( '0' ) << hash;

    return os.str();
}

Multi::StateFingerprint Multi::computeStateFingerprint()
{
    RWStreamBuf stream;
    // Same byte order as in save files
    stream.setBigendian( true );

    stream << World::Get() << GameOver::Result::Get();

    if ( stream.fail() ) {
        ERROR_LOG( "Failed to serialize the game state." )
        return {};
    }

    return { computeFnv1aHash( stream.data(), stream.size() ), stream.size() };
}

void Multi::logStateFingerprint( const std::string_view context )
{
    // The whole game is serialized here
    if ( !isNetworkGame() ) {
        return;
    }

    const StateFingerprint fingerprint = computeStateFingerprint();

    VERBOSE_LOG( "[MULTI] State fingerprint " << context << ": " << fingerprint.hashString() << " (" << fingerprint.size << " bytes)" )
}
