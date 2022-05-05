/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2022                                                    *
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

#include "dialog_hotkeys.h"
#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "screen.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    const int editBoxLength = 266;
    const int hotKeyLength = 66;
    const int windowExtensionHeight = 34;

    fheroes2::Sprite generateMainWindow()
    {
        // Since the original window contains two buttons: Cancel and Okay as well as a small frame just above the button we have to rebuild the window.
        const fheroes2::Sprite & originalWindow = fheroes2::AGG::GetICN( ICN::REQBKG, 0 );
        fheroes2::Sprite window = originalWindow;

        // Copy the bottom of a dialog with Okay buttom from another window.
        const fheroes2::Sprite & mapSelectionWindow = fheroes2::AGG::GetICN( ICN::REQSBKG, 0 );
        fheroes2::Copy( mapSelectionWindow, 0, 409, window, 0, 315, window.width(), window.height() - 314 );

        fheroes2::Copy( originalWindow, 41, 203, window, 41, 203 + windowExtensionHeight, 301, 69 );

        return window;
    }

    class HotKeyList : public Interface::ListBox<Game::HotKeyEvent>
    {
    public:
        using Interface::ListBox<Game::HotKeyEvent>::ActionListSingleClick;
        using Interface::ListBox<Game::HotKeyEvent>::ActionListPressRight;
        using Interface::ListBox<Game::HotKeyEvent>::ActionListDoubleClick;

        explicit HotKeyList( const fheroes2::Point & offset )
            : Interface::ListBox<Game::HotKeyEvent>( offset )
        {
            // Do nothing.
        }

        void RedrawItem( const Game::HotKeyEvent & hotKeyEvent, s32 offsetX, s32 offsetY, bool /*current*/ ) override
        {
            fheroes2::Display & display = fheroes2::Display::instance();

            fheroes2::Text name( Game::getHotKeyEventNameByEventId( hotKeyEvent ), fheroes2::FontType::normalWhite() );
            name.fitToOneRow( editBoxLength - hotKeyLength );
            name.draw( offsetX + 10, offsetY, display );

            fheroes2::Text hotkey( Game::getHotKeyNameByEventId( hotKeyEvent ), fheroes2::FontType::normalWhite() );
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

        void ActionListPressRight( Game::HotKeyEvent & /*unused*/ ) override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( Game::HotKeyEvent & /*unused*/ ) override
        {
            // Do nothing.
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

        fheroes2::Button buttonOk( roi.x + 140, roi.y + 315, ICN::REQUEST, 1, 2 );

        HotKeyList resList( roi.getPosition() );

        resList.RedrawBackground( roi.getPosition() );
        resList.SetScrollButtonUp( ICN::REQUESTS, 5, 6, { roi.x + 327, roi.y + 55 } );
        resList.SetScrollButtonDn( ICN::REQUESTS, 7, 8, { roi.x + 327, roi.y + 257 + windowExtensionHeight } );

        const fheroes2::Sprite & originalSilder = fheroes2::AGG::GetICN( ICN::ESCROLL, 3 );
        const fheroes2::Image scrollbarSlider
            = fheroes2::generateScrollbarSlider( originalSilder, false, 180 + windowExtensionHeight, 11, static_cast<int32_t>( events.size() ),
                                                 { 0, 0, originalSilder.width(), 8 }, { 0, 7, originalSilder.width(), 8 } );
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
