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

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace Translation
{
    // Sets the language with the given name as the current language if the translation for this language is
    // already cached and valid, otherwise does nothing. Returns a pair of two flags, the first of which is
    // set to true if the translation for the given language is already present in the cache (even if this
    // translation is invalid), and the second is set to true if the current language has been successfully
    // set.
    std::pair<bool, bool> setLanguage( const std::string_view langName );

    // Sets the language with the given name as the current language if the translation for this language is
    // already cached and valid. If this translation is not yet cached, then tries to load it from the given
    // file. If the load fails, then adds this language to the cache as invalid and does nothing else. Returns
    // true if the current language has been successfully set, otherwise returns false.
    bool setLanguage( const std::string & langName, const std::string_view fileName );

    // Resets the current language to the default language (English).
    void reset();

    const char * gettext( const char * str );
    const char * gettext( const std::string & str );
    const char * ngettext( const char * str, const char * plural, size_t num );

    // Converts the given string to lowercase in a locale aware way
    std::string StringLower( std::string str );
}

#define _( str ) Translation::gettext( str )
#define _n( str, plural, num ) Translation::ngettext( str, plural, num )

constexpr const char * gettext_noop( const char * s )
{
    return s;
}

// Replaces the pattern in workString with patternReplacement. The patternReplacement is converted to lowercase in a locale
// aware way, except for the first word in a sentence.
void StringReplaceWithLowercase( std::string & workString, const char * pattern, const std::string & patternReplacement );
