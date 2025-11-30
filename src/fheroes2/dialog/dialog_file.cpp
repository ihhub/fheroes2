/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#include "cursor.h"
#include "dialog.h" // IWYU pragma: associated
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_io.h"
#include "game_mode.h"
#include "icn.h"
#include "localevent.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_window.h"

namespace
{
    fheroes2::GameMode selectFileOption()
    {
        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        const auto & config = Settings::Get();
        const bool isEvilInterface = config.isEvilInterfaceEnabled();
        const int bigButtonsICN = isEvilInterface ? ICN::BUTTONS_FILE_DIALOG_EVIL : ICN::BUTTONS_FILE_DIALOG_GOOD;
        fheroes2::ButtonGroup optionButtons( bigButtonsICN );
        fheroes2::StandardWindow background( optionButtons, false, 0, display );

        const fheroes2::ButtonBase & newGameButton = optionButtons.button( 0 );
        const fheroes2::ButtonBase & loadGameButton = optionButtons.button( 1 );
        fheroes2::ButtonBase & restartGameButton = optionButtons.button( 2 );
        const fheroes2::ButtonBase & saveGameButton = optionButtons.button( 3 );
        const fheroes2::ButtonBase & quickSaveButton = optionButtons.button( 4 );
        const fheroes2::ButtonBase & quitButton = optionButtons.button( 5 );

        // For now this button is disabled.
        restartGameButton.disable();

        background.renderSymmetricButtons( optionButtons, 0, false );

        fheroes2::Button buttonCancel;

        background.renderButton( buttonCancel, isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1, { 0, 11 },
                                 fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

        display.render( background.totalArea() );

        fheroes2::GameMode result = fheroes2::GameMode::QUIT_GAME;

        LocalEvent & le = LocalEvent::Get();

        // dialog menu loop
        while ( le.HandleEvents() ) {
            optionButtons.drawOnState( le );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

            if ( le.MouseClickLeft( newGameButton.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_NEW_GAME ) ) {
                if ( Interface::AdventureMap::Get().EventNewGame() == fheroes2::GameMode::NEW_GAME ) {
                    result = fheroes2::GameMode::NEW_GAME;
                    break;
                }
            }
            else if ( le.MouseClickLeft( loadGameButton.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME ) ) {
                if ( Interface::AdventureMap::Get().EventLoadGame() == fheroes2::GameMode::LOAD_GAME ) {
                    result = fheroes2::GameMode::LOAD_GAME;
                    break;
                }
            }
            else if ( restartGameButton.isEnabled() && le.MouseClickLeft( restartGameButton.area() ) ) {
                // TODO: restart the campaign here.
                fheroes2::showStandardTextMessage( _( "Restart Game" ), "This option is under construction.", Dialog::OK );
                result = fheroes2::GameMode::CANCEL;
                break;
            }
            else if ( le.MouseClickLeft( saveGameButton.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_SAVE_GAME ) ) {
                // Special case: since we show a window about file saving we don't want to display the current dialog anymore.
                background.hideWindow();

                return Interface::AdventureMap::Get().EventSaveGame();
            }
            else if ( le.MouseClickLeft( quickSaveButton.area() ) ) {
                if ( !Game::QuickSave() ) {
                    fheroes2::showStandardTextMessage( "", _( "There was an issue during saving." ), Dialog::OK );
                }

                result = fheroes2::GameMode::CANCEL;
                break;
            }

            if ( le.MouseClickLeft( quitButton.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_QUIT ) ) {
                if ( Interface::AdventureMap::EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                    result = fheroes2::GameMode::QUIT_GAME;
                    break;
                }
            }
            else if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyCloseWindow() ) {
                result = fheroes2::GameMode::CANCEL;
                break;
            }

            if ( le.isMouseRightButtonPressedInArea( newGameButton.area() ) ) {
                fheroes2::showStandardTextMessage( _( "New Game" ), _( "Start a single or multi-player game." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( loadGameButton.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Load Game" ), _( "Load a previously saved game." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( restartGameButton.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Restart Game" ), _( "Restart the scenario." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( saveGameButton.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Save Game" ), _( "Save the current game." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( quickSaveButton.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Quick Save" ), _( "Save the current game without name selection." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( quitButton.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Quit" ), _( "Quit out of Heroes of Might and Magic II." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
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
