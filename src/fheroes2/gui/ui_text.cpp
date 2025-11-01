/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2025                                             *
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
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <memory>
#include <numeric>

#include "agg_image.h"
#include "icn.h"
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

    const fheroes2::Sprite & getChar( const uint8_t character, const fheroes2::FontType & fontType )
    {
        if ( character < 0x21 ) {
            return errorImage;
        }

        switch ( fontType.size ) {
        case fheroes2::FontSize::SMALL:
            switch ( fontType.color ) {
            case fheroes2::FontColor::WHITE:
                return fheroes2::AGG::GetICN( ICN::SMALFONT, character - 0x20 );
            case fheroes2::FontColor::GRAY:
                return fheroes2::AGG::GetICN( ICN::GRAY_SMALL_FONT, character - 0x20 );
            case fheroes2::FontColor::YELLOW:
                return fheroes2::AGG::GetICN( ICN::YELLOW_SMALLFONT, character - 0x20 );
            default:
                // Did you add a new font color? Add the corresponding logic for it!
                assert( 0 );
                break;
            }
            break;
        case fheroes2::FontSize::NORMAL:
            switch ( fontType.color ) {
            case fheroes2::FontColor::WHITE:
                return fheroes2::AGG::GetICN( ICN::FONT, character - 0x20 );
            case fheroes2::FontColor::GRAY:
                return fheroes2::AGG::GetICN( ICN::GRAY_FONT, character - 0x20 );
            case fheroes2::FontColor::YELLOW:
                return fheroes2::AGG::GetICN( ICN::YELLOW_FONT, character - 0x20 );
            case fheroes2::FontColor::GOLDEN_GRADIENT:
                return fheroes2::AGG::GetICN( ICN::GOLDEN_GRADIENT_FONT, character - 0x20 );
            case fheroes2::FontColor::SILVER_GRADIENT:
                return fheroes2::AGG::GetICN( ICN::SILVER_GRADIENT_FONT, character - 0x20 );
            default:
                // Did you add a new font color? Add the corresponding logic for it!
                assert( 0 );
                break;
            }
            break;
        case fheroes2::FontSize::LARGE:
            switch ( fontType.color ) {
            case fheroes2::FontColor::WHITE:
                return fheroes2::AGG::GetICN( ICN::WHITE_LARGE_FONT, character - 0x20 );
            case fheroes2::FontColor::GOLDEN_GRADIENT:
                return fheroes2::AGG::GetICN( ICN::GOLDEN_GRADIENT_LARGE_FONT, character - 0x20 );
            case fheroes2::FontColor::SILVER_GRADIENT:
                return fheroes2::AGG::GetICN( ICN::SILVER_GRADIENT_LARGE_FONT, character - 0x20 );
            default:
                // Did you add a new font color? Add the corresponding logic for it!
                assert( 0 );
                break;
            }
            break;
        case fheroes2::FontSize::BUTTON_RELEASED:
            switch ( fontType.color ) {
            case fheroes2::FontColor::WHITE:
                return fheroes2::AGG::GetICN( ICN::BUTTON_GOOD_FONT_RELEASED, character - 0x20 );
            case fheroes2::FontColor::GRAY:
                return fheroes2::AGG::GetICN( ICN::BUTTON_EVIL_FONT_RELEASED, character - 0x20 );
            default:
                // Did you add a new font color? Add the corresponding logic for it!
                assert( 0 );
                break;
            }
            break;
        case fheroes2::FontSize::BUTTON_PRESSED:
            switch ( fontType.color ) {
            case fheroes2::FontColor::WHITE:
                return fheroes2::AGG::GetICN( ICN::BUTTON_GOOD_FONT_PRESSED, character - 0x20 );
            case fheroes2::FontColor::GRAY:
                return fheroes2::AGG::GetICN( ICN::BUTTON_EVIL_FONT_PRESSED, character - 0x20 );
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
            return fheroes2::AGG::GetICNCount( ICN::SMALFONT ) + 0x20 - 1;
        case fheroes2::FontSize::NORMAL:
        case fheroes2::FontSize::LARGE:
            return fheroes2::AGG::GetICNCount( ICN::FONT ) + 0x20 - 1;
        case fheroes2::FontSize::BUTTON_RELEASED:
        case fheroes2::FontSize::BUTTON_PRESSED:
            return fheroes2::AGG::GetICNCount( ICN::BUTTON_GOOD_FONT_RELEASED ) + 0x20 - 1;
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
        const FontCharHandler charHandler( _fontType );

        return getLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), charHandler, _keepLineTrailingSpaces );
    }

    // TODO: Properly handle strings with many text lines ('\n'). Now their heights are counted as if they're one line.
    int32_t Text::height() const
    {
        const auto languageSwitcher = getLanguageSwitcher( *this );
        return getFontHeight( _fontType.size );
    }

    int32_t Text::width( const int32_t maxWidth ) const
    {
        if ( _text.empty() ) {
            return 0;
        }

        const auto languageSwitcher = getLanguageSwitcher( *this );
        const int32_t fontHeight = height();

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
        int32_t startWidth = getMaxWordWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType );
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

        std::vector<TextLineInfo> lineInfos;
        _getTextLineInfos( lineInfos, maxWidth, height(), false );

        const uint8_t * data = reinterpret_cast<const uint8_t *>( _text.data() );
        const FontCharHandler charHandler( _fontType );

        for ( const TextLineInfo & info : lineInfos ) {
            if ( info.characterCount > 0 ) {
                // Center the text line when rendering multi-line texts.
                // TODO: Implement text alignment setting to allow multi-line left aligned text for editor's warning messages.
                const int32_t offsetX = info.offsetX + ( maxWidth - info.lineWidth ) / 2;

                renderSingleLine( data, info.characterCount, x + offsetX, y + info.offsetY, output, imageRoi, charHandler );
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
        const FontCharHandler charHandler( _fontType );

        if ( height( maxWidth ) <= maxHeight ) {
            // Nothing we need to do as the text fits to the area.
            return;
        }

        while ( !_text.empty() && ( height( maxWidth ) > maxHeight ) ) {
            _text.pop_back();
        }

        // We need to add truncation symbol.
        _text += truncationSymbol;
        while ( height( maxWidth ) > maxHeight ) {
            // Remove the truncation symbol and one more character before it.
            for ( size_t i = 0; i < truncationSymbol.size(); ++i ) {
                _text.pop_back();
            }

            _text.pop_back();

            _text += truncationSymbol;
        }
    }

    void Text::_getTextLineInfos( std::vector<TextLineInfo> & textLineInfos, const int32_t maxWidth, const int32_t rowHeight, const bool keepTextTrailingSpaces ) const
    {
        assert( !_text.empty() );

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

                if ( lineWidth + charWidth > maxWidth ) {
                    // Current character has exceeded the maximum line width.

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

                            lineCharCount -= postHyphenCharCount;
                            lineWidth -= getLineWidth( data - postHyphenCharCount, postHyphenCharCount, charHandler, true );

                            data = hyphenPos;
                            ++data;
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
            const int32_t fontHeight = getFontHeight( text._fontType.size );
            const FontCharHandler charHandler( text._fontType );

            offsetX = renderSingleLine( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), offsetX,
                                        y + ( maxFontHeight - fontHeight ) / 2, output, imageRoi, charHandler );
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

            const FontCharHandler charHandler( singleText._fontType );

            while ( data < dataEnd ) {
                if ( infoIter->characterCount > 0 ) {
                    const int32_t offsetX = x + ( maxWidth > 0 ? ( maxWidth - *widthIter ) / 2 : 0 );

                    renderSingleLine( data, infoIter->characterCount, offsetX + infoIter->offsetX, y + infoIter->offsetY, output, imageRoi, charHandler );
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

            const FontCharHandler charHandler( _texts[i]._fontType );

            const int32_t originalTextWidth
                = getLineWidth( reinterpret_cast<const uint8_t *>( _texts[i]._text.data() ), static_cast<int32_t>( _texts[i]._text.size() ), charHandler, true );

            if ( ( i + 1 == _texts.size() ) && originalTextWidth <= widthLeft ) {
                // This is the last text and all texts fit the given width.
                break;
            }

            // This is not the last text and we need to keep space for the possible truncation symbol.
            const int32_t correctedWidthLeft = widthLeft - getTruncationSymbolWidth( _texts[i]._fontType );
            if ( originalTextWidth > correctedWidthLeft ) {
                // The text does not fit the given width.

                const int32_t maxCharacterCount = getMaxCharacterCount( reinterpret_cast<const uint8_t *>( _texts[i]._text.data() ),
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

    bool FontCharHandler::_isValid( const uint8_t character ) const
    {
        return character >= 0x21 && character <= _charLimit;
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

        const FontCharHandler charHandler( fontType );

        return std::all_of( text.begin(), text.end(), [&charHandler]( const int8_t character ) { return charHandler.isAvailable( character ); } );
    }

    int32_t getTruncationSymbolWidth( const FontType fontType )
    {
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
