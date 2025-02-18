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

        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::StandardWindow background( 289, 204, true, display );

        fheroes2::Button buttonWorld;
        fheroes2::Button buttonPuzzle;
        fheroes2::Button buttonInfo;
        fheroes2::Button buttonDig;
        fheroes2::Button buttonCancel;

        const int32_t largeButtonsXOffset = 30;
        const int32_t largeButtonsYOffset = 15;

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        const int apanel = isEvilInterface ? ICN::APANELE : ICN::APANEL;

        background.renderButton( buttonWorld, apanel, 0, 1, { largeButtonsXOffset, largeButtonsYOffset }, fheroes2::StandardWindow::Padding::TOP_LEFT );
        background.renderButton( buttonPuzzle, apanel, 2, 3, { largeButtonsXOffset, largeButtonsYOffset }, fheroes2::StandardWindow::Padding::TOP_RIGHT );
        background.renderButton( buttonInfo, isEvilInterface ? ICN::BUTTON_INFO_EVIL : ICN::BUTTON_INFO_GOOD, 0, 1, { largeButtonsXOffset, largeButtonsYOffset + 2 },
                                 fheroes2::StandardWindow::Padding::CENTER_LEFT );
        background.renderButton( buttonDig, apanel, 6, 7, { largeButtonsXOffset, largeButtonsYOffset + 2 }, fheroes2::StandardWindow::Padding::CENTER_RIGHT );
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
            buttonWorld.drawOnState( le.isMouseLeftButtonPressedInArea( buttonWorld.area() ) );
            buttonPuzzle.drawOnState( le.isMouseLeftButtonPressedInArea( buttonPuzzle.area() ) );
            buttonInfo.drawOnState( le.isMouseLeftButtonPressedInArea( buttonInfo.area() ) );
            buttonDig.drawOnState( le.isMouseLeftButtonPressedInArea( buttonDig.area() ) );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) );

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
