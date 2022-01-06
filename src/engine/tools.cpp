/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>

#include "logging.h"
#include "tools.h"

#include <SDL.h>

/* trim left right space */
std::string StringTrim( std::string str )
{
    if ( str.empty() )
        return str;

    std::string::iterator iter;

    // left
    iter = str.begin();
    while ( iter != str.end() && std::isspace( static_cast<unsigned char>( *iter ) ) )
        ++iter;
    if ( iter != str.begin() )
        str.erase( str.begin(), iter );

    if ( str.empty() )
        return str;

    // right
    iter = str.end() - 1;
    while ( iter != str.begin() && std::isspace( static_cast<unsigned char>( *iter ) ) )
        --iter;
    if ( iter != str.end() - 1 )
        str.erase( iter + 1, str.end() );

    return str;
}

/* convert to lower case */
std::string StringLower( std::string str )
{
    std::transform( str.begin(), str.end(), str.begin(), []( const unsigned char c ) { return std::tolower( c ); } );
    return str;
}

std::string GetStringShort( int value )
{
    if ( std::abs( value ) > 1000 ) {
        if ( std::abs( value ) > 1000000 ) {
            return std::to_string( value / 1000000 ) + 'M';
        }

        return std::to_string( value / 1000 ) + 'K';
    }

    return std::to_string( value );
}

std::string GetHexString( int value, int width )
{
    std::ostringstream stream;
    stream << "0x" << std::setw( width ) << std::setfill( '0' ) << std::hex << value;
    return stream.str();
}

int CountBits( u32 val )
{
    int res = 0;

    for ( u32 itr = 0x00000001; itr; itr <<= 1 )
        if ( val & itr )
            ++res;

    return res;
}

int GetInt( const std::string & str )
{
    int res = 0;

    // decimal
    if ( std::all_of( str.begin(), str.end(), []( const unsigned char c ) { return std::isdigit( c ); } ) ) {
        std::istringstream ss( str );
        ss >> res;
    }
    else if ( str.size() > 2 && ( str.at( 0 ) == '+' || str.at( 0 ) == '-' )
              && std::all_of( str.begin() + 1, str.end(), []( const unsigned char c ) { return std::isdigit( c ); } ) ) {
        std::istringstream ss( str );
        ss >> res;
    }
    else
        // hex
        if ( str.size() > 3 && str.at( 0 ) == '0' && std::tolower( str.at( 1 ) ) == 'x'
             && std::all_of( str.begin() + 2, str.end(), []( const unsigned char c ) { return std::isxdigit( c ); } ) ) {
        std::istringstream ss( str );
        ss >> std::hex >> res;
    }
    else
    // str
    {
        std::string lower = StringLower( str );

        if ( lower == "on" )
            return 1;
        else if ( lower == "one" )
            return 1;
        else if ( lower == "two" )
            return 2;
        else if ( lower == "three" )
            return 3;
        else if ( lower == "four" )
            return 4;
        else if ( lower == "five" )
            return 5;
        else if ( lower == "six" )
            return 6;
        else if ( lower == "seven" )
            return 7;
        else if ( lower == "eight" )
            return 8;
        else if ( lower == "nine" )
            return 9;
    }

    return res;
}

void StringReplace( std::string & dst, const char * pred, const std::string & src )
{
    size_t pos = std::string::npos;

    while ( std::string::npos != ( pos = dst.find( pred ) ) )
        dst.replace( pos, std::strlen( pred ), src );
}

void StringReplace( std::string & dst, const char * pred, int value )
{
    StringReplace( dst, pred, std::to_string( value ) );
}

std::vector<std::string> StringSplit( const std::string & str, const std::string & sep )
{
    std::vector<std::string> vec;
    size_t pos1 = 0;
    size_t pos2 = std::string::npos;

    while ( pos1 < str.size() && std::string::npos != ( pos2 = str.find( sep, pos1 ) ) ) {
        vec.push_back( str.substr( pos1, pos2 - pos1 ) );
        pos1 = pos2 + sep.size();
    }

    // tail
    if ( pos1 < str.size() )
        vec.push_back( str.substr( pos1, str.size() - pos1 ) );

    return vec;
}

std::string InsertString( const std::string & src, size_t pos, const char * c )
{
    std::string res = src;

    if ( pos >= src.size() )
        res.append( c );
    else
        res.insert( pos, c );

    return res;
}

int Sign( int s )
{
    return ( s < 0 ? -1 : ( s > 0 ? 1 : 0 ) );
}

bool SaveMemToFile( const std::vector<u8> & data, const std::string & file )
{
    SDL_RWops * rw = SDL_RWFromFile( file.c_str(), "wb" );

    if ( rw && 1 == SDL_RWwrite( rw, &data[0], static_cast<int>( data.size() ), 1 ) )
        SDL_RWclose( rw );
    else {
        ERROR_LOG( SDL_GetError() );
        return false;
    }

    return true;
}

std::vector<u8> LoadFileToMem( const std::string & file )
{
    std::vector<u8> data;
    SDL_RWops * rw = SDL_RWFromFile( file.c_str(), "rb" );
    if ( rw == nullptr ) {
        ERROR_LOG( SDL_GetError() );
        return data;
    }

    const Sint64 length = SDL_RWseek( rw, 0, RW_SEEK_END );
    if ( length < 0 )
        ERROR_LOG( SDL_GetError() );

    if ( length > 0 ) {
        data.resize( length );
        SDL_RWseek( rw, 0, RW_SEEK_SET );
        SDL_RWread( rw, &data[0], static_cast<int>( data.size() ), 1 );
    }

    SDL_RWclose( rw );

    return data;
}

namespace fheroes2
{
    double GetAngle( const Point & start, const Point & target )
    {
        const int dx = target.x - start.x;
        const int dy = target.y - start.y;
        double angle = atan2( -dy, dx ) * 180.0 / M_PI;
        // we only care about two quadrants, normalize
        if ( dx < 0 ) {
            angle = ( dy <= 0 ) ? 180 - angle : -angle - 180;
        }
        return angle;
    }

    std::vector<Point> GetEuclideanLine( const Point & pt1, const Point & pt2, const uint32_t step )
    {
        const int dx = pt2.x - pt1.x;
        const int dy = pt2.y - pt1.y;
        const uint32_t dist = static_cast<uint32_t>( std::hypot( std::abs( dx ), std::abs( dy ) ) );
        // round up the integer division
        const uint32_t length = ( step > 0 && dist >= step / 2 ) ? ( dist + step / 2 ) / step : 1;
        const double moveX = dx / static_cast<double>( length );
        const double moveY = dy / static_cast<double>( length );

        std::vector<Point> line;
        line.reserve( length );

        for ( uint32_t i = 0; i <= length; ++i ) {
            line.emplace_back( static_cast<int>( pt1.x + i * moveX ), static_cast<int>( pt1.y + i * moveY ) );
        }

        return line;
    }

    std::vector<Point> GetLinePoints( const Point & pt1, const Point & pt2, const int32_t step )
    {
        std::vector<Point> res;
        res.reserve( 10 );

        const int32_t dx = std::abs( pt2.x - pt1.x );
        const int32_t dy = std::abs( pt2.y - pt1.y );

        int32_t ns = std::div( ( dx > dy ? dx : dy ), 2 ).quot;
        Point pt( pt1 );

        for ( u16 i = 0; i <= ( dx > dy ? dx : dy ); ++i ) {
            if ( dx > dy ) {
                pt.x < pt2.x ? ++pt.x : --pt.x;
                ns -= dy;
            }
            else {
                pt.y < pt2.y ? ++pt.y : --pt.y;
                ns -= dx;
            }

            if ( ns < 0 ) {
                if ( dx > dy ) {
                    pt.y < pt2.y ? ++pt.y : --pt.y;
                    ns += dx;
                }
                else {
                    pt.x < pt2.x ? ++pt.x : --pt.x;
                    ns += dy;
                }
            }

            if ( 0 == ( i % step ) )
                res.push_back( pt );
        }

        return res;
    }

    std::vector<Point> GetArcPoints( const Point & from, const Point & to, const Point & max, const int32_t step )
    {
        std::vector<Point> res;

        Point pt1( from );
        Point pt2( from.x + std::abs( max.x - from.x ) / 2, from.y - std::abs( max.y - from.y ) * 3 / 4 );
        const std::vector<Point> & pts1 = GetLinePoints( pt1, pt2, step );
        res.insert( res.end(), pts1.begin(), pts1.end() );

        pt1 = pt2;
        pt2 = max;
        const std::vector<Point> & pts2 = GetLinePoints( pt1, pt2, step );
        res.insert( res.end(), pts2.begin(), pts2.end() );

        pt1 = max;
        pt2 = Point( max.x + std::abs( to.x - max.x ) / 2, to.y - std::abs( to.y - max.y ) * 3 / 4 );
        const std::vector<Point> & pts3 = GetLinePoints( pt1, pt2, step );
        res.insert( res.end(), pts3.begin(), pts3.end() );

        pt1 = pt2;
        pt2 = to;
        const std::vector<Point> & pts4 = GetLinePoints( pt1, pt2, step );
        res.insert( res.end(), pts4.begin(), pts4.end() );

        return res;
    }

    int32_t GetRectIndex( const std::vector<Rect> & rects, const Point & pt )
    {
        for ( size_t i = 0; i < rects.size(); ++i ) {
            if ( rects[i] & pt )
                return static_cast<int32_t>( i );
        }

        return -1;
    }

    std::pair<Rect, Point> Fixed4Blit( const Rect & srcrt, const Rect & dstrt )
    {
        std::pair<Rect, Point> res;
        Rect & srcrtfix = res.first;
        Point & dstptfix = res.second;

        if ( srcrt.width && srcrt.height && srcrt.x + srcrt.width > dstrt.x && srcrt.y + srcrt.height > dstrt.y && srcrt.x < dstrt.x + dstrt.width
             && srcrt.y < dstrt.y + dstrt.height ) {
            srcrtfix.width = srcrt.width;
            srcrtfix.height = srcrt.height;
            dstptfix.x = srcrt.x;
            dstptfix.y = srcrt.y;

            if ( srcrt.x < dstrt.x ) {
                srcrtfix.x = dstrt.x - srcrt.x;
                dstptfix.x = dstrt.x;
            }

            if ( srcrt.y < dstrt.y ) {
                srcrtfix.y = dstrt.y - srcrt.y;
                dstptfix.y = dstrt.y;
            }

            if ( dstptfix.x + srcrtfix.width > dstrt.x + dstrt.width )
                srcrtfix.width = dstrt.x + dstrt.width - dstptfix.x;

            if ( dstptfix.y + srcrtfix.height > dstrt.y + dstrt.height )
                srcrtfix.height = dstrt.y + dstrt.height - dstptfix.y;
        }

        return res;
    }

    Rect getBoundaryRect( const Rect & rt1, const Rect & rt2 )
    {
        Rect rt3;

        rt3.x = std::min( rt1.x, rt2.x );
        rt3.y = std::min( rt1.y, rt2.y );
        rt3.width = std::max( rt1.x + rt1.width, rt2.x + rt2.width ) - rt3.x;
        rt3.height = std::max( rt1.y + rt1.height, rt2.y + rt2.height ) - rt3.y;

        return rt3;
    }

    uint32_t calculateCRC32( const uint8_t * data, const size_t length )
    {
        uint32_t crc = 0xFFFFFFFF;
        for ( size_t i = 0; i < length; ++i ) {
            crc ^= static_cast<uint32_t>( data[i] );

            for ( int bit = 0; bit < 8; ++bit ) {
                const uint32_t poly = ( crc & 1 ) ? 0xEDB88320 : 0x0;
                crc = ( crc >> 1 ) ^ poly;
            }
        }

        return ~crc;
    }
}
