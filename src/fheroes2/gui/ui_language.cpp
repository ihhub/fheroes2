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

#include "ui_language.h"
#include "agg.h"
#include "agg_image.h"
#include "icn.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"

#include <cassert>
#include <map>
#include <set>

namespace
{
    const std::map<uint32_t, fheroes2::SupportedLanguage> languageCRC32 = { { 0x406967B9, fheroes2::SupportedLanguage::French }, // GoG version
                                                                            { 0x04745D1D, fheroes2::SupportedLanguage::German }, // GoG version
                                                                            { 0x88774771, fheroes2::SupportedLanguage::Polish }, // GoG version
                                                                            { 0xDB10FFD8, fheroes2::SupportedLanguage::Russian }, // XXI Vek version
                                                                            { 0xD5CF8AF3, fheroes2::SupportedLanguage::Russian }, // Buka version
                                                                            { 0x219B3124, fheroes2::SupportedLanguage::Italian } };

    // Strings in this map must in lower case and non translatable.
    const std::map<std::string, fheroes2::SupportedLanguage> languageName
        = { { "pl", fheroes2::SupportedLanguage::Polish },      { "polish", fheroes2::SupportedLanguage::Polish },   { "de", fheroes2::SupportedLanguage::German },
            { "german", fheroes2::SupportedLanguage::German },  { "fr", fheroes2::SupportedLanguage::French },       { "french", fheroes2::SupportedLanguage::French },
            { "ru", fheroes2::SupportedLanguage::Russian },     { "russian", fheroes2::SupportedLanguage::Russian }, { "it", fheroes2::SupportedLanguage::Italian },
            { "italian", fheroes2::SupportedLanguage::Italian } };

    fheroes2::SupportedLanguage getResourceLanguage()
    {
        const std::vector<uint8_t> & data = ::AGG::ReadChunk( ICN::GetString( ICN::FONT ) );
        if ( data.empty() ) {
            // How is it possible to run the game without a font?
            assert( 0 );
            return fheroes2::SupportedLanguage::English;
        }

        const uint32_t crc32 = fheroes2::calculateCRC32( data.data(), data.size() );
        auto iter = languageCRC32.find( crc32 );
        if ( iter == languageCRC32.end() ) {
            return fheroes2::SupportedLanguage::English;
        }

        return iter->second;
    }
}

namespace fheroes2
{
    LanguageSwitcher::LanguageSwitcher( const SupportedLanguage language )
        : _currentLanguage( Settings::Get().getGameLanguage() )
    {
        Settings::Get().setGameLanguage( getLanguageAbbreviation( language ) );
    }

    LanguageSwitcher::~LanguageSwitcher()
    {
        Settings::Get().setGameLanguage( _currentLanguage );
    }

    std::vector<SupportedLanguage> getSupportedLanguages()
    {
        std::vector<SupportedLanguage> languages;

        const SupportedLanguage resourceLanguage = getResourceLanguage();
        if ( resourceLanguage != SupportedLanguage::English ) {
            languages.emplace_back( resourceLanguage );
        }

        const std::set<SupportedLanguage> possibleLanguages{ SupportedLanguage::French, SupportedLanguage::Polish, SupportedLanguage::German, SupportedLanguage::Russian,
                                                             SupportedLanguage::Italian };

        for ( const SupportedLanguage language : possibleLanguages ) {
            if ( language != resourceLanguage && AGG::isAlphabetSupported( language ) ) {
                languages.emplace_back( language );
            }
        }

        Settings & conf = Settings::Get();

        fheroes2::SupportedLanguage currentLanguage = fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() );

        std::vector<fheroes2::SupportedLanguage> validSupportedLanguages{ fheroes2::SupportedLanguage::English };

        for ( fheroes2::SupportedLanguage language : languages ) {
            if ( conf.setGameLanguage( fheroes2::getLanguageAbbreviation( language ) ) ) {
                validSupportedLanguages.emplace_back( language );
            }
        }

        conf.setGameLanguage( fheroes2::getLanguageAbbreviation( currentLanguage ) );

        assert( !validSupportedLanguages.empty() );

        return validSupportedLanguages;
    }

    const char * getLanguageName( const SupportedLanguage language )
    {
        switch ( language ) {
        case SupportedLanguage::English:
            return _( "English" );
        case SupportedLanguage::French:
            return _( "French" );
        case SupportedLanguage::Polish:
            return _( "Polish" );
        case SupportedLanguage::German:
            return _( "German" );
        case SupportedLanguage::Russian:
            return _( "Russian" );
        case SupportedLanguage::Italian:
            return _( "Italian" );
        default:
            // Did you add a new language? Please add the code to handle it.
            assert( 0 );
            return nullptr;
        }
    }

    const char * getLanguageAbbreviation( const SupportedLanguage language )
    {
        switch ( language ) {
        case SupportedLanguage::English:
            return ""; // English is a special case. It always returns an empty string as it's a default language.
        case SupportedLanguage::French:
            return "fr";
        case SupportedLanguage::Polish:
            return "pl";
        case SupportedLanguage::German:
            return "de";
        case SupportedLanguage::Russian:
            return "ru";
        case SupportedLanguage::Italian:
            return "it";
        default:
            // Did you add a new language? Please add the code to handle it.
            assert( 0 );
            return nullptr;
        }
    }

    SupportedLanguage getLanguageFromAbbreviation( const std::string & abbreviation )
    {
        if ( abbreviation.empty() ) {
            return SupportedLanguage::English;
        }

        const std::string name( StringLower( abbreviation ) );

        auto iter = languageName.find( name );
        if ( iter == languageName.end() ) {
            // Unsupported language. Fallback to English.
            return SupportedLanguage::English;
        }

        return iter->second;
    }

    void updateAlphabet( const std::string & abbreviation )
    {
        const SupportedLanguage language = getLanguageFromAbbreviation( abbreviation );
        const bool isOriginalResourceLanguage = ( language == SupportedLanguage::English ) || ( language == getResourceLanguage() );

        AGG::updateAlphabet( language, isOriginalResourceLanguage );
    }
}
