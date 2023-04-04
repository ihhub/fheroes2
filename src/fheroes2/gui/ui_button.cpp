/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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

#include <algorithm>
#include <cassert>
#include <utility>

#include "agg_image.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "localevent.h"
#include "pal.h"
#include "settings.h"
#include "tools.h"
#include "ui_text.h"

namespace
{
    fheroes2::Image resizeButton( const fheroes2::Image & original, const int32_t width )
    {
        const int32_t height = original.height();
        assert( height > 0 );

        fheroes2::Image output;
        output.resize( width, height );
        output.reset();

        const int32_t originalWidth = original.width();
        if ( originalWidth >= width ) {
            fheroes2::Copy( original, 0, 0, output, 0, 0, width / 2, height );
            const int32_t secondHalf = width - width / 2;
            fheroes2::Copy( original, originalWidth - secondHalf, 0, output, width - secondHalf, 0, secondHalf, height );
        }
        else {
            const int32_t middleWidth = originalWidth / 3;
            const int32_t overallMiddleWidth = width - middleWidth * 2;
            const int32_t middleWidthCount = overallMiddleWidth / middleWidth;
            const int32_t middleLeftOver = overallMiddleWidth - middleWidthCount * middleWidth;

            fheroes2::Copy( original, 0, 0, output, 0, 0, middleWidth, height );
            int32_t offsetX = middleWidth;
            for ( int32_t i = 0; i < middleWidthCount; ++i ) {
                fheroes2::Copy( original, middleWidth, 0, output, offsetX, 0, middleWidth, height );
                offsetX += middleWidth;
            }

            if ( middleLeftOver > 0 ) {
                fheroes2::Copy( original, middleWidth, 0, output, offsetX, 0, middleLeftOver, height );
                offsetX += middleLeftOver;
            }

            const int32_t rightPartWidth = originalWidth - middleWidth * 2;
            assert( offsetX + rightPartWidth == width );

            fheroes2::Copy( original, originalWidth - rightPartWidth, 0, output, offsetX, 0, rightPartWidth, height );
        }

        return output;
    }
}

namespace fheroes2
{
    ButtonBase::ButtonBase( const int32_t offsetX, const int32_t offsetY )
        : _offsetX( offsetX )
        , _offsetY( offsetY )
        , _isPressed( false )
        , _isEnabled( true )
        , _isVisible( true )
        , _releasedSprite( nullptr )
        , _disabledSprite()
    {}

    bool ButtonBase::press()
    {
        if ( !isEnabled() ) {
            return false;
        }

        _isPressed = true;
        notifySubscriber();
        return true;
    }

    bool ButtonBase::release()
    {
        if ( !isEnabled() ) {
            return false;
        }

        _isPressed = false;
        notifySubscriber();
        return true;
    }

    void ButtonBase::enable()
    {
        _isEnabled = true;
        notifySubscriber();
    }

    void ButtonBase::disable()
    {
        _isEnabled = false;
        _isPressed = false; // button can't be disabled and pressed
        notifySubscriber();
    }

    void ButtonBase::show()
    {
        _isVisible = true;
        notifySubscriber();
    }

    void ButtonBase::hide()
    {
        _isVisible = false;
        notifySubscriber();
    }

    bool ButtonBase::draw( Image & output ) const
    {
        if ( !isVisible() ) {
            return false;
        }

        if ( isPressed() ) {
            // button can't be disabled and pressed
            const Sprite & sprite = _getPressed();
            Blit( sprite, output, _offsetX + sprite.x(), _offsetY + sprite.y() );
        }
        else {
            const Sprite & sprite = isEnabled() ? _getReleased() : _getDisabled();
            Blit( sprite, output, _offsetX + sprite.x(), _offsetY + sprite.y() );
        }

        return true;
    }

    bool ButtonBase::drawOnPress( Display & output )
    {
        if ( isPressed() ) {
            return false;
        }

        if ( !press() ) {
            return false;
        }

        if ( isVisible() ) {
            draw( output );
            output.render( area() );
        }
        return true;
    }

    bool ButtonBase::drawOnRelease( Display & output )
    {
        if ( !isPressed() ) {
            return false;
        }

        if ( !release() ) {
            return false;
        }

        if ( isVisible() ) {
            draw( output );
            output.render( area() );
        }
        return true;
    }

    Rect ButtonBase::area() const
    {
        const Sprite & sprite = isPressed() ? _getPressed() : _getReleased();
        return Rect( _offsetX + sprite.x(), _offsetY + sprite.y(), sprite.width(), sprite.height() );
    }

    const Sprite & ButtonBase::_getDisabled() const
    {
        const Sprite & sprite = _getReleased();
        if ( !_disabledSprite || ( _releasedSprite != &sprite ) ) {
            _releasedSprite = &sprite;
            _disabledSprite.reset( new Sprite( sprite ) );
            ApplyPalette( *_disabledSprite, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
        }

        return *_disabledSprite.get();
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

    ButtonSprite::ButtonSprite( int32_t offsetX, int32_t offsetY, const Sprite & released, const Sprite & pressed, const Sprite & disabled )
        : ButtonBase( offsetX, offsetY )
        , _released( released )
        , _pressed( pressed )
        , _disabled( disabled )
    {}

    void ButtonSprite::setSprite( const Sprite & released, const Sprite & pressed, const Sprite & disabled )
    {
        _released = released;
        _pressed = pressed;
        _disabled = disabled;
    }

    const Sprite & ButtonSprite::_getPressed() const
    {
        return _pressed;
    }

    const Sprite & ButtonSprite::_getReleased() const
    {
        return _released;
    }

    const Sprite & ButtonSprite::_getDisabled() const
    {
        if ( _disabled.empty() ) {
            return ButtonBase::_getDisabled();
        }

        return _disabled;
    }

    ButtonGroup::ButtonGroup( const Rect & area, int buttonTypes )
    {
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        const int buttonYesIcnID = isEvilInterface ? ICN::BUTTON_SMALL_YES_EVIL : ICN::BUTTON_SMALL_YES_GOOD;
        const int buttonNoIcnID = isEvilInterface ? ICN::BUTTON_SMALL_NO_EVIL : ICN::BUTTON_SMALL_NO_GOOD;
        const int buttonOkayIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_OKAY_BUTTON : ICN::UNIFORM_GOOD_OKAY_BUTTON;
        const int buttonCancelIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_CANCEL_BUTTON : ICN::UNIFORM_GOOD_CANCEL_BUTTON;

        Point offset;

        switch ( buttonTypes ) {
        case Dialog::YES | Dialog::NO:
            offset.x = area.x;
            offset.y = area.y + area.height - AGG::GetICN( buttonYesIcnID, 0 ).height();
            createButton( offset.x, offset.y, buttonYesIcnID, 0, 1, Dialog::YES );

            offset.x = area.x + area.width - AGG::GetICN( buttonNoIcnID, 0 ).width();
            offset.y = area.y + area.height - AGG::GetICN( buttonNoIcnID, 0 ).height();
            createButton( offset.x, offset.y, buttonNoIcnID, 0, 1, Dialog::NO );
            break;

        case Dialog::OK | Dialog::CANCEL:
            offset.x = area.x;
            offset.y = area.y + area.height - AGG::GetICN( buttonOkayIcnID, 0 ).height();
            createButton( offset.x, offset.y, buttonOkayIcnID, 0, 1, Dialog::OK );

            offset.x = area.x + area.width - AGG::GetICN( buttonCancelIcnID, 0 ).width();
            offset.y = area.y + area.height - AGG::GetICN( buttonCancelIcnID, 0 ).height();
            createButton( offset.x, offset.y, buttonCancelIcnID, 0, 1, Dialog::CANCEL );
            break;

        case Dialog::OK:
            offset.x = area.x + ( area.width - AGG::GetICN( buttonOkayIcnID, 0 ).width() ) / 2;
            offset.y = area.y + area.height - AGG::GetICN( buttonOkayIcnID, 0 ).height();
            createButton( offset.x, offset.y, buttonOkayIcnID, 0, 1, Dialog::OK );
            break;

        case Dialog::CANCEL:
            offset.x = area.x + ( area.width - AGG::GetICN( buttonCancelIcnID, 0 ).width() ) / 2;
            offset.y = area.y + area.height - AGG::GetICN( buttonCancelIcnID, 0 ).height();
            createButton( offset.x, offset.y, buttonCancelIcnID, 0, 1, Dialog::CANCEL );
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

    void ButtonGroup::addButton( ButtonSprite && button, int returnValue )
    {
        _button.push_back( new ButtonSprite( std::move( button ) ) );
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
        assert( id < _button.size() );
        return *_button[id];
    }

    const ButtonBase & ButtonGroup::button( size_t id ) const
    {
        assert( id < _button.size() );
        return *_button[id];
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
                if ( ( _value[i] == Dialog::YES || _value[i] == Dialog::OK ) && Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
                    return _value[i];
                }
                if ( ( _value[i] == Dialog::CANCEL || _value[i] == Dialog::NO ) && Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                    return _value[i];
                }
            }
        }

        return Dialog::ZERO;
    }

    ButtonRestorer::ButtonRestorer( ButtonBase & button, Image & area )
        : _button( button )
        , _area( area )
        , _isDisabled( button.isDisabled() )
    {
        if ( !_isDisabled ) {
            _button.disable();
            _button.draw( _area );
        }
    }

    ButtonRestorer::~ButtonRestorer()
    {
        if ( !_isDisabled ) {
            _button.enable();
            _button.draw( _area );
        }
    }

    void OptionButtonGroup::addButton( ButtonBase * button )
    {
        if ( button == nullptr )
            return;

        _button.push_back( button );
        button->subscribe( this );
    }

    void OptionButtonGroup::draw( Image & area ) const
    {
        for ( size_t i = 0; i < _button.size(); ++i ) {
            _button[i]->draw( area );
        }
    }

    void OptionButtonGroup::senderUpdate( const ActionObject * sender )
    {
        if ( sender == nullptr ) // how is it even possible?
            return;

        for ( size_t i = 0; i < _button.size(); ++i ) {
            if ( sender == _button[i] ) {
                const ButtonBase * button = _button[i];
                if ( button->isPressed() ) {
                    unsubscribeAll();

                    for ( size_t buttonId = 0; buttonId < _button.size(); ++buttonId ) {
                        if ( i != buttonId ) {
                            _button[buttonId]->release();
                        }
                    }

                    subscribeAll();
                }
            }
        }
    }

    void OptionButtonGroup::subscribeAll()
    {
        for ( size_t i = 0; i < _button.size(); ++i ) {
            _button[i]->subscribe( this );
        }
    }

    void OptionButtonGroup::unsubscribeAll()
    {
        for ( size_t i = 0; i < _button.size(); ++i ) {
            _button[i]->unsubscribe();
        }
    }

    ButtonSprite makeButtonWithBackground( int32_t offsetX, int32_t offsetY, const Sprite & released, const Sprite & pressed, const Image & background )
    {
        const Sprite croppedBackground = Crop( background, offsetX, offsetY, released.width(), released.height() );

        Sprite releasedWithBackground( croppedBackground.width(), croppedBackground.height(), 0, 0 );
        Copy( croppedBackground, releasedWithBackground );
        Blit( released, releasedWithBackground, released.x(), released.y() );

        Sprite pressedWithBackground( croppedBackground.width(), croppedBackground.height(), 0, 0 );
        Copy( croppedBackground, pressedWithBackground );
        Blit( pressed, pressedWithBackground, pressed.x(), pressed.y() );

        Sprite disabled( released );
        ApplyPalette( disabled, PAL::GetPalette( PAL::PaletteType::DARKENING ) );

        Sprite disabledWithBackground( croppedBackground.width(), croppedBackground.height(), 0, 0 );
        Copy( croppedBackground, disabledWithBackground );
        disabledWithBackground.setPosition( 0, 0 );
        Blit( disabled, disabledWithBackground, disabled.x(), disabled.y() );

        return { offsetX, offsetY, releasedWithBackground, pressedWithBackground, disabledWithBackground };
    }

    ButtonSprite makeButtonWithShadow( int32_t offsetX, int32_t offsetY, const Sprite & released, const Sprite & pressed, const Image & background,
                                       const Point & shadowOffset )
    {
        const Sprite & shadow = fheroes2::makeShadow( released, shadowOffset, 3 );

        Sprite croppedBackground = Crop( background, offsetX + shadow.x(), offsetY + shadow.y(), shadow.width(), shadow.height() );
        Blit( shadow, croppedBackground );

        Sprite releasedWithBackground( croppedBackground.width(), croppedBackground.height(), 0, 0 );
        Copy( croppedBackground, releasedWithBackground );
        Blit( released, releasedWithBackground, released.x() - shadow.x(), released.y() - shadow.y() );

        Sprite pressedWithBackground( croppedBackground.width(), croppedBackground.height(), 0, 0 );
        Copy( croppedBackground, pressedWithBackground );
        Blit( pressed, pressedWithBackground, pressed.x() - shadow.x(), pressed.y() - shadow.y() );

        Sprite disabled( released );
        ApplyPalette( disabled, PAL::GetPalette( PAL::PaletteType::DARKENING ) );

        Sprite disabledWithBackground( croppedBackground.width(), croppedBackground.height(), 0, 0 );
        Copy( croppedBackground, disabledWithBackground );
        disabledWithBackground.setPosition( 0, 0 );
        Blit( disabled, disabledWithBackground, disabled.x() - shadow.x(), disabled.y() - shadow.y() );

        return { offsetX + shadow.x(), offsetY + shadow.y(), releasedWithBackground, pressedWithBackground, disabledWithBackground };
    }

    void getCustomNormalButton( Sprite & released, Sprite & pressed, const bool isEvilInterface, int32_t width, Point & releasedOffset, Point & pressedOffset )
    {
        assert( width > 0 );

        releasedOffset = { 7, 5 };
        pressedOffset = { 6, 6 };

        // The actual button sprite is 10 pixels longer.
        width += 10;

        const int32_t icnId = isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON;
        const int32_t minimumButtonSize = 16;
        const int32_t maximumButtonSize = 200; // Why is such a wide button needed?
        width = std::clamp( width, minimumButtonSize, maximumButtonSize );

        const Sprite & originalReleased = AGG::GetICN( icnId, 0 );
        const Sprite & originalPressed = AGG::GetICN( icnId, 1 );

        const int32_t backgroundIcnId = isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK;
        const Sprite & background = AGG::GetICN( backgroundIcnId, 0 );

        Image temp = resizeButton( originalReleased, width );
        released.resize( temp.width(), temp.height() );
        assert( background.width() >= temp.width() && background.height() >= temp.height() );
        Copy( background, ( background.width() - temp.width() ) / 2, ( background.height() - temp.height() ) / 2, released, 0, 0, temp.width(), temp.height() );
        Blit( temp, released );
        released.setPosition( originalReleased.x(), originalReleased.y() );

        temp = resizeButton( originalPressed, width );
        pressed.resize( temp.width(), temp.height() );
        assert( background.width() >= temp.width() && background.height() >= temp.height() );
        Copy( background, ( background.width() - temp.width() ) / 2, ( background.height() - temp.height() ) / 2, pressed, 0, 0, temp.width(), temp.height() );
        Blit( temp, pressed );
        pressed.setPosition( originalPressed.x(), originalPressed.y() );
    }

    void makeButtonSprites( Sprite & released, Sprite & pressed, const std::string & text, const int32_t buttonWidth, const bool isEvilInterface )
    {
        fheroes2::Point releasedOffset;
        fheroes2::Point pressedOffset;
        fheroes2::getCustomNormalButton( released, pressed, isEvilInterface, buttonWidth, releasedOffset, pressedOffset );

        const fheroes2::FontColor fontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;

        const fheroes2::Text releasedText( StringUpper( text ), { fheroes2::FontSize::BUTTON_RELEASED, fontColor } );
        const fheroes2::Text pressedText( StringUpper( text ), { fheroes2::FontSize::BUTTON_PRESSED, fontColor } );

        const fheroes2::Point textOffset{ ( buttonWidth - releasedText.width() ) / 2, ( 16 - fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) ) / 2 };

        releasedText.draw( releasedOffset.x + textOffset.x, releasedOffset.y + textOffset.y, released );
        pressedText.draw( pressedOffset.x + textOffset.x, pressedOffset.y + textOffset.y, pressed );
    }
}
