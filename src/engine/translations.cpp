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

#include "translations.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "logging.h"
#include "serialize.h"
#include "tools.h"

namespace
{
    const char contextSeparator = '|';

    // Character lookup table for custom tolower
    // Compatible with ASCII, custom French encoding, CP1250 and CP1251
    const std::array<unsigned char, 256> tolowerLUT
        = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // Non-printable characters
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,

            // SP !    "    #    $    %    &     '    (    )    *    +    ,    -    .    /  (ASCII)
            // SP !    "    ô    û    %    ù     '    (    )    â    +    ,    -    .    /  (French, #$&* are ôûùâ)
            ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',

            // 0  1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?   (ASCII)
            // 0  1    2    3    4    5    6    7    8    9    :    ;    ï    =    î    ?   (French, <> are ïî)
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',

            // @  A→a  B→b  C→c  D→d  E→e  F→f  G→g  H→h  I→i  J→j  K→k  L→l  M→m  N→n  O→o (ASCII)
            // à  A→a  B→b  C→c  D→d  E→e  F→f  G→g  H→h  I→i  J→j  K→k  L→l  M→m  N→n  O→o (French, @ is à)
            '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',

            // P→p Q→q R→r  S→s  T→t  U→u  V→v  W→w  X→x  Y→y  Z→z  [    \     ]    ^    _  (ASCII)
            // P→p Q→q R→r  S→s  T→t  U→u  V→v  W→w  X→x  Y→y  Z→z  [    \     ]    ç    _  (French, ^ is ç)
            'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '[', '\\', ']', '^', '_',

            // `  a    b    c    d    e    f    g    h    i    j    k    l    m    n    o   (ASCII)
            // è  a    b    c    d    e    f    g    h    i    j    k    l    m    n    o   (French, ` is è)
            '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',

            // p  q    r    s    t    u    v    w    x    y    z    {    |    }    ~    DEL (ASCII)
            // p  q    r    s    t    u    v    w    x    y    z    {    ê    }    é    DEL (French, |~ are êé)
            'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', 0x7F,

            // €         ‚           „     …     †     ‡           ‰     Š→š   ‹     Ś→ś   Ť→ť   Ž→ž   Ź→ź (1250, except €)
            // Ђ→ђ Ѓ→ѓ   ‚     ѓ     „     …     †     ‡     €     ‰     Љ→љ   ‹     Њ→њ   Ќ→ќ   Ћ→ћ   Џ→џ (1251)
            0x90, 0x83, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x9A, 0x8B, 0x9C, 0x9D, 0x9E, 0x9F,

            //     ‘     ’     “     ”     •     –     —           ™     š     ›     ś     ť     ž     ź   (1250)
            // ђ   ‘     ’     “     ”     •     –     —           ™     љ     ›     њ     ќ     ћ     џ   (1251)
            0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,

            // NBS ˇ     ˘     Ł→ł   ¤     Ą→ą   ¦     §     ¨     ©     Ş→ş   «     ¬     SHY   ®     Ż→ż (1250 except ˇ¨)
            // NBP Ў→ў   ў     Ј     ¤     Ґ     ¦     §     Ё→ё   ©     Є→є   «     ¬     SHY   ®     Ї→ї (1251 except ЈҐ)
            0xA0, 0xA2, 0xA2, 0xB3, 0xA4, 0xB9, 0xA6, 0xA7, 0xB8, 0xA9, 0xBA, 0xAB, 0xAC, 0xAD, 0xAE, 0xBF,

            // °   ±     ˛     ł     ´     µ     ¶     ·     ¸     ą     ş     »     Ľ→ľ   ˝     ľ     ż   (1250 except ˛˝)
            // °   ±     І→і   і     ґ     µ     ¶     ·     ё     №     є     »     ј     Ѕ→ѕ   ѕ     ї   (1251 except ј)
            0xB0, 0xB1, 0xB3, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBE, 0xBE, 0xBE, 0xBF,

            // Ŕ→ŕ Á→á   Â→â   Ă→ă   Ä→ä   Ĺ→ĺ   Ć→ć   Ç→ç   Č→č   É→é   Ę→ę   Ë→ë   Ě→ě   Í→í   Î→î   Ď→ď (1250)
            // А→а Б→б   В→в   Г→г   Д→д   Е→е   Ж→ж   З→з   И→и   Й→й   К→к   Л→л   М→м   Н→н   О→о   П→п (1251)
            0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,

            // Đ→đ Ń→ń   Ň→ň   Ó→ó   Ô→ô   Ő→ő   Ö→ö   ×     Ř→ř   Ů→ů   Ú→ú   Ű→ű   Ü→ü   Ý→ý   Ţ→ţ   ß   (1250, except ×ß)
            // Р→р С→с   Т→т   У→у   Ф→ф   Х→х   Ц→ц   Ч→ч   Ш→ш   Щ→щ   Ъ→ъ   Ы→ы   Ь→ь   Э→э   Ю→ю   Я→я (1251)
            0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,

            // ŕ   á     â     ă     ä     ĺ     ć     ç     č     é     ę     ë     ě     í     î     ď   (1250)
            // а   б     в     г     д     е     ж     з     и     й     к     л     м     н     о     п   (1251)
            0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,

            // đ   ń     ň     ó     ô     ő     ö     ÷     ř     ů     ú     ű     ü     ý     ţ     ˙   (1250)
            // р   с     т     у     ф     х     ц     ч     ш     щ     ъ     ы     ь     э     ю     я   (1251)
            0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF };

    enum class LocaleType : int
    {
        LOCALE_EN,
        LOCALE_AF,
        LOCALE_AR,
        LOCALE_BE,
        LOCALE_BG,
        LOCALE_CA,
        LOCALE_CS,
        LOCALE_DE,
        LOCALE_DK,
        LOCALE_EL,
        LOCALE_ES,
        LOCALE_ET,
        LOCALE_EU,
        LOCALE_FI,
        LOCALE_FR,
        LOCALE_GL,
        LOCALE_HE,
        LOCALE_HR,
        LOCALE_HU,
        LOCALE_ID,
        LOCALE_IT,
        LOCALE_LA,
        LOCALE_LT,
        LOCALE_LV,
        LOCALE_MK,
        LOCALE_NB,
        LOCALE_NL,
        LOCALE_PL,
        LOCALE_PT,
        LOCALE_RO,
        LOCALE_RU,
        LOCALE_SK,
        LOCALE_SL,
        LOCALE_SR,
        LOCALE_SV,
        LOCALE_TR,
        LOCALE_UK,
        LOCALE_VI
    };

    LocaleType langToLocale( const std::string_view langName )
    {
        static const std::unordered_map<std::string_view, LocaleType> langLocale{ // Afrikaans
                                                                                  { "af", LocaleType::LOCALE_AF },
                                                                                  { "afrikaans", LocaleType::LOCALE_AF },
                                                                                  // Arabic
                                                                                  { "ar", LocaleType::LOCALE_AR },
                                                                                  { "arabic", LocaleType::LOCALE_AR },
                                                                                  // Belarusian
                                                                                  { "be", LocaleType::LOCALE_BE },
                                                                                  { "belarusian", LocaleType::LOCALE_BE },
                                                                                  // Bulgarian
                                                                                  { "bg", LocaleType::LOCALE_BG },
                                                                                  { "bulgarian", LocaleType::LOCALE_BG },
                                                                                  // Catalan
                                                                                  { "ca", LocaleType::LOCALE_CA },
                                                                                  { "catalan", LocaleType::LOCALE_CA },
                                                                                  // Czech
                                                                                  { "cs", LocaleType::LOCALE_CS },
                                                                                  { "czech", LocaleType::LOCALE_CS },
                                                                                  // Danish
                                                                                  { "dk", LocaleType::LOCALE_DK },
                                                                                  { "danish", LocaleType::LOCALE_DK },
                                                                                  // German
                                                                                  { "de", LocaleType::LOCALE_DE },
                                                                                  { "german", LocaleType::LOCALE_DE },
                                                                                  // Greek
                                                                                  { "el", LocaleType::LOCALE_EL },
                                                                                  { "greek", LocaleType::LOCALE_EL },
                                                                                  // Spanish
                                                                                  { "es", LocaleType::LOCALE_ES },
                                                                                  { "spanish", LocaleType::LOCALE_ES },
                                                                                  // Estonian
                                                                                  { "et", LocaleType::LOCALE_ET },
                                                                                  { "estonian", LocaleType::LOCALE_ET },
                                                                                  // Basque
                                                                                  { "eu", LocaleType::LOCALE_EU },
                                                                                  { "basque", LocaleType::LOCALE_EU },
                                                                                  // Finnish
                                                                                  { "fi", LocaleType::LOCALE_FI },
                                                                                  { "finnish", LocaleType::LOCALE_FI },
                                                                                  // French
                                                                                  { "fr", LocaleType::LOCALE_FR },
                                                                                  { "french", LocaleType::LOCALE_FR },
                                                                                  // Galician
                                                                                  { "gl", LocaleType::LOCALE_GL },
                                                                                  { "galician", LocaleType::LOCALE_GL },
                                                                                  // Hebrew
                                                                                  { "he", LocaleType::LOCALE_HE },
                                                                                  { "hebrew", LocaleType::LOCALE_HE },
                                                                                  // Croatian
                                                                                  { "hr", LocaleType::LOCALE_HR },
                                                                                  { "croatian", LocaleType::LOCALE_HR },
                                                                                  // Hungarian
                                                                                  { "hu", LocaleType::LOCALE_HU },
                                                                                  { "hungarian", LocaleType::LOCALE_HU },
                                                                                  // Indonesian
                                                                                  { "id", LocaleType::LOCALE_ID },
                                                                                  { "indonesian", LocaleType::LOCALE_ID },
                                                                                  // Italian
                                                                                  { "it", LocaleType::LOCALE_IT },
                                                                                  { "italian", LocaleType::LOCALE_IT },
                                                                                  // Latin
                                                                                  { "la", LocaleType::LOCALE_LA },
                                                                                  { "latin", LocaleType::LOCALE_LA },
                                                                                  // Lithuanian
                                                                                  { "lt", LocaleType::LOCALE_LT },
                                                                                  { "lithuanian", LocaleType::LOCALE_LT },
                                                                                  // Latvian
                                                                                  { "lv", LocaleType::LOCALE_LV },
                                                                                  { "latvian", LocaleType::LOCALE_LV },
                                                                                  // Macedonian
                                                                                  { "mk", LocaleType::LOCALE_MK },
                                                                                  { "macedonian", LocaleType::LOCALE_MK },
                                                                                  // Norwegian
                                                                                  { "nb", LocaleType::LOCALE_NB },
                                                                                  { "norwegian", LocaleType::LOCALE_NB },
                                                                                  // Dutch
                                                                                  { "nl", LocaleType::LOCALE_NL },
                                                                                  { "dutch", LocaleType::LOCALE_NL },
                                                                                  // Polish
                                                                                  { "pl", LocaleType::LOCALE_PL },
                                                                                  { "polish", LocaleType::LOCALE_PL },
                                                                                  // Portuguese
                                                                                  { "pt", LocaleType::LOCALE_PT },
                                                                                  { "portuguese", LocaleType::LOCALE_PT },
                                                                                  // Romanian
                                                                                  { "ro", LocaleType::LOCALE_RO },
                                                                                  { "romanian", LocaleType::LOCALE_RO },
                                                                                  // Russian
                                                                                  { "ru", LocaleType::LOCALE_RU },
                                                                                  { "russian", LocaleType::LOCALE_RU },
                                                                                  // Slovak
                                                                                  { "sk", LocaleType::LOCALE_SK },
                                                                                  { "slovak", LocaleType::LOCALE_SK },
                                                                                  // Slovenian
                                                                                  { "sl", LocaleType::LOCALE_SL },
                                                                                  { "slovenian", LocaleType::LOCALE_SL },
                                                                                  // Serbian
                                                                                  { "sr", LocaleType::LOCALE_SR },
                                                                                  { "serbian", LocaleType::LOCALE_SR },
                                                                                  // Swedish
                                                                                  { "sv", LocaleType::LOCALE_SV },
                                                                                  { "swedish", LocaleType::LOCALE_SV },
                                                                                  // Turkish
                                                                                  { "tr", LocaleType::LOCALE_TR },
                                                                                  { "turkish", LocaleType::LOCALE_TR },
                                                                                  // Ukrainian
                                                                                  { "uk", LocaleType::LOCALE_UK },
                                                                                  { "ukrainian", LocaleType::LOCALE_UK },
                                                                                  // Vietnamese
                                                                                  { "vi", LocaleType::LOCALE_VI },
                                                                                  { "vietnamese", LocaleType::LOCALE_VI } };

        const auto iter = langLocale.find( langName );
        if ( iter == langLocale.end() ) {
            assert( 0 );
            return LocaleType::LOCALE_EN;
        }

        return iter->second;
    }

    uint32_t crc32b( const std::string_view str )
    {
        uint32_t crc = 0xFFFFFFFF;

        for ( const char ch : str ) {
            crc ^= static_cast<uint32_t>( ch );

            for ( int bit = 0; bit < 8; ++bit ) {
                const uint32_t poly = ( crc & 1 ) ? 0xEDB88320 : 0x0;
                crc = ( crc >> 1 ) ^ poly;
            }
        }

        return ~crc;
    }

    bool getCharsetFromHeader( const std::string & hdr, std::string & charset )
    {
        constexpr std::string_view hdrEntry{ "Content-Type:" };
        constexpr std::string_view charsetDirective{ "charset=" };

        if ( hdr.size() <= hdrEntry.size() + charsetDirective.size() ) {
            return false;
        }

        if ( hdr.rfind( hdrEntry, 0 ) != 0 ) {
            return false;
        }

        const size_t pos = hdr.find( charsetDirective );
        if ( pos == std::string::npos ) {
            return false;
        }

        charset = hdr.substr( pos + charsetDirective.size() );

        return true;
    }

    const char * stripContext( const char * str )
    {
        const char * pos = std::strchr( str, contextSeparator );

        return pos ? ++pos : str;
    }

    class MOFile
    {
    public:
        MOFile() = default;

        const char * ngettext( const char * str, const size_t plural ) const
        {
            if ( !_isValid ) {
                assert( 0 );

                return stripContext( str );
            }

            const auto iter = std::as_const( _translations ).find( crc32b( str ) );
            if ( iter == _translations.cend() ) {
                return stripContext( str );
            }

            const auto & [dummy, pluralFormTranslations] = *iter;
            if ( plural >= pluralFormTranslations.size() ) {
                return stripContext( str );
            }

            const std::string & translatedStr = pluralFormTranslations[plural];
            if ( translatedStr.empty() ) {
                return stripContext( str );
            }

            return translatedStr.c_str();
        }

        bool load( const std::string_view langName, const std::string & fileName )
        {
            *this = {};

            StreamFile sf;
            if ( !sf.open( fileName, "rb" ) ) {
                return false;
            }

            ROStreamBuf sb = sf.getStreamBuf();
            if ( sf.fail() ) {
                ERROR_LOG( "I/O error when reading " << fileName )
                return false;
            }

            sf.close();

            {
                const uint32_t magicNumber = sb.getLE32();
                if ( sb.fail() ) {
                    ERROR_LOG( "I/O error when parsing " << fileName )
                    return false;
                }

                switch ( magicNumber ) {
                case 0xde120495:
                    sb.setBigendian( true );

                    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, "Setting big-endian mode for " << fileName )
                    break;
                case 0x950412de:
                    sb.setBigendian( false );

                    DEBUG_LOG( DBG_ENGINE, DBG_TRACE, "Setting little-endian mode for " << fileName )
                    break;
                default:
                    ERROR_LOG( "Incorrect magic number " << GetHexString( magicNumber ) << " for " << fileName )
                    return false;
                }
            }

            {
                const uint16_t majorVersion = sb.get16();
                if ( sb.fail() ) {
                    ERROR_LOG( "I/O error when parsing " << fileName )
                    return false;
                }

                if ( majorVersion != 0 ) {
                    ERROR_LOG( "Incorrect major version " << GetHexString( majorVersion, 4 ) << " for " << fileName )
                    return false;
                }
            }

            // Skip the minor version
            sb.skip( 2 );

            const uint32_t stringsCount = sb.get32();
            if ( stringsCount == 0 ) {
                ERROR_LOG( "There are no translated strings in " << fileName )
                return false;
            }

            const uint32_t originalStringsTableOffset = sb.get32();
            const uint32_t translationsTableOffset = sb.get32();
            if ( sb.fail() ) {
                ERROR_LOG( "I/O error when parsing " << fileName )
                return false;
            }

            // The hash table is optional and may be missing, and even if it is present, the actual hashing algorithm depends on the
            // specific implementation and is not documented. See https://www.gnu.org/software/gettext/manual/html_node/MO-Files.html
            // for details.

            for ( uint32_t i = 0; i < stringsCount; ++i ) {
                sb.seek( originalStringsTableOffset + i * 8 );

                const uint32_t origStrLen = sb.get32();
                const uint32_t origStrOff = sb.get32();
                if ( sb.fail() ) {
                    ERROR_LOG( "I/O error when parsing " << fileName )
                    return false;
                }

                if ( origStrLen == 0 ) {
                    // This is an empty original string. Skip it.
                    continue;
                }

                sb.seek( origStrOff );

                const std::string_view origStr = sb.getStringView( origStrLen );
                if ( sb.fail() ) {
                    ERROR_LOG( "I/O error when parsing " << fileName )
                    return false;
                }

                sb.seek( translationsTableOffset + i * 8 );

                const uint32_t tranStrLen = sb.get32();
                const uint32_t tranStrOff = sb.get32();
                if ( sb.fail() ) {
                    ERROR_LOG( "I/O error when parsing " << fileName )
                    return false;
                }

                if ( tranStrLen == 0 ) {
                    // This is an empty translation. Skip it.
                    continue;
                }

                sb.seek( tranStrOff );

                const auto [tranBufPtr, tranBufLen] = sb.getRawView( tranStrLen );
                if ( sb.fail() ) {
                    ERROR_LOG( "I/O error when parsing " << fileName )
                    return false;
                }

                static_assert( std::is_same_v<std::remove_const_t<std::remove_reference_t<decltype( *tranBufPtr )>>, unsigned char> );

                if ( const auto [dummy, inserted]
                     = _translations.try_emplace( crc32b( origStr ), StringSplit( { reinterpret_cast<const char *>( tranBufPtr ), tranBufLen }, '\0' ) );
                     !inserted ) {
                    ERROR_LOG( "Hash collision detected for string \"" << origStr << "\"" )
                }
            }

            if ( _translations.empty() ) {
                ERROR_LOG( "There are no translated strings in " << fileName )
                return false;
            }

            _locale = langToLocale( langName );

            // The empty line in the MO file goes first (since the original lines in it are sorted in increasing lexicographical order),
            // and its "translation" contains service information such as the encoding used and information about the plural form. See
            // https://www.gnu.org/software/gettext/manual/html_node/MO-Files.html for details.
            //
            // TODO: parse the plural form information and use it in the code.
            {
                sb.seek( translationsTableOffset );

                const uint32_t len = sb.get32();
                const uint32_t off = sb.get32();
                if ( sb.fail() ) {
                    ERROR_LOG( "I/O error when parsing " << fileName )
                    return false;
                }

                sb.seek( off );

                const std::string_view str = sb.getStringView( len );
                if ( sb.fail() ) {
                    ERROR_LOG( "I/O error when parsing " << fileName )
                    return false;
                }

                for ( const std::string & hdr : StringSplit( str, '\n' ) ) {
                    if ( getCharsetFromHeader( hdr, _encoding ) ) {
                        break;
                    }
                }
            }

            _isValid = true;

            return true;
        }

        LocaleType getLocale() const
        {
            return _locale;
        }

        bool isEncoding( const std::string_view encoding ) const
        {
            return ( _encoding == encoding );
        }

        bool isValid() const
        {
            return _isValid;
        }

    private:
        LocaleType _locale{ LocaleType::LOCALE_EN };
        std::unordered_map<uint32_t, std::vector<std::string>> _translations;
        std::string _encoding;
        bool _isValid{ false };
    };

    MOFile * current = nullptr;
    std::map<std::string, MOFile, std::less<>> cache;
}

std::pair<bool, bool> Translation::setLanguage( const std::string_view langName )
{
    assert( !langName.empty() );

    if ( const auto iter = cache.find( langName ); iter != cache.end() ) {
        MOFile & item = iter->second;

        if ( item.isValid() ) {
            current = &item;
        }

        return { true, item.isValid() };
    }

    return { false, false };
}

bool Translation::setLanguage( const std::string & langName, const std::string_view fileName )
{
    assert( !langName.empty() );

    const auto [cacheIter, inserted] = cache.try_emplace( langName );
    MOFile & item = cacheIter->second;

    if ( !inserted ) {
        if ( item.isValid() ) {
            current = &item;
        }

        return item.isValid();
    }

    if ( fileName.empty() || !item.load( langName, std::string{ fileName } ) ) {
        assert( !item.isValid() );

        return false;
    }

    assert( item.isValid() );

    current = &item;

    return true;
}

void Translation::reset()
{
    current = nullptr;
}

const char * Translation::gettext( const std::string & str )
{
    return current ? current->ngettext( str.c_str(), 0 ) : stripContext( str.c_str() );
}

const char * Translation::gettext( const char * str )
{
    return current ? current->ngettext( str, 0 ) : stripContext( str );
}

const char * Translation::ngettext( const char * str, const char * plural, size_t n )
{
    if ( current )
        switch ( current->getLocale() ) {
        case LocaleType::LOCALE_AF:
        case LocaleType::LOCALE_BG:
        case LocaleType::LOCALE_DE:
        case LocaleType::LOCALE_DK:
        case LocaleType::LOCALE_ES:
        case LocaleType::LOCALE_ET:
        case LocaleType::LOCALE_EU:
        case LocaleType::LOCALE_FI:
        case LocaleType::LOCALE_GL:
        case LocaleType::LOCALE_HE:
        case LocaleType::LOCALE_HU:
        case LocaleType::LOCALE_ID:
        case LocaleType::LOCALE_IT:
        case LocaleType::LOCALE_LA:
        case LocaleType::LOCALE_NB:
        case LocaleType::LOCALE_NL:
        case LocaleType::LOCALE_SV:
        case LocaleType::LOCALE_TR:
            return current->ngettext( str, ( n != 1 ) );
        case LocaleType::LOCALE_EL:
        case LocaleType::LOCALE_FR:
        case LocaleType::LOCALE_PT:
            return current->ngettext( str, ( n > 1 ) );
        case LocaleType::LOCALE_AR:
            return current->ngettext( str, ( n == 0 ? 0 : n == 1 ? 1 : n == 2 ? 2 : n % 100 >= 3 && n % 100 <= 10 ? 3 : n % 100 >= 11 && n % 100 <= 99 ? 4 : 5 ) );
        case LocaleType::LOCALE_RO:
            return current->ngettext( str, ( n == 1 ? 0 : n == 0 || ( n != 1 && n % 100 >= 1 && n % 100 <= 19 ) ? 1 : 2 ) );
        case LocaleType::LOCALE_SL:
            return current->ngettext( str, ( n % 100 == 1 ? 0 : n % 100 == 2 ? 1 : n % 100 == 3 || n % 100 == 4 ? 2 : 3 ) );
        case LocaleType::LOCALE_SR:
            return current->ngettext( str, ( n == 1 ? 3 : n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && n % 10 <= 4 && ( n % 100 < 10 || n % 100 >= 20 ) ? 1 : 2 ) );
        case LocaleType::LOCALE_CS:
        case LocaleType::LOCALE_SK:
            return current->ngettext( str, ( ( n == 1 ) ? 0 : ( n >= 2 && n <= 4 ) ? 1 : 2 ) );
        case LocaleType::LOCALE_HR:
        case LocaleType::LOCALE_LV:
        case LocaleType::LOCALE_RU:
            return current->ngettext( str, ( n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && n % 10 <= 4 && ( n % 100 < 10 || n % 100 >= 20 ) ? 1 : 2 ) );
        case LocaleType::LOCALE_LT:
            return current->ngettext( str, ( n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && ( n % 100 < 10 || n % 100 >= 20 ) ? 1 : 2 ) );
        case LocaleType::LOCALE_MK:
            return current->ngettext( str, ( n == 1 || n % 10 == 1 ? 0 : 1 ) );
        case LocaleType::LOCALE_PL:
            return current->ngettext( str, ( n == 1 ? 0 : n % 10 >= 2 && n % 10 <= 4 && ( n % 100 < 10 || n % 100 >= 20 ) ? 1 : 2 ) );
        case LocaleType::LOCALE_BE:
        case LocaleType::LOCALE_UK:
            return current->ngettext( str, ( n % 10 == 1 && n % 100 != 11 ? 0 : n % 10 >= 2 && n % 10 <= 4 && ( n % 100 < 12 || n % 100 > 14 ) ? 1 : 2 ) );
        default:
            break;
        }

    return stripContext( n == 1 ? str : plural );
}

std::string Translation::StringLower( std::string str )
{
    if ( current ) {
        // With German, lowercasing strings does more harm than good
        if ( current->getLocale() == LocaleType::LOCALE_DE ) {
            return str;
        }

        // For CP1250/1251 codepages a custom lowercase LUT is implemented
        if ( current->isEncoding( "CP1250" ) || current->isEncoding( "CP1251" ) ) {
            std::transform( str.begin(), str.end(), str.begin(), []( const unsigned char c ) { return tolowerLUT[c]; } );
            return str;
        }
    }

    return ::StringLower( str );
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
