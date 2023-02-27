/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "zzlib.h"

#include <algorithm>
#include <cstring>
#include <ostream>
#include <vector>

#include <zconf.h>
#include <zlib.h>

#include "logging.h"

namespace
{
    constexpr uint16_t FORMAT_VERSION_0 = 0;

    std::vector<uint8_t> zlibDecompress( const uint8_t * src, const size_t srcsz, const size_t realsz = 0 )
    {
        if ( src == nullptr || srcsz == 0 ) {
            return {};
        }

        std::vector<uint8_t> res;
        res.resize( ( realsz ? realsz : srcsz * 7 ), 0 );

        uLong dstsz = static_cast<uLong>( res.size() );
        int ret = Z_BUF_ERROR;

        while ( Z_BUF_ERROR
                == ( ret = uncompress( reinterpret_cast<Bytef *>( res.data() ), &dstsz, reinterpret_cast<const Bytef *>( src ), static_cast<uLong>( srcsz ) ) ) ) {
            dstsz = static_cast<uLong>( res.size() * 2 );
            res.resize( dstsz );
        }

        if ( ret == Z_OK ) {
            res.resize( dstsz );
        }
        else {
            res.clear();
            ERROR_LOG( "zlib error: " << ret )
        }

        return res;
    }

    std::vector<uint8_t> zlibCompress( const uint8_t * src, const size_t srcsz )
    {
        if ( src == nullptr || srcsz == 0 ) {
            return {};
        }

        std::vector<uint8_t> res;
        res.resize( compressBound( static_cast<uLong>( srcsz ) ) );

        uLong dstsz = static_cast<uLong>( res.size() );
        const int ret = compress( reinterpret_cast<Bytef *>( res.data() ), &dstsz, reinterpret_cast<const Bytef *>( src ), static_cast<uLong>( srcsz ) );

        if ( ret == Z_OK ) {
            res.resize( dstsz );
        }
        else {
            res.clear();
            ERROR_LOG( "zlib error: " << ret )
        }

        return res;
    }
}

bool ZStreamBuf::read( const std::string & fn, const size_t offset /* = 0 */ )
{
    StreamFile sf;
    sf.setbigendian( true );

    if ( !sf.open( fn, "rb" ) ) {
        return false;
    }

    if ( offset ) {
        sf.seek( offset );
    }

    const uint32_t rawSize = sf.get32();
    const uint32_t zipSize = sf.get32();
    if ( zipSize == 0 ) {
        return false;
    }

    const uint16_t version = sf.get16();
    if ( version != FORMAT_VERSION_0 ) {
        return false;
    }

    sf.skip( 2 ); // Unused bytes

    const std::vector<uint8_t> zip = sf.getRaw( zipSize );
    const std::vector<uint8_t> raw = zlibDecompress( zip.data(), zip.size(), rawSize );
    if ( raw.size() != rawSize ) {
        return false;
    }

    putRaw( reinterpret_cast<const char *>( raw.data() ), raw.size() );

    return !fail();
}

bool ZStreamBuf::write( const std::string & fn, const bool append /* = false */ ) const
{
    StreamFile sf;
    sf.setbigendian( true );

    if ( !sf.open( fn, append ? "ab" : "wb" ) ) {
        return false;
    }

    const std::vector<uint8_t> zip = zlibCompress( data(), size() );
    if ( zip.empty() ) {
        return false;
    }

    sf.put32( static_cast<uint32_t>( size() ) );
    sf.put32( static_cast<uint32_t>( zip.size() ) );
    sf.put16( FORMAT_VERSION_0 );
    sf.put16( 0 ); // Unused bytes
    sf.putRaw( reinterpret_cast<const char *>( zip.data() ), zip.size() );

    return !sf.fail();
}

fheroes2::Image CreateImageFromZlib( int32_t width, int32_t height, const uint8_t * imageData, size_t imageSize, bool doubleLayer )
{
    if ( imageData == nullptr || imageSize == 0 || width <= 0 || height <= 0 )
        return fheroes2::Image();

    const std::vector<uint8_t> & uncompressedData = zlibDecompress( imageData, imageSize );
    if ( doubleLayer && ( uncompressedData.size() & 1 ) == 1 ) {
        return fheroes2::Image();
    }

    const size_t uncompressedSize = doubleLayer ? uncompressedData.size() / 2 : uncompressedData.size();

    if ( static_cast<size_t>( width * height ) != uncompressedSize )
        return fheroes2::Image();

    fheroes2::Image out( width, height );

    std::memcpy( out.image(), uncompressedData.data(), uncompressedSize );
    if ( doubleLayer ) {
        std::memcpy( out.transform(), uncompressedData.data() + uncompressedSize, uncompressedSize );
    }
    else {
        std::fill( out.transform(), out.transform() + uncompressedSize, static_cast<uint8_t>( 0 ) );
    }
    return out;
}
