/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "serialize.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <ostream>
#include <string>

#include "endian_h2.h"
#include "logging.h"

namespace
{
    const size_t minBufferCapacity = 1024;
}

void StreamBase::setconstbuf( bool f )
{
    if ( f ) {
        _flags |= CONST_BUF;
    }
    else {
        _flags &= ~CONST_BUF;
    }
}

void StreamBase::setbigendian( bool f )
{
    if ( f ) {
        _flags |= BIGENDIAN;
    }
    else {
        _flags &= ~BIGENDIAN;
    }
}

void StreamBase::setfail( bool f )
{
    if ( f ) {
        _flags |= FAILURE;
    }
    else {
        _flags &= ~FAILURE;
    }
}

uint16_t StreamBase::get16()
{
    return bigendian() ? getBE16() : getLE16();
}

uint32_t StreamBase::get32()
{
    return bigendian() ? getBE32() : getLE32();
}

StreamBase & StreamBase::operator>>( bool & v )
{
    v = ( get8() != 0 );
    return *this;
}

StreamBase & StreamBase::operator>>( char & v )
{
    v = get8();
    return *this;
}

StreamBase & StreamBase::operator>>( int8_t & v )
{
    v = static_cast<int8_t>( get8() );
    return *this;
}

StreamBase & StreamBase::operator>>( uint8_t & v )
{
    v = get8();
    return *this;
}

StreamBase & StreamBase::operator>>( uint16_t & v )
{
    v = get16();
    return *this;
}

StreamBase & StreamBase::operator>>( int16_t & v )
{
    v = get16();
    return *this;
}

StreamBase & StreamBase::operator>>( uint32_t & v )
{
    v = get32();
    return *this;
}

StreamBase & StreamBase::operator>>( int32_t & v )
{
    v = get32();
    return *this;
}

StreamBase & StreamBase::operator>>( std::string & v )
{
    uint32_t size = get32();
    v.resize( size );

    for ( std::string::iterator it = v.begin(); it != v.end(); ++it )
        *it = get8();

    return *this;
}

StreamBase & StreamBase::operator>>( fheroes2::Point & point_ )
{
    return *this >> point_.x >> point_.y;
}

void StreamBase::put16( uint16_t v )
{
    bigendian() ? putBE16( v ) : putLE16( v );
}

void StreamBase::put32( uint32_t v )
{
    bigendian() ? putBE32( v ) : putLE32( v );
}

StreamBase & StreamBase::operator<<( const bool v )
{
    put8( v );
    return *this;
}

StreamBase & StreamBase::operator<<( const char v )
{
    put8( v );
    return *this;
}

StreamBase & StreamBase::operator<<( const int8_t v )
{
    put8( static_cast<uint8_t>( v ) );
    return *this;
}

StreamBase & StreamBase::operator<<( const uint8_t v )
{
    put8( v );
    return *this;
}

StreamBase & StreamBase::operator<<( const uint16_t v )
{
    put16( v );
    return *this;
}

StreamBase & StreamBase::operator<<( const int16_t v )
{
    put16( v );
    return *this;
}

StreamBase & StreamBase::operator<<( const int32_t v )
{
    put32( v );
    return *this;
}

StreamBase & StreamBase::operator<<( const uint32_t v )
{
    put32( v );
    return *this;
}

StreamBase & StreamBase::operator<<( const std::string & v )
{
    put32( static_cast<uint32_t>( v.size() ) );
    // A string is a container of bytes so it doesn't matter which endianess is being used.
    putRaw( v.data(), v.size() );

    return *this;
}

StreamBase & StreamBase::operator<<( const fheroes2::Point & point_ )
{
    return *this << point_.x << point_.y;
}

StreamBuf::StreamBuf( const size_t sz )
{
    if ( sz ) {
        reallocbuf( sz );
    }

    setbigendian( IS_BIGENDIAN );
}

StreamBuf::~StreamBuf()
{
    if ( itbeg == nullptr || isconstbuf() ) {
        return;
    }

    delete[] itbeg;
}

StreamBuf::StreamBuf( StreamBuf && stream ) noexcept
    : StreamBase( std::move( stream ) )
{
    std::swap( itbeg, stream.itbeg );
    std::swap( itget, stream.itget );
    std::swap( itput, stream.itput );
    std::swap( itend, stream.itend );
}

StreamBuf::StreamBuf( const std::vector<uint8_t> & buf )
{
    itbeg = const_cast<uint8_t *>( buf.data() );
    itend = itbeg + buf.size();
    itget = itbeg;
    itput = itend;

    setconstbuf( true );
    setbigendian( IS_BIGENDIAN );
}

StreamBuf & StreamBuf::operator=( StreamBuf && stream ) noexcept
{
    if ( this == &stream ) {
        return *this;
    }

    StreamBase::operator=( std::move( stream ) );

    std::swap( itbeg, stream.itbeg );
    std::swap( itget, stream.itget );
    std::swap( itput, stream.itput );
    std::swap( itend, stream.itend );

    return *this;
}

void StreamBuf::reset()
{
    itput = itbeg;
    itget = itbeg;
}

size_t StreamBuf::tellg()
{
    return itget - itbeg;
}

size_t StreamBuf::tellp()
{
    return itput - itbeg;
}

size_t StreamBuf::sizeg()
{
    return itput - itget;
}

size_t StreamBuf::sizep()
{
    return itend - itput;
}

void StreamBuf::reallocbuf( size_t size )
{
    // Instances created from read-only memory blocks are read-only and should never be reallocated
    assert( !isconstbuf() );

    if ( !itbeg ) {
        if ( size < minBufferCapacity ) {
            size = minBufferCapacity;
        }

        itbeg = new uint8_t[size]{};
        itend = itbeg + size;

        reset();
    }
    else if ( sizep() < size ) {
        if ( size < minBufferCapacity ) {
            size = minBufferCapacity;
        }

        uint8_t * ptr = new uint8_t[size]{};

        std::copy( itbeg, itput, ptr );

        itput = ptr + tellp();
        itget = ptr + tellg();

        delete[] itbeg;

        itbeg = ptr;
        itend = itbeg + size;
    }
}

void StreamBuf::put8( const uint8_t v )
{
    if ( sizep() < 1 ) {
        reallocbuf( capacity() + capacity() / 2 );
    }

    if ( sizep() < 1 ) {
        assert( 0 );
        return;
    }

    *itput = v;
    ++itput;
}

uint8_t StreamBuf::get8()
{
    if ( sizeg() )
        return *itget++;
    else
        return 0u;
}

uint16_t StreamBuf::getBE16()
{
    uint16_t result = ( static_cast<uint16_t>( get8() ) << 8 );
    result |= get8();

    return result;
}

uint16_t StreamBuf::getLE16()
{
    uint16_t result = get8();
    result |= ( static_cast<uint16_t>( get8() ) << 8 );

    return result;
}

uint32_t StreamBuf::getBE32()
{
    uint32_t result = ( static_cast<uint32_t>( get8() ) << 24 );
    result |= ( static_cast<uint32_t>( get8() ) << 16 );
    result |= ( static_cast<uint32_t>( get8() ) << 8 );
    result |= get8();

    return result;
}

uint32_t StreamBuf::getLE32()
{
    uint32_t result = get8();
    result |= ( static_cast<uint32_t>( get8() ) << 8 );
    result |= ( static_cast<uint32_t>( get8() ) << 16 );
    result |= ( static_cast<uint32_t>( get8() ) << 24 );

    return result;
}

void StreamBuf::putBE16( uint16_t v )
{
    put8( v >> 8 );
    put8( v & 0xFF );
}

void StreamBuf::putLE16( uint16_t v )
{
    put8( v & 0xFF );
    put8( v >> 8 );
}

void StreamBuf::putBE32( uint32_t v )
{
    put8( v >> 24 );
    put8( ( v >> 16 ) & 0xFF );
    put8( ( v >> 8 ) & 0xFF );
    put8( v & 0xFF );
}

void StreamBuf::putLE32( uint32_t v )
{
    put8( v & 0xFF );
    put8( ( v >> 8 ) & 0xFF );
    put8( ( v >> 16 ) & 0xFF );
    put8( v >> 24 );
}

std::vector<uint8_t> StreamBuf::getRaw( size_t sz )
{
    const size_t remainSize = sizeg();
    const size_t dataSize = sz > 0 ? sz : remainSize;

    std::vector<uint8_t> v( dataSize, 0 );
    const size_t copySize = dataSize < remainSize ? dataSize : remainSize;
    memcpy( v.data(), itget, copySize );

    itget += copySize;

    return v;
}

void StreamBuf::putRaw( const void * ptr, size_t sz )
{
    if ( sz == 0 ) {
        return;
    }

    if ( sizep() < sz ) {
        if ( sz < capacity() / 2 ) {
            reallocbuf( capacity() + capacity() / 2 );
        }
        else {
            reallocbuf( capacity() + sz );
        }
    }

    if ( sizep() < sz ) {
        assert( 0 );
        return;
    }

    memcpy( itput, ptr, sz );
    itput = itput + sz;
}

std::string StreamBuf::toString( const size_t size /* = 0 */ )
{
    const size_t length = ( size > 0 && size < sizeg() ) ? size : sizeg();

    uint8_t * it1 = itget;
    itget += length;
    uint8_t * it2 = std::find( it1, itget, 0 );

    return { it1, it2 };
}

void StreamBuf::skip( size_t sz )
{
    itget += sz <= sizeg() ? sz : sizeg();
}

bool StreamFile::open( const std::string & fn, const std::string & mode )
{
    _file.reset( std::fopen( fn.c_str(), mode.c_str() ) );
    if ( !_file ) {
        ERROR_LOG( "Error opening file " << fn )
    }

    setfail( !_file );

    return !fail();
}

void StreamFile::close()
{
    _file.reset();
}

size_t StreamFile::size()
{
    if ( !_file ) {
        return 0;
    }

    const long pos = std::ftell( _file.get() );
    if ( pos < 0 ) {
        setfail( true );

        return 0;
    }

    if ( std::fseek( _file.get(), 0, SEEK_END ) != 0 ) {
        setfail( true );

        return 0;
    }

    const long len = std::ftell( _file.get() );
    if ( len < 0 ) {
        setfail( true );

        return 0;
    }

    if ( std::fseek( _file.get(), pos, SEEK_SET ) != 0 ) {
        setfail( true );

        return 0;
    }

    return static_cast<size_t>( len );
}

size_t StreamFile::tell()
{
    return tellg();
}

void StreamFile::seek( size_t pos )
{
    if ( !_file ) {
        return;
    }

    if ( std::fseek( _file.get(), static_cast<long>( pos ), SEEK_SET ) != 0 ) {
        setfail( true );
    }
}

size_t StreamFile::sizeg()
{
    if ( !_file ) {
        return 0;
    }

    const long pos = std::ftell( _file.get() );
    if ( pos < 0 ) {
        setfail( true );

        return 0;
    }

    if ( std::fseek( _file.get(), 0, SEEK_END ) != 0 ) {
        setfail( true );

        return 0;
    }

    const long len = std::ftell( _file.get() );
    if ( len < 0 ) {
        setfail( true );

        return 0;
    }

    if ( std::fseek( _file.get(), pos, SEEK_SET ) != 0 ) {
        setfail( true );

        return 0;
    }

    // Something weird has happened
    if ( len < pos ) {
        setfail( true );

        return 0;
    }

    return static_cast<size_t>( len - pos );
}

size_t StreamFile::tellg()
{
    if ( !_file ) {
        return 0;
    }

    const long pos = std::ftell( _file.get() );
    if ( pos < 0 ) {
        setfail( true );

        return 0;
    }

    return static_cast<size_t>( pos );
}

size_t StreamFile::sizep()
{
    return sizeg();
}

size_t StreamFile::tellp()
{
    return tellg();
}

void StreamFile::skip( size_t pos )
{
    if ( !_file ) {
        return;
    }

    if ( std::fseek( _file.get(), static_cast<long int>( pos ), SEEK_CUR ) != 0 ) {
        setfail( true );
    }
}

uint8_t StreamFile::get8()
{
    return getUint<uint8_t>();
}

void StreamFile::put8( const uint8_t v )
{
    putUint<uint8_t>( v );
}

uint16_t StreamFile::getBE16()
{
    return be16toh( getUint<uint16_t>() );
}

uint16_t StreamFile::getLE16()
{
    return le16toh( getUint<uint16_t>() );
}

uint32_t StreamFile::getBE32()
{
    return be32toh( getUint<uint32_t>() );
}

uint32_t StreamFile::getLE32()
{
    return le32toh( getUint<uint32_t>() );
}

void StreamFile::putBE16( uint16_t val )
{
    putUint<uint16_t>( htobe16( val ) );
}

void StreamFile::putLE16( uint16_t val )
{
    putUint<uint16_t>( htole16( val ) );
}

void StreamFile::putBE32( uint32_t val )
{
    putUint<uint32_t>( htobe32( val ) );
}

void StreamFile::putLE32( uint32_t val )
{
    putUint<uint32_t>( htole32( val ) );
}

std::vector<uint8_t> StreamFile::getRaw( const size_t size )
{
    const size_t chunkSize = size > 0 ? size : sizeg();
    if ( chunkSize == 0 || !_file ) {
        return {};
    }

    std::vector<uint8_t> v( chunkSize );

    if ( std::fread( v.data(), chunkSize, 1, _file.get() ) != 1 ) {
        setfail( true );

        return {};
    }

    return v;
}

void StreamFile::putRaw( const void * ptr, size_t sz )
{
    if ( sz == 0 ) {
        // Nothing to write. Ignore it.
        return;
    }

    if ( !_file ) {
        return;
    }

    if ( std::fwrite( ptr, sz, 1, _file.get() ) != 1 ) {
        setfail( true );
    }
}

StreamBuf StreamFile::toStreamBuf( const size_t size /* = 0 */ )
{
    const size_t chunkSize = size > 0 ? size : sizeg();
    if ( chunkSize == 0 || !_file ) {
        return StreamBuf{};
    }

    StreamBuf buffer( chunkSize );

    if ( std::fread( buffer.dataForWriting(), chunkSize, 1, _file.get() ) != 1 ) {
        setfail( true );

        return StreamBuf{};
    }

    buffer.advance( chunkSize );

    return buffer;
}

std::string StreamFile::toString( const size_t size /* = 0 */ )
{
    const std::vector<uint8_t> buf = getRaw( size );

    const auto itend = std::find( buf.begin(), buf.end(), 0 );

    return { buf.begin(), itend };
}
