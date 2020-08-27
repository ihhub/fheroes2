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

namespace fheroes2
{
    // An abstract class for button usage
    class ButtonBase
    {
    public:
        ButtonBase( int32_t offsetX = 0, int32_t offsetY = 0 );
        virtual ~ButtonBase();

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

        void draw( Image & area = Display::instance() ) const; // will draw on screen by default
        bool drawOnPress( Image & area = Display::instance() ); // will draw on screen by default. Returns true in case of state change
        bool drawOnRelease( Image & area = Display::instance() ); // will draw on screen by default. Returns true in case of state change

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
    };

    class Button : public ButtonBase
    {
    public:
        Button( int32_t offsetX = 0, int32_t offsetY = 0 );
        Button( int32_t offsetX, int32_t offsetY, int icnId, uint32_t releasedIndex, uint32_t pressedIndex );
        virtual ~Button();

        void setICNInfo( int icnId, uint32_t releasedIndex, uint32_t pressedIndex );

    protected:
        virtual const Sprite & _getPressed() const override;
        virtual const Sprite & _getReleased() const override;

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
        virtual ~ButtonSprite();

        void setSprite( const Sprite & released, const Sprite & pressed );

    protected:
        virtual const Sprite & _getPressed() const override;
        virtual const Sprite & _getReleased() const override;

    private:
        Sprite _released;
        Sprite _pressed;
    };

    class ButtonGroup
    {
    public:
        // Please refer to dialog.h enumeration for states
        ButtonGroup( const Rect & area = Rect(), int buttonTypes = 0 );
        ~ButtonGroup();

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
}
