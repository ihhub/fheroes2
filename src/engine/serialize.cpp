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
#include <cstring>
#include <ostream>
#include <string>
#include <string_view>

#include "logging.h"

namespace
{
    const size_t minBufferCapacity = 1024;
}

void StreamBase::setBigendian( bool f )
{
    if ( f ) {
        _flags |= BIGENDIAN;
    }
    else {
        _flags &= ~BIGENDIAN;
    }
}

void StreamBase::setFail( bool f )
{
    if ( f ) {
        _flags |= FAILURE;
    }
    else {
        _flags &= ~FAILURE;
    }
}

uint16_t IStreamBase::get16()
{
    return bigendian() ? getBE16() : getLE16();
}

uint32_t IStreamBase::get32()
{
    return bigendian() ? getBE32() : getLE32();
}

IStreamBase & IStreamBase::operator>>( bool & v )
{
    v = ( get8() != 0 );

    return *this;
}

IStreamBase & IStreamBase::operator>>( char & v )
{
    v = get8();

    return *this;
}

IStreamBase & IStreamBase::operator>>( int8_t & v )
{
    v = static_cast<int8_t>( get8() );

    return *this;
}

IStreamBase & IStreamBase::operator>>( uint8_t & v )
{
    v = get8();

    return *this;
}

IStreamBase & IStreamBase::operator>>( int16_t & v )
{
    v = get16();

    return *this;
}

IStreamBase & IStreamBase::operator>>( uint16_t & v )
{
    v = get16();

    return *this;
}

IStreamBase & IStreamBase::operator>>( int32_t & v )
{
    v = get32();

    return *this;
}

IStreamBase & IStreamBase::operator>>( uint32_t & v )
{
    v = get32();

    return *this;
}

IStreamBase & IStreamBase::operator>>( std::string & v )
{
    v.resize( get32() );

    std::for_each( v.begin(), v.end(), [this]( char & item ) { item = get8(); } );

    return *this;
}

IStreamBase & IStreamBase::operator>>( fheroes2::Point & v )
{
    return *this >> v.x >> v.y;
}

void OStreamBase::put16( uint16_t v )
{
    bigendian() ? putBE16( v ) : putLE16( v );
}

void OStreamBase::put32( uint32_t v )
{
    bigendian() ? putBE32( v ) : putLE32( v );
}

OStreamBase & OStreamBase::operator<<( const bool v )
{
    put8( v );

    return *this;
}

OStreamBase & OStreamBase::operator<<( const char v )
{
    put8( v );

    return *this;
}

OStreamBase & OStreamBase::operator<<( const int8_t v )
{
    put8( static_cast<uint8_t>( v ) );

    return *this;
}

OStreamBase & OStreamBase::operator<<( const uint8_t v )
{
    put8( v );

    return *this;
}

OStreamBase & OStreamBase::operator<<( const int16_t v )
{
    put16( v );

    return *this;
}

OStreamBase & OStreamBase::operator<<( const uint16_t v )
{
    put16( v );

    return *this;
}

OStreamBase & OStreamBase::operator<<( const int32_t v )
{
    put32( v );

    return *this;
}

OStreamBase & OStreamBase::operator<<( const uint32_t v )
{
    put32( v );

    return *this;
}

OStreamBase & OStreamBase::operator<<( const std::string_view v )
{
    put32( static_cast<uint32_t>( v.size() ) );

    // A string is a container of bytes so it doesn't matter which endianess is being used.
    putRaw( v.data(), v.size() );

    return *this;
}

OStreamBase & OStreamBase::operator<<( const fheroes2::Point & v )
{
    return *this << v.x << v.y;
}

RWStreamBuf::RWStreamBuf( const size_t size )
{
    if ( size ) {
        reallocBuf( size );
    }

    setBigendian( IS_BIGENDIAN );
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

void RWStreamBuf::putRaw( const void * ptr, size_t size )
{
    if ( size == 0 ) {
        return;
    }

    if ( sizep() < size ) {
        if ( size < capacity() / 2 ) {
            reallocBuf( capacity() + capacity() / 2 );
        }
        else {
            reallocBuf( capacity() + size );
        }
    }

    if ( sizep() < size ) {
        assert( 0 );
        return;
    }

    memcpy( _itput, ptr, size );

    _itput = _itput + size;
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

size_t RWStreamBuf::tellp() const
{
    assert( _itbeg <= _itput );

    return _itput - _itbeg;
}

size_t RWStreamBuf::sizep() const
{
    assert( _itput <= _itend );

    return _itend - _itput;
}

void RWStreamBuf::reallocBuf( size_t size )
{
    if ( !_buf ) {
        assert( ( []( const auto... args ) { return ( ( args == nullptr ) && ... ); }( _itbeg, _itget, _itput, _itend ) ) );

        size = std::max( size, minBufferCapacity );

        _buf = std::make_unique<uint8_t[]>( size );

        _itbeg = _buf.get();
        _itend = _itbeg + size;

        _itput = _itbeg;
        _itget = _itbeg;

        return;
    }

    assert( ( []( const auto... args ) { return ( ( args != nullptr ) && ... ); }( _itbeg, _itget, _itput, _itend ) ) && _itbeg <= _itget && _itget <= _itput
            && _itput <= _itend );

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

    setBigendian( IS_BIGENDIAN );
}

ROStreamBuf::ROStreamBuf( std::vector<uint8_t> && buf )
    : _buf( std::move( buf ) )
{
    _itbeg = _buf.data();
    _itend = _itbeg + _buf.size();
    _itget = _itbeg;
    _itput = _itend;

    setBigendian( IS_BIGENDIAN );
}

std::pair<const uint8_t *, size_t> ROStreamBuf::getRawView( const size_t size /* = 0 */ )
{
    const size_t remainSize = sizeg();
    const size_t resultSize = size > 0 ? std::min( size, remainSize ) : remainSize;

    auto v = std::make_pair( _itget, resultSize );

    _itget += resultSize;

    return v;
}

std::string_view ROStreamBuf::getStringView( const size_t size /* = 0 */ )
{
    const size_t remainSize = sizeg();
    const size_t sizeToSkip = size > 0 ? std::min( size, remainSize ) : remainSize;

    const uint8_t * strBeg = _itget;
    _itget += sizeToSkip;

    const uint8_t * strEnd = std::find( strBeg, _itget, 0 );
    assert( strBeg <= strEnd );

    static_assert( std::is_same_v<std::remove_const_t<std::remove_reference_t<decltype( *strBeg )>>, unsigned char> );

    return { reinterpret_cast<const char *>( strBeg ), static_cast<size_t>( strEnd - strBeg ) };
}

bool StreamFile::open( const std::string & fn, const std::string & mode )
{
    _file.reset( std::fopen( fn.c_str(), mode.c_str() ) );
    if ( !_file ) {
        ERROR_LOG( "Error opening file " << fn )
    }

    setFail( !_file );

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
        setFail();

        return 0;
    }

    if ( std::fseek( _file.get(), 0, SEEK_END ) != 0 ) {
        setFail();

        return 0;
    }

    const long len = std::ftell( _file.get() );
    if ( len < 0 ) {
        setFail();

        return 0;
    }

    if ( std::fseek( _file.get(), pos, SEEK_SET ) != 0 ) {
        setFail();

        return 0;
    }

    return static_cast<size_t>( len );
}

size_t StreamFile::tell()
{
    if ( !_file ) {
        return 0;
    }

    const long pos = std::ftell( _file.get() );
    if ( pos < 0 ) {
        setFail();

        return 0;
    }

    return static_cast<size_t>( pos );
}

void StreamFile::seek( const size_t pos )
{
    if ( !_file ) {
        return;
    }

    if ( std::fseek( _file.get(), static_cast<long>( pos ), SEEK_SET ) != 0 ) {
        setFail();
    }
}

size_t StreamFile::sizeg()
{
    if ( !_file ) {
        return 0;
    }

    const long pos = std::ftell( _file.get() );
    if ( pos < 0 ) {
        setFail();

        return 0;
    }

    if ( std::fseek( _file.get(), 0, SEEK_END ) != 0 ) {
        setFail();

        return 0;
    }

    const long len = std::ftell( _file.get() );
    if ( len < 0 ) {
        setFail();

        return 0;
    }

    if ( std::fseek( _file.get(), pos, SEEK_SET ) != 0 ) {
        setFail();

        return 0;
    }

    // Something weird has happened
    if ( len < pos ) {
        setFail();

        return 0;
    }

    return static_cast<size_t>( len - pos );
}

void StreamFile::skip( size_t size )
{
    if ( !_file ) {
        return;
    }

    if ( std::fseek( _file.get(), static_cast<long int>( size ), SEEK_CUR ) != 0 ) {
        setFail();
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

void StreamFile::putBE16( uint16_t v )
{
    putUint<uint16_t>( htobe16( v ) );
}

void StreamFile::putLE16( uint16_t v )
{
    putUint<uint16_t>( htole16( v ) );
}

void StreamFile::putBE32( uint32_t v )
{
    putUint<uint32_t>( htobe32( v ) );
}

void StreamFile::putLE32( uint32_t v )
{
    putUint<uint32_t>( htole32( v ) );
}

std::vector<uint8_t> StreamFile::getRaw( const size_t size )
{
    const size_t chunkSize = size > 0 ? size : sizeg();
    if ( chunkSize == 0 || !_file ) {
        return {};
    }

    std::vector<uint8_t> v( chunkSize, 0 );

    if ( std::fread( v.data(), chunkSize, 1, _file.get() ) != 1 ) {
        setFail();

        return {};
    }

    return v;
}

void StreamFile::putRaw( const void * ptr, size_t size )
{
    if ( size == 0 ) {
        // Nothing to write. Ignore it.
        return;
    }

    if ( !_file ) {
        return;
    }

    if ( std::fwrite( ptr, size, 1, _file.get() ) != 1 ) {
        setFail();
    }
}

ROStreamBuf StreamFile::getStreamBuf( const size_t size /* = 0 */ )
{
    return ROStreamBuf{ getRaw( size ) };
}

std::string StreamFile::getString( const size_t size /* = 0 */ )
{
    const std::vector<uint8_t> buf = getRaw( size );

    return { buf.begin(), std::find( buf.begin(), buf.end(), 0 ) };
}
