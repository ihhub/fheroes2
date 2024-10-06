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
#include <memory>

#include "agg_image.h"
#include "ui_language.h"

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
        TextLineInfo() = default;

        TextLineInfo( const int32_t offsetX_, const int32_t offsetY_, const int32_t lineWidth_, const int32_t count )
            : offsetX( offsetX_ )
            , offsetY( offsetY_ )
            , lineWidth( lineWidth_ )
            , characterCount( count )
        {
            // Do nothing.
        }

        int32_t offsetX{ 0 };
        int32_t offsetY{ 0 };
        int32_t lineWidth{ 0 };
        int32_t characterCount{ 0 };
    };

    int32_t getLineWidth( const uint8_t * data, const int32_t size, const fheroes2::FontCharHandler & charHandler )
    {
        assert( data != nullptr && size > 0 );

        int32_t width = 0;

        const uint8_t * dataEnd = data + size;
        while ( data != dataEnd ) {
            width += charHandler.getWidth( *data );

            ++data;
        }

        return width;
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

    // Ignore spaces at the end of the line. This function must be used only at the time of final rendering.
    int32_t getTruncatedLineWidth( const uint8_t * data, const int32_t size, const fheroes2::FontCharHandler & charHandler )
    {
        assert( data != nullptr && size > 0 );

        int32_t width = 0;
        int32_t spaceWidth = 0;

        const int32_t spaceCharWidth = charHandler.getSpaceCharWidth();

        const uint8_t * dataEnd = data + size;
        while ( data != dataEnd ) {
            if ( isSpaceChar( *data ) ) {
                spaceWidth += spaceCharWidth;
            }
            else if ( !isLineSeparator( *data ) ) {
                width += spaceWidth + charHandler.getWidth( *data );

                spaceWidth = 0;
            }

            ++data;
        }

        return width;
    }

    // Returns text lines parameters (in pixels) in 'offsets': x - horizontal line shift, y - vertical line shift.
    // And in 'characterCount' - the number of characters on the line, in 'lineWidth' the width including the `offsetX` value.
    void getTextLineInfos( const uint8_t * data, const int32_t size, const int32_t maxWidth, const int32_t firstLineOffsetX, const fheroes2::FontType fontType,
                           const int32_t rowHeight, std::vector<TextLineInfo> & textLineInfos )
    {
        assert( data != nullptr && size > 0 );

        int32_t lineWidth = firstLineOffsetX;
        int32_t offsetY = textLineInfos.empty() ? 0 : textLineInfos.back().offsetY;

        const fheroes2::FontCharHandler charHandler( fontType );

        if ( maxWidth < 1 ) {
            // The text will be displayed in a single line.

            lineWidth += getLineWidth( data, size, charHandler );
            textLineInfos.emplace_back( firstLineOffsetX, offsetY, lineWidth, size );

            return;
        }

        int32_t offsetX = firstLineOffsetX;
        int32_t lineCharCount = 0;
        int32_t lastWordCharCount = 0;

        const uint8_t * dataEnd = data + size;

        while ( data != dataEnd ) {
            if ( isLineSeparator( *data ) ) {
                textLineInfos.emplace_back( offsetX, offsetY, lineWidth, lineCharCount + 1 );

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

                    if ( isSpaceChar( *data ) ) {
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
                            lineCharCount -= static_cast<int32_t>( data - hyphenPos ) - 1;
                            lineWidth = getLineWidth( data - lineCharCount, lineCharCount, charHandler );

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
                        lineWidth -= getLineWidth( data, lastWordCharCount, charHandler );
                    }

                    textLineInfos.emplace_back( offsetX, offsetY, lineWidth, lineCharCount );

                    offsetX = 0;
                    offsetY += rowHeight;
                    lineCharCount = 0;
                    lastWordCharCount = 0;
                    lineWidth = 0;
                }
                else {
                    lastWordCharCount = isSpaceChar( *data ) ? 0 : ( lastWordCharCount + 1 );

                    ++data;
                    ++lineCharCount;
                    lineWidth += charWidth;
                }
            }
        }

        textLineInfos.emplace_back( offsetX, offsetY, lineWidth, lineCharCount );
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

    void renderCenterAlignedLine( const uint8_t * data, const int32_t size, const int32_t x, const int32_t y, const int32_t maxWidth, fheroes2::Image & output,
                                  const fheroes2::Rect & imageRoi, const fheroes2::FontType fontType )
    {
        const fheroes2::FontCharHandler charHandler( fontType );

        const int32_t correctedLineWidth = getTruncatedLineWidth( data, size, charHandler );

        assert( correctedLineWidth <= maxWidth );
        // For button font single letters in a row we add 1 extra pixel to the width to more properly center odd-width letters.
        const int32_t extraOffsetX
            = ( size == 1 && ( maxWidth % 2 == 0 ) && ( fontType.size == fheroes2::FontSize::BUTTON_RELEASED || fontType.size == fheroes2::FontSize::BUTTON_PRESSED ) )
                  ? 1
                  : 0;
        renderSingleLine( data, size, x + ( maxWidth - correctedLineWidth + extraOffsetX ) / 2, y, output, imageRoi, charHandler );
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
        const auto langugeSwitcher = getLanguageSwitcher( *this );
        const fheroes2::FontCharHandler charHandler( _fontType );

        return getLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), charHandler );
    }

    // TODO: Properly handle strings with many text lines ('\n'). Now their heights are counted as if they're one line.
    int32_t Text::height() const
    {
        const auto langugeSwitcher = getLanguageSwitcher( *this );
        return getFontHeight( _fontType.size );
    }

    int32_t Text::width( const int32_t maxWidth ) const
    {
        if ( _text.empty() ) {
            return 0;
        }

        const auto langugeSwitcher = getLanguageSwitcher( *this );
        const int32_t fontHeight = height();

        std::vector<TextLineInfo> lineInfos;
        getTextLineInfos( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), maxWidth, 0, _fontType, fontHeight, lineInfos );

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
            getTextLineInfos( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), currentWidth, 0, _fontType, fontHeight,
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

        const auto langugeSwitcher = getLanguageSwitcher( *this );
        const int32_t fontHeight = height();

        std::vector<TextLineInfo> lineInfos;
        getTextLineInfos( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), maxWidth, 0, _fontType, fontHeight, lineInfos );

        return lineInfos.back().offsetY + fontHeight;
    }

    int32_t Text::rows( const int32_t maxWidth ) const
    {
        if ( _text.empty() ) {
            return 0;
        }

        const auto langugeSwitcher = getLanguageSwitcher( *this );
        std::vector<TextLineInfo> lineInfos;
        getTextLineInfos( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), maxWidth, 0, _fontType, height(), lineInfos );

        return static_cast<int32_t>( lineInfos.size() );
    }

    void Text::drawInRoi( const int32_t x, const int32_t y, Image & output, const Rect & imageRoi ) const
    {
        if ( output.empty() || _text.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        const auto langugeSwitcher = getLanguageSwitcher( *this );
        const fheroes2::FontCharHandler charHandler( _fontType );

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

        const auto langugeSwitcher = getLanguageSwitcher( *this );
        const uint8_t * data = reinterpret_cast<const uint8_t *>( _text.data() );

        std::vector<TextLineInfo> lineInfos;
        getTextLineInfos( data, static_cast<int32_t>( _text.size() ), maxWidth, 0, _fontType, height(), lineInfos );

        for ( const TextLineInfo & info : lineInfos ) {
            if ( info.characterCount > 0 ) {
                renderCenterAlignedLine( data, info.characterCount, x + info.offsetX, y + info.offsetY, maxWidth, output, imageRoi, _fontType );
            }

            data += info.characterCount;
        }
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

        const auto langugeSwitcher = getLanguageSwitcher( *this );
        const fheroes2::FontCharHandler charHandler( _fontType );

        const int32_t originalTextWidth
            = ignoreSpacesAtTextEnd ? getTruncatedLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), charHandler )
                                    : getLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), charHandler );
        if ( originalTextWidth <= maxWidth ) {
            // Nothing to do. The text is not longer than the provided maximum width.
            return;
        }

        const std::string truncatedEnding( "..." );
        const int32_t truncationSymbolWidth
            = getLineWidth( reinterpret_cast<const uint8_t *>( truncatedEnding.data() ), static_cast<int32_t>( truncatedEnding.size() ), charHandler );

        const int32_t maxCharacterCount = getMaxCharacterCount( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), charHandler,
                                                                maxWidth - truncationSymbolWidth );

        _text.resize( maxCharacterCount );
        _text += truncatedEnding;
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
        for ( const Text & text : _texts ) {
            const auto langugeSwitcher = getLanguageSwitcher( text );
            getTextLineInfos( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), maxWidth,
                              lineInfos.empty() ? 0 : lineInfos.back().lineWidth, text._fontType, maxFontHeight, lineInfos );
        }

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
        for ( const Text & text : _texts ) {
            const auto langugeSwitcher = getLanguageSwitcher( text );
            getTextLineInfos( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), maxWidth,
                              lineInfos.empty() ? 0 : lineInfos.back().lineWidth, text._fontType, maxFontHeight, lineInfos );
        }
        return lineInfos.back().offsetY + maxFontHeight;
    }

    int32_t MultiFontText::rows( const int32_t maxWidth ) const
    {
        if ( _texts.empty() ) {
            return 0;
        }

        const int32_t maxFontHeight = height();

        std::vector<TextLineInfo> lineInfos;
        for ( const Text & text : _texts ) {
            if ( text._text.empty() ) {
                continue;
            }

            const auto langugeSwitcher = getLanguageSwitcher( text );
            getTextLineInfos( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ),
                              lineInfos.empty() ? 0 : lineInfos.back().lineWidth, maxWidth, text._fontType, maxFontHeight, lineInfos );
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
            const auto langugeSwitcher = getLanguageSwitcher( text );
            const int32_t fontHeight = getFontHeight( text._fontType.size );
            const fheroes2::FontCharHandler charHandler( text._fontType );

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
        for ( const Text & text : _texts ) {
            const auto langugeSwitcher = getLanguageSwitcher( text );
            getTextLineInfos( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), maxWidth,
                              lineInfos.empty() ? 0 : lineInfos.back().lineWidth, text._fontType, maxFontHeight, lineInfos );
        }

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
            const auto langugeSwitcher = getLanguageSwitcher( singleText );
            const uint8_t * data = reinterpret_cast<const uint8_t *>( singleText._text.data() );

            const uint8_t * dataEnd = data + singleText._text.size();

            const fheroes2::FontCharHandler charHandler( singleText._fontType );

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

        std::vector<TextLineInfo> lineInfos;
        getTextLineInfos( reinterpret_cast<const uint8_t *>( textString.data() ), static_cast<int32_t>( textSize ), textWidth, 0, fontType, fontHeight, lineInfos );

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
        const int32_t maxOffsetX = pointerCursorOffset.x - textRoi.x - ( textRoi.width - lineInfos[pointerLine].lineWidth ) / 2;

        if ( maxOffsetX <= 0 ) {
            // Pointer is to the left of the text line.
            return ( cursorPosition > currentTextCursorPosition ) ? cursorPosition - 1 : cursorPosition;
        }

        if ( maxOffsetX > lineInfos[pointerLine].lineWidth ) {
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
