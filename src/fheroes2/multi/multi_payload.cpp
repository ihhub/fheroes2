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

#include "multi_payload.h"

#include "serialize.h"

void Multi::writeUint64( OStreamBase & stream, const uint64_t value )
{
    stream << static_cast<uint32_t>( value >> 32 ) << static_cast<uint32_t>( value & 0xFFFFFFFFU );
}

uint64_t Multi::readUint64( IStreamBase & stream )
{
    uint32_t high = 0;
    uint32_t low = 0;
    stream >> high >> low;

    return ( static_cast<uint64_t>( high ) << 32 ) | low;
}

bool Multi::readString( ROStreamBuf & stream, std::string & value, const uint32_t maxSize )
{
    uint32_t size = 0;
    stream >> size;

    if ( stream.fail() || size > maxSize || size > stream.size() ) {
        return false;
    }

    value.clear();

    // getRaw( 0 ) would return all the remaining data.
    if ( size > 0 ) {
        const std::vector<uint8_t> bytes = stream.getRaw( size );
        value.assign( bytes.begin(), bytes.end() );
    }

    return true;
}

std::vector<uint8_t> Multi::toPayload( const RWStreamBuf & stream )
{
    return { stream.data(), stream.data() + stream.size() };
}
