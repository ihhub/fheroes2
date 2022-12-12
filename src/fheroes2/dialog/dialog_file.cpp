/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dir.h"
#include "game.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_mode.h"
#include "gamedefs.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

fheroes2::GameMode Dialog::FileOptions()
{
    // preload
    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int cpanbkg = isEvilInterface ? ICN::CPANBKGE : ICN::CPANBKG;

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    // Image sprite.
    const fheroes2::Sprite & background = fheroes2::AGG::GetICN( cpanbkg, 0 );

    fheroes2::Display & display = fheroes2::Display::instance();

    // Since the original image contains shadow it is important to remove it from calculation of window's position.
    const fheroes2::Point rb( ( display.width() - background.width() - BORDERWIDTH ) / 2, ( display.height() - background.height() + BORDERWIDTH ) / 2 );
    fheroes2::ImageRestorer back( display, rb.x, rb.y, background.width(), background.height() );
    fheroes2::Blit( background, display, rb.x, rb.y );

    LocalEvent & le = LocalEvent::Get();

    fheroes2::Button buttonNew( rb.x + 62, rb.y + 31, isEvilInterface ? ICN::BUTTON_NEW_GAME_EVIL : ICN::BUTTON_NEW_GAME_GOOD, 0, 1 );
    fheroes2::Button buttonLoad( rb.x + 195, rb.y + 31, isEvilInterface ? ICN::BUTTON_LOAD_GAME_EVIL : ICN::BUTTON_LOAD_GAME_GOOD, 0, 1 );
    fheroes2::Button buttonSave( rb.x + 62, rb.y + 107, isEvilInterface ? ICN::BUTTON_SAVE_GAME_EVIL : ICN::BUTTON_SAVE_GAME_GOOD, 0, 1 );
    fheroes2::Button buttonQuit( rb.x + 195, rb.y + 107, isEvilInterface ? ICN::BUTTON_QUIT_EVIL : ICN::BUTTON_QUIT_GOOD, 0, 1 );
    fheroes2::Button buttonCancel( rb.x + 128, rb.y + 184, isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1 );

    buttonNew.draw();
    buttonLoad.draw();
    buttonSave.draw();
    buttonQuit.draw();
    buttonCancel.draw();

    display.render();

    fheroes2::GameMode result = fheroes2::GameMode::QUIT_GAME;

    // dialog menu loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonNew.area() ) ? buttonNew.drawOnPress() : buttonNew.drawOnRelease();
        le.MousePressLeft( buttonLoad.area() ) ? buttonLoad.drawOnPress() : buttonLoad.drawOnRelease();
        le.MousePressLeft( buttonSave.area() ) ? buttonSave.drawOnPress() : buttonSave.drawOnRelease();
        le.MousePressLeft( buttonQuit.area() ) ? buttonQuit.drawOnPress() : buttonQuit.drawOnRelease();
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        if ( le.MouseClickLeft( buttonNew.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_NEW_GAME ) ) {
            if ( Interface::Basic::Get().EventNewGame() == fheroes2::GameMode::NEW_GAME ) {
                result = fheroes2::GameMode::NEW_GAME;
                break;
            }
        }
        else if ( le.MouseClickLeft( buttonLoad.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME ) ) {
            if ( ListFiles::IsEmpty( Game::GetSaveDir(), Game::GetSaveFileExtension(), false ) ) {
                fheroes2::showMessage( fheroes2::Text( _( "Load Game" ), fheroes2::FontType::normalYellow() ),
                                       fheroes2::Text( _( "No save files to load." ), fheroes2::FontType::normalWhite() ), Dialog::OK );
            }
            else {
                result = Interface::Basic::Get().EventLoadGame();
                break;
            }
        }
        else if ( le.MouseClickLeft( buttonSave.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_SAVE_GAME ) ) {
            // Special case: since we show a window about file saving we don't want to display the current dialog anymore.
            back.restore();

            return Interface::Basic::Get().EventSaveGame();
        }
        else if ( le.MouseClickLeft( buttonQuit.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_QUIT ) ) {
            if ( Interface::Basic::EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                result = fheroes2::GameMode::QUIT_GAME;
                break;
            }
        }
        else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyCloseWindow() ) {
            result = fheroes2::GameMode::CANCEL;
            break;
        }
        else if ( le.MousePressRight( buttonNew.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "New Game" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Start a single or multi-player game." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( buttonLoad.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Load Game" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Load a previously saved game." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( buttonSave.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Save Game" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Save the current game." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( buttonQuit.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Quit" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Quit out of Heroes of Might and Magic II." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( buttonCancel.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Cancel" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Exit this menu without doing anything." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
    }

    // restore background
    back.restore();
    display.render();

    return result;
}
