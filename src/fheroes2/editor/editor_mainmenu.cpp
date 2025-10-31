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
#include "image.h"
#include "localevent.h"
#include "logging.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "mus.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
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

    fheroes2::GameMode editNewMapFromScratch( const Maps::MapSize & mapSize )
    {
        fheroes2::fadeOutDisplay();
        Game::setDisplayFadeIn();

        Interface::EditorInterface & editorInterface = Interface::EditorInterface::Get();
        if ( editorInterface.generateNewMap( mapSize ) ) {
            return editorInterface.startEdit();
        }
        return fheroes2::GameMode::EDITOR_NEW_MAP;
    }

    fheroes2::GameMode editNewRandomMap( const Maps::MapSize & mapSize )
    {
        fheroes2::fadeOutDisplay();
        Game::setDisplayFadeIn();

        Interface::EditorInterface & editorInterface = Interface::EditorInterface::Get();
        if ( editorInterface.generateRandomMap( mapSize ) ) {
            return editorInterface.startEdit();
        }
        return fheroes2::GameMode::EDITOR_NEW_MAP;
    }
}

namespace Editor
{
    fheroes2::GameMode menuMain( const bool straightToSelectMapSize )
    {
        // Stop all sounds, but not the music
        AudioManager::stopSounds();

        AudioManager::PlayMusicAsync( MUS::MAINMENU, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::drawEditorMainMenuScreen();

        // Setup main dialog buttons.
        const int menuButtonsIcnIndex = Settings::Get().isEvilInterfaceEnabled() ? ICN::BUTTONS_EDITOR_MENU_EVIL : ICN::BUTTONS_EDITOR_MENU_GOOD;
        fheroes2::ButtonGroup mainModeButtons;
        // Only add the buttons needed for the initial state of the dialog.
        for ( int32_t i = 0; i < 2; ++i ) {
            mainModeButtons.createButton( 0, 0, menuButtonsIcnIndex, i * 2, i * 2 + 1, i );
        }

        const fheroes2::ButtonBase & buttonNewMap = mainModeButtons.button( 0 );
        const fheroes2::ButtonBase & buttonLoadMap = mainModeButtons.button( 1 );

        // Generate dialog background with extra space for cancel button and empty space for 3 buttons to match the original dialog's height.
        const int32_t spaceBetweenButtons = 10;
        fheroes2::StandardWindow background( mainModeButtons, true, ( buttonNewMap.area().height + spaceBetweenButtons ) * 4 );

        background.applyGemDecoratedCorners();

        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::ImageRestorer emptyDialog( display, background.activeArea().x, background.activeArea().y, background.activeArea().width,
                                             background.activeArea().height );

        background.renderSymmetricButtons( mainModeButtons, 0, true );

        fheroes2::Button buttonMainMenu( buttonNewMap.area().x,
                                         background.activeArea().y * 2 + background.activeArea().height - buttonNewMap.area().y - buttonNewMap.area().height,
                                         menuButtonsIcnIndex, 4, 5 );

        fheroes2::Button buttonBack( buttonMainMenu.area().x, buttonMainMenu.area().y, menuButtonsIcnIndex, 6, 7 );

        // Add From Scratch and Random buttons. Currently unused until Random map generator has been implemented.
        fheroes2::ButtonGroup mapCreationModeButtons;
        for ( int32_t i = 0; i < 2; ++i ) {
            mapCreationModeButtons.createButton( buttonNewMap.area().x, buttonNewMap.area().y + i * ( buttonNewMap.area().height + spaceBetweenButtons ),
                                                 menuButtonsIcnIndex, ( i + 4 ) * 2, ( i + 4 ) * 2 + 1, i );
        }

        const fheroes2::ButtonBase & buttonScratchMap = mapCreationModeButtons.button( 0 );
        const fheroes2::ButtonBase & buttonRandomMap = mapCreationModeButtons.button( 1 );

        // Add map size buttons
        fheroes2::ButtonGroup mapSizeButtons;
        for ( int32_t i = 0; i < static_cast<int32_t>( mapSizeCount ); ++i ) {
            mapSizeButtons.createButton( buttonNewMap.area().x, buttonNewMap.area().y + i * ( buttonNewMap.area().height + spaceBetweenButtons ), menuButtonsIcnIndex,
                                         ( i + 6 ) * 2, ( i + 6 ) * 2 + 1, i );
        }
        mapSizeButtons.disable();

        if ( !straightToSelectMapSize ) {
            outputEditorMainMenuInTextSupportMode();

            mapCreationModeButtons.disable();
            buttonBack.disable();

            buttonMainMenu.draw();
            buttonMainMenu.drawShadow( display );
        }
        else {
            outputEditorMapSizeMenuInTextSupportMode();

            mainModeButtons.disable();
            buttonMainMenu.disable();
            emptyDialog.restore();

            mapCreationModeButtons.draw();
            mapCreationModeButtons.drawShadows( display );

            buttonBack.draw();
            buttonBack.drawShadow( display );
        }

        fheroes2::validateFadeInAndRender();

        LocalEvent & le = LocalEvent::Get();

        bool generateRandomMap = false;

        while ( le.HandleEvents() ) {
            if ( buttonNewMap.isEnabled() ) {
                mainModeButtons.drawOnState( le );
                buttonMainMenu.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonMainMenu.area() ) );

                if ( le.MouseClickLeft( buttonNewMap.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_NEW_MAP_MENU ) ) {
                    mainModeButtons.disable();
                    buttonMainMenu.disable();

                    emptyDialog.restore();
                    mapCreationModeButtons.enable();
                    mapCreationModeButtons.draw();
                    mapCreationModeButtons.drawShadows( display );

                    buttonBack.enable();
                    buttonBack.draw();
                    buttonBack.drawShadow( display );

                    display.render( background.activeArea() );

                    outputEditorNewMapMenuInTextSupportMode();
                }
                else if ( le.MouseClickLeft( buttonLoadMap.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_LOAD_MAP_MENU ) ) {
                    return fheroes2::GameMode::EDITOR_LOAD_MAP;
                }

                if ( le.MouseClickLeft( buttonMainMenu.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                    return fheroes2::GameMode::MAIN_MENU;
                }

                if ( le.isMouseRightButtonPressedInArea( buttonNewMap.area() ) ) {
                    fheroes2::showStandardTextMessage( _( "New Map" ), _( "Create a new map, either from scratch or using the random map generator." ), Dialog::ZERO );
                }
                else if ( le.isMouseRightButtonPressedInArea( buttonLoadMap.area() ) ) {
                    fheroes2::showStandardTextMessage( _( "Load Map" ), _( "Load an existing map." ), Dialog::ZERO );
                }
                else if ( le.isMouseRightButtonPressedInArea( buttonMainMenu.area() ) ) {
                    fheroes2::showStandardTextMessage( _( "Main Menu" ), _( "Exit the Editor and return to the game's Main Menu." ), Dialog::ZERO );
                }
            }
            else if ( mapCreationModeButtons.button( 0 ).isEnabled() ) {
                mapCreationModeButtons.drawOnState( le );
                buttonBack.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonBack.area() ) );

                auto prepareMapSizeMenu = [&]() {
                    mapCreationModeButtons.disable();
                    emptyDialog.restore();

                    mapSizeButtons.enable();
                    mapSizeButtons.draw();
                    mapSizeButtons.drawShadows( display );

                    buttonBack.draw();
                    buttonBack.drawShadow( display );

                    display.render( background.activeArea() );

                    outputEditorMapSizeMenuInTextSupportMode();
                };

                if ( le.MouseClickLeft( buttonScratchMap.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_FROM_SCRATCH_MAP_MENU ) ) {
                    generateRandomMap = false;

                    prepareMapSizeMenu();
                }
                else if ( le.MouseClickLeft( buttonRandomMap.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::EDITOR_RANDOM_MAP_MENU ) ) {
                    generateRandomMap = true;

                    prepareMapSizeMenu();

                    fheroes2::showStandardTextMessage(
                        _( "Warning!" ),
                        "This feature is still in development and has some limitations. Errors might occur. This feature will continue to change as we are working on improving it.",
                        Dialog::OK );
                }
                else if ( le.MouseClickLeft( buttonBack.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                    mapCreationModeButtons.disable();
                    buttonBack.disable();
                    emptyDialog.restore();

                    mainModeButtons.enable();
                    mainModeButtons.draw();
                    mainModeButtons.drawShadows( display );

                    buttonMainMenu.enable();
                    buttonMainMenu.draw();
                    buttonMainMenu.drawShadow( display );

                    display.render( background.activeArea() );

                    outputEditorMainMenuInTextSupportMode();
                }
                else if ( le.isMouseRightButtonPressedInArea( buttonScratchMap.area() ) ) {
                    fheroes2::showStandardTextMessage( _( "From Scratch" ), _( "Start from scratch with a blank map." ), Dialog::ZERO );
                }
                else if ( le.isMouseRightButtonPressedInArea( buttonRandomMap.area() ) ) {
                    fheroes2::showStandardTextMessage( _( "Random" ), _( "Create a randomly generated map." ), Dialog::ZERO );
                }
                else if ( le.isMouseRightButtonPressedInArea( buttonBack.area() ) ) {
                    fheroes2::showStandardTextMessage( _( "Back" ), _( "Return to the Editor's main menu options." ), Dialog::ZERO );
                }
            }
            else if ( mapSizeButtons.button( 0 ).isEnabled() ) {
                mapSizeButtons.drawOnState( le );
                buttonBack.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonBack.area() ) );

                // Loop through all map size buttons.
                for ( size_t i = 0; i < mapSizeCount; ++i ) {
                    if ( le.MouseClickLeft( mapSizeButtons.button( i ).area() ) || Game::HotKeyPressEvent( mapSizeHotkeys[i] ) ) {
                        if ( generateRandomMap ) {
                            return editNewRandomMap( mapSizes[i] );
                        }

                        return editNewMapFromScratch( mapSizes[i] );
                    }

                    if ( le.isMouseRightButtonPressedInArea( mapSizeButtons.button( i ).area() ) ) {
                        std::string mapSize = std::to_string( mapSizes[i] );
                        std::string message;
                        if ( generateRandomMap ) {
                            message = _( "Generate a random map that is %{size} squares wide and %{size} squares high." );
                        }
                        else {
                            message = _( "Create a map that is %{size} squares wide and %{size} squares high." );
                        }
                        StringReplace( message, "%{size}", mapSize );
                        mapSize += " x " + mapSize;
                        fheroes2::showStandardTextMessage( mapSize, message, Dialog::ZERO );

                        break;
                    }
                }

                if ( le.MouseClickLeft( buttonBack.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                    mapSizeButtons.disable();
                    emptyDialog.restore();

                    mapCreationModeButtons.enable();
                    mapCreationModeButtons.draw();
                    mapCreationModeButtons.drawShadows( display );

                    buttonBack.draw();
                    buttonBack.drawShadow( display );

                    display.render( background.activeArea() );

                    outputEditorMainMenuInTextSupportMode();
                }

                else if ( le.isMouseRightButtonPressedInArea( buttonBack.area() ) ) {
                    fheroes2::showStandardTextMessage( _( "Back" ), _( "Return to the previous menu options." ), Dialog::ZERO );
                }
            }
        }

        return fheroes2::GameMode::MAIN_MENU;
    }

    fheroes2::GameMode menuLoadMap()
    {
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::drawEditorMainMenuScreen();

        fheroes2::validateFadeInAndRender();

        MapsFileInfoList lists = Maps::getResurrectionMapFileInfos( true, 0 );
        if ( lists.empty() ) {
            fheroes2::showStandardTextMessage( _( "Warning" ), _( "No maps available!" ), Dialog::OK );
            return fheroes2::GameMode::EDITOR_MAIN_MENU;
        }

        const Maps::FileInfo * fileInfo = Dialog::SelectScenario( lists, true );
        if ( lists.empty() ) {
            // This can happen if all maps have been deleted.
            return fheroes2::GameMode::EDITOR_MAIN_MENU;
        }

        if ( fileInfo == nullptr ) {
            return fheroes2::GameMode::EDITOR_MAIN_MENU;
        }

        Interface::EditorInterface & editorInterface = Interface::EditorInterface::Get();
        if ( !editorInterface.loadMap( fileInfo->filename ) ) {
            return fheroes2::GameMode::EDITOR_MAIN_MENU;
        }

        fheroes2::fadeOutDisplay();
        Game::setDisplayFadeIn();

        return Interface::EditorInterface::Get().startEdit();
    }
}
