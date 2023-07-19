/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2023                                             *
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

#include <cassert>
#include <cstdint>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_hotkeys.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_scrollbar.h"
#include "ui_text.h"

namespace
{
    const int editBoxLength = 266;
    const int hotKeyLength = 68;
    const int windowExtensionHeight = 34;

    fheroes2::Sprite generateMainWindow()
    {
        // Since the original window contains two buttons: Cancel and Okay as well as a small frame just above the button we have to rebuild the window.
        const fheroes2::Sprite & originalWindow = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
        fheroes2::Sprite window = originalWindow;

        // Copy the bottom of a dialog with Okay button from another window.
        const fheroes2::Sprite & mapSelectionWindow = fheroes2::AGG::GetICN( ICN::REQSBKG, 0 );
        fheroes2::Copy( mapSelectionWindow, 0, 409, window, 0, 315, window.width(), window.height() - 314 );

        fheroes2::Copy( originalWindow, 41, 203, window, 41, 203 + windowExtensionHeight, 301, 69 );

        return window;
    }

    class HotKeyElement : public fheroes2::DialogElement
    {
    public:
        HotKeyElement( const fheroes2::Key key, fheroes2::Image & output )
            : _restorer( output, 0, 0, 0, 0 )
            , _key( key )
            , _keyChanged( false )
        {
            // Text always occupies the whole width of the dialog.
            _area = { BOXAREA_WIDTH, fheroes2::Text( StringUpper( KeySymGetName( _key ) ), fheroes2::FontType::normalYellow() ).height( BOXAREA_WIDTH ) };
        }

        ~HotKeyElement() override = default;

        void draw( fheroes2::Image & output, const fheroes2::Point & offset ) const override
        {
            _restorer.restore();

            const fheroes2::Text text( StringUpper( KeySymGetName( _key ) ), fheroes2::FontType::normalYellow() );
            _restorer.update( offset.x, offset.y, BOXAREA_WIDTH, text.height() );

            text.draw( offset.x, offset.y, BOXAREA_WIDTH, output );
        }

        void processEvents( const fheroes2::Point & /*offset*/ ) const override
        {
            const LocalEvent & le = LocalEvent::Get();
            if ( le.KeyPress() ) {
                _key = le.KeyValue();
                _keyChanged = true;
            }
        }

        // Never call this method as a custom image has nothing to popup.
        void showPopup( const int /*buttons*/ ) const override
        {
            assert( 0 );
        }

        bool update( fheroes2::Image & output, const fheroes2::Point & offset ) const override
        {
            if ( _keyChanged ) {
                _keyChanged = false;
                draw( output, offset );
                return true;
            }

            return false;
        }

        void reset()
        {
            _restorer.reset();
        }

        fheroes2::Key getKey() const
        {
            return _key;
        }

    private:
        mutable fheroes2::ImageRestorer _restorer;
        mutable fheroes2::Key _key;
        mutable bool _keyChanged;
    };

    class HotKeyList : public Interface::ListBox<Game::HotKeyEvent>
    {
    public:
        using Interface::ListBox<Game::HotKeyEvent>::ListBox;

        using Interface::ListBox<Game::HotKeyEvent>::ActionListSingleClick;
        using Interface::ListBox<Game::HotKeyEvent>::ActionListPressRight;
        using Interface::ListBox<Game::HotKeyEvent>::ActionListDoubleClick;

        void RedrawItem( const Game::HotKeyEvent & hotKeyEvent, int32_t offsetX, int32_t offsetY, bool current ) override
        {
            fheroes2::Display & display = fheroes2::Display::instance();

            const fheroes2::FontType fontType = current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite();

            fheroes2::Text name( _( Game::getHotKeyEventNameByEventId( hotKeyEvent ) ), fontType );
            name.fitToOneRow( editBoxLength - hotKeyLength );
            name.draw( offsetX + 5, offsetY, display );

            fheroes2::Text hotkey( Game::getHotKeyNameByEventId( hotKeyEvent ), fontType );
            hotkey.fitToOneRow( hotKeyLength );
            hotkey.draw( offsetX + editBoxLength - hotKeyLength, offsetY, hotKeyLength, display );
        }

        void RedrawBackground( const fheroes2::Point & dst ) override
        {
            const fheroes2::Sprite & panel = generateMainWindow();
            fheroes2::Blit( panel, fheroes2::Display::instance(), dst.x, dst.y );
        }

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListSingleClick( Game::HotKeyEvent & /*unused*/ ) override
        {
            // Do nothing.
        }

        void ActionListPressRight( Game::HotKeyEvent & hotKeyEvent ) override
        {
            fheroes2::showMessage( fheroes2::Text{ _( Game::getHotKeyEventNameByEventId( hotKeyEvent ) ), fheroes2::FontType::normalWhite() },
                                   fheroes2::Text{ Game::getHotKeyNameByEventId( hotKeyEvent ), fheroes2::FontType::normalYellow() }, Dialog::ZERO );
        }

        void ActionListDoubleClick( Game::HotKeyEvent & hotKeyEvent ) override
        {
            HotKeyElement hotKeyUI( Game::getHotKeyForEvent( hotKeyEvent ), fheroes2::Display::instance() );

            // Okay and Cancel events are special cases as they are used in dialogs. By default we need to disable these events to allow to be set any key for an event.
            // Global events (that work on all screens) must be disabled as well.
            const fheroes2::Key okayEventKey = Game::getHotKeyForEvent( Game::HotKeyEvent::DEFAULT_OKAY );
            const fheroes2::Key cancelEventKey = Game::getHotKeyForEvent( Game::HotKeyEvent::DEFAULT_CANCEL );
            const fheroes2::Key fullscreenEventKey = Game::getHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_FULLSCREEN );
            const fheroes2::Key textSupportModeEventKey = Game::getHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_TEXT_SUPPORT_MODE );

            Game::setHotKeyForEvent( Game::HotKeyEvent::DEFAULT_OKAY, fheroes2::Key::NONE );
            Game::setHotKeyForEvent( Game::HotKeyEvent::DEFAULT_CANCEL, fheroes2::Key::NONE );
            Game::setHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_FULLSCREEN, fheroes2::Key::NONE );
            Game::setHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_TEXT_SUPPORT_MODE, fheroes2::Key::NONE );

            const int returnValue = fheroes2::showMessage( fheroes2::Text{ _( Game::getHotKeyEventNameByEventId( hotKeyEvent ) ), fheroes2::FontType::normalWhite() },
                                                           fheroes2::Text{ "", fheroes2::FontType::normalWhite() }, Dialog::OK | Dialog::CANCEL, { &hotKeyUI } );

            Game::setHotKeyForEvent( Game::HotKeyEvent::DEFAULT_OKAY, okayEventKey );
            Game::setHotKeyForEvent( Game::HotKeyEvent::DEFAULT_CANCEL, cancelEventKey );
            Game::setHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_FULLSCREEN, fullscreenEventKey );
            Game::setHotKeyForEvent( Game::HotKeyEvent::GLOBAL_TOGGLE_TEXT_SUPPORT_MODE, textSupportModeEventKey );

            // To avoid UI issues we need to reset restorer manually.
            hotKeyUI.reset();

            if ( returnValue == Dialog::CANCEL ) {
                return;
            }

            Game::setHotKeyForEvent( hotKeyEvent, hotKeyUI.getKey() );
            Game::HotKeySave();
        }
    };

    void redrawWindowHeader( const fheroes2::Point & dst )
    {
        fheroes2::Text text( _( "Hot Keys:" ), fheroes2::FontType::normalYellow() );
        text.draw( dst.x + ( 377 - text.width() ) / 2, dst.y + 30, fheroes2::Display::instance() );
    }
}

namespace fheroes2
{
    void openHotkeysDialog()
    {
        std::vector<Game::HotKeyEvent> events = Game::getAllHotKeyEvents();

        fheroes2::Display & display = fheroes2::Display::instance();

        // setup cursor
        const CursorRestorer cursorRestorer( true, ::Cursor::POINTER );

        const fheroes2::Sprite & sprite = generateMainWindow();
        const fheroes2::Sprite & spriteShadow = fheroes2::AGG::GetICN( ICN::REQBKG, 1 );

        const fheroes2::Point dialogOffset( ( display.width() - sprite.width() ) / 2, ( display.height() - sprite.height() ) / 2 );
        const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

        fheroes2::ImageRestorer restorer( display, shadowOffset.x, shadowOffset.y, sprite.width() + BORDERWIDTH, sprite.height() + BORDERWIDTH );
        const fheroes2::Rect roi( dialogOffset.x, dialogOffset.y, sprite.width(), sprite.height() );

        fheroes2::Blit( spriteShadow, display, roi.x - BORDERWIDTH, roi.y + BORDERWIDTH );

        fheroes2::Button buttonOk( roi.x + 140, roi.y + 315, ICN::BUTTON_SMALL_OKAY_GOOD, 0, 1 );

        HotKeyList resList( roi.getPosition() );

        resList.RedrawBackground( roi.getPosition() );
        resList.SetScrollButtonUp( ICN::REQUESTS, 5, 6, { roi.x + 327, roi.y + 55 } );
        resList.SetScrollButtonDn( ICN::REQUESTS, 7, 8, { roi.x + 327, roi.y + 257 + windowExtensionHeight } );

        const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( ICN::ESCROLL, 3 );
        const fheroes2::Image scrollbarSlider
            = fheroes2::generateScrollbarSlider( originalSlider, false, 180 + windowExtensionHeight, 11, static_cast<int32_t>( events.size() ),
                                                 { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );
        resList.setScrollBarArea( { roi.x + 328, roi.y + 73, 12, 180 + windowExtensionHeight } );
        resList.setScrollBarImage( scrollbarSlider );
        resList.SetAreaMaxItems( 13 );
        resList.SetAreaItems( { roi.x + 41, roi.y + 55 + 3, editBoxLength, 215 + windowExtensionHeight } );

        resList.SetListContent( events );

        resList.Redraw();
        buttonOk.draw();
        redrawWindowHeader( roi.getPosition() );
        display.render();

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();

            resList.QueueEventProcessing();

            if ( ( buttonOk.isEnabled() && le.MouseClickLeft( buttonOk.area() ) ) || Game::HotKeyCloseWindow() ) {
                return;
            }
            if ( le.MousePressRight( buttonOk.area() ) ) {
                fheroes2::Text header( _( "Okay" ), fheroes2::FontType::normalYellow() );
                fheroes2::Text body( _( "Exit this menu without doing anything." ), fheroes2::FontType::normalWhite() );
                fheroes2::showMessage( header, body, 0 );
            }

            if ( !resList.IsNeedRedraw() ) {
                continue;
            }

            resList.Redraw();
            buttonOk.draw();
            redrawWindowHeader( roi.getPosition() );
            display.render();
        }
    }
}
