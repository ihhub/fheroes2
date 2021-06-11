/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "screen.h"
#include <memory>

namespace fheroes2
{
    // Action-event class to communicate between sender and receiver (only for user actions)
    class ActionObject
    {
    public:
        ActionObject();
        virtual ~ActionObject() = default;

        void subscribe( ActionObject * receiver );
        void unsubscribe();

    protected:
        void updateSubscription();

        virtual void senderUpdate( const ActionObject * ) {}

    private:
        ActionObject * _receiver;
    };

    // An abstract class for button usage
    class ButtonBase : public ActionObject
    {
    public:
        ButtonBase( int32_t offsetX = 0, int32_t offsetY = 0 );
        ~ButtonBase() override = default;

        bool isEnabled() const;
        bool isDisabled() const;
        bool isPressed() const;
        bool isReleased() const;
        bool isVisible() const;
        bool isHidden() const;

        void press();
        void release();
        void enable();
        void disable(); // button becomes disabled and released
        void show(); // this method doesn't call draw
        void hide(); // this method doesn't call draw

        void setPosition( int32_t offsetX_, int32_t offsetY_ );

        void draw( Image & output = Display::instance() ); // will draw on screen by default

        // Will draw on screen by default. Returns true in case of state change. This method calls render() internally.
        bool drawOnPress( Image & output = Display::instance() );

        // Will draw on screen by default. Returns true in case of state change. This method calls render() internally.
        bool drawOnRelease( Image & output = Display::instance() );

        Rect area() const;

    protected:
        virtual const Sprite & _getPressed() const = 0;
        virtual const Sprite & _getReleased() const = 0;

    private:
        int32_t _offsetX;
        int32_t _offsetY;

        bool _isPressed;
        bool _isEnabled;
        bool _isVisible;

        const Sprite * _releasedSprite;
        std::unique_ptr<Sprite> _releasedDisabled;
    };

    class Button : public ButtonBase
    {
    public:
        Button( int32_t offsetX = 0, int32_t offsetY = 0 );
        Button( int32_t offsetX, int32_t offsetY, int icnId, uint32_t releasedIndex, uint32_t pressedIndex );
        ~Button() override = default;

        void setICNInfo( int icnId, uint32_t releasedIndex, uint32_t pressedIndex );

    protected:
        const Sprite & _getPressed() const override;
        const Sprite & _getReleased() const override;

    private:
        int _icnId;
        uint32_t _releasedIndex;
        uint32_t _pressedIndex;
    };

    // This button class is used for custom Sprites
    class ButtonSprite : public ButtonBase
    {
    public:
        ButtonSprite( int32_t offsetX = 0, int32_t offsetY = 0 );
        ButtonSprite( int32_t offsetX, int32_t offsetY, const Sprite & released, const Sprite & pressed );
        ~ButtonSprite() override = default;

        void setSprite( const Sprite & released, const Sprite & pressed );

    protected:
        const Sprite & _getPressed() const override;
        const Sprite & _getReleased() const override;

    private:
        Sprite _released;
        Sprite _pressed;
    };

    class ButtonGroup
    {
    public:
        // Please refer to dialog.h enumeration for states
        ButtonGroup( const Rect & area = Rect(), int buttonTypes = 0 );
        ButtonGroup( const ButtonGroup & ) = delete;

        ~ButtonGroup();

        ButtonGroup & operator=( const ButtonGroup & ) = delete;

        void createButton( int32_t offsetX, int32_t offsetY, int icnId, uint32_t releasedIndex, uint32_t pressedIndex, int returnValue );
        void createButton( int32_t offsetX, int32_t offsetY, const Sprite & released, const Sprite & pressed, int returnValue );
        void draw( Image & area = Display::instance() ) const; // will draw on screen by default

        // Make sure that id is less than size!
        ButtonBase & button( size_t id );
        const ButtonBase & button( size_t id ) const;

        size_t size() const;

        int processEvents();

    private:
        std::vector<ButtonBase *> _button;
        std::vector<int> _value;
    };

    // this class is used for a situations when we need to disabled a button for certain action and restore it within the scope of code
    class ButtonRestorer
    {
    public:
        explicit ButtonRestorer( ButtonBase & button, Image & area = Display::instance() );
        ButtonRestorer( const ButtonRestorer & ) = delete;

        ~ButtonRestorer();

        ButtonRestorer & operator=( const ButtonRestorer & ) = delete;

    private:
        ButtonBase & _button;
        Image & _area;
        bool _isDisabled;
    };

    class OptionButtonGroup : public ActionObject
    {
    public:
        void addButton( ButtonBase * button );

        void draw( Image & area = Display::instance() ) const; // will draw on screen by default

    protected:
        void senderUpdate( const ActionObject * sender ) override;

    private:
        std::vector<ButtonBase *> _button;

        void subscribeAll();
        void unsubscribeAll();
    };
}
