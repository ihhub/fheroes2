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

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "image.h"
#include "math_base.h"

namespace fheroes2
{
    enum class FontSize : uint8_t
    {
        SMALL,
        NORMAL,
        LARGE,
        // These are special fonts designed only for buttons. Use WHITE font color for Good Interface and GRAY for Evil Interface.
        BUTTON_RELEASED,
        BUTTON_PRESSED,
    };

    enum class FontColor : uint8_t
    {
        WHITE,
        GRAY,
        YELLOW
    };

    struct FontType
    {
        FontType() = default;

        FontType( const FontSize size_, const FontColor color_ )
            : size( size_ )
            , color( color_ )
        {
            // Do nothing.
        }

        FontSize size = FontSize::NORMAL;
        FontColor color = FontColor::WHITE;

        static FontType normalWhite()
        {
            return { FontSize::NORMAL, FontColor::WHITE };
        }

        static FontType normalYellow()
        {
            return { FontSize::NORMAL, FontColor::YELLOW };
        }

        static FontType smallWhite()
        {
            return { FontSize::SMALL, FontColor::WHITE };
        }

        static FontType smallYellow()
        {
            return { FontSize::SMALL, FontColor::YELLOW };
        }

        static FontType largeWhite()
        {
            return { FontSize::LARGE, FontColor::WHITE };
        }
    };

    struct TextLineInfo
    {
        TextLineInfo() = default;

        TextLineInfo( const int32_t lineWidth_, const int32_t offsetY_, const int32_t count )
            : lineWidth( lineWidth_ )
            , offsetY( offsetY_ )
            , characterCount( count )
        {
            // Do nothing.
        }

        int32_t lineWidth{ 0 };
        int32_t offsetY{ 0 };
        int32_t characterCount{ 0 };
    };

    int32_t getFontHeight( const FontSize fontSize );

    class TextBase
    {
    public:
        TextBase() = default;
        virtual ~TextBase();

        int32_t width() const
        {
            return width( 0 );
        }

        int32_t height() const
        {
            return height( 0 );
        }

        int32_t width( const int32_t maxWidth ) const
        {
            const_cast<TextBase *>( this )->setMaxWidth( maxWidth );
            return _width;
        }

        int32_t height( const int32_t maxWidth ) const
        {
            const_cast<TextBase *>( this )->setMaxWidth( maxWidth );
            return _height;
        }

        // Returns number of multi-line text rows limited by width of a line. It can be 0 if the text is empty.
        int32_t rows( const int32_t maxWidth ) const
        {
            const_cast<TextBase *>( this )->setMaxWidth( maxWidth );

            return static_cast<int32_t>( _textLineInfos.size() );
        }

        // Draw text as a single line text.
        void draw( const int32_t x, const int32_t y, Image & output ) const
        {
            drawInRoi( x, y, 0, output, { 0, 0, output.width(), output.height() } );
        }

        // Draw text as a multi-line limited by width of a line. The text will be centered according to the provided maximum width.
        void draw( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output ) const
        {
            drawInRoi( x, y, maxWidth, output, { 0, 0, output.width(), output.height() } );
        }

        // Draw text within a given image ROI. If maximum width is more than 0 then the text will be limited by width of a line
        // and centered according to the provided maximum width value.
        virtual void drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const = 0;

        // Returns true if nothing to draw.
        virtual bool empty() const = 0;

        virtual void setMaxWidth( const int32_t maxWidth ) = 0;

        // Returns full text. Multi-text class cannot return by reference hence returning by value.
        virtual std::string text() const = 0;

        void setUniformVerticalAlignment( const bool isUniform )
        {
            _isUniformedVerticalAlignment = isUniform;
        }

        void setFirstLineOffsetX( const int32_t firstLineOffsetX )
        {
            _firstLineOffsetX = firstLineOffsetX;
        }

        const std::vector<TextLineInfo> & getTextLineInfos() const
        {
            return _textLineInfos;
        }

    protected:
        bool _isUniformedVerticalAlignment{ true };

        int32_t _maxWidth{ -1 };
        int32_t _width{ 0 };
        int32_t _height{ 0 };
        int32_t _firstLineOffsetX{ 0 };

        std::vector<TextLineInfo> _textLineInfos;
    };

    class Text final : public TextBase
    {
    public:
        friend class MultiFontText;

        Text() = default;

        Text( std::string text, const FontType fontType )
            : _text( std::move( text ) )
            , _fontType( fontType )
        {
            // Do nothing.
        }

        Text( const Text & text ) = default;
        Text( Text && text ) noexcept = default;
        Text & operator=( const Text & text ) = default;
        Text & operator=( Text && text ) noexcept = default;

        ~Text() override = default;

        void drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const override;

        bool empty() const override
        {
            return _text.empty();
        }

        void set( std::string text, const FontType fontType )
        {
            _text = std::move( text );
            _fontType = fontType;

            _updateWidthAndHeight();
        }

        // This method modifies the underlying text and ends it with '...' if it is longer than the provided width.
        // By default it ignores spaces at the end of the text phrase.
        void fitToOneRow( const int32_t maxWidth, const bool ignoreSpacesAtTextEnd = true );

        std::string text() const override
        {
            return _text;
        }

        FontType getFontType() const
        {
            return _fontType;
        }

        void setMaxWidth( const int32_t maxWidth ) override;

    private:
        void _updateTextLineInfo();
        void _updateWidthAndHeight();

        std::string _text;

        FontType _fontType;
    };

    class MultiFontText final : public TextBase
    {
    public:
        MultiFontText() = default;
        ~MultiFontText() override = default;

        void add( Text text )
        {
            if ( !text.empty() ) {
                _texts.emplace_back( std::move( text ) );
            }
        }

        void drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const override;

        bool empty() const override
        {
            return _texts.empty();
        }

        std::string text() const override;

        void setMaxWidth( const int32_t maxWidth ) override;

    private:
        std::vector<Text> _texts;
    };

    class FontCharHandler
    {
    public:
        explicit FontCharHandler( const FontType fontType );

        // Returns true if character is available to render, including space (' ') and new line ('\n').
        bool isAvailable( const uint8_t character ) const;

        const Sprite & getSprite( const uint8_t character ) const;

        int32_t getWidth( const uint8_t character ) const;

        int32_t getSpaceCharWidth() const
        {
            return _spaceCharWidth;
        }

    private:
        // Returns true if character is valid for the current code page, excluding space (' ') and new line ('\n').
        bool _isValid( const uint8_t character ) const;

        int32_t _getSpaceCharWidth() const;

        const FontType _fontType;
        const uint32_t _charLimit;
        const int32_t _spaceCharWidth;
    };

    // This function is usually useful for text generation on buttons as button font is a separate set of sprites.
    bool isFontAvailable( const std::string_view text, const FontType fontType );
}
