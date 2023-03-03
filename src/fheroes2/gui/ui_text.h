/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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

#include <cstdint>
#include <string>
#include <vector>

namespace fheroes2
{
    class Image;
    // TODO: the old Text classes render text with 2 pixel shift by Y axis. We need to do something to keep the same drawings while replacing old code.

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
        {}

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

    int32_t getFontHeight( const FontSize fontSize );

    class TextBase
    {
    public:
        TextBase() = default;
        virtual ~TextBase();

        // Returns width of a text as a single-line text only.
        virtual int32_t width() const = 0;

        // Returns height of a text as a single-line text only.
        virtual int32_t height() const = 0;

        // Returns width of a text as a multi-line text limited by maximum width of a line.
        virtual int32_t width( const int32_t maxWidth ) const = 0;

        // Returns height of a text as a multi-line text limited by width of a line.
        virtual int32_t height( const int32_t maxWidth ) const = 0;

        // Returns number of multi-line text rows limited by width of a line. It can be 0 if the text is empty.
        virtual int32_t rows( const int32_t maxWidth ) const = 0;

        // Draw text as a single line text.
        virtual void draw( const int32_t x, const int32_t y, Image & output ) const = 0;

        // Draw text as a multi-line limited by width of a line. The text will be centered according to the provided maximum width.
        virtual void draw( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output ) const = 0;

        // Returns true if nothing to draw.
        virtual bool empty() const = 0;

        // Returns full text. Multi-text class cannot return by reference hence returning by value.
        virtual std::string text() const = 0;
    };

    class Text : public TextBase
    {
    public:
        friend class MultiFontText;

        Text() = default;
        Text( const std::string & text, const FontType fontType );
        Text( std::string && text, const FontType fontType );
        Text( const Text & text ) = default;
        Text( Text && text ) = default;
        Text & operator=( const Text & text ) = default;
        Text & operator=( Text && text ) = default;

        ~Text() override;

        int32_t width() const override;
        int32_t height() const override;

        int32_t width( const int32_t maxWidth ) const override;
        int32_t height( const int32_t maxWidth ) const override;

        int32_t rows( const int32_t maxWidth ) const override;

        void draw( const int32_t x, const int32_t y, Image & output ) const override;
        void draw( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output ) const override;

        bool empty() const override;

        void set( const std::string & text, const FontType fontType );
        void set( std::string && text, const FontType fontType );

        // This method modifies the underlying text and ends it with '...' if it is longer than the provided width.
        void fitToOneRow( const int32_t maxWidth );

        std::string text() const override;

    private:
        std::string _text;

        FontType _fontType;
    };

    class MultiFontText : public TextBase
    {
    public:
        MultiFontText() = default;
        ~MultiFontText() override;

        void add( const Text & text );
        void add( Text && text );

        int32_t width() const override;
        int32_t height() const override;

        int32_t width( const int32_t maxWidth ) const override;
        int32_t height( const int32_t maxWidth ) const override;

        int32_t rows( const int32_t maxWidth ) const override;

        void draw( const int32_t x, const int32_t y, Image & output ) const override;
        void draw( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output ) const override;

        bool empty() const override;

        std::string text() const override;

    private:
        std::vector<Text> _texts;
    };

    // This function is usually useful for text generation on buttons as button font is a separate set of sprites.
    bool isFontAvailable( const std::string & text, const FontType fontType );
}
