/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#include "agg_file.h"

#include <cctype>
#include <cstdint>
#include <iterator>
#include <string>

namespace fheroes2
{
    bool AGGFile::open( const std::string & fileName )
    {
        if ( !_stream.open( fileName, "rb" ) ) {
            return false;
        }

        const size_t size = _stream.size();
        const size_t count = _stream.getLE16();
        const size_t fileRecordSize = sizeof( uint32_t ) * 3;

        if ( count * ( fileRecordSize + _maxFilenameSize ) >= size ) {
            return false;
        }

        ROStreamBuf fileEntries = _stream.getStreamBuf( count * fileRecordSize );
        const size_t nameEntriesSize = _maxFilenameSize * count;
        _stream.seek( size - nameEntriesSize );
        ROStreamBuf nameEntries = _stream.getStreamBuf( nameEntriesSize );

        for ( size_t i = 0; i < count; ++i ) {
            std::string name = nameEntries.getString( _maxFilenameSize );

            // Check 32-bit filename hash.
            if ( fileEntries.getLE32() != calculateAggFilenameHash( name ) ) {
                // Hash check failed. AGG file is corrupted.
                _files.clear();
                return false;
            }

            const uint32_t fileOffset = fileEntries.getLE32();
            const uint32_t fileSize = fileEntries.getLE32();
            _files.try_emplace( std::move( name ), std::make_pair( fileSize, fileOffset ) );
        }

        if ( _files.size() != count ) {
            _files.clear();
            return false;
        }

        return !_stream.fail();
    }

    std::vector<uint8_t> AGGFile::read( const std::string & fileName )
    {
        auto it = _files.find( fileName );
        if ( it == _files.end() ) {
            return {};
        }

        const auto [fileSize, fileOffset] = it->second;
        if ( fileSize > 0 ) {
            _stream.seek( fileOffset );
            return _stream.getRaw( fileSize );
        }

        return {};
    }

    uint32_t calculateAggFilenameHash( const std::string_view str )
    {
        uint32_t hash = 0;
        uint32_t sum = 0;

        for ( auto iter = str.rbegin(); iter != str.rend(); ++iter ) {
            const unsigned char c = static_cast<unsigned char>( std::toupper( static_cast<unsigned char>( *iter ) ) );

            hash = ( hash << 5 ) + ( hash >> 25 );

            sum += c;
            hash += sum + c;
        }

        return hash;
    }
}

IStreamBase & operator>>( IStreamBase & stream, fheroes2::ICNHeader & icn )
{
    icn.offsetX = static_cast<int16_t>( stream.getLE16() );
    icn.offsetY = static_cast<int16_t>( stream.getLE16() );
    icn.width = stream.getLE16();
    icn.height = stream.getLE16();
    icn.animationFrames = stream.get();
    icn.offsetData = stream.getLE32();

    return stream;
}
