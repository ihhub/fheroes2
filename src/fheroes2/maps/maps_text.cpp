/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "maps_text.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#if defined( _WIN32 )
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined( WITH_ICONV )
#include <iconv.h>
#endif

#include "logging.h"
#include "ui_language.h"

namespace
{
    enum class LegacyChineseEncoding : uint8_t
    {
        GBK,
        Big5
    };

    bool hasHighBitByte( const std::string_view text )
    {
        return std::any_of( text.begin(), text.end(), []( const char character ) { return static_cast<uint8_t>( character ) >= 0x80; } );
    }

    bool isValidUtf8( const std::string_view text, bool & hasNonAscii )
    {
        hasNonAscii = false;

        for ( size_t offset = 0; offset < text.size(); ) {
            const auto firstByte = static_cast<uint8_t>( text[offset] );
            uint32_t codePoint = 0;
            size_t byteCount = 1;

            if ( firstByte < 0x80 ) {
                ++offset;
                continue;
            }

            hasNonAscii = true;

            if ( ( firstByte & 0xE0 ) == 0xC0 && offset + 1 < text.size() ) {
                const auto secondByte = static_cast<uint8_t>( text[offset + 1] );
                if ( ( secondByte & 0xC0 ) != 0x80 ) {
                    return false;
                }

                codePoint = ( static_cast<uint32_t>( firstByte & 0x1F ) << 6 ) | static_cast<uint32_t>( secondByte & 0x3F );
                byteCount = 2;
                if ( codePoint < 0x80 ) {
                    return false;
                }
            }
            else if ( ( firstByte & 0xF0 ) == 0xE0 && offset + 2 < text.size() ) {
                const auto secondByte = static_cast<uint8_t>( text[offset + 1] );
                const auto thirdByte = static_cast<uint8_t>( text[offset + 2] );
                if ( ( secondByte & 0xC0 ) != 0x80 || ( thirdByte & 0xC0 ) != 0x80 ) {
                    return false;
                }

                codePoint = ( static_cast<uint32_t>( firstByte & 0x0F ) << 12 ) | ( static_cast<uint32_t>( secondByte & 0x3F ) << 6 )
                            | static_cast<uint32_t>( thirdByte & 0x3F );
                byteCount = 3;
                if ( codePoint < 0x800 || ( codePoint >= 0xD800 && codePoint <= 0xDFFF ) ) {
                    return false;
                }
            }
            else if ( ( firstByte & 0xF8 ) == 0xF0 && offset + 3 < text.size() ) {
                const auto secondByte = static_cast<uint8_t>( text[offset + 1] );
                const auto thirdByte = static_cast<uint8_t>( text[offset + 2] );
                const auto fourthByte = static_cast<uint8_t>( text[offset + 3] );
                if ( ( secondByte & 0xC0 ) != 0x80 || ( thirdByte & 0xC0 ) != 0x80 || ( fourthByte & 0xC0 ) != 0x80 ) {
                    return false;
                }

                codePoint = ( static_cast<uint32_t>( firstByte & 0x07 ) << 18 ) | ( static_cast<uint32_t>( secondByte & 0x3F ) << 12 )
                            | ( static_cast<uint32_t>( thirdByte & 0x3F ) << 6 ) | static_cast<uint32_t>( fourthByte & 0x3F );
                byteCount = 4;
                if ( codePoint < 0x10000 || codePoint > 0x10FFFF ) {
                    return false;
                }
            }
            else {
                return false;
            }

            offset += byteCount;
        }

        return true;
    }

    bool isCjkCodePoint( const uint32_t codePoint )
    {
        return ( codePoint >= 0x3400 && codePoint <= 0x4DBF ) || ( codePoint >= 0x4E00 && codePoint <= 0x9FFF )
               || ( codePoint >= 0xF900 && codePoint <= 0xFAFF ) || ( codePoint >= 0x20000 && codePoint <= 0x2EBEF )
               || ( codePoint >= 0x30000 && codePoint <= 0x323AF );
    }

    bool isCjkPunctuation( const uint32_t codePoint )
    {
        return ( codePoint >= 0x3000 && codePoint <= 0x303F ) || ( codePoint >= 0xFF00 && codePoint <= 0xFFEF );
    }

    struct Utf8Score
    {
        size_t cjkCount{ 0 };
        size_t cjkPunctuationCount{ 0 };
        size_t nonAsciiCount{ 0 };
    };

    Utf8Score getUtf8Score( const std::string_view text )
    {
        Utf8Score score;

        for ( size_t offset = 0; offset < text.size(); ) {
            const auto firstByte = static_cast<uint8_t>( text[offset] );
            uint32_t codePoint = firstByte;
            size_t byteCount = 1;

            if ( firstByte < 0x80 ) {
                ++offset;
                continue;
            }

            if ( ( firstByte & 0xE0 ) == 0xC0 ) {
                codePoint = ( static_cast<uint32_t>( firstByte & 0x1F ) << 6 ) | static_cast<uint32_t>( static_cast<uint8_t>( text[offset + 1] ) & 0x3F );
                byteCount = 2;
            }
            else if ( ( firstByte & 0xF0 ) == 0xE0 ) {
                codePoint = ( static_cast<uint32_t>( firstByte & 0x0F ) << 12 )
                            | ( static_cast<uint32_t>( static_cast<uint8_t>( text[offset + 1] ) & 0x3F ) << 6 )
                            | static_cast<uint32_t>( static_cast<uint8_t>( text[offset + 2] ) & 0x3F );
                byteCount = 3;
            }
            else {
                codePoint = ( static_cast<uint32_t>( firstByte & 0x07 ) << 18 )
                            | ( static_cast<uint32_t>( static_cast<uint8_t>( text[offset + 1] ) & 0x3F ) << 12 )
                            | ( static_cast<uint32_t>( static_cast<uint8_t>( text[offset + 2] ) & 0x3F ) << 6 )
                            | static_cast<uint32_t>( static_cast<uint8_t>( text[offset + 3] ) & 0x3F );
                byteCount = 4;
            }

            ++score.nonAsciiCount;
            if ( isCjkCodePoint( codePoint ) ) {
                ++score.cjkCount;
            }
            else if ( isCjkPunctuation( codePoint ) ) {
                ++score.cjkPunctuationCount;
            }

            offset += byteCount;
        }

        return score;
    }

#if defined( _WIN32 )
    std::optional<std::string> convertToUtf8( const std::string_view text, const LegacyChineseEncoding encoding )
    {
        const UINT codePage = encoding == LegacyChineseEncoding::GBK ? 936 : 950;
        const auto inputSize = static_cast<int>( text.size() );

        const int wideSize = MultiByteToWideChar( codePage, MB_ERR_INVALID_CHARS, text.data(), inputSize, nullptr, 0 );
        if ( wideSize <= 0 ) {
            return std::nullopt;
        }

        std::wstring wideText( static_cast<size_t>( wideSize ), L'\0' );
        if ( MultiByteToWideChar( codePage, MB_ERR_INVALID_CHARS, text.data(), inputSize, wideText.data(), wideSize ) != wideSize ) {
            return std::nullopt;
        }

        const int utf8Size = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, wideText.data(), wideSize, nullptr, 0, nullptr, nullptr );
        if ( utf8Size <= 0 ) {
            return std::nullopt;
        }

        std::string output( static_cast<size_t>( utf8Size ), '\0' );
        if ( WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, wideText.data(), wideSize, output.data(), utf8Size, nullptr, nullptr ) != utf8Size ) {
            return std::nullopt;
        }

        return output;
    }
#elif defined( WITH_ICONV )
    const char * getEncodingName( const LegacyChineseEncoding encoding )
    {
        return encoding == LegacyChineseEncoding::GBK ? "GBK" : "BIG5";
    }

    std::optional<std::string> convertToUtf8( const std::string_view text, const LegacyChineseEncoding encoding )
    {
        iconv_t converter = iconv_open( "UTF-8", getEncodingName( encoding ) );
        if ( converter == reinterpret_cast<iconv_t>( -1 ) ) {
            return std::nullopt;
        }

        std::string output( text.size() * 4 + 4, '\0' );
        char * inputBuffer = const_cast<char *>( text.data() );
        size_t inputBytesLeft = text.size();
        char * outputBuffer = output.data();
        size_t outputBytesLeft = output.size();

        const size_t result = iconv( converter, &inputBuffer, &inputBytesLeft, &outputBuffer, &outputBytesLeft );
        iconv_close( converter );

        if ( result == static_cast<size_t>( -1 ) || inputBytesLeft != 0 ) {
            return std::nullopt;
        }

        output.resize( output.size() - outputBytesLeft );
        return output;
    }
#else
    std::optional<std::string> convertToUtf8( const std::string_view, const LegacyChineseEncoding )
    {
        return std::nullopt;
    }
#endif

    bool shouldAttemptLegacyChineseDecode( const std::optional<fheroes2::SupportedLanguage> language )
    {
        const fheroes2::SupportedLanguage targetLanguage = language.value_or( fheroes2::getCurrentLanguage() );

        return targetLanguage == fheroes2::SupportedLanguage::SimplifiedChinese || targetLanguage == fheroes2::SupportedLanguage::TraditionalChinese;
    }

    bool isLikelyChineseText( const std::string_view text )
    {
        bool hasNonAscii = false;
        if ( !isValidUtf8( text, hasNonAscii ) || !hasNonAscii ) {
            return false;
        }

        const Utf8Score score = getUtf8Score( text );
        return score.cjkCount > 0 && ( score.cjkCount + score.cjkPunctuationCount ) * 2 >= score.nonAsciiCount;
    }

    void logDecodeFailureOnce()
    {
        static bool isLogged = false;
        if ( isLogged ) {
            return;
        }

        DEBUG_LOG( DBG_GAME, DBG_WARN, "Unable to decode a legacy Chinese map text as GBK or Big5. The original bytes will be displayed." )
        isLogged = true;
    }
}

namespace Maps
{
    std::string decodeLegacyChineseTextForDisplay( std::string text, const std::optional<fheroes2::SupportedLanguage> language )
    {
        bool hasNonAscii = false;
        if ( text.empty() || isValidUtf8( text, hasNonAscii ) || !hasHighBitByte( text ) ) {
            return text;
        }

        if ( !shouldAttemptLegacyChineseDecode( language ) ) {
            return text;
        }

        const fheroes2::SupportedLanguage targetLanguage = language.value_or( fheroes2::getCurrentLanguage() );
        const std::array<LegacyChineseEncoding, 2> encodings = targetLanguage == fheroes2::SupportedLanguage::TraditionalChinese
                                                                  ? std::array<LegacyChineseEncoding, 2>{ LegacyChineseEncoding::Big5, LegacyChineseEncoding::GBK }
                                                                  : std::array<LegacyChineseEncoding, 2>{ LegacyChineseEncoding::GBK, LegacyChineseEncoding::Big5 };

        for ( const LegacyChineseEncoding encoding : encodings ) {
            std::optional<std::string> convertedText = convertToUtf8( text, encoding );
            if ( convertedText && isLikelyChineseText( *convertedText ) ) {
                return std::move( *convertedText );
            }
        }

        logDecodeFailureOnce();
        return text;
    }
}
