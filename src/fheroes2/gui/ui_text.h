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

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "image.h"
#include "math_base.h"

namespace fheroes2
{
    enum class SupportedLanguage : uint8_t;

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
        YELLOW,
        GOLDEN_GRADIENT,
        SILVER_GRADIENT,
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

        static FontType buttonReleasedWhite()
        {
            return { FontSize::BUTTON_RELEASED, FontColor::WHITE };
        }
    };

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

        // Returns the text line ROI relative to the text line begin. It analyzes offset and size of all characters in the text.
        virtual Rect area() const = 0;

        // Draw text as a single line text.
        void draw( const int32_t x, const int32_t y, Image & output ) const
        {
            drawInRoi( x, y, output, { 0, 0, output.width(), output.height() } );
        }

        // Draw text as a multi-line limited by width of a line. The text will be centered according to the provided maximum width.
        void draw( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output ) const
        {
            drawInRoi( x, y, maxWidth, output, { 0, 0, output.width(), output.height() } );
        }

        // Draw text as a single line text within a given image ROI.
        virtual void drawInRoi( const int32_t x, const int32_t y, Image & output, const Rect & imageRoi ) const = 0;

        // Draw text as a multi-line within a given image ROI and limited by width of a line. The text will be centered according to the provided maximum width.
        virtual void drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const = 0;

        // Returns true if nothing to draw.
        virtual bool empty() const = 0;

        // This method modifies the underlying text and ends it with '...' if it is longer than the provided width.
        virtual void fitToOneRow( const int32_t maxWidth ) = 0;

        // Returns full text. Multi-text class cannot return by reference hence returning by value.
        virtual std::string text() const = 0;

        void setUniformVerticalAlignment( const bool isUniform )
        {
            _isUniformedVerticalAlignment = isUniform;
        }

        const std::optional<SupportedLanguage> & getLanguage() const
        {
            return _language;
        }

    protected:
        std::optional<SupportedLanguage> _language;

        bool _isUniformedVerticalAlignment{ true };
    };

    class Text : public TextBase
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

        Text( std::string text, const FontType fontType, const std::optional<SupportedLanguage> language )
            : _text( std::move( text ) )
            , _fontType( fontType )
        {
            _language = language;
        }

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

        Rect area() const override;

        void drawInRoi( const int32_t x, const int32_t y, Image & output, const Rect & imageRoi ) const override;
        void drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const override;

        bool empty() const override
        {
            return _text.empty();
        }

        void set( std::string text, const FontType fontType )
        {
            _text = std::move( text );
            _fontType = fontType;
            _language = std::nullopt;
        }

        void set( std::string text, const FontType fontType, const std::optional<SupportedLanguage> language )
        {
            _text = std::move( text );
            _fontType = fontType;
            _language = language;
        }

        void fitToOneRow( const int32_t maxWidth ) override;

        void fitToArea( const int32_t maxWidth, const int32_t maxHeight );

        std::string text() const override
        {
            return _text;
        }

        // Sets to keep trailing spaces at each text line end including the end of the text.
        void keepLineTrailingSpaces()
        {
            _keepLineTrailingSpaces = true;
        }

    protected:
        // Returns text lines parameters (in pixels) in 'offsets': x - horizontal line shift, y - vertical line shift.
        // And in 'characterCount' - the number of characters on the line, in 'lineWidth' the width including the `offsetX` value.
        // The 'keepTextTrailingSpaces' is used to take into account all the spaces at the text end in example when you want to join multiple texts in multi-font texts.
        void _getTextLineInfos( std::vector<TextLineInfo> & textLineInfos, const int32_t maxWidth, const int32_t rowHeight, const bool keepTextTrailingSpaces ) const;

        std::string _text;

        FontType _fontType;

        bool _keepLineTrailingSpaces{ false };
    };

    class TextInput final : public Text
    {
    public:
        // Every text input field has limited width and the only font type.
        explicit TextInput( const FontType fontType, const int32_t maxTextWidth, const bool isMultiLine, const std::optional<SupportedLanguage> language )
            : Text( {}, fontType )
            , _maxTextWidth( maxTextWidth )
            , _isMultiLine( isMultiLine )
        {
            _language = language;
            _keepLineTrailingSpaces = true;

            _updateCursorAreaInText();
        }

        void set( std::string text, const int32_t cursorPosition )
        {
            _text = std::move( text );
            _cursorPositionInText = cursorPosition;
            _visibleTextLength = static_cast<int32_t>( _text.size() );

            _updateCursorAreaInText();
        }

        int32_t width() const override;
        // Use `width( const int32_t maxWidth )` from the Text class.
        using Text::width;

        void drawInRoi( const int32_t x, const int32_t y, Image & output, const Rect & imageRoi ) const override;
        // Use `drawInRoi( ..., const int32_t maxWidth, ... )` from the Text class.
        using Text::drawInRoi;

        void fitToOneRow( const int32_t maxWidth ) override;

        size_t getCursorPosition( const Point & pos, const Rect & roi, const bool isCenterAligned ) const
        {
            if ( _isMultiLine ) {
                return _getMultiLineTextInputCursorPosition( pos, roi );
            }

            return _getTextInputCursorPosition( pos.x, roi, isCenterAligned );
        }

        Rect cursorArea() const
        {
            return _cursorArea;
        }

        size_t getCursorPositionInAdjacentLine( const size_t currentPos, const int32_t maxWidth, const bool moveUp );

    private:
        // Update the area of text occupied by cursor and fit the text if the `_autoFitToWidth` is > 0.
        void _updateCursorAreaInText();

        size_t _getMultiLineTextInputCursorPosition( const Point & cursorOffset, const Rect & roi ) const;
        size_t _getTextInputCursorPosition( const int32_t cursorOffsetX, const Rect & roi, const bool isCenterAligned ) const;

        // Cursor position relative to the text draw position and cursor's size.
        Rect _cursorArea;
        int32_t _cursorPositionInText{ 0 };
        int32_t _visibleTextBeginPos{ 0 };
        int32_t _visibleTextLength{ 0 };

        // The (<1) value of `_maxTextWidth` will make the code to render text in one line without limiting its width.
        int32_t _maxTextWidth{ 0 };
        // When `false` the text that exceeds the `_maxTextWidth` will be moved to the next line, otherwise it will be truncated.
        bool _isMultiLine{ false };
    };

    class MultiFontText final : public TextBase
    {
    public:
        MultiFontText() = default;
        ~MultiFontText() override;

        void add( Text text );

        int32_t width() const override;
        int32_t height() const override;

        int32_t width( const int32_t maxWidth ) const override;
        int32_t height( const int32_t maxWidth ) const override;

        int32_t rows( const int32_t maxWidth ) const override;

        Rect area() const override;

        void drawInRoi( const int32_t x, const int32_t y, Image & output, const Rect & imageRoi ) const override;
        void drawInRoi( const int32_t x, const int32_t y, const int32_t maxWidth, Image & output, const Rect & imageRoi ) const override;

        bool empty() const override
        {
            return _texts.empty();
        }

        void fitToOneRow( const int32_t maxWidth ) override;

        std::string text() const override;

    private:
        void _getMultiFontTextLineInfos( std::vector<TextLineInfo> & textLineInfos, const int32_t maxWidth, const int32_t rowHeight ) const;

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
        int32_t getWidth( const std::string_view text ) const;

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

    // This function will return the width in pixels of the truncation symbol for the given font type.
    int32_t getTruncationSymbolWidth( const FontType fontType );

    const Sprite & getCursorSprite( const FontType type );
}
