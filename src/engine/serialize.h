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

#ifndef H2SERIALIZE_H
#define H2SERIALIZE_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "endian_h2.h"
#include "math_base.h"

class StreamBase
{
public:
    StreamBase() = default;

    StreamBase( const StreamBase & ) = delete;

    StreamBase( StreamBase && stream ) noexcept
    {
        std::swap( _flags, stream._flags );
    }

    virtual ~StreamBase() = default;

    StreamBase & operator=( const StreamBase & ) = delete;

    StreamBase & operator=( StreamBase && stream ) noexcept
    {
        if ( this == &stream ) {
            return *this;
        }

        std::swap( _flags, stream._flags );

        return *this;
    }

    void setbigendian( bool f );

    bool fail() const
    {
        return ( _flags & FAILURE ) != 0;
    }

    bool bigendian() const
    {
        return ( _flags & BIGENDIAN ) != 0;
    }

    virtual void skip( size_t ) = 0;

    virtual uint16_t getBE16() = 0;
    virtual uint16_t getLE16() = 0;
    virtual uint32_t getBE32() = 0;
    virtual uint32_t getLE32() = 0;

    virtual void putBE32( uint32_t ) = 0;
    virtual void putLE32( uint32_t ) = 0;
    virtual void putBE16( uint16_t ) = 0;
    virtual void putLE16( uint16_t ) = 0;

    // 0 stands for all data.
    virtual std::vector<uint8_t> getRaw( size_t = 0 ) = 0;
    virtual void putRaw( const void *, size_t ) = 0;

    uint16_t get16();
    uint32_t get32();

    void put16( uint16_t );
    void put32( uint32_t );

    uint8_t get()
    {
        return get8();
    }

    void put( const uint8_t ch )
    {
        put8( ch );
    }

    StreamBase & operator>>( bool & v );
    StreamBase & operator>>( char & v );
    StreamBase & operator>>( int8_t & v );
    StreamBase & operator>>( uint8_t & v );
    StreamBase & operator>>( uint16_t & v );
    StreamBase & operator>>( int16_t & v );
    StreamBase & operator>>( uint32_t & v );
    StreamBase & operator>>( int32_t & v );
    StreamBase & operator>>( std::string & v );

    StreamBase & operator>>( fheroes2::Point & point_ );

    StreamBase & operator<<( const bool v );
    StreamBase & operator<<( const char v );
    StreamBase & operator<<( const int8_t v );
    StreamBase & operator<<( const uint8_t v );
    StreamBase & operator<<( const uint16_t v );
    StreamBase & operator<<( const int16_t v );
    StreamBase & operator<<( const uint32_t v );
    StreamBase & operator<<( const int32_t v );
    StreamBase & operator<<( const std::string & v );

    StreamBase & operator<<( const fheroes2::Point & point_ );

    template <class Type1, class Type2>
    StreamBase & operator>>( std::pair<Type1, Type2> & p )
    {
        return *this >> p.first >> p.second;
    }

    template <class Type>
    StreamBase & operator>>( std::vector<Type> & v )
    {
        const uint32_t size = get32();
        v.resize( size );
        for ( typename std::vector<Type>::iterator it = v.begin(); it != v.end(); ++it )
            *this >> *it;
        return *this;
    }

    template <class Type>
    StreamBase & operator>>( std::list<Type> & v )
    {
        const uint32_t size = get32();
        v.resize( size );
        for ( typename std::list<Type>::iterator it = v.begin(); it != v.end(); ++it )
            *this >> *it;
        return *this;
    }

    template <class Type1, class Type2>
    StreamBase & operator>>( std::map<Type1, Type2> & v )
    {
        const uint32_t size = get32();
        v.clear();
        for ( uint32_t ii = 0; ii < size; ++ii ) {
            std::pair<Type1, Type2> pr;
            *this >> pr;
            v.emplace( std::move( pr ) );
        }
        return *this;
    }

    template <class Type, size_t Count>
    StreamBase & operator>>( std::array<Type, Count> & data )
    {
        const uint32_t size = get32();
        if ( size != data.size() ) {
            // This is a corrupted file!
            assert( 0 );
            data = {};

            return *this;
        }

        for ( auto & value : data ) {
            *this >> value;
        }

        return *this;
    }

    template <class Type1, class Type2>
    StreamBase & operator<<( const std::pair<Type1, Type2> & p )
    {
        return *this << p.first << p.second;
    }

    template <class Type>
    StreamBase & operator<<( const std::vector<Type> & v )
    {
        put32( static_cast<uint32_t>( v.size() ) );
        for ( typename std::vector<Type>::const_iterator it = v.begin(); it != v.end(); ++it )
            *this << *it;
        return *this;
    }

    template <class Type>
    StreamBase & operator<<( const std::list<Type> & v )
    {
        put32( static_cast<uint32_t>( v.size() ) );
        for ( typename std::list<Type>::const_iterator it = v.begin(); it != v.end(); ++it )
            *this << *it;
        return *this;
    }

    template <class Type1, class Type2>
    StreamBase & operator<<( const std::map<Type1, Type2> & v )
    {
        put32( static_cast<uint32_t>( v.size() ) );
        for ( typename std::map<Type1, Type2>::const_iterator it = v.begin(); it != v.end(); ++it )
            *this << *it;
        return *this;
    }

    template <class Type, size_t Count>
    StreamBase & operator<<( const std::array<Type, Count> & data )
    {
        put32( static_cast<uint32_t>( data.size() ) );
        for ( const auto & value : data ) {
            *this << value;
        }
        return *this;
    }

protected:
    virtual uint8_t get8() = 0;
    virtual void put8( const uint8_t ) = 0;

    virtual size_t sizeg() = 0;
    virtual size_t sizep() = 0;
    virtual size_t tellg() = 0;
    virtual size_t tellp() = 0;

    void setfail( bool f );

private:
    enum : uint32_t
    {
        FAILURE = 0x00000001,
        BIGENDIAN = 0x00000002
    };

    uint32_t _flags{ 0 };
};

class StreamBuf : public StreamBase
{
public:
    virtual const uint8_t * data() const = 0;
    virtual size_t size() = 0;

protected:
    StreamBuf() = default;
};

template <typename T, typename = typename std::enable_if_t<std::is_same_v<T, uint8_t> || std::is_same_v<T, const uint8_t>>>
class StreamBufTmpl : public StreamBuf
{
public:
    StreamBufTmpl( const StreamBufTmpl & ) = delete;

    ~StreamBufTmpl() override = default;

    StreamBufTmpl & operator=( const StreamBufTmpl & ) = delete;

    const uint8_t * data() const override
    {
        return itget;
    }

    size_t size() override
    {
        return sizeg();
    }

    size_t capacity() const
    {
        return itend - itbeg;
    }

    void seek( size_t sz )
    {
        itget = ( itbeg + sz < itend ? itbeg + sz : itend );
    }

    void skip( size_t sz ) override
    {
        itget += ( sz <= sizeg() ? sz : sizeg() );
    }

    uint16_t getBE16() override
    {
        uint16_t result = ( static_cast<uint16_t>( get8() ) << 8 );

        result |= get8();

        return result;
    }

    uint16_t getLE16() override
    {
        uint16_t result = get8();

        result |= ( static_cast<uint16_t>( get8() ) << 8 );

        return result;
    }

    uint32_t getBE32() override
    {
        uint32_t result = ( static_cast<uint32_t>( get8() ) << 24 );

        result |= ( static_cast<uint32_t>( get8() ) << 16 );
        result |= ( static_cast<uint32_t>( get8() ) << 8 );
        result |= get8();

        return result;
    }

    uint32_t getLE32() override
    {
        uint32_t result = get8();

        result |= ( static_cast<uint32_t>( get8() ) << 8 );
        result |= ( static_cast<uint32_t>( get8() ) << 16 );
        result |= ( static_cast<uint32_t>( get8() ) << 24 );

        return result;
    }

    // 0 stands for all data.
    std::vector<uint8_t> getRaw( size_t sz = 0 ) override
    {
        const size_t actualSize = sizeg();
        const size_t resultSize = sz > 0 ? sz : actualSize;
        const size_t sizeToCopy = std::min( resultSize, actualSize );

        std::vector<uint8_t> result( resultSize, 0 );
        memcpy( result.data(), itget, sizeToCopy );

        itget += sizeToCopy;

        return result;
    }

    // 0 stands for all data.
    std::string toString( const size_t sz = 0 )
    {
        const size_t length = ( sz > 0 && sz < sizeg() ) ? sz : sizeg();

        T * strBeg = itget;
        itget += length;

        return { strBeg, std::find( strBeg, itget, 0 ) };
    }

protected:
    StreamBufTmpl() = default;

    StreamBufTmpl( StreamBufTmpl && stream ) noexcept
        : StreamBuf( std::move( stream ) )
    {
        std::swap( itbeg, stream.itbeg );
        std::swap( itget, stream.itget );
        std::swap( itput, stream.itput );
        std::swap( itend, stream.itend );
    }

    StreamBufTmpl & operator=( StreamBufTmpl && stream ) noexcept
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

    size_t tellg() override
    {
        return itget - itbeg;
    }

    size_t tellp() override
    {
        return itput - itbeg;
    }

    size_t sizeg() override
    {
        return itput - itget;
    }

    size_t sizep() override
    {
        return itend - itput;
    }

    uint8_t get8() override
    {
        if ( sizeg() > 0 ) {
            return *( itget++ );
        }

        return 0;
    }

    T * itbeg{ nullptr };
    T * itget{ nullptr };
    T * itput{ nullptr };
    T * itend{ nullptr };
};

class RWStreamBuf final : public StreamBufTmpl<uint8_t>
{
public:
    explicit RWStreamBuf( const size_t sz = 0 );

    RWStreamBuf( const RWStreamBuf & ) = delete;
    RWStreamBuf( RWStreamBuf && stream ) = default;

    ~RWStreamBuf() override;

    RWStreamBuf & operator=( const RWStreamBuf & ) = delete;
    RWStreamBuf & operator=( RWStreamBuf && stream ) = default;

    void putBE32( uint32_t v ) override;
    void putLE32( uint32_t v ) override;
    void putBE16( uint16_t v ) override;
    void putLE16( uint16_t v ) override;

    void putRaw( const void * ptr, size_t sz ) override;

private:
    friend class StreamFile;

    void put8( const uint8_t v ) override;

    void reallocBuf( size_t size );

    // After using this method to write data, update the cursor by calling the advance() method.
    uint8_t * rwData()
    {
        return itput;
    }

    // Advances the cursor intended for writing data forward by a specified number of bytes.
    void advance( const size_t size )
    {
        itput += size;
    }
};

class ROStreamBuf final : public StreamBufTmpl<const uint8_t>
{
public:
    explicit ROStreamBuf( const std::vector<uint8_t> & buf );

    ROStreamBuf( const ROStreamBuf & ) = delete;
    ROStreamBuf( ROStreamBuf && stream ) = default;

    ~ROStreamBuf() override = default;

    ROStreamBuf & operator=( const ROStreamBuf & ) = delete;
    ROStreamBuf & operator=( ROStreamBuf && stream ) = default;

    void putBE32( uint32_t v ) override;
    void putLE32( uint32_t v ) override;
    void putBE16( uint16_t v ) override;
    void putLE16( uint16_t v ) override;

    void putRaw( const void * ptr, size_t sz ) override;

private:
    void put8( const uint8_t v ) override;
};

class StreamFile final : public StreamBase
{
public:
    StreamFile() = default;

    StreamFile( const StreamFile & ) = delete;

    ~StreamFile() override = default;

    StreamFile & operator=( const StreamFile & ) = delete;

    size_t size();
    size_t tell();

    bool open( const std::string & fn, const std::string & mode );
    void close();

    // 0 stands for all data.
    RWStreamBuf toStreamBuf( const size_t size = 0 );

    void seek( size_t );
    void skip( size_t ) override;

    uint16_t getBE16() override;
    uint16_t getLE16() override;
    uint32_t getBE32() override;
    uint32_t getLE32() override;

    void putBE16( uint16_t ) override;
    void putLE16( uint16_t ) override;
    void putBE32( uint32_t ) override;
    void putLE32( uint32_t ) override;

    // 0 stands for all data.
    std::vector<uint8_t> getRaw( const size_t size = 0 ) override;

    void putRaw( const void * ptr, size_t sz ) override;

    // 0 stands for all data.
    std::string toString( const size_t size = 0 );

private:
    size_t sizeg() override;
    size_t sizep() override;
    size_t tellg() override;
    size_t tellp() override;

    uint8_t get8() override;
    void put8( const uint8_t v ) override;

    template <typename T>
    T getUint()
    {
        if ( !_file ) {
            return 0;
        }

        T val;

        if ( std::fread( &val, sizeof( T ), 1, _file.get() ) != 1 ) {
            setfail( true );

            return 0;
        }

        return val;
    }

    template <typename T>
    void putUint( const T val )
    {
        if ( !_file ) {
            return;
        }

        if ( std::fwrite( &val, sizeof( T ), 1, _file.get() ) != 1 ) {
            setfail( true );
        }
    }

    std::unique_ptr<std::FILE, int ( * )( std::FILE * )> _file{ nullptr, std::fclose };
};

namespace fheroes2
{
    // Get a value of type T in the system byte order from the buffer in which it was originally stored in the little-endian byte order
    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value>::type>
    T getLEValue( const char * data, const size_t base, const size_t offset = 0 )
    {
        const char * begin = data + base + offset * sizeof( T );
        const char * end = begin + sizeof( T );

        T result;

#if defined( BYTE_ORDER ) && defined( LITTLE_ENDIAN ) && BYTE_ORDER == LITTLE_ENDIAN
        std::copy( begin, end, reinterpret_cast<char *>( &result ) );
#elif defined( BYTE_ORDER ) && defined( BIG_ENDIAN ) && BYTE_ORDER == BIG_ENDIAN
        std::reverse_copy( begin, end, reinterpret_cast<char *>( &result ) );
#else
#error "Unknown byte order"
#endif

        return result;
    }
}

#endif
