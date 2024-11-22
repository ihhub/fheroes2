/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "tinyconfig.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <regex>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "logging.h"
#include "serialize.h"
#include "tools.h"

namespace
{
    std::string ModifyKey( const std::string & str )
    {
        std::string key = StringTrim( StringLower( str ) );

        // Replace consecutive space-like characters with only one such character
        key.erase( std::unique( key.begin(), key.end(), []( const unsigned char a, const unsigned char b ) { return std::isspace( a ) && std::isspace( b ); } ),
                   key.end() );

        // Replace all space-like characters with spaces
        std::replace_if(
            key.begin(), key.end(), []( const unsigned char c ) { return std::isspace( c ); }, '\x20' );

        return key;
    }

    template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
    bool convertToInt( const std::string_view str, T & intValue )
    {
        const char * first = str.data();
        const char * last = str.data() + str.size();

        const auto [ptr, ec] = std::from_chars( first, last, intValue );

        return ( ptr == last && ec == std::errc() );
    }
}

TinyConfig::TinyConfig( const char sep, const char com )
    : separator( sep )
    , comment( com )
{}

bool TinyConfig::Load( const std::string & cfile )
{
    StreamFile sf;
    if ( !sf.open( cfile, "rb" ) ) {
        return false;
    }

    for ( const std::string & line : StringSplit( sf.getString(), '\n' ) ) {
        std::string str = StringTrim( line );

        if ( str.empty() || str[0] == comment ) {
            continue;
        }

        size_t pos = str.find( separator );
        if ( pos == std::string::npos ) {
            continue;
        }

        const std::string key = ModifyKey( StringTrim( str.substr( 0, pos ) ) );
        const std::string val = StringTrim( str.substr( pos + 1, str.length() - pos - 1 ) );

        if ( const auto [dummy, inserted] = emplace( key, val ); !inserted ) {
            ERROR_LOG( "Duplicate key '" << key << "' was found when reading the config file " << cfile )
        }
    }

    return true;
}

int TinyConfig::IntParams( const std::string & key ) const
{
    const_iterator it = find( ModifyKey( key ) );
    if ( it == end() ) {
        return 0;
    }

    int result;

    if ( !convertToInt( it->second, result ) ) {
        return 0;
    }

    return result;
}

std::string TinyConfig::StrParams( const std::string & key ) const
{
    const_iterator it = find( ModifyKey( key ) );
    return it != end() ? it->second : "";
}

fheroes2::Point TinyConfig::PointParams( const std::string & key, const fheroes2::Point & fallbackValue ) const
{
    const const_iterator it = find( ModifyKey( key ) );
    if ( it == end() ) {
        return fallbackValue;
    }

    static const std::regex pointRegex( "^\\[ *(-?[0-9]+) *, *(-?[0-9]+) *]$", std::regex_constants::extended );

    std::smatch pointRegexMatch;

    if ( !std::regex_match( it->second, pointRegexMatch, pointRegex ) ) {
        return fallbackValue;
    }

    assert( pointRegexMatch.size() == 3 );

    fheroes2::Point result;

    static_assert( std::is_integral<decltype( result.x )>::value && std::is_integral<decltype( result.y )>::value,
                   "The type of result fields is not integer, check the logic of this method" );

    if ( !convertToInt( pointRegexMatch[1].str(), result.x ) ) {
        return fallbackValue;
    }
    if ( !convertToInt( pointRegexMatch[2].str(), result.y ) ) {
        return fallbackValue;
    }

    return result;
}

fheroes2::ResolutionInfo TinyConfig::ResolutionParams( const std::string & key, const fheroes2::ResolutionInfo & fallbackValue ) const
{
    const const_iterator it = find( ModifyKey( key ) );
    if ( it == end() ) {
        return fallbackValue;
    }

    {
        // Current format: in-game width x in-game height : on-screen width x on-screen height
        static const std::regex resolutionRegex( "^ *([0-9]+) *x *([0-9]+) *: *([0-9]+) *x *([0-9]+) *$", std::regex_constants::extended );

        std::smatch resolutionRegexMatch;

        if ( std::regex_match( it->second, resolutionRegexMatch, resolutionRegex ) ) {
            assert( resolutionRegexMatch.size() == 5 );

            int32_t gameWidth{ 0 };
            int32_t gameHeight{ 0 };
            int32_t screenWidth{ 0 };
            int32_t screenHeight{ 0 };

            if ( !convertToInt( resolutionRegexMatch[1].str(), gameWidth ) ) {
                return fallbackValue;
            }
            if ( !convertToInt( resolutionRegexMatch[2].str(), gameHeight ) ) {
                return fallbackValue;
            }
            if ( !convertToInt( resolutionRegexMatch[3].str(), screenWidth ) ) {
                return fallbackValue;
            }
            if ( !convertToInt( resolutionRegexMatch[4].str(), screenHeight ) ) {
                return fallbackValue;
            }

            return { gameWidth, gameHeight, screenWidth, screenHeight };
        }
    }

    {
        // Old format: in-game width x in-game height x on-screen multiplier
        static const std::regex resolutionRegex( "^ *([0-9]+) *x *([0-9]+) *x *([0-9]+) *$", std::regex_constants::extended );

        std::smatch resolutionRegexMatch;

        if ( std::regex_match( it->second, resolutionRegexMatch, resolutionRegex ) ) {
            assert( resolutionRegexMatch.size() == 4 );

            int32_t gameWidth{ 0 };
            int32_t gameHeight{ 0 };
            int32_t screenMultiplier{ 0 };

            if ( !convertToInt( resolutionRegexMatch[1].str(), gameWidth ) ) {
                return fallbackValue;
            }
            if ( !convertToInt( resolutionRegexMatch[2].str(), gameHeight ) ) {
                return fallbackValue;
            }
            if ( !convertToInt( resolutionRegexMatch[3].str(), screenMultiplier ) ) {
                return fallbackValue;
            }

            assert( gameWidth >= 0 && gameHeight >= 0 && screenMultiplier >= 0 );

            if ( screenMultiplier == 0 ) {
                return fallbackValue;
            }
            // Check for potential signed overflow on multiplication
            if ( gameWidth > std::numeric_limits<int32_t>::max() / screenMultiplier || gameHeight > std::numeric_limits<int32_t>::max() / screenMultiplier ) {
                return fallbackValue;
            }

            return { gameWidth, gameHeight, gameWidth * screenMultiplier, gameHeight * screenMultiplier };
        }
    }

    {
        // Abbreviated format: in-game width x in-game height with on-screen multiplier 1
        static const std::regex resolutionRegex( "^ *([0-9]+) *x *([0-9]+) *$", std::regex_constants::extended );

        std::smatch resolutionRegexMatch;

        if ( std::regex_match( it->second, resolutionRegexMatch, resolutionRegex ) ) {
            assert( resolutionRegexMatch.size() == 3 );

            int32_t gameWidth{ 0 };
            int32_t gameHeight{ 0 };

            if ( !convertToInt( resolutionRegexMatch[1].str(), gameWidth ) ) {
                return fallbackValue;
            }
            if ( !convertToInt( resolutionRegexMatch[2].str(), gameHeight ) ) {
                return fallbackValue;
            }

            return { gameWidth, gameHeight };
        }
    }

    return fallbackValue;
}

bool TinyConfig::Exists( const std::string & key ) const
{
    return end() != find( ModifyKey( key ) );
}
