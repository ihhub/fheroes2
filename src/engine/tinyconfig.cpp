/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <regex>
#include <system_error>
#include <utility>
#include <vector>

#include "serialize.h"
#include "tinyconfig.h"
#include "tools.h"

namespace
{
    bool SpaceCompare( char a, char b )
    {
        return std::isspace( a ) && std::isspace( b );
    }

    std::string ModifyKey( const std::string & str )
    {
        std::string key = StringTrim( StringLower( str ) );

        // remove multiple space
        key.erase( std::unique( key.begin(), key.end(), SpaceCompare ), key.end() );

        // change space
        std::replace_if( key.begin(), key.end(), ::isspace, '\x20' );

        return key;
    }
}

TinyConfig::TinyConfig( char sep, char com )
    : separator( sep )
    , comment( com )
{}

bool TinyConfig::Load( const std::string & cfile )
{
    StreamFile sf;
    if ( !sf.open( cfile, "rb" ) )
        return false;

    std::vector<std::string> rows = StringSplit( sf.toString(), "\n" );

    for ( std::vector<std::string>::const_iterator it = rows.begin(); it != rows.end(); ++it ) {
        std::string str = StringTrim( *it );

        if ( str.empty() || str[0] == comment )
            continue;

        size_t pos = str.find( separator );
        if ( std::string::npos != pos ) {
            std::string left( str.substr( 0, pos ) );
            std::string right( str.substr( pos + 1, str.length() - pos - 1 ) );

            left = StringTrim( left );
            right = StringTrim( right );

            emplace( ModifyKey( left ), right );
        }
    }

    return true;
}

int TinyConfig::IntParams( const std::string & key ) const
{
    const_iterator it = find( ModifyKey( key ) );
    return it != end() ? GetInt( it->second ) : 0;
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

    auto convertToInt = []( const std::string & str, auto & intValue ) {
        auto [ptr, ec] = std::from_chars( str.data(), str.data() + str.size(), intValue );

        return ec == std::errc();
    };

    fheroes2::Point result;

    if ( !convertToInt( pointRegexMatch[1].str(), result.x ) ) {
        return fallbackValue;
    }
    if ( !convertToInt( pointRegexMatch[2].str(), result.y ) ) {
        return fallbackValue;
    }

    return result;
}

bool TinyConfig::Exists( const std::string & key ) const
{
    return end() != find( ModifyKey( key ) );
}
