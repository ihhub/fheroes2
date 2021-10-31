/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

namespace fheroes2
{
    enum class SupportedLanguage : int
    {
        English = 0, // default language for all version of the game.
        French, // GoG version
        Polish, // GoG version
        German, // GoG version
        Russian, // Buka and XXI Vek versions
        Italian // Rare version?
    };

    // Game resources can support up to 2 languages. One of them is always English.
    // This function returns supported from resources language except English, otherwise English language will be returned.
    SupportedLanguage getSupportedLanguage();

    // Return name of the language.
    const char * getLanguageName( const SupportedLanguage language );

    const char * getLanguageAbbreviation( const SupportedLanguage language );

    SupportedLanguage getLanguageFromAbbreviation( const std::string & abbreviation );
}
