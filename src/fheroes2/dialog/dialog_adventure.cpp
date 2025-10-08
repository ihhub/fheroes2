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
    int selectAdventureOption( const bool enableDig )
    {
        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        const int imageIcn = isEvilInterface ? ICN::BUTTONS_ADVENTURE_OPTIONS_DIALOG_EVIL : ICN::BUTTONS_ADVENTURE_OPTIONS_DIALOG_GOOD;
        fheroes2::ButtonGroup optionButtons( imageIcn );

        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::StandardWindow background( optionButtons, false, 0, display );
        background.renderSymmetricButtons( optionButtons, 0, false );

        fheroes2::ButtonBase & buttonWorld = optionButtons.button( 0 );
        fheroes2::ButtonBase & buttonPuzzle = optionButtons.button( 1 );
        fheroes2::ButtonBase & buttonInfo = optionButtons.button( 2 );
        fheroes2::ButtonBase & buttonDig = optionButtons.button( 3 );

        fheroes2::Button buttonCancel;
        background.renderButton( buttonCancel, isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1, { 0, 11 },
                                 fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

        if ( !enableDig ) {
            buttonDig.disable();
            buttonDig.draw();
        }

        display.render( background.totalArea() );

        int result = Dialog::ZERO;

        LocalEvent & le = LocalEvent::Get();

        // dialog menu loop
        while ( le.HandleEvents() ) {
            buttonWorld.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonWorld.area() ) );
            buttonPuzzle.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonPuzzle.area() ) );
            buttonInfo.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonInfo.area() ) );
            buttonDig.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonDig.area() ) );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

            if ( le.MouseClickLeft( buttonWorld.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_VIEW_WORLD ) ) {
                result = Dialog::WORLD;
                break;
            }
            if ( le.MouseClickLeft( buttonPuzzle.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_PUZZLE_MAP ) ) {
                result = Dialog::PUZZLE;
                break;
            }
            if ( le.MouseClickLeft( buttonInfo.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCENARIO_INFORMATION ) ) {
                result = Dialog::INFO;
                break;
            }
            if ( enableDig && ( le.MouseClickLeft( buttonDig.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_DIG_ARTIFACT ) ) ) {
                result = Dialog::DIG;
                break;
            }
            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyCloseWindow() ) {
                result = Dialog::CANCEL;
                break;
            }

            // right info
            if ( le.isMouseRightButtonPressedInArea( buttonWorld.area() ) )
                fheroes2::showStandardTextMessage( _( "View World" ), _( "View the entire world." ), Dialog::ZERO );
            else if ( le.isMouseRightButtonPressedInArea( buttonPuzzle.area() ) )
                fheroes2::showStandardTextMessage( _( "Puzzle" ), _( "View the obelisk puzzle." ), Dialog::ZERO );
            else if ( le.isMouseRightButtonPressedInArea( buttonInfo.area() ) )
                fheroes2::showStandardTextMessage( _( "Scenario Information" ), _( "View information on the scenario you are currently playing." ), Dialog::ZERO );
            else if ( le.isMouseRightButtonPressedInArea( buttonDig.area() ) )
                fheroes2::showStandardTextMessage( _( "Digging" ), _( "Dig for the Ultimate Artifact." ), Dialog::ZERO );
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) )
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
        }

        return result;
    }
}

int Dialog::AdventureOptions( const bool enableDig )
{
    const int result = selectAdventureOption( enableDig );
    return result;
}
