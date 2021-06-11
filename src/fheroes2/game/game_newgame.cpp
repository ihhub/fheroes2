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

#include <cassert>

#include "agg.h"
#include "agg_image.h"
#include "audio_mixer.h"
#include "audio_music.h"
#include "campaign_savedata.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_mainmenu_ui.h"
#include "game_video.h"
#include "gamedefs.h"
#include "icn.h"
#include "mus.h"
#include "settings.h"
#include "smk_decoder.h"
#include "text.h"
#include "ui_button.h"
#include "ui_tool.h"
#include "world.h"

namespace
{
    const int32_t buttonYStep = 66;

    // Draw button panel and return the position for a button.
    fheroes2::Point drawButtonPanel()
    {
        const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HEROES, 0 );
        const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REDBACK, 0 );

        const uint32_t panelOffset = fheroes2::Display::DEFAULT_HEIGHT - panel.height();
        const uint32_t panelXPos = back.width() - ( panel.width() + panelOffset );
        fheroes2::Blit( panel, fheroes2::Display::instance(), panelXPos, panelOffset );

        const int32_t buttonMiddlePos = panelXPos + SHADOWWIDTH + ( panel.width() - SHADOWWIDTH ) / 2;

        const fheroes2::Sprite & buttonSample = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 0 );
        const int32_t buttonWidth = buttonSample.width();
        const int32_t buttonXPos = buttonMiddlePos - buttonWidth / 2 - 3; // 3 is button shadow
        const int32_t buttonYPos = 46;

        return fheroes2::Point( buttonXPos, buttonYPos );
    }
}

fheroes2::GameMode Game::NewStandard()
{
    Settings & conf = Settings::Get();
    if ( conf.isCampaignGameType() )
        conf.SetCurrentFileInfo( Maps::FileInfo() );
    conf.SetGameType( Game::TYPE_STANDARD );
    conf.SetPreferablyCountPlayers( 0 );
    return fheroes2::GameMode::SELECT_SCENARIO;
}

fheroes2::GameMode Game::NewBattleOnly()
{
    Settings & conf = Settings::Get();
    conf.SetGameType( Game::TYPE_BATTLEONLY );

    return fheroes2::GameMode::NEW_MULTI;
}

fheroes2::GameMode Game::NewHotSeat()
{
    Settings & conf = Settings::Get();
    if ( conf.isCampaignGameType() )
        conf.SetCurrentFileInfo( Maps::FileInfo() );

    if ( conf.IsGameType( Game::TYPE_BATTLEONLY ) ) {
        conf.SetPreferablyCountPlayers( 2 );
        world.NewMaps( 10, 10 );
        return StartBattleOnly();
    }
    else {
        conf.SetGameType( Game::TYPE_HOTSEAT );
        const u32 select = SelectCountPlayers();
        if ( select ) {
            conf.SetPreferablyCountPlayers( select );
            return fheroes2::GameMode::SELECT_SCENARIO;
        }
    }
    return fheroes2::GameMode::MAIN_MENU;
}

fheroes2::GameMode Game::CampaignSelection()
{
    if ( !isPriceOfLoyaltyCampaignPresent() ) {
        return fheroes2::GameMode::NEW_SUCCESSION_WARS_CAMPAIGN;
    }

    fheroes2::drawMainMenuScreen();
    const fheroes2::Point buttonPos = drawButtonPanel();

    fheroes2::Button buttonSuccessionWars( buttonPos.x, buttonPos.y, ICN::X_LOADCM, 0, 1 );
    fheroes2::Button buttonPriceOfLoyalty( buttonPos.x, buttonPos.y + buttonYStep * 1, ICN::X_LOADCM, 2, 3 );
    fheroes2::Button buttonCancelGame( buttonPos.x, buttonPos.y + buttonYStep * 5, ICN::BTNMP, 8, 9 );

    buttonSuccessionWars.draw();
    buttonPriceOfLoyalty.draw();
    buttonCancelGame.draw();

    fheroes2::Display::instance().render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonSuccessionWars.area() ) ? buttonSuccessionWars.drawOnPress() : buttonSuccessionWars.drawOnRelease();
        le.MousePressLeft( buttonPriceOfLoyalty.area() ) ? buttonPriceOfLoyalty.drawOnPress() : buttonPriceOfLoyalty.drawOnRelease();
        le.MousePressLeft( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        if ( le.MouseClickLeft( buttonSuccessionWars.area() ) )
            return fheroes2::GameMode::NEW_SUCCESSION_WARS_CAMPAIGN;
        if ( le.MouseClickLeft( buttonPriceOfLoyalty.area() ) )
            return fheroes2::GameMode::NEW_PRICE_OF_LOYALTY_CAMPAIGN;
        if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancelGame.area() ) )
            return fheroes2::GameMode::MAIN_MENU;

        if ( le.MousePressRight( buttonSuccessionWars.area() ) ) {
            Dialog::Message( _( "Original Campaign" ), _( "Either Roland's or Archibald's campaign from the original Heroes of Might and Magic II." ), Font::BIG );
        }
        if ( le.MousePressRight( buttonPriceOfLoyalty.area() ) ) {
            Dialog::Message( _( "Expansion Campaign" ), _( "One of the four new campaigns from the Price of Loyalty expansion set." ), Font::BIG );
        }
        if ( le.MousePressRight( buttonCancelGame.area() ) ) {
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}

fheroes2::GameMode Game::NewSuccessionWarsCampaign()
{
    Settings::Get().SetGameType( Game::TYPE_CAMPAIGN );

    Mixer::Pause();
    Music::Pause();

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point roiOffset( ( display.width() - display.DEFAULT_WIDTH ) / 2, ( display.height() - display.DEFAULT_HEIGHT ) / 2 );

    display.fill( 0 );

    const Text loadingScreen( "Loading video. Please wait...", Font::BIG );
    loadingScreen.Blit( display.width() / 2 - loadingScreen.w() / 2, display.height() / 2 - loadingScreen.h() / 2 );
    display.render();

    std::vector<fheroes2::Rect> campaignRoi;
    campaignRoi.emplace_back( 382 + roiOffset.x, 58 + roiOffset.y, 222, 298 );
    campaignRoi.emplace_back( 30 + roiOffset.x, 59 + roiOffset.y, 224, 297 );

    // Reset all sound and music before playing videos
    AGG::ResetMixer();

    const CursorRestorer cursorRestorer( false, Cursor::POINTER );

    Video::ShowVideo( "INTRO.SMK", Video::VideoAction::PLAY_TILL_VIDEO_END );

    AGG::ResetMixer();
    Video::ShowVideo( "CHOOSEW.SMK", Video::VideoAction::IGNORE_VIDEO );
    const int chosenCampaign = Video::ShowVideo( "CHOOSE.SMK", Video::VideoAction::LOOP_VIDEO, campaignRoi );

    Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
    campaignSaveData.reset();
    campaignSaveData.setCampaignID( chosenCampaign );
    campaignSaveData.setCurrentScenarioID( 0 );

    AGG::PlayMusic( MUS::VICTORY, true, true );

    return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
}

fheroes2::GameMode Game::NewPriceOfLoyaltyCampaign()
{
    // TODO: Properly choose the campaign instead of this hackish way
    Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
    campaignSaveData.reset();
    campaignSaveData.setCampaignID( Campaign::PRICE_OF_LOYALTY_CAMPAIGN );
    campaignSaveData.setCurrentScenarioID( 0 );

    std::string videoPath;
    if ( !Video::isVideoFile( "IVYPOL.SMK", videoPath ) ) {
        // File doesn't exist. Fallback to PoL campaign.
        return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
    }

    SMKVideoSequence video( videoPath );
    if ( video.frameCount() < 1 ) {
        // File is incorrect. Fallback to PoL campaign.
        return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
    }

    const fheroes2::ScreenPaletteRestorer screenRestorer;

    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const std::vector<uint8_t> palette = video.getCurrentPalette();
    screenRestorer.changePalette( palette.data() );

    Cursor::Get().setVideoPlaybackCursor();

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point roiOffset( ( display.width() - display.DEFAULT_WIDTH ) / 2, ( display.height() - display.DEFAULT_HEIGHT ) / 2 );

    display.fill( 0 );

    const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::X_IVY, 1 );
    fheroes2::Blit( background, 0, 0, display, roiOffset.x, roiOffset.y, background.width(), background.height() );

    const fheroes2::Sprite & campaignChoice = fheroes2::AGG::GetICN( ICN::X_IVY, 0 );
    fheroes2::Blit( campaignChoice, 0, 0, display, roiOffset.x + campaignChoice.x(), roiOffset.y + campaignChoice.y(), campaignChoice.width(), campaignChoice.height() );

    display.render();

    const fheroes2::Rect priceOfLoyaltyRoi( roiOffset.x + 192, roiOffset.y + 23, 248, 163 );
    const fheroes2::Rect voyageHomeRoi( roiOffset.x + 19, roiOffset.y + 120, 166, 193 );
    const fheroes2::Rect wizardsIsleRoi( roiOffset.x + 450, roiOffset.y + 120, 166, 193 );
    const fheroes2::Rect descendantsRoi( roiOffset.x + 192, roiOffset.y + 240, 248, 163 );

    fheroes2::GameMode gameChoice = fheroes2::GameMode::NEW_CAMPAIGN_SELECTION;

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        if ( le.MouseClickLeft( priceOfLoyaltyRoi ) ) {
            campaignSaveData.setCampaignID( Campaign::PRICE_OF_LOYALTY_CAMPAIGN );
            gameChoice = fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            break;
        }
        else if ( le.MouseClickLeft( voyageHomeRoi ) ) {
            campaignSaveData.setCampaignID( Campaign::VOYAGE_HOME_CAMPAIGN );
            gameChoice = fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            break;
        }
        else if ( le.MouseClickLeft( wizardsIsleRoi ) ) {
            campaignSaveData.setCampaignID( Campaign::WIZARDS_ISLE_CAMPAIGN );
            gameChoice = fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            break;
        }
        else if ( le.MouseClickLeft( descendantsRoi ) ) {
            campaignSaveData.setCampaignID( Campaign::DESCENDANTS_CAMPAIGN );
            gameChoice = fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            break;
        }
    }

    display.fill( 0 );

    return gameChoice;
}

#ifdef NETWORK_ENABLE
fheroes2::GameMode Game::NewNetwork()
{
    Settings & conf = Settings::Get();
    conf.SetGameType( conf.GameType() | Game::TYPE_NETWORK );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawMainMenuScreen();
    const fheroes2::Point buttonPos = drawButtonPanel();

    fheroes2::Button buttonHost( buttonPos.x, buttonPos.y, ICN::BTNNET, 0, 1 );
    fheroes2::Button buttonGuest( buttonPos.x, buttonPos.y + buttonYStep, ICN::BTNNET, 2, 3 );
    fheroes2::Button buttonCancelGame( buttonPos.x, buttonPos.y + buttonYStep * 2, ICN::BTNMP, 8, 9 );

    buttonHost.draw();
    buttonGuest.draw();
    buttonCancelGame.draw();

    fheroes2::Display::instance().render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonHost.area() ) ? buttonHost.drawOnPress() : buttonHost.drawOnRelease();
        le.MousePressLeft( buttonGuest.area() ) ? buttonGuest.drawOnPress() : buttonGuest.drawOnRelease();
        le.MousePressLeft( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancelGame.area() ) )
            return fheroes2::GameMode::MAIN_MENU;

        // right info
        if ( le.MousePressRight( buttonHost.area() ) )
            Dialog::Message( _( "Host" ), _( "The host sets up the game options. There can only be one host per network game." ), Font::BIG );
        if ( le.MousePressRight( buttonGuest.area() ) )
            Dialog::Message( _( "Guest" ),
                             _( "The guest waits for the host to set up the game, then is automatically added in. There can be multiple guests for TCP/IP games." ),
                             Font::BIG );
        if ( le.MousePressRight( buttonCancelGame.area() ) )
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );
    }

    return fheroes2::GameMode::MAIN_MENU;
}
#endif

fheroes2::GameMode Game::NewGame()
{
    Mixer::Pause();
    AGG::PlayMusic( MUS::MAINMENU, true, true );
    Settings & conf = Settings::Get();

    // reset last save name
    Game::SetLastSavename( "" );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    // load game settings
    conf.BinaryLoad();

    fheroes2::drawMainMenuScreen();
    const fheroes2::Point buttonPos = drawButtonPanel();

    fheroes2::Button buttonStandartGame( buttonPos.x, buttonPos.y, ICN::BTNNEWGM, 0, 1 );
    fheroes2::Button buttonCampainGame( buttonPos.x, buttonPos.y + buttonYStep * 1, ICN::BTNNEWGM, 2, 3 );
    fheroes2::Button buttonMultiGame( buttonPos.x, buttonPos.y + buttonYStep * 2, ICN::BTNNEWGM, 4, 5 );
    fheroes2::Button buttonBattleGame( buttonPos.x, buttonPos.y + buttonYStep * 3, ICN::BTNBATTLEONLY, 0, 1 );
    fheroes2::Button buttonSettings( buttonPos.x, buttonPos.y + buttonYStep * 4, ICN::BTNDCCFG, 4, 5 );
    fheroes2::Button buttonCancelGame( buttonPos.x, buttonPos.y + buttonYStep * 5, ICN::BTNNEWGM, 6, 7 );

    if ( !isSuccessionWarsCampaignPresent() ) {
        buttonCampainGame.disable();
    }

    buttonStandartGame.draw();
    buttonCampainGame.draw();
    buttonMultiGame.draw();
    buttonBattleGame.draw();
    buttonSettings.draw();
    buttonCancelGame.draw();

    display.render();

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonStandartGame.area() ) ? buttonStandartGame.drawOnPress() : buttonStandartGame.drawOnRelease();

        if ( buttonCampainGame.isEnabled() ) {
            le.MousePressLeft( buttonCampainGame.area() ) ? buttonCampainGame.drawOnPress() : buttonCampainGame.drawOnRelease();
        }
        le.MousePressLeft( buttonMultiGame.area() ) ? buttonMultiGame.drawOnPress() : buttonMultiGame.drawOnRelease();
        le.MousePressLeft( buttonBattleGame.area() ) ? buttonBattleGame.drawOnPress() : buttonBattleGame.drawOnRelease();
        le.MousePressLeft( buttonSettings.area() ) ? buttonSettings.drawOnPress() : buttonSettings.drawOnRelease();
        le.MousePressLeft( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        if ( HotKeyPressEvent( EVENT_BUTTON_STANDARD ) || le.MouseClickLeft( buttonStandartGame.area() ) )
            return fheroes2::GameMode::NEW_STANDARD;
        if ( buttonCampainGame.isEnabled() && ( HotKeyPressEvent( EVENT_BUTTON_CAMPAIGN ) || le.MouseClickLeft( buttonCampainGame.area() ) ) )
            return fheroes2::GameMode::NEW_CAMPAIGN_SELECTION;
        if ( HotKeyPressEvent( EVENT_BUTTON_MULTI ) || le.MouseClickLeft( buttonMultiGame.area() ) )
            return fheroes2::GameMode::NEW_MULTI;
        if ( HotKeyPressEvent( EVENT_BUTTON_SETTINGS ) || le.MouseClickLeft( buttonSettings.area() ) ) {
            Dialog::ExtSettings( false );
            display.render();
        }
        if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancelGame.area() ) )
            return fheroes2::GameMode::MAIN_MENU;

        if ( HotKeyPressEvent( EVENT_BUTTON_BATTLEONLY ) || le.MouseClickLeft( buttonBattleGame.area() ) )
            return fheroes2::GameMode::NEW_BATTLE_ONLY;

        if ( le.MousePressRight( buttonStandartGame.area() ) )
            Dialog::Message( _( "Standard Game" ), _( "A single player game playing out a single map." ), Font::BIG );
        else if ( le.MousePressRight( buttonCampainGame.area() ) )
            Dialog::Message( _( "Campaign Game" ), _( "A single player game playing through a series of maps." ), Font::BIG );
        else if ( le.MousePressRight( buttonMultiGame.area() ) )
            Dialog::Message( _( "Multi-Player Game" ), _( "A multi-player game, with several human players completing against each other on a single map." ), Font::BIG );
        else if ( le.MousePressRight( buttonBattleGame.area() ) )
            Dialog::Message( _( "Battle Only" ), _( "Setup and play a battle without loading any map." ), Font::BIG );
        else if ( le.MousePressRight( buttonSettings.area() ) )
            Dialog::Message( _( "Settings" ), _( "Experimental game settings." ), Font::BIG );
        else if ( le.MousePressRight( buttonCancelGame.area() ) )
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );
    }

    return fheroes2::GameMode::QUIT_GAME;
}

fheroes2::GameMode Game::NewMulti()
{
    Settings & conf = Settings::Get();

    if ( !( conf.IsGameType( Game::TYPE_BATTLEONLY ) ) )
        conf.SetGameType( Game::TYPE_STANDARD );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawMainMenuScreen();
    const fheroes2::Point buttonPos = drawButtonPanel();

    fheroes2::Button buttonHotSeat( buttonPos.x, buttonPos.y, ICN::BTNMP, 0, 1 );
    fheroes2::Button buttonNetwork( buttonPos.x, buttonPos.y + buttonYStep * 1, ICN::BTNMP, 2, 3 );
    fheroes2::Button buttonCancelGame( buttonPos.x, buttonPos.y + buttonYStep * 5, ICN::BTNMP, 8, 9 );

    buttonHotSeat.draw();
    buttonCancelGame.draw();
    buttonNetwork.disable();

    fheroes2::Display::instance().render();

    LocalEvent & le = LocalEvent::Get();
    // newgame loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonHotSeat.area() ) ? buttonHotSeat.drawOnPress() : buttonHotSeat.drawOnRelease();
        le.MousePressLeft( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        if ( le.MouseClickLeft( buttonHotSeat.area() ) || HotKeyPressEvent( EVENT_BUTTON_HOTSEAT ) )
            return fheroes2::GameMode::NEW_HOT_SEAT;
        if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancelGame.area() ) )
            return fheroes2::GameMode::MAIN_MENU;

        // right info
        if ( le.MousePressRight( buttonHotSeat.area() ) )
            Dialog::Message( _( "Hot Seat" ),
                             _( "Play a Hot Seat game, where 2 to 4 players play around the same computer, switching into the 'Hot Seat' when it is their turn." ),
                             Font::BIG );
        if ( le.MousePressRight( buttonCancelGame.area() ) )
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );

#ifdef NETWORK_ENABLE
        if ( buttonNetwork.isEnabled() ) {
            le.MousePressLeft( buttonNetwork.area() ) ? buttonNetwork.drawOnPress() : buttonNetwork.drawOnRelease();
            if ( le.MouseClickLeft( buttonNetwork.area() ) || HotKeyPressEvent( EVENT_BUTTON_NETWORK ) )
                return fheroes2::GameMode::NEWNETWORK;
            if ( le.MousePressRight( buttonNetwork.area() ) )
                Dialog::Message( _( "Network" ), _( "Play a network game, where 2 players use their own computers connected through a LAN (Local Area Network)." ),
                                 Font::BIG );
        }
#endif
    }

    return fheroes2::GameMode::QUIT_GAME;
}

u32 Game::SelectCountPlayers( void )
{
    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawMainMenuScreen();
    const fheroes2::Point buttonPos = drawButtonPanel();

    fheroes2::Button button2Players( buttonPos.x, buttonPos.y, ICN::BTNHOTST, 0, 1 );
    fheroes2::Button button3Players( buttonPos.x, buttonPos.y + buttonYStep * 1, ICN::BTNHOTST, 2, 3 );
    fheroes2::Button button4Players( buttonPos.x, buttonPos.y + buttonYStep * 2, ICN::BTNHOTST, 4, 5 );
    fheroes2::Button button5Players( buttonPos.x, buttonPos.y + buttonYStep * 3, ICN::BTNHOTST, 6, 7 );
    fheroes2::Button button6Players( buttonPos.x, buttonPos.y + buttonYStep * 4, ICN::BTNHOTST, 8, 9 );
    fheroes2::Button buttonCancel( buttonPos.x, buttonPos.y + buttonYStep * 5, ICN::BTNNEWGM, 6, 7 );

    button2Players.draw();
    button3Players.draw();
    button4Players.draw();
    button5Players.draw();
    button6Players.draw();
    buttonCancel.draw();

    fheroes2::Display::instance().render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.MousePressLeft( button2Players.area() ) ? button2Players.drawOnPress() : button2Players.drawOnRelease();
        le.MousePressLeft( button3Players.area() ) ? button3Players.drawOnPress() : button3Players.drawOnRelease();
        le.MousePressLeft( button4Players.area() ) ? button4Players.drawOnPress() : button4Players.drawOnRelease();
        le.MousePressLeft( button5Players.area() ) ? button5Players.drawOnPress() : button5Players.drawOnRelease();
        le.MousePressLeft( button6Players.area() ) ? button6Players.drawOnPress() : button6Players.drawOnRelease();

        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        if ( le.MouseClickLeft( button2Players.area() ) || le.KeyPress( KEY_2 ) )
            return 2;
        if ( le.MouseClickLeft( button3Players.area() ) || le.KeyPress( KEY_3 ) )
            return 3;
        if ( le.MouseClickLeft( button4Players.area() ) || le.KeyPress( KEY_4 ) )
            return 4;
        if ( le.MouseClickLeft( button5Players.area() ) || le.KeyPress( KEY_5 ) )
            return 5;
        if ( le.MouseClickLeft( button6Players.area() ) || le.KeyPress( KEY_6 ) )
            return 6;

        if ( HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancel.area() ) )
            return 0;

        // right info
        if ( le.MousePressRight( button2Players.area() ) )
            Dialog::Message( _( "2 Players" ), _( "Play with 2 human players, and optionally, up, to 4 additional computer players." ), Font::BIG );
        if ( le.MousePressRight( button3Players.area() ) )
            Dialog::Message( _( "3 Players" ), _( "Play with 3 human players, and optionally, up, to 3 additional computer players." ), Font::BIG );
        if ( le.MousePressRight( button4Players.area() ) )
            Dialog::Message( _( "4 Players" ), _( "Play with 4 human players, and optionally, up, to 2 additional computer players." ), Font::BIG );
        if ( le.MousePressRight( button5Players.area() ) )
            Dialog::Message( _( "5 Players" ), _( "Play with 5 human players, and optionally, up, to 1 additional computer players." ), Font::BIG );
        if ( le.MousePressRight( button6Players.area() ) )
            Dialog::Message( _( "6 Players" ), _( "Play with 6 human players." ), Font::BIG );
        if ( le.MousePressRight( buttonCancel.area() ) )
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );
    }

    return 0;
}
