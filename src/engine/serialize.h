/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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

#ifndef H2SERIALIZE_H
#define H2SERIALIZE_H

#include <algorithm>
#include <cstdio>
#include <list>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

#include "endian_h2.h"
#include "math_base.h"
#include "types.h"

class StreamBase
{
public:
    StreamBase()
        : flags( 0 )
    {}
    virtual ~StreamBase() = default;

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

    virtual u16 getBE16() = 0;
    virtual u16 getLE16() = 0;
    virtual u32 getBE32() = 0;
    virtual u32 getLE32() = 0;

    virtual void putBE32( u32 ) = 0;
    virtual void putLE32( u32 ) = 0;
    virtual void putBE16( u16 ) = 0;
    virtual void putLE16( u16 ) = 0;

    virtual std::vector<u8> getRaw( size_t = 0 /* all data */ ) = 0;
    virtual void putRaw( const char *, size_t ) = 0;

    u16 get16();
    u32 get32();

    void put16( u16 );
    void put32( u32 );

    uint8_t get()
    {
        return get8();
    }

    void put( const uint8_t ch )
    {
        put8( ch );
    }

    StreamBase & operator>>( bool & );
    StreamBase & operator>>( char & );
    StreamBase & operator>>( u8 & );
    StreamBase & operator>>( u16 & );
    StreamBase & operator>>( int16_t & );
    StreamBase & operator>>( u32 & );
    StreamBase & operator>>( s32 & );
    StreamBase & operator>>( std::string & );

    StreamBase & operator>>( fheroes2::Point & point_ );

    StreamBase & operator<<( const bool );
    StreamBase & operator<<( const char );
    StreamBase & operator<<( const u8 );
    StreamBase & operator<<( const u16 );
    StreamBase & operator<<( const int16_t );
    StreamBase & operator<<( const u32 );
    StreamBase & operator<<( const s32 );
    StreamBase & operator<<( const std::string & );

    StreamBase & operator<<( const fheroes2::Point & point_ );

    template <class Type1, class Type2>
    StreamBase & operator>>( std::pair<Type1, Type2> & p )
    {
        return *this >> p.first >> p.second;
    }

    template <class Type>
    StreamBase & operator>>( std::vector<Type> & v )
    {
        const u32 size = get32();
        v.resize( size );
        for ( typename std::vector<Type>::iterator it = v.begin(); it != v.end(); ++it )
            *this >> *it;
        return *this;
    }

    template <class Type>
    StreamBase & operator>>( std::list<Type> & v )
    {
        const u32 size = get32();
        v.resize( size );
        for ( typename std::list<Type>::iterator it = v.begin(); it != v.end(); ++it )
            *this >> *it;
        return *this;
    }

    template <class Type1, class Type2>
    StreamBase & operator>>( std::map<Type1, Type2> & v )
    {
        const u32 size = get32();
        v.clear();
        for ( u32 ii = 0; ii < size; ++ii ) {
            std::pair<Type1, Type2> pr;
            *this >> pr;
            v.insert( pr );
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
        put32( static_cast<u32>( v.size() ) );
        for ( typename std::vector<Type>::const_iterator it = v.begin(); it != v.end(); ++it )
            *this << *it;
        return *this;
    }

    template <class Type>
    StreamBase & operator<<( const std::list<Type> & v )
    {
        put32( static_cast<u32>( v.size() ) );
        for ( typename std::list<Type>::const_iterator it = v.begin(); it != v.end(); ++it )
            *this << *it;
        return *this;
    }

    template <class Type1, class Type2>
    StreamBase & operator<<( const std::map<Type1, Type2> & v )
    {
        put32( static_cast<u32>( v.size() ) );
        for ( typename std::map<Type1, Type2>::const_iterator it = v.begin(); it != v.end(); ++it )
            *this << *it;
        return *this;
    }

protected:
    size_t flags;

    virtual u8 get8() = 0;
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
    explicit StreamBuf( size_t = 0 );
    StreamBuf( const StreamBuf & );
    explicit StreamBuf( const std::vector<u8> & );
    StreamBuf( const u8 *, size_t );

    ~StreamBuf() override;

    StreamBuf & operator=( const StreamBuf & );

    const u8 * data( void ) const;
    size_t size( void ) const;
    size_t capacity( void ) const;

    void seek( size_t );
    void skip( size_t ) override;

    u16 getBE16() override;
    u16 getLE16() override;
    u32 getBE32() override;
    u32 getLE32() override;

    void putBE32( u32 ) override;
    void putLE32( u32 ) override;
    void putBE16( u16 ) override;
    void putLE16( u16 ) override;

    std::vector<u8> getRaw( size_t = 0 /* all data */ ) override;
    void putRaw( const char *, size_t ) override;

    std::string toString( size_t = 0 /* all data */ );

protected:
    void reset( void );

    size_t tellg( void ) const override;
    size_t tellp( void ) const override;
    size_t sizeg( void ) const override;
    size_t sizep( void ) const override;

    void copy( const StreamBuf & );
    void reallocbuf( size_t );

    u8 get8() override;
    void put8( const uint8_t v ) override;

    friend class ZStreamBuf;

    u8 * itbeg;
    u8 * itget;
    u8 * itput;
    u8 * itend;
};

class StreamFile : public StreamBase
{
public:
    StreamFile();
    StreamFile( const StreamFile & ) = delete;
    StreamFile & operator=( const StreamFile & ) = delete;

    ~StreamFile() override;

    size_t size( void ) const;
    size_t tell( void ) const;

    bool open( const std::string &, const std::string & mode );
    void close( void );

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
    size_t sizeg( void ) const override;
    size_t sizep( void ) const override;
    size_t tellg( void ) const override;
    size_t tellp( void ) const override;

    u8 get8() override;
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
        static_assert( false, "Unknown byte order" );
#endif

        return result;
    }
}

#endif
