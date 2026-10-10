/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2026                                             *
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

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>

#include "image.h"
#include "zzlib.h"

namespace
{
    // 4 bytes - file identifier
    // 4 bytes - number of files
    // 4 bytes - file offset
    // 4 bytes - file size
    // 5 bytes - file name
    const size_t minFileSize = 4 + 4 + 4 + 4 + 5 + 1;

    const uint8_t version{ 3U };
    const std::array<uint8_t, 4> magicSequence{ 'H', '2', 'D', version };
}

namespace fheroes2
{
    bool H2DReader::open( const std::string & path )
    {
        _fileNameVsInfo.clear();
        _fileStream.close();

        if ( !_fileStream.open( path, "rb" ) ) {
            return false;
        }

        const size_t fileSize = _fileStream.size();
        if ( fileSize < minFileSize ) {
            return false;
        }

        for ( const uint8_t value : magicSequence ) {
            if ( _fileStream.get() != value ) {
                return false;
            }
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

            std::string extraInfo;
            _fileStream >> extraInfo;
            if ( size == 0 || static_cast<size_t>( offset ) + size > fileSize || name.empty() ) {
                continue;
            }

            _fileNameVsInfo.try_emplace( std::move( name ), EntryInfo{ offset, size, std::move( extraInfo ) } );
        }

        return true;
    }

    std::vector<uint8_t> H2DReader::getFile( const std::string & fileName )
    {
        const auto it = _fileNameVsInfo.find( fileName );
        if ( it == _fileNameVsInfo.end() ) {
            return {};
        }

        _fileStream.seek( it->second.offset );
        const auto compressedData = _fileStream.getRaw( it->second.size );

        return Compression::unzipData( compressedData.data(), compressedData.size() );
    }

    std::set<std::string, std::less<>> H2DReader::getAllFileNames() const
    {
        std::set<std::string, std::less<>> names;

        for ( const auto & [name, info] : _fileNameVsInfo ) {
            names.insert( name );
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

        for ( const uint8_t value : magicSequence ) {
            fileStream.put( value );
        }

        fileStream.putLE32( static_cast<uint32_t>( _fileData.size() ) );

        // Calculate file info section size.
        size_t fileInfoSection = ( 4 + 4 ) * _fileData.size();
        for ( const auto & [name, info] : _fileData ) {
            // 4 byte for string size.
            fileInfoSection += ( name.size() + 4 );
            fileInfoSection += ( info.extraInfo.size() + 4 );
        }

        // 4 bytes for magic sequence and 4 bytes for the number of files.
        size_t offset = fileInfoSection + 4 + 4;
        for ( const auto & [name, info] : _fileData ) {
            fileStream.putLE32( static_cast<uint32_t>( offset ) );
            fileStream.putLE32( static_cast<uint32_t>( info.data.size() ) );
            fileStream << name;
            fileStream << info.extraInfo;
            offset += info.data.size();
        }

        for ( const auto & [name, info] : _fileData ) {
            fileStream.putRaw( info.data.data(), info.data.size() );
        }

        return true;
    }

    bool H2DWriter::add( const std::string & name, const std::vector<uint8_t> & data, std::string extraInfo )
    {
        if ( name.empty() || data.empty() ) {
            return false;
        }

        _fileData[name] = { Compression::zipData( data.data(), data.size(), true ), std::move( extraInfo ) };
        return true;
    }

    bool H2DWriter::add( H2DReader & reader )
    {
        for ( const auto & [name, info] : reader.getAllEntries() ) {
            if ( !add( name, reader.getFile( name ), info.info ) ) {
                return false;
            }
        }

        return true;
    }

    bool readImageFromH2D( H2DReader & reader, const std::string & name, Sprite & image )
    {
        const size_t imageInfoLength{ 4 + 4 + 4 + 4 + 1 };

        const std::vector<uint8_t> & data = reader.getFile( name );
        if ( data.size() < imageInfoLength + 1 ) {
            // Empty or invalid image.
            return false;
        }

        ROStreamBuf stream( data );
        const int32_t width = static_cast<int32_t>( stream.getLE32() );
        const int32_t height = static_cast<int32_t>( stream.getLE32() );
        const int32_t x = static_cast<int32_t>( stream.getLE32() );
        const int32_t y = static_cast<int32_t>( stream.getLE32() );
        const bool isSingleLayer = ( stream.get() != 0 );

        const size_t size = static_cast<size_t>( width ) * static_cast<size_t>( height );

        if ( ( size * ( isSingleLayer ? 1 : 2 ) + imageInfoLength ) != data.size() ) {
            return false;
        }

        if ( isSingleLayer ) {
            image._disableTransformLayer();
        }

        image.resize( width, height );
        memcpy( image.image(), data.data() + imageInfoLength, size );

        if ( !isSingleLayer ) {
            memcpy( image.transform(), data.data() + imageInfoLength + size, size );
        }

        image.setPosition( x, y );

        return true;
    }

    bool writeImageToH2D( H2DWriter & writer, const std::string & name, const Sprite & image, std::string extraInfo )
    {
        assert( !image.empty() );

        RWStreamBuf stream;
        stream.putLE32( static_cast<uint32_t>( image.width() ) );
        stream.putLE32( static_cast<uint32_t>( image.height() ) );
        stream.putLE32( static_cast<uint32_t>( image.x() ) );
        stream.putLE32( static_cast<uint32_t>( image.y() ) );
        stream.put( image.singleLayer() );

        const size_t imageSize = static_cast<size_t>( image.width() ) * static_cast<size_t>( image.height() );
        stream.putRaw( image.image(), imageSize );
        if ( !image.singleLayer() ) {
            stream.putRaw( image.transform(), imageSize );
        }

        return writer.add( name, stream.getRaw( 0 ), std::move( extraInfo ) );
    }
}
