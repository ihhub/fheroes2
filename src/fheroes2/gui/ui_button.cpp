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

#include "ui_button.h"
#include "agg.h"

namespace fheroes2
{
    Button::Button( int32_t offsetX, int32_t offsetY )
        : _offsetX( offsetX )
        , _offsetY( offsetY )
        , _icnId( -1 )
        , _releasedIndex( 0 )
        , _pressedIndex( 0 )
        , _isPressed( false )
        , _isEnabled( true )
    {}

    Button::Button( int32_t offsetX, int32_t offsetY, int icnId, uint32_t releasedIndex, uint32_t pressedIndex )
        : _offsetX( offsetX )
        , _offsetY( offsetY )
        , _icnId( icnId )
        , _releasedIndex( releasedIndex )
        , _pressedIndex( pressedIndex )
        , _isPressed( false )
        , _isEnabled( true )
    {}

    bool Button::isEnabled() const
    {
        return _isEnabled;
    }

    bool Button::isDisabled() const
    {
        return !_isEnabled;
    }

    bool Button::isPressed() const
    {
        return _isPressed;
    }

    bool Button::isReleased() const
    {
        return !_isPressed;
    }

    void Button::press()
    {
        if ( isEnabled() ) {
            _isPressed = true;
        }
    }

    void Button::release()
    {
        if ( isEnabled() ) {
            _isPressed = false;
        }
    }

    void Button::enable()
    {
        _isEnabled = true;
    }

    void Button::disable()
    {
        _isEnabled = false;
        _isPressed = false; // button can't be disabled and pressed
    }

    void Button::setICNInfo( int icnId, uint32_t releasedIndex, uint32_t pressedIndex )
    {
        _icnId = icnId;
        _releasedIndex = releasedIndex;
        _pressedIndex = pressedIndex;
    }

    void Button::setPosition( int32_t offsetX, int32_t offsetY )
    {
        _offsetX = offsetX;
        _offsetY = offsetY;
    }

    void Button::draw( Image & area ) const
    {
        if ( isPressed() ) {
            // button can't be disabled and pressed
            const Sprite & sprite = AGG::GetICN( _icnId, _pressedIndex );
            Blit( sprite, area, _offsetX + sprite.x(), _offsetY + sprite.y() );
        }
        else {
            const Sprite & sprite = AGG::GetICN( _icnId, _releasedIndex );
            if ( isEnabled() ) {
                Blit( sprite, area, _offsetX + sprite.x(), _offsetY + sprite.y() );
            }
            else {
                // TODO: cache this Sprite to speed up everything
                Sprite image = sprite;
                ApplyPallete( image, PAL::GetPalette( PAL::DARKENING ) );
                Blit( image, area, _offsetX + sprite.x(), _offsetY + sprite.y() );
            }
        }
    }

    bool Button::drawOnPress( Image & area )
    {
        if ( !isPressed() ) {
            press();
            draw( area );
            return true;
        }
        return false;
    }

    bool Button::drawOnRelease( Image & area )
    {
        if ( isPressed() ) {
            release();
            draw( area );
            return true;
        }
        return false;
    }

    Rect Button::area()
    {
        const Sprite & sprite = AGG::GetICN( _icnId, isPressed() ? _pressedIndex : _releasedIndex );
        return Rect( _offsetX + sprite.x(), _offsetY + sprite.y(), sprite.width(), sprite.height() );
    }
}
