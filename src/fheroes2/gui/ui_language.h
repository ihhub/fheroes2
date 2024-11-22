/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include <string>
#include <vector>

#include "game_language.h"

namespace fheroes2
{
    class LanguageSwitcher
    {
    public:
        explicit LanguageSwitcher( const SupportedLanguage language );
        LanguageSwitcher( const LanguageSwitcher & ) = delete;
        LanguageSwitcher( LanguageSwitcher && ) = delete;

        LanguageSwitcher & operator=( const LanguageSwitcher & ) = delete;
        LanguageSwitcher & operator=( LanguageSwitcher && ) = delete;

        ~LanguageSwitcher();

    private:
        const std::string _currentLanguage;
    };

    SupportedLanguage getResourceLanguage();

    // This function returns an array of supported languages. If the array contains only one language it must be English.
    std::vector<SupportedLanguage> getSupportedLanguages();

    // Return name of the language. Call this function only within the scope of LanguageSwitcher object.
    const char * getLanguageName( const SupportedLanguage language );

    const char * getLanguageAbbreviation( const SupportedLanguage language );

    SupportedLanguage getLanguageFromAbbreviation( const std::string & abbreviation );

    void updateAlphabet( const std::string & abbreviation );

    SupportedLanguage getCurrentLanguage();
}
