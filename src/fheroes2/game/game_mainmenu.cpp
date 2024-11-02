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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

#include "agg_image.h"
#include "audio.h"
#include "audio_manager.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_game_settings.h"
#include "dialog_language_selection.h"
#include "dialog_resolution.h"
#include "editor_mainmenu.h"
#include "game.h" // IWYU pragma: associated
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_mainmenu_ui.h"
#include "game_mode.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "logging.h"
#include "math_base.h"
#include "mus.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "ui_text.h"
#include "ui_tool.h"

namespace
{
    struct ButtonInfo
    {
        uint32_t frame;
        fheroes2::Button & button;
        bool isOver;
        bool wasOver;
    };

    enum
    {
        NEWGAME_DEFAULT = 1,
        LOADGAME_DEFAULT = 5,
        HIGHSCORES_DEFAULT = 9,
        CREDITS_DEFAULT = 13,
        QUIT_DEFAULT = 17,
        EDITOR_DEFAULT = 20
    };

    void outputMainMenuInTextSupportMode()
    {
        START_TEXT_SUPPORT_MODE
        COUT( "Main Menu\n" )

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_NEW_GAME ) << " to choose New Game." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_LOAD_GAME ) << " to choose Load previously saved game." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_HIGHSCORES ) << " to show High Scores." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_CREDITS ) << " to show Credits." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_SETTINGS ) << " to open Game Settings." )

        if ( Settings::Get().isPriceOfLoyaltySupported() ) {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::EDITOR_MAIN_MENU ) << " to open Editor." )
        }

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_QUIT ) << " or " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL )
                       << " to Quit the game." )
    }
}

void Game::mainGameLoop( bool isFirstGameRun, bool isProbablyDemoVersion )
{
    fheroes2::GameMode result = fheroes2::GameMode::MAIN_MENU;

    bool exit = false;

    while ( !exit ) {
        switch ( result ) {
        case fheroes2::GameMode::QUIT_GAME:
            exit = true;
            break;
        case fheroes2::GameMode::MAIN_MENU:
            result = Game::MainMenu( isFirstGameRun );
            isFirstGameRun = false;
            break;
        case fheroes2::GameMode::NEW_GAME:
            result = Game::NewGame( isProbablyDemoVersion );
            isProbablyDemoVersion = false;
            break;
        case fheroes2::GameMode::LOAD_GAME:
            result = Game::LoadGame();
            break;
        case fheroes2::GameMode::HIGHSCORES_STANDARD:
            result = Game::DisplayHighScores( false );
            break;
        case fheroes2::GameMode::HIGHSCORES_CAMPAIGN:
            result = Game::DisplayHighScores( true );
            break;
        case fheroes2::GameMode::CREDITS:
            result = Game::Credits();
            break;
        case fheroes2::GameMode::NEW_STANDARD:
            result = Game::NewStandard();
            break;
        case fheroes2::GameMode::NEW_CAMPAIGN_SELECTION:
            result = Game::CampaignSelection();
            break;
        case fheroes2::GameMode::NEW_SUCCESSION_WARS_CAMPAIGN:
            result = Game::NewSuccessionWarsCampaign();
            break;
        case fheroes2::GameMode::NEW_PRICE_OF_LOYALTY_CAMPAIGN:
            result = Game::NewPriceOfLoyaltyCampaign();
            break;
        case fheroes2::GameMode::NEW_MULTI:
            result = Game::NewMulti();
            break;
        case fheroes2::GameMode::NEW_HOT_SEAT:
            result = Game::NewHotSeat();
            break;
        case fheroes2::GameMode::NEW_BATTLE_ONLY:
            result = Game::NewBattleOnly();
            break;
        case fheroes2::GameMode::LOAD_STANDARD:
            result = Game::LoadStandard();
            break;
        case fheroes2::GameMode::LOAD_CAMPAIGN:
            result = Game::LoadCampaign();
            break;
        case fheroes2::GameMode::LOAD_MULTI:
            result = Game::LoadMulti();
            break;
        case fheroes2::GameMode::LOAD_HOT_SEAT:
            result = Game::LoadHotseat();
            break;
        case fheroes2::GameMode::SELECT_SCENARIO_ONE_HUMAN_PLAYER:
        case fheroes2::GameMode::SELECT_SCENARIO_TWO_HUMAN_PLAYERS:
        case fheroes2::GameMode::SELECT_SCENARIO_THREE_HUMAN_PLAYERS:
        case fheroes2::GameMode::SELECT_SCENARIO_FOUR_HUMAN_PLAYERS:
        case fheroes2::GameMode::SELECT_SCENARIO_FIVE_HUMAN_PLAYERS:
        case fheroes2::GameMode::SELECT_SCENARIO_SIX_HUMAN_PLAYERS: {
            const uint8_t humanPlayerCount
                = static_cast<uint8_t>( static_cast<int>( result ) - static_cast<int>( fheroes2::GameMode::SELECT_SCENARIO_ONE_HUMAN_PLAYER ) );
            // Add +1 since we don't have zero human player option.
            result = Game::SelectScenario( humanPlayerCount + 1 );
            break;
        }
        case fheroes2::GameMode::START_GAME:
            result = Game::StartGame();
            break;
        case fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO:
            result = Game::SelectCampaignScenario( fheroes2::GameMode::MAIN_MENU, false );
            break;
        case fheroes2::GameMode::COMPLETE_CAMPAIGN_SCENARIO:
            result = Game::CompleteCampaignScenario( false );
            break;
        case fheroes2::GameMode::COMPLETE_CAMPAIGN_SCENARIO_FROM_LOAD_FILE:
            result = Game::CompleteCampaignScenario( true );
            while ( result == fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO ) {
                result = Game::SelectCampaignScenario( fheroes2::GameMode::LOAD_CAMPAIGN, false );
            }
            break;
        case fheroes2::GameMode::EDITOR_MAIN_MENU:
            result = Editor::menuMain();
            break;
        case fheroes2::GameMode::EDITOR_NEW_MAP:
            result = Editor::menuNewFromScratchMap();
            break;
        case fheroes2::GameMode::EDITOR_LOAD_MAP:
            result = Editor::menuLoadMap();
            break;

        default:
            // If this assertion blows up then you are entering an infinite loop!
            // Add the logic for the newly added entry.
            assert( 0 );
            exit = true;
            break;
        }
    }

    // We are quitting the game, so fade-out the screen.
    fheroes2::fadeOutDisplay();
}

fheroes2::GameMode Game::MainMenu( const bool isFirstGameRun )
{
    // Stop all sounds, but not the music
    AudioManager::stopSounds();

    AudioManager::PlayMusicAsync( MUS::MAINMENU, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

    Settings & conf = Settings::Get();

    conf.SetGameType( TYPE_MENU );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    // image background
    fheroes2::drawMainMenuScreen();
    if ( isFirstGameRun ) {
        // Fade in Main Menu image before showing messages. This also resets the "need fade" state to have no fade-in after these messages.
        fheroes2::validateFadeInAndRender();

        fheroes2::selectLanguage( fheroes2::getSupportedLanguages(), fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() ), true );

        if ( System::isHandheldDevice() ) {
            // Handheld devices should use the minimal game's resolution. Users on handheld devices aren't asked to choose resolution.
            fheroes2::showStandardTextMessage( _( "Greetings!" ), _( "Welcome to Heroes of Might and Magic II powered by fheroes2 engine!" ), Dialog::OK );
        }
        else {
            fheroes2::showStandardTextMessage(
                _( "Greetings!" ),
                _( "Welcome to Heroes of Might and Magic II powered by the fheroes2 engine!\nBefore starting the game, please select a game resolution." ), Dialog::OK );
            const bool isResolutionChanged = Dialog::SelectResolution();
            if ( isResolutionChanged ) {
                fheroes2::drawMainMenuScreen();
            }
        }

        fheroes2::Text header( _( "Please Remember" ), fheroes2::FontType::normalYellow() );

        fheroes2::MultiFontText body;
        body.add( { _( "You can always change the language, resolution and settings of the game by clicking on the " ), fheroes2::FontType::normalWhite() } );
        body.add( { _( "door" ), fheroes2::FontType::normalYellow() } );
        body.add( { _( " on the left side of the Main Menu, or with the " ), fheroes2::FontType::normalWhite() } );
        body.add( { _( "CONFIG" ), fheroes2::FontType::normalYellow() } );
        body.add( { _( " button from the " ), fheroes2::FontType::normalWhite() } );
        body.add( { _( "NEW GAME" ), fheroes2::FontType::normalYellow() } );
        body.add( { _( " menu. \n\nEnjoy the game!" ), fheroes2::FontType::normalWhite() } );

        fheroes2::showMessage( header, body, Dialog::OK );

        conf.resetFirstGameRun();
        conf.Save( Settings::configFileName );
    }

    outputMainMenuInTextSupportMode();

    LocalEvent & le = LocalEvent::Get();

    fheroes2::Button buttonNewGame( 0, 0, ICN::BTNSHNGL, NEWGAME_DEFAULT, NEWGAME_DEFAULT + 2 );
    fheroes2::Button buttonLoadGame( 0, 0, ICN::BTNSHNGL, LOADGAME_DEFAULT, LOADGAME_DEFAULT + 2 );
    fheroes2::Button buttonHighScores( 0, 0, ICN::BTNSHNGL, HIGHSCORES_DEFAULT, HIGHSCORES_DEFAULT + 2 );
    fheroes2::Button buttonCredits( 0, 0, ICN::BTNSHNGL, CREDITS_DEFAULT, CREDITS_DEFAULT + 2 );
    fheroes2::Button buttonEditor( 0, 0, ICN::BTNSHNGL, EDITOR_DEFAULT, EDITOR_DEFAULT + 2 );
    fheroes2::Button buttonQuit( 0, 0, ICN::BTNSHNGL, QUIT_DEFAULT, QUIT_DEFAULT + 2 );

    const fheroes2::Sprite & lantern10 = fheroes2::AGG::GetICN( ICN::SHNGANIM, 0 );
    fheroes2::Blit( lantern10, display, lantern10.x(), lantern10.y() );

    const fheroes2::Sprite & lantern11 = fheroes2::AGG::GetICN( ICN::SHNGANIM, ICN::getAnimatedIcnIndex( ICN::SHNGANIM, 0, 0 ) );
    fheroes2::Blit( lantern11, display, lantern11.x(), lantern11.y() );

    buttonNewGame.draw();
    buttonLoadGame.draw();
    buttonHighScores.draw();
    buttonCredits.draw();
    buttonQuit.draw();

    fheroes2::validateFadeInAndRender();

    const double scaleX = static_cast<double>( display.width() ) / fheroes2::Display::DEFAULT_WIDTH;
    const double scaleY = static_cast<double>( display.height() ) / fheroes2::Display::DEFAULT_HEIGHT;

    const double scale = std::min( scaleX, scaleY );
    const int32_t offsetX = static_cast<int32_t>( std::lround( display.width() - fheroes2::Display::DEFAULT_WIDTH * scale ) ) / 2;
    const int32_t offsetY = static_cast<int32_t>( std::lround( display.height() - fheroes2::Display::DEFAULT_HEIGHT * scale ) ) / 2;

    const fheroes2::Rect settingsArea( static_cast<int32_t>( 63 * scale ) + offsetX, static_cast<int32_t>( 202 * scale ) + offsetY, static_cast<int32_t>( 108 * scale ),
                                       static_cast<int32_t>( 160 * scale ) );

    uint32_t lantern_frame = 0;

    const bool isPOLPresent = conf.isPriceOfLoyaltySupported();

    std::vector<ButtonInfo> buttons{ ButtonInfo{ NEWGAME_DEFAULT, buttonNewGame, false, false }, ButtonInfo{ LOADGAME_DEFAULT, buttonLoadGame, false, false },
                                     ButtonInfo{ HIGHSCORES_DEFAULT, buttonHighScores, false, false }, ButtonInfo{ CREDITS_DEFAULT, buttonCredits, false, false },
                                     ButtonInfo{ QUIT_DEFAULT, buttonQuit, false, false } };

    if ( isPOLPresent ) {
        buttons.emplace_back( ButtonInfo{ EDITOR_DEFAULT, buttonEditor, false, false } );
    }

    for ( size_t i = 0; le.hasMouseMoved() && i < buttons.size(); ++i ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::BTNSHNGL, buttons[i].frame );
        fheroes2::Blit( sprite, display, sprite.x(), sprite.y() );
    }

    fheroes2::Sprite highlightDoor = fheroes2::AGG::GetICN( ICN::SHNGANIM, 18 );
    fheroes2::ApplyPalette( highlightDoor, 8 );

    while ( true ) {
        if ( !le.HandleEvents( true, true ) ) {
            if ( Interface::AdventureMap::EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                break;
            }
            else {
                continue;
            }
        }

        bool redrawScreen = false;

        for ( size_t i = 0; i < buttons.size(); ++i ) {
            buttons[i].wasOver = buttons[i].isOver;

            if ( le.isMouseLeftButtonPressedInArea( buttons[i].button.area() ) ) {
                buttons[i].button.drawOnPress();
            }
            else {
                buttons[i].button.drawOnRelease();
            }

            buttons[i].isOver = le.isMouseCursorPosInArea( buttons[i].button.area() );

            if ( buttons[i].isOver != buttons[i].wasOver ) {
                uint32_t frame = buttons[i].frame;

                if ( buttons[i].isOver && !buttons[i].wasOver )
                    ++frame;

                if ( !redrawScreen ) {
                    redrawScreen = true;
                }
                const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::BTNSHNGL, frame );
                fheroes2::Blit( sprite, display, sprite.x(), sprite.y() );
            }
        }

        if ( redrawScreen ) {
            display.render();
        }

        if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_NEW_GAME ) || le.MouseClickLeft( buttonNewGame.area() ) ) {
            return fheroes2::GameMode::NEW_GAME;
        }

        if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_LOAD_GAME ) || le.MouseClickLeft( buttonLoadGame.area() ) ) {
            return fheroes2::GameMode::LOAD_GAME;
        }

        if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_HIGHSCORES ) || le.MouseClickLeft( buttonHighScores.area() ) ) {
            return fheroes2::GameMode::HIGHSCORES_STANDARD;
        }

        if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_CREDITS ) || le.MouseClickLeft( buttonCredits.area() ) ) {
            return fheroes2::GameMode::CREDITS;
        }

        if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_QUIT ) || HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonQuit.area() ) ) {
            if ( Interface::AdventureMap::EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                return fheroes2::GameMode::QUIT_GAME;
            }
        }
        else if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_SETTINGS ) || le.MouseClickLeft( settingsArea ) ) {
            fheroes2::openGameSettings();

            return fheroes2::GameMode::MAIN_MENU;
        }
        else if ( isPOLPresent && ( HotKeyPressEvent( HotKeyEvent::EDITOR_MAIN_MENU ) || le.MouseClickLeft( buttonEditor.area() ) ) ) {
            return fheroes2::GameMode::EDITOR_MAIN_MENU;
        }

        // right info
        if ( le.isMouseRightButtonPressedInArea( buttonQuit.area() ) )
            fheroes2::showStandardTextMessage( _( "Quit" ), _( "Quit Heroes of Might and Magic II and return to the operating system." ), Dialog::ZERO );
        else if ( le.isMouseRightButtonPressedInArea( buttonLoadGame.area() ) )
            fheroes2::showStandardTextMessage( _( "Load Game" ), _( "Load a previously saved game." ), Dialog::ZERO );
        else if ( le.isMouseRightButtonPressedInArea( buttonCredits.area() ) )
            fheroes2::showStandardTextMessage( _( "Credits" ), _( "View the credits screen." ), Dialog::ZERO );
        else if ( le.isMouseRightButtonPressedInArea( buttonHighScores.area() ) )
            fheroes2::showStandardTextMessage( _( "High Scores" ), _( "View the high scores screen." ), Dialog::ZERO );
        else if ( le.isMouseRightButtonPressedInArea( buttonNewGame.area() ) )
            fheroes2::showStandardTextMessage( _( "New Game" ), _( "Start a single or multi-player game." ), Dialog::ZERO );
        else if ( isPOLPresent && le.isMouseRightButtonPressedInArea( buttonEditor.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Editor" ), _( "Create new or modify existing maps." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( settingsArea ) )
            fheroes2::showStandardTextMessage( _( "Game Settings" ), _( "Change language, resolution and settings of the game." ), Dialog::ZERO );

        if ( validateAnimationDelay( MAIN_MENU_DELAY ) ) {
            const fheroes2::Sprite & lantern12 = fheroes2::AGG::GetICN( ICN::SHNGANIM, ICN::getAnimatedIcnIndex( ICN::SHNGANIM, 0, lantern_frame ) );
            ++lantern_frame;
            fheroes2::Blit( lantern12, display, lantern12.x(), lantern12.y() );
            if ( le.isMouseCursorPosInArea( settingsArea ) ) {
                const int32_t doorOffsetY = static_cast<int32_t>( 55 * scale ) + offsetY;
                fheroes2::Blit( highlightDoor, 0, doorOffsetY, display, highlightDoor.x(), highlightDoor.y() + doorOffsetY, highlightDoor.width(),
                                highlightDoor.height() );
            }

            display.render();
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}
