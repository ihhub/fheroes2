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

#include "ui_button.h"

#include <algorithm>
#include <cassert>

#include "agg_image.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "localevent.h"
#include "pal.h"
#include "settings.h"
#include "translations.h"
#include "ui_text.h"

namespace
{
    fheroes2::Image resizeButton( const fheroes2::Image & original, const fheroes2::Size & buttonSize )
    {
        const int32_t originalWidth = original.width();
        const int32_t originalHeight = original.height();

        assert( originalHeight > 0 && originalWidth > 0 );

        fheroes2::Image output;

        if ( originalHeight == buttonSize.height && originalWidth == buttonSize.width ) {
            fheroes2::Copy( original, output );
            return output;
        }

        output.resize( buttonSize.width, buttonSize.height );
        output.reset();

        // Buttons that only are wider.
        if ( buttonSize.width > originalWidth && buttonSize.height == originalHeight ) {
            const int32_t middleWidth = originalWidth / 3;
            const int32_t overallMiddleWidth = buttonSize.width - middleWidth * 2;
            const int32_t middleWidthCount = overallMiddleWidth / middleWidth;
            const int32_t middleWidthLeftOver = overallMiddleWidth - middleWidthCount * middleWidth;

            fheroes2::Copy( original, 0, 0, output, 0, 0, middleWidth, originalHeight );

            int32_t offsetX = middleWidth;
            for ( int32_t i = 0; i < middleWidthCount; ++i ) {
                fheroes2::Copy( original, middleWidth, 0, output, offsetX, 0, middleWidth, originalHeight );
                offsetX += middleWidth;
            }

            if ( middleWidthLeftOver > 0 ) {
                fheroes2::Copy( original, middleWidth, 0, output, offsetX, 0, middleWidthLeftOver, originalHeight );
                offsetX += middleWidthLeftOver;
            }
            assert( offsetX + originalWidth - middleWidth * 2 == buttonSize.width );

            fheroes2::Copy( original, originalWidth - middleWidth, 0, output, offsetX, 0, middleWidth, originalHeight );
        }
        // Buttons that only are taller.
        else if ( buttonSize.height > originalHeight && buttonSize.width == originalWidth ) {
            const int32_t middleHeight = originalHeight / 5;
            const int32_t overallMiddleHeight = buttonSize.height - middleHeight * 2;
            const int32_t middleHeightCount = overallMiddleHeight / middleHeight;
            const int32_t middleHeightLeftOver = overallMiddleHeight - middleHeightCount * middleHeight;

            fheroes2::Copy( original, 0, 0, output, 0, 0, originalWidth, middleHeight );

            int32_t offsetY = middleHeight;
            for ( int32_t i = 0; i < middleHeightCount; ++i ) {
                fheroes2::Copy( original, 0, middleHeight, output, 0, offsetY, originalWidth, middleHeight );
                offsetY += middleHeight;
            }

            if ( middleHeightLeftOver > 0 ) {
                fheroes2::Copy( original, 0, middleHeight, output, 0, offsetY, originalWidth, middleHeightLeftOver );
                offsetY += middleHeightLeftOver;
            }
            assert( offsetY + originalHeight - middleHeight * 4 == buttonSize.height );

            fheroes2::Copy( original, 0, originalHeight - middleHeight, output, 0, offsetY, originalWidth, middleHeight );
        }
        // Buttons that have shrunk in any direction.
        else if ( buttonSize.height <= originalHeight && buttonSize.width <= originalWidth ) {
            fheroes2::Copy( original, 0, 0, output, 0, 0, buttonSize.width / 2, buttonSize.height / 2 );

            const int32_t secondHalfHeight = buttonSize.height - buttonSize.height / 2;
            const int32_t secondHalfWidth = buttonSize.width - buttonSize.width / 2;

            fheroes2::Copy( original, 0, originalHeight - secondHalfHeight, output, 0, buttonSize.height - secondHalfHeight, secondHalfWidth, secondHalfHeight );
            fheroes2::Copy( original, originalWidth - secondHalfWidth, 0, output, buttonSize.width - secondHalfWidth, 0, secondHalfWidth, secondHalfHeight );
            fheroes2::Copy( original, originalWidth - secondHalfWidth, originalHeight - secondHalfHeight, output, buttonSize.width - secondHalfWidth,
                            buttonSize.height - secondHalfHeight, secondHalfWidth, secondHalfHeight );
        }
        // Buttons that have increased width and height.
        else if ( buttonSize.height > originalHeight && buttonSize.width > originalWidth ) {
            const int32_t middleWidth = originalWidth / 3;
            const int32_t overallMiddleWidth = buttonSize.width - middleWidth * 2;
            const int32_t middleWidthCount = overallMiddleWidth / middleWidth;
            const int32_t middleWidthLeftOver = overallMiddleWidth - middleWidthCount * middleWidth;

            const int32_t middleHeight = originalHeight / 5;
            const int32_t overallMiddleHeight = buttonSize.height - middleHeight * 2;
            const int32_t middleHeightCount = overallMiddleHeight / middleHeight;
            const int32_t middleHeightLeftOver = overallMiddleHeight - middleHeightCount * middleHeight;

            fheroes2::Copy( original, 0, 0, output, 0, 0, middleWidth, middleHeight );
            const int32_t rightPartWidth = originalWidth - middleWidth * 2;
            fheroes2::Copy( original, originalWidth - rightPartWidth, 0, output, buttonSize.width - rightPartWidth, 0, middleWidth, middleHeight );

            int32_t offsetY = middleHeight;
            for ( int32_t i = 0; i < middleHeightCount; ++i ) {
                fheroes2::Copy( original, 0, middleHeight, output, 0, offsetY, middleWidth, middleHeight );
                fheroes2::Copy( original, originalWidth - rightPartWidth, middleHeight, output, buttonSize.width - rightPartWidth, offsetY, middleWidth, middleHeight );
                offsetY += middleHeight;
            }

            if ( middleHeightLeftOver > 0 ) {
                fheroes2::Copy( original, 0, middleHeight, output, 0, offsetY, middleWidth, middleHeightLeftOver );
                fheroes2::Copy( original, originalWidth - rightPartWidth, middleHeight, output, buttonSize.width - rightPartWidth, offsetY, middleWidth,
                                middleHeightLeftOver );
                offsetY += middleHeightLeftOver;
            }

            const int32_t bottomPartHeight = originalHeight - middleHeight * 4;

            fheroes2::Copy( original, 0, originalHeight - bottomPartHeight, output, 0, offsetY, middleWidth, bottomPartHeight );
            fheroes2::Copy( original, originalWidth - rightPartWidth, originalHeight - bottomPartHeight, output, buttonSize.width - rightPartWidth, offsetY, middleWidth,
                            bottomPartHeight );

            int32_t offsetX = middleWidth;
            for ( int32_t i = 0; i < middleWidthCount; ++i ) {
                fheroes2::Copy( original, middleWidth, 0, output, offsetX, 0, middleWidth, middleHeight );
                fheroes2::Copy( original, middleWidth, originalHeight - bottomPartHeight, output, offsetX, offsetY, middleWidth, middleHeight );
                offsetX += middleWidth;
            }

            if ( middleWidthLeftOver > 0 ) {
                fheroes2::Copy( original, middleWidth, 0, output, offsetX, 0, middleWidthLeftOver, middleHeight );
                fheroes2::Copy( original, middleWidth, originalHeight - bottomPartHeight, output, offsetX, offsetY, middleWidthLeftOver, middleHeight );
                offsetX += middleWidthLeftOver;
            }

            assert( offsetX + rightPartWidth == buttonSize.width );
            assert( offsetY + bottomPartHeight == buttonSize.height );

            // Find the background color using the central pixel of the button background.
            const uint32_t centralPosition = ( originalWidth * originalHeight / 2 ) - 1 - ( originalWidth >= originalHeight ? originalHeight : originalWidth ) / 2;
            fheroes2::Fill( output, middleWidth, middleHeight, offsetX - middleWidth, offsetY - middleHeight, original.image()[centralPosition] );
        }
        else {
            // You are trying to modify the size of the button in an unexpected way.
            assert( 0 );
        }

        return output;
    }

    void getButtonSpecificValues( const int emptyButtonIcnID, fheroes2::FontColor & font, fheroes2::Point & textAreaBorders, fheroes2::Size & minimumTextArea,
                                  fheroes2::Size & maximumTextArea, fheroes2::Size & backgroundBorders, fheroes2::Point & releasedOffset,
                                  fheroes2::Point & pressedOffset )
    {
        switch ( emptyButtonIcnID ) {
        case ICN::EMPTY_GOOD_BUTTON:
            font = fheroes2::FontColor::WHITE;
            textAreaBorders.x = 4 + 4;
            textAreaBorders.y = 1 + 1;
            // The minimum text area width for Good campaign buttons is 86 judging from the shared widths of the
            // original OKAY and the CANCEL buttons even though OKAY is a shorter word
            minimumTextArea.width = 86;
            maximumTextArea.height = 50;
            backgroundBorders.width = 6 + 4;
            backgroundBorders.height = 4 + 4;
            releasedOffset = { 6, 4 };
            pressedOffset = { 5, 5 };
            break;
        case ICN::EMPTY_EVIL_BUTTON:
            font = fheroes2::FontColor::GRAY;
            textAreaBorders.x = 4 + 4;
            textAreaBorders.y = 1 + 1;
            minimumTextArea.width = 87;
            maximumTextArea.height = 50;
            backgroundBorders.width = 6 + 3;
            backgroundBorders.height = 4 + 4;
            releasedOffset = { 6, 4 };
            pressedOffset = { 5, 5 };
            break;
        // TODO: POL buttons are just EVIL campaign theme buttons. With some adjustments, the POL button code can be removed.
        case ICN::EMPTY_POL_BUTTON:
            font = fheroes2::FontColor::GRAY;
            textAreaBorders.x = 4 + 4;
            textAreaBorders.y = 2 + 1;
            minimumTextArea.width = 87;
            // The empty POL button is 24 pixels tall. This does not divide evenly by 5, so we force it to be 24 pixels tall.
            // (2 + 1) + 15 = 18 -> 18 + (3 + 3) = 24
            minimumTextArea.height = 18;
            maximumTextArea.height = 18;
            backgroundBorders.width = 4 + 3;
            backgroundBorders.height = 3 + 3;
            releasedOffset = { 4, 4 };
            pressedOffset = { 3, 5 };
            break;
        case ICN::EMPTY_GUILDWELL_BUTTON:
            font = fheroes2::FontColor::WHITE;
            textAreaBorders.x = 2 + 2;
            textAreaBorders.y = 0 + 0;
            minimumTextArea.width = 53;
            minimumTextArea.height = 13;
            maximumTextArea.height = 14;
            backgroundBorders.width = 5 + 3;
            backgroundBorders.height = 2 + 3;
            releasedOffset = { 4, 2 };
            pressedOffset = { 3, 3 };
            break;
        case ICN::EMPTY_VERTICAL_GOOD_BUTTON:
            font = fheroes2::FontColor::WHITE;
            textAreaBorders.x = 0 + 0;
            textAreaBorders.y = 2 + 2;
            minimumTextArea.width = 19;
            minimumTextArea.height = 110;
            // TODO: Currently the empty vertical button has a width of 28 which is not evenly dividable by 3 in resizeButton().
            // It should be widened by 1 px to allow for widening.
            maximumTextArea.width = 19;
            backgroundBorders.width = 5 + 4;
            backgroundBorders.height = 4 + 6;
            releasedOffset = { 5, 5 };
            pressedOffset = { 4, 6 };
            break;
        case ICN::EMPTY_MAP_SELECT_BUTTON:
            font = fheroes2::FontColor::WHITE;
            textAreaBorders.x = 2 + 2;
            textAreaBorders.y = 1 + 0;
            minimumTextArea.width = 60;
            minimumTextArea.height = 13;
            maximumTextArea.height = 14;
            backgroundBorders.width = 6 + 3;
            backgroundBorders.height = 2 + 3;
            releasedOffset = { 6, 3 };
            pressedOffset = { 5, 4 };
            break;
        default:
            // Was a new empty button template added?
            assert( 0 );
            break;
        }
    }
}

namespace fheroes2
{
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

    bool ButtonBase::drawOnPress( Display & output /* = Display::instance() */ )
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

    bool ButtonBase::drawOnRelease( Display & output /* = Display::instance() */ )
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
        return { _offsetX + sprite.x(), _offsetY + sprite.y(), sprite.width(), sprite.height() };
    }

    const Sprite & ButtonBase::_getDisabled() const
    {
        const Sprite & sprite = _getReleased();
        if ( !_disabledSprite || ( _releasedSprite != &sprite ) ) {
            _releasedSprite = &sprite;
            _disabledSprite.reset( new Sprite( sprite ) );
            ApplyPalette( *_disabledSprite, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
        }

        return *_disabledSprite;
    }

    const Sprite & Button::_getPressed() const
    {
        return AGG::GetICN( _icnId, _pressedIndex );
    }

    const Sprite & Button::_getReleased() const
    {
        return AGG::GetICN( _icnId, _releasedIndex );
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

    ButtonGroup::ButtonGroup( const Rect & area /* = Rect() */, const int buttonTypes /* = 0 */ )
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

    void ButtonGroup::createButton( const int32_t offsetX, const int32_t offsetY, const int icnId, const uint32_t releasedIndex, const uint32_t pressedIndex,
                                    const int returnValue )
    {
        _button.push_back( std::make_unique<Button>( offsetX, offsetY, icnId, releasedIndex, pressedIndex ) );
        _value.emplace_back( returnValue );
    }

    void ButtonGroup::createButton( const int32_t offsetX, const int32_t offsetY, const Sprite & released, const Sprite & pressed, const int returnValue )
    {
        _button.push_back( std::make_unique<ButtonSprite>( offsetX, offsetY, released, pressed ) );
        _value.emplace_back( returnValue );
    }

    void ButtonGroup::addButton( ButtonSprite && button, const int returnValue )
    {
        _button.push_back( std::make_unique<ButtonSprite>( std::move( button ) ) );
        _value.emplace_back( returnValue );
    }

    void ButtonGroup::draw( Image & output /* = Display::instance() */ ) const
    {
        for ( const auto & button : _button ) {
            button->draw( output );
        }
    }

    int ButtonGroup::processEvents()
    {
        LocalEvent & le = LocalEvent::Get();

        for ( const auto & button : _button ) {
            if ( button->isEnabled() ) {
                button->drawOnState( le.isMouseLeftButtonPressedInArea( button->area() ) );
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

    ButtonRestorer::ButtonRestorer( ButtonBase & button )
        : _button( button )
        , _isEnabled( button.isEnabled() )
    {
        if ( _isEnabled ) {
            Display & display = Display::instance();

            _button.disable();
            _button.draw( display );
            display.render( _button.area() );
        }
    }

    ButtonRestorer::~ButtonRestorer()
    {
        if ( _isEnabled ) {
            Display & display = Display::instance();

            _button.enable();
            _button.draw( display );
            display.render( _button.area() );
        }
    }

    void OptionButtonGroup::addButton( ButtonBase * button )
    {
        if ( button == nullptr ) {
            return;
        }

        _button.push_back( button );
        button->subscribe( this );
    }

    void OptionButtonGroup::draw( Image & output /* = Display::instance() */ ) const
    {
        for ( const ButtonBase * button : _button ) {
            button->draw( output );
        }
    }

    void OptionButtonGroup::senderUpdate( const ActionObject * sender )
    {
        if ( sender == nullptr ) {
            // How is it even possible?
            assert( 0 );

            return;
        }

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
        for ( ButtonBase * button : _button ) {
            button->subscribe( this );
        }
    }

    void OptionButtonGroup::unsubscribeAll()
    {
        for ( ButtonBase * button : _button ) {
            button->unsubscribe();
        }
    }

    void makeTransparentBackground( const Sprite & released, Sprite & pressed, const int backgroundIcnID )
    {
        // We need to copy the background image to pressed button only where it does not overlay the image of released button.
        const fheroes2::Sprite & background = fheroes2::AGG::GetICN( backgroundIcnID, 0 );
        // you are trying to apply transform on an image that is single-layered
        assert( !pressed.singleLayer() && !released.singleLayer() );
        const uint8_t * releasedTransform = released.transform();
        uint8_t * pressedTransform = pressed.transform();
        uint8_t * pressedImage = pressed.image();
        const uint8_t * backgroundImage
            = background.image() + ( background.width() - pressed.width() ) / 2 + ( background.height() - pressed.height() ) * background.width() / 2;
        const int32_t pressedArea = pressed.width() * pressed.height();

        for ( int32_t x = 0; x < pressedArea; ++x ) {
            if ( ( *( pressedTransform + x ) == 1 ) && ( *( releasedTransform + x ) == 0 ) ) {
                *( pressedImage + x ) = *( backgroundImage + ( x % pressed.width() ) + static_cast<ptrdiff_t>( x / pressed.width() ) * background.width() );
                *( pressedTransform + x ) = 0;
            }
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

        return { offsetX, offsetY, std::move( releasedWithBackground ), std::move( pressedWithBackground ), std::move( disabledWithBackground ) };
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

        return { offsetX + shadow.x(), offsetY + shadow.y(), std::move( releasedWithBackground ), std::move( pressedWithBackground ),
                 std::move( disabledWithBackground ) };
    }

    void getCustomNormalButton( Sprite & released, Sprite & pressed, const bool isEvilInterface, int32_t width, Point & releasedOffset, Point & pressedOffset,
                                const bool isTransparentBackground /* = false */ )
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

        released = resizeButton( originalReleased, { width, originalReleased.height() } );
        pressed = resizeButton( originalPressed, { width, originalPressed.height() } );

        if ( !isTransparentBackground ) {
            const int backgroundIcnId = isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK;
            makeTransparentBackground( released, pressed, backgroundIcnId );
        }
    }

    void getTextAdaptedButton( Sprite & released, Sprite & pressed, const char * text, const int emptyButtonIcnID, const int buttonBackgroundIcnID )
    {
        fheroes2::FontColor buttonFont = fheroes2::FontColor::WHITE;
        fheroes2::Point textAreaMargins = { 0, 3 };

        fheroes2::Size minimumTextArea = { 0, 15 };
        fheroes2::Size maximumTextArea = { 200, 200 }; // Why is such a wide button needed?

        fheroes2::Size backgroundBorders = { 0, 7 };

        fheroes2::Point releasedOffset = {};
        fheroes2::Point pressedOffset = {};

        getButtonSpecificValues( emptyButtonIcnID, buttonFont, textAreaMargins, minimumTextArea, maximumTextArea, backgroundBorders, releasedOffset, pressedOffset );

        const fheroes2::FontType releasedButtonFont{ fheroes2::FontSize::BUTTON_RELEASED, buttonFont };

        const char * translatedText = _( text );
        const char * supportedText = fheroes2::isFontAvailable( translatedText, releasedButtonFont ) ? translatedText : text;

        const fheroes2::Text releasedText( supportedText, releasedButtonFont );
        const fheroes2::Text pressedText( supportedText, { fheroes2::FontSize::BUTTON_PRESSED, buttonFont } );

        // We need to pass an argument to width() so that it correctly accounts for multi-lined texts.
        // TODO: Remove the need for the argument once width() has been improved to handle this.
        const int32_t textWidth = releasedText.width( maximumTextArea.width );
        assert( textWidth > 0 );

        const int32_t borderedTextWidth = textWidth + textAreaMargins.x;

        const int32_t textAreaWidth = std::clamp( borderedTextWidth, minimumTextArea.width, maximumTextArea.width );
        assert( textAreaWidth + backgroundBorders.width > 0 );

        const int32_t textHeight = releasedText.height( textAreaWidth );
        assert( textHeight > 0 );

        const int32_t borderedTextHeight = textHeight + textAreaMargins.y;
        const int32_t textAreaHeight = std::clamp( borderedTextHeight, minimumTextArea.height, maximumTextArea.height );

        assert( textAreaHeight + backgroundBorders.height > 0 );

        released = resizeButton( AGG::GetICN( emptyButtonIcnID, 0 ), { textAreaWidth + backgroundBorders.width, textAreaHeight + backgroundBorders.height } );
        pressed = resizeButton( AGG::GetICN( emptyButtonIcnID, 1 ), { textAreaWidth + backgroundBorders.width, textAreaHeight + backgroundBorders.height } );

        if ( buttonBackgroundIcnID != ICN::UNKNOWN ) {
            makeTransparentBackground( released, pressed, buttonBackgroundIcnID );
        }

        const fheroes2::Size releasedTextSize( releasedText.width( textAreaWidth ), releasedText.height( textAreaWidth ) );
        const fheroes2::Size pressedTextSize( pressedText.width( textAreaWidth ), pressedText.height( textAreaWidth ) );

        // The button font letters are all shifted 1 pixel to the left due to shadows, so we have to add 1 to the x position when drawing
        // to properly center-align.
        releasedText.draw( releasedOffset.x + 1, releasedOffset.y + ( textAreaHeight - releasedTextSize.height ) / 2, textAreaWidth, released );
        pressedText.draw( pressedOffset.x + 1, pressedOffset.y + ( textAreaHeight - pressedTextSize.height ) / 2, textAreaWidth, pressed );
    }

    void makeButtonSprites( Sprite & released, Sprite & pressed, const std::string & text, const int32_t buttonWidth, const bool isEvilInterface,
                            const bool isTransparentBackground )
    {
        fheroes2::Point releasedOffset;
        fheroes2::Point pressedOffset;
        fheroes2::getCustomNormalButton( released, pressed, isEvilInterface, buttonWidth, releasedOffset, pressedOffset, isTransparentBackground );

        const fheroes2::FontColor fontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;

        const fheroes2::Text releasedText( text, { fheroes2::FontSize::BUTTON_RELEASED, fontColor } );
        const fheroes2::Text pressedText( text, { fheroes2::FontSize::BUTTON_PRESSED, fontColor } );

        const fheroes2::Point textOffset{ ( buttonWidth - releasedText.width() ) / 2, ( 16 - fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) ) / 2 };

        releasedText.draw( releasedOffset.x + textOffset.x, releasedOffset.y + textOffset.y, released );
        pressedText.draw( pressedOffset.x + textOffset.x, pressedOffset.y + textOffset.y, pressed );
    }
}
