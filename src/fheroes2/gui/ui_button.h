/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "image.h"
#include "math_base.h"
#include "screen.h"
#include "ui_base.h"

namespace fheroes2
{
    // An abstract class for button usage
    class ButtonBase : public ActionObject
    {
    public:
        ButtonBase() = default;
        ButtonBase( const int32_t offsetX, const int32_t offsetY )
            : _offsetX( offsetX )
            , _offsetY( offsetY )
        {
            // Do nothing.
        }

        ButtonBase( const ButtonBase & ) = delete;
        ButtonBase( ButtonBase && ) noexcept = default;

        ~ButtonBase() override = default;

        ButtonBase & operator=( const ButtonBase & button ) = delete;
        ButtonBase & operator=( ButtonBase && ) noexcept = default;

        bool isEnabled() const
        {
            return _isEnabled;
        }

        bool isDisabled() const
        {
            return !_isEnabled;
        }

        bool isPressed() const
        {
            return _isPressed;
        }

        bool isReleased() const
        {
            return !_isPressed;
        }

        bool isVisible() const
        {
            return _isVisible;
        }

        bool isHidden() const
        {
            return !_isVisible;
        }

        bool press();
        bool release();
        void enable();
        // Button becomes disabled and released
        void disable();
        // This method doesn't call draw()
        void show();
        // This method doesn't call draw()
        void hide();

        void setPosition( const int32_t offsetX, const int32_t offsetY )
        {
            _offsetX = offsetX;
            _offsetY = offsetY;
        }

        // Will draw on screen by default
        bool draw( Image & output = Display::instance() ) const;

        // Will draw and render on screen by default. Returns true in case of state change. This method calls render() internally.
        bool drawOnPress( Display & output = Display::instance() );

        // Will draw and render on screen by default. Returns true in case of state change. This method calls render() internally.
        bool drawOnRelease( Display & output = Display::instance() );

        // Will draw and render on screen by default. Returns true in case of state change. This method calls render() internally.
        bool drawOnState( const bool isPressedState, Display & output = Display::instance() )
        {
            if ( isPressedState ) {
                return drawOnPress( output );
            }

            return drawOnRelease( output );
        }

        Rect area() const;

    protected:
        virtual const Sprite & _getPressed() const = 0;
        virtual const Sprite & _getReleased() const = 0;
        virtual const Sprite & _getDisabled() const;

    private:
        int32_t _offsetX{ 0 };
        int32_t _offsetY{ 0 };

        bool _isPressed{ false };
        bool _isEnabled{ true };
        bool _isVisible{ true };

        mutable const Sprite * _releasedSprite = nullptr;
        mutable std::unique_ptr<Sprite> _disabledSprite;
    };

    class Button final : public ButtonBase
    {
    public:
        explicit Button( const int32_t offsetX = 0, const int32_t offsetY = 0 )
            : ButtonBase( offsetX, offsetY )
        {
            // Do nothing.
        }

        Button( const int32_t offsetX, const int32_t offsetY, const int icnId, const uint32_t releasedIndex, const uint32_t pressedIndex )
            : ButtonBase( offsetX, offsetY )
            , _icnId( icnId )
            , _releasedIndex( releasedIndex )
            , _pressedIndex( pressedIndex )
        {
            // Do nothing.
        }

        ~Button() override = default;

        void setICNInfo( const int icnId, const uint32_t releasedIndex, const uint32_t pressedIndex )
        {
            _icnId = icnId;
            _releasedIndex = releasedIndex;
            _pressedIndex = pressedIndex;
        }

        void setICNIndexes( const uint32_t releasedIndex, const uint32_t pressedIndex )
        {
            _releasedIndex = releasedIndex;
            _pressedIndex = pressedIndex;
        }

    protected:
        const Sprite & _getPressed() const override;
        const Sprite & _getReleased() const override;

    private:
        int _icnId{ -1 };
        uint32_t _releasedIndex{ 0 };
        uint32_t _pressedIndex{ 0 };
    };

    // This button class is used for custom Sprites
    class ButtonSprite final : public ButtonBase
    {
    public:
        explicit ButtonSprite( const int32_t offsetX = 0, const int32_t offsetY = 0 )
            : ButtonBase( offsetX, offsetY )
        {
            // Do nothing.
        }

        ButtonSprite( const int32_t offsetX, const int32_t offsetY, Sprite released, Sprite pressed, Sprite disabled = {} )
            : ButtonBase( offsetX, offsetY )
            , _released( std::move( released ) )
            , _pressed( std::move( pressed ) )
            , _disabled( std::move( disabled ) )
        {
            // Do nothing.
        }

        ButtonSprite( const ButtonSprite & ) = delete;
        ButtonSprite( ButtonSprite && ) noexcept = default;

        ~ButtonSprite() override = default;

        ButtonSprite & operator=( const ButtonSprite & ) = delete;
        ButtonSprite & operator=( ButtonSprite && ) noexcept = default;

        void setSprite( const Sprite & released, const Sprite & pressed, const Sprite & disabled = {} )
        {
            _released = released;
            _pressed = pressed;
            _disabled = disabled;
        }

    protected:
        const Sprite & _getPressed() const override;
        const Sprite & _getReleased() const override;
        const Sprite & _getDisabled() const override;

    private:
        Sprite _released;
        Sprite _pressed;
        Sprite _disabled;
    };

    class ButtonGroup
    {
    public:
        // Please refer to dialog.h enumeration for states
        explicit ButtonGroup( const Rect & area = Rect(), const int buttonTypes = 0 );
        ButtonGroup( const ButtonGroup & ) = delete;

        ~ButtonGroup() = default;

        ButtonGroup & operator=( const ButtonGroup & ) = delete;

        void createButton( const int32_t offsetX, const int32_t offsetY, const int icnId, const uint32_t releasedIndex, const uint32_t pressedIndex,
                           const int returnValue );
        void createButton( const int32_t offsetX, const int32_t offsetY, const Sprite & released, const Sprite & pressed, const int returnValue );
        void addButton( ButtonSprite && button, const int returnValue );

        // Will draw on screen by default
        void draw( Image & output = Display::instance() ) const;

        // Make sure that id is less than size!
        ButtonBase & button( const size_t id )
        {
            assert( id < _button.size() );
            return *_button[id];
        }

        const ButtonBase & button( const size_t id ) const
        {
            assert( id < _button.size() );
            return *_button[id];
        }

        int processEvents();

    private:
        std::vector<std::unique_ptr<ButtonBase>> _button;
        std::vector<int> _value;
    };

    // This class is used for a situations when we need to disable a button for certain action
    // and restore it within the scope of code. The changed button is immediately rendered on display.
    class ButtonRestorer
    {
    public:
        explicit ButtonRestorer( ButtonBase & button );
        ButtonRestorer( const ButtonRestorer & ) = delete;

        ~ButtonRestorer();

        ButtonRestorer & operator=( const ButtonRestorer & ) = delete;

    private:
        ButtonBase & _button;
        bool _isEnabled;
    };

    class OptionButtonGroup final : public ActionObject
    {
    public:
        void addButton( ButtonBase * button );

        // Will draw on screen by default
        void draw( Image & output = Display::instance() ) const;

    protected:
        void senderUpdate( const ActionObject * sender ) override;

    private:
        std::vector<ButtonBase *> _button;

        void subscribeAll();
        void unsubscribeAll();
    };

    // Make transparent edges around buttons making the pressed state appear without parts of the released state
    void makeTransparentBackground( const Sprite & released, Sprite & pressed, const int backgroundIcnID );

    // Makes a button with the background (usually from display): it can be used when original button sprites do not contain pieces of background in the pressed state
    ButtonSprite makeButtonWithBackground( int32_t offsetX, int32_t offsetY, const Sprite & released, const Sprite & pressed, const Image & background );

    // Makes a button with the shadow: for that it needs to capture the background from the display at construct time
    ButtonSprite makeButtonWithShadow( int32_t offsetX, int32_t offsetY, const Sprite & released, const Sprite & pressed, const Image & background,
                                       const Point & shadowOffset = Point( -4, 6 ) );

    // The height of text area is only 16 pixels. If 'isTransparentBackground' is set to false the button sprite will have a default background pattern from
    // STONEBAK or STONEBAK_EVIL (for Evil interface). The pattern is the same for all buttons.
    void getCustomNormalButton( Sprite & released, Sprite & pressed, const bool isEvilInterface, int32_t width, Point & releasedOffset, Point & pressedOffset,
                                const bool isTransparentBackground = false );

    // Makes a button that has the width necessary to fit a provided text using an empty button template
    void getTextAdaptedButton( Sprite & released, Sprite & pressed, const char * text, const int icnId, const int buttonBackgroundIcnID );

    // Generate released and pressed button sprites with the text on it over a transparent or a default (STONEBAK/STONEBAK_EVIL) background.
    void makeButtonSprites( Sprite & released, Sprite & pressed, const std::string & text, const int32_t buttonWidth, const bool isEvilInterface,
                            const bool isTransparentBackground );
}
