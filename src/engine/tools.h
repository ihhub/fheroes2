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
#ifndef H2TOOLS_H
#define H2TOOLS_H

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "math_base.h"

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
std::string GetHexString( T value, int width = 8 )
{
    std::ostringstream stream;

    stream << "0x" << std::setw( width ) << std::setfill( '0' ) << std::hex << value;

    return stream.str();
}

int GetInt( const std::string & );
int Sign( int );

std::string StringTrim( std::string );

std::string StringLower( std::string str );
std::string StringUpper( std::string str );

std::vector<std::string> StringSplit( const std::string &, const std::string & );

// Function to replace the pattern in workString with patternReplacement. Here the patternReplacement is converted to lowercase except for the first word in a sentence.
void StringReplaceWithLowercase( std::string & workString, const char * pattern, const std::string & patternReplacement );
void StringReplace( std::string &, const char *, const std::string & );
void StringReplace( std::string &, const char *, int );

int CountBits( uint32_t );

std::string InsertString( const std::string &, size_t, const char * );

namespace fheroes2
{
    double GetAngle( const Point & start, const Point & target );
    std::vector<Point> GetEuclideanLine( const Point & pt1, const Point & pt2, const uint32_t step );
    std::vector<Point> GetLinePoints( const Point & pt1, const Point & pt2, const int32_t step );
    std::vector<Point> GetArcPoints( const Point & from, const Point & to, const int32_t arcHeight, const int32_t step );

    int32_t GetRectIndex( const std::vector<Rect> & rects, const Point & pt );

    Rect getBoundaryRect( const Rect & rt1, const Rect & rt2 );

    uint32_t calculateCRC32( const uint8_t * data, const size_t length );

    template <class T>
    void hashCombine( uint32_t & seed, const T & v )
    {
        std::hash<T> hasher;
        seed ^= hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    }

    template <size_t N>
    std::bitset<N> makeBitsetFromVector( const std::vector<int> & vector )
    {
        std::bitset<N> result;
        for ( const int index : vector ) {
            result.set( index, true );
        }
        return result;
    }

    void replaceStringEnding( std::string & output, const char * originalEnding, const char * correctedEnding );

    std::string abbreviateNumber( const int num );
}

#endif
