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

#include "ui_text.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <memory>

#include "agg_image.h"

namespace
{
    const uint8_t hyphenChar{ '-' };

    const uint8_t invalidChar{ '?' };

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

    struct TextLineInfo
    {
        fheroes2::Point offset;

        int32_t characterCount{ 0 };
    };

    int32_t getLineWidth( const uint8_t * data, const int32_t size, const fheroes2::FontType fontType )
    {
        assert( data != nullptr && size > 0 );

        const fheroes2::FontCharHandler charHandler( fontType );

        int32_t width = 0;

        const uint8_t * dataEnd = data + size;
        while ( data != dataEnd ) {
            width += charHandler.getWidth( *data );

            ++data;
        }

        return width;
    }

    int32_t getMaxCharacterCount( const uint8_t * data, const int32_t size, const fheroes2::FontType fontType, const int32_t maxWidth )
    {
        assert( data != nullptr && size > 0 && maxWidth > 0 );

        const fheroes2::FontCharHandler charHandler( fontType );

        int32_t width = 0;

        for ( int32_t characterCount = 0; characterCount < size; ++characterCount, ++data ) {
            width += charHandler.getWidth( *data );

            if ( width > maxWidth ) {
                return characterCount;
            }
        }

        return size;
    }

    // Ignore spaces at the end of the line. This function must be used only at the time of final rendering.
    int32_t getTruncatedLineWidth( const uint8_t * data, const int32_t size, const fheroes2::FontType fontType )
    {
        assert( data != nullptr && size > 0 );

        const fheroes2::FontCharHandler charHandler( fontType );

        int32_t width = 0;
        int32_t spaceWidth = 0;

        const int32_t spaceCharWidth = charHandler.getSpaceCharWidth();

        const uint8_t * dataEnd = data + size;
        while ( data != dataEnd ) {
            if ( isSpaceChar( *data ) ) {
                spaceWidth += spaceCharWidth;
            }
            else {
                width += spaceWidth + charHandler.getWidth( *data );

                spaceWidth = 0;
            }

            ++data;
        }

        return width;
    }

    // Returns text lines parameters (in pixels) in 'offsets': x - line widths, y - vertical line shift. And in 'charsInLine' - the number of characters on the line.
    void getMultiRowInfo( const uint8_t * data, const int32_t size, const int32_t maxWidth, const fheroes2::FontType fontType, const int32_t rowHeight,
                          std::deque<TextLineInfo> & textLineInfo )
    {
        assert( data != nullptr && size > 0 && maxWidth > 0 );

        if ( textLineInfo.empty() ) {
            textLineInfo.emplace_back();
        }

        TextLineInfo * lineInfo = &textLineInfo.back();

        const fheroes2::FontCharHandler charHandler( fontType );

        // We need to cut sentences not in the middle of a word but by a space or invalid characters.
        const uint8_t * dataEnd = data + size;

        int32_t lineLength = lineInfo->characterCount;
        int32_t lastWordLength = 0;
        int32_t lineWidth = lineInfo->offset.x;

        while ( data != dataEnd ) {
            if ( isLineSeparator( *data ) ) {
                if ( lineLength > 0 ) {
                    lineInfo->offset.x = lineWidth;
                    lineInfo->characterCount = lineLength;
                    lineLength = 0;
                    lastWordLength = 0;
                    lineWidth = 0;
                }

                const int32_t prevY = textLineInfo.back().offset.y;

                textLineInfo.emplace_back();
                lineInfo = &textLineInfo.back();
                lineInfo->offset.y = prevY + rowHeight;

                ++data;
            }
            else {
                // This is another character in the line. Get its width.

                const int32_t charWidth = charHandler.getWidth( *data );

                if ( lineWidth + charWidth > maxWidth ) {
                    // Current character has exceeded the maximum line width.

                    if ( lineLength == lastWordLength ) {
                        // This is the only word in the line.
                        // Search for '-' symbol to avoid truncating the word in the middle.
                        const uint8_t * hyphenPos = data - lineLength;
                        for ( ; hyphenPos != data; ++hyphenPos ) {
                            if ( *hyphenPos == hyphenChar ) {
                                break;
                            }
                        }

                        if ( hyphenPos != data ) {
                            // The '-' symbol has been found. In this case we consider everything after it as a separate word.
                            lineInfo->characterCount = lineLength - static_cast<int32_t>( data - hyphenPos ) + 1;
                            lineInfo->offset.x = getLineWidth( data - lineLength, lineInfo->characterCount, fontType );
                            data = hyphenPos;
                            ++data;
                        }
                        else if ( lineInfo->characterCount == 0 ) {
                            lineInfo->characterCount = lineLength;
                            lineInfo->offset.x = lineWidth;
                        }
                        else {
                            // This word was not the first in the line so we can move it to the next line.
                            // It can happen in the case of the multi-font text.
                            data -= lastWordLength;
                        }
                    }
                    else if ( isSpaceChar( *data ) ) {
                        // Current character could be a space character then current line is over.
                        // For the characters count we take this space into the account.
                        lineInfo->characterCount = lineLength + 1;
                        lineInfo->offset.x = lineWidth;

                        // We skip this space character.
                        ++data;
                    }
                    else if ( lastWordLength > 0 ) {
                        // Exclude last word from this line.
                        data -= lastWordLength;

                        lineInfo->characterCount = lineLength - lastWordLength;
                        lineInfo->offset.x = lineWidth - getLineWidth( data, lastWordLength, fontType );
                    }
                    else {
                        lineInfo->characterCount = lineLength;
                        lineInfo->offset.x = lineWidth;
                    }

                    lineLength = 0;
                    lastWordLength = 0;
                    lineWidth = 0;

                    const int32_t prevY = textLineInfo.back().offset.y;

                    textLineInfo.emplace_back();
                    lineInfo = &textLineInfo.back();
                    lineInfo->offset.y = prevY + rowHeight;
                }
                else {
                    lastWordLength = isSpaceChar( *data ) ? 0 : ( lastWordLength + 1 );

                    ++data;
                    ++lineLength;
                    lineWidth += charWidth;
                }
            }
        }

        lineInfo->characterCount = lineLength;
        lineInfo->offset.x = lineWidth;
    }

    int32_t renderSingleLine( const uint8_t * data, const int32_t size, const int32_t x, const int32_t y, fheroes2::Image & output, const fheroes2::Rect & imageRoi,
                              const fheroes2::FontType fontType )
    {
        assert( data != nullptr && size > 0 && !output.empty() );

        const fheroes2::FontCharHandler charHandler( fontType );

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

    void renderLine( const uint8_t * data, const int32_t size, const int32_t x, const int32_t y, const int32_t maxWidth, fheroes2::Image & output,
                     const fheroes2::Rect & imageRoi, const fheroes2::FontType fontType, const bool align )
    {
        if ( align ) {
            const int32_t correctedLineWidth = getTruncatedLineWidth( data, size, fontType );

            assert( correctedLineWidth <= maxWidth );
            // For button font single letters in a row we add 1 extra pixel to the width to more properly center odd-width letters.
            const int32_t extraOffsetX = ( size == 1 && ( maxWidth % 2 == 0 )
                                           && ( fontType.size == fheroes2::FontSize::BUTTON_RELEASED || fontType.size == fheroes2::FontSize::BUTTON_PRESSED ) )
                                             ? 1
                                             : 0;
            renderSingleLine( data, size, x + ( maxWidth - correctedLineWidth + extraOffsetX ) / 2, y, output, imageRoi, fontType );
        }
        else {
            renderSingleLine( data, size, x, y, output, imageRoi, fontType );
        }
    }

    void renderMultiLine( const uint8_t * data, const int32_t size, const int32_t x, const int32_t y, const int32_t maxWidth, fheroes2::Image & output,
                          const fheroes2::Rect & imageRoi, const fheroes2::FontType fontType, const int32_t rowHeight, const bool align,
                          std::deque<TextLineInfo> & lineInfos )
    {
        assert( data != nullptr && size > 0 && !output.empty() && maxWidth > 0 );

        const fheroes2::FontCharHandler charHandler( fontType );

        // We need to cut sentences not in the middle of a word but by a space or invalid characters.
        const uint8_t * dataEnd = data + size;

        int32_t lineLength = 0;
        int32_t lastWordLength = 0;
        int32_t lineWidth = 0;

        TextLineInfo staticLineInfo;
        TextLineInfo * lineInfo;
        const bool hasInputOffsets = !lineInfos.empty();

        if ( hasInputOffsets ) {
            lineInfo = &lineInfos.front();
        }
        else {
            lineInfo = &staticLineInfo;
        }

        const int32_t fontHeight = fheroes2::getFontHeight( fontType.size );
        const int32_t yPos = y + ( rowHeight - fontHeight ) / 2;

        while ( data != dataEnd ) {
            if ( isLineSeparator( *data ) ) {
                // End of line. Render the current line.
                if ( lineLength > 0 ) {
                    renderLine( data - lineLength, lineLength, x + lineInfo->offset.x, yPos + lineInfo->offset.y, maxWidth, output, imageRoi, fontType, align );
                    lineLength = 0;
                    lastWordLength = 0;
                    lineWidth = 0;
                }

                if ( hasInputOffsets ) {
                    lineInfos.pop_front();
                    assert( !lineInfos.empty() );

                    lineInfo = &lineInfos.front();
                }
                else {
                    lineInfo->offset.x = 0;
                    lineInfo->offset.y += rowHeight;
                }

                ++data;
            }
            else {
                const int32_t charWidth = charHandler.getWidth( *data );

                if ( lineInfo->offset.x + lineWidth + charWidth > maxWidth ) {
                    // Current character has exceeded the maximum line width.
                    const uint8_t * line = data - lineLength;

                    if ( lineLength == 0 && lastWordLength == 0 ) {
                        // No new characters can be added.
                        // This can happen when a new text in a multi-font text being appended just at the end of the line.
                        assert( lineWidth == 0 );
                    }
                    else if ( lineLength == lastWordLength ) {
                        // This is the only word in the line.
                        // Search for '-' symbol to avoid truncating the word in the middle.
                        const uint8_t * hyphenPos = data - lineLength;
                        for ( ; hyphenPos != data; ++hyphenPos ) {
                            if ( *hyphenPos == hyphenChar ) {
                                break;
                            }
                        }

                        if ( hyphenPos != data ) {
                            renderLine( line, static_cast<int32_t>( hyphenPos + lineLength - data ) + 1, x + lineInfo->offset.x, yPos + lineInfo->offset.y, maxWidth,
                                        output, imageRoi, fontType, align );
                            data = hyphenPos;
                            ++data;
                        }
                        else if ( lineInfo->offset.x < maxWidth / 2 ) {
                            // TODO: this is a wrong way to do as renderer should not care about special cases
                            //       but rather just follow 'instructions'.
                            //       This check is done to fix possible crashes while rendering multi-font text.
                            renderLine( line, lineLength, x + lineInfo->offset.x, yPos + lineInfo->offset.y, maxWidth, output, imageRoi, fontType, align );
                        }
                        else {
                            // This first word starts from the end of the line so we can move it to the next line.
                            // It can happen in the case of the multi-font text.
                            data -= lastWordLength;
                        }
                    }
                    else if ( isSpaceChar( *data ) ) {
                        // Current character could be a space character then current line is over.
                        renderLine( line, lineLength, x + lineInfo->offset.x, yPos + lineInfo->offset.y, maxWidth, output, imageRoi, fontType, align );

                        // We skip this space character.
                        ++data;
                    }
                    else {
                        // Exclude last word from this line.
                        renderLine( line, lineLength - lastWordLength, x + lineInfo->offset.x, yPos + lineInfo->offset.y, maxWidth, output, imageRoi, fontType, align );

                        // Go back to the start of the word.
                        data -= lastWordLength;
                    }

                    lineLength = 0;
                    lastWordLength = 0;
                    lineWidth = 0;

                    if ( hasInputOffsets ) {
                        lineInfos.pop_front();
                        assert( !lineInfos.empty() );

                        lineInfo = &lineInfos.front();
                    }
                    else {
                        lineInfo->offset.x = 0;
                        lineInfo->offset.y += rowHeight;
                    }

                    // This is a new line. getMultiRowInfo() function does estimations while ignoring whitespace characters at the start and end of lines.
                    // If the next line starts from a whitespace character it is important to skip it.
                    while ( isSpaceChar( *data ) && data != dataEnd ) {
                        ++data;
                    }
                }
                else {
                    lastWordLength = isSpaceChar( *data ) ? 0 : ( lastWordLength + 1 );

                    ++data;
                    ++lineLength;
                    lineWidth += charWidth;
                }
            }
        }

        if ( lineLength > 0 ) {
            renderLine( data - lineLength, lineLength, x + lineInfo->offset.x, yPos + lineInfo->offset.y, maxWidth, output, imageRoi, fontType, align );
            lineInfo->offset.x += lineWidth;
        }
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

        if ( maxWidth < width ) {
            return width;
        }

        return maxWidth;
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
        return getLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType );
    }

    // TODO: Properly handle strings with many text lines ('\n'). Now their heights are counted as if they're one line.
    int32_t Text::height() const
    {
        return getFontHeight( _fontType.size );
    }

    int32_t Text::width( const int32_t maxWidth ) const
    {
        if ( _text.empty() ) {
            return 0;
        }

        const int32_t fontHeight = height();

        std::deque<TextLineInfo> lineInfos;
        getMultiRowInfo( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), maxWidth, _fontType, fontHeight, lineInfos );

        if ( lineInfos.size() == 1 ) {
            // This is a single-line message.
            return lineInfos.front().offset.x;
        }

        if ( !_isUniformedVerticalAlignment ) {
            // This is a multi-lined message and we try to fit as many words on every line as possible.
            return std::max_element( lineInfos.begin(), lineInfos.end(), []( const TextLineInfo & a, const TextLineInfo & b ) { return a.offset.x < b.offset.x; } )
                ->offset.x;
        }

        // This is a multi-line message. Optimize it to fit the text evenly to the same number of lines.
        int32_t startWidth = getMaxWordWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType );
        int32_t endWidth = maxWidth;

        while ( startWidth + 1 < endWidth ) {
            const int32_t currentWidth = ( endWidth + startWidth ) / 2;
            std::deque<TextLineInfo> tempLineInfos;
            getMultiRowInfo( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), currentWidth, _fontType, fontHeight,
                             tempLineInfos );

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

        const int32_t fontHeight = height();

        std::deque<TextLineInfo> lineInfos;
        getMultiRowInfo( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), maxWidth, _fontType, fontHeight, lineInfos );

        return lineInfos.back().offset.y + fontHeight;
    }

    int32_t Text::rows( const int32_t maxWidth ) const
    {
        if ( _text.empty() ) {
            return 0;
        }

        const int32_t fontHeight = height();

        std::deque<TextLineInfo> lineInfos;
        getMultiRowInfo( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), maxWidth, _fontType, fontHeight, lineInfos );

        return static_cast<int32_t>( lineInfos.size() );
    }

    void Text::drawInRoi( const int32_t x, const int32_t y, Image & output, const Rect & imageRoi ) const
    {
        if ( output.empty() || _text.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        renderSingleLine( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), x, y, output, imageRoi, _fontType );
    }

    void Text::drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const
    {
        if ( output.empty() || _text.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        assert( maxWidth > 0 ); // Why is the limit less than 1?
        if ( maxWidth <= 0 ) {
            drawInRoi( x, y, output, imageRoi );
            return;
        }

        const int32_t correctedWidth = width( maxWidth );

        assert( correctedWidth <= maxWidth );

        // Center text according to the maximum width.
        const int32_t xOffset = ( maxWidth - correctedWidth ) / 2;

        const int32_t fontHeight = getFontHeight( _fontType.size );

        std::deque<TextLineInfo> lineInfos;
        renderMultiLine( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), x + xOffset, y, correctedWidth, output, imageRoi,
                         _fontType, fontHeight, true, lineInfos );
    }

    bool Text::empty() const
    {
        return _text.empty();
    }

    void Text::fitToOneRow( const int32_t maxWidth, const bool ignoreSpacesAtTextEnd /* = true */ )
    {
        assert( maxWidth > 0 ); // Why is the limit less than 1?
        if ( maxWidth <= 0 ) {
            return;
        }

        if ( _text.empty() ) {
            // Nothing needs to be done.
            return;
        }

        const int32_t originalTextWidth
            = ignoreSpacesAtTextEnd ? getTruncatedLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType )
                                    : getLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType );
        if ( originalTextWidth <= maxWidth ) {
            // Nothing to do. The text is not longer than the provided maximum width.
            return;
        }

        const std::string truncatedEnding( "..." );
        const int32_t truncationSymbolWidth
            = getLineWidth( reinterpret_cast<const uint8_t *>( truncatedEnding.data() ), static_cast<int32_t>( truncatedEnding.size() ), _fontType );

        const int32_t maxCharacterCount = getMaxCharacterCount( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType,
                                                                maxWidth - truncationSymbolWidth );

        _text.resize( maxCharacterCount );
        _text += truncatedEnding;
    }

    std::string Text::text() const
    {
        return _text;
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
            const int32_t height = text.height();
            if ( maxHeight < height ) {
                maxHeight = height;
            }
        }

        return maxHeight;
    }

    int32_t MultiFontText::width( const int32_t maxWidth ) const
    {
        const int32_t maxFontHeight = height();

        std::deque<TextLineInfo> lineInfos;
        for ( const Text & text : _texts ) {
            getMultiRowInfo( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), maxWidth, text._fontType, maxFontHeight,
                             lineInfos );
        }

        int32_t maxRowWidth = lineInfos.front().offset.x;
        for ( const TextLineInfo & lineInfo : lineInfos ) {
            maxRowWidth = std::max( maxRowWidth, lineInfo.offset.x );
        }

        return maxRowWidth;
    }

    int32_t MultiFontText::height( const int32_t maxWidth ) const
    {
        const int32_t maxFontHeight = height();

        std::deque<TextLineInfo> lineInfos;
        for ( const Text & text : _texts ) {
            getMultiRowInfo( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), maxWidth, text._fontType, maxFontHeight,
                             lineInfos );
        }
        return lineInfos.back().offset.y + maxFontHeight;
    }

    int32_t MultiFontText::rows( const int32_t maxWidth ) const
    {
        if ( _texts.empty() ) {
            return 0;
        }

        const int32_t maxFontHeight = height();

        std::deque<TextLineInfo> lineInfos;
        for ( const Text & text : _texts ) {
            if ( text._text.empty() ) {
                continue;
            }

            getMultiRowInfo( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), maxWidth, text._fontType, maxFontHeight,
                             lineInfos );
        }

        if ( lineInfos.empty() ) {
            return 0;
        }

        return static_cast<int32_t>( lineInfos.size() );
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
            const int32_t fontHeight = getFontHeight( text._fontType.size );
            offsetX = renderSingleLine( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), offsetX,
                                        y + ( maxFontHeight - fontHeight ) / 2, output, imageRoi, text._fontType );
        }
    }

    void MultiFontText::drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const
    {
        if ( output.empty() || _texts.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        assert( maxWidth > 0 ); // Why is the limit less than 1?
        if ( maxWidth <= 0 ) {
            drawInRoi( x, y, output, imageRoi );
            return;
        }

        const int32_t maxFontHeight = height();

        std::deque<TextLineInfo> lineInfos;
        for ( const Text & text : _texts ) {
            getMultiRowInfo( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), maxWidth, text._fontType, maxFontHeight,
                             lineInfos );
        }

        int32_t xOffset = 0;
        int32_t correctedWidth = maxWidth;
        if ( lineInfos.size() > 1 ) {
            if ( _isUniformedVerticalAlignment ) {
                // This is a multi-line message. Optimize it to fit the text evenly.
                int32_t startWidth = 1;
                for ( const Text & text : _texts ) {
                    const int32_t maxWordWidth
                        = getMaxWordWidth( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), text._fontType );
                    if ( startWidth < maxWordWidth ) {
                        startWidth = maxWordWidth;
                    }
                }

                int32_t endWidth = maxWidth;
                while ( startWidth + 1 < endWidth ) {
                    const int32_t currentWidth = ( endWidth + startWidth ) / 2;
                    std::deque<TextLineInfo> tempLineInfos;
                    for ( const Text & text : _texts ) {
                        getMultiRowInfo( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), currentWidth, text._fontType,
                                         maxFontHeight, tempLineInfos );
                    }

                    if ( tempLineInfos.size() > lineInfos.size() ) {
                        startWidth = currentWidth;
                        continue;
                    }

                    correctedWidth = currentWidth;
                    endWidth = currentWidth;
                    std::swap( lineInfos, tempLineInfos );
                }

                xOffset = ( maxWidth - correctedWidth ) / 2;
            }
            else {
                // This is a multi-lined message and we try to fit as many words on every line as possible.
                correctedWidth = width( maxWidth );
            }
        }
        else {
            // This is a single-line message. Find its length and center it according to the maximum width.
            correctedWidth = width();
            assert( correctedWidth <= maxWidth );
            xOffset = ( maxWidth - correctedWidth ) / 2;
        }

        for ( TextLineInfo & lineInfo : lineInfos ) {
            lineInfo.offset.x = ( correctedWidth - lineInfo.offset.x ) / 2;
        }

        for ( const Text & text : _texts ) {
            renderMultiLine( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), x + xOffset, y, correctedWidth, output,
                             imageRoi, text._fontType, maxFontHeight, false, lineInfos );
        }
    }

    bool MultiFontText::empty() const
    {
        return _texts.empty();
    }

    std::string MultiFontText::text() const
    {
        std::string output;

        for ( const Text & singleText : _texts ) {
            output += singleText.text();
        }

        return output;
    }

    FontCharHandler::FontCharHandler( const FontType fontType )
        : _fontType( fontType )
        , _charLimit( AGG::getCharacterLimit( fontType.size ) )
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
        return AGG::getChar( _isValid( character ) ? character : invalidChar, _fontType );
    }

    int32_t FontCharHandler::getWidth( const uint8_t character ) const
    {
        if ( isSpaceChar( character ) ) {
            return _spaceCharWidth;
        }

        const Sprite & image = getSprite( character );

        assert( ( _fontType.size != FontSize::BUTTON_RELEASED && _fontType.size != FontSize::BUTTON_PRESSED && image.x() >= 0 ) || image.x() < 0 );

        return image.x() + image.width();
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

    size_t getTextInputCursorPosition( const Text & text, const size_t currentTextCursorPosition, const Point & pointerCursorOffset, const Rect & textRoi )
    {
        // TODO: expose `Text` helper functions used in this method and convert it to a function in 'ui_tools.cpp'

        if ( text.empty() ) {
            // The text is empty.
            return 0;
        }

        const FontType fontType = text.getFontType();
        const int32_t fontHeight = getFontHeight( fontType.size );
        const int32_t pointerLine = ( pointerCursorOffset.y - textRoi.y ) / fontHeight;

        if ( pointerLine < 0 ) {
            // Pointer is upper than the first text line.
            return 0;
        }

        const int32_t textWidth = text.width( textRoi.width );
        const std::string & textString = text.text();
        const size_t textSize = textString.size();

        std::deque<TextLineInfo> lineInfos;
        getMultiRowInfo( reinterpret_cast<const uint8_t *>( textString.data() ), static_cast<int32_t>( textSize ), textWidth, fontType, fontHeight, lineInfos );

        if ( pointerLine >= static_cast<int32_t>( lineInfos.size() ) ) {
            // Pointer is lower than the last text line.
            // Reduce textSize by 1 because the cursor character ('_') was added to the line.
            return textSize - 1;
        }

        size_t cursorPosition = 0;
        for ( int32_t i = 0; i < pointerLine; ++i ) {
            cursorPosition += lineInfos[i].characterCount;
        }

        int32_t positionOffsetX = 0;
        const int32_t maxOffsetX = pointerCursorOffset.x - textRoi.x - ( textRoi.width - lineInfos[pointerLine].offset.x ) / 2;

        if ( maxOffsetX <= 0 ) {
            // Pointer is to the left of the text line.
            return ( cursorPosition > currentTextCursorPosition ) ? cursorPosition - 1 : cursorPosition;
        }

        if ( maxOffsetX > lineInfos[pointerLine].offset.x ) {
            // Pointer is to the right of the text line.
            cursorPosition += lineInfos[pointerLine].characterCount;

            return ( cursorPosition > currentTextCursorPosition ) ? cursorPosition - 1 : cursorPosition;
        }

        const FontCharHandler charHandler( fontType );

        for ( size_t i = cursorPosition; i < textSize; ++i ) {
            const int32_t charWidth = charHandler.getWidth( static_cast<uint8_t>( textString[i] ) );
            positionOffsetX += charWidth;

            if ( positionOffsetX > maxOffsetX ) {
                // Take into account that the cursor character ('_') was added to the line.
                return ( i > currentTextCursorPosition ) ? i - 1 : i;
            }
        }

        // Reduce textSize by 1 because the cursor character ('_') was added to the line.
        return textSize - 1;
    }

    bool isFontAvailable( const std::string_view text, const FontType fontType )
    {
        if ( text.empty() ) {
            return true;
        }

        const FontCharHandler charHandler( fontType );

        return std::all_of( text.begin(), text.end(), [&charHandler]( const int8_t character ) { return charHandler.isAvailable( character ); } );
    }
}
