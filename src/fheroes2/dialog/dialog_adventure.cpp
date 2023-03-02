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

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
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

int Dialog::AdventureOptions( bool enabledig )
{
    // preload
    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int apanbkg = isEvilInterface ? ICN::APANBKGE : ICN::APANBKG;
    const int apanel = isEvilInterface ? ICN::APANELE : ICN::APANEL;

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( apanbkg, 0 );

    // The sprite contains shadow area at left and bottom side so to center it we have to subtract it
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Point rb( ( display.width() - box.width() - BORDERWIDTH ) / 2, ( display.height() - box.height() + BORDERWIDTH ) / 2 );
    fheroes2::ImageRestorer back( display, rb.x, rb.y, box.width(), box.height() );
    fheroes2::Blit( box, display, rb.x, rb.y );

    LocalEvent & le = LocalEvent::Get();

    fheroes2::Button buttonWorld( rb.x + 62, rb.y + 30, apanel, 0, 1 );
    fheroes2::Button buttonPuzzle( rb.x + 195, rb.y + 30, apanel, 2, 3 );
    fheroes2::Button buttonInfo( rb.x + 62, rb.y + 107, isEvilInterface ? ICN::BUTTON_INFO_EVIL : ICN::BUTTON_INFO_GOOD, 0, 1 );
    fheroes2::Button buttonDig( rb.x + 195, rb.y + 107, apanel, 6, 7 );
    fheroes2::Button buttonCancel( rb.x + 128, rb.y + 184, isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1 );

    if ( !enabledig )
        buttonDig.disable();

    buttonWorld.draw();
    buttonPuzzle.draw();
    buttonInfo.draw();
    buttonDig.draw();
    buttonCancel.draw();

    display.render();

    int result = Dialog::ZERO;

    // dialog menu loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonWorld.area() ) ? buttonWorld.drawOnPress() : buttonWorld.drawOnRelease();
        le.MousePressLeft( buttonPuzzle.area() ) ? buttonPuzzle.drawOnPress() : buttonPuzzle.drawOnRelease();
        le.MousePressLeft( buttonInfo.area() ) ? buttonInfo.drawOnPress() : buttonInfo.drawOnRelease();
        le.MousePressLeft( buttonDig.area() ) ? buttonDig.drawOnPress() : buttonDig.drawOnRelease();
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

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
        if ( ( le.MouseClickLeft( buttonDig.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_DIG_ARTIFACT ) ) && buttonDig.isEnabled() ) {
            result = Dialog::DIG;
            break;
        }
        if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyCloseWindow() ) {
            result = Dialog::CANCEL;
            break;
        }

        // right info
        if ( le.MousePressRight( buttonWorld.area() ) )
            fheroes2::showStandardTextMessage( _( "View World" ), _( "View the entire world." ), Dialog::ZERO );
        if ( le.MousePressRight( buttonPuzzle.area() ) )
            fheroes2::showStandardTextMessage( _( "Puzzle" ), _( "View the obelisk puzzle." ), Dialog::ZERO );
        if ( le.MousePressRight( buttonInfo.area() ) )
            fheroes2::showStandardTextMessage( _( "Scenario Information" ), _( "View information on the scenario you are currently playing." ), Dialog::ZERO );
        if ( le.MousePressRight( buttonDig.area() ) )
            fheroes2::showStandardTextMessage( _( "Digging" ), _( "Dig for the Ultimate Artifact." ), Dialog::ZERO );
        if ( le.MousePressRight( buttonCancel.area() ) )
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
    }

    // restore background
    back.restore();
    display.render();

    return result;
}
