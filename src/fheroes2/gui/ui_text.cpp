/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2026                                             *
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

#include "ui_text.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cctype>
#include <cstdlib>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#if defined( WITH_CJK_TEXT )
#include <filesystem>
#include <set>
#include <system_error>

#if defined( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#include <SDL_ttf.h>

#if SDL_TTF_VERSION_ATLEAST( 2, 0, 18 )
#define FHEROES2_SDL_TTF_HAS_32BIT_GLYPHS
#endif

#if defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif
#endif

#include "agg_image.h"
#include "game_language.h"
#include "icn.h"
#include "logging.h"
#include "settings.h"
#include "system.h"
#include "ui_language.h"

namespace
{
    const uint8_t hyphenChar{ '-' };

    const uint8_t invalidChar{ '?' };

    const uint8_t cursorChar{ '|' };

    const std::string truncationSymbol( "..." );

    // Returns true if character is a line separator ('\n').
    bool isLineSeparator( const uint8_t character )
    {
        return ( character == '\n' );
    }

    // Returns true if character is a Space character (' ').
    bool isSpaceChar( const uint8_t character )
    {
        return ( character == ' ' );
    }

    const fheroes2::Sprite errorImage;

    constexpr uint32_t unicodeReplacementGlyph{ 0x25A1 };

    struct Utf8Character
    {
        uint32_t codePoint{ 0 };
        size_t byteOffset{ 0 };
        size_t byteCount{ 0 };
    };

    bool isCjkCodePoint( const uint32_t codePoint )
    {
        return ( codePoint >= 0x3400 && codePoint <= 0x4DBF ) || ( codePoint >= 0x4E00 && codePoint <= 0x9FFF )
               || ( codePoint >= 0xF900 && codePoint <= 0xFAFF ) || ( codePoint >= 0x3040 && codePoint <= 0x30FF )
               || ( codePoint >= 0x3100 && codePoint <= 0x312F ) || ( codePoint >= 0x20000 && codePoint <= 0x2A6DF )
               || ( codePoint >= 0x2A700 && codePoint <= 0x2B73F ) || ( codePoint >= 0x2B740 && codePoint <= 0x2B81F )
               || ( codePoint >= 0x2B820 && codePoint <= 0x2EBEF ) || ( codePoint >= 0x2F800 && codePoint <= 0x2FA1F )
               || ( codePoint >= 0x30000 && codePoint <= 0x323AF );
    }

    bool decodeUtf8( const std::string_view text, std::vector<Utf8Character> & output, bool & hasNonAscii )
    {
        output.clear();
        hasNonAscii = false;

        bool isValid = true;

        for ( size_t offset = 0; offset < text.size(); ) {
            const auto firstByte = static_cast<uint8_t>( text[offset] );
            uint32_t codePoint = unicodeReplacementGlyph;
            size_t byteCount = 1;
            bool isSequenceValid = true;

            if ( firstByte < 0x80 ) {
                codePoint = firstByte;
            }
            else if ( ( firstByte & 0xE0 ) == 0xC0 && offset + 1 < text.size() ) {
                const auto secondByte = static_cast<uint8_t>( text[offset + 1] );
                isSequenceValid = ( secondByte & 0xC0 ) == 0x80;
                codePoint = ( static_cast<uint32_t>( firstByte & 0x1F ) << 6 ) | static_cast<uint32_t>( secondByte & 0x3F );
                byteCount = 2;

                if ( codePoint < 0x80 ) {
                    isSequenceValid = false;
                }
            }
            else if ( ( firstByte & 0xF0 ) == 0xE0 && offset + 2 < text.size() ) {
                const auto secondByte = static_cast<uint8_t>( text[offset + 1] );
                const auto thirdByte = static_cast<uint8_t>( text[offset + 2] );
                isSequenceValid = ( ( secondByte & 0xC0 ) == 0x80 ) && ( ( thirdByte & 0xC0 ) == 0x80 );
                codePoint = ( static_cast<uint32_t>( firstByte & 0x0F ) << 12 ) | ( static_cast<uint32_t>( secondByte & 0x3F ) << 6 )
                            | static_cast<uint32_t>( thirdByte & 0x3F );
                byteCount = 3;

                if ( codePoint < 0x800 || ( codePoint >= 0xD800 && codePoint <= 0xDFFF ) ) {
                    isSequenceValid = false;
                }
            }
            else if ( ( firstByte & 0xF8 ) == 0xF0 && offset + 3 < text.size() ) {
                const auto secondByte = static_cast<uint8_t>( text[offset + 1] );
                const auto thirdByte = static_cast<uint8_t>( text[offset + 2] );
                const auto fourthByte = static_cast<uint8_t>( text[offset + 3] );
                isSequenceValid
                    = ( ( secondByte & 0xC0 ) == 0x80 ) && ( ( thirdByte & 0xC0 ) == 0x80 ) && ( ( fourthByte & 0xC0 ) == 0x80 );
                codePoint = ( static_cast<uint32_t>( firstByte & 0x07 ) << 18 ) | ( static_cast<uint32_t>( secondByte & 0x3F ) << 12 )
                            | ( static_cast<uint32_t>( thirdByte & 0x3F ) << 6 ) | static_cast<uint32_t>( fourthByte & 0x3F );
                byteCount = 4;

                if ( codePoint < 0x10000 || codePoint > 0x10FFFF ) {
                    isSequenceValid = false;
                }
            }
            else {
                isSequenceValid = false;
            }

            if ( firstByte >= 0x80 ) {
                hasNonAscii = true;
            }

            if ( !isSequenceValid ) {
                isValid = false;
                codePoint = unicodeReplacementGlyph;
                byteCount = 1;
            }

            output.push_back( { codePoint, offset, byteCount } );
            offset += byteCount;
        }

        return isValid;
    }

    bool isChineseLanguage( const fheroes2::SupportedLanguage language )
    {
        return language == fheroes2::SupportedLanguage::SimplifiedChinese || language == fheroes2::SupportedLanguage::TraditionalChinese;
    }

    bool hasHighBitByte( const std::string_view text )
    {
        return std::any_of( text.begin(), text.end(), []( const char character ) { return static_cast<uint8_t>( character ) >= 0x80; } );
    }

    bool isUnicodeTextCandidate( const std::string_view text, const std::optional<fheroes2::SupportedLanguage> & language )
    {
        if ( text.empty() ) {
            return false;
        }

        // Pure ASCII text (the vast majority of rendered strings) can never be a CJK candidate.
        // Skip the full UTF-8 decode and its allocation in that very common case.
        if ( !hasHighBitByte( text ) ) {
            return false;
        }

        std::vector<Utf8Character> characters;
        bool hasNonAscii = false;
        if ( !decodeUtf8( text, characters, hasNonAscii ) || !hasNonAscii ) {
            return false;
        }

        const fheroes2::SupportedLanguage textLanguage = language.value_or( fheroes2::getCurrentLanguage() );
        if ( isChineseLanguage( textLanguage ) ) {
            return true;
        }

        return std::any_of( characters.begin(), characters.end(), []( const Utf8Character & character ) { return isCjkCodePoint( character.codePoint ); } );
    }

    void removeLastUtf8Character( std::string & text )
    {
        if ( text.empty() ) {
            return;
        }

        size_t newSize = text.size() - 1;
        while ( newSize > 0 && ( static_cast<uint8_t>( text[newSize] ) & 0xC0 ) == 0x80 ) {
            --newSize;
        }

        text.resize( newSize );
    }

#if defined( WITH_CJK_TEXT )
    // Button captions are part of low-resolution assets and need dedicated CJK font tuning.
    bool isButtonFontSize( const fheroes2::FontSize fontSize )
    {
        return fontSize == fheroes2::FontSize::BUTTON_RELEASED || fontSize == fheroes2::FontSize::BUTTON_PRESSED;
    }

    bool isUnicodeLineSeparator( const uint32_t codePoint )
    {
        return codePoint == '\n';
    }

    bool isUnicodeSpace( const uint32_t codePoint )
    {
        return codePoint == ' ' || codePoint == '\t' || codePoint == 0x3000;
    }

    bool isCjkPunctuation( const uint32_t codePoint )
    {
        return ( codePoint >= 0x3000 && codePoint <= 0x303F ) || ( codePoint >= 0xFF00 && codePoint <= 0xFFEF );
    }

    bool isLineStartProhibitedPunctuation( const uint32_t codePoint )
    {
        switch ( codePoint ) {
        case 0x3001: // Ideographic comma.
        case 0x3002: // Ideographic full stop.
        case 0xFF0C: // Fullwidth comma.
        case 0xFF0E: // Fullwidth full stop.
        case 0xFF1A: // Fullwidth colon.
        case 0xFF1B: // Fullwidth semicolon.
        case 0xFF01: // Fullwidth exclamation mark.
        case 0xFF1F: // Fullwidth question mark.
        case 0xFF09: // Fullwidth right parenthesis.
        case 0xFF3D: // Fullwidth right square bracket.
        case 0xFF5D: // Fullwidth right curly bracket.
        case 0x3009: // Right angle bracket.
        case 0x300B: // Right double angle bracket.
        case 0x300D: // Right corner bracket.
        case 0x300F: // Right white corner bracket.
        case 0x3011: // Right black lenticular bracket.
        case 0x3015: // Right tortoise shell bracket.
        case 0x3017: // Right white lenticular bracket.
            return true;
        default:
            return false;
        }
    }

    bool isUnicodeBreakAllowedAfter( const uint32_t currentCodePoint, const uint32_t nextCodePoint )
    {
        if ( isUnicodeSpace( currentCodePoint ) || currentCodePoint == '-' ) {
            return true;
        }

        if ( isCjkCodePoint( currentCodePoint ) || isCjkPunctuation( currentCodePoint ) ) {
            return !isLineStartProhibitedPunctuation( nextCodePoint );
        }

        return false;
    }

    int32_t getFallbackSpaceWidth( const fheroes2::FontSize fontSize )
    {
        switch ( fontSize ) {
        case fheroes2::FontSize::SMALL:
            return 4;
        case fheroes2::FontSize::NORMAL:
            return 6;
        case fheroes2::FontSize::LARGE:
        case fheroes2::FontSize::BUTTON_RELEASED:
        case fheroes2::FontSize::BUTTON_PRESSED:
            return 8;
        default:
            assert( 0 );
            break;
        }

        return 0;
    }

    constexpr uint32_t unicodeQuestionMark{ '?' };

    struct GlyphCacheKey
    {
        fheroes2::FontSize size{ fheroes2::FontSize::NORMAL };
        fheroes2::FontColor color{ fheroes2::FontColor::WHITE };
        size_t fontIndex{ 0 };
        uint32_t codePoint{ 0 };
        uint32_t renderableCodePoint{ 0 };

        bool operator<( const GlyphCacheKey & other ) const
        {
            if ( size != other.size ) {
                return static_cast<uint8_t>( size ) < static_cast<uint8_t>( other.size );
            }

            if ( color != other.color ) {
                return static_cast<uint8_t>( color ) < static_cast<uint8_t>( other.color );
            }

            if ( fontIndex != other.fontIndex ) {
                return fontIndex < other.fontIndex;
            }

            if ( codePoint != other.codePoint ) {
                return codePoint < other.codePoint;
            }

            return renderableCodePoint < other.renderableCodePoint;
        }
    };

    struct FontCacheKey
    {
        size_t fontIndex{ 0 };
        fheroes2::FontSize size{ fheroes2::FontSize::NORMAL };

        bool operator<( const FontCacheKey & other ) const
        {
            if ( fontIndex != other.fontIndex ) {
                return fontIndex < other.fontIndex;
            }

            return static_cast<uint8_t>( size ) < static_cast<uint8_t>( other.size );
        }
    };

    struct RenderableGlyph
    {
        size_t fontIndex{ static_cast<size_t>( -1 ) };
        uint32_t codePoint{ 0 };
        bool isValid{ false };
    };

    struct CjkGlyph
    {
        fheroes2::Sprite sprite;
        int32_t advance{ 0 };
    };

    struct TtfFontDeleter
    {
        void operator()( TTF_Font * font ) const
        {
            if ( font != nullptr ) {
                TTF_CloseFont( font );
            }
        }
    };

    std::string toLowerAscii( std::string str )
    {
        std::transform( str.begin(), str.end(), str.begin(), []( const unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
        return str;
    }

    bool hasFontExtension( const std::string & fileName )
    {
        const std::string lowerName = toLowerAscii( fileName );

        return lowerName.size() > 4
               && ( lowerName.compare( lowerName.size() - 4, 4, ".ttf" ) == 0 || lowerName.compare( lowerName.size() - 4, 4, ".ttc" ) == 0
                    || lowerName.compare( lowerName.size() - 4, 4, ".otf" ) == 0 );
    }

    int32_t getCjkFontNamePriority( const std::string & fileName )
    {
        const std::string lowerName = toLowerAscii( fileName );

        if ( lowerName.find( "serif" ) != std::string::npos ) {
            return 3;
        }

        if ( lowerName.find( "noto" ) != std::string::npos || lowerName.find( "sourcehan" ) != std::string::npos
             || lowerName.find( "source han" ) != std::string::npos || lowerName.find( "wqy" ) != std::string::npos
             || lowerName.find( "wenquanyi" ) != std::string::npos ) {
            return 0;
        }

        if ( lowerName.find( "msjh" ) != std::string::npos || lowerName.find( "msyh" ) != std::string::npos || lowerName.find( "simhei" ) != std::string::npos
             || lowerName.find( "deng" ) != std::string::npos || lowerName.find( "pingfang" ) != std::string::npos
             || lowerName.find( "hiragino" ) != std::string::npos || lowerName.find( "heiti" ) != std::string::npos ) {
            return 1;
        }

        if ( lowerName.find( "mingliu" ) != std::string::npos || lowerName.find( "simsun" ) != std::string::npos ) {
            return 2;
        }

        return 5;
    }

    // Button labels have limited space, so prefer fonts that fit compact UI controls.
    int32_t getCjkButtonFontNamePriority( const std::string & fileName )
    {
        const std::string lowerName = toLowerAscii( fileName );

        if ( lowerName.find( "kaiti" ) != std::string::npos || lowerName.find( "stkaiti" ) != std::string::npos || lowerName.find( "ukai" ) != std::string::npos
             || lowerName.find( " kai" ) != std::string::npos || lowerName.find( "-kai" ) != std::string::npos ) {
            return 0;
        }

        if ( lowerName.find( "mingliu" ) != std::string::npos || lowerName.find( "simsun" ) != std::string::npos || lowerName.find( "songti" ) != std::string::npos
             || lowerName.find( "sourcehanserif" ) != std::string::npos || lowerName.find( "source han serif" ) != std::string::npos
             || lowerName.find( "notoserif" ) != std::string::npos || lowerName.find( "noto serif" ) != std::string::npos
             || lowerName.find( "uming" ) != std::string::npos || lowerName.find( "serif" ) != std::string::npos ) {
            return 1;
        }

        return 2;
    }

    bool isPreferredCjkFontName( const std::string & fileName )
    {
        return getCjkFontNamePriority( fileName ) < 5;
    }

    const std::vector<std::string> & getKnownCjkFontFileNames()
    {
        static const std::vector<std::string> fontNames{ "NotoSansCJK-Regular.ttc",     "NotoSansCJKtc-Regular.otf", "NotoSansTC-Regular.otf",
                                                         "NotoSansSC-Regular.otf",      "SourceHanSans-Regular.ttc", "SourceHanSansTC-Regular.otf",
                                                         "SourceHanSansCN-Regular.otf", "wqy-microhei.ttc",          "wqy-zenhei.ttc",
                                                         "msjh.ttc",                    "msyh.ttc",                  "simhei.ttf",
                                                         "mingliu.ttc",                 "simsun.ttc",                "Songti.ttc",
                                                         "Kaiti.ttc",                   "Hiragino Sans GB.ttc",      "PingFang.ttc",
                                                         "STHeiti Medium.ttc",          "STHeiti Light.ttc",
                                                         "Arial Unicode.ttf",           "NotoSerifCJK-Regular.ttc",  "NotoSerifTC-Regular.otf",
                                                         "SourceHanSerifTC-Regular.otf" };

        return fontNames;
    }

    void appendFontPathIfExists( std::vector<std::string> & fontPaths, const std::string & path )
    {
        if ( path.empty() || !System::IsFile( path ) ) {
            return;
        }

        if ( std::find( fontPaths.begin(), fontPaths.end(), path ) == fontPaths.end() ) {
            fontPaths.emplace_back( path );
        }
    }

    void appendFontPathsByPriority( std::vector<std::string> & fontPaths, std::vector<std::string> candidatePaths )
    {
        std::sort( candidatePaths.begin(), candidatePaths.end(), []( const std::string & lhs, const std::string & rhs ) {
            const int32_t leftPriority = getCjkFontNamePriority( lhs );
            const int32_t rightPriority = getCjkFontNamePriority( rhs );
            if ( leftPriority != rightPriority ) {
                return leftPriority < rightPriority;
            }

            return toLowerAscii( lhs ) < toLowerAscii( rhs );
        } );

        for ( const std::string & candidatePath : candidatePaths ) {
            appendFontPathIfExists( fontPaths, candidatePath );
        }
    }

    void appendKnownFontsInDirectory( std::vector<std::string> & fontPaths, const std::string & directory )
    {
        if ( !System::IsDirectory( directory ) ) {
            return;
        }

        for ( const std::string & fontName : getKnownCjkFontFileNames() ) {
            appendFontPathIfExists( fontPaths, System::concatPath( directory, fontName ) );
        }
    }

    void appendFontsInDirectory( std::vector<std::string> & fontPaths, const std::string & directory, const bool requirePreferredName )
    {
        if ( !System::IsDirectory( directory ) ) {
            return;
        }

        std::vector<std::string> candidatePaths;
        std::error_code ec;
        for ( const std::filesystem::directory_entry & entry : std::filesystem::directory_iterator( directory, ec ) ) {
            if ( !entry.is_regular_file( ec ) ) {
                continue;
            }

            const std::string fileName = System::fsPathToString( entry.path().filename() );
            if ( !hasFontExtension( fileName ) || ( requirePreferredName && !isPreferredCjkFontName( fileName ) ) ) {
                continue;
            }

            candidatePaths.emplace_back( System::fsPathToString( entry.path() ) );
        }

        appendFontPathsByPriority( fontPaths, std::move( candidatePaths ) );
    }

    void appendPortableCjkFontPaths( std::vector<std::string> & fontPaths )
    {
        for ( const std::string & rootDir : Settings::GetRootDirs() ) {
            for ( const std::string & fontDirectoryName : { std::string( "files/fonts" ), std::string( "fonts" ), std::string( "Fonts" ) } ) {
                const std::string fontDirectory = System::concatPath( rootDir, fontDirectoryName );

                appendKnownFontsInDirectory( fontPaths, fontDirectory );
                appendFontsInDirectory( fontPaths, fontDirectory, false );
            }
        }
    }

    std::vector<std::string> findCjkFontPaths()
    {
        std::vector<std::string> fontPaths;

        const std::string & configuredFontPath = Settings::Get().getCjkFontPath();
        if ( !configuredFontPath.empty() ) {
            if ( System::IsFile( configuredFontPath ) ) {
                appendFontPathIfExists( fontPaths, configuredFontPath );
            }
            else {
                ERROR_LOG( "Configured CJK font file was not found: " << configuredFontPath )
            }
        }

        appendPortableCjkFontPaths( fontPaths );

#if defined( _WIN32 )
        for ( const std::string & fontPath : { std::string( "C:/Windows/Fonts/msjh.ttc" ), std::string( "C:/Windows/Fonts/msjhbd.ttc" ),
                                               std::string( "C:/Windows/Fonts/msyh.ttc" ), std::string( "C:/Windows/Fonts/msyhbd.ttc" ),
                                               std::string( "C:/Windows/Fonts/simhei.ttf" ), std::string( "C:/Windows/Fonts/mingliu.ttc" ),
                                               std::string( "C:/Windows/Fonts/simsun.ttc" ) } ) {
            appendFontPathIfExists( fontPaths, fontPath );
        }
#elif defined( __APPLE__ ) && !defined( __IPHONEOS__ )
        for ( const std::string & fontPath : { std::string( "/System/Library/Fonts/Hiragino Sans GB.ttc" ),
                                               std::string( "/System/Library/Fonts/Supplemental/Hiragino Sans GB.ttc" ),
                                               std::string( "/System/Library/Fonts/STHeiti Light.ttc" ),
                                               std::string( "/System/Library/Fonts/Supplemental/STHeiti Light.ttc" ),
                                               std::string( "/System/Library/Fonts/Supplemental/Songti.ttc" ),
                                               std::string( "/System/Library/Fonts/Supplemental/Kaiti.ttc" ),
                                               std::string( "/System/Library/Fonts/Supplemental/Arial Unicode.ttf" ),
                                               std::string( "/System/Library/Fonts/PingFang.ttc" ) } ) {
            appendFontPathIfExists( fontPaths, fontPath );
        }
#elif defined( __linux__ ) && !defined( ANDROID )
        for ( const std::string & fontPath :
              { std::string( "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc" ),
                std::string( "/usr/share/fonts/opentype/noto/NotoSansCJKtc-Regular.otf" ),
                std::string( "/usr/share/fonts/opentype/noto/NotoSansTC-Regular.otf" ),
                std::string( "/usr/share/fonts/opentype/noto/NotoSansSC-Regular.otf" ),
                std::string( "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc" ),
                std::string( "/usr/share/fonts/opentype/source-han-sans/SourceHanSans-Regular.ttc" ),
                std::string( "/usr/share/fonts/opentype/source-han-sans/SourceHanSansTC-Regular.otf" ),
                std::string( "/usr/share/fonts/opentype/source-han-sans/SourceHanSansCN-Regular.otf" ),
                std::string( "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc" ),
                std::string( "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc" ),
                std::string( "/usr/share/fonts/wenquanyi/wqy-microhei/wqy-microhei.ttc" ),
                std::string( "/usr/share/fonts/wenquanyi/wqy-zenhei/wqy-zenhei.ttc" ) } ) {
            appendFontPathIfExists( fontPaths, fontPath );
        }

        auto appendPreferredFontsRecursively = []( std::vector<std::string> & fontPaths, const std::string & directory ) {
            if ( !System::IsDirectory( directory ) ) {
                return;
            }

            std::vector<std::string> candidatePaths;
            std::error_code ec;
            for ( const std::filesystem::directory_entry & entry : std::filesystem::recursive_directory_iterator( directory, ec ) ) {
                if ( !entry.is_regular_file( ec ) ) {
                    continue;
                }

                const std::string fileName = System::fsPathToString( entry.path().filename() );
                if ( hasFontExtension( fileName ) && isPreferredCjkFontName( fileName ) ) {
                    candidatePaths.emplace_back( System::fsPathToString( entry.path() ) );
                }
            }

            appendFontPathsByPriority( fontPaths, std::move( candidatePaths ) );
        };

        std::vector<std::string> linuxFontDirectories{ "/usr/share/fonts", "/usr/local/share/fonts" };
        if ( const char * homePath = getenv( "HOME" ); homePath != nullptr ) {
            linuxFontDirectories.emplace_back( System::concatPath( System::concatPath( homePath, ".local" ), "share/fonts" ) );
        }

        for ( const std::string & fontDirectory : linuxFontDirectories ) {
            appendPreferredFontsRecursively( fontPaths, fontDirectory );
        }
#endif

        return fontPaths;
    }

    // Map game font slots to CJK TTF pixel sizes so CJK text fits the low-resolution UI.
    int32_t getTtfPixelSize( const fheroes2::FontSize fontSize )
    {
        switch ( fontSize ) {
        case fheroes2::FontSize::SMALL:
            return 10;
        case fheroes2::FontSize::NORMAL:
            return 13;
        case fheroes2::FontSize::LARGE:
            return 24;
        case fheroes2::FontSize::BUTTON_RELEASED:
        case fheroes2::FontSize::BUTTON_PRESSED:
            return 14;
        default:
            assert( 0 );
            break;
        }

        return 16;
    }

    uint8_t getCjkGlyphOutlineColor( const fheroes2::FontType fontType )
    {
        if ( fontType.size == fheroes2::FontSize::BUTTON_RELEASED || fontType.size == fheroes2::FontSize::BUTTON_PRESSED ) {
            return 10;
        }

        return 0;
    }

    // Non-button white CJK glyphs keep the original white-to-gray brightness ramp.
    uint8_t getWhiteCjkGlyphColor( const uint8_t alpha )
    {
        if ( alpha > 224 ) {
            return 10;
        }
        if ( alpha > 184 ) {
            return 12;
        }
        if ( alpha > 144 ) {
            return 14;
        }
        if ( alpha > 104 ) {
            return 16;
        }

        return 21;
    }

    // Non-button yellow CJK glyphs use the main yellow and golden palette range.
    uint8_t getYellowCjkGlyphColor( const uint8_t alpha )
    {
        if ( alpha > 224 ) {
            return 219;
        }
        if ( alpha > 184 ) {
            return 115;
        }
        if ( alpha > 144 ) {
            return 117;
        }
        if ( alpha > 104 ) {
            return 118;
        }

        return 120;
    }

    // Non-button gray CJK glyphs keep the disabled-state look without near-black edges.
    uint8_t getGrayCjkGlyphColor( const uint8_t alpha )
    {
        if ( alpha > 224 ) {
            return 17;
        }
        if ( alpha > 184 ) {
            return 19;
        }
        if ( alpha > 144 ) {
            return 21;
        }
        if ( alpha > 104 ) {
            return 23;
        }

        return 25;
    }

    // Button glyphs need harder pixel edges, so filter more TTF antialiasing.
    uint8_t getCjkGlyphAlphaThreshold( const fheroes2::FontType fontType )
    {
        return isButtonFontSize( fontType.size ) ? 128 : 72;
    }

    // CJK button captions keep a small extra advance to avoid cramped horizontal text.
    int32_t getCjkGlyphExtraAdvance( const fheroes2::FontType fontType, const uint32_t codePoint )
    {
        return isButtonFontSize( fontType.size ) && isCjkCodePoint( codePoint ) ? 2 : 0;
    }

    // Map SDL_ttf antialiasing alpha to the game palette while preserving button colors.
    uint8_t getCjkGlyphColor( const fheroes2::FontType fontType, const uint8_t alpha )
    {
        if ( fontType.size == fheroes2::FontSize::BUTTON_RELEASED ) {
            return fontType.color == fheroes2::FontColor::GRAY ? 30 : 56;
        }

        if ( fontType.size == fheroes2::FontSize::BUTTON_PRESSED ) {
            return fontType.color == fheroes2::FontColor::GRAY ? 36 : 62;
        }

        switch ( fontType.color ) {
        case fheroes2::FontColor::YELLOW:
        case fheroes2::FontColor::GOLDEN_GRADIENT:
            return getYellowCjkGlyphColor( alpha );
        case fheroes2::FontColor::GRAY:
            return getGrayCjkGlyphColor( alpha );
        case fheroes2::FontColor::WHITE:
        case fheroes2::FontColor::SILVER_GRADIENT:
            return getWhiteCjkGlyphColor( alpha );
        default:
            assert( 0 );
            break;
        }

        return 36;
    }

    uint8_t getSurfaceAlpha( const SDL_Surface & surface, const int32_t x, const int32_t y )
    {
        // Glyphs are always produced by TTF_RenderGlyph(32)_Blended, which yields a 32-bit ARGB surface.
        assert( surface.format->BytesPerPixel == 4 );

        const auto * pixel = static_cast<const uint8_t *>( surface.pixels ) + y * surface.pitch + x * surface.format->BytesPerPixel;
        const uint32_t pixelValue = *reinterpret_cast<const uint32_t *>( pixel );

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        uint8_t alpha = 0;
        SDL_GetRGBA( pixelValue, surface.format, &red, &green, &blue, &alpha );

        return alpha;
    }

    // Some CJK glyph bounding boxes sit low, so use a stable anchor for vertical placement.
    int32_t getCjkGlyphVerticalOffset( TTF_Font * font, const uint32_t codePoint, const int32_t maxY, const int32_t paddingTop )
    {
        int32_t anchorMaxY = maxY;
        if ( isCjkCodePoint( codePoint ) || isCjkPunctuation( codePoint ) ) {
            anchorMaxY = std::max( anchorMaxY, TTF_FontAscent( font ) - 2 );
        }

        return TTF_FontAscent( font ) - anchorMaxY - paddingTop;
    }

    void addCjkButtonOffsetContour( fheroes2::Sprite & input, const fheroes2::Point & contourOffset, const uint8_t colorId )
    {
        if ( input.empty() || input.singleLayer() || contourOffset.x > 0 || contourOffset.y < 0 || ( -contourOffset.x >= input.width() )
             || ( contourOffset.y >= input.height() ) ) {
            return;
        }

        fheroes2::Sprite output = input;

        const int32_t imageWidth = output.width();
        const int32_t width = imageWidth + contourOffset.x;
        const int32_t height = output.height() - contourOffset.y;

        const int32_t offsetY = imageWidth * contourOffset.y;
        uint8_t * imageOutY = output.image() + offsetY;
        const uint8_t * transformInY = input.transform() - contourOffset.x;
        uint8_t * transformOutY = output.transform() + offsetY;
        const uint8_t * transformOutYEnd = transformOutY + imageWidth * height;

        for ( ; transformOutY != transformOutYEnd; transformInY += imageWidth, transformOutY += imageWidth, imageOutY += imageWidth ) {
            uint8_t * imageOutX = imageOutY;
            const uint8_t * transformInX = transformInY;
            uint8_t * transformOutX = transformOutY;
            const uint8_t * transformOutXEnd = transformOutX + width;

            for ( ; transformOutX != transformOutXEnd; ++transformInX, ++transformOutX, ++imageOutX ) {
                if ( *transformOutX == 1 && ( *transformInX == 0 || ( *( transformInX + contourOffset.x ) == 0 && *( transformInX + offsetY ) == 0 ) ) ) {
                    *imageOutX = colorId;
                    *transformOutX = 0;
                }
            }
        }

        input = std::move( output );
    }

    // CJK strokes are denser than bitmap Latin glyphs; button states use minimal effects.
    void applyCjkButtonGlyphEffects( fheroes2::Sprite & glyph, const fheroes2::FontType fontType )
    {
        if ( !isButtonFontSize( fontType.size ) ) {
            fheroes2::updateShadow( glyph, { -1, fontType.size == fheroes2::FontSize::SMALL ? 1 : 2 }, 2, false );
            return;
        }

        if ( fontType.size == fheroes2::FontSize::BUTTON_RELEASED ) {
            fheroes2::updateShadow( glyph, { 1, -1 }, 2, true );
            addCjkButtonOffsetContour( glyph, { -1, 1 }, getCjkGlyphOutlineColor( fontType ) );
            return;
        }

        fheroes2::updateShadow( glyph, { 1, -1 }, 2, true );
        fheroes2::updateShadow( glyph, { -1, 1 }, 7, true );
    }

    class CjkFontRenderer
    {
    public:
        // Initialize SDL_ttf and build a font chain for per-glyph fallback.
        bool isReady()
        {
            if ( _initializationAttempted ) {
                return _isReady;
            }

            _initializationAttempted = true;

            if ( TTF_WasInit() == 0 && TTF_Init() != 0 ) {
                ERROR_LOG( "Unable to initialize SDL_ttf for CJK text rendering: " << TTF_GetError() )
                return false;
            }

            _fontPaths = findCjkFontPaths();
            if ( _fontPaths.empty() ) {
                ERROR_LOG( "CJK font was not found. Set cjk_font_path or place a CJK TTF/TTC/OTF font under files/fonts/." )
                return false;
            }

            _isReady = true;
            DEBUG_LOG( DBG_GAME, DBG_INFO, "Using CJK font chain with " << _fontPaths.size() << " entries. Primary font: " << _fontPaths.front() )

            return true;
        }

        int32_t getLineHeight( const fheroes2::FontSize fontSize )
        {
            if ( isButtonFontSize( fontSize ) ) {
                return fheroes2::getFontHeight( fontSize );
            }

            for ( const size_t fontIndex : getFontSearchOrder( fontSize ) ) {
                if ( TTF_Font * font = getFont( fontIndex, fontSize ); font != nullptr ) {
                    return std::max( fheroes2::getFontHeight( fontSize ), TTF_FontLineSkip( font ) );
                }
            }

            return fheroes2::getFontHeight( fontSize );
        }

        int32_t getSpaceWidth( const fheroes2::FontSize fontSize )
        {
            for ( const size_t fontIndex : getFontSearchOrder( fontSize ) ) {
                TTF_Font * font = getFont( fontIndex, fontSize );
                if ( font == nullptr ) {
                    continue;
                }

                int minX = 0;
                int maxX = 0;
                int minY = 0;
                int maxY = 0;
                int advance = 0;
                if ( getGlyphMetrics( font, ' ', &minX, &maxX, &minY, &maxY, &advance ) && advance > 0 ) {
                    return advance;
                }
            }

            return getFallbackSpaceWidth( fontSize );
        }

        const CjkGlyph & getGlyph( const fheroes2::FontType fontType, const uint32_t codePoint )
        {
            const RenderableGlyph renderableGlyph = getRenderableGlyph( fontType.size, codePoint );
            const GlyphCacheKey key{ fontType.size, fontType.color, renderableGlyph.fontIndex, codePoint, renderableGlyph.codePoint };
            if ( const auto iter = _glyphCache.find( key ); iter != _glyphCache.end() ) {
                return iter->second;
            }

            // The full CJK repertoire across several font sizes and colors would let the cache grow without bound.
            // Glyphs are cheap to rebuild and have a high reuse rate, so a coarse flush keeps memory in check without
            // the bookkeeping of an LRU. The returned reference is consumed immediately by the caller, so clearing
            // before the insertion below is safe.
            if ( _glyphCache.size() >= glyphCacheLimit ) {
                _glyphCache.clear();
                _missingGlyphs.clear();
            }

            auto [iter, inserted] = _glyphCache.emplace( key, buildGlyph( fontType, codePoint, renderableGlyph ) );
            (void)inserted;
            assert( inserted );

            return iter->second;
        }

        // Drop all cached fonts and glyphs and shut SDL_ttf down. Must run while SDL is still alive.
        void release()
        {
            _glyphCache.clear();
            _missingGlyphs.clear();
            _fonts.clear();

            if ( TTF_WasInit() != 0 ) {
                TTF_Quit();
            }

            _initializationAttempted = false;
            _isReady = false;
        }

    private:
        using TtfFontPtr = std::unique_ptr<TTF_Font, TtfFontDeleter>;

        std::vector<size_t> getFontSearchOrder( const fheroes2::FontSize fontSize ) const
        {
            std::vector<size_t> order( _fontPaths.size() );
            std::iota( order.begin(), order.end(), size_t{ 0 } );

            if ( !isButtonFontSize( fontSize ) ) {
                return order;
            }

            const std::string & configuredFontPath = Settings::Get().getCjkFontPath();
            std::stable_sort( order.begin(), order.end(), [this, &configuredFontPath]( const size_t lhs, const size_t rhs ) {
                const bool isLeftConfigured = !configuredFontPath.empty() && _fontPaths[lhs] == configuredFontPath;
                const bool isRightConfigured = !configuredFontPath.empty() && _fontPaths[rhs] == configuredFontPath;
                if ( isLeftConfigured != isRightConfigured ) {
                    return isLeftConfigured;
                }

                const int32_t leftPriority = getCjkButtonFontNamePriority( _fontPaths[lhs] );
                const int32_t rightPriority = getCjkButtonFontNamePriority( _fontPaths[rhs] );
                if ( leftPriority != rightPriority ) {
                    return leftPriority < rightPriority;
                }

                return lhs < rhs;
            } );

            return order;
        }

        TTF_Font * getFont( const size_t fontIndex, const fheroes2::FontSize fontSize )
        {
            if ( !isReady() || fontIndex >= _fontPaths.size() ) {
                return nullptr;
            }

            const FontCacheKey key{ fontIndex, fontSize };
            auto [iter, inserted] = _fonts.try_emplace( key );
            if ( inserted ) {
                iter->second.reset( TTF_OpenFont( _fontPaths[fontIndex].c_str(), getTtfPixelSize( fontSize ) ) );
                if ( !iter->second ) {
                    ERROR_LOG( "Unable to open CJK font " << _fontPaths[fontIndex] << ": " << TTF_GetError() )
                }
                else {
                    // Text keeps normal hinting, while button glyphs snap harder to the pixel grid.
                    TTF_SetFontHinting( iter->second.get(), isButtonFontSize( fontSize ) ? TTF_HINTING_MONO : TTF_HINTING_NORMAL );
                }
            }

            return iter->second.get();
        }

        bool isGlyphProvided( TTF_Font * font, const uint32_t codePoint ) const
        {
            if ( font == nullptr ) {
                return false;
            }

#if defined( FHEROES2_SDL_TTF_HAS_32BIT_GLYPHS )
            return TTF_GlyphIsProvided32( font, codePoint ) != 0;
#else
            return codePoint <= 0xFFFF && TTF_GlyphIsProvided( font, static_cast<Uint16>( codePoint ) ) != 0;
#endif
        }

        bool getGlyphMetrics( TTF_Font * font, const uint32_t codePoint, int * minX, int * maxX, int * minY, int * maxY, int * advance ) const
        {
            if ( font == nullptr ) {
                return false;
            }

#if defined( FHEROES2_SDL_TTF_HAS_32BIT_GLYPHS )
            return TTF_GlyphMetrics32( font, codePoint, minX, maxX, minY, maxY, advance ) == 0;
#else
            return codePoint <= 0xFFFF && TTF_GlyphMetrics( font, static_cast<Uint16>( codePoint ), minX, maxX, minY, maxY, advance ) == 0;
#endif
        }

        SDL_Surface * renderGlyph( TTF_Font * font, const uint32_t codePoint, const SDL_Color color ) const
        {
            if ( font == nullptr ) {
                return nullptr;
            }

#if defined( FHEROES2_SDL_TTF_HAS_32BIT_GLYPHS )
            return TTF_RenderGlyph32_Blended( font, codePoint, color );
#else
            if ( codePoint > 0xFFFF ) {
                return nullptr;
            }

            return TTF_RenderGlyph_Blended( font, static_cast<Uint16>( codePoint ), color );
#endif
        }

        RenderableGlyph getRenderableGlyph( const fheroes2::FontSize fontSize, const uint32_t codePoint )
        {
            // Try the original code point across the font chain before falling back.
            for ( const size_t fontIndex : getFontSearchOrder( fontSize ) ) {
                if ( isGlyphProvided( getFont( fontIndex, fontSize ), codePoint ) ) {
                    return { fontIndex, codePoint, true };
                }
            }

            if ( _missingGlyphs.insert( codePoint ).second ) {
                ERROR_LOG( "CJK font chain does not provide glyph for Unicode code point " << codePoint << ". A fallback glyph will be used." )
            }

            for ( const uint32_t fallbackCodePoint : { unicodeReplacementGlyph, unicodeQuestionMark } ) {
                for ( const size_t fontIndex : getFontSearchOrder( fontSize ) ) {
                    if ( isGlyphProvided( getFont( fontIndex, fontSize ), fallbackCodePoint ) ) {
                        return { fontIndex, fallbackCodePoint, true };
                    }
                }
            }

            return {};
        }

        CjkGlyph buildGlyph( const fheroes2::FontType fontType, const uint32_t codePoint, const RenderableGlyph renderableGlyph )
        {
            CjkGlyph glyph;

            if ( isUnicodeSpace( codePoint ) ) {
                glyph.advance = getSpaceWidth( fontType.size );
                return glyph;
            }

            if ( isUnicodeLineSeparator( codePoint ) ) {
                return glyph;
            }

            TTF_Font * font = getFont( renderableGlyph.fontIndex, fontType.size );
            if ( font == nullptr || !renderableGlyph.isValid ) {
                glyph.advance = getFallbackSpaceWidth( fontType.size );
                return glyph;
            }

            int minX = 0;
            int maxX = 0;
            int minY = 0;
            int maxY = 0;
            int advance = 0;
            if ( !getGlyphMetrics( font, renderableGlyph.codePoint, &minX, &maxX, &minY, &maxY, &advance ) || advance <= 0 ) {
                advance = getFallbackSpaceWidth( fontType.size );
            }

            const SDL_Color color{ 255, 255, 255, 255 };
            std::unique_ptr<SDL_Surface, decltype( &SDL_FreeSurface )> surface( renderGlyph( font, renderableGlyph.codePoint, color ), SDL_FreeSurface );
            if ( !surface ) {
                ERROR_LOG( "Unable to render CJK glyph " << codePoint << ": " << TTF_GetError() )
                glyph.advance = advance;
                return glyph;
            }

            const int32_t paddingLeft = 2;
            const int32_t paddingTop = isButtonFontSize( fontType.size ) ? 2 : 1;
            const int32_t paddingRight = 2;
            const int32_t paddingBottom = 3;

            glyph.sprite.resize( surface->w + paddingLeft + paddingRight, surface->h + paddingTop + paddingBottom );
            glyph.sprite.reset();

            if ( SDL_MUSTLOCK( surface.get() ) && SDL_LockSurface( surface.get() ) != 0 ) {
                ERROR_LOG( "Unable to lock CJK glyph surface: " << SDL_GetError() )
                glyph.advance = advance;
                return glyph;
            }

            for ( int32_t y = 0; y < surface->h; ++y ) {
                for ( int32_t x = 0; x < surface->w; ++x ) {
                    const uint8_t alpha = getSurfaceAlpha( *surface, x, y );
                    if ( alpha <= getCjkGlyphAlphaThreshold( fontType ) ) {
                        continue;
                    }

                    const int32_t outputX = x + paddingLeft;
                    const int32_t outputY = y + paddingTop;
                    const int32_t outputOffset = outputY * glyph.sprite.width() + outputX;

                    glyph.sprite.image()[outputOffset] = getCjkGlyphColor( fontType, alpha );
                    glyph.sprite.transform()[outputOffset] = 0;
                }
            }

            if ( SDL_MUSTLOCK( surface.get() ) ) {
                SDL_UnlockSurface( surface.get() );
            }

            glyph.sprite.setPosition( minX - paddingLeft, getCjkGlyphVerticalOffset( font, codePoint, maxY, paddingTop ) );
            applyCjkButtonGlyphEffects( glyph.sprite, fontType );

            glyph.advance = advance + getCjkGlyphExtraAdvance( fontType, codePoint );
            return glyph;
        }

        // Upper bound for the glyph cache. Sized to comfortably hold a full screen of distinct CJK characters across
        // the handful of font size and color combinations used in the UI, while bounding worst-case memory growth.
        static constexpr size_t glyphCacheLimit{ 4096 };

        std::vector<std::string> _fontPaths;
        std::map<FontCacheKey, TtfFontPtr> _fonts;
        std::map<GlyphCacheKey, CjkGlyph> _glyphCache;
        std::set<uint32_t> _missingGlyphs;
        bool _initializationAttempted{ false };
        bool _isReady{ false };
    };

    CjkFontRenderer & getCjkFontRenderer()
    {
        static CjkFontRenderer renderer;
        return renderer;
    }

    bool shouldUseUnicodeTextPath( const std::string_view text, const std::optional<fheroes2::SupportedLanguage> & language, const fheroes2::FontType fontType )
    {
        (void)fontType;

        return isUnicodeTextCandidate( text, language ) && getCjkFontRenderer().isReady();
    }

    // The Unicode path still uses the original bitmap font for ASCII characters.
    bool isBitmapFontCodePoint( const uint32_t codePoint, const fheroes2::FontCharHandler & charHandler )
    {
        return codePoint <= 0x7F && charHandler.isAvailable( static_cast<uint8_t>( codePoint ) );
    }

    void mergeTextArea( fheroes2::Rect & area, bool & hasArea, const fheroes2::Rect & glyphArea )
    {
        if ( hasArea ) {
            const int32_t left = std::min( area.x, glyphArea.x );
            const int32_t top = std::min( area.y, glyphArea.y );
            const int32_t right = std::max( area.x + area.width, glyphArea.x + glyphArea.width );
            const int32_t bottom = std::max( area.y + area.height, glyphArea.y + glyphArea.height );
            area = { left, top, right - left, bottom - top };
            return;
        }

        area = glyphArea;
        hasArea = true;
    }

    int32_t getUnicodeCodePointWidth( const fheroes2::FontType fontType, const uint32_t codePoint, const fheroes2::FontCharHandler & charHandler )
    {
        if ( isBitmapFontCodePoint( codePoint, charHandler ) ) {
            return charHandler.getWidth( static_cast<uint8_t>( codePoint ) );
        }

        if ( isUnicodeLineSeparator( codePoint ) ) {
            return 0;
        }

        if ( isUnicodeSpace( codePoint ) ) {
            return getCjkFontRenderer().getSpaceWidth( fontType.size );
        }

        return getCjkFontRenderer().getGlyph( fontType, codePoint ).advance;
    }

    int32_t renderBitmapCodePoint( const uint32_t codePoint, const int32_t x, const int32_t y, fheroes2::Image & output, const fheroes2::Rect & imageRoi,
                                   const fheroes2::FontCharHandler & charHandler )
    {
        const auto character = static_cast<uint8_t>( codePoint );
        if ( isSpaceChar( character ) ) {
            return x + charHandler.getWidth( character );
        }

        if ( isLineSeparator( character ) ) {
            return x;
        }

        const fheroes2::Sprite & charSprite = charHandler.getSprite( character );
        const fheroes2::Rect charRoi{ x + charSprite.x(), y + charSprite.y(), charSprite.width(), charSprite.height() };
        const fheroes2::Rect overlappedRoi = imageRoi ^ charRoi;

        fheroes2::Blit( charSprite, overlappedRoi.x - charRoi.x, overlappedRoi.y - charRoi.y, output, overlappedRoi.x, overlappedRoi.y, overlappedRoi.width,
                        overlappedRoi.height );

        return x + charHandler.getWidth( character );
    }

    int32_t getUnicodeFontHeight( const fheroes2::FontSize fontSize )
    {
        return getCjkFontRenderer().getLineHeight( fontSize );
    }

    int32_t getUnicodeLineWidth( const std::string_view text, const fheroes2::FontType fontType, const bool keepTrailingSpaces )
    {
        std::vector<Utf8Character> characters;
        bool hasNonAscii = false;
        decodeUtf8( text, characters, hasNonAscii );
        const fheroes2::FontCharHandler charHandler( fontType );

        int32_t width = 0;
        int32_t trailingSpaceWidth = 0;

        for ( const Utf8Character & character : characters ) {
            const int32_t characterWidth = getUnicodeCodePointWidth( fontType, character.codePoint, charHandler );
            if ( isUnicodeSpace( character.codePoint ) ) {
                trailingSpaceWidth += characterWidth;
                width += keepTrailingSpaces ? characterWidth : 0;
            }
            else if ( !isUnicodeLineSeparator( character.codePoint ) ) {
                if ( !keepTrailingSpaces ) {
                    width += trailingSpaceWidth;
                    trailingSpaceWidth = 0;
                }
                width += characterWidth;
            }
        }

        return width;
    }

    int32_t getUnicodeMaxByteCount( const std::string_view text, const fheroes2::FontType fontType, const int32_t maxWidth )
    {
        std::vector<Utf8Character> characters;
        bool hasNonAscii = false;
        decodeUtf8( text, characters, hasNonAscii );
        const fheroes2::FontCharHandler charHandler( fontType );

        int32_t width = 0;
        for ( const Utf8Character & character : characters ) {
            width += getUnicodeCodePointWidth( fontType, character.codePoint, charHandler );
            if ( width > maxWidth ) {
                return static_cast<int32_t>( character.byteOffset );
            }
        }

        return static_cast<int32_t>( text.size() );
    }

    int32_t getUnicodeMaxWordWidth( const std::string_view text, const fheroes2::FontType fontType )
    {
        std::vector<Utf8Character> characters;
        bool hasNonAscii = false;
        decodeUtf8( text, characters, hasNonAscii );
        const fheroes2::FontCharHandler charHandler( fontType );

        int32_t maxWidth = 1;
        int32_t width = 0;

        for ( size_t i = 0; i < characters.size(); ++i ) {
            const Utf8Character & character = characters[i];
            const uint32_t nextCodePoint = ( i + 1 < characters.size() ) ? characters[i + 1].codePoint : 0;

            if ( isUnicodeSpace( character.codePoint ) || isUnicodeLineSeparator( character.codePoint ) ) {
                maxWidth = std::max( maxWidth, width == 0 ? getUnicodeCodePointWidth( fontType, character.codePoint, charHandler ) : width );
                width = 0;
                continue;
            }

            width += getUnicodeCodePointWidth( fontType, character.codePoint, charHandler );
            if ( isUnicodeBreakAllowedAfter( character.codePoint, nextCodePoint ) ) {
                maxWidth = std::max( maxWidth, width );
                width = 0;
            }
        }

        return std::max( maxWidth, width );
    }

    fheroes2::Rect getUnicodeTextLineArea( const std::string_view text, const fheroes2::FontType fontType )
    {
        std::vector<Utf8Character> characters;
        bool hasNonAscii = false;
        decodeUtf8( text, characters, hasNonAscii );
        const fheroes2::FontCharHandler charHandler( fontType );

        fheroes2::Rect area;
        bool hasArea = false;
        int32_t offsetX = 0;

        for ( const Utf8Character & character : characters ) {
            if ( isBitmapFontCodePoint( character.codePoint, charHandler ) ) {
                const auto asciiCharacter = static_cast<uint8_t>( character.codePoint );
                if ( !isSpaceChar( asciiCharacter ) && !isLineSeparator( asciiCharacter ) ) {
                    const fheroes2::Sprite & sprite = charHandler.getSprite( asciiCharacter );
                    mergeTextArea( area, hasArea, { offsetX + sprite.x(), sprite.y(), sprite.width(), sprite.height() } );
                }

                offsetX += charHandler.getWidth( asciiCharacter );
                continue;
            }

            if ( isUnicodeSpace( character.codePoint ) ) {
                offsetX += getUnicodeCodePointWidth( fontType, character.codePoint, charHandler );
                continue;
            }

            if ( isUnicodeLineSeparator( character.codePoint ) ) {
                continue;
            }

            const CjkGlyph & glyph = getCjkFontRenderer().getGlyph( fontType, character.codePoint );
            const fheroes2::Sprite & sprite = glyph.sprite;

            if ( !sprite.empty() ) {
                const fheroes2::Rect glyphArea{ offsetX + sprite.x(), sprite.y(), sprite.width(), sprite.height() };
                mergeTextArea( area, hasArea, glyphArea );
            }

            offsetX += glyph.advance;
        }

        if ( !hasArea ) {
            area.width = offsetX;
            area.height = getUnicodeFontHeight( fontType.size );
        }

        return area;
    }

    int32_t renderUnicodeSingleLine( const std::string_view text, const int32_t x, const int32_t y, fheroes2::Image & output, const fheroes2::Rect & imageRoi,
                                     const fheroes2::FontType fontType )
    {
        assert( !output.empty() );

        std::vector<Utf8Character> characters;
        bool hasNonAscii = false;
        decodeUtf8( text, characters, hasNonAscii );
        const fheroes2::FontCharHandler charHandler( fontType );

        int32_t offsetX = x;

        for ( const Utf8Character & character : characters ) {
            if ( isBitmapFontCodePoint( character.codePoint, charHandler ) ) {
                offsetX = renderBitmapCodePoint( character.codePoint, offsetX, y, output, imageRoi, charHandler );
                continue;
            }

            if ( isUnicodeSpace( character.codePoint ) ) {
                offsetX += getUnicodeCodePointWidth( fontType, character.codePoint, charHandler );
                continue;
            }

            if ( isUnicodeLineSeparator( character.codePoint ) ) {
                continue;
            }

            const CjkGlyph & glyph = getCjkFontRenderer().getGlyph( fontType, character.codePoint );
            const fheroes2::Sprite & sprite = glyph.sprite;

            if ( !sprite.empty() ) {
                const fheroes2::Rect charRoi{ offsetX + sprite.x(), y + sprite.y(), sprite.width(), sprite.height() };
                const fheroes2::Rect overlappedRoi = imageRoi ^ charRoi;

                fheroes2::Blit( sprite, overlappedRoi.x - charRoi.x, overlappedRoi.y - charRoi.y, output, overlappedRoi.x, overlappedRoi.y, overlappedRoi.width,
                                overlappedRoi.height );
            }

            offsetX += glyph.advance;
        }

        return offsetX;
    }

    void getUnicodeTextLineInfos( std::vector<fheroes2::TextLineInfo> & textLineInfos, const std::string_view text, const fheroes2::FontType fontType,
                                  const int32_t maxWidth, const int32_t rowHeight, const bool keepLineTrailingSpaces, const bool keepTextTrailingSpaces )
    {
        std::vector<Utf8Character> characters;
        bool hasNonAscii = false;
        decodeUtf8( text, characters, hasNonAscii );
        const fheroes2::FontCharHandler charHandler( fontType );

        const int32_t firstLineOffsetX = textLineInfos.empty() ? 0 : textLineInfos.back().lineWidth;
        int32_t offsetX = firstLineOffsetX;
        int32_t offsetY = textLineInfos.empty() ? 0 : textLineInfos.back().offsetY;

        if ( maxWidth < 1 ) {
            const int32_t lineWidth = firstLineOffsetX + getUnicodeLineWidth( text, fontType, keepLineTrailingSpaces || keepTextTrailingSpaces );
            textLineInfos.emplace_back( firstLineOffsetX, offsetY, lineWidth, static_cast<int32_t>( text.size() ) );
            return;
        }

        size_t lineStartIndex = 0;
        size_t characterIndex = 0;
        int32_t lineWidth = firstLineOffsetX;
        int32_t trailingSpaceWidth = 0;
        size_t lastBreakIndex = std::string_view::npos;
        int32_t lastBreakWidth = 0;

        auto appendLine = [&]( const size_t endIndex, const int32_t width ) {
            const size_t lineStartByte = lineStartIndex < characters.size() ? characters[lineStartIndex].byteOffset : text.size();
            const size_t lineEndByte = endIndex < characters.size() ? characters[endIndex].byteOffset : text.size();
            textLineInfos.emplace_back( offsetX, offsetY, width, static_cast<int32_t>( lineEndByte - lineStartByte ) );

            offsetX = 0;
            offsetY += rowHeight;
            lineStartIndex = endIndex;
            characterIndex = endIndex;
            lineWidth = 0;
            trailingSpaceWidth = 0;
            lastBreakIndex = std::string_view::npos;
            lastBreakWidth = 0;
        };

        while ( characterIndex < characters.size() ) {
            const Utf8Character & character = characters[characterIndex];

            if ( isUnicodeLineSeparator( character.codePoint ) ) {
                const int32_t width = keepLineTrailingSpaces ? lineWidth : lineWidth - trailingSpaceWidth;
                appendLine( characterIndex + 1, width );
                continue;
            }

            const int32_t characterWidth = getUnicodeCodePointWidth( fontType, character.codePoint, charHandler );

            if ( lineWidth != 0 && lineWidth + characterWidth > maxWidth ) {
                if ( lastBreakIndex != std::string_view::npos && lastBreakIndex > lineStartIndex ) {
                    appendLine( lastBreakIndex, lastBreakWidth );
                    continue;
                }

                if ( characterIndex > lineStartIndex ) {
                    appendLine( characterIndex, keepLineTrailingSpaces ? lineWidth : lineWidth - trailingSpaceWidth );
                    continue;
                }
            }

            lineWidth += characterWidth;

            if ( isUnicodeSpace( character.codePoint ) ) {
                trailingSpaceWidth += characterWidth;
            }
            else {
                trailingSpaceWidth = 0;
            }

            const uint32_t nextCodePoint = ( characterIndex + 1 < characters.size() ) ? characters[characterIndex + 1].codePoint : 0;
            if ( isUnicodeBreakAllowedAfter( character.codePoint, nextCodePoint ) ) {
                lastBreakIndex = characterIndex + 1;
                lastBreakWidth = keepLineTrailingSpaces ? lineWidth : lineWidth - trailingSpaceWidth;
            }

            ++characterIndex;
        }

        const int32_t finalWidth = ( keepLineTrailingSpaces || keepTextTrailingSpaces ) ? lineWidth : lineWidth - trailingSpaceWidth;
        textLineInfos.emplace_back( offsetX, offsetY, finalWidth, static_cast<int32_t>( text.size() - ( lineStartIndex < characters.size() ? characters[lineStartIndex].byteOffset : text.size() ) ) );
    }
#else
    bool shouldUseUnicodeTextPath( const std::string_view text, const std::optional<fheroes2::SupportedLanguage> & language, const fheroes2::FontType fontType )
    {
        (void)fontType;

        if ( isUnicodeTextCandidate( text, language ) ) {
            static bool isLogged = false;
            if ( !isLogged ) {
                ERROR_LOG( "UTF-8 CJK text rendering is disabled because the game was built without SDL_ttf support." )
                isLogged = true;
            }
        }

        return false;
    }

    int32_t getUnicodeFontHeight( const fheroes2::FontSize fontSize )
    {
        return fheroes2::getFontHeight( fontSize );
    }

    int32_t getUnicodeLineWidth( const std::string_view, const fheroes2::FontType, const bool )
    {
        return 0;
    }

    int32_t getUnicodeMaxByteCount( const std::string_view, const fheroes2::FontType, const int32_t )
    {
        return 0;
    }

    int32_t getUnicodeMaxWordWidth( const std::string_view, const fheroes2::FontType )
    {
        return 1;
    }

    fheroes2::Rect getUnicodeTextLineArea( const std::string_view, const fheroes2::FontType )
    {
        return {};
    }

    int32_t renderUnicodeSingleLine( const std::string_view, const int32_t x, const int32_t, fheroes2::Image &, const fheroes2::Rect &, const fheroes2::FontType )
    {
        return x;
    }

    void getUnicodeTextLineInfos( std::vector<fheroes2::TextLineInfo> &, const std::string_view, const fheroes2::FontType, const int32_t, const int32_t, const bool,
                                  const bool )
    {
        // Do nothing.
    }
#endif

    int32_t getUnicodeTextVerticalOffset( const fheroes2::FontType fontType )
    {
#if defined( WITH_CJK_TEXT )
        // Button labels use bitmap-style vertical placement, while TTF CJK glyphs sit lower by default.
        return isButtonFontSize( fontType.size ) ? -1 : 0;
#else
        (void)fontType;

        return 0;
#endif
    }

    const fheroes2::Sprite & getChar( const uint8_t character, const fheroes2::FontType & fontType )
    {
        switch ( fontType.size ) {
        case fheroes2::FontSize::SMALL:
            switch ( fontType.color ) {
            case fheroes2::FontColor::WHITE:
                return fheroes2::AGG::GetICN( ICN::SMALFONT, character );
            case fheroes2::FontColor::GRAY:
                return fheroes2::AGG::GetICN( ICN::GRAY_SMALL_FONT, character );
            case fheroes2::FontColor::YELLOW:
                return fheroes2::AGG::GetICN( ICN::YELLOW_SMALLFONT, character );
            default:
                // Did you add a new font color? Add the corresponding logic for it!
                assert( 0 );
                break;
            }
            break;
        case fheroes2::FontSize::NORMAL:
            switch ( fontType.color ) {
            case fheroes2::FontColor::WHITE:
                return fheroes2::AGG::GetICN( ICN::FONT, character );
            case fheroes2::FontColor::GRAY:
                return fheroes2::AGG::GetICN( ICN::GRAY_FONT, character );
            case fheroes2::FontColor::YELLOW:
                return fheroes2::AGG::GetICN( ICN::YELLOW_FONT, character );
            case fheroes2::FontColor::GOLDEN_GRADIENT:
                return fheroes2::AGG::GetICN( ICN::GOLDEN_GRADIENT_FONT, character );
            case fheroes2::FontColor::SILVER_GRADIENT:
                return fheroes2::AGG::GetICN( ICN::SILVER_GRADIENT_FONT, character );
            default:
                // Did you add a new font color? Add the corresponding logic for it!
                assert( 0 );
                break;
            }
            break;
        case fheroes2::FontSize::LARGE:
            switch ( fontType.color ) {
            case fheroes2::FontColor::WHITE:
                return fheroes2::AGG::GetICN( ICN::WHITE_LARGE_FONT, character );
            case fheroes2::FontColor::GOLDEN_GRADIENT:
                return fheroes2::AGG::GetICN( ICN::GOLDEN_GRADIENT_LARGE_FONT, character );
            case fheroes2::FontColor::SILVER_GRADIENT:
                return fheroes2::AGG::GetICN( ICN::SILVER_GRADIENT_LARGE_FONT, character );
            default:
                // Did you add a new font color? Add the corresponding logic for it!
                assert( 0 );
                break;
            }
            break;
        case fheroes2::FontSize::BUTTON_RELEASED:
            switch ( fontType.color ) {
            case fheroes2::FontColor::WHITE:
                return fheroes2::AGG::GetICN( ICN::BUTTON_GOOD_FONT_RELEASED, character );
            case fheroes2::FontColor::GRAY:
                return fheroes2::AGG::GetICN( ICN::BUTTON_EVIL_FONT_RELEASED, character );
            default:
                // Did you add a new font color? Add the corresponding logic for it!
                assert( 0 );
                break;
            }
            break;
        case fheroes2::FontSize::BUTTON_PRESSED:
            switch ( fontType.color ) {
            case fheroes2::FontColor::WHITE:
                return fheroes2::AGG::GetICN( ICN::BUTTON_GOOD_FONT_PRESSED, character );
            case fheroes2::FontColor::GRAY:
                return fheroes2::AGG::GetICN( ICN::BUTTON_EVIL_FONT_PRESSED, character );
            default:
                // Did you add a new font color? Add the corresponding logic for it!
                assert( 0 );
                break;
            }
            break;
        default:
            // Did you add a new font size? Add the corresponding logic for it!
            assert( 0 );
            break;
        }

        assert( 0 ); // Did you add a new font size? Please add implementation.

        return errorImage;
    }

    uint32_t getCharacterLimit( const fheroes2::FontSize fontSize )
    {
        switch ( fontSize ) {
        case fheroes2::FontSize::SMALL:
            return fheroes2::AGG::GetICNCount( ICN::SMALFONT );
        case fheroes2::FontSize::NORMAL:
        case fheroes2::FontSize::LARGE:
            return fheroes2::AGG::GetICNCount( ICN::FONT );
        case fheroes2::FontSize::BUTTON_RELEASED:
        case fheroes2::FontSize::BUTTON_PRESSED:
            return fheroes2::AGG::GetICNCount( ICN::BUTTON_GOOD_FONT_RELEASED );
        default:
            // Did you add a new font size? Please add implementation.
            assert( 0 );
        }

        return 0;
    }

    int32_t getLineWidth( const uint8_t * data, const int32_t size, const fheroes2::FontCharHandler & charHandler, const bool keepTrailingSpaces )
    {
        assert( data != nullptr && size > 0 );

        int32_t width = 0;
        const uint8_t * dataEnd = data + size;

        if ( keepTrailingSpaces ) {
            for ( ; data != dataEnd; ++data ) {
                width += charHandler.getWidth( *data );
            }

            return width;
        }

        int32_t spaceWidth = 0;
        const int32_t spaceCharWidth = charHandler.getSpaceCharWidth();

        for ( ; data != dataEnd; ++data ) {
            if ( isSpaceChar( *data ) ) {
                spaceWidth += spaceCharWidth;
            }
            else if ( !isLineSeparator( *data ) ) {
                width += spaceWidth + charHandler.getWidth( *data );

                spaceWidth = 0;
            }
        }

        return width;
    }

    fheroes2::Rect getTextLineArea( const uint8_t * data, const int32_t size, const fheroes2::FontCharHandler & charHandler )
    {
        assert( data != nullptr && size > 0 );

        const uint8_t * dataEnd = data + size;

        const fheroes2::Sprite & firstCharSprite = charHandler.getSprite( *data );
        fheroes2::Rect area( firstCharSprite.x(), firstCharSprite.y(), firstCharSprite.width(), firstCharSprite.height() );
        ++data;

        for ( ; data != dataEnd; ++data ) {
            const fheroes2::Sprite & sprite = charHandler.getSprite( *data );

            if ( const int32_t spriteY = sprite.y(); spriteY < area.y ) {
                // This character sprite is drawn higher than all previous - update `height` and `y`.
                area.height += area.y - spriteY;
                area.y = spriteY;
                area.height = std::max( area.height, sprite.height() );
            }
            else {
                area.height = std::max( area.height, spriteY - area.y + sprite.height() );
            }

            area.width += sprite.x() + sprite.width();
        }

        return area;
    }

    int32_t getMaxCharacterCount( const uint8_t * data, const int32_t size, const fheroes2::FontCharHandler & charHandler, const int32_t maxWidth )
    {
        assert( data != nullptr && size > 0 && maxWidth > 0 );

        int32_t width = 0;

        for ( int32_t characterCount = 0; characterCount < size; ++characterCount, ++data ) {
            width += charHandler.getWidth( *data );

            if ( width > maxWidth ) {
                return characterCount;
            }
        }

        return size;
    }

    int32_t renderSingleLine( const uint8_t * data, const int32_t size, const int32_t x, const int32_t y, fheroes2::Image & output, const fheroes2::Rect & imageRoi,
                              const fheroes2::FontCharHandler & charHandler )
    {
        assert( data != nullptr && size > 0 && !output.empty() );

        int32_t offsetX = x;

        const int32_t spaceCharWidth = charHandler.getSpaceCharWidth();
        const uint8_t * dataEnd = data + size;

        for ( ; data != dataEnd; ++data ) {
            if ( isSpaceChar( *data ) ) {
                offsetX += spaceCharWidth;
                continue;
            }

            // TODO: remove this hack or expand it to cover more cases.
            if ( isLineSeparator( *data ) ) {
                // This should never happen as a line cannot contain line separator in the middle.
                // But due to some limitations in UI we have to deal with it.
                // The only way is to just ignore it here.
                continue;
            }

            const fheroes2::Sprite & charSprite = charHandler.getSprite( *data );
            assert( !charSprite.empty() );

            const fheroes2::Rect charRoi{ offsetX + charSprite.x(), y + charSprite.y(), charSprite.width(), charSprite.height() };

            const fheroes2::Rect overlappedRoi = imageRoi ^ charRoi;

            fheroes2::Blit( charSprite, overlappedRoi.x - charRoi.x, overlappedRoi.y - charRoi.y, output, overlappedRoi.x, overlappedRoi.y, overlappedRoi.width,
                            overlappedRoi.height );
            offsetX += charSprite.width() + charSprite.x();
        }

        return offsetX;
    }

    int32_t getMaxWordWidth( const uint8_t * data, const int32_t size, const fheroes2::FontType fontType )
    {
        assert( data != nullptr && size > 0 );

        const fheroes2::FontCharHandler charHandler( fontType );

        int32_t maxWidth = 1;
        int32_t width = 0;

        const uint8_t * dataEnd = data + size;
        while ( data != dataEnd ) {
            if ( isSpaceChar( *data ) || isLineSeparator( *data ) ) {
                // If it is the end of line ("\n") or a space (" "), then the word has ended.
                if ( width == 0 && isSpaceChar( *data ) ) {
                    // No words exist on this till now. Let's put maximum width as space width.
                    width = charHandler.getWidth( *data );
                }

                if ( maxWidth < width ) {
                    maxWidth = width;
                }

                width = 0;
            }
            else {
                width += charHandler.getWidth( *data );
            }

            ++data;
        }

        return std::max( maxWidth, width );
    }

    std::unique_ptr<fheroes2::LanguageSwitcher> getLanguageSwitcher( const fheroes2::TextBase & text )
    {
        const auto & language = text.getLanguage();
        if ( !language ) {
            return {};
        }

        return std::make_unique<fheroes2::LanguageSwitcher>( language.value() );
    }
}

namespace fheroes2
{
    int32_t getFontHeight( const FontSize fontSize )
    {
        switch ( fontSize ) {
        case FontSize::SMALL:
            return 8 + 2 + 1;
        case FontSize::NORMAL:
            return 13 + 3 + 1;
        case FontSize::LARGE:
            return 26 + 6 + 1;
        case FontSize::BUTTON_RELEASED:
        case FontSize::BUTTON_PRESSED:
            return 15;
        default:
            assert( 0 ); // Did you add a new font size? Please add implementation.
            break;
        }

        return 0;
    }

    TextBase::~TextBase() = default;

    Text::~Text() = default;

    // TODO: Properly handle strings with many text lines ('\n'). Now their widths are counted as if they're one line.
    int32_t Text::width() const
    {
        const auto languageSwitcher = getLanguageSwitcher( *this );
        if ( shouldUseUnicodeTextPath( _text, _language, _fontType ) ) {
            return getUnicodeLineWidth( _text, _fontType, _keepLineTrailingSpaces );
        }

        const FontCharHandler charHandler( _fontType );

        return getLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), charHandler, _keepLineTrailingSpaces );
    }

    // TODO: Properly handle strings with many text lines ('\n'). Now their heights are counted as if they're one line.
    int32_t Text::height() const
    {
        const auto languageSwitcher = getLanguageSwitcher( *this );
        if ( shouldUseUnicodeTextPath( _text, _language, _fontType ) ) {
            return getUnicodeFontHeight( _fontType.size );
        }

        return getFontHeight( _fontType.size );
    }

    int32_t Text::width( const int32_t maxWidth ) const
    {
        if ( _text.empty() ) {
            return 0;
        }

        const auto languageSwitcher = getLanguageSwitcher( *this );
        const int32_t fontHeight = height();
        const bool useUnicodeTextPath = shouldUseUnicodeTextPath( _text, _language, _fontType );

        std::vector<TextLineInfo> lineInfos;
        _getTextLineInfos( lineInfos, maxWidth, fontHeight, false );

        if ( lineInfos.size() == 1 ) {
            // This is a single-line message.
            return lineInfos.front().lineWidth;
        }

        if ( !_isUniformedVerticalAlignment ) {
            // This is a multi-lined message and we try to fit as many words on every line as possible.
            return std::max_element( lineInfos.begin(), lineInfos.end(), []( const TextLineInfo & a, const TextLineInfo & b ) { return a.lineWidth < b.lineWidth; } )
                ->lineWidth;
        }

        // This is a multi-line message. Optimize it to fit the text evenly to the same number of lines.
        int32_t startWidth = useUnicodeTextPath ? getUnicodeMaxWordWidth( _text, _fontType )
                                                : getMaxWordWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType );
        int32_t endWidth = maxWidth;

        while ( startWidth + 1 < endWidth ) {
            const int32_t currentWidth = ( endWidth + startWidth ) / 2;
            std::vector<TextLineInfo> tempLineInfos;
            _getTextLineInfos( tempLineInfos, currentWidth, fontHeight, false );

            if ( tempLineInfos.size() > lineInfos.size() ) {
                startWidth = currentWidth;
                continue;
            }

            endWidth = currentWidth;
        }

        return endWidth;
    }

    int32_t Text::height( const int32_t maxWidth ) const
    {
        if ( _text.empty() ) {
            return 0;
        }

        const auto languageSwitcher = getLanguageSwitcher( *this );
        const int32_t fontHeight = height();

        std::vector<TextLineInfo> lineInfos;
        _getTextLineInfos( lineInfos, maxWidth, fontHeight, false );

        return lineInfos.back().offsetY + fontHeight;
    }

    int32_t Text::rows( const int32_t maxWidth ) const
    {
        if ( _text.empty() ) {
            return 0;
        }

        const auto languageSwitcher = getLanguageSwitcher( *this );
        std::vector<TextLineInfo> lineInfos;
        _getTextLineInfos( lineInfos, maxWidth, height(), false );

        return static_cast<int32_t>( lineInfos.size() );
    }

    Rect Text::area() const
    {
        if ( _text.empty() ) {
            return {};
        }

        const auto languageSwitcher = getLanguageSwitcher( *this );
        if ( shouldUseUnicodeTextPath( _text, _language, _fontType ) ) {
            return getUnicodeTextLineArea( _text, _fontType );
        }

        const FontCharHandler charHandler( _fontType );

        return getTextLineArea( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), charHandler );
    }

    void Text::drawInRoi( const int32_t x, const int32_t y, Image & output, const Rect & imageRoi ) const
    {
        if ( output.empty() || _text.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        const auto languageSwitcher = getLanguageSwitcher( *this );
        if ( shouldUseUnicodeTextPath( _text, _language, _fontType ) ) {
            renderUnicodeSingleLine( _text, x, y + getUnicodeTextVerticalOffset( _fontType ), output, imageRoi, _fontType );
            return;
        }

        const FontCharHandler charHandler( _fontType );

        renderSingleLine( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), x, y, output, imageRoi, charHandler );
    }

    void Text::drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const
    {
        if ( output.empty() || _text.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        if ( maxWidth <= 0 ) {
            drawInRoi( x, y, output, imageRoi );
            return;
        }

        const auto languageSwitcher = getLanguageSwitcher( *this );
        const bool useUnicodeTextPath = shouldUseUnicodeTextPath( _text, _language, _fontType );

        std::vector<TextLineInfo> lineInfos;
        _getTextLineInfos( lineInfos, maxWidth, height(), false );

        const uint8_t * data = reinterpret_cast<const uint8_t *>( _text.data() );
        const FontCharHandler charHandler( _fontType );

        for ( const TextLineInfo & info : lineInfos ) {
            if ( info.characterCount > 0 ) {
                // Center the text line when rendering multi-line texts.
                // TODO: Implement text alignment setting to allow multi-line left aligned text for editor's warning messages.
                const int32_t offsetX = info.offsetX + ( maxWidth - info.lineWidth ) / 2;

                if ( useUnicodeTextPath ) {
                    renderUnicodeSingleLine( std::string_view( reinterpret_cast<const char *>( data ), static_cast<size_t>( info.characterCount ) ), x + offsetX,
                                             y + info.offsetY + getUnicodeTextVerticalOffset( _fontType ), output, imageRoi, _fontType );
                }
                else {
                    renderSingleLine( data, info.characterCount, x + offsetX, y + info.offsetY, output, imageRoi, charHandler );
                }
            }

            data += info.characterCount;
        }
    }

    void Text::fitToOneRow( const int32_t maxWidth )
    {
        assert( maxWidth > 0 ); // Why is the limit less than 1?
        if ( maxWidth <= 0 ) {
            return;
        }

        if ( _text.empty() ) {
            // Nothing needs to be done.
            return;
        }

        const auto languageSwitcher = getLanguageSwitcher( *this );
        if ( shouldUseUnicodeTextPath( _text, _language, _fontType ) ) {
            const int32_t originalTextWidth = getUnicodeLineWidth( _text, _fontType, _keepLineTrailingSpaces );
            if ( originalTextWidth <= maxWidth ) {
                return;
            }

            _text.resize( getUnicodeMaxByteCount( _text, _fontType, maxWidth - getTruncationSymbolWidth( _fontType ) ) );
            _text += truncationSymbol;
            return;
        }

        const FontCharHandler charHandler( _fontType );

        const int32_t originalTextWidth
            = getLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), charHandler, _keepLineTrailingSpaces );
        if ( originalTextWidth <= maxWidth ) {
            // Nothing to do. The text is not longer than the provided maximum width.
            return;
        }

        const int32_t maxCharacterCount = getMaxCharacterCount( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), charHandler,
                                                                maxWidth - getTruncationSymbolWidth( _fontType ) );

        _text.resize( maxCharacterCount );
        _text += truncationSymbol;
    }

    void Text::fitToArea( const int32_t maxWidth, const int32_t maxHeight )
    {
        assert( maxWidth > 0 && maxHeight > 1 ); // Why is the limit less than 1?
        if ( maxWidth <= 0 || maxHeight <= 0 ) {
            return;
        }

        if ( _text.empty() ) {
            // Nothing needs to be done.
            return;
        }

        const auto languageSwitcher = getLanguageSwitcher( *this );
        const bool useUnicodeTextPath = shouldUseUnicodeTextPath( _text, _language, _fontType );
        const FontCharHandler charHandler( _fontType );

        if ( height( maxWidth ) <= maxHeight ) {
            // Nothing we need to do as the text fits to the area.
            return;
        }

        while ( !_text.empty() && ( height( maxWidth ) > maxHeight ) ) {
            if ( useUnicodeTextPath ) {
                removeLastUtf8Character( _text );
            }
            else {
                _text.pop_back();
            }
        }

        // We need to add truncation symbol.
        _text += truncationSymbol;
        while ( height( maxWidth ) > maxHeight ) {
            // Remove the truncation symbol and one more character before it.
            for ( size_t i = 0; i < truncationSymbol.size(); ++i ) {
                _text.pop_back();
            }

            if ( useUnicodeTextPath ) {
                removeLastUtf8Character( _text );
            }
            else {
                _text.pop_back();
            }

            _text += truncationSymbol;
        }
    }

    void Text::_getTextLineInfos( std::vector<TextLineInfo> & textLineInfos, const int32_t maxWidth, const int32_t rowHeight, const bool keepTextTrailingSpaces ) const
    {
        assert( !_text.empty() );

        if ( shouldUseUnicodeTextPath( _text, _language, _fontType ) ) {
            getUnicodeTextLineInfos( textLineInfos, _text, _fontType, maxWidth, rowHeight, _keepLineTrailingSpaces, keepTextTrailingSpaces );
            return;
        }

        const uint8_t * data = reinterpret_cast<const uint8_t *>( _text.data() );

        const int32_t firstLineOffsetX = textLineInfos.empty() ? 0 : textLineInfos.back().lineWidth;
        int32_t lineWidth = firstLineOffsetX;
        int32_t offsetY = textLineInfos.empty() ? 0 : textLineInfos.back().offsetY;

        const FontCharHandler charHandler( _fontType );

        if ( maxWidth < 1 ) {
            // The text will be displayed in a single line.

            const int32_t size = static_cast<int32_t>( _text.size() );
            lineWidth += getLineWidth( data, size, charHandler, _keepLineTrailingSpaces || keepTextTrailingSpaces );

            textLineInfos.emplace_back( firstLineOffsetX, offsetY, lineWidth, size );
            return;
        }

        int32_t offsetX = firstLineOffsetX;
        int32_t lineCharCount = 0;
        int32_t lastWordCharCount = 0;
        int32_t spaceCharCount = 0;

        const uint8_t * dataEnd = data + _text.size();

        while ( data != dataEnd ) {
            if ( isLineSeparator( *data ) ) {
                if ( !_keepLineTrailingSpaces ) {
                    lineWidth -= spaceCharCount * charHandler.getSpaceCharWidth();
                }

                textLineInfos.emplace_back( offsetX, offsetY, lineWidth, lineCharCount + 1 );

                spaceCharCount = 0;
                offsetX = 0;
                offsetY += rowHeight;
                lineCharCount = 0;
                lastWordCharCount = 0;
                lineWidth = 0;

                ++data;
            }
            else {
                // This is another character in the line. Get its width.

                const int32_t charWidth = charHandler.getWidth( *data );

                if ( ( lineWidth != 0 ) && ( lineWidth + charWidth > maxWidth ) ) {
                    // Current character has exceeded the maximum line width
                    // and the character is not bigger than the maximum width.

                    if ( !_keepLineTrailingSpaces && isSpaceChar( *data ) ) {
                        // Current character could be a space character then current line is over.
                        // For the characters count we take this space into the account.
                        ++lineCharCount;

                        // Skip this space character.
                        ++data;
                    }
                    else if ( lineCharCount == lastWordCharCount ) {
                        // This is the only word in the line.
                        // Search for '-' symbol to avoid truncating the word in the middle.
                        const uint8_t * hyphenPos = data - lineCharCount;
                        for ( ; hyphenPos != data; ++hyphenPos ) {
                            if ( *hyphenPos == hyphenChar ) {
                                break;
                            }
                        }

                        if ( hyphenPos != data ) {
                            // The '-' symbol has been found. In this case we consider everything after it as a separate word.
                            const int32_t postHyphenCharCount = static_cast<int32_t>( data - hyphenPos ) - 1;

                            // Only split at the hyphen if there are characters after it to move to the next line.
                            if ( postHyphenCharCount > 0 ) {
                                lineCharCount -= postHyphenCharCount;
                                lineWidth -= getLineWidth( data - postHyphenCharCount, postHyphenCharCount, charHandler, true );

                                data = hyphenPos;
                                ++data;
                            }
                        }
                        else if ( firstLineOffsetX > 0 && ( textLineInfos.empty() || textLineInfos.back().offsetY == offsetY ) ) {
                            // This word was not the first in the line so we can move it to the next line.
                            // It can happen in the case of the multi-font text.
                            data -= lastWordCharCount;

                            lineCharCount = 0;
                            lineWidth = firstLineOffsetX;
                        }
                    }
                    else if ( lastWordCharCount > 0 ) {
                        // Exclude last word from this line.
                        data -= lastWordCharCount;

                        lineCharCount -= lastWordCharCount;
                        lineWidth -= getLineWidth( data, lastWordCharCount, charHandler, true );
                    }

                    if ( !_keepLineTrailingSpaces ) {
                        lineWidth -= spaceCharCount * charHandler.getSpaceCharWidth();
                    }

                    textLineInfos.emplace_back( offsetX, offsetY, lineWidth, lineCharCount );

                    spaceCharCount = 0;
                    offsetX = 0;
                    offsetY += rowHeight;
                    lineCharCount = 0;
                    lastWordCharCount = 0;
                    lineWidth = 0;
                }
                else {
                    if ( isSpaceChar( *data ) ) {
                        lastWordCharCount = 0;
                        ++spaceCharCount;
                    }
                    else {
                        ++lastWordCharCount;
                        spaceCharCount = 0;
                    }

                    ++data;
                    ++lineCharCount;
                    lineWidth += charWidth;
                }
            }
        }

        if ( !_keepLineTrailingSpaces && !keepTextTrailingSpaces ) {
            lineWidth -= spaceCharCount * charHandler.getSpaceCharWidth();
        }

        textLineInfos.emplace_back( offsetX, offsetY, lineWidth, lineCharCount );
    }

    int32_t TextInput::width() const
    {
        if ( _text.empty() ) {
            return 0;
        }

        if ( _isMultiLine && _maxTextWidth > 0 ) {
            // This is a multi-line text.
            return Text::width( _maxTextWidth );
        }

        const auto langugeSwitcher = getLanguageSwitcher( *this );
        if ( shouldUseUnicodeTextPath( _text, _language, _fontType ) ) {
            const std::string_view visibleText( _text.data() + _visibleTextBeginPos, static_cast<size_t>( _visibleTextLength ) );
            const int32_t textWidth = getUnicodeLineWidth( visibleText, _fontType, _keepLineTrailingSpaces );

            const bool isTextTruncatedAtBegin = ( _visibleTextBeginPos != 0 );
            const bool isTextTruncatedAtEnd = ( _visibleTextLength + _visibleTextBeginPos < static_cast<int32_t>( _text.size() ) );

            if ( isTextTruncatedAtBegin && isTextTruncatedAtEnd ) {
                return textWidth + 2 * getTruncationSymbolWidth( _fontType );
            }
            if ( isTextTruncatedAtBegin || isTextTruncatedAtEnd ) {
                return textWidth + getTruncationSymbolWidth( _fontType );
            }

            return textWidth;
        }

        const FontCharHandler charHandler( _fontType );

        const int32_t textWidth
            = getLineWidth( reinterpret_cast<const uint8_t *>( _text.data() + _visibleTextBeginPos ), _visibleTextLength, charHandler, _keepLineTrailingSpaces );

        const bool isTextTruncatedAtBegin = ( _visibleTextBeginPos != 0 );
        const bool isTextTruncatedAtEnd = ( _visibleTextLength + _visibleTextBeginPos < static_cast<int32_t>( _text.size() ) );

        if ( isTextTruncatedAtBegin && isTextTruncatedAtEnd ) {
            return textWidth + 2 * getTruncationSymbolWidth( _fontType );
        }
        if ( isTextTruncatedAtBegin || isTextTruncatedAtEnd ) {
            return textWidth + getTruncationSymbolWidth( _fontType );
        }

        return textWidth;
    }

    void TextInput::drawInRoi( const int32_t x, const int32_t y, Image & output, const Rect & imageRoi ) const
    {
        if ( output.empty() || _text.empty() || _visibleTextLength == 0 ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        if ( _isMultiLine && _maxTextWidth > 0 ) {
            Text::drawInRoi( x, y, _maxTextWidth, output, imageRoi );
            return;
        }

        const auto langugeSwitcher = getLanguageSwitcher( *this );
        if ( shouldUseUnicodeTextPath( _text, _language, _fontType ) ) {
            int32_t offsetX = x;

            if ( _visibleTextBeginPos != 0 ) {
                offsetX = renderUnicodeSingleLine( truncationSymbol, x, y, output, imageRoi, _fontType );
            }

            offsetX = renderUnicodeSingleLine( std::string_view( _text.data() + _visibleTextBeginPos,
                                                                 _visibleTextLength == 0
                                                                     ? static_cast<size_t>( static_cast<int32_t>( _text.size() ) - _visibleTextBeginPos )
                                                                     : static_cast<size_t>( _visibleTextLength ) ),
                                               offsetX, y, output, imageRoi, _fontType );

            if ( _visibleTextLength + _visibleTextBeginPos < static_cast<int32_t>( _text.size() ) ) {
                renderUnicodeSingleLine( truncationSymbol, offsetX, y, output, imageRoi, _fontType );
            }

            return;
        }

        const FontCharHandler charHandler( _fontType );

        int32_t offsetX = x;

        if ( _visibleTextBeginPos != 0 ) {
            // Insert truncation symbol at the beginning.
            offsetX = renderSingleLine( reinterpret_cast<const uint8_t *>( truncationSymbol.data() ), static_cast<int32_t>( truncationSymbol.size() ), x, y, output,
                                        imageRoi, charHandler );
        }

        offsetX = renderSingleLine( reinterpret_cast<const uint8_t *>( _text.data() ) + _visibleTextBeginPos,
                                    _visibleTextLength == 0 ? static_cast<int32_t>( _text.size() ) - _visibleTextBeginPos : _visibleTextLength, offsetX, y, output,
                                    imageRoi, charHandler );

        // Insert truncation symbol at the end if required.
        if ( _visibleTextLength + _visibleTextBeginPos < static_cast<int32_t>( _text.size() ) ) {
            renderSingleLine( reinterpret_cast<const uint8_t *>( truncationSymbol.data() ), static_cast<int32_t>( truncationSymbol.size() ), offsetX, y, output, imageRoi,
                              charHandler );
        }
    }

    size_t TextInput::getCursorPositionInAdjacentLine( const size_t currentPos, const int32_t maxWidth, const bool moveUp )
    {
        std::vector<TextLineInfo> tempLineInfos;
        _getTextLineInfos( tempLineInfos, maxWidth, height(), true );
        if ( tempLineInfos.empty() ) {
            return currentPos;
        }

        size_t currentLineNumber = 0;
        size_t numberOfCharacters = 0;
        while ( numberOfCharacters + tempLineInfos[currentLineNumber].characterCount <= currentPos && currentLineNumber < tempLineInfos.size() - 1 ) {
            numberOfCharacters += tempLineInfos[currentLineNumber].characterCount;
            ++currentLineNumber;
        }

        size_t targetLineNumber = 0;
        if ( moveUp ) {
            if ( currentLineNumber == 0 ) {
                return currentPos;
            }
            targetLineNumber = currentLineNumber - 1;
        }
        else {
            if ( currentLineNumber == tempLineInfos.size() - 1 ) {
                return currentPos;
            }
            targetLineNumber = currentLineNumber + 1;
        }

        const fheroes2::FontCharHandler charHandler( _fontType );

        auto countCharacters = []( const size_t count, const TextLineInfo & textLineInfo ) { return count + textLineInfo.characterCount; };
        const size_t currentLineStartPos = std::accumulate( tempLineInfos.data(), &tempLineInfos[currentLineNumber], size_t{ 0 }, countCharacters );
        const size_t targetLineStartPos = std::accumulate( tempLineInfos.data(), &tempLineInfos[targetLineNumber], size_t{ 0 }, countCharacters );

        // TODO update those line once we support different alignment in multi-line text.
        const int32_t currentXPos = ( ( maxWidth - tempLineInfos[currentLineNumber].lineWidth ) / 2 )
                                    + charHandler.getWidth( std::string_view( &_text[currentLineStartPos], currentPos - currentLineStartPos ) );
        const int32_t targetLineXOffset = ( maxWidth - tempLineInfos[targetLineNumber].lineWidth ) / 2;

        size_t bestPos = targetLineStartPos;
        int32_t bestDistance = std::abs( currentXPos - targetLineXOffset );

        int32_t targetXPos = targetLineXOffset;
        for ( int32_t i = 0; i < tempLineInfos[targetLineNumber].characterCount + 1; ++i ) {
            const size_t textPos = targetLineStartPos + i;
            const int32_t distance = std::abs( currentXPos - targetXPos );
            if ( distance < bestDistance ) {
                bestDistance = distance;
                bestPos = textPos;
            }

            if ( textPos < _text.size() ) {
                targetXPos += charHandler.getWidth( static_cast<uint8_t>( _text[textPos] ) );
            }
        }

        return bestPos;
    }

    void TextInput::fitToOneRow( const int32_t maxWidth )
    {
        assert( maxWidth > 0 );
        if ( maxWidth <= 0 || _text.empty() ) {
            _visibleTextBeginPos = 0;
            _visibleTextLength = 0;

            return;
        }

        const auto langugeSwitcher = getLanguageSwitcher( *this );
        if ( shouldUseUnicodeTextPath( _text, _language, _fontType ) ) {
            _visibleTextBeginPos = 0;
            _visibleTextLength = static_cast<int32_t>( _text.size() );

            const int32_t originalTextWidth = getUnicodeLineWidth( _text, _fontType, true );
            if ( originalTextWidth < maxWidth ) {
                return;
            }

            _visibleTextLength = getUnicodeMaxByteCount( _text, _fontType, maxWidth - getTruncationSymbolWidth( _fontType ) );
            return;
        }

        const FontCharHandler charHandler( _fontType );
        const uint8_t * textData = reinterpret_cast<const uint8_t *>( _text.data() );
        _visibleTextLength = static_cast<int32_t>( _text.size() );

        const int32_t originalTextWidth = getLineWidth( textData, _visibleTextLength, charHandler, true );
        if ( originalTextWidth < maxWidth ) {
            // There is no need to fit the text. Reset text fitting values.
            _visibleTextBeginPos = 0;

            return;
        }

        // We want to keep cursor not close to the truncated text edges.
        constexpr int32_t cursorToTruncationDistance = 4;

        // First we update the left text truncation position by updating the visible text beginning position.
        // If the cursor is to the left of or close to the visible text begin position, we update this position.
        // And we don't allow the begin position to be negative.
        _visibleTextBeginPos
            = ( _cursorPositionInText > cursorToTruncationDistance ) ? std::min( _visibleTextBeginPos, _cursorPositionInText - cursorToTruncationDistance ) : 0;

        assert( _visibleTextBeginPos <= _visibleTextLength );

        _visibleTextLength -= _visibleTextBeginPos;

        int32_t currentWidth = getLineWidth( textData + _visibleTextBeginPos, _visibleTextLength, charHandler, true );
        const int32_t truncationSymbolWidth = getTruncationSymbolWidth( _fontType );
        const int32_t truncatedMaxWidth = maxWidth - truncationSymbolWidth;
        const int32_t twiceTruncatedMaxWidth = truncatedMaxWidth - truncationSymbolWidth;

        // If the text should be truncated from the right side.
        if ( currentWidth > truncatedMaxWidth ) {
            _visibleTextLength = getMaxCharacterCount( textData + _visibleTextBeginPos, _visibleTextLength, charHandler,
                                                       _visibleTextBeginPos != 0 ? twiceTruncatedMaxWidth : truncatedMaxWidth );
            currentWidth = getLineWidth( textData + _visibleTextBeginPos, _visibleTextLength, charHandler, true );
        }

        const int32_t originalTextSize = static_cast<int32_t>( _text.size() );
        int32_t textEndPos = _visibleTextBeginPos + _visibleTextLength;

        // If the cursor is to the right of or close to the visible text end position, we update this position to show the cursor and text around it.
        while ( ( textEndPos < _cursorPositionInText + cursorToTruncationDistance ) && ( textEndPos < originalTextSize ) ) {
            currentWidth += charHandler.getWidth( textData[textEndPos] );
            ++textEndPos;
            ++_visibleTextLength;

            while ( currentWidth > ( textEndPos != originalTextSize ? twiceTruncatedMaxWidth : truncatedMaxWidth ) ) {
                // Remove characters from the begin.
                currentWidth -= charHandler.getWidth( textData[_visibleTextBeginPos] );
                ++_visibleTextBeginPos;
                --_visibleTextLength;
            }

            while ( textEndPos != originalTextSize ) {
                // Some characters from the text end might also fit the width.
                const int32_t charWidth = charHandler.getWidth( textData[textEndPos] );
                if ( currentWidth + charWidth > twiceTruncatedMaxWidth ) {
                    break;
                }

                currentWidth += charHandler.getWidth( textData[textEndPos] );
                ++textEndPos;
                ++_visibleTextLength;
            }
        }

        assert( _cursorPositionInText >= _visibleTextBeginPos );

        while ( _visibleTextBeginPos != 0 ) {
            // Some characters from the text begin might also fit the width.
            const int32_t prevCharWidth = charHandler.getWidth( textData[_visibleTextBeginPos - 1] );
            if ( currentWidth + prevCharWidth > ( textEndPos != originalTextSize ? twiceTruncatedMaxWidth : truncatedMaxWidth ) ) {
                break;
            }

            --_visibleTextBeginPos;
            ++_visibleTextLength;
            currentWidth += prevCharWidth;
        }

        if ( textEndPos != originalTextSize && textEndPos + cursorToTruncationDistance > originalTextSize
             && getLineWidth( textData + textEndPos, originalTextSize - textEndPos, charHandler, true ) <= truncatedMaxWidth - currentWidth ) {
            // The end of the text fits the given width. Update text length to remove truncation at the end.
            _visibleTextLength = originalTextSize - _visibleTextBeginPos;
        }

        if ( _visibleTextBeginPos > 0 && _visibleTextBeginPos < cursorToTruncationDistance
             && getLineWidth( textData, _visibleTextBeginPos, charHandler, true ) <= truncatedMaxWidth - currentWidth ) {
            // The beginning of the text fits the given width. Update _textBeginPos to remove truncation at the beginning.
            _visibleTextBeginPos = 0;
            _visibleTextLength = getMaxCharacterCount( textData, originalTextSize, charHandler, truncatedMaxWidth );
        }
    }

    size_t TextInput::_getMultiLineTextInputCursorPosition( const Point & cursorOffset, const Rect & roi ) const
    {
        if ( _text.empty() || roi.width < 1 || roi.height < 1 ) {
            // The text is empty.
            return 0;
        }

        const int32_t fontHeight = getFontHeight( _fontType.size );
        const int32_t pointerLine = ( cursorOffset.y - roi.y ) / fontHeight;

        if ( pointerLine < 0 ) {
            // Pointer is upper than the first text line.
            return 0;
        }

        std::vector<TextLineInfo> lineInfos;
        _getTextLineInfos( lineInfos, _maxTextWidth, fontHeight, true );

        if ( pointerLine >= static_cast<int32_t>( lineInfos.size() ) ) {
            // Pointer is lower than the last text line.
            return _text.size() - 1;
        }

        size_t cursorPosition = 0;
        for ( int32_t i = 0; i < pointerLine; ++i ) {
            cursorPosition += lineInfos[i].characterCount;
        }

        int32_t positionOffsetX = 0;
        const int32_t maxOffsetX = cursorOffset.x - roi.x - ( _maxTextWidth - lineInfos[pointerLine].lineWidth ) / 2;

        if ( maxOffsetX <= 0 ) {
            // Pointer is to the left of the text line.
            return cursorPosition;
        }

        if ( maxOffsetX > lineInfos[pointerLine].lineWidth ) {
            // Pointer is to the right of the text line.
            cursorPosition += lineInfos[pointerLine].characterCount;

            return cursorPosition;
        }

        const FontCharHandler charHandler( _fontType );
        const size_t textSize = _text.size();

        for ( size_t i = cursorPosition; i < textSize; ++i ) {
            const int32_t charWidth = charHandler.getWidth( static_cast<uint8_t>( _text[i] ) );

            if ( positionOffsetX + charWidth / 2 >= maxOffsetX ) {
                return i;
            }

            positionOffsetX += charWidth;
        }

        return textSize - 1;
    }

    size_t TextInput::_getTextInputCursorPosition( const int32_t cursorOffsetX, const Rect & roi, const bool isCenterAligned ) const
    {
        if ( _text.empty() ) {
            return 0;
        }

        const int32_t textStartOffsetX
            = roi.x + ( isCenterAligned ? ( roi.width - width() ) / 2 : 0 ) + ( _visibleTextBeginPos == 0 ? 0 : getTruncationSymbolWidth( _fontType ) );

        if ( cursorOffsetX <= textStartOffsetX ) {
            // The text is empty or mouse cursor position is to the left of input field.
            return _visibleTextBeginPos;
        }

        const int32_t maxOffset = cursorOffsetX - textStartOffsetX;
        const std::string visibleText = { ( _text.data() ) + _visibleTextBeginPos, static_cast<size_t>( _visibleTextLength ) };
        const size_t textSize = visibleText.size();
        int32_t positionOffset = 0;
        const FontCharHandler charHandler( _fontType );

        for ( size_t i = 0; i < textSize; ++i ) {
            const int32_t currentCharWidth = charHandler.getWidth( static_cast<uint8_t>( visibleText[i] ) );

            if ( positionOffset + currentCharWidth / 2 >= maxOffset ) {
                return i + _visibleTextBeginPos;
            }
            positionOffset += currentCharWidth;
        }

        return textSize + _visibleTextBeginPos;
    }

    void TextInput::_updateCursorAreaInText()
    {
        if ( !_isMultiLine && _maxTextWidth > 0 ) {
            fitToOneRow( _maxTextWidth );
        }

        const auto langugeSwitcher = getLanguageSwitcher( *this );

        const FontCharHandler charHandler( _fontType );

        const Sprite & charSprite = charHandler.getSprite( cursorChar );
        assert( !charSprite.empty() );

        _cursorArea.width = charSprite.width();
        _cursorArea.height = charSprite.height();
        _cursorArea.x = charSprite.x() - _cursorArea.width / 2;
        // Move cursor symbol one pixel up by deducting 1 from y.
        _cursorArea.y = charSprite.y() - 1;

        if ( _text.empty() ) {
            if ( _isMultiLine && _maxTextWidth > 0 ) {
                _cursorArea.x += _maxTextWidth / 2;
            }
            return;
        }

        int32_t textLineBegin = _visibleTextBeginPos;

        if ( _isMultiLine && _maxTextWidth > 0 ) {
            // This is a multi-line text.

            const int32_t textHeight = height();
            std::vector<TextLineInfo> lineInfos;
            _getTextLineInfos( lineInfos, _maxTextWidth, textHeight, true );

            if ( _cursorPositionInText == static_cast<int32_t>( _text.size() ) ) {
                // The cursor is at the end of the text.
                _cursorArea.y += static_cast<int32_t>( lineInfos.size() - 1 ) * textHeight;
                _cursorArea.x += ( _maxTextWidth + lineInfos.back().lineWidth ) / 2;
                return;
            }

            textLineBegin = 0;

            for ( size_t i = 0; i < lineInfos.size(); ++i ) {
                if ( _cursorPositionInText < textLineBegin + lineInfos[i].characterCount ) {
                    _cursorArea.x += ( _maxTextWidth - lineInfos[i].lineWidth ) / 2;
                    _cursorArea.y += static_cast<int32_t>( i ) * textHeight;
                    break;
                }

                textLineBegin += lineInfos[i].characterCount;
            }
        }
        else if ( _visibleTextBeginPos != 0 ) {
            // The visible text is truncated at the begin.
            _cursorArea.x += getTruncationSymbolWidth( _fontType );
        }

        if ( _cursorPositionInText > textLineBegin ) {
            _cursorArea.x += getLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ) + textLineBegin, _cursorPositionInText - textLineBegin, charHandler, true );
        }
    }

    MultiFontText::~MultiFontText() = default;

    void MultiFontText::add( Text text )
    {
        if ( !text._text.empty() ) {
            _texts.emplace_back( std::move( text ) );
        }
    }

    int32_t MultiFontText::width() const
    {
        int32_t totalWidth = 0;
        for ( const Text & text : _texts ) {
            totalWidth += text.width();
        }

        return totalWidth;
    }

    int32_t MultiFontText::height() const
    {
        int32_t maxHeight = 0;

        for ( const Text & text : _texts ) {
            maxHeight = std::max( maxHeight, text.height() );
        }

        return maxHeight;
    }

    int32_t MultiFontText::width( const int32_t maxWidth ) const
    {
        const int32_t maxFontHeight = height();

        std::vector<TextLineInfo> lineInfos;
        _getMultiFontTextLineInfos( lineInfos, maxWidth, maxFontHeight );

        int32_t maxRowWidth = lineInfos.front().lineWidth;
        for ( const TextLineInfo & lineInfo : lineInfos ) {
            maxRowWidth = std::max( maxRowWidth, lineInfo.lineWidth );
        }

        return maxRowWidth;
    }

    int32_t MultiFontText::height( const int32_t maxWidth ) const
    {
        const int32_t maxFontHeight = height();

        std::vector<TextLineInfo> lineInfos;
        _getMultiFontTextLineInfos( lineInfos, maxWidth, maxFontHeight );

        return lineInfos.back().offsetY + maxFontHeight;
    }

    int32_t MultiFontText::rows( const int32_t maxWidth ) const
    {
        if ( _texts.empty() ) {
            return 0;
        }

        const int32_t maxFontHeight = height();

        std::vector<TextLineInfo> lineInfos;
        _getMultiFontTextLineInfos( lineInfos, maxWidth, maxFontHeight );

        if ( lineInfos.empty() ) {
            return 0;
        }

        return static_cast<int32_t>( lineInfos.size() );
    }

    Rect MultiFontText::area() const
    {
        Rect area;
        bool isFirstText = true;

        for ( const Text & text : _texts ) {
            if ( isFirstText ) {
                isFirstText = false;
                area = text.area();
                continue;
            }

            const Rect & textArea = text.area();

            if ( textArea.y < area.y ) {
                // This character sprite is drawn higher than all previous - update `height` and `y`.
                area.height += area.y - textArea.y;
                area.y = textArea.y;
                area.height = std::max( area.height, textArea.height );
            }
            else {
                area.height = std::max( area.height, textArea.y - area.y + textArea.height );
            }

            area.width += textArea.x + textArea.width;
        }

        return area;
    }

    void MultiFontText::drawInRoi( const int32_t x, const int32_t y, Image & output, const Rect & imageRoi ) const
    {
        if ( output.empty() || _texts.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        const int32_t maxFontHeight = height();

        int32_t offsetX = x;
        for ( const Text & text : _texts ) {
            const auto languageSwitcher = getLanguageSwitcher( text );
            const bool useUnicodeTextPath = shouldUseUnicodeTextPath( text._text, text._language, text._fontType );
            const int32_t fontHeight = useUnicodeTextPath ? getUnicodeFontHeight( text._fontType.size ) : getFontHeight( text._fontType.size );

            if ( useUnicodeTextPath ) {
                offsetX = renderUnicodeSingleLine( text._text, offsetX, y + ( maxFontHeight - fontHeight ) / 2 + getUnicodeTextVerticalOffset( text._fontType ),
                                                   output, imageRoi, text._fontType );
            }
            else {
                const FontCharHandler charHandler( text._fontType );
                offsetX = renderSingleLine( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), offsetX,
                                            y + ( maxFontHeight - fontHeight ) / 2, output, imageRoi, charHandler );
            }
        }
    }

    void MultiFontText::drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const
    {
        if ( output.empty() || _texts.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        const int32_t maxFontHeight = height();

        std::vector<TextLineInfo> lineInfos;
        _getMultiFontTextLineInfos( lineInfos, maxWidth, maxFontHeight );

        if ( lineInfos.empty() ) {
            return;
        }

        // One line can contain text with a different font. Calculate the width of each line.
        std::vector<int32_t> lineWidths;
        lineWidths.reserve( lineInfos.size() );

        int32_t offsetY = 0;

        for ( const TextLineInfo & info : lineInfos ) {
            if ( lineWidths.empty() || offsetY != info.offsetY ) {
                lineWidths.push_back( info.lineWidth );
                offsetY = info.offsetY;
            }
            else {
                lineWidths.back() = info.lineWidth;
            }
        }

        auto widthIter = lineWidths.cbegin();
        auto infoIter = lineInfos.cbegin();

        for ( const Text & singleText : _texts ) {
            const auto languageSwitcher = getLanguageSwitcher( singleText );
            const uint8_t * data = reinterpret_cast<const uint8_t *>( singleText._text.data() );

            const uint8_t * dataEnd = data + singleText._text.size();

            const bool useUnicodeTextPath = shouldUseUnicodeTextPath( singleText._text, singleText._language, singleText._fontType );
            const FontCharHandler charHandler( singleText._fontType );

            while ( data < dataEnd ) {
                if ( infoIter->characterCount > 0 ) {
                    const int32_t offsetX = x + ( maxWidth > 0 ? ( maxWidth - *widthIter ) / 2 : 0 );

                    if ( useUnicodeTextPath ) {
                        renderUnicodeSingleLine( std::string_view( reinterpret_cast<const char *>( data ), static_cast<size_t>( infoIter->characterCount ) ),
                                                 offsetX + infoIter->offsetX, y + infoIter->offsetY + getUnicodeTextVerticalOffset( singleText._fontType ), output,
                                                 imageRoi, singleText._fontType );
                    }
                    else {
                        renderSingleLine( data, infoIter->characterCount, offsetX + infoIter->offsetX, y + infoIter->offsetY, output, imageRoi, charHandler );
                    }
                }

                data += infoIter->characterCount;

                ++widthIter;
                ++infoIter;
            }

            --widthIter;
        }
    }

    void MultiFontText::fitToOneRow( const int32_t maxWidth )
    {
        int32_t widthLeft = maxWidth;

        for ( size_t i = 0; i < _texts.size(); ++i ) {
            const auto languageSwitcher = getLanguageSwitcher( _texts[i] );

            const bool useUnicodeTextPath = shouldUseUnicodeTextPath( _texts[i]._text, _texts[i]._language, _texts[i]._fontType );
            const FontCharHandler charHandler( _texts[i]._fontType );

            const int32_t originalTextWidth = useUnicodeTextPath
                                                  ? getUnicodeLineWidth( _texts[i]._text, _texts[i]._fontType, true )
                                                  : getLineWidth( reinterpret_cast<const uint8_t *>( _texts[i]._text.data() ),
                                                                  static_cast<int32_t>( _texts[i]._text.size() ), charHandler, true );

            if ( ( i + 1 == _texts.size() ) && originalTextWidth <= widthLeft ) {
                // This is the last text and all texts fit the given width.
                break;
            }

            // This is not the last text and we need to keep space for the possible truncation symbol.
            const int32_t correctedWidthLeft = widthLeft - getTruncationSymbolWidth( _texts[i]._fontType );
            if ( originalTextWidth > correctedWidthLeft ) {
                // The text does not fit the given width.

                const int32_t maxCharacterCount
                    = useUnicodeTextPath ? getUnicodeMaxByteCount( _texts[i]._text, _texts[i]._fontType, correctedWidthLeft )
                                         : getMaxCharacterCount( reinterpret_cast<const uint8_t *>( _texts[i]._text.data() ),
                                                                 static_cast<int32_t>( _texts[i]._text.size() ), charHandler, correctedWidthLeft );

                // Remove the characters that do not fit the given width.
                _texts[i]._text.resize( maxCharacterCount );
                _texts[i]._text += truncationSymbol;

                // Remove other texts that do not fit the given width.
                _texts.resize( i + 1 );

                break;
            }

            // The text is not longer than the provided maximum width. Go to the next text.
            widthLeft -= originalTextWidth;
        }
    }

    std::string MultiFontText::text() const
    {
        std::string output;

        for ( const Text & singleText : _texts ) {
            output += singleText.text();
        }

        return output;
    }

    void MultiFontText::_getMultiFontTextLineInfos( std::vector<TextLineInfo> & textLineInfos, const int32_t maxWidth, const int32_t rowHeight ) const
    {
        const size_t textsCount = _texts.size();
        for ( size_t i = 0; i < textsCount; ++i ) {
            const auto languageSwitcher = getLanguageSwitcher( _texts[i] );

            // To properly render a multi-font text we must not ignore spaces at the end of a text entry which is not the last one.
            const bool isNotLastTextEntry = ( i != textsCount - 1 );

            _texts[i]._getTextLineInfos( textLineInfos, maxWidth, rowHeight, isNotLastTextEntry );
        }
    }

    FontCharHandler::FontCharHandler( const FontType fontType )
        : _fontType( fontType )
        , _charLimit( getCharacterLimit( fontType.size ) )
        , _spaceCharWidth( _getSpaceCharWidth() )
    {
        // Do nothing.
    }

    bool FontCharHandler::isAvailable( const uint8_t character ) const
    {
        return ( isSpaceChar( character ) || _isValid( character ) || isLineSeparator( character ) );
    }

    const Sprite & FontCharHandler::getSprite( const uint8_t character ) const
    {
        // Display '?' in place of the invalid character.
        return getChar( _isValid( character ) ? character : invalidChar, _fontType );
    }

    int32_t FontCharHandler::getWidth( const uint8_t character ) const
    {
        if ( isSpaceChar( character ) ) {
            return _spaceCharWidth;
        }

        if ( isLineSeparator( character ) ) {
            return 0;
        }

        const Sprite & image = getSprite( character );

        assert( ( _fontType.size != FontSize::BUTTON_RELEASED && _fontType.size != FontSize::BUTTON_PRESSED && image.x() >= 0 ) || image.x() < 0 );

        return image.x() + image.width();
    }

    int32_t FontCharHandler::getWidth( const std::string_view text ) const
    {
        int32_t width = 0;
        for ( const char c : text ) {
            width += getWidth( c );
        }
        return width;
    }

    int32_t FontCharHandler::_getSpaceCharWidth() const
    {
        switch ( _fontType.size ) {
        case FontSize::SMALL:
            return 4;
        case FontSize::NORMAL:
            return 6;
        case FontSize::LARGE:
        case FontSize::BUTTON_RELEASED:
        case FontSize::BUTTON_PRESSED:
            return 8;
        default:
            // Did you add a new font size? Please add implementation.
            assert( 0 );
            break;
        }

        return 0;
    }

    bool isFontAvailable( const std::string_view text, const FontType fontType )
    {
        if ( text.empty() ) {
            return true;
        }

        if ( shouldUseUnicodeTextPath( text, std::nullopt, fontType ) ) {
            return true;
        }

        const FontCharHandler charHandler( fontType );

        return std::all_of( text.begin(), text.end(), [&charHandler]( const int8_t character ) { return charHandler.isAvailable( character ); } );
    }

    bool isCjkTextRenderingAvailable()
    {
#if defined( WITH_CJK_TEXT )
        return getCjkFontRenderer().isReady();
#else
        return false;
#endif
    }

    void releaseCjkTextResources()
    {
#if defined( WITH_CJK_TEXT )
        // Release the fonts and shut SDL_ttf down explicitly so that it happens while SDL is still alive,
        // instead of relying on the destruction order of the function-local renderer singleton at program exit.
        getCjkFontRenderer().release();
#endif
    }

    int32_t getTruncationSymbolWidth( const FontType fontType )
    {
        if ( shouldUseUnicodeTextPath( truncationSymbol, std::nullopt, fontType ) ) {
            return getUnicodeLineWidth( truncationSymbol, fontType, true );
        }

        // Symbol width depends on font size and not on its color.
        static std::map<FontSize, int32_t> truncationSymbolWidth;

        auto [iter, isEmplaced] = truncationSymbolWidth.try_emplace( fontType.size, 0 );

        if ( isEmplaced ) {
            // Set the correct width value for the just emplaced element.
            iter->second = getLineWidth( reinterpret_cast<const uint8_t *>( truncationSymbol.data() ), static_cast<int32_t>( truncationSymbol.size() ),
                                         FontCharHandler( fontType ), true );
        }

        return iter->second;
    }

    const Sprite & getCursorSprite( const FontType type )
    {
        return FontCharHandler{ type }.getSprite( cursorChar );
    }
}
