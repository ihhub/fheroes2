/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <cstdlib>
#include <cstring>
#include <limits>
#include <system_error>

#include <zconf.h>
#include <zlib.h>

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

void StringReplace( std::string & dst, const char * pred, const std::string_view src )
{
    size_t pos;

    while ( std::string::npos != ( pos = dst.find( pred ) ) ) {
        dst.replace( pos, std::strlen( pred ), src );
    }
}

std::vector<std::string> StringSplit( const std::string_view str, const char sep )
{
    std::vector<std::string> result;

    size_t startPos = 0;

    for ( size_t sepPos = str.find( sep ); sepPos != std::string::npos; sepPos = str.find( sep, startPos ) ) {
        assert( startPos < str.size() && sepPos < str.size() );

        result.emplace_back( str.begin() + startPos, str.begin() + sepPos );

        startPos = sepPos + 1;
    }

    assert( startPos <= str.size() );

    result.emplace_back( str.begin() + startPos, str.end() );

    return result;
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

int Sign( const int i )
{
    if ( i < 0 ) {
        return -1;
    }
    if ( i > 0 ) {
        return 1;
    }
    return 0;
}

namespace fheroes2
{
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

    void appendModifierToString( std::string & str, const int mod )
    {
        if ( mod < 0 ) {
            // The minus sign is already present
            str.append( " " );
        }
        else if ( mod > 0 ) {
            str.append( " +" );
        }

        str.append( std::to_string( mod ) );
    }
}
