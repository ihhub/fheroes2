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

#include <cstdint>

#include "cursor.h"
#include "dialog.h" // IWYU pragma: associated
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_mode.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_window.h"

namespace
{
    fheroes2::GameMode selectFileOption()
    {
        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        const int32_t totalDialogWidth = 337;
        const int32_t totalDialogHeight = 252;

        // Prepare restorer of the adventure map when save feedback dialog is shown.
        fheroes2::ImageRestorer back( display, ( display.width() - totalDialogWidth - fheroes2::borderWidthPx ) / 2,
                                      ( display.height() - totalDialogHeight + fheroes2::borderWidthPx ) / 2, totalDialogWidth, totalDialogHeight );

        fheroes2::StandardWindow background( 289, 204, true, display );

        fheroes2::Button buttonNew;
        fheroes2::Button buttonLoad;
        fheroes2::Button buttonSave;
        fheroes2::Button buttonQuit;
        fheroes2::Button buttonCancel;

        const int32_t largeButtonsXOffset = 30;
        const int32_t largeButtonsYOffset = 15;

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        background.renderButton( buttonNew, isEvilInterface ? ICN::BUTTON_NEW_GAME_EVIL : ICN::BUTTON_NEW_GAME_GOOD, 0, 1, { largeButtonsXOffset, largeButtonsYOffset },
                                 fheroes2::StandardWindow::Padding::TOP_LEFT );
        background.renderButton( buttonLoad, isEvilInterface ? ICN::BUTTON_LOAD_GAME_EVIL : ICN::BUTTON_LOAD_GAME_GOOD, 0, 1,
                                 { largeButtonsXOffset, largeButtonsYOffset }, fheroes2::StandardWindow::Padding::TOP_RIGHT );
        background.renderButton( buttonSave, isEvilInterface ? ICN::BUTTON_SAVE_GAME_EVIL : ICN::BUTTON_SAVE_GAME_GOOD, 0, 1,
                                 { largeButtonsXOffset, largeButtonsYOffset + 2 }, fheroes2::StandardWindow::Padding::CENTER_LEFT );
        background.renderButton( buttonQuit, isEvilInterface ? ICN::BUTTON_QUIT_EVIL : ICN::BUTTON_QUIT_GOOD, 0, 1, { largeButtonsXOffset, largeButtonsYOffset + 2 },
                                 fheroes2::StandardWindow::Padding::CENTER_RIGHT );
        background.renderButton( buttonCancel, isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1, { 0, 11 },
                                 fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

        const fheroes2::Rect buttonNewArea = buttonNew.area();
        const fheroes2::Rect buttonLoadArea = buttonLoad.area();
        const fheroes2::Rect buttonSaveArea = buttonSave.area();
        const fheroes2::Rect buttonQuitArea = buttonQuit.area();
        const fheroes2::Rect buttonCancelArea = buttonCancel.area();

        display.render( background.totalArea() );

        fheroes2::GameMode result = fheroes2::GameMode::QUIT_GAME;

        LocalEvent & le = LocalEvent::Get();

        // dialog menu loop
        while ( le.HandleEvents() ) {
            buttonNew.drawOnState( le.isMouseLeftButtonPressedInArea( buttonNewArea ) );
            buttonLoad.drawOnState( le.isMouseLeftButtonPressedInArea( buttonLoadArea ) );
            buttonSave.drawOnState( le.isMouseLeftButtonPressedInArea( buttonSaveArea ) );
            buttonQuit.drawOnState( le.isMouseLeftButtonPressedInArea( buttonQuitArea ) );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedInArea( buttonCancelArea ) );

            if ( le.MouseClickLeft( buttonNewArea ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_NEW_GAME ) ) {
                if ( Interface::AdventureMap::Get().EventNewGame() == fheroes2::GameMode::NEW_GAME ) {
                    result = fheroes2::GameMode::NEW_GAME;
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonLoadArea ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME ) ) {
                if ( Interface::AdventureMap::Get().EventLoadGame() == fheroes2::GameMode::LOAD_GAME ) {
                    result = fheroes2::GameMode::LOAD_GAME;
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonSaveArea ) || Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_SAVE_GAME ) ) {
                // Special case: since we show a window about file saving we don't want to display the current dialog anymore.
                back.restore();

                return Interface::AdventureMap::Get().EventSaveGame();
            }

            if ( le.MouseClickLeft( buttonQuitArea ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_QUIT ) ) {
                if ( Interface::AdventureMap::EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                    result = fheroes2::GameMode::QUIT_GAME;
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonCancelArea ) || Game::HotKeyCloseWindow() ) {
                result = fheroes2::GameMode::CANCEL;
                break;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonNewArea ) ) {
                fheroes2::showStandardTextMessage( _( "New Game" ), _( "Start a single or multi-player game." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonLoadArea ) ) {
                fheroes2::showStandardTextMessage( _( "Load Game" ), _( "Load a previously saved game." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonSaveArea ) ) {
                fheroes2::showStandardTextMessage( _( "Save Game" ), _( "Save the current game." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonQuitArea ) ) {
                fheroes2::showStandardTextMessage( _( "Quit" ), _( "Quit out of Heroes of Might and Magic II." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancelArea ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
        }

        return result;
    }
}

fheroes2::GameMode Dialog::FileOptions()
{
    const fheroes2::GameMode result = selectFileOption();
    return result;
}
