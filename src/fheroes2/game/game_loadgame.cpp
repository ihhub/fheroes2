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

#include "game.h" // IWYU pragma: associated

#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>

#include "agg_image.h"
#include "audio.h"
#include "audio_manager.h"
#include "cursor.h"
#include "dialog.h"
#include "dir.h"
#include "game_hotkeys.h"
#include "game_io.h"
#include "game_mainmenu_ui.h"
#include "game_mode.h"
#include "icn.h"
#include "localevent.h"
#include "logging.h"
#include "math_base.h"
#include "mus.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_tool.h"

namespace
{
    const int32_t buttonYStep = 66;

    void outputLoadGameInTextSupportMode()
    {
        START_TEXT_SUPPORT_MODE

        COUT( "Load Game\n" )

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_STANDARD ) << " to choose Standard Game." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_CAMPAIGN ) << " to choose Campaign Game." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_MULTI ) << " to show Multi-Player Game." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL ) << " to go back to Main Menu." )
    }
}

fheroes2::GameMode Game::LoadCampaign()
{
    Settings::Get().SetGameType( Game::TYPE_CAMPAIGN );
    return DisplayLoadGameDialog();
}

fheroes2::GameMode Game::LoadHotseat()
{
    Settings::Get().SetGameType( Game::TYPE_HOTSEAT );
    return DisplayLoadGameDialog();
}

fheroes2::GameMode Game::LoadMulti()
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    // image background
    fheroes2::drawMainMenuScreen();

    const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

    fheroes2::Button buttonHotSeat( buttonPos.x, buttonPos.y, ICN::BUTTON_HOT_SEAT, 0, 1 );
    fheroes2::Button buttonNetwork( buttonPos.x, buttonPos.y + buttonYStep * 1, ICN::BTNMP, 2, 3 );
    fheroes2::Button buttonCancelGame( buttonPos.x, buttonPos.y + buttonYStep * 5, ICN::BUTTON_LARGE_CANCEL, 0, 1 );

    buttonHotSeat.draw();
    buttonCancelGame.draw();
    buttonNetwork.disable();

    display.render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonHotSeat.area() ) ? buttonHotSeat.drawOnPress() : buttonHotSeat.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        if ( le.MouseClickLeft( buttonHotSeat.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_HOTSEAT ) ) {
            if ( ListFiles::IsEmpty( GetSaveDir(), GetSaveFileExtension( Game::TYPE_HOTSEAT ), false ) ) {
                fheroes2::showStandardTextMessage( _( "Load Game" ), _( "No save files to load." ), Dialog::OK );
            }
            else {
                return fheroes2::GameMode::LOAD_HOT_SEAT;
            }
        }
        else if ( HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancelGame.area() ) ) {
            return fheroes2::GameMode::LOAD_GAME;
        }

        // right info
        else if ( le.isMouseRightButtonPressedInArea( buttonHotSeat.area() ) ) {
            fheroes2::showStandardTextMessage(
                _( "Hot Seat" ), _( "Play a Hot Seat game, where 2 to 6 players play around the same computer, switching into the 'Hot Seat' when it is their turn." ),
                Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonCancelGame.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the main menu." ), Dialog::ZERO );
        }
    }

    return fheroes2::GameMode::LOAD_GAME;
}

fheroes2::GameMode Game::LoadGame()
{
    outputLoadGameInTextSupportMode();

    // Stop all sounds, but not the music
    AudioManager::stopSounds();

    AudioManager::PlayMusicAsync( MUS::MAINMENU, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawMainMenuScreen();

    const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

    fheroes2::Button buttonStandardGame( 0, 0, ICN::BUTTON_STANDARD_GAME, 0, 1 );
    fheroes2::ButtonSprite buttonCampaignGame( buttonPos.x, buttonPos.y + buttonYStep * 1, fheroes2::AGG::GetICN( ICN::BUTTON_CAMPAIGN_GAME, 0 ),
                                               fheroes2::AGG::GetICN( ICN::BUTTON_CAMPAIGN_GAME, 1 ), fheroes2::AGG::GetICN( ICN::NEW_CAMPAIGN_DISABLED_BUTTON, 0 ) );
    fheroes2::Button buttonMultiplayerGame( 0, 0, ICN::BUTTON_MULTIPLAYER_GAME, 0, 1 );
    fheroes2::Button buttonCancel( 0, 0, ICN::BUTTON_LARGE_CANCEL, 0, 1 );

    std::array<fheroes2::ButtonBase *, 4> buttons{ &buttonStandardGame, &buttonCampaignGame, &buttonMultiplayerGame, &buttonCancel };

    if ( !isSuccessionWarsCampaignPresent() ) {
        buttonCampaignGame.disable();
    }

    static_assert( buttons.size() > 1, "The number of buttons in this dialog cannot be less than 2!" );
    for ( size_t i = 0; i < buttons.size() - 1; ++i ) {
        buttons[i]->setPosition( buttonPos.x, buttonPos.y + buttonYStep * static_cast<int32_t>( i ) );
        buttons[i]->draw();
    }

    // following the cancel button in newgame
    buttonCancel.setPosition( buttonPos.x, buttonPos.y + buttonYStep * 5 );
    buttonCancel.draw();

    fheroes2::validateFadeInAndRender();

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        for ( fheroes2::ButtonBase * button : buttons ) {
            le.isMouseLeftButtonPressedInArea( button->area() ) ? button->drawOnPress() : button->drawOnRelease();
        }

        if ( le.MouseClickLeft( buttonStandardGame.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_STANDARD ) ) {
            if ( ListFiles::IsEmpty( GetSaveDir(), GetSaveFileExtension( Game::TYPE_STANDARD ), false ) ) {
                fheroes2::showStandardTextMessage( _( "Load Game" ), _( "No save files to load." ), Dialog::OK );
            }
            else {
                return fheroes2::GameMode::LOAD_STANDARD;
            }
        }
        else if ( buttonCampaignGame.isEnabled() && ( le.MouseClickLeft( buttonCampaignGame.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_CAMPAIGN ) ) ) {
            if ( ListFiles::IsEmpty( GetSaveDir(), GetSaveFileExtension( Game::TYPE_CAMPAIGN ), false ) ) {
                fheroes2::showStandardTextMessage( _( "Load Game" ), _( "No save files to load." ), Dialog::OK );
            }
            else {
                return fheroes2::GameMode::LOAD_CAMPAIGN;
            }
        }
        else if ( le.MouseClickLeft( buttonMultiplayerGame.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_MULTI ) ) {
            return fheroes2::GameMode::LOAD_MULTI;
        }
        else if ( le.MouseClickLeft( buttonCancel.area() ) || HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) ) {
            return fheroes2::GameMode::MAIN_MENU;
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonStandardGame.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Standard Game" ), _( "A single player game playing out a single map." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonCampaignGame.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Campaign Game" ), _( "A single player game playing through a series of maps." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonMultiplayerGame.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Multi-Player Game" ),
                                               _( "A multi-player game, with several human players completing against each other on a single map." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the main menu." ), Dialog::ZERO );
        }
    }

    return fheroes2::GameMode::MAIN_MENU;
}

fheroes2::GameMode Game::LoadStandard()
{
    Settings::Get().SetGameType( Game::TYPE_STANDARD );
    return DisplayLoadGameDialog();
}

fheroes2::GameMode Game::DisplayLoadGameDialog()
{
    // Stop all sounds, but not the music
    AudioManager::stopSounds();

    AudioManager::PlayMusicAsync( MUS::MAINMENU, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    // image background
    fheroes2::drawMainMenuScreen();

    fheroes2::validateFadeInAndRender();

    const std::string file = Dialog::SelectFileLoad();
    if ( file.empty() ) {
        return fheroes2::GameMode::LOAD_GAME;
    }

    const fheroes2::GameMode returnValue = Game::Load( file );
    if ( returnValue == fheroes2::GameMode::CANCEL ) {
        return fheroes2::GameMode::LOAD_GAME;
    }

    // We are loading a game so fade-out main menu screen.
    fheroes2::fadeOutDisplay();

    setDisplayFadeIn();

    return returnValue;
}
