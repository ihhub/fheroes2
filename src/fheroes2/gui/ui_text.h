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
        LARGE
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
    };

    class TextBase
    {
    public:
        TextBase() = default;
        virtual ~TextBase();

        // Returns width of a text as a single-line text only.
        virtual int32_t width() const = 0;

        // Returns height of a text as a single-line text only.
        virtual int32_t height() const = 0;

        // Returns height of a text as a multi-line text limited by width of a line.
        virtual int32_t height( const int32_t maxWidth ) const = 0;

        // Returns number of multi-line text rows limited by width of a line.
        virtual int32_t rows( const int32_t maxWidth ) const = 0;

        // Draw text as a single line text.
        virtual void draw( const int32_t x, const int32_t y, Image & output ) const = 0;

        // Draw text as a multi-line limited by width of a line.
        virtual void draw( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output ) const = 0;

        // Returns true if here is something to draw.
        virtual bool empty() const = 0;
    };

    class Text : public TextBase
    {
    public:
        friend class MultiFontText;

        Text() = default;
        Text( const std::string & text, const FontType fontType );
        ~Text() override;

        int32_t width() const override;
        int32_t height() const override;

        int32_t height( const int32_t maxWidth ) const override;
        int32_t rows( const int32_t maxWidth ) const override;

        void draw( const int32_t x, const int32_t y, Image & output ) const override;
        void draw( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output ) const override;

        bool empty() const override;

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
        void add( const Text && text );

        int32_t width() const override;
        int32_t height() const override;

        int32_t height( const int32_t maxWidth ) const override;
        int32_t rows( const int32_t maxWidth ) const override;

        void draw( const int32_t x, const int32_t y, Image & output ) const override;
        void draw( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output ) const override;

        bool empty() const override;

    private:
        std::vector<Text> _texts;
    };
}
