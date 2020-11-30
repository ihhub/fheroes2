/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include <cctype>
#include <iterator>
#include <string>

#include "agg_file.h"

namespace fheroes2
{
    AGGFile::AGGFile() {}

    uint32_t AGGFile::aggFilenameHash( const std::string & s )
    {
        uint32_t a = 0, b = 0;
        for ( auto cri = s.crbegin(); cri != s.crend(); cri++ ) {
            a = ( a << 5 ) + ( a >> 25 );
            uint8_t c = static_cast<uint8_t>( std::toupper( *cri ) );
            b += c;
            a += b + c;
        }
        return a;
    }

    bool AGGFile::isGood() const
    {
        return !_stream.fail() && _files.size();
    }

    bool AGGFile::open( const std::string & filename )
    {
        if ( !_stream.open( filename, "rb" ) )
            return false;

        const size_t size = _stream.size();
        const size_t count = _stream.getLE16();
        const size_t fileRecordSize = sizeof( uint32_t ) * 3;

        if ( count * ( fileRecordSize + _maxFilenameSize ) >= size )
            return false;

        StreamBuf fileEntries = _stream.toStreamBuf( count * fileRecordSize );
        const size_t nameEntriesSize = _maxFilenameSize * count;
        _stream.seek( size - nameEntriesSize );
        StreamBuf nameEntries = _stream.toStreamBuf( nameEntriesSize );

        for ( uint16_t i = 0; i < count; ++i ) {
            const uint32_t hash = fileEntries.getLE32();
            const std::string & name = nameEntries.toString( _maxFilenameSize );
            if ( hash == aggFilenameHash( name ) && !_files.count( hash ) ) {
                const uint32_t fileOffset = fileEntries.getLE32();
                const uint32_t fileSize = fileEntries.getLE32();
                _files[hash] = std::make_pair( fileSize, fileOffset );
                _names[name] = hash;
            }
            else {
                _files.clear();
                _names.clear();
                return false;
            }
        }
        return !_stream.fail();
    }

    const std::vector<uint8_t> & AGGFile::read( uint32_t hash )
    {
        auto it = _files.find( hash );
        if ( it != _files.end() ) {
            auto f = it->second;
            if ( f.first ) {
                _stream.seek( f.second );
                _body = _stream.getRaw( f.first );
                return _body;
            }
        }
        _body.clear();
        _key.clear();
        return _body;
    }

    const std::vector<uint8_t> & AGGFile::read( const std::string & str )
    {
        if ( _key != str ) {
            auto it = _names.find( str );
            if ( it != _names.end() ) {
                _key = str;
                return read( it->second );
            }
        }
        _body.clear();
        _key.clear();
        return _body;
    }
}
