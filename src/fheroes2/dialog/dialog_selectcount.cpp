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
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game_delays.h"
#include "game_language.h"
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
    void SwitchMaxMinButtons( fheroes2::ButtonBase & minButton, fheroes2::ButtonBase & maxButton, uint32_t currentValue, uint32_t minimumValue )
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

class SelectValue final
{
public:
    SelectValue( const uint32_t min, const uint32_t max, const uint32_t cur, const uint32_t st )
        : vmin( min )
        , vmax( max )
        , vcur( cur )
        , step( st )
        , timedBtnUp( [this]() { return btnUp.isPressed(); } )
        , timedBtnDn( [this]() { return btnDn.isPressed(); } )
    {
        vmin = std::min( vmin, vmax );

        if ( vcur > vmax || vcur < vmin ) {
            vcur = vmin;
        }

        btnUp.setICNInfo( ICN::TOWNWIND, 5, 6 );
        btnDn.setICNInfo( ICN::TOWNWIND, 7, 8 );

        btnUp.subscribe( &timedBtnUp );
        btnDn.subscribe( &timedBtnDn );

        pos.width = 90;
        pos.height = 30;
    }

    void setValue( const uint32_t v )
    {
        vcur = v;
    }

    void SetPos( const fheroes2::Point & pt )
    {
        pos.x = pt.x;
        pos.y = pt.y;

        btnUp.setPosition( pt.x + 70, pt.y );
        btnDn.setPosition( pt.x + 70, pt.y + 16 );
    }

    uint32_t getValue() const
    {
        return vcur;
    }

    const fheroes2::Rect & getArea() const
    {
        return pos;
    }

    void Redraw() const
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Sprite & sprite_edit = fheroes2::AGG::GetICN( ICN::TOWNWIND, 4 );
        fheroes2::Blit( sprite_edit, display, pos.x, pos.y + 4 );

        const fheroes2::Text text( std::to_string( vcur ), fheroes2::FontType::normalWhite() );
        text.draw( pos.x + ( sprite_edit.width() - text.width() ) / 2, pos.y + 7, display );

        btnUp.draw();
        btnDn.draw();
    }

    bool QueueEventProcessing()
    {
        LocalEvent & le = LocalEvent::Get();

        le.MousePressLeft( btnUp.area() ) ? btnUp.drawOnPress() : btnUp.drawOnRelease();
        le.MousePressLeft( btnDn.area() ) ? btnDn.drawOnPress() : btnDn.drawOnRelease();

        if ( ( le.MouseWheelUp() || le.MouseClickLeft( btnUp.area() ) || timedBtnUp.isDelayPassed() ) && vcur < vmax ) {
            vcur += ( ( vcur + step ) <= vmax ) ? step : ( vmax - vcur );
            return true;
        }

        if ( ( le.MouseWheelDn() || le.MouseClickLeft( btnDn.area() ) || timedBtnDn.isDelayPassed() ) && vmin < vcur ) {
            vcur -= ( ( vmin + vcur ) >= step ) ? step : ( vcur - vmin );
            return true;
        }

        return false;
    }

private:
    uint32_t vmin;
    uint32_t vmax;
    uint32_t vcur;
    uint32_t step;

    fheroes2::Rect pos;

    fheroes2::Button btnUp;
    fheroes2::Button btnDn;

    fheroes2::TimedEventValidator timedBtnUp;
    fheroes2::TimedEventValidator timedBtnDn;
};

bool Dialog::SelectCount( std::string header, const uint32_t min, const uint32_t max, uint32_t & selectedValue, const uint32_t step,
                          const fheroes2::Image & backgroundImage )
{
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::Text headerText( std::move( header ), fheroes2::FontType::normalWhite() );
    int32_t headerOffsetY{ 10 };
    const int32_t selectionAreaHeight{ 30 };
    const int32_t headerHeight = headerText.height( BOXAREA_WIDTH );
    const int32_t imageHeight = backgroundImage.height();

    const FrameBox box( headerHeight + headerOffsetY + selectionAreaHeight + imageHeight, true );

    SelectValue selectionBox( min, max, selectedValue, step );

    const fheroes2::Rect & windowArea = box.GetArea();

    fheroes2::Display & display = fheroes2::Display::instance();
    headerText.draw( windowArea.x, windowArea.y, BOXAREA_WIDTH, display );

    if ( !backgroundImage.empty() ) {
        fheroes2::Blit( backgroundImage, 0, 0, display, windowArea.x + ( windowArea.width - backgroundImage.width() ) / 2, windowArea.y + headerHeight + headerOffsetY,
                        backgroundImage.width(), backgroundImage.height() );

        headerOffsetY = headerOffsetY * 2;
    }

    selectionBox.SetPos( fheroes2::Point( windowArea.x + 80, windowArea.y + headerOffsetY + headerHeight + imageHeight ) );
    selectionBox.Redraw();

    fheroes2::ButtonGroup btnGroups( box.GetArea(), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    const fheroes2::Text mainText( _( "MAX" ), fheroes2::FontType::smallWhite() );
    const int32_t maxAreaOffsetY{ ( 26 - mainText.height() ) / 2 };
    const fheroes2::Rect rectMax{ windowArea.x + 176, windowArea.y + headerOffsetY + headerHeight + imageHeight + maxAreaOffsetY, mainText.width(), mainText.height() };
    mainText.draw( rectMax.x, rectMax.y + 2, display );

    display.render();

    int result = Dialog::ZERO;

    LocalEvent & le = LocalEvent::Get();
    while ( result == Dialog::ZERO && le.HandleEvents() ) {
        bool redraw_count = false;

        if ( fheroes2::PressIntKey( max, selectedValue ) ) {
            selectionBox.setValue( selectedValue );
            redraw_count = true;
        }

        if ( le.MouseClickLeft( rectMax ) ) {
            selectionBox.setValue( max );
            redraw_count = true;
        }

        if ( selectionBox.QueueEventProcessing() ) {
            redraw_count = true;
        }

        if ( redraw_count ) {
            selectionBox.Redraw();
            display.render( selectionBox.getArea() );
        }

        result = btnGroups.processEvents();
    }

    selectedValue = ( result == Dialog::OK ) ? selectionBox.getValue() : 0;

    return result == Dialog::OK;
}

bool Dialog::inputString( std::string header, std::string & result, std::string title, const size_t charLimit, const bool isMultiLine,
                          const bool englishOnlyVirtualKeyboard )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    result.reserve( charLimit == 0 ? 48 : charLimit );
    size_t charInsertPos = result.size();

    const bool hasTitle = !title.empty();

    const fheroes2::Text titlebox( std::move( title ), fheroes2::FontType::normalYellow() );
    const fheroes2::Text textbox( std::move( header ), fheroes2::FontType::normalWhite() );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    const int32_t titleHeight = hasTitle ? titlebox.height( BOXAREA_WIDTH ) + 10 : 0;
    const int32_t keyBoardButtonExtraHeight = 20;

    const fheroes2::Sprite & inputArea = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::BUYBUILD : ICN::BUYBUILE ), 3 );

    const int32_t inputAreaWidth = isMultiLine ? 224 : inputArea.width();
    const int32_t inputAreaHeight = isMultiLine ? 265 : inputArea.height();

    const int32_t textboxHeight = textbox.height( BOXAREA_WIDTH );

    const int32_t frameBoxHeight = 10 + titleHeight + textboxHeight + 10 + inputAreaHeight + keyBoardButtonExtraHeight;
    const FrameBox box( frameBoxHeight, true );
    const fheroes2::Rect & frameBoxArea = box.GetArea();

    // Title text.
    if ( hasTitle ) {
        titlebox.draw( frameBoxArea.x, frameBoxArea.y + 12, BOXAREA_WIDTH, display );
    }

    // Header text.
    textbox.draw( frameBoxArea.x, frameBoxArea.y + 12 + titleHeight, BOXAREA_WIDTH, display );

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
    fheroes2::Text text( insertCharToString( result, charInsertPos, isCursorVisible ? '_' : '\x7F' ), fontType );
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

        buttonOk.drawOnState( buttonOk.isEnabled() && le.MousePressLeft( buttonOk.area() ) );
        buttonCancel.drawOnState( le.MousePressLeft( buttonCancel.area() ) );
        buttonVirtualKB.drawOnState( le.MousePressLeft( buttonVirtualKB.area() ) );

        // In this dialog we input text so we need to use hotkeys that cannot be use in text typing.
        if ( ( !isMultiLine && le.KeyPress( fheroes2::Key::KEY_ENTER ) ) || ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) ) {
            return !result.empty();
        }

        if ( le.KeyPress( fheroes2::Key::KEY_ESCAPE ) || le.MouseClickLeft( buttonCancel.area() ) ) {
            result.clear();
            return false;
        }

        if ( le.MouseClickLeft( buttonVirtualKB.area() ) || ( isInGameKeyboardRequired && le.MouseClickLeft( textInputArea ) ) ) {
            // TODO: remove this logic once the Editor is ready to support other languages.
            if ( englishOnlyVirtualKeyboard ) {
                const fheroes2::LanguageSwitcher switcher( fheroes2::SupportedLanguage::English );
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
        else if ( le.KeyPress() && ( charLimit == 0 || charLimit > result.size() || le.KeyValue() == fheroes2::Key::KEY_BACKSPACE ) ) {
            // Handle new line input for multi-line texts only.
            if ( isMultiLine && le.KeyValue() == fheroes2::Key::KEY_ENTER ) {
                result.insert( charInsertPos, 1, '\n' );
                ++charInsertPos;
            }
            else {
                charInsertPos = InsertKeySym( result, charInsertPos, le.KeyValue(), LocalEvent::getCurrentKeyModifiers() );
            }
            redraw = true;
        }
        else if ( le.MouseClickLeft( textInputArea ) ) {
            charInsertPos = fheroes2::getTextInputCursorPosition( text, charInsertPos, le.GetMouseCursor(), textInputArea );

            redraw = true;
        }
        else if ( le.MousePressRight( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( buttonOk.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to apply the entered text." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( buttonVirtualKB.area() ) ) {
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

            text.set( insertCharToString( result, charInsertPos, isCursorVisible ? '_' : '\x7F' ), fontType );

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

int Dialog::ArmySplitTroop( uint32_t freeSlots, const uint32_t redistributeMax, uint32_t & redistributeCount, bool & useFastSplit, const std::string & troopName )
{
    assert( freeSlots > 0 );

    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const uint32_t min = std::min( 1U, redistributeMax );
    const int spacer = 10;
    const fheroes2::Text header( troopName, fheroes2::FontType::normalYellow() );
    const int32_t headerHeight = header.height() + 6;

    const std::string msg( _( "How many creatures do you wish to move?" ) );
    fheroes2::Text titleText( msg, fheroes2::FontType::normalWhite() );
    titleText.setUniformVerticalAlignment( false );
    const int32_t titleHeight = headerHeight + titleText.rows( BOXAREA_WIDTH ) * titleText.height();

    fheroes2::Text slotSeparationText( _( "Select how many units to separate into:" ), fheroes2::FontType::normalWhite() );
    slotSeparationText.setUniformVerticalAlignment( false );
    const int32_t bodyHeight = slotSeparationText.rows( BOXAREA_WIDTH ) * slotSeparationText.height();

    const int defaultYPosition = 160;
    const int boxHeight = freeSlots > 1 ? 63 + spacer + titleHeight + bodyHeight : 45;
    const int boxYPosition = defaultYPosition + ( ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2 ) - boxHeight;

    const NonFixedFrameBox box( boxHeight, boxYPosition, true );
    SelectValue sel( min, redistributeMax, redistributeCount, 1 );

    const fheroes2::Rect & pos = box.GetArea();
    const int center = pos.x + pos.width / 2;
    const int textTopOffset = 13;

    header.draw( pos.x, pos.y + 2, BOXAREA_WIDTH, display );
    titleText.draw( pos.x, pos.y + 2 + headerHeight, BOXAREA_WIDTH, display );

    sel.SetPos( fheroes2::Point( pos.x + 70, pos.y + textTopOffset + titleHeight ) );
    sel.Redraw();

    fheroes2::MovableSprite ssp;
    std::vector<fheroes2::Rect> vrts( freeSlots - 1 );

    if ( freeSlots > 1 ) {
        std::vector<fheroes2::Sprite> sprites( freeSlots - 1 );

        int spriteIconIdx = 21;
        const int deltaX = 10;
        const int deltaXStart = static_cast<int>( freeSlots - 2 ) * -5;

        for ( uint32_t i = 0; i < freeSlots - 1; ++i ) {
            sprites[i] = fheroes2::AGG::GetICN( ICN::REQUESTS, spriteIconIdx );
            ++spriteIconIdx;

            const int spriteWidth = sprites[i].width();
            const int offset = spriteWidth * ( 2 * static_cast<int>( i ) + 1 - static_cast<int>( freeSlots ) ) / 2;
            vrts[i] = fheroes2::Rect( center + offset + deltaXStart + static_cast<int>( i ) * deltaX, pos.y + textTopOffset + titleHeight + bodyHeight + 45, spriteWidth,
                                      sprites[i].height() );
        }

        slotSeparationText.draw( pos.x, pos.y + textTopOffset + titleHeight + 37, BOXAREA_WIDTH, display );

        for ( uint32_t i = 0; i < freeSlots - 1; ++i ) {
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
    SwitchMaxMinButtons( buttonMin, buttonMax, redistributeCount, min );

    LocalEvent & le = LocalEvent::Get();

    display.render();

    // message loop
    int bres = Dialog::ZERO;
    while ( bres == Dialog::ZERO && le.HandleEvents() ) {
        bool redraw_count = false;

        if ( buttonMax.isVisible() ) {
            le.MousePressLeft( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();
        }

        if ( buttonMin.isVisible() ) {
            le.MousePressLeft( buttonMin.area() ) ? buttonMin.drawOnPress() : buttonMin.drawOnRelease();
        }

        if ( fheroes2::PressIntKey( redistributeMax, redistributeCount ) ) {
            sel.setValue( redistributeCount );
            redraw_count = true;
        }
        else if ( buttonMax.isVisible() && le.MouseClickLeft( buttonMax.area() ) ) {
            le.MousePressLeft( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();
            redistributeCount = redistributeMax;
            sel.setValue( redistributeMax );
            redraw_count = true;
        }
        else if ( buttonMin.isVisible() && le.MouseClickLeft( buttonMin.area() ) ) {
            le.MousePressLeft( buttonMin.area() ) ? buttonMin.drawOnPress() : buttonMin.drawOnRelease();
            redistributeCount = min;
            sel.setValue( min );
            redraw_count = true;
        }
        else if ( sel.QueueEventProcessing() ) {
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
            SwitchMaxMinButtons( buttonMin, buttonMax, sel.getValue(), min );
            if ( !ssp.empty() ) {
                ssp.hide();
            }
            sel.Redraw();

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
        redistributeCount = sel.getValue();

        if ( !ssp.isHidden() ) {
            const fheroes2::Rect rt( ssp.x(), ssp.y(), ssp.width(), ssp.height() );

            for ( uint32_t i = 0; i < freeSlots - 1; ++i ) {
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
