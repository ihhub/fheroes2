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

RWStreamBuf::RWStreamBuf( const size_t sz )
{
    if ( sz ) {
        reallocBuf( sz );
    }

    setbigendian( IS_BIGENDIAN );
}

void RWStreamBuf::putBE16( uint16_t v )
{
    put8( v >> 8 );
    put8( v & 0xFF );
}

void RWStreamBuf::putLE16( uint16_t v )
{
    put8( v & 0xFF );
    put8( v >> 8 );
}

void RWStreamBuf::putBE32( uint32_t v )
{
    put8( v >> 24 );
    put8( ( v >> 16 ) & 0xFF );
    put8( ( v >> 8 ) & 0xFF );
    put8( v & 0xFF );
}

void RWStreamBuf::putLE32( uint32_t v )
{
    put8( v & 0xFF );
    put8( ( v >> 8 ) & 0xFF );
    put8( ( v >> 16 ) & 0xFF );
    put8( v >> 24 );
}

void RWStreamBuf::putRaw( const void * ptr, size_t sz )
{
    if ( sz == 0 ) {
        return;
    }

    if ( sizep() < sz ) {
        if ( sz < capacity() / 2 ) {
            reallocBuf( capacity() + capacity() / 2 );
        }
        else {
            reallocBuf( capacity() + sz );
        }
    }

    if ( sizep() < sz ) {
        assert( 0 );
        return;
    }

    memcpy( _itput, ptr, sz );

    _itput = _itput + sz;
}

void RWStreamBuf::put8( const uint8_t v )
{
    if ( sizep() < 1 ) {
        reallocBuf( capacity() + capacity() / 2 );
    }

    if ( sizep() < 1 ) {
        assert( 0 );
        return;
    }

    *_itput = v;
    ++_itput;
}

void RWStreamBuf::reallocBuf( size_t size )
{
    if ( !_buf ) {
        assert( _itbeg == nullptr && _itget == nullptr && _itput == nullptr && _itend == nullptr );

        size = std::max( size, minBufferCapacity );

        _buf = std::make_unique<uint8_t[]>( size );

        _itbeg = _buf.get();
        _itend = _itbeg + size;

        _itput = _itbeg;
        _itget = _itbeg;

        return;
    }

    assert( _itbeg != nullptr && _itget != nullptr && _itput != nullptr && _itend != nullptr );

    if ( sizep() < size ) {
        size = std::max( size, minBufferCapacity );

        std::unique_ptr<uint8_t[]> newBuf = std::make_unique<uint8_t[]>( size );

        std::copy( _itbeg, _itput, newBuf.get() );

        _itput = newBuf.get() + tellp();
        _itget = newBuf.get() + tellg();

        _buf = std::move( newBuf );

        _itbeg = _buf.get();
        _itend = _itbeg + size;
    }
}

ROStreamBuf::ROStreamBuf( const std::vector<uint8_t> & buf )
{
    _itbeg = buf.data();
    _itend = _itbeg + buf.size();
    _itget = _itbeg;
    _itput = _itend;

    setbigendian( IS_BIGENDIAN );
}

void ROStreamBuf::putBE16( uint16_t /* v */ )
{
    assert( 0 );
}

void ROStreamBuf::putLE16( uint16_t /* v */ )
{
    assert( 0 );
}

void ROStreamBuf::putBE32( uint32_t /* v */ )
{
    assert( 0 );
}

void ROStreamBuf::putLE32( uint32_t /* v */ )
{
    assert( 0 );
}

void ROStreamBuf::putRaw( const void * /* ptr */, size_t /* sz */ )
{
    assert( 0 );
}

void ROStreamBuf::put8( const uint8_t /* v */ )
{
    assert( 0 );
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

RWStreamBuf StreamFile::toStreamBuf( const size_t size /* = 0 */ )
{
    const size_t chunkSize = size > 0 ? size : sizeg();
    if ( chunkSize == 0 || !_file ) {
        return {};
    }

    RWStreamBuf buffer( chunkSize );

    if ( std::fread( buffer.rwData(), chunkSize, 1, _file.get() ) != 1 ) {
        setfail( true );

        return {};
    }

    buffer.advance( chunkSize );

    return buffer;
}

std::string StreamFile::toString( const size_t size /* = 0 */ )
{
    const std::vector<uint8_t> buf = getRaw( size );

    return { buf.begin(), std::find( buf.begin(), buf.end(), 0 ) };
}
