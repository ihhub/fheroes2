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
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
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

    bool isconstbuf() const
    {
        return ( _flags & CONST_BUF ) != 0;
    }

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

    virtual std::vector<uint8_t> getRaw( size_t = 0 /* all data */ ) = 0;
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

    void setconstbuf( bool f );
    void setfail( bool f );

private:
    enum : uint32_t
    {
        FAILURE = 0x00000001,
        CONST_BUF = 0x00000002,
        BIGENDIAN = 0x00000004
    };

    uint32_t _flags{ 0 };
};

class StreamBuf final : public StreamBase
{
public:
    explicit StreamBuf( const size_t sz = 0 );
    explicit StreamBuf( const std::vector<uint8_t> & buf );

    StreamBuf( const StreamBuf & ) = delete;
    StreamBuf( StreamBuf && stream ) noexcept;

    ~StreamBuf() override;

    StreamBuf & operator=( const StreamBuf & ) = delete;
    StreamBuf & operator=( StreamBuf && stream ) noexcept;

    const uint8_t * data() const
    {
        return itget;
    }

    size_t size()
    {
        return sizeg();
    }

    size_t capacity() const
    {
        return itend - itbeg;
    }

    void seek( size_t sz )
    {
        itget = itbeg + sz < itend ? itbeg + sz : itend;
    }

    void skip( size_t sz ) override;

    uint16_t getBE16() override;
    uint16_t getLE16() override;
    uint32_t getBE32() override;
    uint32_t getLE32() override;

    void putBE32( uint32_t v ) override;
    void putLE32( uint32_t v ) override;
    void putBE16( uint16_t v ) override;
    void putLE16( uint16_t v ) override;

    std::vector<uint8_t> getRaw( size_t sz = 0 /* all data */ ) override;
    void putRaw( const void * ptr, size_t sz ) override;

    std::string toString( const size_t size = 0 );

private:
    friend class StreamFile;

    void reset();

    size_t tellg() override;
    size_t tellp() override;
    size_t sizeg() override;
    size_t sizep() override;

    void reallocbuf( size_t size );

    uint8_t get8() override;
    void put8( const uint8_t v ) override;

    // After using this method to write data, update the cursor by calling the advance() method.
    uint8_t * dataForWriting()
    {
        return itput;
    }

    // Advances the cursor intended for writing data forward by a specified number of bytes.
    void advance( const size_t size )
    {
        itput += size;
    }

    uint8_t * itbeg{ nullptr };
    uint8_t * itget{ nullptr };
    uint8_t * itput{ nullptr };
    uint8_t * itend{ nullptr };
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

    // 0 stands for full data.
    StreamBuf toStreamBuf( const size_t size = 0 );

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

    // 0 stands for full data.
    std::vector<uint8_t> getRaw( const size_t size = 0 ) override;

    void putRaw( const void * ptr, size_t sz ) override;

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

    std::unique_ptr<std::FILE, std::function<int( std::FILE * )>> _file{ nullptr, std::fclose };
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
