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
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#if defined( __FreeBSD__ ) || defined( __OpenBSD__ )
#include <sys/endian.h>

#elif defined( _WIN32 )
#include <cstdlib>

#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#define BYTE_ORDER LITTLE_ENDIAN

#define htobe16( x ) _byteswap_ushort( x )
#define htole16( x ) ( x )
#define be16toh( x ) _byteswap_ushort( x )
#define le16toh( x ) ( x )
#define htobe32( x ) _byteswap_ulong( x )
#define htole32( x ) ( x )
#define be32toh( x ) _byteswap_ulong( x )
#define le32toh( x ) ( x )

#elif defined( __APPLE__ )
#include <libkern/OSByteOrder.h>
#define htobe16( x ) OSSwapHostToBigInt16( x )
#define htole16( x ) OSSwapHostToLittleInt16( x )
#define be16toh( x ) OSSwapBigToHostInt16( x )
#define le16toh( x ) OSSwapLittleToHostInt16( x )
#define htobe32( x ) OSSwapHostToBigInt32( x )
#define htole32( x ) OSSwapHostToLittleInt32( x )
#define be32toh( x ) OSSwapBigToHostInt32( x )
#define le32toh( x ) OSSwapLittleToHostInt32( x )

#elif defined( TARGET_PS_VITA )
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#define BYTE_ORDER LITTLE_ENDIAN

#define htobe16( x ) __builtin_bswap16( x )
#define htole16( x ) ( x )
#define be16toh( x ) __builtin_bswap16( x )
#define le16toh( x ) ( x )
#define htobe32( x ) __builtin_bswap32( x )
#define htole32( x ) ( x )
#define be32toh( x ) __builtin_bswap32( x )
#define le32toh( x ) ( x )

#elif defined( TARGET_NINTENDO_SWITCH )
#include <machine/endian.h>
#define LITTLE_ENDIAN _LITTLE_ENDIAN
#define BIG_ENDIAN _BIG_ENDIAN
#define BYTE_ORDER _BYTE_ORDER
#define htobe16( x ) __bswap16( x )
#define htole16( x ) ( x )
#define be16toh( x ) __bswap16( x )
#define le16toh( x ) ( x )
#define htobe32( x ) __bswap32( x )
#define htole32( x ) ( x )
#define be32toh( x ) __bswap32( x )
#define le32toh( x ) ( x )

#else
// POSIX 1003.1-2024
// https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/endian.h.html
#include <endian.h>

#endif

#include "math_base.h"

#define IS_BIGENDIAN ( BYTE_ORDER == BIG_ENDIAN )

// Base class for all I/O facilities
class StreamBase
{
public:
    StreamBase( const StreamBase & ) = delete;

    virtual ~StreamBase() = default;

    StreamBase & operator=( const StreamBase & ) = delete;

    void setBigendian( bool f );

    bool fail() const
    {
        return ( _flags & FAILURE ) != 0;
    }

    bool bigendian() const
    {
        return ( _flags & BIGENDIAN ) != 0;
    }

protected:
    StreamBase() = default;

    StreamBase( StreamBase && stream ) noexcept;

    StreamBase & operator=( StreamBase && stream ) noexcept;

    void setFail( bool f );

private:
    enum : uint32_t
    {
        FAILURE = 0x00000001,
        BIGENDIAN = 0x00000002
    };

    uint32_t _flags{ 0 };
};

// Interface that declares the methods needed to read from a stream
class IStreamBase : virtual public StreamBase
{
public:
    IStreamBase( const IStreamBase & ) = delete;

    ~IStreamBase() override = default;

    IStreamBase & operator=( const IStreamBase & ) = delete;

    virtual void skip( size_t ) = 0;

    virtual uint16_t getBE16() = 0;
    virtual uint16_t getLE16() = 0;
    virtual uint32_t getBE32() = 0;
    virtual uint32_t getLE32() = 0;

    // 0 stands for all data.
    virtual std::vector<uint8_t> getRaw( size_t = 0 ) = 0;

    uint16_t get16();
    uint32_t get32();

    uint8_t get()
    {
        return get8();
    }

    IStreamBase & operator>>( bool & v );
    IStreamBase & operator>>( char & v );
    IStreamBase & operator>>( int8_t & v );
    IStreamBase & operator>>( uint8_t & v );
    IStreamBase & operator>>( int16_t & v );
    IStreamBase & operator>>( uint16_t & v );
    IStreamBase & operator>>( int32_t & v );
    IStreamBase & operator>>( uint32_t & v );
    IStreamBase & operator>>( std::string & v );

    IStreamBase & operator>>( fheroes2::Point & v );

    template <class Type1, class Type2>
    IStreamBase & operator>>( std::pair<Type1, Type2> & v )
    {
        return *this >> v.first >> v.second;
    }

    template <class Type>
    IStreamBase & operator>>( std::vector<Type> & v )
    {
        v.resize( get32() );

        std::for_each( v.begin(), v.end(), [this]( auto & item ) { *this >> item; } );

        return *this;
    }

    template <class Type>
    IStreamBase & operator>>( std::list<Type> & v )
    {
        v.resize( get32() );

        std::for_each( v.begin(), v.end(), [this]( auto & item ) { *this >> item; } );

        return *this;
    }

    template <class Type1, class Type2>
    IStreamBase & operator>>( std::map<Type1, Type2> & v )
    {
        const uint32_t size = get32();

        v.clear();

        for ( uint32_t i = 0; i < size; ++i ) {
            std::pair<Type1, Type2> pr;

            *this >> pr;

            v.emplace( std::move( pr ) );
        }

        return *this;
    }

    template <class Type, size_t Count>
    IStreamBase & operator>>( std::array<Type, Count> & v )
    {
        const uint32_t size = get32();
        if ( size != v.size() ) {
            setFail( true );

            v = {};

            return *this;
        }

        std::for_each( v.begin(), v.end(), [this]( auto & item ) { *this >> item; } );

        return *this;
    }

protected:
    IStreamBase() = default;

    IStreamBase( IStreamBase && ) = default;

    // All this operator does is call the corresponding base class operator. It is not recommended to
    // declare these operators as default in the case of a virtual base, even if they are trivial, to
    // catch a situation where, in the case of the diamond inheritance, the corresponding operator of
    // the base class will be called multiple times. GCC has a special warning for this case (see the
    // description of the -Wno-virtual-move-assign switch).
    IStreamBase & operator=( IStreamBase && stream ) noexcept;

    virtual uint8_t get8() = 0;

    virtual size_t sizeg() = 0;
    virtual size_t tellg() = 0;
};

// Interface that declares the methods needed to write to a stream
class OStreamBase : virtual public StreamBase
{
public:
    OStreamBase( const OStreamBase & ) = delete;

    ~OStreamBase() override = default;

    OStreamBase & operator=( const OStreamBase & ) = delete;

    virtual void putBE16( uint16_t ) = 0;
    virtual void putLE16( uint16_t ) = 0;
    virtual void putBE32( uint32_t ) = 0;
    virtual void putLE32( uint32_t ) = 0;

    virtual void putRaw( const void *, size_t ) = 0;

    void put16( uint16_t );
    void put32( uint32_t );

    void put( const uint8_t ch )
    {
        put8( ch );
    }

    OStreamBase & operator<<( const bool v );
    OStreamBase & operator<<( const char v );
    OStreamBase & operator<<( const int8_t v );
    OStreamBase & operator<<( const uint8_t v );
    OStreamBase & operator<<( const int16_t v );
    OStreamBase & operator<<( const uint16_t v );
    OStreamBase & operator<<( const int32_t v );
    OStreamBase & operator<<( const uint32_t v );
    OStreamBase & operator<<( const std::string_view v );

    OStreamBase & operator<<( const fheroes2::Point & v );

    template <class Type1, class Type2>
    OStreamBase & operator<<( const std::pair<Type1, Type2> & v )
    {
        return *this << v.first << v.second;
    }

    template <class Type>
    OStreamBase & operator<<( const std::vector<Type> & v )
    {
        put32( static_cast<uint32_t>( v.size() ) );

        std::for_each( v.begin(), v.end(), [this]( const auto & item ) { *this << item; } );

        return *this;
    }

    template <class Type>
    OStreamBase & operator<<( const std::list<Type> & v )
    {
        put32( static_cast<uint32_t>( v.size() ) );

        std::for_each( v.begin(), v.end(), [this]( const auto & item ) { *this << item; } );

        return *this;
    }

    template <class Type1, class Type2>
    OStreamBase & operator<<( const std::map<Type1, Type2> & v )
    {
        put32( static_cast<uint32_t>( v.size() ) );

        std::for_each( v.begin(), v.end(), [this]( const auto & item ) { *this << item; } );

        return *this;
    }

    template <class Type, size_t Count>
    OStreamBase & operator<<( const std::array<Type, Count> & v )
    {
        put32( static_cast<uint32_t>( v.size() ) );

        std::for_each( v.begin(), v.end(), [this]( const auto & item ) { *this << item; } );

        return *this;
    }

protected:
    OStreamBase() = default;

    OStreamBase( OStreamBase && ) = default;

    virtual void put8( const uint8_t ) = 0;

    virtual size_t sizep() = 0;
    virtual size_t tellp() = 0;
};

// Interface that declares a stream with an in-memory storage backend that can be read from
class IStreamBuf : public IStreamBase
{
public:
    virtual const uint8_t * data() const = 0;
    virtual size_t size() = 0;

protected:
    IStreamBuf() = default;
};

// Class template for a stream with an in-memory storage backend that can store either const or non-const data as desired
template <typename T, typename = typename std::enable_if_t<std::is_same_v<T, uint8_t> || std::is_same_v<T, const uint8_t>>>
class StreamBufTmpl : public IStreamBuf
{
public:
    StreamBufTmpl( const StreamBufTmpl & ) = delete;

    ~StreamBufTmpl() override = default;

    StreamBufTmpl & operator=( const StreamBufTmpl & ) = delete;

    const uint8_t * data() const override
    {
        return _itget;
    }

    size_t size() override
    {
        return sizeg();
    }

    size_t tell()
    {
        return tellg();
    }

    void seek( size_t sz )
    {
        _itget = ( _itbeg + sz < _itend ? _itbeg + sz : _itend );
    }

    void skip( size_t sz ) override
    {
        _itget += ( sz <= sizeg() ? sz : sizeg() );
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

        std::copy( _itget, _itget + sizeToCopy, result.data() );

        _itget += sizeToCopy;

        return result;
    }

    // 0 stands for all data.
    std::string toString( const size_t sz = 0 )
    {
        const size_t length = ( sz > 0 && sz < sizeg() ) ? sz : sizeg();

        T * strBeg = _itget;
        _itget += length;

        return { strBeg, std::find( strBeg, _itget, 0 ) };
    }

protected:
    StreamBufTmpl() = default;

    StreamBufTmpl( StreamBufTmpl && stream ) noexcept
        : IStreamBuf( std::move( stream ) )
    {
        std::swap( _itbeg, stream._itbeg );
        std::swap( _itget, stream._itget );
        std::swap( _itput, stream._itput );
        std::swap( _itend, stream._itend );
    }

    StreamBufTmpl & operator=( StreamBufTmpl && stream ) noexcept
    {
        if ( this == &stream ) {
            return *this;
        }

        IStreamBuf::operator=( std::move( stream ) );

        std::swap( _itbeg, stream._itbeg );
        std::swap( _itget, stream._itget );
        std::swap( _itput, stream._itput );
        std::swap( _itend, stream._itend );

        return *this;
    }

    size_t tellg() override
    {
        return _itget - _itbeg;
    }

    size_t sizeg() override
    {
        return _itput - _itget;
    }

    uint8_t get8() override
    {
        if ( sizeg() > 0 ) {
            return *( _itget++ );
        }

        setFail( true );

        return 0;
    }

    size_t capacity() const
    {
        return _itend - _itbeg;
    }

    T * _itbeg{ nullptr };
    T * _itget{ nullptr };
    T * _itput{ nullptr };
    T * _itend{ nullptr };
};

// Stream with a dynamically-sized in-memory storage backend that supports both reading and writing
class RWStreamBuf final : public StreamBufTmpl<uint8_t>, public OStreamBase
{
public:
    RWStreamBuf()
        : RWStreamBuf( 0 )
    {}

    explicit RWStreamBuf( const size_t sz );

    RWStreamBuf( const RWStreamBuf & ) = delete;
    RWStreamBuf( RWStreamBuf && ) = default;

    ~RWStreamBuf() override = default;

    RWStreamBuf & operator=( const RWStreamBuf & ) = delete;
    RWStreamBuf & operator=( RWStreamBuf && stream ) noexcept;

    void putBE32( uint32_t v ) override;
    void putLE32( uint32_t v ) override;
    void putBE16( uint16_t v ) override;
    void putLE16( uint16_t v ) override;

    void putRaw( const void * ptr, size_t sz ) override;

private:
    friend class StreamFile;

    void put8( const uint8_t v ) override;

    size_t sizep() override;
    size_t tellp() override;

    void reallocBuf( size_t size );

    // After using this method to write data, update the cursor by calling the advance() method.
    uint8_t * rwData()
    {
        return _itput;
    }

    // Advances the cursor intended for writing data forward by a specified number of bytes.
    void advance( const size_t size )
    {
        _itput += size;
    }

    std::unique_ptr<uint8_t[]> _buf;
};

// Stream with read-only in-memory storage backed by a const vector instance
class ROStreamBuf final : public StreamBufTmpl<const uint8_t>
{
public:
    explicit ROStreamBuf( const std::vector<uint8_t> & buf );

    ROStreamBuf( const ROStreamBuf & ) = delete;

    ~ROStreamBuf() override = default;

    ROStreamBuf & operator=( const ROStreamBuf & ) = delete;
};

// Stream with a file storage backend that supports both reading and writing
class StreamFile final : public IStreamBase, public OStreamBase
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
            setFail( true );

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
            setFail( true );
        }
    }

    std::unique_ptr<std::FILE, int ( * )( std::FILE * )> _file{ nullptr, []( std::FILE * f ) { return std::fclose( f ); } };
};

namespace fheroes2
{
    // Get a value of type T in the system byte order from the buffer in which it was originally stored in the little-endian byte order
    template <typename T, typename = typename std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
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
