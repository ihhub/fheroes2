/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include <cstring>
#include <sstream>

#ifdef WITH_ZLIB
#include <zlib.h>
#endif

#include "logging.h"
#include "zzlib.h"

#ifdef WITH_ZLIB

std::vector<u8> zlibDecompress( const u8 * src, size_t srcsz, size_t realsz /* = 0 */ )
{
    std::vector<u8> res;

    if ( src && srcsz ) {
        if ( realsz )
            res.reserve( realsz );
        res.resize( ( realsz ? realsz : srcsz * 7 ), 0 );
        uLong dstsz = static_cast<uLong>( res.size() );
        int ret = Z_BUF_ERROR;

        while ( Z_BUF_ERROR
                == ( ret = uncompress( reinterpret_cast<Bytef *>( &res[0] ), &dstsz, reinterpret_cast<const Bytef *>( src ), static_cast<uLong>( srcsz ) ) ) ) {
            dstsz = static_cast<uLong>( res.size() * 2 );
            res.resize( dstsz );
        }

        if ( ret == Z_OK )
            res.resize( dstsz );
        else {
            res.clear();
            std::ostringstream os;
            os << "zlib error:" << ret;
            ERROR_LOG( os.str().c_str() );
        }
    }

    return res;
}

std::vector<u8> zlibCompress( const u8 * src, size_t srcsz )
{
    std::vector<u8> res;

    if ( src && srcsz ) {
        res.resize( compressBound( static_cast<uLong>( srcsz ) ) );
        uLong dstsz = static_cast<uLong>( res.size() );
        int ret = compress( reinterpret_cast<Bytef *>( &res[0] ), &dstsz, reinterpret_cast<const Bytef *>( src ), static_cast<uLong>( srcsz ) );

        if ( ret == Z_OK )
            res.resize( dstsz );
        else {
            res.clear();
            std::ostringstream os;
            os << "zlib error:" << ret;
            ERROR_LOG( os.str().c_str() );
        }
    }

    return res;
}

fheroes2::Image CreateImageFromZlib( int32_t width, int32_t height, const uint8_t * imageData, size_t imageSize, bool doubleLayer )
{
    if ( imageData == NULL || imageSize == 0 || width <= 0 || height <= 0 )
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
        std::fill( out.transform(), out.transform() + uncompressedSize, 0 );
    }
    return out;
}

#endif // WITH_ZLIB

ZStreamFile::ZStreamFile( bool compressIfSupported )
    : StreamBuf()
    , _compressIfSupported( compressIfSupported )
{
#ifndef WITH_ZLIB
    (void)_compressIfSupported; // Silence the "private field is not used" compiler warning
#endif
}

bool ZStreamFile::read( const std::string & fn, size_t offset /* = 0 */ )
{
    StreamFile sf;
    sf.setbigendian( true );

    if ( sf.open( fn, "rb" ) ) {
        if ( offset )
            sf.seek( offset );
#ifdef WITH_ZLIB
        if ( _compressIfSupported ) {
            const u32 size0 = sf.get32(); // raw size
            if ( size0 == 0 ) {
                return false;
            }
            const u32 size1 = sf.get32(); // zip size
            if ( size1 == 0 ) {
                return false;
            }
            sf.skip( 4 ); // old stream format
            std::vector<u8> zip = sf.getRaw( size1 );
            std::vector<u8> raw = zlibDecompress( &zip[0], zip.size(), size0 );
            putRaw( reinterpret_cast<char *>( &raw[0] ), raw.size() );
            seek( 0 );
        }
        else {
#endif
            const u32 size0 = sf.get32(); // raw size
            if ( size0 == 0 ) {
                return false;
            }
            std::vector<u8> raw = sf.getRaw( size0 );
            putRaw( reinterpret_cast<char *>( &raw[0] ), raw.size() );
            seek( 0 );
#ifdef WITH_ZLIB
        }
#endif
        return !fail();
    }
    return false;
}

bool ZStreamFile::write( const std::string & fn, bool append /* = false */ ) const
{
    StreamFile sf;
    sf.setbigendian( true );

    if ( sf.open( fn, append ? "ab" : "wb" ) ) {
#ifdef WITH_ZLIB
        if ( _compressIfSupported ) {
            std::vector<u8> zip = zlibCompress( data(), size() );

            if ( !zip.empty() ) {
                sf.put32( static_cast<uint32_t>( size() ) );
                sf.put32( static_cast<uint32_t>( zip.size() ) );
                sf.put32( 0 ); // unused, old format support
                sf.putRaw( reinterpret_cast<char *>( &zip[0] ), zip.size() );
                return !sf.fail();
            }
        }
        else {
#endif
            sf.put32( static_cast<uint32_t>( size() ) );
            sf.putRaw( reinterpret_cast<const char *>( data() ), size() );
            return !sf.fail();
#ifdef WITH_ZLIB
        }
#endif
    }
    return false;
}
