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

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
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

    void setFail()
    {
        setFail( true );
    }

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

    // If a zero size is specified, then all still unread data is returned
    virtual std::vector<uint8_t> getRaw( size_t ) = 0;

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

    template <typename Type, std::enable_if_t<std::is_enum_v<Type>, bool> = true>
    IStreamBase & operator>>( Type & v )
    {
        std::underlying_type_t<Type> temp{};
        *this >> temp;

        v = static_cast<Type>( temp );

        return *this;
    }

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
            setFail();

            v = {};

            return *this;
        }

        std::for_each( v.begin(), v.end(), [this]( auto & item ) { *this >> item; } );

        return *this;
    }

protected:
    IStreamBase() = default;

    virtual uint8_t get8() = 0;
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

    template <typename Type, std::enable_if_t<std::is_enum_v<Type>, bool> = true>
    OStreamBase & operator<<( const Type v )
    {
        return *this << static_cast<std::underlying_type_t<Type>>( v );
    }

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

    virtual void put8( const uint8_t ) = 0;
};

// Interface that declares a stream with an in-memory storage backend that can be read from
class IStreamBuf : public IStreamBase
{
public:
    virtual const uint8_t * data() const = 0;
    virtual size_t size() const = 0;

protected:
    IStreamBuf() = default;
};

// Class template for a stream with an in-memory storage backend that can store either const or non-const data as desired
template <typename T, std::enable_if_t<std::is_same_v<T, uint8_t> || std::is_same_v<T, const uint8_t>, bool> = true>
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

    size_t size() const override
    {
        return sizeg();
    }

    size_t tell() const
    {
        return tellg();
    }

    void seek( const size_t pos )
    {
        assert( _itbeg <= _itput );

        const size_t putPos = _itput - _itbeg;

        _itget = ( pos < putPos ? _itbeg + pos : _itput );
    }

    void skip( size_t size ) override
    {
        _itget += ( size < sizeg() ? size : sizeg() );
    }

    uint16_t getBE16() override
    {
        uint16_t v = ( static_cast<uint16_t>( get8() ) << 8 );

        v |= get8();

        return v;
    }

    uint16_t getLE16() override
    {
        uint16_t v = get8();

        v |= ( static_cast<uint16_t>( get8() ) << 8 );

        return v;
    }

    uint32_t getBE32() override
    {
        uint32_t v = ( static_cast<uint32_t>( get8() ) << 24 );

        v |= ( static_cast<uint32_t>( get8() ) << 16 );
        v |= ( static_cast<uint32_t>( get8() ) << 8 );
        v |= get8();

        return v;
    }

    uint32_t getLE32() override
    {
        uint32_t v = get8();

        v |= ( static_cast<uint32_t>( get8() ) << 8 );
        v |= ( static_cast<uint32_t>( get8() ) << 16 );
        v |= ( static_cast<uint32_t>( get8() ) << 24 );

        return v;
    }

    // If a zero size is specified, then all still unread data is returned
    std::vector<uint8_t> getRaw( size_t size ) override
    {
        const size_t remainSize = sizeg();
        const size_t resultSize = size > 0 ? size : remainSize;
        const size_t sizeToCopy = std::min( resultSize, remainSize );

        std::vector<uint8_t> v( resultSize, 0 );

        std::copy( _itget, _itget + sizeToCopy, v.data() );

        _itget += sizeToCopy;

        return v;
    }

    // Reads no more than 'size' bytes of data (if a zero size is specified, then all still unread data
    // is read), forms a string that ends with the first null character found in this data (or includes
    // all data if this data does not contain null characters), and returns this string
    std::string getString( const size_t size = 0 )
    {
        const size_t remainSize = sizeg();
        const size_t sizeToSkip = size > 0 ? std::min( size, remainSize ) : remainSize;

        T * strBeg = _itget;
        _itget += sizeToSkip;

        return { strBeg, std::find( strBeg, _itget, 0 ) };
    }

protected:
    StreamBufTmpl() = default;

    size_t sizeg() const
    {
        assert( _itget <= _itput );

        return _itput - _itget;
    }

    size_t tellg() const
    {
        assert( _itbeg <= _itget );

        return _itget - _itbeg;
    }

    uint8_t get8() override
    {
        if ( sizeg() > 0 ) {
            return *( _itget++ );
        }

        setFail();

        return 0;
    }

    size_t capacity() const
    {
        assert( _itbeg <= _itend );

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

    explicit RWStreamBuf( const size_t size );

    RWStreamBuf( const RWStreamBuf & ) = delete;

    ~RWStreamBuf() override = default;

    RWStreamBuf & operator=( const RWStreamBuf & ) = delete;

    void putBE32( uint32_t v ) override;
    void putLE32( uint32_t v ) override;
    void putBE16( uint16_t v ) override;
    void putLE16( uint16_t v ) override;

    void putRaw( const void * ptr, size_t size ) override;

private:
    void put8( const uint8_t v ) override;

    size_t sizep() const;
    size_t tellp() const;

    void reallocBuf( size_t size );

    std::unique_ptr<uint8_t[]> _buf;
};

// Stream with read-only in-memory storage backed by a const vector instance (either internal or external, depending on the constructor used)
class ROStreamBuf final : public StreamBufTmpl<const uint8_t>
{
public:
    // Creates a non-owning stream on top of an external buffer ("view mode")
    explicit ROStreamBuf( const std::vector<uint8_t> & buf );
    // Takes ownership of the given buffer (through the move operation) and creates a stream on top of it
    explicit ROStreamBuf( std::vector<uint8_t> && buf );

    ROStreamBuf( const ROStreamBuf & ) = delete;

    ~ROStreamBuf() override = default;

    ROStreamBuf & operator=( const ROStreamBuf & ) = delete;

    // If a zero size is specified, then a view of all still unread data is returned
    std::pair<const uint8_t *, size_t> getRawView( const size_t size = 0 );

    // Returns a string view of no more than 'size' bytes of data ending with the first null character found in
    // this data (or of all the data in the corresponding range if this data does not contain null characters).
    // If a zero size is specified, then the entire unread amount of data is considered. Advances the cursor
    // intended for reading data forward by the full amount of the data used (regardless of the presence of null
    // characters in this data).
    std::string_view getStringView( const size_t size = 0 );

private:
    // Buffer to which the transfer of ownership of the external buffer takes place. This buffer is not used in
    // the non-owning ("view") mode.
    const std::vector<uint8_t> _buf;
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

    // If a zero size is specified, then all still unread data is returned
    ROStreamBuf getStreamBuf( const size_t size = 0 );

    void seek( const size_t pos );
    void skip( size_t size ) override;

    uint16_t getBE16() override;
    uint16_t getLE16() override;
    uint32_t getBE32() override;
    uint32_t getLE32() override;

    void putBE16( uint16_t v ) override;
    void putLE16( uint16_t v ) override;
    void putBE32( uint32_t v ) override;
    void putLE32( uint32_t v ) override;

    // If a zero size is specified, then all still unread data is returned
    std::vector<uint8_t> getRaw( const size_t size ) override;

    void putRaw( const void * ptr, size_t size ) override;

    // Reads no more than 'size' bytes of data (if a zero size is specified, then all still unread data
    // is read), forms a string that ends with the first null character found in this data (or includes
    // all data if this data does not contain null characters), and returns this string
    std::string getString( const size_t size = 0 );

private:
    size_t sizeg();

    uint8_t get8() override;
    void put8( const uint8_t v ) override;

    template <typename T>
    T getUint()
    {
        if ( !_file ) {
            return 0;
        }

        T v;

        if ( std::fread( &v, sizeof( T ), 1, _file.get() ) != 1 ) {
            setFail();

            return 0;
        }

        return v;
    }

    template <typename T>
    void putUint( const T v )
    {
        if ( !_file ) {
            return;
        }

        if ( std::fwrite( &v, sizeof( T ), 1, _file.get() ) != 1 ) {
            setFail();
        }
    }

    std::unique_ptr<std::FILE, int ( * )( std::FILE * )> _file{ nullptr, []( std::FILE * f ) { return std::fclose( f ); } };
};

namespace fheroes2
{
    // Get a value of type T in the system byte order from the buffer in which it was originally stored in the little-endian byte order
    template <typename T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, bool> = true>
    T getLEValue( const char * data, const size_t base, const size_t offset = 0 )
    {
        const char * begin = data + base + offset * sizeof( T );
        const char * end = begin + sizeof( T );

        T v;

#if defined( BYTE_ORDER ) && defined( LITTLE_ENDIAN ) && BYTE_ORDER == LITTLE_ENDIAN
        std::copy( begin, end, reinterpret_cast<char *>( &v ) );
#elif defined( BYTE_ORDER ) && defined( BIG_ENDIAN ) && BYTE_ORDER == BIG_ENDIAN
        std::reverse_copy( begin, end, reinterpret_cast<char *>( &v ) );
#else
#error "Unknown byte order"
#endif

        return v;
    }
}
