/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2025                                             *
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

#include "editor_mainmenu.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>

#include "audio.h"
#include "audio_manager.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectscenario.h"
#include "editor_interface.h"
#include "game.h"
#include "game_hotkeys.h"
#include "game_mainmenu_ui.h"
#include "game_mode.h"
#include "icn.h"
#include "localevent.h"
#include "logging.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "mus.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_tool.h"

namespace
{
    const int32_t buttonYStep = 66;
    const size_t mapSizeCount = 4;
    const std::array<Game::HotKeyEvent, mapSizeCount> mapSizeHotkeys = { Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_SMALL, Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_MEDIUM,
                                                                         Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_LARGE, Game::HotKeyEvent::MAIN_MENU_MAP_SIZE_EXTRA_LARGE };
    const std::array<Maps::MapSize, mapSizeCount> mapSizes = { Maps::SMALL, Maps::MEDIUM, Maps::LARGE, Maps::XLARGE };

    void outputEditorMainMenuInTextSupportMode()
    {
        START_TEXT_SUPPORT_MODE

        COUT( "Map Editor\n" )

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::EDITOR_NEW_MAP_MENU )
                       << " to create a new map either from scratch or using the random map generator." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::EDITOR_LOAD_MAP_MENU ) << " to load an existing map." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL ) << " to go back to Main Menu." )
    }

    void outputEditorNewMapMenuInTextSupportMode()
    {
        START_TEXT_SUPPORT_MODE

        COUT( "New Map\n" )

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::EDITOR_FROM_SCRATCH_MAP_MENU ) << " to create a blank map from scratch." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::EDITOR_RANDOM_MAP_MENU ) << " to create a randomly generated map." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL ) << " to go back to Map Editor main menu." )
    }

    void outputEditorMapSizeMenuInTextSupportMode()
    {
        START_TEXT_SUPPORT_MODE

        COUT( "Map Size\n" )

        const size_t buttonCount = mapSizeHotkeys.size();
        for ( size_t i = 0; i < buttonCount; ++i ) {
            std::string message = " to create a map that is %{size} squares wide and %{size} squares high.";
            StringReplace( message, "%{size}", std::to_string( mapSizes[i] ) );

            COUT( "Press " << Game::getHotKeyNameByEventId( mapSizeHotkeys[i] ) << message )
        }

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL ) << " to go back to New Map menu." )
    }

    void showWIPInfo()
    {
        fheroes2::showStandardTextMessage( _( "Warning" ), "The Map Editor is still in development. This function is not available yet.", Dialog::OK );
    }

    Maps::MapSize selectMapSize()
    {
        outputEditorMapSizeMenuInTextSupportMode();

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::drawEditorMainMenuScreen();

        const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

        std::array<fheroes2::Button, mapSizeCount> buttons;

        for ( uint32_t i = 0; i < mapSizeCount; ++i ) {
            buttons[i].setICNInfo( ICN::BTNESIZE, 0 + i * 2, 1 + i * 2 );
            buttons[i].setPosition( buttonPos.x, buttonPos.y + buttonYStep * static_cast<int32_t>( i ) );
            buttons[i].draw();
        }

        fheroes2::Button buttonCancel( buttonPos.x, buttonPos.y + 5 * buttonYStep, ICN::BUTTON_LARGE_CANCEL, 0, 1 );

        buttonCancel.draw();

        fheroes2::validateFadeInAndRender();

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            for ( size_t i = 0; i < mapSizeCount; ++i ) {
                buttons[i].drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttons[i].area() ) );

                if ( le.MouseClickLeft( buttons[i].area() ) || Game::HotKeyPressEvent( mapSizeHotkeys[i] ) ) {
                    return mapSizes[i];
                }

                if ( le.isMouseRightButtonPressedInArea( buttons[i].area() ) ) {
                    std::string mapSize = std::to_string( mapSizes[i] );
                    std::string message = _( "Create a map that is %{size} squares wide and %{size} squares high." );
                    StringReplace( message, "%{size}", mapSize );
                    mapSize += " x " + mapSize;
                    fheroes2::showStandardTextMessage( mapSize, message, Dialog::ZERO );
                }
            }

            buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return Maps::ZERO;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the New Map menu." ), Dialog::ZERO );
            }
        }

        return Maps::ZERO;
    }
}

namespace Editor
{
    fheroes2::GameMode menuMain()
    {
        outputEditorMainMenuInTextSupportMode();

        // Stop all sounds, but not the music
        AudioManager::stopSounds();

        AudioManager::PlayMusicAsync( MUS::MAINMENU, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::drawEditorMainMenuScreen();

        const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

        fheroes2::Button buttonNewMap( buttonPos.x, buttonPos.y, ICN::BUTTON_NEW_MAP, 0, 1 );
        fheroes2::Button buttonLoadMap( buttonPos.x, buttonPos.y + buttonYStep, ICN::BUTTON_LOAD_MAP, 0, 1 );
        fheroes2::Button buttonCancel( buttonPos.x, buttonPos.y + 5 * buttonYStep, ICN::BUTTON_LARGE_CANCEL, 0, 1 );

        buttonNewMap.draw();
        buttonLoadMap.draw();
        buttonCancel.draw();

        fheroes2::validateFadeInAndRender();

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            buttonNewMap.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonNewMap.area() ) );
            buttonLoadMap.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonLoadMap.area() ) );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

            if ( le.MouseClickLeft( buttonNewMap.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_NEW_MAP_MENU ) ) {
                return fheroes2::GameMode::EDITOR_NEW_MAP;
            }
            if ( le.MouseClickLeft( buttonLoadMap.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_LOAD_MAP_MENU ) ) {
                return fheroes2::GameMode::EDITOR_LOAD_MAP;
            }
            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return fheroes2::GameMode::MAIN_MENU;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonNewMap.area() ) ) {
                // TODO: update this text once random map generator is ready.
                //       The original text should be "Create a new map, either from scratch or using the random map generator."
                fheroes2::showStandardTextMessage( _( "New Map" ), _( "Create a new map from scratch." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonLoadMap.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Load Map" ), _( "Load an existing map." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the main menu." ), Dialog::ZERO );
            }
        }

        return fheroes2::GameMode::MAIN_MENU;
    }

    fheroes2::GameMode menuNewMap()
    {
        outputEditorNewMapMenuInTextSupportMode();

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::drawEditorMainMenuScreen();

        const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

        fheroes2::Button buttonScratchMap( buttonPos.x, buttonPos.y, ICN::BTNENEW, 0, 1 );
        fheroes2::Button buttonRandomMap( buttonPos.x, buttonPos.y + buttonYStep, ICN::BTNENEW, 2, 3 );
        fheroes2::Button buttonCancel( buttonPos.x, buttonPos.y + 5 * buttonYStep, ICN::BUTTON_LARGE_CANCEL, 0, 1 );

        // TODO: enable it back once random map generator is ready.
        buttonRandomMap.disable();

        buttonScratchMap.draw();
        buttonRandomMap.draw();
        buttonCancel.draw();

        fheroes2::validateFadeInAndRender();

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            buttonScratchMap.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonScratchMap.area() ) );

            if ( buttonRandomMap.isEnabled() ) {
                buttonRandomMap.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonRandomMap.area() ) );
            }

            buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

            if ( le.MouseClickLeft( buttonScratchMap.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_FROM_SCRATCH_MAP_MENU ) ) {
                const Maps::MapSize mapSize = selectMapSize();
                if ( mapSize != Maps::ZERO ) {
                    fheroes2::fadeOutDisplay();
                    Game::setDisplayFadeIn();

                    Interface::EditorInterface & editorInterface = Interface::EditorInterface::Get();
                    if ( editorInterface.generateNewMap( mapSize ) ) {
                        return editorInterface.startEdit();
                    }
                }
                return fheroes2::GameMode::EDITOR_NEW_MAP;
            }

            if ( buttonRandomMap.isEnabled() && ( le.MouseClickLeft( buttonRandomMap.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_RANDOM_MAP_MENU ) ) ) {
                if ( selectMapSize() != Maps::ZERO ) {
                    showWIPInfo();
                }
                return fheroes2::GameMode::EDITOR_NEW_MAP;
            }

            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return fheroes2::GameMode::EDITOR_MAIN_MENU;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonScratchMap.area() ) ) {
                fheroes2::showStandardTextMessage( _( "From Scratch" ), _( "Start from scratch with a blank map." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonRandomMap.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Random" ), _( "Create a randomly generated map." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the Map Editor main menu." ), Dialog::ZERO );
            }
        }

        return fheroes2::GameMode::EDITOR_MAIN_MENU;
    }

    fheroes2::GameMode menuLoadMap()
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::drawEditorMainMenuScreen();

        fheroes2::validateFadeInAndRender();

        const MapsFileInfoList lists = Maps::getResurrectionMapFileInfos( true, 0 );
        if ( lists.empty() ) {
            fheroes2::showStandardTextMessage( _( "Warning" ), _( "No maps available!" ), Dialog::OK );
            return fheroes2::GameMode::EDITOR_MAIN_MENU;
        }

        const Maps::FileInfo * fileInfo = Dialog::SelectScenario( lists, true );
        if ( fileInfo == nullptr ) {
            return fheroes2::GameMode::EDITOR_MAIN_MENU;
        }

        Interface::EditorInterface & editorInterface = Interface::EditorInterface::Get();
        if ( !editorInterface.loadMap( fileInfo->filename ) ) {
            return fheroes2::GameMode::CANCEL;
        }

        fheroes2::fadeOutDisplay();
        Game::setDisplayFadeIn();

        return Interface::EditorInterface::Get().startEdit();
    }

    fheroes2::GameMode menuNewFromScratchMap()
    {
        const Maps::MapSize mapSize = selectMapSize();
        if ( mapSize != Maps::ZERO ) {
            fheroes2::fadeOutDisplay();
            Game::setDisplayFadeIn();

            Interface::EditorInterface & editorInterface = Interface::EditorInterface::Get();
            if ( editorInterface.generateNewMap( mapSize ) ) {
                return editorInterface.startEdit();
            }
        }
        return fheroes2::GameMode::EDITOR_MAIN_MENU;
    }
}
