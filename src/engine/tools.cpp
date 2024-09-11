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

std::vector<std::string> StringSplit( const std::string & str, const char sep )
{
    std::vector<std::string> vec;
    size_t pos1 = 0;
    size_t pos2 = 0;

    while ( pos1 < str.size() ) {
        pos2 = str.find( sep, pos1 );
        if ( pos2 == std::string::npos ) {
            break;
        }

        vec.push_back( str.substr( pos1, pos2 - pos1 ) );
        pos1 = pos2 + 1;
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
