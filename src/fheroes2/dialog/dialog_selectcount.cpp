/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_keyboard.h"
#include "ui_text.h"
#include "ui_tool.h"

namespace
{
    const int32_t textOffset = 24;

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

class SelectValue : public fheroes2::Rect
{
public:
    SelectValue( uint32_t min, uint32_t max, uint32_t cur, uint32_t st )
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

    void SetCur( uint32_t v )
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

    uint32_t getCur() const
    {
        return vcur;
    }

    void Redraw() const
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Sprite & sprite_edit = fheroes2::AGG::GetICN( ICN::TOWNWIND, 4 );
        fheroes2::Blit( sprite_edit, display, pos.x, pos.y + 4 );

        const Text text( std::to_string( vcur ), Font::BIG );
        text.Blit( pos.x + ( sprite_edit.width() - text.w() ) / 2, pos.y + 5 );

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

protected:
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

bool Dialog::SelectCount( const std::string & header, uint32_t min, uint32_t max, uint32_t & cur, int step )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    Text text( header, Font::BIG );
    const int spacer = 10;

    const FrameBox box( text.h() + spacer + 30, true );
    SelectValue sel( min, max, cur, step );

    const fheroes2::Rect & pos = box.GetArea();

    text.Blit( pos.x + ( pos.width - text.w() ) / 2, pos.y );

    sel.SetPos( fheroes2::Point( pos.x + 80, pos.y + 30 ) );
    sel.Redraw();

    fheroes2::ButtonGroup btnGroups( box.GetArea(), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    text.Set( _( "MAX" ), Font::SMALL );
    const fheroes2::Rect rectMax( pos.x + 173, pos.y + 38, text.w(), text.h() );
    text.Blit( rectMax.x, rectMax.y );

    LocalEvent & le = LocalEvent::Get();

    display.render();

    // message loop
    int result = Dialog::ZERO;
    while ( result == Dialog::ZERO && le.HandleEvents() ) {
        bool redraw_count = false;

        if ( fheroes2::PressIntKey( max, cur ) ) {
            sel.SetCur( cur );
            redraw_count = true;
        }

        // max
        if ( le.MouseClickLeft( rectMax ) ) {
            sel.SetCur( max );
            redraw_count = true;
        }

        if ( sel.QueueEventProcessing() ) {
            redraw_count = true;
        }

        if ( redraw_count ) {
            sel.Redraw();
            display.render();
        }

        result = btnGroups.processEvents();
    }

    cur = result == Dialog::OK ? sel.getCur() : 0;

    return result == Dialog::OK;
}

bool Dialog::InputString( const std::string & header, std::string & res, const std::string & title, const size_t charLimit )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    res.clear();
    res.reserve( 48 );
    size_t charInsertPos = 0;

    TextBox titlebox( title, Font::YELLOW_BIG, BOXAREA_WIDTH );
    TextBox textbox( header, Font::BIG, BOXAREA_WIDTH );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    const fheroes2::Sprite & inputArea = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::BUYBUILD : ICN::BUYBUILE ), 3 );

    const int32_t titleHeight = title.empty() ? 0 : titlebox.h() + 10;
    const FrameBox box( 10 + titleHeight + textbox.h() + 10 + inputArea.height(), true );
    const fheroes2::Rect & box_rt = box.GetArea();

    if ( !title.empty() ) {
        titlebox.Blit( box_rt.x + ( box_rt.width - textbox.w() ) / 2, box_rt.y + 10 );
    }

    // text
    fheroes2::Point dst_pt;
    dst_pt.x = box_rt.x + ( box_rt.width - textbox.w() ) / 2;
    dst_pt.y = box_rt.y + 10 + titleHeight;
    textbox.Blit( dst_pt.x, dst_pt.y );

    dst_pt.y = box_rt.y + 10 + titleHeight + textbox.h() + 10;
    dst_pt.x = box_rt.x + ( box_rt.width - inputArea.width() ) / 2;
    fheroes2::Blit( inputArea, display, dst_pt.x, dst_pt.y );
    const fheroes2::Rect text_rt( dst_pt.x, dst_pt.y, inputArea.width(), inputArea.height() );

    fheroes2::Text text( "_", fheroes2::FontType::normalWhite() );
    fheroes2::Blit( inputArea, display, text_rt.x, text_rt.y );
    text.draw( text_rt.x + ( text_rt.width - text.width() ) / 2, text_rt.y + 3, display );

    const int okayButtonICNID = isEvilInterface ? ICN::UNIFORM_EVIL_OKAY_BUTTON : ICN::UNIFORM_GOOD_OKAY_BUTTON;

    dst_pt.x = box_rt.x;
    dst_pt.y = box_rt.y + box_rt.height - fheroes2::AGG::GetICN( okayButtonICNID, 0 ).height();
    fheroes2::Button buttonOk( dst_pt.x, dst_pt.y, okayButtonICNID, 0, 1 );

    const int cancelButtonIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_CANCEL_BUTTON : ICN::UNIFORM_GOOD_CANCEL_BUTTON;

    dst_pt.x = box_rt.x + box_rt.width - fheroes2::AGG::GetICN( cancelButtonIcnID, 0 ).width();
    dst_pt.y = box_rt.y + box_rt.height - fheroes2::AGG::GetICN( cancelButtonIcnID, 0 ).height();
    fheroes2::Button buttonCancel( dst_pt.x, dst_pt.y, cancelButtonIcnID, 0, 1 );

    // Generate a button to open the Virtual Keyboard window.
    fheroes2::Sprite releasedVirtualKB;
    fheroes2::Sprite pressedVirtualKB;
    const int32_t buttonVirtualKBWidth = 15;

    makeButtonSprites( releasedVirtualKB, pressedVirtualKB, "...", buttonVirtualKBWidth, isEvilInterface, true );
    // To center the button horizontally we have to take into account that actual button sprite is 10 pixels longer then the requested button width.
    fheroes2::ButtonSprite buttonVirtualKB
        = makeButtonWithBackground( box_rt.x + ( box_rt.width - buttonVirtualKBWidth - 10 ) / 2, dst_pt.y, releasedVirtualKB, pressedVirtualKB, display );

    if ( res.empty() ) {
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

    bool isCursorVisible = true;

    Game::AnimateResetDelay( Game::DelayType::CURSOR_BLINK_DELAY );

    const bool isInGameKeyboardRequired = System::isVirtualKeyboardSupported();

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::DelayType::CURSOR_BLINK_DELAY } ) ) ) {
        bool redraw = false;

        buttonOk.isEnabled() && le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();
        le.MousePressLeft( buttonVirtualKB.area() ) ? buttonVirtualKB.drawOnPress() : buttonVirtualKB.drawOnRelease();

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) ) {
            break;
        }

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancel.area() ) ) {
            res.clear();
            break;
        }

        if ( le.MouseClickLeft( buttonVirtualKB.area() ) || ( isInGameKeyboardRequired && le.MouseClickLeft( text_rt ) ) ) {
            fheroes2::openVirtualKeyboard( res );
            if ( charLimit > 0 && res.size() > charLimit ) {
                res.resize( charLimit );
            }
            charInsertPos = res.size();
            redraw = true;
        }
        else if ( le.KeyPress() ) {
            if ( charLimit == 0 || charLimit > res.size() || le.KeyValue() == fheroes2::Key::KEY_BACKSPACE ) {
                charInsertPos = InsertKeySym( res, charInsertPos, le.KeyValue(), LocalEvent::getCurrentKeyModifiers() );
                redraw = true;
            }
        }
        else if ( le.MouseClickLeft( text_rt ) ) {
            charInsertPos = fheroes2::getTextInputCursorPosition( res, fheroes2::FontType::normalWhite(), charInsertPos, le.GetMouseCursor().x,
                                                                  text_rt.x + ( text_rt.width - text.width() ) / 2 );

            redraw = true;
        }

        if ( le.MousePressRight( buttonCancel.area() ) ) {
            Dialog::Message( _( "Cancel" ), _( "Exit this menu without doing anything." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonOk.area() ) ) {
            Dialog::Message( _( "Okay" ), _( "Click to apply the entered text." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonVirtualKB.area() ) ) {
            Dialog::Message( _( "Open Virtual Keyboard" ), _( "Click to open the Virtual Keyboard dialog." ), Font::BIG );
        }

        // Text input cursor blink.
        if ( Game::validateAnimationDelay( Game::DelayType::CURSOR_BLINK_DELAY ) ) {
            isCursorVisible = !isCursorVisible;
            redraw = true;
        }

        if ( redraw ) {
            if ( res.empty() && buttonOk.isEnabled() ) {
                buttonOk.disable();
                buttonOk.draw();
            }

            if ( !res.empty() && !buttonOk.isEnabled() ) {
                buttonOk.enable();
                buttonOk.draw();
            }

            text.set( insertCharToString( res, charInsertPos, isCursorVisible ? '_' : '\x7F' ), fheroes2::FontType::normalWhite() );
            text.fitToOneRow( inputArea.width() - textOffset );

            fheroes2::Blit( inputArea, display, text_rt.x, text_rt.y );
            text.draw( text_rt.x + ( text_rt.width - text.width() ) / 2, text_rt.y + 3, display );
            display.render();
        }
    }

    return !res.empty();
}

int Dialog::ArmySplitTroop( uint32_t freeSlots, const uint32_t redistributeMax, uint32_t & redistributeCount, bool & useFastSplit )
{
    assert( freeSlots > 0 );

    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const uint32_t min = std::min( 1U, redistributeMax );
    const int spacer = 10;

    const int defaultYPosition = 160;
    const int boxHeight = freeSlots > 1 ? 90 + spacer : 45;
    const int boxYPosition = defaultYPosition + ( ( display.height() - display.DEFAULT_HEIGHT ) / 2 ) - boxHeight;

    const NonFixedFrameBox box( boxHeight, boxYPosition, true );
    SelectValue sel( min, redistributeMax, redistributeCount, 1 );
    Text text;

    const fheroes2::Rect & pos = box.GetArea();
    const int center = pos.x + pos.width / 2;

    text.Set( _( "How many troops to move?" ), Font::BIG );
    text.Blit( center - text.w() / 2, pos.y );

    sel.SetPos( fheroes2::Point( pos.x + 70, pos.y + 30 ) );
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
            vrts[i] = fheroes2::Rect( center + offset + deltaXStart + i * deltaX, pos.y + 95, spriteWidth, sprites[i].height() );
        }

        text.Set( _( "Fast separation into slots:" ), Font::BIG );
        text.Blit( center - text.w() / 2, pos.y + 65 );

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

    const fheroes2::Point minMaxButtonOffset( pos.x + 165, pos.y + 30 );
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
            sel.SetCur( redistributeCount );
            redraw_count = true;
        }
        else if ( buttonMax.isVisible() && le.MouseClickLeft( buttonMax.area() ) ) {
            le.MousePressLeft( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();
            redistributeCount = redistributeMax;
            sel.SetCur( redistributeMax );
            redraw_count = true;
        }
        else if ( buttonMin.isVisible() && le.MouseClickLeft( buttonMin.area() ) ) {
            le.MousePressLeft( buttonMin.area() ) ? buttonMin.drawOnPress() : buttonMin.drawOnRelease();
            redistributeCount = min;
            sel.SetCur( min );
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
            SwitchMaxMinButtons( buttonMin, buttonMax, sel.getCur(), min );
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
        redistributeCount = sel.getCur();

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
