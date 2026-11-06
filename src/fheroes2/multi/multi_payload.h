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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

class IStreamBase;
class OStreamBase;
class ROStreamBuf;
class RWStreamBuf;

// Helpers for the payloads of messages, which are big-endian.
namespace Multi
{
    void writeUint64( OStreamBase & stream, const uint64_t value );
    uint64_t readUint64( IStreamBase & stream );

    // Never trust a size coming from the network: a string longer than the given size or than the remaining data is refused.
    bool readString( ROStreamBuf & stream, std::string & value, const uint32_t maxSize );

    std::vector<uint8_t> toPayload( const RWStreamBuf & stream );
}
