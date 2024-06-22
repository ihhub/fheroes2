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
#include <cstddef>
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

    int32_t calculateLineWidth( const uint8_t * data, const int32_t size, const fheroes2::FontType fontType )
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
            else if ( !isLineSeparator( *data ) ) {
                width += spaceWidth + charHandler.getWidth( *data );

                spaceWidth = 0;
            }

            ++data;
        }

        return width;
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

    void renderCenterAlignedLine( const uint8_t * data, const int32_t size, const int32_t x, const int32_t y, const int32_t maxWidth, fheroes2::Image & output,
                                  const fheroes2::Rect & imageRoi, const fheroes2::FontType fontType )
    {
        const int32_t correctedLineWidth = getTruncatedLineWidth( data, size, fontType );

        assert( correctedLineWidth <= maxWidth );
        // For button font single letters in a row we add 1 extra pixel to the width to more properly center odd-width letters.
        const int32_t extraOffsetX
            = ( size == 1 && ( maxWidth % 2 == 0 ) && ( fontType.size == fheroes2::FontSize::BUTTON_RELEASED || fontType.size == fheroes2::FontSize::BUTTON_PRESSED ) )
                  ? 1
                  : 0;
        renderSingleLine( data, size, x + ( maxWidth - correctedLineWidth + extraOffsetX ) / 2, y, output, imageRoi, fontType );
    }

    int32_t getMaxWordWidth( const uint8_t * data, const int32_t size, const fheroes2::FontType fontType )
    {
        assert( data != nullptr && size > 0 );

        int32_t maxWidth = 1;

        const fheroes2::FontCharHandler charHandler( fontType );

        int32_t width = 0;

        const uint8_t * dataEnd = data + size;
        while ( data != dataEnd ) {
            if ( isSpaceChar( *data ) || isLineSeparator( *data ) ) {
                // If it is the end of line ("\n") or a space (" "), then the word has ended.
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

    void Text::drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const
    {
        if ( output.empty() || _text.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        const_cast<Text *>( this )->setMaxWidth( maxWidth );

        const uint8_t * data = reinterpret_cast<const uint8_t *>( _text.data() );

        bool isFirstLine = true;

        for ( const TextLineInfo & info : _textLineInfos ) {
            if ( info.characterCount > 0 ) {
                if ( maxWidth > 0 ) {
                    renderCenterAlignedLine( data, info.characterCount, isFirstLine ? x + _firstLineOffsetX : x, y + info.offsetY, _maxWidth, output, imageRoi,
                                             _fontType );
                }
                else {
                    renderSingleLine( data, info.characterCount, x, y, output, imageRoi, _fontType );
                }
            }

            data += info.characterCount;
            isFirstLine = false;
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

        const int32_t originalTextWidth
            = ignoreSpacesAtTextEnd ? getTruncatedLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType )
                                    : calculateLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType );
        if ( originalTextWidth <= maxWidth ) {
            // Nothing to do. The text is not longer than the provided maximum width.
            return;
        }

        const std::string truncatedEnding( "..." );
        const int32_t truncationSymbolWidth
            = calculateLineWidth( reinterpret_cast<const uint8_t *>( truncatedEnding.data() ), static_cast<int32_t>( truncatedEnding.size() ), _fontType );

        const int32_t maxCharacterCount = getMaxCharacterCount( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType,
                                                                maxWidth - truncationSymbolWidth );

        _text.resize( maxCharacterCount );
        _text += truncatedEnding;

        _maxWidth = maxWidth;

        _updateWidthAndHeight();
    }

    void Text::setMaxWidth( const int32_t maxWidth )
    {
        if ( maxWidth == _maxWidth ) {
            return;
        }

        _maxWidth = maxWidth;

        _updateWidthAndHeight();
    }

    void Text::_updateWidthAndHeight()
    {
        if ( _text.empty() ) {
            _textLineInfos.clear();
            _width = 0;
            _height = 0;

            return;
        }

        _updateTextLineInfo();

        assert( !_textLineInfos.empty() );

        if ( _textLineInfos.size() == 1 ) {
            // This is a single-line message.

            _width = _textLineInfos.front().lineWidth;
            _height = getFontHeight( _fontType.size );

            return;
        }

        assert( _maxWidth > 0 );

        if ( _isUniformedVerticalAlignment ) {
            // This is a multi-lined message and we try to fit as many words on every line as possible.

            const int32_t maxWidth = _maxWidth;

            int32_t endWidth = maxWidth;
            int32_t startWidth = getMaxWordWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType );
            const size_t rows = _textLineInfos.size();

            // Backup the text info data.
            std::vector<TextLineInfo> backupInfo = std::move( _textLineInfos );

            while ( startWidth + 1 < endWidth ) {
                _maxWidth = ( endWidth + startWidth ) / 2;
                _updateTextLineInfo();

                if ( _textLineInfos.size() > rows ) {
                    startWidth = _maxWidth;
                    continue;
                }

                endWidth = _maxWidth;
            }

            // Restore the maximum width value.
            _maxWidth = maxWidth;

            if ( _textLineInfos.size() != rows ) {
                _textLineInfos = std::move( backupInfo );
            }
        }

        _width = std::max_element( _textLineInfos.begin(), _textLineInfos.end(), []( const TextLineInfo & lhs, const TextLineInfo & rhs ) {
                     return lhs.lineWidth < rhs.lineWidth;
                 } )->lineWidth;
        _height = _textLineInfos.back().offsetY + getFontHeight( _fontType.size );
    }

    void Text::_updateTextLineInfo()
    {
        _textLineInfos.clear();

        const uint8_t * data = reinterpret_cast<const uint8_t *>( _text.data() );

        int32_t lineWidth = ( _firstLineOffsetX > 0 ) ? _firstLineOffsetX : 0;

        if ( _maxWidth < 1 ) {
            // The text will be displayed in a single line.
            _maxWidth = 0;

            const int32_t lineCharCount = static_cast<int32_t>( _text.size() );
            lineWidth += ( lineCharCount == 0 ) ? 0 : calculateLineWidth( data, lineCharCount, _fontType );
            _textLineInfos.emplace_back( lineWidth, 0, lineCharCount );

            return;
        }

        const fheroes2::FontCharHandler charHandler( _fontType );

        int32_t lineCharCount = 0;
        int32_t lastWordCharCount = 0;
        int32_t offsetY = 0;

        const int32_t rowHeight = getFontHeight( _fontType.size );

        const uint8_t * dataEnd = data + _text.size();

        while ( data != dataEnd ) {
            if ( isLineSeparator( *data ) ) {
                _textLineInfos.emplace_back( lineWidth, offsetY, lineCharCount + 1 );

                offsetY += rowHeight;
                lineCharCount = 0;
                lastWordCharCount = 0;
                lineWidth = 0;

                ++data;
            }
            else {
                // This is another character in the line. Get its width.

                const int32_t charWidth = charHandler.getWidth( *data );

                if ( lineWidth + charWidth > _maxWidth ) {
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
                            lineWidth = calculateLineWidth( data - lineCharCount, lineCharCount, _fontType );

                            data = hyphenPos;
                            ++data;
                        }
                        else if ( _firstLineOffsetX > 0 && _textLineInfos.empty() ) {
                            // This word was not the first in the line so we can move it to the next line.
                            // It can happen in the case of the multi-font text.
                            data -= lastWordCharCount;

                            lineCharCount = 0;
                            lineWidth = _firstLineOffsetX;
                        }
                    }
                    else if ( lastWordCharCount > 0 ) {
                        // Exclude last word from this line.
                        data -= lastWordCharCount;

                        lineCharCount -= lastWordCharCount;
                        lineWidth -= calculateLineWidth( data, lastWordCharCount, _fontType );
                    }

                    _textLineInfos.emplace_back( lineWidth, offsetY, lineCharCount );

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

        _textLineInfos.emplace_back( lineWidth, offsetY, lineCharCount );
    }

    void MultiFontText::drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const
    {
        if ( output.empty() || _texts.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        const_cast<MultiFontText *>( this )->setMaxWidth( maxWidth );

        int32_t offsetY = y;

        auto infoIter = _textLineInfos.begin();

        for ( const Text & singleText : _texts ) {
            const uint8_t * data = reinterpret_cast<const uint8_t *>( singleText._text.data() );

            bool isFirstLine = true;

            for ( const TextLineInfo & info : singleText.getTextLineInfos() ) {
                if ( info.characterCount > 0 ) {
                    const int32_t offsetX = x + ( maxWidth > 0 ? ( maxWidth - infoIter->lineWidth ) / 2 : 0 );

                    renderSingleLine( data, info.characterCount, isFirstLine ? offsetX + singleText._firstLineOffsetX : offsetX, offsetY + +info.offsetY, output,
                                      imageRoi, singleText.getFontType() );
                }

                data += info.characterCount;
                isFirstLine = false;

                ++infoIter;
            }

            --infoIter;

            offsetY += singleText.getTextLineInfos().back().offsetY;
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

    void MultiFontText::setMaxWidth( const int32_t maxWidth )
    {
        if ( maxWidth == _maxWidth ) {
            return;
        }

        _textLineInfos.clear();

        _maxWidth = maxWidth;

        _textLineInfos.emplace_back( _firstLineOffsetX, 0, 0 );

        for ( Text & singleText : _texts ) {
            singleText.setFirstLineOffsetX( _textLineInfos.back().lineWidth );
            singleText.setMaxWidth( maxWidth );

            const std::vector<TextLineInfo> & info = singleText.getTextLineInfos();

            assert( !info.empty() );

            _textLineInfos.back().characterCount += info.front().characterCount;
            _textLineInfos.back().lineWidth = info.front().lineWidth;

            const int32_t offsetY = _textLineInfos.back().offsetY;

            for ( size_t i = 1; i < info.size(); ++i ) {
                _textLineInfos.emplace_back( info[i].lineWidth, offsetY + info[i].offsetY, info[i].characterCount );
            }
        }

        _width = std::max_element( _textLineInfos.begin(), _textLineInfos.end(), []( const TextLineInfo & lhs, const TextLineInfo & rhs ) {
                     return lhs.lineWidth < rhs.lineWidth;
                 } )->lineWidth;
        _height = _textLineInfos.back().offsetY + getFontHeight( _texts.back().getFontType().size );
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

        if ( isLineSeparator( character ) ) {
            return 0;
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

    bool isFontAvailable( const std::string_view text, const FontType fontType )
    {
        if ( text.empty() ) {
            return true;
        }

        const FontCharHandler charHandler( fontType );

        return std::all_of( text.begin(), text.end(), [&charHandler]( const int8_t character ) { return charHandler.isAvailable( character ); } );
    }
}
