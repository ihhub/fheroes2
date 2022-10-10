/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>

#include "serialize.h"

namespace
{
    const size_t minBufferCapacity = 1024;
}

void StreamBase::setconstbuf( bool f )
{
    if ( f )
        flags |= 0x00001000;
    else
        flags &= ~0x00001000;
}

void StreamBase::setbigendian( bool f )
{
    if ( f )
        flags |= 0x80000000;
    else
        flags &= ~0x80000000;
}

void StreamBase::setfail( bool f )
{
    if ( f )
        flags |= 0x00000001;
    else
        flags &= ~0x00000001;
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

    for ( std::string::const_iterator it = v.begin(); it != v.end(); ++it )
        put8( *it );

    return *this;
}

StreamBase & StreamBase::operator<<( const fheroes2::Point & point_ )
{
    return *this << point_.x << point_.y;
}

StreamBuf::StreamBuf( const size_t sz )
    : itbeg( nullptr )
    , itget( nullptr )
    , itput( nullptr )
    , itend( nullptr )
{
    if ( sz )
        reallocbuf( sz );
    setbigendian( IS_BIGENDIAN ); /* default: hardware endian */
}

StreamBuf::~StreamBuf()
{
    if ( itbeg && !isconstbuf() )
        delete[] itbeg;
}

StreamBuf::StreamBuf( StreamBuf && st ) noexcept
    : StreamBase( std::move( st ) )
    , itbeg( nullptr )
    , itget( nullptr )
    , itput( nullptr )
    , itend( nullptr )
{
    std::swap( itbeg, st.itbeg );
    std::swap( itget, st.itget );
    std::swap( itput, st.itput );
    std::swap( itend, st.itend );
}

StreamBuf::StreamBuf( const std::vector<uint8_t> & buf )
    : itbeg( nullptr )
    , itget( nullptr )
    , itput( nullptr )
    , itend( nullptr )
{
    itbeg = const_cast<uint8_t *>( &buf[0] );
    itend = itbeg + buf.size();
    itget = itbeg;
    itput = itend;
    setconstbuf( true );
    setbigendian( IS_BIGENDIAN ); /* default: hardware endian */
}

StreamBuf::StreamBuf( const uint8_t * buf, size_t bufsz )
    : itbeg( nullptr )
    , itget( nullptr )
    , itput( nullptr )
    , itend( nullptr )
{
    itbeg = const_cast<uint8_t *>( buf );
    itend = itbeg + bufsz;
    itget = itbeg;
    itput = itend;
    setconstbuf( true );
    setbigendian( IS_BIGENDIAN ); /* default: hardware endian */
}

StreamBuf & StreamBuf::operator=( StreamBuf && st ) noexcept
{
    if ( &st != this ) {
        StreamBase::operator=( std::move( st ) );

        std::swap( itbeg, st.itbeg );
        std::swap( itget, st.itget );
        std::swap( itput, st.itput );
        std::swap( itend, st.itend );
    }

    return *this;
}

size_t StreamBuf::capacity() const
{
    return itend - itbeg;
}

const uint8_t * StreamBuf::data() const
{
    return itget;
}

size_t StreamBuf::size() const
{
    return sizeg();
}

void StreamBuf::reset()
{
    itput = itbeg;
    itget = itbeg;
}

size_t StreamBuf::tellg() const
{
    return itget - itbeg;
}

size_t StreamBuf::tellp() const
{
    return itput - itbeg;
}

size_t StreamBuf::sizeg() const
{
    return itput - itget;
}

size_t StreamBuf::sizep() const
{
    return itend - itput;
}

void StreamBuf::reallocbuf( size_t sz )
{
    setconstbuf( false );

    if ( !itbeg ) {
        if ( sz < minBufferCapacity )
            sz = minBufferCapacity;

        itbeg = new uint8_t[sz];
        itend = itbeg + sz;
        std::fill( itbeg, itend, static_cast<uint8_t>( 0 ) );

        reset();
    }
    else if ( sizep() < sz ) {
        if ( sz < minBufferCapacity )
            sz = minBufferCapacity;

        uint8_t * ptr = new uint8_t[sz];

        std::fill( ptr, ptr + sz, static_cast<uint8_t>( 0 ) );
        std::copy( itbeg, itput, ptr );

        itput = ptr + tellp();
        itget = ptr + tellg();

        delete[] itbeg;

        itbeg = ptr;
        itend = itbeg + sz;
    }
}

void StreamBuf::put8( const uint8_t v )
{
    if ( sizep() == 0 )
        reallocbuf( capacity() + capacity() / 2 );

    if ( sizep() > 0 )
        *itput++ = v;
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

void StreamBuf::putRaw( const char * ptr, size_t sz )
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

    // Make sure that the possible previous memory reallocation was correct.
    assert( sizep() >= sz );

    memcpy( itput, ptr, sz );
    itput = itput + sz;
}

std::string StreamBuf::toString( size_t sz )
{
    uint8_t * it1 = itget;
    uint8_t * it2 = itget + ( sz ? sz : sizeg() );
    it2 = std::find( it1, it2, 0 );
    itget = it1 + ( sz ? sz : sizeg() );
    return std::string( it1, it2 );
}

void StreamBuf::skip( size_t sz )
{
    itget += sz <= sizeg() ? sz : sizeg();
}

void StreamBuf::seek( size_t sz )
{
    itget = itbeg + sz < itend ? itbeg + sz : itend;
}

StreamFile::StreamFile()
    : _file( nullptr )
{}

StreamFile::~StreamFile()
{
    close();
}

bool StreamFile::open( const std::string & fn, const std::string & mode )
{
    _file = std::fopen( fn.c_str(), mode.c_str() );
    return _file != nullptr;
}

void StreamFile::close()
{
    if ( _file ) {
        std::fclose( _file );
        _file = nullptr;
    }
}

size_t StreamFile::size() const
{
    if ( !_file )
        return 0;
    const long pos = std::ftell( _file );
    std::fseek( _file, 0, SEEK_END );
    const long len = std::ftell( _file );
    std::fseek( _file, pos, SEEK_SET );
    return static_cast<size_t>( len );
}

size_t StreamFile::tell() const
{
    return tellg();
}

void StreamFile::seek( size_t pos )
{
    if ( _file )
        std::fseek( _file, static_cast<long>( pos ), SEEK_SET );
}

size_t StreamFile::sizeg() const
{
    if ( !_file )
        return 0;
    const long pos = std::ftell( _file );
    std::fseek( _file, 0, SEEK_END );
    const long len = std::ftell( _file );
    std::fseek( _file, pos, SEEK_SET );
    return static_cast<size_t>( len - pos );
}

size_t StreamFile::tellg() const
{
    return _file ? static_cast<size_t>( std::ftell( _file ) ) : 0;
}

size_t StreamFile::sizep() const
{
    return sizeg();
}

size_t StreamFile::tellp() const
{
    return tellg();
}

void StreamFile::skip( size_t pos )
{
    if ( _file )
        std::fseek( _file, static_cast<long int>( pos ), SEEK_CUR );
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

std::vector<uint8_t> StreamFile::getRaw( size_t sz )
{
    const size_t chunkSize = sz > 0 ? sz : sizeg();
    if ( chunkSize == 0 || !_file ) {
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> v( sz ? sz : sizeg() );
    const size_t count = std::fread( &v[0], chunkSize, 1, _file );
    if ( count != 1 ) {
        setfail( true );
        v.clear();
    }

    return v;
}

void StreamFile::putRaw( const char * ptr, size_t sz )
{
    if ( _file )
        std::fwrite( ptr, sz, 1, _file );
}

StreamBuf StreamFile::toStreamBuf( size_t sz )
{
    std::vector<uint8_t> buf = getRaw( sz );
    StreamBuf sb( buf.size() );
    sb.putRaw( reinterpret_cast<const char *>( &buf[0] ), buf.size() );
    return sb;
}

std::string StreamFile::toString( size_t sz )
{
    const std::vector<uint8_t> buf = getRaw( sz );
    std::vector<uint8_t>::const_iterator itend = std::find( buf.begin(), buf.end(), 0 );
    return std::string( buf.begin(), itend != buf.end() ? itend : buf.end() );
}
