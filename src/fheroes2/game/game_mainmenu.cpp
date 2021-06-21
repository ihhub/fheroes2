/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include "agg.h"
#include "agg_image.h"
#include "audio_mixer.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_resolution.h"
#include "game.h"
#include "game_delays.h"
#include "game_interface.h"
#include "game_mainmenu_ui.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "mus.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    struct ButtonInfo
    {
        u32 frame;
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
        QUIT_DEFAULT = 17
    };
}

void Game::mainGameLoop( bool isFirstGameRun )
{
    fheroes2::GameMode result = fheroes2::GameMode::MAIN_MENU;

    while ( result != fheroes2::GameMode::QUIT_GAME ) {
        switch ( result ) {
        case fheroes2::GameMode::MAIN_MENU:
            result = Game::MainMenu( isFirstGameRun );
            isFirstGameRun = false;
            break;
        case fheroes2::GameMode::NEW_GAME:
            result = Game::NewGame();
            break;
        case fheroes2::GameMode::LOAD_GAME:
            result = Game::LoadGame();
            break;
        case fheroes2::GameMode::HIGHSCORES:
            result = Game::HighScores();
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
#ifdef NETWORK_ENABLE
        case fheroes2::GameMode::NEW_NETWORK:
            result = Game::NewNetwork();
            break;
#endif
        case fheroes2::GameMode::NEW_BATTLE_ONLY:
            result = Game::NewBattleOnly();
            break;
        case fheroes2::GameMode::LOAD_STANDARD:
            result = Game::LoadStandard();
            break;
        case fheroes2::GameMode::LOAD_CAMPAIN:
            result = Game::LoadCampaign();
            break;
        case fheroes2::GameMode::LOAD_MULTI:
            result = Game::LoadMulti();
            break;
        case fheroes2::GameMode::LOAD_HOT_SEAT:
            result = Game::LoadHotseat();
            break;
        case fheroes2::GameMode::LOAD_NETWORK:
            result = Game::LoadNetwork();
            break;
        case fheroes2::GameMode::SCENARIO_INFO:
            result = Game::ScenarioInfo();
            break;
        case fheroes2::GameMode::SELECT_SCENARIO:
            result = Game::SelectScenario();
            break;
        case fheroes2::GameMode::START_GAME:
            result = Game::StartGame();
            break;
        case fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO:
            result = Game::SelectCampaignScenario( fheroes2::GameMode::NEW_GAME );
            break;
        case fheroes2::GameMode::COMPLETE_CAMPAIGN_SCENARIO:
            result = Game::CompleteCampaignScenario();
            break;
        case fheroes2::GameMode::COMPLETE_CAMPAIGN_SCENARIO_FROM_LOAD_FILE:
            result = Game::CompleteCampaignScenario();
            if ( result == fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO ) {
                result = Game::SelectCampaignScenario( fheroes2::GameMode::LOAD_CAMPAIN );
            }
            break;

        default:
            break;
        }
    }
}

fheroes2::GameMode Game::MainMenu( bool isFirstGameRun )
{
    Mixer::Pause();
    AGG::PlayMusic( MUS::MAINMENU, true, true );

    Settings & conf = Settings::Get();

    conf.SetGameType( TYPE_MENU );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    // image background
    fheroes2::drawMainMenuScreen();
    if ( isFirstGameRun ) {
        Dialog::Message( _( "Greetings!" ), _( "Welcome to Free Heroes of Might and Magic II! Before starting the game please choose game resolution." ), Font::BIG,
                         Dialog::OK );

        bool isResolutionChanged = Dialog::SelectResolution();
        if ( isResolutionChanged ) {
            fheroes2::drawMainMenuScreen();
        }

        fheroes2::Text header( _( "Please Remember" ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } );

        fheroes2::MultiFontText body;
        body.add( { _( "You can always change game resolution by clicking on the " ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } } );
        body.add( { _( "door" ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } } );
        body.add( { _( " on the left side of main menu.\n\nTo switch between windowed and full screen modes\npress " ),
                    { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } } );
        body.add( { _( "F4" ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } } );
        body.add( { _( " key on the keyboard.\n\nEnjoy the game!" ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } } );

        fheroes2::showMessage( header, body, Dialog::OK );

        conf.resetFirstGameRun();
        conf.Save( "fheroes2.cfg" );
    }

    LocalEvent & le = LocalEvent::Get();

    fheroes2::Button buttonNewGame( 0, 0, ICN::BTNSHNGL, NEWGAME_DEFAULT, NEWGAME_DEFAULT + 2 );
    fheroes2::Button buttonLoadGame( 0, 0, ICN::BTNSHNGL, LOADGAME_DEFAULT, LOADGAME_DEFAULT + 2 );
    fheroes2::Button buttonHighScores( 0, 0, ICN::BTNSHNGL, HIGHSCORES_DEFAULT, HIGHSCORES_DEFAULT + 2 );
    fheroes2::Button buttonCredits( 0, 0, ICN::BTNSHNGL, CREDITS_DEFAULT, CREDITS_DEFAULT + 2 );
    fheroes2::Button buttonQuit( 0, 0, ICN::BTNSHNGL, QUIT_DEFAULT, QUIT_DEFAULT + 2 );

    const fheroes2::Sprite & lantern10 = fheroes2::AGG::GetICN( ICN::SHNGANIM, 0 );
    fheroes2::Blit( lantern10, display, lantern10.x(), lantern10.y() );

    const fheroes2::Sprite & lantern11 = fheroes2::AGG::GetICN( ICN::SHNGANIM, ICN::AnimationFrame( ICN::SHNGANIM, 0, 0 ) );
    fheroes2::Blit( lantern11, display, lantern11.x(), lantern11.y() );

    buttonNewGame.draw();
    buttonLoadGame.draw();
    buttonHighScores.draw();
    buttonCredits.draw();
    buttonQuit.draw();

    display.render();

    const double scaleX = static_cast<double>( display.width() ) / fheroes2::Display::DEFAULT_WIDTH;
    const double scaleY = static_cast<double>( display.height() ) / fheroes2::Display::DEFAULT_HEIGHT;
    const fheroes2::Rect resolutionArea( static_cast<int32_t>( 63 * scaleX ), static_cast<int32_t>( 202 * scaleY ), static_cast<int32_t>( 90 * scaleX ),
                                         static_cast<int32_t>( 160 * scaleY ) );

    u32 lantern_frame = 0;

    std::vector<ButtonInfo> buttons{ { NEWGAME_DEFAULT, buttonNewGame, false, false },
                                     { LOADGAME_DEFAULT, buttonLoadGame, false, false },
                                     { HIGHSCORES_DEFAULT, buttonHighScores, false, false },
                                     { CREDITS_DEFAULT, buttonCredits, false, false },
                                     { QUIT_DEFAULT, buttonQuit, false, false } };

    for ( size_t i = 0; le.MouseMotion() && i < buttons.size(); ++i ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::BTNSHNGL, buttons[i].frame );
        fheroes2::Blit( sprite, display, sprite.x(), sprite.y() );
    }

    fheroes2::Sprite highlightDoor = fheroes2::AGG::GetICN( ICN::SHNGANIM, 18 );
    fheroes2::ApplyPalette( highlightDoor, 8 );

    // mainmenu loop
    while ( 1 ) {
        if ( !le.HandleEvents( true, true ) ) {
            if ( Interface::Basic::EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                // if ( conf.ExtGameUseFade() )
                //    display.Fade();
                break;
            }
            else {
                continue;
            }
        }

        bool redrawScreen = false;

        for ( size_t i = 0; i < buttons.size(); ++i ) {
            buttons[i].wasOver = buttons[i].isOver;

            if ( le.MousePressLeft( buttons[i].button.area() ) ) {
                buttons[i].button.drawOnPress();
            }
            else {
                buttons[i].button.drawOnRelease();
            }

            buttons[i].isOver = le.MouseCursor( buttons[i].button.area() );

            if ( buttons[i].isOver != buttons[i].wasOver ) {
                u32 frame = buttons[i].frame;

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

        if ( HotKeyPressEvent( EVENT_BUTTON_NEWGAME ) || le.MouseClickLeft( buttonNewGame.area() ) )
            return fheroes2::GameMode::NEW_GAME;
        else if ( HotKeyPressEvent( EVENT_BUTTON_LOADGAME ) || le.MouseClickLeft( buttonLoadGame.area() ) )
            return fheroes2::GameMode::LOAD_GAME;
        else if ( HotKeyPressEvent( EVENT_BUTTON_HIGHSCORES ) || le.MouseClickLeft( buttonHighScores.area() ) )
            return fheroes2::GameMode::HIGHSCORES;
        else if ( HotKeyPressEvent( EVENT_BUTTON_CREDITS ) || le.MouseClickLeft( buttonCredits.area() ) )
            return fheroes2::GameMode::CREDITS;
        else if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonQuit.area() ) ) {
            if ( Interface::Basic::EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                // if ( conf.ExtGameUseFade() )
                //     display.Fade();
                return fheroes2::GameMode::QUIT_GAME;
            }
        }
        else if ( le.MouseClickLeft( resolutionArea ) ) {
            if ( Dialog::SelectResolution() ) {
                conf.Save( "fheroes2.cfg" );
                // force interface to reset area and positions
                Interface::Basic::Get().Reset();
                return fheroes2::GameMode::MAIN_MENU;
            }
        }

        // right info
        if ( le.MousePressRight( buttonQuit.area() ) )
            Dialog::Message( _( "Quit" ), _( "Quit Heroes of Might and Magic and return to the operating system." ), Font::BIG );
        else if ( le.MousePressRight( buttonLoadGame.area() ) )
            Dialog::Message( _( "Load Game" ), _( "Load a previously saved game." ), Font::BIG );
        else if ( le.MousePressRight( buttonCredits.area() ) )
            Dialog::Message( _( "Credits" ), _( "View the credits screen." ), Font::BIG );
        else if ( le.MousePressRight( buttonHighScores.area() ) )
            Dialog::Message( _( "High Scores" ), _( "View the high score screen." ), Font::BIG );
        else if ( le.MousePressRight( buttonNewGame.area() ) )
            Dialog::Message( _( "New Game" ), _( "Start a single or multi-player game." ), Font::BIG );
        else if ( le.MousePressRight( resolutionArea ) )
            Dialog::Message( _( "Select Game Resolution" ), _( "Change resolution of the game." ), Font::BIG );

        if ( validateAnimationDelay( MAIN_MENU_DELAY ) ) {
            const fheroes2::Sprite & lantern12 = fheroes2::AGG::GetICN( ICN::SHNGANIM, ICN::AnimationFrame( ICN::SHNGANIM, 0, lantern_frame ) );
            ++lantern_frame;
            fheroes2::Blit( lantern12, display, lantern12.x(), lantern12.y() );
            if ( le.MouseCursor( resolutionArea ) ) {
                const int32_t offsetY = static_cast<int32_t>( 55 * scaleY );
                fheroes2::Blit( highlightDoor, 0, offsetY, display, highlightDoor.x(), highlightDoor.y() + offsetY, highlightDoor.width(), highlightDoor.height() );
            }

            display.render();
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}
