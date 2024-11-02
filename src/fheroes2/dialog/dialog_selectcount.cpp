/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h" // IWYU pragma: associated
#include "game_delays.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_keyboard.h"
#include "ui_language.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    void SwitchMaxMinButtons( fheroes2::ButtonBase & minButton, fheroes2::ButtonBase & maxButton, const int32_t currentValue, const int32_t minimumValue )
    {
        const bool isMinValue = ( currentValue <= minimumValue );

        if ( isMinValue ) {
            minButton.hide();
            maxButton.show();
        }
        else {
            minButton.show();
            maxButton.hide();
        }

        minButton.draw();
        maxButton.draw();
    }
}

bool Dialog::SelectCount( std::string header, const int32_t min, const int32_t max, int32_t & selectedValue, const int32_t step,
                          const fheroes2::DialogElement * uiElement )
{
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::Text headerText( std::move( header ), fheroes2::FontType::normalWhite() );
    int32_t headerOffsetY{ 10 };
    const int32_t selectionAreaHeight{ 30 };
    const int32_t headerHeight = headerText.height( fheroes2::boxAreaWidthPx );
    const int32_t uiWidth = uiElement ? uiElement->area().width : 0;
    const int32_t uiHeight = uiElement ? uiElement->area().height : 0;

    const FrameBox box( headerHeight + headerOffsetY + selectionAreaHeight + uiHeight, true );

    const fheroes2::Rect & windowArea = box.GetArea();

    fheroes2::Display & display = fheroes2::Display::instance();
    headerText.draw( windowArea.x, windowArea.y, fheroes2::boxAreaWidthPx, display );

    const fheroes2::Point uiOffset{ windowArea.x + ( windowArea.width - uiWidth ) / 2, windowArea.y + headerHeight + headerOffsetY };
    if ( uiElement ) {
        uiElement->draw( display, uiOffset );
        headerOffsetY *= 2;
    }

    const fheroes2::Size valueSelectionSize{ fheroes2::ValueSelectionDialogElement::getArea() };
    const fheroes2::Rect selectionBoxArea{ windowArea.x + 80, windowArea.y + headerOffsetY + headerHeight + uiHeight, valueSelectionSize.width,
                                           valueSelectionSize.height };

    fheroes2::ValueSelectionDialogElement valueSelectionElement( min, max, selectedValue, step, selectionBoxArea.getPosition() );
    valueSelectionElement.ignoreMouseWheelEventRoiCheck();
    valueSelectionElement.draw( display );

    fheroes2::ButtonGroup btnGroups( box.GetArea(), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    const fheroes2::Text mainText( _( "MAX" ), fheroes2::FontType::smallWhite() );
    const int32_t maxAreaOffsetY{ ( 26 - mainText.height() ) / 2 };
    const fheroes2::Rect rectMax{ windowArea.x + 176, windowArea.y + headerOffsetY + headerHeight + uiHeight + maxAreaOffsetY, mainText.width(), mainText.height() };
    mainText.draw( rectMax.x, rectMax.y + 2, display );

    display.render();

    int result = Dialog::ZERO;

    const fheroes2::Rect uiRect = uiElement ? fheroes2::Rect{ uiOffset, uiElement->area() } : fheroes2::Rect{};

    LocalEvent & le = LocalEvent::Get();
    while ( result == Dialog::ZERO && le.HandleEvents() ) {
        bool redraw_count = false;

        if ( fheroes2::processIntegerValueTyping( min, max, selectedValue ) ) {
            valueSelectionElement.setValue( selectedValue );
            redraw_count = true;
        }

        if ( le.MouseClickLeft( rectMax ) ) {
            valueSelectionElement.setValue( max );
            redraw_count = true;
        }

        if ( valueSelectionElement.processEvents() ) {
            selectedValue = valueSelectionElement.getValue();
            redraw_count = true;
        }

        if ( uiElement && ( le.isMouseLeftButtonReleasedInArea( uiRect ) || le.isMouseRightButtonPressedInArea( uiRect ) ) ) {
            uiElement->processEvents( uiOffset );
            display.render();
        }

        if ( redraw_count ) {
            valueSelectionElement.draw( display );
            display.render( selectionBoxArea );
        }

        result = btnGroups.processEvents();
    }

    selectedValue = ( result == Dialog::OK ) ? valueSelectionElement.getValue() : 0;

    return result == Dialog::OK;
}

bool Dialog::inputString( const fheroes2::TextBase & title, const fheroes2::TextBase & body, std::string & result, const size_t charLimit, const bool isMultiLine,
                          const std::optional<fheroes2::SupportedLanguage> & textLanguage )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    result.reserve( charLimit == 0 ? 48 : charLimit );
    size_t charInsertPos = result.size();

    const bool hasTitle = !title.empty();

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    const int32_t titleHeight = hasTitle ? title.height( fheroes2::boxAreaWidthPx ) + 10 : 0;
    const int32_t keyBoardButtonExtraHeight = 20;

    const fheroes2::Sprite & inputArea = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::BUYBUILD : ICN::BUYBUILE ), 3 );

    const int32_t inputAreaWidth = isMultiLine ? 224 : inputArea.width();
    const int32_t inputAreaHeight = isMultiLine ? 265 : inputArea.height();

    const int32_t textboxHeight = body.height( fheroes2::boxAreaWidthPx );

    const int32_t frameBoxHeight = 10 + titleHeight + textboxHeight + 10 + inputAreaHeight + keyBoardButtonExtraHeight;
    const FrameBox box( frameBoxHeight, true );
    const fheroes2::Rect & frameBoxArea = box.GetArea();

    // Title text.
    if ( hasTitle ) {
        title.draw( frameBoxArea.x, frameBoxArea.y + 12, fheroes2::boxAreaWidthPx, display );
    }

    // Header text.
    body.draw( frameBoxArea.x, frameBoxArea.y + 12 + titleHeight, fheroes2::boxAreaWidthPx, display );

    fheroes2::Point dst_pt{ frameBoxArea.x + ( frameBoxArea.width - inputAreaWidth ) / 2, frameBoxArea.y + 10 + titleHeight + textboxHeight + 10 };

    fheroes2::Rect inputAreaOffset;
    if ( isMultiLine ) {
        inputAreaOffset = { 5, 5, -10, -10 };
        fheroes2::StandardWindow::applyTextBackgroundShading( display, { dst_pt.x, dst_pt.y, inputAreaWidth, inputAreaHeight } );
    }
    else {
        inputAreaOffset = { 13, 1, -26, -3 };
        fheroes2::Blit( inputArea, display, dst_pt.x, dst_pt.y );
    }
    const fheroes2::Rect textInputArea( dst_pt.x + inputAreaOffset.x, dst_pt.y + inputAreaOffset.y, inputAreaWidth + inputAreaOffset.width,
                                        inputAreaHeight + inputAreaOffset.height );

    fheroes2::ImageRestorer textBackground( display, textInputArea.x, textInputArea.y, textInputArea.width, textInputArea.height );

    bool isCursorVisible = true;
    const fheroes2::FontType fontType( fheroes2::FontType::normalWhite() );
    fheroes2::Text text( insertCharToString( result, charInsertPos, isCursorVisible ? '_' : '\x7F' ), fontType, textLanguage );
    if ( !isMultiLine ) {
        text.fitToOneRow( textInputArea.width, false );
    }
    text.drawInRoi( textInputArea.x, textInputArea.y + 2, textInputArea.width, display, textInputArea );

    const int okayButtonICNID = isEvilInterface ? ICN::UNIFORM_EVIL_OKAY_BUTTON : ICN::UNIFORM_GOOD_OKAY_BUTTON;

    dst_pt.x = frameBoxArea.x;
    dst_pt.y = frameBoxArea.y + frameBoxArea.height - fheroes2::AGG::GetICN( okayButtonICNID, 0 ).height();
    fheroes2::Button buttonOk( dst_pt.x, dst_pt.y, okayButtonICNID, 0, 1 );

    const int cancelButtonIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_CANCEL_BUTTON : ICN::UNIFORM_GOOD_CANCEL_BUTTON;
    const fheroes2::Sprite & cancelButtonIcn = fheroes2::AGG::GetICN( cancelButtonIcnID, 0 );

    dst_pt.x = frameBoxArea.x + frameBoxArea.width - cancelButtonIcn.width();
    dst_pt.y = frameBoxArea.y + frameBoxArea.height - cancelButtonIcn.height();
    fheroes2::Button buttonCancel( dst_pt.x, dst_pt.y, cancelButtonIcnID, 0, 1 );

    // Generate a button to open the Virtual Keyboard window.
    fheroes2::Sprite releasedVirtualKB;
    fheroes2::Sprite pressedVirtualKB;
    const int32_t buttonVirtualKBWidth = 40;

    makeButtonSprites( releasedVirtualKB, pressedVirtualKB, "...", buttonVirtualKBWidth, isEvilInterface, true );
    // To center the button horizontally we have to take into account that actual button sprite is 10 pixels longer then the requested button width.
    fheroes2::ButtonSprite buttonVirtualKB = makeButtonWithBackground( frameBoxArea.x + ( frameBoxArea.width - buttonVirtualKBWidth - 10 ) / 2, dst_pt.y - 30,
                                                                       releasedVirtualKB, pressedVirtualKB, display );

    if ( result.empty() ) {
        buttonOk.disable();
    }
    else {
        buttonOk.enable();
    }

    buttonOk.draw();
    buttonCancel.draw();
    buttonVirtualKB.draw();

    display.render();

    LocalEvent & le = LocalEvent::Get();

    Game::AnimateResetDelay( Game::DelayType::CURSOR_BLINK_DELAY );

    const bool isInGameKeyboardRequired = System::isVirtualKeyboardSupported();

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::DelayType::CURSOR_BLINK_DELAY } ) ) ) {
        bool redraw = false;

        buttonOk.drawOnState( buttonOk.isEnabled() && le.isMouseLeftButtonPressedInArea( buttonOk.area() ) );
        buttonCancel.drawOnState( le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) );
        buttonVirtualKB.drawOnState( le.isMouseLeftButtonPressedInArea( buttonVirtualKB.area() ) );

        // In this dialog we input text so we need to use hotkeys that cannot be use in text typing.
        if ( ( !isMultiLine && le.isKeyPressed( fheroes2::Key::KEY_ENTER ) ) || ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) ) {
            return !result.empty();
        }

        if ( le.isKeyPressed( fheroes2::Key::KEY_ESCAPE ) || le.MouseClickLeft( buttonCancel.area() ) ) {
            result.clear();
            return false;
        }

        if ( le.MouseClickLeft( buttonVirtualKB.area() ) || ( isInGameKeyboardRequired && le.MouseClickLeft( textInputArea ) ) ) {
            if ( textLanguage.has_value() ) {
                const fheroes2::LanguageSwitcher switcher( textLanguage.value() );
                fheroes2::openVirtualKeyboard( result );
            }
            else {
                fheroes2::openVirtualKeyboard( result );
            }

            if ( charLimit > 0 && result.size() > charLimit ) {
                result.resize( charLimit );
            }
            charInsertPos = result.size();
            redraw = true;
        }
        else if ( le.isAnyKeyPressed() && ( charLimit == 0 || charLimit > result.size() || le.getPressedKeyValue() == fheroes2::Key::KEY_BACKSPACE ) ) {
            // Handle new line input for multi-line texts only.
            if ( isMultiLine && le.getPressedKeyValue() == fheroes2::Key::KEY_ENTER ) {
                result.insert( charInsertPos, 1, '\n' );
                ++charInsertPos;
            }
            else {
                charInsertPos = InsertKeySym( result, charInsertPos, le.getPressedKeyValue(), LocalEvent::getCurrentKeyModifiers() );
            }
            redraw = true;
        }
        else if ( le.MouseClickLeft( textInputArea ) ) {
            charInsertPos = fheroes2::getTextInputCursorPosition( text, charInsertPos, le.getMouseCursorPos(), textInputArea );

            redraw = true;
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to apply the entered text." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonVirtualKB.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Open Virtual Keyboard" ), _( "Click to open the Virtual Keyboard dialog." ), Dialog::ZERO );
        }

        // Text input cursor blink.
        if ( Game::validateAnimationDelay( Game::DelayType::CURSOR_BLINK_DELAY ) ) {
            isCursorVisible = !isCursorVisible;
            redraw = true;
        }

        if ( redraw ) {
            bool redrawOkButton = false;
            if ( result.empty() && buttonOk.isEnabled() ) {
                buttonOk.disable();
                redrawOkButton = true;
            }
            else if ( !result.empty() && !buttonOk.isEnabled() ) {
                buttonOk.enable();
                redrawOkButton = true;
            }

            if ( redrawOkButton ) {
                buttonOk.draw();
                display.updateNextRenderRoi( buttonOk.area() );
            }

            text.set( insertCharToString( result, charInsertPos, isCursorVisible ? '_' : '\x7F' ), fontType, textLanguage );

            if ( !isMultiLine ) {
                text.fitToOneRow( textInputArea.width, false );
            }

            textBackground.restore();
            text.drawInRoi( textInputArea.x, textInputArea.y + 2, textInputArea.width, display, textInputArea );
            display.render( textInputArea );
        }
    }

    return !result.empty();
}

int Dialog::ArmySplitTroop( const int32_t freeSlots, const int32_t redistributeMax, int32_t & redistributeCount, bool & useFastSplit, const std::string & troopName )
{
    assert( redistributeCount >= 0 );
    assert( freeSlots > 0 );

    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const int32_t redistributeMin = std::min( 1, redistributeMax );
    const int spacer = 10;
    const fheroes2::Text header( troopName, fheroes2::FontType::normalYellow() );
    const int32_t headerHeight = header.height() + 6;

    const std::string msg( _( "How many creatures do you wish to move?" ) );
    fheroes2::Text titleText( msg, fheroes2::FontType::normalWhite() );
    titleText.setUniformVerticalAlignment( false );
    const int32_t titleHeight = headerHeight + titleText.rows( fheroes2::boxAreaWidthPx ) * titleText.height();

    fheroes2::Text slotSeparationText( _( "Select how many units to separate into:" ), fheroes2::FontType::normalWhite() );
    slotSeparationText.setUniformVerticalAlignment( false );
    const int32_t bodyHeight = slotSeparationText.rows( fheroes2::boxAreaWidthPx ) * slotSeparationText.height();

    const int defaultYPosition = 160;
    const int boxHeight = freeSlots > 1 ? 63 + spacer + titleHeight + bodyHeight : 45;
    const int boxYPosition = defaultYPosition + ( ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2 ) - boxHeight;

    const NonFixedFrameBox box( boxHeight, boxYPosition, true );

    const fheroes2::Rect & pos = box.GetArea();
    const int center = pos.x + pos.width / 2;
    const int textTopOffset = 13;

    header.draw( pos.x, pos.y + 2, fheroes2::boxAreaWidthPx, display );
    titleText.draw( pos.x, pos.y + 2 + headerHeight, fheroes2::boxAreaWidthPx, display );

    const fheroes2::Size valueSelectionSize{ fheroes2::ValueSelectionDialogElement::getArea() };
    const fheroes2::Rect selectionBoxArea{ pos.x + 70, pos.y + textTopOffset + titleHeight, valueSelectionSize.width, valueSelectionSize.height };

    fheroes2::ValueSelectionDialogElement valueSelectionElement( redistributeMin, redistributeMax, redistributeCount, 1, selectionBoxArea.getPosition() );
    valueSelectionElement.ignoreMouseWheelEventRoiCheck();
    valueSelectionElement.draw( display );

    fheroes2::MovableSprite ssp;
    std::vector<fheroes2::Rect> vrts( freeSlots - 1 );

    if ( freeSlots > 1 ) {
        std::vector<fheroes2::Sprite> sprites( freeSlots - 1 );

        int spriteIconIdx = 21;
        const int deltaX = 10;
        const int deltaXStart = ( freeSlots - 2 ) * -5;

        for ( int32_t i = 0; i < freeSlots - 1; ++i ) {
            sprites[i] = fheroes2::AGG::GetICN( ICN::REQUESTS, spriteIconIdx );
            ++spriteIconIdx;

            const int spriteWidth = sprites[i].width();
            const int offset = spriteWidth * ( 2 * i + 1 - freeSlots ) / 2;
            vrts[i] = { center + offset + deltaXStart + i * deltaX, pos.y + textTopOffset + titleHeight + bodyHeight + 45, spriteWidth, sprites[i].height() };
        }

        slotSeparationText.draw( pos.x, pos.y + textTopOffset + titleHeight + 37, fheroes2::boxAreaWidthPx, display );

        for ( int32_t i = 0; i < freeSlots - 1; ++i ) {
            fheroes2::Blit( sprites[i], display, vrts[i].x, vrts[i].y );
        }

        ssp.resize( sprites[0].width(), sprites[0].height() );
        ssp.reset();

        fheroes2::DrawBorder( ssp, 214 );

        if ( useFastSplit ) {
            ssp.setPosition( vrts[0].x, vrts[0].y );
            ssp.show();
        }
    }

    fheroes2::ButtonGroup btnGroups( box.GetArea(), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    const fheroes2::Point minMaxButtonOffset( pos.x + 165, pos.y + textTopOffset + titleHeight );
    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    fheroes2::Button buttonMax( minMaxButtonOffset.x, minMaxButtonOffset.y, isEvilInterface ? ICN::UNIFORM_EVIL_MAX_BUTTON : ICN::UNIFORM_GOOD_MAX_BUTTON, 0, 1 );
    fheroes2::Button buttonMin( minMaxButtonOffset.x, minMaxButtonOffset.y, isEvilInterface ? ICN::UNIFORM_EVIL_MIN_BUTTON : ICN::UNIFORM_GOOD_MIN_BUTTON, 0, 1 );

    const fheroes2::Rect buttonArea( 5, 0, 61, 25 );
    SwitchMaxMinButtons( buttonMin, buttonMax, redistributeCount, redistributeMin );

    LocalEvent & le = LocalEvent::Get();

    display.render();

    // message loop
    int bres = Dialog::ZERO;
    while ( bres == Dialog::ZERO && le.HandleEvents() ) {
        bool redraw_count = false;

        if ( buttonMax.isVisible() ) {
            le.isMouseLeftButtonPressedInArea( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();
        }

        if ( buttonMin.isVisible() ) {
            le.isMouseLeftButtonPressedInArea( buttonMin.area() ) ? buttonMin.drawOnPress() : buttonMin.drawOnRelease();
        }

        if ( fheroes2::processIntegerValueTyping( redistributeMin, redistributeMax, redistributeCount ) ) {
            valueSelectionElement.setValue( redistributeCount );
            redraw_count = true;
        }
        else if ( buttonMax.isVisible() && le.MouseClickLeft( buttonMax.area() ) ) {
            le.isMouseLeftButtonPressedInArea( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();
            redistributeCount = redistributeMax;
            valueSelectionElement.setValue( redistributeMax );
            redraw_count = true;
        }
        else if ( buttonMin.isVisible() && le.MouseClickLeft( buttonMin.area() ) ) {
            le.isMouseLeftButtonPressedInArea( buttonMin.area() ) ? buttonMin.drawOnPress() : buttonMin.drawOnRelease();
            redistributeCount = redistributeMin;
            valueSelectionElement.setValue( redistributeMin );
            redraw_count = true;
        }
        else if ( valueSelectionElement.processEvents() ) {
            redistributeCount = valueSelectionElement.getValue();
            redraw_count = true;
        }

        if ( !ssp.empty() ) {
            for ( std::vector<fheroes2::Rect>::const_iterator it = vrts.begin(); it != vrts.end(); ++it ) {
                if ( le.MouseClickLeft( *it ) ) {
                    ssp.setPosition( it->x, it->y );
                    ssp.show();
                    display.render();
                }
            }
        }

        if ( redraw_count ) {
            SwitchMaxMinButtons( buttonMin, buttonMax, valueSelectionElement.getValue(), redistributeMin );
            if ( !ssp.empty() ) {
                ssp.hide();
            }
            valueSelectionElement.draw( display );

            if ( buttonMax.isVisible() ) {
                buttonMax.draw();
            }

            if ( buttonMin.isVisible() ) {
                buttonMin.draw();
            }

            display.render();
        }

        bres = btnGroups.processEvents();
    }

    int result = 0;

    if ( bres == Dialog::OK ) {
        redistributeCount = valueSelectionElement.getValue();

        if ( !ssp.isHidden() ) {
            const fheroes2::Rect rt( ssp.x(), ssp.y(), ssp.width(), ssp.height() );

            for ( int32_t i = 0; i < freeSlots - 1; ++i ) {
                if ( rt == vrts[i] ) {
                    result = i + 2;
                    break;
                }
            }

            useFastSplit = true;
        }
        else {
            result = 2;
            useFastSplit = false;
        }
    }

    return result;
}
