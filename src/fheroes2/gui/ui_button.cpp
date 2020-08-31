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
#include "dialog.h"
#include "game.h"
#include "pal.h"
#include "settings.h"

namespace fheroes2
{
    ButtonBase::ButtonBase( int32_t offsetX, int32_t offsetY )
        : _offsetX( offsetX )
        , _offsetY( offsetY )
        , _isPressed( false )
        , _isEnabled( true )
        , _isVisible( true )
    {}

    ButtonBase::~ButtonBase() {}

    bool ButtonBase::isEnabled() const
    {
        return _isEnabled;
    }

    bool ButtonBase::isDisabled() const
    {
        return !_isEnabled;
    }

    bool ButtonBase::isPressed() const
    {
        return _isPressed;
    }

    bool ButtonBase::isReleased() const
    {
        return !_isPressed;
    }

    bool ButtonBase::isVisible() const
    {
        return _isVisible;
    }

    bool ButtonBase::isHidden() const
    {
        return !_isVisible;
    }

    void ButtonBase::press()
    {
        if ( isEnabled() ) {
            _isPressed = true;
        }
    }

    void ButtonBase::release()
    {
        if ( isEnabled() ) {
            _isPressed = false;
        }
    }

    void ButtonBase::enable()
    {
        _isEnabled = true;
    }

    void ButtonBase::disable()
    {
        _isEnabled = false;
        _isPressed = false; // button can't be disabled and pressed
    }

    void ButtonBase::show()
    {
        _isVisible = true;
    }

    void ButtonBase::hide()
    {
        _isVisible = false;
    }

    void ButtonBase::setPosition( int32_t offsetX_, int32_t offsetY_ )
    {
        _offsetX = offsetX_;
        _offsetY = offsetY_;
    }

    void ButtonBase::draw( Image & area ) const
    {
        if ( !isVisible() )
            return;

        if ( isPressed() ) {
            // button can't be disabled and pressed
            const Sprite & sprite = _getPressed();
            Blit( sprite, area, _offsetX + sprite.x(), _offsetY + sprite.y() );
        }
        else {
            const Sprite & sprite = _getReleased();
            if ( isEnabled() ) {
                Blit( sprite, area, _offsetX + sprite.x(), _offsetY + sprite.y() );
            }
            else {
                // TODO: cache this Sprite to speed up everything
                Sprite image = sprite;
                ApplyPalette( image, PAL::GetPalette( PAL::DARKENING ) );
                Blit( image, area, _offsetX + sprite.x(), _offsetY + sprite.y() );
            }
        }
    }

    bool ButtonBase::drawOnPress( Image & area )
    {
        if ( !isPressed() ) {
            press();
            draw( area );
            Display::instance().render();
            return true;
        }
        return false;
    }

    bool ButtonBase::drawOnRelease( Image & area )
    {
        if ( isPressed() ) {
            release();
            draw( area );
            Display::instance().render();
            return true;
        }
        return false;
    }

    Rect ButtonBase::area() const
    {
        const Sprite & sprite = isPressed() ? _getPressed() : _getReleased();
        return Rect( _offsetX + sprite.x(), _offsetY + sprite.y(), sprite.width(), sprite.height() );
    }

    Button::Button( int32_t offsetX, int32_t offsetY )
        : ButtonBase( offsetX, offsetY )
        , _icnId( -1 )
        , _releasedIndex( 0 )
        , _pressedIndex( 0 )
    {}

    Button::Button( int32_t offsetX, int32_t offsetY, int icnId, uint32_t releasedIndex, uint32_t pressedIndex )
        : ButtonBase( offsetX, offsetY )
        , _icnId( icnId )
        , _releasedIndex( releasedIndex )
        , _pressedIndex( pressedIndex )
    {}

    Button::~Button() {}

    void Button::setICNInfo( int icnId, uint32_t releasedIndex, uint32_t pressedIndex )
    {
        _icnId = icnId;
        _releasedIndex = releasedIndex;
        _pressedIndex = pressedIndex;
    }

    const Sprite & Button::_getPressed() const
    {
        return AGG::GetICN( _icnId, _pressedIndex );
    }

    const Sprite & Button::_getReleased() const
    {
        return AGG::GetICN( _icnId, _releasedIndex );
    }

    ButtonSprite::ButtonSprite( int32_t offsetX, int32_t offsetY )
        : ButtonBase( offsetX, offsetY )
    {}

    ButtonSprite::ButtonSprite( int32_t offsetX, int32_t offsetY, const Sprite & released, const Sprite & pressed )
        : ButtonBase( offsetX, offsetY )
        , _released( released )
        , _pressed( pressed )
    {}

    ButtonSprite::~ButtonSprite() {}

    void ButtonSprite::setSprite( const Sprite & released, const Sprite & pressed )
    {
        _released = released;
        _pressed = pressed;
    }

    const Sprite & ButtonSprite::_getPressed() const
    {
        return _pressed;
    }

    const Sprite & ButtonSprite::_getReleased() const
    {
        return _released;
    }

    ButtonGroup::ButtonGroup( const Rect & area, int buttonTypes )
    {
        const int icnId = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;

        Point offset;

        switch ( buttonTypes ) {
        case Dialog::YES | Dialog::NO:
            offset.x = area.x;
            offset.y = area.y + area.height - AGG::GetICN( icnId, 5 ).height();
            createButton( offset.x, offset.y, icnId, 5, 6, Dialog::YES );

            offset.x = area.x + area.width - AGG::GetICN( icnId, 7 ).width();
            offset.y = area.y + area.height - AGG::GetICN( icnId, 7 ).height();
            createButton( offset.x, offset.y, icnId, 7, 8, Dialog::NO );
            break;

        case Dialog::OK | Dialog::CANCEL:
            offset.x = area.x;
            offset.y = area.y + area.height - AGG::GetICN( icnId, 1 ).height();
            createButton( offset.x, offset.y, icnId, 1, 2, Dialog::OK );

            offset.x = area.x + area.width - AGG::GetICN( icnId, 3 ).width();
            offset.y = area.y + area.height - AGG::GetICN( icnId, 3 ).height();
            createButton( offset.x, offset.y, icnId, 3, 4, Dialog::CANCEL );
            break;

        case Dialog::OK:
            offset.x = area.x + ( area.width - AGG::GetICN( icnId, 1 ).width() ) / 2;
            offset.y = area.y + area.height - AGG::GetICN( icnId, 1 ).height();
            createButton( offset.x, offset.y, icnId, 1, 2, Dialog::OK );
            break;

        case Dialog::CANCEL:
            offset.x = area.x + ( area.width - AGG::GetICN( icnId, 3 ).width() ) / 2;
            offset.y = area.y + area.height - AGG::GetICN( icnId, 3 ).height();
            createButton( offset.x, offset.y, icnId, 3, 4, Dialog::CANCEL );
            break;

        default:
            break;
        }
    }

    ButtonGroup::~ButtonGroup()
    {
        for ( size_t i = 0; i < _button.size(); ++i ) {
            delete _button[i];
        }

        _button.clear();
        _value.clear();
    }

    void ButtonGroup::createButton( int32_t offsetX, int32_t offsetY, int icnId, uint32_t releasedIndex, uint32_t pressedIndex, int returnValue )
    {
        _button.push_back( new Button( offsetX, offsetY, icnId, releasedIndex, pressedIndex ) );
        _value.emplace_back( returnValue );
    }

    void ButtonGroup::createButton( int32_t offsetX, int32_t offsetY, const Sprite & released, const Sprite & pressed, int returnValue )
    {
        _button.push_back( new ButtonSprite( offsetX, offsetY, released, pressed ) );
        _value.emplace_back( returnValue );
    }

    void ButtonGroup::draw( Image & area ) const
    {
        for ( size_t i = 0; i < _button.size(); ++i ) {
            _button[i]->draw( area );
        }
    }

    ButtonBase & ButtonGroup::button( size_t id )
    {
        return *_button[id];
    }

    const ButtonBase & ButtonGroup::button( size_t id ) const
    {
        return *_button[id];
    }

    size_t ButtonGroup::size() const
    {
        return _button.size();
    }

    int ButtonGroup::processEvents()
    {
        LocalEvent & le = LocalEvent::Get();

        for ( size_t i = 0; i < _button.size(); ++i ) {
            if ( _button[i]->isEnabled() ) {
                le.MousePressLeft( _button[i]->area() ) ? _button[i]->drawOnPress() : _button[i]->drawOnRelease();
            }
        }

        for ( size_t i = 0; i < _button.size(); ++i ) {
            if ( _button[i]->isEnabled() && le.MouseClickLeft( _button[i]->area() ) ) {
                return _value[i];
            }
        }

        for ( size_t i = 0; i < _button.size(); ++i ) {
            if ( _button[i]->isEnabled() ) {
                if ( ( _value[i] == Dialog::YES || _value[i] == Dialog::OK ) && Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) ) {
                    return _value[i];
                }
                if ( ( _value[i] == Dialog::CANCEL || _value[i] == Dialog::NO ) && Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) ) {
                    return _value[i];
                }
            }
        }

        return Dialog::ZERO;
    }
}
