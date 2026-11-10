/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.              *
 ***************************************************************************/

#include "ui_font_ttf.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <SDL_ttf.h>

#include "dir.h"
#include "logging.h"
#include "settings.h"
#include "ui_language.h"
#include "ui_text.h"

namespace
{
    // The bitmap fonts in the engine use palette indices 10 - 36 as an anti-aliasing gradient (index 36 is the brightest color).
    // Different font colors use different index ranges which are taken from the font palette conversion tables in pal.cpp.
    constexpr uint8_t glyphIndexStart = 10;
    constexpr uint8_t glyphIndexEnd = 36; // full coverage

    constexpr uint8_t yellowIndexStart = 114;
    constexpr uint8_t yellowIndexEnd = 130;

    constexpr uint8_t grayIndexStart = 15;
    constexpr uint8_t grayIndexEnd = 36;

    // CJK glyphs are rendered at a higher resolution and then box-filtered down to the target size.
    // Supersampling avoids the distorted strokes that hinting produces when rasterizing dense CJK
    // glyphs directly at tiny pixel sizes (10 - 16 px).
    constexpr int32_t glyphRenderScale = 4;

    // Alpha values below this threshold are treated as fully transparent.
    constexpr uint8_t alphaMinThreshold = 12;

    int32_t getRowHeight( const fheroes2::FontSize size )
    {
        return fheroes2::getFontHeight( size );
    }

    int32_t getTtfPixelSize( const fheroes2::FontSize size )
    {
        switch ( size ) {
        case fheroes2::FontSize::SMALL:
            return 10;
        case fheroes2::FontSize::NORMAL:
            return 16;
        case fheroes2::FontSize::LARGE:
            return 32;
        case fheroes2::FontSize::BUTTON_RELEASED:
        case fheroes2::FontSize::BUTTON_PRESSED:
            return 14;
        default:
            assert( 0 );
            break;
        }

        return 15;
    }

    // Boosts the mid-tones of the anti-aliasing gradient so that glyph strokes read as solid and crisp
    // instead of muddy gray. Smaller fonts need a stronger boost to stay legible on dark backgrounds.
    uint8_t applyAlphaGamma( const uint8_t alpha, const fheroes2::FontSize size )
    {
        double gamma = 0.8;
        switch ( size ) {
        case fheroes2::FontSize::SMALL:
            gamma = 0.6;
            break;
        case fheroes2::FontSize::BUTTON_RELEASED:
        case fheroes2::FontSize::BUTTON_PRESSED:
            gamma = 0.7;
            break;
        case fheroes2::FontSize::NORMAL:
            gamma = 0.8;
            break;
        case fheroes2::FontSize::LARGE:
            gamma = 0.9;
            break;
        default:
            break;
        }

        const double normalized = alpha / 255.0;
        const double boosted = std::pow( normalized, gamma );

        return static_cast<uint8_t>( std::min( 255.0, boosted * 255.0 + 0.5 ) );
    }

    uint8_t mapAlphaToIndex( const uint8_t alpha, const fheroes2::FontColor color )
    {
        if ( alpha <= 8 ) {
            // Fully transparent.
            return 0;
        }

        // IMPORTANT: in the original game's palette the font gradients run from bright to dark:
        // solid glyph pixels use the LOWEST index of the range (white #FCF0F0 at index 10,
        // golden yellow #E4C814 at index 114), while anti-aliased edges use higher indices
        // which are progressively darker. Therefore higher alpha must map to a LOWER index.
        const int32_t alphaValue = 255 - alpha;
        int32_t index = 0;

        switch ( color ) {
        case fheroes2::FontColor::WHITE:
        case fheroes2::FontColor::SILVER_GRADIENT:
            index = glyphIndexStart + ( ( alphaValue * ( glyphIndexEnd - glyphIndexStart ) ) / 255 );
            break;
        case fheroes2::FontColor::GRAY:
            index = grayIndexStart + ( ( alphaValue * ( grayIndexEnd - grayIndexStart ) ) / 255 );
            break;
        case fheroes2::FontColor::YELLOW:
        case fheroes2::FontColor::GOLDEN_GRADIENT:
            index = yellowIndexStart + ( ( alphaValue * ( yellowIndexEnd - yellowIndexStart ) ) / 255 );
            break;
        default:
            assert( 0 );
            index = glyphIndexStart;
            break;
        }

        return static_cast<uint8_t>( index );
    }

    SDL_Color getColor( const fheroes2::FontColor color )
    {
        switch ( color ) {
        case fheroes2::FontColor::WHITE:
        case fheroes2::FontColor::SILVER_GRADIENT:
            return { 255, 255, 255, 255 };
        case fheroes2::FontColor::GRAY:
            return { 176, 176, 176, 255 };
        case fheroes2::FontColor::YELLOW:
            return { 255, 224, 64, 255 };
        case fheroes2::FontColor::GOLDEN_GRADIENT:
            return { 255, 215, 0, 255 };
        default:
            assert( 0 );
            break;
        }

        return { 255, 255, 255, 255 };
    }

    class TtfFontLibrary
    {
    public:
        TtfFontLibrary() = default;
        TtfFontLibrary( const TtfFontLibrary & ) = delete;
        TtfFontLibrary & operator=( const TtfFontLibrary & ) = delete;

        ~TtfFontLibrary()
        {
            for ( TTF_Font * font : _fonts ) {
                TTF_CloseFont( font );
            }

            if ( TTF_WasInit() ) {
                TTF_Quit();
            }
        }

        bool load()
        {
            if ( _loaded ) {
                return true;
            }

            const std::string fontPath = findFontFile();
            if ( fontPath.empty() ) {
                ERROR_LOG( "No CJK TrueType font found. Please place a .ttf/.ttc/.otf file (for example simhei.ttf) into the 'files/fonts' directory." )
                return false;
            }

            if ( TTF_Init() != 0 ) {
                ERROR_LOG( "Unable to initialize SDL_ttf: " << TTF_GetError() )
                return false;
            }

            // Fonts are loaded at glyphRenderScale times the target pixel size: glyphs are rasterized
            // at high resolution and box-filtered down. Hinting is disabled so that the downsampled
            // result stays smooth and uniform.
            for ( const int32_t pixelSize : { getTtfPixelSize( fheroes2::FontSize::SMALL ) * glyphRenderScale,
                                              getTtfPixelSize( fheroes2::FontSize::BUTTON_RELEASED ) * glyphRenderScale,
                                              getTtfPixelSize( fheroes2::FontSize::NORMAL ) * glyphRenderScale,
                                              getTtfPixelSize( fheroes2::FontSize::LARGE ) * glyphRenderScale } ) {
                TTF_Font * font = TTF_OpenFont( fontPath.c_str(), pixelSize );
                if ( font == nullptr ) {
                    ERROR_LOG( "Unable to load CJK font " << fontPath << ": " << TTF_GetError() )
                    return false;
                }

                TTF_SetFontHinting( font, TTF_HINTING_NONE );
                _fonts.push_back( font );
            }

            _loaded = true;
            DEBUG_LOG( DBG_ENGINE, DBG_INFO, "CJK font " << fontPath << " has been loaded" )

            return true;
        }

        TTF_Font * getFont( const fheroes2::FontSize size )
        {
            if ( !_loaded && !load() ) {
                return nullptr;
            }

            switch ( size ) {
            case fheroes2::FontSize::SMALL:
                return _fonts[0];
            case fheroes2::FontSize::BUTTON_RELEASED:
            case fheroes2::FontSize::BUTTON_PRESSED:
                return _fonts[1];
            case fheroes2::FontSize::NORMAL:
                return _fonts[2];
            case fheroes2::FontSize::LARGE:
                return _fonts[3];
            default:
                assert( 0 );
                break;
            }

            return nullptr;
        }

    private:
        static std::string findFontFile()
        {
            const std::vector<std::string> fontExtensions = { ".ttf", ".ttc", ".otf" };

            // First, search in the game directory: <root>/files/fonts/.
            const ListFiles files = Settings::FindFiles( "files/fonts", "", false );
            for ( const std::string & file : files ) {
                for ( const std::string & ext : fontExtensions ) {
                    if ( file.size() > ext.size() && file.compare( file.size() - ext.size(), ext.size(), ext ) == 0 ) {
                        return file;
                    }
                }
            }

            // Fallback to well-known system fonts with CJK support (Windows).
            const std::vector<std::string> systemFonts = { "C:\\Windows\\Fonts\\msyh.ttc", "C:\\Windows\\Fonts\\msyh.ttf", "C:\\Windows\\Fonts\\simhei.ttf",
                                                           "C:\\Windows\\Fonts\\simsun.ttc", "C:\\Windows\\Fonts\\msjh.ttc", "C:\\Windows\\Fonts\\Deng.ttf" };

            for ( const std::string & path : systemFonts ) {
                std::ifstream stream( path, std::ios_base::binary );
                if ( stream.is_open() ) {
                    return path;
                }
            }

            return {};
        }

        bool _loaded{ false };
        std::vector<TTF_Font *> _fonts;
    };

    TtfFontLibrary & getFontLibrary()
    {
        static TtfFontLibrary library;
        return library;
    }

    std::map<uint64_t, fheroes2::Sprite> & getGlyphCache()
    {
        static std::map<uint64_t, fheroes2::Sprite> cache;
        return cache;
    }

    uint64_t getCacheKey( const uint32_t codePoint, const fheroes2::FontType & fontType )
    {
        return ( static_cast<uint64_t>( codePoint ) << 16 ) | ( static_cast<uint64_t>( fontType.size ) << 8 ) | static_cast<uint64_t>( fontType.color );
    }

    fheroes2::Sprite renderGlyph( TTF_Font * font, const uint32_t codePoint, const fheroes2::FontType & fontType )
    {
        SDL_Surface * surface = TTF_RenderGlyph32_Blended( font, codePoint, getColor( fontType.color ) );
        if ( surface == nullptr ) {
            return {};
        }

        struct SurfaceDeleter
        {
            void operator()( SDL_Surface * s ) const
            {
                SDL_FreeSurface( s );
            }
        };

        std::unique_ptr<SDL_Surface, SurfaceDeleter> surfaceGuard( surface );

        if ( surface->format->format != SDL_PIXELFORMAT_ARGB8888 ) {
            // Convert the surface to a known format.
            SDL_Surface * converted = SDL_ConvertSurfaceFormat( surface, SDL_PIXELFORMAT_ARGB8888, 0 );
            if ( converted == nullptr ) {
                return {};
            }
            surfaceGuard.reset( converted );
            surface = converted;
        }

        // Box-filter the high-resolution glyph down to the target size, averaging the alpha channel only
        // (the glyph color is constant and is mapped to palette indices afterwards).
        const int32_t scale = glyphRenderScale;
        const int32_t glyphWidth = ( surface->w + scale - 1 ) / scale;
        const int32_t glyphHeight = ( surface->h + scale - 1 ) / scale;

        std::vector<uint8_t> alphaMap( static_cast<size_t>( glyphWidth ) * glyphHeight, 0 );

        const uint8_t * pixels = static_cast<const uint8_t *>( surface->pixels );
        const int32_t pitch = surface->pitch;

        for ( int32_t y = 0; y < glyphHeight; ++y ) {
            for ( int32_t x = 0; x < glyphWidth; ++x ) {
                uint32_t sum = 0;
                for ( int32_t sy = 0; sy < scale; ++sy ) {
                    const int32_t srcY = y * scale + sy;
                    if ( srcY >= surface->h ) {
                        break;
                    }
                    const uint32_t * src = reinterpret_cast<const uint32_t *>( pixels + srcY * pitch );
                    for ( int32_t sx = 0; sx < scale; ++sx ) {
                        const int32_t srcX = x * scale + sx;
                        if ( srcX >= surface->w ) {
                            break;
                        }
                        sum += ( src[srcX] >> 24 ) & 0xFF;
                    }
                }

                alphaMap[static_cast<size_t>( y ) * glyphWidth + x] = static_cast<uint8_t>( sum / ( scale * scale ) );
            }
        }

        // Reserve 1 extra pixel column on the right for the outline and 1 as inter-character spacing,
        // 1 extra pixel row on the top and the bottom for the outline.
        const int32_t width = glyphWidth + 2;
        const int32_t height = glyphHeight + 2;

        fheroes2::Sprite sprite( width, height, 0, 0 );
        sprite.reset(); // fully transparent (transform layer is set to 1)
        sprite.setPosition( 0, getRowHeight( fontType.size ) - glyphHeight - 2 );

        uint8_t * image = sprite.image();
        uint8_t * transform = sprite.transform();

        // Boost the mid-tones first so both the glyph and its outline use the same coverage values.
        std::vector<uint8_t> boostMap( static_cast<size_t>( glyphWidth ) * glyphHeight, 0 );
        for ( int32_t y = 0; y < glyphHeight; ++y ) {
            for ( int32_t x = 0; x < glyphWidth; ++x ) {
                boostMap[static_cast<size_t>( y ) * glyphWidth + x] = applyAlphaGamma( alphaMap[static_cast<size_t>( y ) * glyphWidth + x], fontType.size );
            }
        }

        // The glyph is shifted by 1 pixel inside the sprite to leave room for the outline on all sides.
        constexpr int32_t glyphOffsetX = 1;
        constexpr int32_t glyphOffsetY = 1;

        // Draw a solid dark outline around the glyph, matching the look of the Chinese main menu buttons:
        // crisp characters with a black contour and no shadow. Only pixels that are themselves transparent
        // and adjacent to a sufficiently solid glyph pixel receive the outline.
        constexpr uint8_t outlineIndex = 0; // black in the default palette
        constexpr uint8_t outlineNeighborThreshold = 100;

        for ( int32_t y = 0; y < height; ++y ) {
            for ( int32_t x = 0; x < width; ++x ) {
                const int32_t gx = x - glyphOffsetX;
                const int32_t gy = y - glyphOffsetY;

                if ( gx >= 0 && gx < glyphWidth && gy >= 0 && gy < glyphHeight
                     && boostMap[static_cast<size_t>( gy ) * glyphWidth + gx] > alphaMinThreshold ) {
                    // The glyph itself covers this pixel; it is filled in the pass below.
                    continue;
                }

                bool nextToGlyph = false;
                for ( int32_t dy = -1; dy <= 1 && !nextToGlyph; ++dy ) {
                    const int32_t ny = gy + dy;
                    if ( ny < 0 || ny >= glyphHeight ) {
                        continue;
                    }
                    for ( int32_t dx = -1; dx <= 1; ++dx ) {
                        const int32_t nx = gx + dx;
                        if ( nx < 0 || nx >= glyphWidth ) {
                            continue;
                        }
                        if ( boostMap[static_cast<size_t>( ny ) * glyphWidth + nx] >= outlineNeighborThreshold ) {
                            nextToGlyph = true;
                            break;
                        }
                    }
                }

                if ( nextToGlyph ) {
                    const int32_t dstOffset = y * width + x;
                    image[dstOffset] = outlineIndex;
                    transform[dstOffset] = 0;
                }
            }
        }

        for ( int32_t y = 0; y < glyphHeight; ++y ) {
            for ( int32_t x = 0; x < glyphWidth; ++x ) {
                const uint8_t alpha = boostMap[static_cast<size_t>( y ) * glyphWidth + x];
                if ( alpha <= alphaMinThreshold ) {
                    continue;
                }

                const uint8_t paletteIndex = mapAlphaToIndex( alpha, fontType.color );
                if ( paletteIndex == 0 ) {
                    continue;
                }

                const int32_t dstOffset = ( y + glyphOffsetY ) * width + ( x + glyphOffsetX );
                image[dstOffset] = paletteIndex;
                transform[dstOffset] = 0;
            }
        }

        return sprite;
    }
}

namespace fheroes2
{
    namespace CjkFont
    {
        bool isCjkTextRenderingEnabled()
        {
            // Do not cache the result here: the game language can be changed at runtime.
            return Settings::Get().getGameLanguage() == "cn";
        }

        bool isLoaded()
        {
            return getFontLibrary().load();
        }

        uint8_t getUtf8CharSize( const uint8_t firstByte )
        {
            if ( firstByte < 0x80 ) {
                return 1;
            }
            if ( ( firstByte & 0xE0 ) == 0xC0 ) {
                return 2;
            }
            if ( ( firstByte & 0xF0 ) == 0xE0 ) {
                return 3;
            }
            if ( ( firstByte & 0xF8 ) == 0xF0 ) {
                return 4;
            }

            // Invalid UTF-8 lead byte.
            return 1;
        }

        uint32_t decodeUtf8Char( const uint8_t * data, const uint8_t size )
        {
            if ( size == 1 ) {
                return data[0];
            }

            switch ( size ) {
            case 2:
                return ( ( static_cast<uint32_t>( data[0] ) & 0x1F ) << 6 ) | ( static_cast<uint32_t>( data[1] ) & 0x3F );
            case 3:
                return ( ( static_cast<uint32_t>( data[0] ) & 0x0F ) << 12 ) | ( ( static_cast<uint32_t>( data[1] ) & 0x3F ) << 6 )
                       | ( static_cast<uint32_t>( data[2] ) & 0x3F );
            case 4:
                return ( ( static_cast<uint32_t>( data[0] ) & 0x07 ) << 18 ) | ( ( static_cast<uint32_t>( data[1] ) & 0x3F ) << 12 )
                       | ( ( static_cast<uint32_t>( data[2] ) & 0x3F ) << 6 ) | ( static_cast<uint32_t>( data[3] ) & 0x3F );
            default:
                break;
            }

            return 0xFFFD; // Unicode replacement character
        }

        bool hasGlyph( const uint32_t codePoint )
        {
            TTF_Font * font = getFontLibrary().getFont( FontSize::NORMAL );
            if ( font == nullptr ) {
                return false;
            }

            return TTF_GlyphIsProvided32( font, codePoint ) != 0;
        }

        const Sprite & getGlyph( const uint32_t codePoint, const FontType & fontType )
        {
            static const Sprite emptySprite;

            TTF_Font * font = getFontLibrary().getFont( fontType.size );
            if ( font == nullptr ) {
                return emptySprite;
            }

            const auto [iter, inserted] = getGlyphCache().try_emplace( getCacheKey( codePoint, fontType ) );
            if ( inserted ) {
                iter->second = renderGlyph( font, codePoint, fontType );
            }

            return iter->second;
        }
    }
}
