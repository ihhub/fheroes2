/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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
    StreamBase()
        : flags( 0 )
    {}

    StreamBase( const StreamBase & ) = delete;

    StreamBase( StreamBase && stream ) noexcept
        : flags( 0 )
    {
        std::swap( flags, stream.flags );
    }

    virtual ~StreamBase() = default;

    StreamBase & operator=( const StreamBase & ) = delete;

    StreamBase & operator=( StreamBase && stream ) noexcept
    {
        std::swap( flags, stream.flags );

        return *this;
    }

    void setbigendian( bool );

    bool isconstbuf() const
    {
        return ( flags & 0x00001000 ) != 0;
    }

    bool fail() const
    {
        return flags & 0x00000001;
    }

    bool bigendian() const
    {
        return ( flags & 0x80000000 ) != 0;
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
    virtual void putRaw( const char *, size_t ) = 0;

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
    StreamBase & operator>>( uint8_t & v );
    StreamBase & operator>>( uint16_t & v );
    StreamBase & operator>>( int16_t & v );
    StreamBase & operator>>( uint32_t & v );
    StreamBase & operator>>( int32_t & v );
    StreamBase & operator>>( std::string & v );

    StreamBase & operator>>( fheroes2::Point & point_ );

    StreamBase & operator<<( const bool v );
    StreamBase & operator<<( const char v );
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
    size_t flags;

    virtual uint8_t get8() = 0;
    virtual void put8( const uint8_t ) = 0;

    virtual size_t sizeg() const = 0;
    virtual size_t sizep() const = 0;
    virtual size_t tellg() const = 0;
    virtual size_t tellp() const = 0;

    void setconstbuf( bool );
    void setfail( bool );
};

class StreamBuf : public StreamBase
{
public:
    explicit StreamBuf( const size_t sz = 0 );
    StreamBuf( const StreamBuf & st ) = delete;
    StreamBuf( StreamBuf && st ) noexcept;

    explicit StreamBuf( const std::vector<uint8_t> & );
    StreamBuf( const uint8_t *, size_t );

    ~StreamBuf() override;

    StreamBuf & operator=( const StreamBuf & st ) = delete;
    StreamBuf & operator=( StreamBuf && st ) noexcept;

    const uint8_t * data() const;
    size_t size() const;
    size_t capacity() const;

    void seek( size_t );
    void skip( size_t ) override;

    uint16_t getBE16() override;
    uint16_t getLE16() override;
    uint32_t getBE32() override;
    uint32_t getLE32() override;

    void putBE32( uint32_t v ) override;
    void putLE32( uint32_t v ) override;
    void putBE16( uint16_t v ) override;
    void putLE16( uint16_t v ) override;

    std::vector<uint8_t> getRaw( size_t sz = 0 /* all data */ ) override;
    void putRaw( const char * ptr, size_t sz ) override;

    std::string toString( size_t sz = 0 /* all data */ );

protected:
    void reset();

    size_t tellg() const override;
    size_t tellp() const override;
    size_t sizeg() const override;
    size_t sizep() const override;

    void reallocbuf( size_t );

    uint8_t get8() override;
    void put8( const uint8_t v ) override;

    friend class ZStreamBuf;

    uint8_t * itbeg;
    uint8_t * itget;
    uint8_t * itput;
    uint8_t * itend;
};

class StreamFile : public StreamBase
{
public:
    StreamFile();
    StreamFile( const StreamFile & ) = delete;
    StreamFile( StreamFile && ) = delete;

    StreamFile & operator=( const StreamFile & ) = delete;
    StreamFile & operator=( StreamFile && ) = delete;

    ~StreamFile() override;

    size_t size() const;
    size_t tell() const;

    bool open( const std::string &, const std::string & mode );
    void close();

    StreamBuf toStreamBuf( size_t = 0 /* all data */ );

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

    std::vector<uint8_t> getRaw( size_t = 0 /* all data */ ) override;
    void putRaw( const char *, size_t ) override;

    std::string toString( size_t = 0 /* all data */ );

protected:
    size_t sizeg() const override;
    size_t sizep() const override;
    size_t tellg() const override;
    size_t tellp() const override;

    uint8_t get8() override;
    void put8( const uint8_t v ) override;

private:
    std::FILE * _file;

    template <typename T>
    T getUint()
    {
        if ( !_file )
            return 0;
        T val;
        return std::fread( &val, sizeof( T ), 1, _file ) == 1 ? val : 0;
    }

    template <typename T>
    void putUint( const T val )
    {
        if ( _file )
            std::fwrite( &val, sizeof( T ), 1, _file );
    }
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
