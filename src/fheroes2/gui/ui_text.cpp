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

#include "ui_text.h"
#include "agg_image.h"
#include "image.h"

#include <cassert>
#include <deque>

namespace
{
    const uint8_t lineSeparator = '\n';

    class CharValidator
    {
    public:
        explicit CharValidator( const fheroes2::FontSize fontSize )
            : _charLimit( fheroes2::AGG::getCharacterLimit( fontSize ) )
        {}

        bool isValid( const uint8_t character ) const
        {
            return character >= 0x21 && character <= _charLimit;
        }

    private:
        const uint32_t _charLimit;
    };

    int32_t getInvalidCharWidth( const fheroes2::FontSize fontSize )
    {
        switch ( fontSize ) {
        case fheroes2::FontSize::SMALL:
            return 4;
        case fheroes2::FontSize::NORMAL:
            return 6;
        case fheroes2::FontSize::LARGE:
            return 12;
        default:
            assert( 0 ); // Did you add a new font size? Please add implementation.
        }

        return 0;
    }

    int32_t getFontHeight( const fheroes2::FontSize fontSize )
    {
        switch ( fontSize ) {
        case fheroes2::FontSize::SMALL:
            return 8 + 2 + 1;
        case fheroes2::FontSize::NORMAL:
            return 13 + 3 + 1;
        case fheroes2::FontSize::LARGE:
            return 26 + 6 + 1;
        default:
            assert( 0 ); // Did you add a new font size? Please add implementation.
        }

        return 0;
    }

    int32_t getLineWidth( const uint8_t * data, const int32_t size, const fheroes2::FontType & fontType )
    {
        assert( data != nullptr && size != 0 );

        const CharValidator validator( fontType.size );

        int32_t width = 0;

        const uint8_t * dataEnd = data + size;
        while ( data != dataEnd ) {
            if ( validator.isValid( *data ) ) {
                width += fheroes2::AGG::getChar( *data, fontType ).width();
            }
            else {
                width += getInvalidCharWidth( fontType.size );
            }
            ++data;
        }

        return width;
    }

    // Ignore all spaces or invalid characters at the end of the line.
    int32_t getTruncatedLineWidth( const uint8_t * data, const int32_t size, const fheroes2::FontType & fontType )
    {
        assert( data != nullptr && size != 0 );

        const CharValidator validator( fontType.size );

        int32_t width = 0;

        int32_t invalidCharWidth = 0;

        const uint8_t * dataEnd = data + size;
        while ( data != dataEnd ) {
            if ( validator.isValid( *data ) ) {
                width += invalidCharWidth;
                invalidCharWidth = 0;
                width += fheroes2::AGG::getChar( *data, fontType ).width();
            }
            else {
                invalidCharWidth += getInvalidCharWidth( fontType.size );
            }
            ++data;
        }

        return width;
    }

    void getMultiRowInfo( const uint8_t * data, const int32_t size, const int32_t maxWidth, const fheroes2::FontType & fontType, const int32_t rowHeight,
                          std::deque<fheroes2::Point> & offsets )
    {
        assert( data != nullptr && size > 0 && maxWidth > 0 );

        if ( offsets.empty() ) {
            offsets.emplace_back();
        }

        const CharValidator validator( fontType.size );

        // We need to cut sentences not in the middle of a word but by a space or invalid characters.
        const uint8_t * character = data;
        const uint8_t * characterEnd = character + size;

        int32_t lineLength = 0;
        int32_t lastWordLength = 0;
        int32_t lineWidth = 0;

        fheroes2::Point * offset = &offsets.back();

        while ( character != characterEnd ) {
            if ( *character == lineSeparator ) {
                // End of line.
                if ( lineLength > 0 ) {
                    const uint8_t * line = character - lineLength;
                    offset->x += getTruncatedLineWidth( line, lineLength, fontType );
                    lineLength = 0;
                    lastWordLength = 0;
                    lineWidth = 0;
                }

                offsets.emplace_back( 0, offset->y + rowHeight );
                offset = &offsets.back();

                ++character;
            }
            else {
                ++lineLength;
                if ( validator.isValid( *character ) ) {
                    ++lastWordLength;
                    lineWidth += fheroes2::AGG::getChar( *character, fontType ).width();
                }
                else {
                    lastWordLength = 0;
                    lineWidth += getInvalidCharWidth( fontType.size );
                }

                if ( offset->x + lineWidth > maxWidth ) {
                    const uint8_t * line = character - ( lineLength - 1 );
                    if ( lineLength == lastWordLength ) {
                        offset->x += getTruncatedLineWidth( line, lineLength, fontType );
                        ++character;
                    }
                    else {
                        offset->x += getTruncatedLineWidth( line, lineLength - lastWordLength, fontType );
                        character -= lastWordLength - 1;
                    }

                    lineLength = 0;
                    lastWordLength = 0;
                    lineWidth = 0;

                    offsets.emplace_back( 0, offset->y + rowHeight );
                    offset = &offsets.back();
                }
                else {
                    ++character;
                }
            }
        }

        offset->x += lineWidth;
    }

    int32_t render( const uint8_t * data, const int32_t size, const int32_t x, const int32_t y, fheroes2::Image & output, const fheroes2::FontType & fontType )
    {
        assert( data != nullptr && size > 0 && !output.empty() );

        const CharValidator validator( fontType.size );

        int32_t offsetX = x;

        const uint8_t * character = data;
        const uint8_t * characterEnd = character + size;

        for ( ; character != characterEnd; ++character ) {
            if ( !validator.isValid( *character ) ) {
                offsetX += getInvalidCharWidth( fontType.size );
                continue;
            }

            const fheroes2::Sprite & charSprite = fheroes2::AGG::getChar( *character, fontType );
            assert( !charSprite.empty() );

            fheroes2::Blit( charSprite, output, offsetX + charSprite.x(), y + charSprite.y() );
            offsetX += charSprite.width() + charSprite.x();
        }

        return offsetX;
    }

    void renderLine( const uint8_t * data, const int32_t size, const int32_t x, const int32_t y, const int32_t maxWidth, fheroes2::Image & output,
                     const fheroes2::FontType & fontType, const bool align )
    {
        if ( align ) {
            const int32_t correctedLineWidth = getTruncatedLineWidth( data, size, fontType );
            render( data, size, x + ( maxWidth - correctedLineWidth ) / 2, y, output, fontType );
        }
        else {
            render( data, size, x, y, output, fontType );
        }
    }

    void render( const uint8_t * data, const int32_t size, const int32_t x, const int32_t y, const int32_t maxWidth, fheroes2::Image & output,
                 const fheroes2::FontType & fontType, const int32_t rowHeight, const bool align, std::deque<fheroes2::Point> & offsets )
    {
        assert( data != nullptr && size > 0 && !output.empty() && maxWidth > 0 );

        const CharValidator validator( fontType.size );

        // We need to cut sentences not in the middle of a word but by a space or invalid characters.
        const uint8_t * character = data;
        const uint8_t * characterEnd = character + size;

        int32_t lineLength = 0;
        int32_t lastWordLength = 0;
        int32_t lineWidth = 0;

        fheroes2::Point staticOffset;
        fheroes2::Point * offset;

        if ( !offsets.empty() ) {
            offset = &offsets.front();
        }
        else {
            offset = &staticOffset;
        }

        const int32_t fontHeight = getFontHeight( fontType.size );
        const int32_t yPos = y + ( rowHeight - fontHeight ) / 2;

        while ( character != characterEnd ) {
            if ( *character == lineSeparator ) {
                // End of line. Render the current line.
                if ( lineLength > 0 ) {
                    renderLine( character - lineLength, lineLength, x + offset->x, yPos + offset->y, maxWidth, output, fontType, align );
                    lineLength = 0;
                    lastWordLength = 0;
                    lineWidth = 0;
                }

                if ( !offsets.empty() ) {
                    offsets.pop_front();
                    assert( !offsets.empty() );
                }
                if ( !offsets.empty() ) {
                    offset = &offsets.front();
                }
                else {
                    offset = &staticOffset;
                    offset->x = 0;
                    offset->y += rowHeight;
                }

                ++character;
            }
            else {
                ++lineLength;
                if ( validator.isValid( *character ) ) {
                    ++lastWordLength;
                    lineWidth += fheroes2::AGG::getChar( *character, fontType ).width();
                }
                else {
                    lastWordLength = 0;
                    lineWidth += getInvalidCharWidth( fontType.size );
                }

                if ( offset->x + lineWidth > maxWidth ) {
                    const uint8_t * line = character - ( lineLength - 1 );
                    if ( lineLength == lastWordLength ) {
                        // Looks like a word is bigger than line width.
                        renderLine( line, lineLength, x + offset->x, yPos + offset->y, maxWidth, output, fontType, align );
                        ++character;
                    }
                    else {
                        renderLine( line, lineLength - lastWordLength, x + offset->x, yPos + offset->y, maxWidth, output, fontType, align );
                        character -= lastWordLength - 1;
                    }

                    lineLength = 0;
                    lastWordLength = 0;
                    lineWidth = 0;

                    if ( !offsets.empty() ) {
                        offsets.pop_front();
                        assert( !offsets.empty() );
                    }
                    if ( !offsets.empty() ) {
                        offset = &offsets.front();
                    }
                    else {
                        offset = &staticOffset;
                        offset->x = 0;
                        offset->y += rowHeight;
                    }
                }
                else {
                    ++character;
                }
            }
        }

        if ( lineLength > 0 ) {
            renderLine( character - lineLength, lineLength, x + offset->x, yPos + offset->y, maxWidth, output, fontType, align );
            offset->x += lineWidth;
        }
    }
}

namespace fheroes2
{
    TextBase::~TextBase() = default;

    Text::Text( const std::string & text, const FontType fontType )
        : _text( text )
        , _fontType( fontType )
    {}

    Text::~Text() = default;

    int32_t Text::width() const
    {
        return getLineWidth( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), _fontType );
    }

    int32_t Text::height() const
    {
        return getFontHeight( _fontType.size );
    }

    int32_t Text::height( const int32_t maxWidth ) const
    {
        const int32_t fontHeight = getFontHeight( _fontType.size );

        std::deque<Point> offsets;
        getMultiRowInfo( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), maxWidth, _fontType, fontHeight, offsets );

        return offsets.back().y + fontHeight;
    }

    int32_t Text::rows( const int32_t maxWidth ) const
    {
        const int32_t fontHeight = getFontHeight( _fontType.size );

        std::deque<Point> offsets;
        getMultiRowInfo( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), maxWidth, _fontType, fontHeight, offsets );

        return offsets.back().y / fontHeight + 1;
    }

    void Text::draw( const int32_t x, const int32_t y, Image & output ) const
    {
        if ( output.empty() || _text.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        render( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), x, y, output, _fontType );
    }

    void Text::draw( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output ) const
    {
        if ( output.empty() || _text.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        assert( maxWidth > 0 ); // Why is the limit less than 1?
        if ( maxWidth <= 0 ) {
            draw( x, y, output );
            return;
        }

        std::deque<Point> offsets;
        render( reinterpret_cast<const uint8_t *>( _text.data() ), static_cast<int32_t>( _text.size() ), x, y, maxWidth, output, _fontType,
                getFontHeight( _fontType.size ), true, offsets );
    }

    bool Text::empty() const
    {
        return _text.empty();
    }

    MultiFontText::~MultiFontText() = default;

    void MultiFontText::add( const Text & text )
    {
        if ( !text._text.empty() ) {
            _texts.emplace_back( text );
        }
    }

    void MultiFontText::add( const Text && text )
    {
        if ( !text._text.empty() ) {
            _texts.emplace_back( text );
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

    int32_t MultiFontText::height( const int32_t maxWidth ) const
    {
        const int32_t maxFontHeight = height();

        std::deque<Point> offsets;
        for ( const Text & text : _texts ) {
            getMultiRowInfo( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), maxWidth, text._fontType, maxFontHeight,
                             offsets );
        }
        return offsets.back().y + maxFontHeight;
    }

    int32_t MultiFontText::rows( const int32_t maxWidth ) const
    {
        const int32_t maxFontHeight = height();

        std::deque<Point> offsets;
        for ( const Text & text : _texts ) {
            getMultiRowInfo( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), maxWidth, text._fontType, maxFontHeight,
                             offsets );
        }

        return offsets.back().y / maxFontHeight + 1;
    }

    void MultiFontText::draw( const int32_t x, const int32_t y, Image & output ) const
    {
        if ( output.empty() || _texts.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        const int32_t maxFontHeight = height();

        int32_t offsetX = x;
        for ( const Text & text : _texts ) {
            const int32_t fontHeight = getFontHeight( text._fontType.size );
            offsetX = render( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), offsetX,
                              y + ( maxFontHeight - fontHeight ) / 2, output, text._fontType );
        }
    }

    void MultiFontText::draw( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output ) const
    {
        if ( output.empty() || _texts.empty() ) {
            // No use to render something on an empty image or if something is empty.
            return;
        }

        assert( maxWidth > 0 ); // Why is the limit less than 1?
        if ( maxWidth <= 0 ) {
            draw( x, y, output );
            return;
        }

        const int32_t maxFontHeight = height();

        std::deque<Point> offsets;
        for ( const Text & text : _texts ) {
            getMultiRowInfo( reinterpret_cast<const uint8_t *>( text._text.data() ), static_cast<int32_t>( text._text.size() ), maxWidth, text._fontType, maxFontHeight,
                             offsets );
        }

        for ( Point & point : offsets ) {
            point.x = ( maxWidth - point.x ) / 2;
        }

        for ( size_t i = 0; i < _texts.size(); ++i ) {
            render( reinterpret_cast<const uint8_t *>( _texts[i]._text.data() ), static_cast<int32_t>( _texts[i]._text.size() ), x, y, maxWidth, output,
                    _texts[i]._fontType, maxFontHeight, false, offsets );
        }
    }

    bool MultiFontText::empty() const
    {
        return _texts.empty();
    }
}
