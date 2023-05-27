/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "audio.h"
#include "audio_manager.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_hotkeys.h"
#include "game_mainmenu_ui.h"
#include "game_mode.h"
#include "icn.h"
#include "localevent.h"
#include "math_base.h"
#include "mus.h"
#include "text.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    const int32_t buttonYStep = 66;

    void showWIPInfo()
    {
        fheroes2::showMessage( fheroes2::Text{ _( "Warning!" ), fheroes2::FontType::normalYellow() },
                               fheroes2::Text{ "The Map Editor is still in WIP. This function is not available yet.", fheroes2::FontType::normalWhite() }, Dialog::OK );
    }

}

fheroes2::GameMode Game::editorMainMenu()
{
    // Stop all sounds, but not the music
    AudioManager::stopSounds();

    AudioManager::PlayMusicAsync( MUS::MAINMENU, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawEditorMainMenuScreen();

    const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

    fheroes2::Button newMap( buttonPos.x, buttonPos.y, ICN::BTNEMAIN, 0, 1 );
    fheroes2::Button loadMap( buttonPos.x, buttonPos.y + buttonYStep, ICN::BTNEMAIN, 2, 3 );
    fheroes2::Button cancel( buttonPos.x, buttonPos.y + 5 * buttonYStep, ICN::BUTTON_LARGE_CANCEL, 0, 1 );

    newMap.draw();
    loadMap.draw();
    cancel.draw();

    fheroes2::validateFadeInAndRender();

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        le.MousePressLeft( newMap.area() ) ? newMap.drawOnPress() : newMap.drawOnRelease();
        le.MousePressLeft( loadMap.area() ) ? loadMap.drawOnPress() : loadMap.drawOnRelease();
        le.MousePressLeft( cancel.area() ) ? cancel.drawOnPress() : cancel.drawOnRelease();

        if ( le.MouseClickLeft( newMap.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_STANDARD ) ) {
            return fheroes2::GameMode::EDITOR_NEW_MAP;
        }
        else if ( le.MouseClickLeft( loadMap.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_CAMPAIGN ) ) {
            return fheroes2::GameMode::EDITOR_LOAD_MAP;
        }
        else if ( le.MouseClickLeft( cancel.area() ) || HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) ) {
            return fheroes2::GameMode::MAIN_MENU;
        }

        if ( le.MousePressRight( newMap.area() ) ) {
            Dialog::Message( _( "New Map" ), _( "Create a new map either from scratch or using the random map generator." ), Font::BIG );
        }
        else if ( le.MousePressRight( loadMap.area() ) ) {
            Dialog::Message( _( "Load Map" ), _( "Load an existing map." ), Font::BIG );
        }
        else if ( le.MousePressRight( cancel.area() ) ) {
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );
        }
    }

    return fheroes2::GameMode::MAIN_MENU;
}

fheroes2::GameMode Game::editorNewMap()
{
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawEditorMainMenuScreen();

    const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

    fheroes2::Button scratchMap( buttonPos.x, buttonPos.y, ICN::BTNENEW, 0, 1 );
    fheroes2::Button randomMap( buttonPos.x, buttonPos.y + buttonYStep, ICN::BTNENEW, 2, 3 );
    fheroes2::Button cancel( buttonPos.x, buttonPos.y + 5 * buttonYStep, ICN::BUTTON_LARGE_CANCEL, 0, 1 );

    scratchMap.draw();
    randomMap.draw();
    cancel.draw();

    fheroes2::validateFadeInAndRender();

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        le.MousePressLeft( scratchMap.area() ) ? scratchMap.drawOnPress() : scratchMap.drawOnRelease();
        le.MousePressLeft( randomMap.area() ) ? randomMap.drawOnPress() : randomMap.drawOnRelease();
        le.MousePressLeft( cancel.area() ) ? cancel.drawOnPress() : cancel.drawOnRelease();

        if ( le.MouseClickLeft( scratchMap.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_STANDARD ) ) {
            showWIPInfo();
        }
        else if ( le.MouseClickLeft( randomMap.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_CAMPAIGN ) ) {
            showWIPInfo();
        }
        else if ( le.MouseClickLeft( cancel.area() ) || HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) ) {
            return fheroes2::GameMode::EDITOR_MAIN_MENU;
        }

        if ( le.MousePressRight( scratchMap.area() ) ) {
            Dialog::Message( _( "From Scratch" ), _( "Start from scratch with a blank map." ), Font::BIG );
        }
        else if ( le.MousePressRight( randomMap.area() ) ) {
            Dialog::Message( _( "Random" ), _( "Create a randomly generated map." ), Font::BIG );
        }
        else if ( le.MousePressRight( cancel.area() ) ) {
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );
        }
    }

    return fheroes2::GameMode::EDITOR_MAIN_MENU;
}

fheroes2::GameMode Game::editorLoadMap()
{
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawEditorMainMenuScreen();

    fheroes2::validateFadeInAndRender();

    showWIPInfo();

    return fheroes2::GameMode::EDITOR_MAIN_MENU;
}