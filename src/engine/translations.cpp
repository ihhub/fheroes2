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

#include "translations.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <ostream>
#include <string>
#include <utility>
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
        LOCALE_UK
    };

    struct chunk
    {
        uint32_t offset;
        uint32_t length;

        chunk( const uint32_t off, const uint32_t len )
            : offset( off )
            , length( len )
        {
            // Do nothing.
        }
    };

    uint32_t crc32b( const char * msg )
    {
        uint32_t crc = 0xFFFFFFFF;
        uint32_t index = 0;

        while ( msg[index] ) {
            crc ^= static_cast<uint32_t>( msg[index] );

            for ( int bit = 0; bit < 8; ++bit ) {
                const uint32_t poly = ( crc & 1 ) ? 0xEDB88320 : 0x0;
                crc = ( crc >> 1 ) ^ poly;
            }

            ++index;
        }

        return ~crc;
    }

    std::string getTag( const std::string & str, const std::string & tag, const std::string & sep )
    {
        if ( str.size() > tag.size() && str.rfind( tag, 0 ) == 0 ) {
            const size_t pos = str.find( sep );
            if ( pos != std::string::npos ) {
                return str.substr( pos + sep.size() );
            }
        }

        return {};
    }

    const char * stripContext( const char * str )
    {
        const char * pos = std::strchr( str, contextSeparator );

        return pos ? ++pos : str;
    }

    struct mofile
    {
        // TODO: plural forms are not in use: Plural-Forms.
        LocaleType locale{ LocaleType::LOCALE_EN };
        StreamBuf buf;
        std::map<uint32_t, chunk> hash_offsets;
        std::string domain;
        std::string encoding;

        const char * ngettext( const char * str, size_t plural )
        {
            std::map<uint32_t, chunk>::const_iterator it = hash_offsets.find( crc32b( str ) );
            if ( it == hash_offsets.end() )
                return stripContext( str );

            buf.seek( ( *it ).second.offset );
            const uint8_t * ptr = buf.data();

            while ( plural > 0 ) {
                while ( *ptr ) {
                    ++ptr;
                }

                --plural;
                ++ptr;
            }

            return reinterpret_cast<const char *>( ptr );
        }

        bool open( const std::string & file )
        {
            StreamFile sf;
            if ( !sf.open( file, "rb" ) ) {
                return false;
            }

            const size_t fileSize = sf.size();
            uint32_t magicNumber = 0;
            sf >> magicNumber;

            if ( magicNumber != 0x950412de ) {
                ERROR_LOG( "Incorrect magic number " << GetHexString( magicNumber ) << " for " << file )
                return false;
            }

            uint16_t majorVersion;
            uint16_t minorVersion;
            sf >> majorVersion >> minorVersion;

            if ( 0 != majorVersion ) {
                ERROR_LOG( "Incorrect major version " << GetHexString( majorVersion, 4 ) << " for " << file )
                return false;
            }

            uint32_t originalOffset = 0;
            uint32_t translationOffset = 0;
            uint32_t stringCount = 0;

            // TODO: validate these values.
            uint32_t hashSize = 0;
            uint32_t hashOffset = 0;

            sf >> stringCount >> originalOffset >> translationOffset >> hashSize >> hashOffset;

            sf.seek( 0 );
            buf = sf.toStreamBuf( fileSize );
            sf.close();

            // Parse encoding.
            if ( stringCount > 0 ) {
                buf.seek( translationOffset );
                const uint32_t length2 = buf.get32();
                const uint32_t offset2 = buf.get32();

                buf.seek( offset2 );
                const std::vector<std::string> tags = StringSplit( buf.toString( length2 ), '\n' );

                for ( const std::string & tag : tags ) {
                    if ( encoding.empty() ) {
                        encoding = getTag( tag, "Content-Type", "charset=" );
                    }
                    // TODO: parse plural form information here and use it in the code.
                }
            }

            uint32_t totalTranslationStrings = stringCount;

            for ( uint32_t index = 0; index < stringCount; ++index ) {
                buf.seek( originalOffset + index * 8 );

                const uint32_t length1 = buf.get32();
                if ( length1 == 0 ) {
                    // This is an empty original text. Skip it.
                    --totalTranslationStrings;
                    continue;
                }

                const uint32_t offset1 = buf.get32();
                buf.seek( offset1 );
                const std::string msg1 = buf.toString( length1 );

                const uint32_t crc = crc32b( msg1.data() );
                buf.seek( translationOffset + index * 8 );

                const uint32_t length2 = buf.get32();
                if ( length2 == 0 ) {
                    // This is an empty translation. Skip it.
                    --totalTranslationStrings;
                    continue;
                }

                const uint32_t offset2 = buf.get32();

                const auto [dummy, inserted] = hash_offsets.try_emplace( crc, chunk{ offset2, length2 } );
                if ( !inserted ) {
                    ERROR_LOG( "Clashing hash value for: " << msg1 )
                }
            }

            return ( totalTranslationStrings > 0 );
        }
    };

    mofile * current = nullptr;
    std::map<std::string, mofile> domains;
}

namespace Translation
{
    bool bindDomain( const char * domain, const char * file )
    {
        std::string str( domain );

        // Search for already loaded domain or load from file
        std::map<std::string, mofile>::iterator it = domains.find( str );
        if ( it != domains.end() ) {
            current = &( *it ).second;
            return true;
        }
        if ( !domains[str].open( file ) )
            return false;

        current = &domains[str];

        // Update locale
        current->domain = str;
        if ( str == "af" || str == "afrikaans" )
            current->locale = LocaleType::LOCALE_AF;
        else if ( str == "ar" || str == "arabic" )
            current->locale = LocaleType::LOCALE_AR;
        else if ( str == "be" || str == "belarusian" )
            current->locale = LocaleType::LOCALE_BE;
        else if ( str == "bg" || str == "bulgarian" )
            current->locale = LocaleType::LOCALE_BG;
        else if ( str == "ca" || str == "catalan" )
            current->locale = LocaleType::LOCALE_CA;
        else if ( str == "dk" || str == "danish" )
            current->locale = LocaleType::LOCALE_DK;
        else if ( str == "de" || str == "german" )
            current->locale = LocaleType::LOCALE_DE;
        else if ( str == "el" || str == "greek" )
            current->locale = LocaleType::LOCALE_EL;
        else if ( str == "es" || str == "spanish" )
            current->locale = LocaleType::LOCALE_ES;
        else if ( str == "et" || str == "estonian" )
            current->locale = LocaleType::LOCALE_ET;
        else if ( str == "eu" || str == "basque" )
            current->locale = LocaleType::LOCALE_EU;
        else if ( str == "fi" || str == "finnish" )
            current->locale = LocaleType::LOCALE_FI;
        else if ( str == "fr" || str == "french" )
            current->locale = LocaleType::LOCALE_FR;
        else if ( str == "gl" || str == "galician" )
            current->locale = LocaleType::LOCALE_GL;
        else if ( str == "he" || str == "hebrew" )
            current->locale = LocaleType::LOCALE_HE;
        else if ( str == "hr" || str == "croatian" )
            current->locale = LocaleType::LOCALE_HR;
        else if ( str == "hu" || str == "hungarian" )
            current->locale = LocaleType::LOCALE_HU;
        else if ( str == "id" || str == "indonesian" )
            current->locale = LocaleType::LOCALE_ID;
        else if ( str == "it" || str == "italian" )
            current->locale = LocaleType::LOCALE_IT;
        else if ( str == "la" || str == "latin" )
            current->locale = LocaleType::LOCALE_LA;
        else if ( str == "lt" || str == "lithuanian" )
            current->locale = LocaleType::LOCALE_LT;
        else if ( str == "lv" || str == "latvian" )
            current->locale = LocaleType::LOCALE_LV;
        else if ( str == "mk" || str == "macedonia" )
            current->locale = LocaleType::LOCALE_MK;
        else if ( str == "nb" || str == "norwegian" )
            current->locale = LocaleType::LOCALE_NB;
        else if ( str == "nl" || str == "dutch" )
            current->locale = LocaleType::LOCALE_NL;
        else if ( str == "pl" || str == "polish" )
            current->locale = LocaleType::LOCALE_PL;
        else if ( str == "pt" || str == "portuguese" )
            current->locale = LocaleType::LOCALE_PT;
        else if ( str == "ro" || str == "romanian" )
            current->locale = LocaleType::LOCALE_RO;
        else if ( str == "ru" || str == "russian" )
            current->locale = LocaleType::LOCALE_RU;
        else if ( str == "sk" || str == "slovak" )
            current->locale = LocaleType::LOCALE_SK;
        else if ( str == "sl" || str == "slovenian" )
            current->locale = LocaleType::LOCALE_SL;
        else if ( str == "sr" || str == "serbian" )
            current->locale = LocaleType::LOCALE_SR;
        else if ( str == "sv" || str == "swedish" )
            current->locale = LocaleType::LOCALE_SV;
        else if ( str == "tr" || str == "turkish" )
            current->locale = LocaleType::LOCALE_TR;
        else if ( str == "uk" || str == "ukrainian" )
            current->locale = LocaleType::LOCALE_UK;
        return true;
    }

    void reset()
    {
        current = nullptr;
    }

    const char * gettext( const std::string & str )
    {
        const char * data = str.data();
        return current ? current->ngettext( data, 0 ) : stripContext( data );
    }

    const char * gettext( const char * str )
    {
        return current ? current->ngettext( str, 0 ) : stripContext( str );
    }

    const char * ngettext( const char * str, const char * plural, size_t n )
    {
        if ( current )
            switch ( current->locale ) {
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
                return current->ngettext( str, ( n == 1                                                            ? 3
                                                 : n % 10 == 1 && n % 100 != 11                                    ? 0
                                                 : n % 10 >= 2 && n % 10 <= 4 && ( n % 100 < 10 || n % 100 >= 20 ) ? 1
                                                                                                                   : 2 ) );
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

    std::string StringLower( std::string str )
    {
        if ( current ) {
            // With German, lowercasing strings does more harm than good
            if ( current->locale == LocaleType::LOCALE_DE )
                return str;

            // For CP1250/1251 codepages a custom lowercase LUT is implemented
            if ( ( current->encoding == "CP1250" ) || ( current->encoding == "CP1251" ) ) {
                std::transform( str.begin(), str.end(), str.begin(), []( const unsigned char c ) { return tolowerLUT[c]; } );
                return str;
            }
        }
        return ::StringLower( str );
    }
}
