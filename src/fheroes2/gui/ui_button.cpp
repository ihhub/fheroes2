/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2025                                             *
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

        if ( originalHeight == buttonSize.height && originalWidth == buttonSize.width ) {
            return original;
        }

        fheroes2::Image output( buttonSize.width, buttonSize.height );
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
        // Buttons that are taller but less wide.
        else if ( buttonSize.height > originalHeight && buttonSize.width < originalWidth ) {
            // Height increase
            const int32_t middleHeight = originalHeight / 5;
            const int32_t overallMiddleHeight = buttonSize.height - middleHeight * 2;
            const int32_t middleHeightCount = overallMiddleHeight / middleHeight;
            const int32_t middleHeightLeftOver = overallMiddleHeight - middleHeightCount * middleHeight;

            const int32_t middleWidth = originalWidth / 3;

            // The new button cannot even fit the two end corners. Are you using the wrong empty button to generate the new one?
            assert( buttonSize.width >= middleWidth * 2 );

            const int32_t rightSideWidth = buttonSize.width - middleWidth;

            fheroes2::Copy( original, 0, 0, output, 0, 0, middleWidth, middleHeight );
            fheroes2::Copy( original, originalWidth - rightSideWidth, 0, output, middleWidth, 0, rightSideWidth, middleHeight );

            int32_t offsetY = middleHeight;
            for ( int32_t i = 0; i < middleHeightCount; ++i ) {
                fheroes2::Copy( original, 0, middleHeight, output, 0, offsetY, middleWidth, middleHeight );
                fheroes2::Copy( original, originalWidth - rightSideWidth, middleHeight, output, middleWidth, offsetY, rightSideWidth, middleHeight );
                offsetY += middleHeight;
            }

            if ( middleHeightLeftOver > 0 ) {
                fheroes2::Copy( original, 0, middleHeight, output, 0, offsetY, middleWidth, middleHeightLeftOver );
                fheroes2::Copy( original, originalWidth - rightSideWidth, middleHeight, output, middleWidth, offsetY, rightSideWidth, middleHeightLeftOver );
                offsetY += middleHeightLeftOver;
            }
            assert( offsetY + originalHeight - middleHeight * 4 == buttonSize.height );

            fheroes2::Copy( original, 0, originalHeight - middleHeight, output, 0, offsetY, middleWidth, middleHeight );
            fheroes2::Copy( original, originalWidth - rightSideWidth, originalHeight - middleHeight, output, middleWidth, offsetY, rightSideWidth, middleHeight );
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

    void addButtonShine( fheroes2::Sprite & buttonImage, const int emptyButtonIcnID )
    {
        const bool isGoodButton = ( emptyButtonIcnID == ICN::EMPTY_GOOD_BUTTON );
        if ( isGoodButton || emptyButtonIcnID == ICN::EMPTY_EVIL_BUTTON ) {
            const uint8_t firstColor = 10;
            const uint8_t secondColor = isGoodButton ? 37 : 15;
            const uint8_t lastColor = isGoodButton ? 39 : 16;
            // Left-side shine
            fheroes2::SetPixel( buttonImage, 11, 4, firstColor );
            fheroes2::SetPixel( buttonImage, 13, 4, firstColor );
            fheroes2::SetPixel( buttonImage, 9, 6, firstColor );
            fheroes2::SetPixel( buttonImage, 10, 5, secondColor );
            fheroes2::SetPixel( buttonImage, 12, 5, secondColor );
            fheroes2::SetPixel( buttonImage, 8, 7, lastColor );
            fheroes2::SetPixel( buttonImage, 15, 4, lastColor );
            // Right-side shine
            const int32_t buttonWidth = buttonImage.width();
            fheroes2::SetPixel( buttonImage, buttonWidth - 9, 4, firstColor );
            fheroes2::SetPixel( buttonImage, buttonWidth - 7, 4, firstColor );
            fheroes2::DrawLine( buttonImage, { buttonWidth - 10, 5 }, { buttonWidth - 11, 6 }, secondColor );
            fheroes2::SetPixel( buttonImage, buttonWidth - 8, 5, secondColor );

            const int32_t buttonHeight = buttonImage.height();
            // To avoid overcrowding the '...' virtual keyboard button with decorations we set 50.
            if ( buttonWidth > 50 ) {
                // Longer left-side shine
                fheroes2::DrawLine( buttonImage, { 11, 4 }, { 9, 6 }, firstColor );
                fheroes2::SetPixel( buttonImage, 8, 7, lastColor );
                fheroes2::SetPixel( buttonImage, 13, 4, secondColor );
                fheroes2::DrawLine( buttonImage, { 12, 5 }, { 9, 8 }, lastColor );
                fheroes2::DrawLine( buttonImage, { 8, 9 }, { 7, 10 }, firstColor );
                fheroes2::SetPixel( buttonImage, 6, 11, lastColor );
                fheroes2::DrawLine( buttonImage, { 15, 4 }, { 13, 6 }, firstColor );
                fheroes2::SetPixel( buttonImage, 12, 7, lastColor );
                // Longer right-side shine
                fheroes2::SetPixel( buttonImage, buttonWidth - 10, 2, firstColor );
                fheroes2::SetPixel( buttonImage, buttonWidth - 8, 3, secondColor );
                fheroes2::SetPixel( buttonImage, buttonWidth - 11, 3, secondColor );
                fheroes2::SetPixel( buttonImage, buttonWidth - 12, 4, firstColor );
                fheroes2::DrawLine( buttonImage, { buttonWidth - 13, 5 }, { buttonWidth - 14, 6 }, lastColor );
                fheroes2::DrawLine( buttonImage, { buttonWidth - 10, 5 }, { buttonWidth - 12, 7 }, firstColor );
                fheroes2::DrawLine( buttonImage, { buttonWidth - 13, 8 }, { buttonWidth - 14, 9 }, secondColor );
                fheroes2::DrawLine( buttonImage, { buttonWidth - 8, 5 }, { buttonWidth - 10, 7 }, firstColor );
                fheroes2::SetPixel( buttonImage, buttonWidth - 11, 8, lastColor );
                // Bottom left-side shine
                fheroes2::SetPixel( buttonImage, 11, buttonHeight - 6, lastColor );
                fheroes2::DrawLine( buttonImage, { 12, buttonHeight - 7 }, { 13, buttonHeight - 8 }, firstColor );
                fheroes2::SetPixel( buttonImage, 14, buttonHeight - 9, lastColor );
                // This is mainly to decorate the space bar button
                if ( buttonWidth > 96 || buttonHeight > 25 ) {
                    // Longer bottom left shine
                    fheroes2::SetPixel( buttonImage, 10, buttonHeight - 5, lastColor );
                    // Center part shine
                    fheroes2::SetPixel( buttonImage, 30, buttonHeight - 5, secondColor );
                    fheroes2::SetPixel( buttonImage, 31, buttonHeight - 6, lastColor );
                    fheroes2::DrawLine( buttonImage, { 32, buttonHeight - 5 }, { 34, buttonHeight - 7 }, firstColor );
                    fheroes2::SetPixel( buttonImage, 35, buttonHeight - 8, secondColor );
                    fheroes2::SetPixel( buttonImage, 36, buttonHeight - 9, lastColor );
                    fheroes2::SetPixel( buttonImage, 34, buttonHeight - 5, secondColor );
                    fheroes2::SetPixel( buttonImage, 35, buttonHeight - 6, lastColor );
                }
                if ( buttonHeight > 25 ) {
                    // Bottom right-side shine
                    fheroes2::DrawLine( buttonImage, { buttonWidth - 20, buttonHeight - 5 }, { buttonWidth - 16, buttonHeight - 9 }, secondColor );
                    fheroes2::SetPixel( buttonImage, buttonWidth - 15, buttonHeight - 10, lastColor );
                    fheroes2::DrawLine( buttonImage, { buttonWidth - 18, buttonHeight - 5 }, { buttonWidth - 14, buttonHeight - 9 }, firstColor );
                    fheroes2::DrawLine( buttonImage, { buttonWidth - 13, buttonHeight - 10 }, { buttonWidth - 12, buttonHeight - 11 }, secondColor );
                    fheroes2::DrawLine( buttonImage, { buttonWidth - 11, buttonHeight - 12 }, { buttonWidth - 10, buttonHeight - 13 }, lastColor );
                    fheroes2::DrawLine( buttonImage, { buttonWidth - 16, buttonHeight - 5 }, { buttonWidth - 12, buttonHeight - 9 }, secondColor );
                    fheroes2::SetPixel( buttonImage, buttonWidth - 11, buttonHeight - 10, lastColor );
                }
            }
        }
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

        _updateReleasedArea();
    }

    void ButtonBase::disable()
    {
        _isEnabled = false;
        _isPressed = false; // button can't be disabled and pressed
        notifySubscriber();

        _updateReleasedArea();
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
            Blit( sprite, output, _areaPressed );
        }
        else {
            const Sprite & sprite = isEnabled() ? _getReleased() : _getDisabled();
            Blit( sprite, output, _areaReleased );
        }

        return true;
    }

    void ButtonBase::drawShadow( Image & output ) const
    {
        const Point buttonPoint = area().getPosition();
        // Did you forget to set the position of the button?
        assert( buttonPoint != Point( 0, 0 ) );
        fheroes2::addGradientShadow( _getReleased(), output, buttonPoint, { -5, 5 } );
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
        case Dialog::YES | Dialog::NO: {
            const Sprite & yesButtonSprite = AGG::GetICN( buttonYesIcnID, 0 );
            const Sprite & noButtonSprite = AGG::GetICN( buttonNoIcnID, 0 );

            const int32_t horizontalFreeSpace = area.width - yesButtonSprite.width() - noButtonSprite.width();
            const int32_t padding = horizontalFreeSpace / 4;
            assert( horizontalFreeSpace > 0 );

            offset.x = area.x + padding;
            offset.y = area.y + area.height - yesButtonSprite.height();
            createButton( offset.x, offset.y, buttonYesIcnID, 0, 1, Dialog::YES );

            offset.x = area.x + area.width - noButtonSprite.width() - padding;
            offset.y = area.y + area.height - noButtonSprite.height();
            createButton( offset.x, offset.y, buttonNoIcnID, 0, 1, Dialog::NO );
            break;
        }

        case Dialog::OK | Dialog::CANCEL: {
            const Sprite & okayButtonSprite = AGG::GetICN( buttonOkayIcnID, 0 );
            const Sprite & cancelButtonSprite = AGG::GetICN( buttonCancelIcnID, 0 );
            const int32_t horizontalFreeSpace = area.width - okayButtonSprite.width() - cancelButtonSprite.width();
            const int32_t padding = horizontalFreeSpace / 4;
            assert( horizontalFreeSpace > 0 );

            offset.x = area.x + padding;
            offset.y = area.y + area.height - okayButtonSprite.height();
            createButton( offset.x, offset.y, buttonOkayIcnID, 0, 1, Dialog::OK );

            offset.x = area.x + area.width - cancelButtonSprite.width() - padding;
            offset.y = area.y + area.height - cancelButtonSprite.height();
            createButton( offset.x, offset.y, buttonCancelIcnID, 0, 1, Dialog::CANCEL );
            break;
        }

        case Dialog::OK: {
            const Sprite & okayButtonSprite = AGG::GetICN( buttonOkayIcnID, 0 );

            offset.x = area.x + ( area.width - okayButtonSprite.width() ) / 2;
            offset.y = area.y + area.height - okayButtonSprite.height();
            createButton( offset.x, offset.y, buttonOkayIcnID, 0, 1, Dialog::OK );
            break;
        }

        case Dialog::CANCEL: {
            const Sprite & cancelButtonSprite = AGG::GetICN( buttonCancelIcnID, 0 );

            offset.x = area.x + ( area.width - cancelButtonSprite.width() ) / 2;
            offset.y = area.y + area.height - cancelButtonSprite.height();
            createButton( offset.x, offset.y, buttonCancelIcnID, 0, 1, Dialog::CANCEL );
            break;
        }

        default:
            break;
        }
    }

    ButtonGroup::ButtonGroup( const std::vector<const char *> & texts )
    {
        const size_t textCount = texts.size();
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        std::vector<Sprite> sprites;
        makeSymmetricBackgroundSprites( sprites, texts, isEvilInterface, 86 );

        for ( size_t i = 0; i < textCount; ++i ) {
            createButton( 0, 0, std::move( sprites[i * 2] ), std::move( sprites[i * 2 + 1] ), static_cast<int>( i ) );
        }
    }

    ButtonGroup::ButtonGroup( const int icnID )
    {
        // Button ICNs contain released and pressed states so we divide by two to find the number of buttons they correspond to.
        const int32_t buttonCount = static_cast<int32_t>( fheroes2::AGG::GetICNCount( icnID ) ) / 2;
        for ( int32_t i = 0; i < buttonCount; ++i ) {
            createButton( 0, 0, icnID, i * 2, i * 2 + 1, i );
        }
    }

    void ButtonGroup::createButton( const int32_t offsetX, const int32_t offsetY, const int icnId, const uint32_t releasedIndex, const uint32_t pressedIndex,
                                    const int returnValue )
    {
        _button.push_back( std::make_unique<Button>( offsetX, offsetY, icnId, releasedIndex, pressedIndex ) );
        _value.emplace_back( returnValue );
    }

    void ButtonGroup::createButton( const int32_t offsetX, const int32_t offsetY, Sprite released, Sprite pressed, const int returnValue )
    {
        _button.push_back( std::make_unique<ButtonSprite>( offsetX, offsetY, std::move( released ), std::move( pressed ) ) );
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

    void ButtonGroup::drawShadows( Image & output ) const
    {
        for ( const auto & button : _button ) {
            button->drawShadow( output );
        }
    }

    void ButtonGroup::disable() const
    {
        for ( const auto & button : _button ) {
            button->disable();
        }
    }

    void ButtonGroup::enable() const
    {
        for ( const auto & button : _button ) {
            button->enable();
        }
    }

    void ButtonGroup::drawOnState( const LocalEvent & le ) const
    {
        for ( const auto & button : _button ) {
            button->drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( button->area() ) );
        }
    }

    int ButtonGroup::processEvents()
    {
        LocalEvent & le = LocalEvent::Get();

        for ( const auto & button : _button ) {
            if ( button->isEnabled() ) {
                button->drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( button->area() ) );
            }
        }

        const size_t buttonsCount = _button.size();

        assert( buttonsCount == _value.size() );

        for ( size_t i = 0; i < buttonsCount; ++i ) {
            if ( _button[i]->isEnabled() && le.MouseClickLeft( _button[i]->area() ) ) {
                return _value[i];
            }
        }

        if ( ( buttonsCount == 1 ) && ( _value[0] == Dialog::OK || _value[0] == Dialog::CANCEL ) && Game::HotKeyCloseWindow() ) {
            // This dialog has only one OK or CANCEL button so allow to close it by any hotkey for these buttons.
            // Reset the hotkey pressed state.
            le.reset();
            return _value[0];
        }

        for ( size_t i = 0; i < buttonsCount; ++i ) {
            if ( _button[i]->isEnabled() ) {
                if ( ( _value[i] == Dialog::YES || _value[i] == Dialog::OK ) && Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
                    // Reset the hotkey pressed state.
                    le.reset();
                    return _value[i];
                }
                if ( ( _value[i] == Dialog::CANCEL || _value[i] == Dialog::NO ) && Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                    // Reset the hotkey pressed state.
                    le.reset();
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
            display.updateNextRenderRoi( _button.area() );
        }
    }

    ButtonRestorer::~ButtonRestorer()
    {
        if ( _isEnabled ) {
            Display & display = Display::instance();

            _button.enable();
            _button.draw( display );
            display.updateNextRenderRoi( _button.area() );
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

    void OptionButtonGroup::unsubscribeAll() const
    {
        for ( ButtonBase * button : _button ) {
            button->unsubscribe();
        }
    }

    void makeTransparentBackground( const Sprite & released, Sprite & pressed, const int backgroundIcnID )
    {
        // We need to copy the background image to pressed button only where it does not overlay the image of released button.
        const Sprite & background = AGG::GetICN( backgroundIcnID, 0 );
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
        const Sprite & shadow = makeShadow( released, shadowOffset, 3 );

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

    void getCustomNormalButton( Sprite & released, Sprite & pressed, const bool isEvilInterface, Size buttonSize, Point & releasedOffset, Point & pressedOffset,
                                const int backgroundIcnId )
    {
        assert( buttonSize.width > 0 && buttonSize.height > 0 );

        releasedOffset = { 7, 5 };
        pressedOffset = { 6, 6 };

        // The actual button sprite is 10 pixels wider.
        buttonSize.width += 10;

        const int32_t minimumButtonWidth = 16;
        const int32_t maximumButtonWidth = 200; // Why is such a wide button needed?
        buttonSize.width = std::clamp( buttonSize.width, minimumButtonWidth, maximumButtonWidth );

        const int32_t minimumButtonHeight = 25;
        const int32_t maximumButtonHeight = 200; // Why is such a tall button needed?
        buttonSize.height = std::clamp( buttonSize.height, minimumButtonHeight, maximumButtonHeight );

        const int32_t icnId = isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON;

        const Sprite & originalReleased = AGG::GetICN( icnId, 0 );
        const Sprite & originalPressed = AGG::GetICN( icnId, 1 );

        released = resizeButton( originalReleased, buttonSize );
        pressed = resizeButton( originalPressed, buttonSize );

        addButtonShine( released, icnId );

        if ( backgroundIcnId != ICN::UNKNOWN ) {
            makeTransparentBackground( released, pressed, backgroundIcnId );
        }
    }

    void getTextAdaptedSprite( Sprite & released, Sprite & pressed, const char * text, const int emptyButtonIcnID, const int buttonBackgroundIcnID )
    {
        FontColor buttonFont = FontColor::WHITE;
        Point textAreaMargins = { 0, 3 };

        Size minimumTextArea = { 0, 15 };
        Size maximumTextArea = { 200, 200 }; // Why is such a wide button needed?

        Size backgroundBorders = { 0, 7 };

        Point releasedOffset;
        Point pressedOffset;

        getButtonSpecificValues( emptyButtonIcnID, buttonFont, textAreaMargins, minimumTextArea, maximumTextArea, backgroundBorders, releasedOffset, pressedOffset );

        const Text releasedText( text, { FontSize::BUTTON_RELEASED, buttonFont } );
        const Text pressedText( text, { FontSize::BUTTON_PRESSED, buttonFont } );

        // We need to pass an argument to width() so that it correctly accounts for multi-lined texts.
        const int32_t textWidth = releasedText.width( maximumTextArea.width );
        assert( textWidth > 0 );

        const int32_t borderedTextWidth = textWidth + textAreaMargins.x;

        const int32_t textAreaWidth = std::clamp( borderedTextWidth, minimumTextArea.width, maximumTextArea.width );
        assert( textAreaWidth + backgroundBorders.width > 0 );

        const int32_t textHeight = releasedText.height( textAreaWidth );
        assert( textHeight > 0 );

        // Add extra y-margin for multi-lined texts on normal buttons.
        if ( ( emptyButtonIcnID == ICN::EMPTY_EVIL_BUTTON || emptyButtonIcnID == ICN::EMPTY_GOOD_BUTTON ) && textHeight > 17 ) {
            textAreaMargins.y += 16;
        }
        const int32_t borderedTextHeight = textHeight + textAreaMargins.y;
        const int32_t textAreaHeight = std::clamp( borderedTextHeight, minimumTextArea.height, maximumTextArea.height );

        const Size buttonSize( textAreaWidth + backgroundBorders.width, textAreaHeight + backgroundBorders.height );

        assert( buttonSize.height > 0 );

        released = resizeButton( AGG::GetICN( emptyButtonIcnID, 0 ), buttonSize );
        pressed = resizeButton( AGG::GetICN( emptyButtonIcnID, 1 ), buttonSize );

        if ( buttonBackgroundIcnID != ICN::UNKNOWN ) {
            makeTransparentBackground( released, pressed, buttonBackgroundIcnID );
        }
        if ( emptyButtonIcnID == ICN::EMPTY_EVIL_BUTTON || emptyButtonIcnID == ICN::EMPTY_GOOD_BUTTON ) {
            addButtonShine( released, emptyButtonIcnID );
        }

        // The button font letters are all shifted 1 pixel to the left due to shadows, so we have to add 1 to the x position when drawing
        // to properly center-align.
        releasedText.draw( releasedOffset.x + 1, releasedOffset.y + ( textAreaHeight - textHeight ) / 2, textAreaWidth, released );
        pressedText.draw( pressedOffset.x + 1, pressedOffset.y + ( textAreaHeight - textHeight ) / 2, textAreaWidth, pressed );
    }

    void makeButtonSprites( Sprite & released, Sprite & pressed, const std::string & text, const Size buttonSize, const bool isEvilInterface, const int backgroundIcnId )
    {
        Point releasedOffset;
        Point pressedOffset;
        getCustomNormalButton( released, pressed, isEvilInterface, buttonSize, releasedOffset, pressedOffset, backgroundIcnId );

        const fheroes2::FontColor buttonFontColor = isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE;
        renderTextOnButton( released, pressed, text, releasedOffset, pressedOffset, buttonSize, buttonFontColor );
    }

    void makeSymmetricBackgroundSprites( std::vector<Sprite> & backgroundSprites, const std::vector<const char *> & texts, const bool isEvilInterface,
                                         const int32_t minWidth )
    {
        if ( texts.size() < 2 ) {
            // You are trying to make a group of buttons with 0 or only one text.
            assert( 0 );
            return;
        }

        backgroundSprites.resize( texts.size() * 2 );

        const FontType buttonFontType = { FontSize::BUTTON_RELEASED, ( isEvilInterface ? fheroes2::FontColor::GRAY : fheroes2::FontColor::WHITE ) };

        std::vector<Text> buttonTexts;
        buttonTexts.reserve( texts.size() );
        for ( const char * text : texts ) {
            buttonTexts.emplace_back( text, buttonFontType );
        }
        // This max value is needed to make the width and height calculations correctly take into account that texts with \n are multi-lined.
        const int32_t maxAllowedWidth = 200;
        int32_t maxWidth = 0;

        for ( const auto & text : buttonTexts ) {
            maxWidth = std::max( maxWidth, text.width( maxAllowedWidth ) );
        }

        // We add 6 to have some extra horizontal margin.
        const int32_t finalWidth = std::clamp( maxWidth + 6, minWidth, maxAllowedWidth );

        int32_t maxHeight = 0;
        for ( const auto & text : buttonTexts ) {
            maxHeight = std::max( maxHeight, text.height( finalWidth ) );
        }

        // Add extra vertical margin depending on how many lines of text there are.
        if ( maxHeight > getFontHeight( buttonFontType.size ) ) {
            const int32_t maxAllowedHeight = 200;
            maxHeight = std::clamp( maxHeight, 56, maxAllowedHeight );
        }
        else {
            maxHeight += 10;
        }

        const int backgroundIcnID = isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK;

        for ( size_t i = 0; i < buttonTexts.size(); ++i ) {
            Sprite & released = backgroundSprites[i * 2];
            Sprite & pressed = backgroundSprites[i * 2 + 1];
            makeButtonSprites( released, pressed, buttonTexts[i].text(), { finalWidth, maxHeight }, isEvilInterface, backgroundIcnID );
        }
    }

    const char * getSupportedText( const char * untranslatedText, const FontType font )
    {
        const char * translatedText = _( untranslatedText );
        return isFontAvailable( translatedText, font ) ? translatedText : untranslatedText;
    }

    void renderTextOnButton( Image & releasedState, Image & pressedState, const std::string & text, const Point & releasedTextOffset, const Point & pressedTextOffset,
                             const Size & buttonSize, const FontColor fontColor )
    {
        const FontType releasedFont{ FontSize::BUTTON_RELEASED, fontColor };
        const FontType pressedFont{ FontSize::BUTTON_PRESSED, fontColor };

        const Text releasedText( text, releasedFont );
        const Text pressedText( text, pressedFont );

        releasedText.draw( releasedTextOffset.x, ( buttonSize.height - releasedText.height( buttonSize.width ) ) / 2, buttonSize.width, releasedState );
        pressedText.draw( pressedTextOffset.x, ( buttonSize.height - pressedText.height( buttonSize.width ) ) / 2 + 1, buttonSize.width, pressedState );
    }
}
