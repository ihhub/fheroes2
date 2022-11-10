/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

// IWYU pragma: no_include <bits/std_abs.h>
// IWYU pragma: no_include <ext/alloc_traits.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream> // IWYU pragma: keep

#include "logging.h"
#include "tools.h"

/* trim left right space */
std::string StringTrim( std::string str )
{
    if ( str.empty() ) {
        return str;
    }

    // left
    std::string::iterator iter = str.begin();
    while ( iter != str.end() && std::isspace( static_cast<unsigned char>( *iter ) ) ) {
        ++iter;
    }

    if ( iter == str.end() ) {
        // Do not erase anything if we reached the end of the string. Just immediately return an empty string.
        return {};
    }

    if ( iter != str.begin() )
        str.erase( str.begin(), iter );

    // right
    iter = str.end() - 1;
    while ( iter != str.begin() && std::isspace( static_cast<unsigned char>( *iter ) ) ) {
        --iter;
    }

    if ( iter != str.end() - 1 ) {
        str.erase( iter + 1, str.end() );
    }

    return str;
}

std::string StringLower( std::string str )
{
    std::transform( str.begin(), str.end(), str.begin(), []( const unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
    return str;
}

std::string StringUpper( std::string str )
{
    std::transform( str.begin(), str.end(), str.begin(), []( const unsigned char c ) { return static_cast<char>( std::toupper( c ) ); } );
    return str;
}

int CountBits( uint32_t val )
{
    int res = 0;

    for ( uint32_t itr = 0x00000001; itr; itr <<= 1 )
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

bool SaveMemToFile( const std::vector<uint8_t> & data, const std::string & path )
{
    std::fstream file;
    file.open( path, std::fstream::out | std::fstream::trunc | std::fstream::binary );

    if ( !file ) {
        ERROR_LOG( "Unable to open file for writing: " << path )
        return false;
    }

    file.write( reinterpret_cast<const char *>( data.data() ), static_cast<std::streamsize>( data.size() ) );

    return true;
}

std::vector<uint8_t> LoadFileToMem( const std::string & path )
{
    std::fstream file;
    file.open( path, std::fstream::in | std::fstream::binary );
    if ( !file ) {
        return {};
    }

    file.seekg( 0, std::fstream::end );
    std::streamoff length = file.tellg();
    if ( length < 1 ) {
        return {};
    }

    std::vector<uint8_t> data( length );

    size_t dataToRead = static_cast<size_t>( length );
    size_t dataAlreadyRead = 0;

    const size_t blockSize = 4 * 1024 * 1024; // read by 4 MB blocks

    while ( dataToRead > 0 ) {
        size_t readSize = dataToRead > blockSize ? blockSize : dataToRead;

        file.read( reinterpret_cast<char *>( data.data() + dataAlreadyRead ), static_cast<std::streamsize>( readSize ) );

        dataAlreadyRead += readSize;
        dataToRead -= readSize;
    }

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

        const int32_t dx = std::abs( pt2.x - pt1.x );
        const int32_t dy = std::abs( pt2.y - pt1.y );

        int32_t ns = ( dx > dy ? dx : dy ) / 2;

        Point pt( pt1 );

        for ( int32_t i = 0; i <= ( dx > dy ? dx : dy ); ++i ) {
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

        Point tempPoint( from.x + std::abs( max.x - from.x ) / 2, from.y - std::abs( max.y - from.y ) * 3 / 4 );
        std::vector<Point> points = GetLinePoints( from, tempPoint, step );
        res.insert( res.end(), points.begin(), points.end() );

        points = GetLinePoints( tempPoint, max, step );
        res.insert( res.end(), points.begin(), points.end() );

        tempPoint = { max.x + std::abs( to.x - max.x ) / 2, to.y - std::abs( to.y - max.y ) * 3 / 4 };
        points = GetLinePoints( max, tempPoint, step );
        res.insert( res.end(), points.begin(), points.end() );

        points = GetLinePoints( tempPoint, to, step );
        res.insert( res.end(), points.begin(), points.end() );

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

    Rect getBoundaryRect( const Rect & rt1, const Rect & rt2 )
    {
        const int32_t x = std::min( rt1.x, rt2.x );
        const int32_t y = std::min( rt1.y, rt2.y );
        const int32_t width = std::max( rt1.x + rt1.width, rt2.x + rt2.width ) - x;
        const int32_t height = std::max( rt1.y + rt1.height, rt2.y + rt2.height ) - y;

        return { x, y, width, height };
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

    void replaceStringEnding( std::string & output, const char * originalEnding, const char * correctedEnding )
    {
        assert( originalEnding != nullptr && correctedEnding != nullptr );

        const size_t originalEndingSize = strlen( originalEnding );
        const size_t correctedEndingSize = strlen( correctedEnding );
        if ( output.size() < originalEndingSize ) {
            // The original string is smaller than the ending.
            return;
        }

        if ( memcmp( output.data() + output.size() - originalEndingSize, originalEnding, originalEndingSize ) != 0 ) {
            // The string does not have the required ending.
            return;
        }

        output.replace( output.size() - originalEndingSize, originalEndingSize, correctedEnding, correctedEndingSize );
    }

    std::string abbreviateNumber( const int num )
    {
        if ( std::abs( num ) >= 1000000 ) {
            return std::to_string( num / 1000000 ) + 'M';
        }

        if ( std::abs( num ) >= 1000 ) {
            return std::to_string( num / 1000 ) + 'K';
        }

        return std::to_string( num );
    }
}
