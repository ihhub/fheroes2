/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include "h2d_file.h"

#include <cassert>
#include <cstdint>
#include <cstring>

#include "image.h"

namespace
{
    // 4 bytes - file identifier
    // 4 bytes - number of files
    // 4 bytes - file offset
    // 4 bytes - file size
    // 5 bytes - file name
    const size_t minFileSize = 4 + 4 + 4 + 4 + 5 + 1;
}

namespace fheroes2
{
    bool H2DReader::open( const std::string & path )
    {
        _fileNameAndOffset.clear();
        _fileStream.close();

        if ( !_fileStream.open( path, "rb" ) ) {
            return false;
        }

        const size_t fileSize = _fileStream.size();
        if ( fileSize < minFileSize ) {
            return false;
        }

        if ( _fileStream.get() != 'H' ) {
            return false;
        }
        if ( _fileStream.get() != '2' ) {
            return false;
        }
        if ( _fileStream.get() != 'D' ) {
            return false;
        }
        if ( _fileStream.get() != '\0' ) {
            return false;
        }

        const uint32_t fileCount = _fileStream.getLE32();
        if ( fileCount == 0 ) {
            return false;
        }

        for ( uint32_t i = 0; i < fileCount; ++i ) {
            const uint32_t offset = _fileStream.getLE32();
            const uint32_t size = _fileStream.getLE32();
            std::string name;
            _fileStream >> name;
            if ( size == 0 || offset + size > fileSize || name.empty() ) {
                continue;
            }

            _fileNameAndOffset.try_emplace( std::move( name ), std::make_pair( offset, size ) );
        }

        return true;
    }

    std::vector<uint8_t> H2DReader::getFile( const std::string & fileName )
    {
        const auto it = _fileNameAndOffset.find( fileName );
        if ( it == _fileNameAndOffset.end() ) {
            return std::vector<uint8_t>();
        }

        _fileStream.seek( it->second.first );
        return _fileStream.getRaw( it->second.second );
    }

    std::set<std::string, std::less<>> H2DReader::getAllFileNames() const
    {
        std::set<std::string, std::less<>> names;

        for ( const auto & value : _fileNameAndOffset ) {
            names.insert( value.first );
        }

        return names;
    }

    bool H2DWriter::write( const std::string & path ) const
    {
        if ( _fileData.empty() ) {
            // Nothing to write.
            return false;
        }

        StreamFile fileStream;
        if ( !fileStream.open( path, "wb" ) ) {
            return false;
        }

        fileStream.put( 'H' );
        fileStream.put( '2' );
        fileStream.put( 'D' );
        fileStream.put( '\0' );

        fileStream.putLE32( static_cast<uint32_t>( _fileData.size() ) );

        // Calculate file info section size.
        size_t fileInfoSection = ( 4 + 4 ) * _fileData.size();
        for ( const auto & data : _fileData ) {
            // 4 byte for string size.
            fileInfoSection += ( data.first.size() + 4 );
        }

        size_t offset = fileInfoSection + 4 + 4;
        for ( const auto & data : _fileData ) {
            fileStream.putLE32( static_cast<uint32_t>( offset ) );
            fileStream.putLE32( static_cast<uint32_t>( data.second.size() ) );
            fileStream << data.first;
            offset += data.second.size();
        }

        for ( const auto & data : _fileData ) {
            fileStream.putRaw( data.second.data(), data.second.size() );
        }

        return true;
    }

    bool H2DWriter::add( const std::string & name, const std::vector<uint8_t> & data )
    {
        if ( name.empty() || data.empty() ) {
            return false;
        }

        _fileData[name] = data;
        return true;
    }

    bool H2DWriter::add( H2DReader & reader )
    {
        const std::set<std::string, std::less<>> names = reader.getAllFileNames();

        for ( const std::string & name : names ) {
            if ( !add( name, reader.getFile( name ) ) ) {
                return false;
            }
        }

        return true;
    }

    bool readImageFromH2D( H2DReader & reader, const std::string & name, Sprite & image )
    {
        // TODO: Store in h2d images the 'isSingleLayer' state to disable and skip transform layer for such images.
        assert( !image.singleLayer() );

        const std::vector<uint8_t> & data = reader.getFile( name );
        if ( data.size() < 4 + 4 + 4 + 4 + 1 ) {
            // Empty or invalid image.
            return false;
        }

        ROStreamBuf stream( data );
        const int32_t width = static_cast<int32_t>( stream.getLE32() );
        const int32_t height = static_cast<int32_t>( stream.getLE32() );
        const int32_t x = static_cast<int32_t>( stream.getLE32() );
        const int32_t y = static_cast<int32_t>( stream.getLE32() );
        if ( static_cast<size_t>( width * height * 2 + 4 + 4 + 4 + 4 ) != data.size() ) {
            return false;
        }

        const size_t size = static_cast<size_t>( width * height );
        image.resize( width, height );
        memcpy( image.image(), data.data() + 4 + 4 + 4 + 4, size );
        memcpy( image.transform(), data.data() + 4 + 4 + 4 + 4 + size, size );

        image.setPosition( x, y );

        return true;
    }

    bool writeImageToH2D( H2DWriter & writer, const std::string & name, const Sprite & image )
    {
        // TODO: Store in h2d images the 'isSingleLayer' state to disable and skip transform layer for such images.
        assert( !image.empty() && !image.singleLayer() );

        RWStreamBuf stream;
        stream.putLE32( static_cast<uint32_t>( image.width() ) );
        stream.putLE32( static_cast<uint32_t>( image.height() ) );
        stream.putLE32( static_cast<uint32_t>( image.x() ) );
        stream.putLE32( static_cast<uint32_t>( image.y() ) );

        const size_t imageSize = static_cast<size_t>( image.width() ) * static_cast<size_t>( image.height() );
        stream.putRaw( image.image(), imageSize );
        stream.putRaw( image.transform(), imageSize );

        return writer.add( name, stream.getRaw( 0 ) );
    }
}
