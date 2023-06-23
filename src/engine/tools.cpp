/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include "tools.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream> // IWYU pragma: keep
#include <limits>
#include <memory>
#include <system_error>

#include <zconf.h>
#include <zlib.h>

#include "translations.h"

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

void StringReplaceWithLowercase( std::string & workString, const char * pattern, const std::string & patternReplacement )
{
    if ( pattern == nullptr ) {
        return;
    }

    for ( size_t position = workString.find( pattern ); position != std::string::npos; position = workString.find( pattern ) ) {
        // To determine if the end of a sentence was before this word we parse the character before it
        // for the presence of full stop, question mark, or exclamation mark, skipping whitespace characters.
        const char prevWordEnd = [&workString, position]() {
            assert( position < workString.size() );

            const auto iter = std::find_if_not( workString.rbegin() + static_cast<int32_t>( workString.size() - position ), workString.rend(),
                                                []( const unsigned char c ) { return std::isspace( c ); } );
            if ( iter != workString.rend() ) {
                return *iter;
            }

            // Before 'position' there is nothing, or there are only spaces.
            return '\0';
        }();

        // Also if the insert 'position' equals zero, then it is the first word in a sentence.
        if ( position == 0 || prevWordEnd == '.' || prevWordEnd == '?' || prevWordEnd == '!' ) {
            // Also, 'patternReplacement' can consist of two words (for example, "Power Liches") and if
            // it is placed as the first word in sentence, then we have to lowercase only the second word.
            // To detect this, we look for a space mark in 'patternReplacement'.
            const size_t spacePosition = patternReplacement.find( ' ' );

            // The first (and possibly only) word of 'patternReplacement' replaces 'pattern' in 'workString'.
            workString.replace( position, std::strlen( pattern ), patternReplacement.substr( 0, spacePosition ) );

            // Check if a space mark was found to insert the rest part of 'patternReplacement' with lowercase applied.
            if ( spacePosition != std::string::npos ) {
                workString.insert( position + spacePosition, Translation::StringLower( patternReplacement.substr( spacePosition ) ) );
            }
        }
        else {
            // For all other cases lowercase the 'patternReplacement' and replace the 'pattern' with it in 'workString'.
            workString.replace( position, std::strlen( pattern ), Translation::StringLower( patternReplacement ) );
        }
    }
}

void StringReplace( std::string & dst, const char * pred, const std::string & src )
{
    size_t pos;

    while ( std::string::npos != ( pos = dst.find( pred ) ) ) {
        dst.replace( pos, std::strlen( pred ), src );
    }
}

std::vector<std::string> StringSplit( const std::string & str, const std::string & sep )
{
    std::vector<std::string> vec;
    size_t pos1 = 0;
    size_t pos2;

    while ( pos1 < str.size() && std::string::npos != ( pos2 = str.find( sep, pos1 ) ) ) {
        vec.push_back( str.substr( pos1, pos2 - pos1 ) );
        pos1 = pos2 + sep.size();
    }

    // tail
    if ( pos1 < str.size() )
        vec.push_back( str.substr( pos1, str.size() - pos1 ) );

    return vec;
}

std::string insertCharToString( const std::string & inputString, const size_t position, const char character )
{
    std::string outputString = inputString;

    if ( position >= inputString.size() ) {
        outputString.append( 1, character );
    }
    else {
        outputString.insert( position, 1, character );
    }

    return outputString;
}

int Sign( int s )
{
    return ( s < 0 ? -1 : ( s > 0 ? 1 : 0 ) );
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
        const uint32_t dist = static_cast<uint32_t>( std::hypot( dx, dy ) );
        // Round up the integer division and avoid the division by zero in calculation of total line points.
        const uint32_t length = ( step > 0 ) ? ( dist + step / 2 ) / step : 0;

        std::vector<Point> line;

        if ( length < 2 ) {
            // If the length is equal to 0 than 'pt2' could be closer to 'pt1' than 'step'.
            // In this case we put 'pt1' as the start of the line.
            line.emplace_back( pt1 );
            // And put 'pt2' as the end of the line only if 'pt1' is not equal to 'pt2'.
            if ( pt1 != pt2 ) {
                line.emplace_back( pt2 );
            }
        }
        else {
            // Otherwise we calculate the euclidean line, using the determined parameters.
            const double moveX = dx / static_cast<double>( length );
            const double moveY = dy / static_cast<double>( length );

            line.reserve( length + 1 );

            for ( uint32_t i = 0; i <= length; ++i ) {
                line.emplace_back( static_cast<int>( pt1.x + i * moveX ), static_cast<int>( pt1.y + i * moveY ) );
            }
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

    std::vector<Point> GetArcPoints( const Point & from, const Point & to, const int32_t arcHeight, const int32_t step )
    {
        std::vector<Point> res;
        Point pt( from );
        // The first projectile point is "from"
        res.push_back( pt );

        // Calculate the number of projectile trajectory points
        const int32_t steps = ( to.x - from.x ) / step;

        // Trajectory start point coordinates
        const double x1 = from.x;
        const double y1 = from.y;

        // Distance to the destination point along the axes
        const double dx = to.x - x1;
        const double dy = to.y - y1;

        // The movement of the projectile is determined according to the parabolic
        // throwing approximation. The first two parabola points are "from" and
        // "to" with an exception that the second ("to") point is at the same
        // height as the start point. The parabola third point "y" coordinate is
        // set using the "arcHeight" parameter, which determines the height of the
        // parabola arc. And its "x" coordinate is taken equal to half the path
        // from the start point to the end point. Using this three point
        // coordinates, a system of three linear equations (y=a*x*x+b*x+c) in
        // three variables is solved by substituting these points "x" and "y".
        // Considering that on an isometric battlefield, the target location above
        // or below corresponds to a simple turn of the shooter to the left or
        // right, a linear movement from point "from" to point "to" is added to the
        // parabola ('dy/dx' in 'b' constant and '-x1*dy/dx' in 'c' constant).

        // Calculation of the parabola equation coefficients
        const double a = 4 * arcHeight / dx / dx;
        const double b = dy / dx - a * ( dx + 2 * x1 );
        const double c = y1 + a * x1 * ( dx + x1 ) - x1 * dy / dx;

        for ( int32_t i = 1; i <= steps; ++i ) {
            pt.x += step;
            pt.y = static_cast<int32_t>( std::lround( a * pt.x * pt.x + b * pt.x + c ) );
            res.push_back( pt );
        }

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
        if ( rt2.width == 0 && rt2.height == 0 ) {
            return rt1;
        }

        if ( rt1.width == 0 && rt1.height == 0 ) {
            return rt2;
        }

        const int32_t x = std::min( rt1.x, rt2.x );
        const int32_t y = std::min( rt1.y, rt2.y );
        const int32_t width = std::max( rt1.x + rt1.width, rt2.x + rt2.width ) - x;
        const int32_t height = std::max( rt1.y + rt1.height, rt2.y + rt2.height ) - y;

        return { x, y, width, height };
    }

    uint32_t calculateCRC32( const uint8_t * data, const size_t length )
    {
        if ( length > std::numeric_limits<uInt>::max() ) {
            throw std::
                system_error( std::make_error_code( std::errc::value_too_large ),
                              "Too large `length` provided to `calculateCRC32`. Must be no larger than `std::numeric_limits<uInt>::max()` (usually `(1 << 32) - 1`)." );
        }

        return static_cast<uint32_t>( crc32( 0, data, static_cast<uInt>( length ) ) );
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
