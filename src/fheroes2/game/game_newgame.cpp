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

#include "game.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "agg_image.h"
#include "audio.h"
#include "audio_manager.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_game_settings.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_io.h"
#include "game_mainmenu_ui.h"
#include "game_mode.h"
#include "game_video.h"
#include "game_video_type.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "logging.h"
#include "math_base.h"
#include "mus.h"
#include "screen.h"
#include "settings.h"
#include "smk_decoder.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"

namespace
{
    const int32_t buttonYStep = 66;

    std::unique_ptr<SMKVideoSequence> getVideo( const std::string & fileName )
    {
        std::string videoPath;
        if ( !Video::getVideoFilePath( fileName, videoPath ) ) {
            return nullptr;
        }

        std::unique_ptr<SMKVideoSequence> video = std::make_unique<SMKVideoSequence>( videoPath );
        if ( video->frameCount() < 1 ) {
            return nullptr;
        }

        return video;
    }

    void outputNewMenuInTextSupportMode()
    {
        START_TEXT_SUPPORT_MODE

        COUT( "New Game\n" )

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_STANDARD ) << " to choose Standard Game." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_CAMPAIGN ) << " to choose Campaign Game." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_MULTI ) << " to choose Multiplayer Game." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_SETTINGS ) << " to open Game Settings." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL ) << " to come back to Main Menu." )
    }

    void outputNewCampaignSelectionInTextSupportMode()
    {
        START_TEXT_SUPPORT_MODE

        COUT( "New Campaign\n" )

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_NEW_ORIGINAL_CAMPAIGN ) << " to choose The Original Campaign." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_NEW_EXPANSION_CAMPAIGN ) << " to choose The Expansion Campaign." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL ) << " to come back to Main Menu." )
    }

    void outputNewSuccessionWarsCampaignInTextSupportMode()
    {
        START_TEXT_SUPPORT_MODE

        COUT( "Choose your Lord:\n" )

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_ROLAND ) << " to choose Roland Campaign." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_ARCHIBALD ) << " to choose Archibald Campaign." )
    }

    void outputPriceOfLoyaltyCampaignInTextSupportMode()
    {
        START_TEXT_SUPPORT_MODE

        COUT( "Select your campaign\n" )

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_PRICE_OF_LOYALTY ) << " to choose The Price of Loyalty Campaign." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_VOYAGE_HOME ) << " to choose Voyage Home Campaign." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_WIZARDS_ISLE ) << " to choose Wizard's Isle Campaign." )
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_DESCENDANTS ) << " to choose Descendants Campaign." )
    }

    void showMissingVideoFilesWindow()
    {
        fheroes2::showStandardTextMessage( _( "Warning!" ),
                                           _( "The required video files for the campaign selection window are missing. "
                                              "Please make sure that all necessary files are present in the system." ),
                                           Dialog::OK );
    }
}

fheroes2::GameMode Game::NewStandard()
{
    Settings & conf = Settings::Get();
    if ( conf.isCampaignGameType() )
        conf.SetCurrentFileInfo( {} );
    conf.SetGameType( Game::TYPE_STANDARD );
    return fheroes2::GameMode::SELECT_SCENARIO_ONE_HUMAN_PLAYER;
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
        conf.SetCurrentFileInfo( {} );

    if ( conf.IsGameType( Game::TYPE_BATTLEONLY ) ) {
        // Redraw the main menu screen without multiplayer sub-menu to show it after the battle using screen restorer.
        fheroes2::drawMainMenuScreen();

        return StartBattleOnly();
    }
    else {
        conf.SetGameType( Game::TYPE_HOTSEAT );
        const uint8_t humanPlayerCount = SelectCountPlayers();

        switch ( humanPlayerCount ) {
        case 2:
            return fheroes2::GameMode::SELECT_SCENARIO_TWO_HUMAN_PLAYERS;
        case 3:
            return fheroes2::GameMode::SELECT_SCENARIO_THREE_HUMAN_PLAYERS;
        case 4:
            return fheroes2::GameMode::SELECT_SCENARIO_FOUR_HUMAN_PLAYERS;
        case 5:
            return fheroes2::GameMode::SELECT_SCENARIO_FIVE_HUMAN_PLAYERS;
        case 6:
            return fheroes2::GameMode::SELECT_SCENARIO_SIX_HUMAN_PLAYERS;
        default:
            break;
        }
    }
    return fheroes2::GameMode::MAIN_MENU;
}

fheroes2::GameMode Game::CampaignSelection()
{
    if ( !isPriceOfLoyaltyCampaignPresent() ) {
        return fheroes2::GameMode::NEW_SUCCESSION_WARS_CAMPAIGN;
    }

    outputNewCampaignSelectionInTextSupportMode();

    fheroes2::drawMainMenuScreen();
    const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

    fheroes2::Button buttonSuccessionWars( buttonPos.x, buttonPos.y, ICN::BUTTON_ORIGINAL_CAMPAIGN, 0, 1 );
    fheroes2::Button buttonPriceOfLoyalty( buttonPos.x, buttonPos.y + buttonYStep * 1, ICN::BUTTON_EXPANSION_CAMPAIGN, 0, 1 );
    fheroes2::Button buttonCancelGame( buttonPos.x, buttonPos.y + buttonYStep * 5, ICN::BUTTON_LARGE_CANCEL, 0, 1 );

    buttonSuccessionWars.draw();
    buttonPriceOfLoyalty.draw();
    buttonCancelGame.draw();

    fheroes2::Display::instance().render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonSuccessionWars.area() ) ? buttonSuccessionWars.drawOnPress() : buttonSuccessionWars.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonPriceOfLoyalty.area() ) ? buttonPriceOfLoyalty.drawOnPress() : buttonPriceOfLoyalty.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        if ( le.MouseClickLeft( buttonSuccessionWars.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_NEW_ORIGINAL_CAMPAIGN ) ) {
            return fheroes2::GameMode::NEW_SUCCESSION_WARS_CAMPAIGN;
        }
        if ( le.MouseClickLeft( buttonPriceOfLoyalty.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_NEW_EXPANSION_CAMPAIGN ) ) {
            return fheroes2::GameMode::NEW_PRICE_OF_LOYALTY_CAMPAIGN;
        }
        if ( HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancelGame.area() ) ) {
            return fheroes2::GameMode::MAIN_MENU;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonSuccessionWars.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Original Campaign" ), _( "Either Roland's or Archibald's campaign from the original Heroes of Might and Magic II." ),
                                               Dialog::ZERO );
        }
        if ( le.isMouseRightButtonPressedInArea( buttonPriceOfLoyalty.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Expansion Campaign" ), _( "One of the four new campaigns from the Price of Loyalty expansion set." ), Dialog::ZERO );
        }
        if ( le.isMouseRightButtonPressedInArea( buttonCancelGame.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the main menu." ), Dialog::ZERO );
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}

fheroes2::GameMode Game::NewSuccessionWarsCampaign()
{
    Settings::Get().SetGameType( Game::TYPE_CAMPAIGN );

    // Reset all sound and music before playing videos
    AudioManager::ResetAudio();

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point roiOffset( ( display.width() - display.DEFAULT_WIDTH ) / 2, ( display.height() - display.DEFAULT_HEIGHT ) / 2 );

    // Fade-out screen before playing video.
    fheroes2::fadeOutDisplay();

    const fheroes2::Text loadingScreen( _( "Loading video. Please wait..." ), fheroes2::FontType::normalWhite() );
    loadingScreen.draw( display.width() / 2 - loadingScreen.width() / 2, display.height() / 2 - loadingScreen.height() / 2 + 2, display );
    display.render();

    Video::ShowVideo( "INTRO.SMK", Video::VideoAction::PLAY_TILL_VIDEO_END );

    Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
    campaignSaveData.reset();

    std::unique_ptr<SMKVideoSequence> video = getVideo( "CHOOSE.SMK" );
    if ( !video ) {
        showMissingVideoFilesWindow();
        campaignSaveData.setCurrentScenarioInfo( { Campaign::ROLAND_CAMPAIGN, 0 } );
        return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
    }

    const std::array<fheroes2::Rect, 2> campaignRoi{ fheroes2::Rect( 382 + roiOffset.x, 58 + roiOffset.y, 222, 298 ),
                                                     fheroes2::Rect( 30 + roiOffset.x, 59 + roiOffset.y, 224, 297 ) };

    const uint64_t customDelay = static_cast<uint64_t>( std::lround( 1000.0 / video->fps() ) );

    outputNewSuccessionWarsCampaignInTextSupportMode();

    AudioManager::ResetAudio();
    Video::ShowVideo( "CHOOSEW.SMK", Video::VideoAction::IGNORE_VIDEO );

    const fheroes2::ScreenPaletteRestorer screenRestorer;

    std::vector<uint8_t> palette = video->getCurrentPalette();
    screenRestorer.changePalette( palette.data() );

    const CursorRestorer cursorRestorer( true, Cursor::POINTER );
    Cursor::Get().setVideoPlaybackCursor();

    // Immediately indicate that the delay has passed to render first frame immediately.
    Game::passCustomAnimationDelay( customDelay );
    // Make sure that the first run is passed immediately.
    assert( !Game::isCustomDelayNeeded( customDelay ) );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents( Game::isCustomDelayNeeded( customDelay ) ) ) {
        if ( le.MouseClickLeft( campaignRoi[0] ) || HotKeyPressEvent( HotKeyEvent::CAMPAIGN_ROLAND ) ) {
            campaignSaveData.setCurrentScenarioInfo( { Campaign::ROLAND_CAMPAIGN, 0 } );
            break;
        }
        if ( le.MouseClickLeft( campaignRoi[1] ) || HotKeyPressEvent( HotKeyEvent::CAMPAIGN_ARCHIBALD ) ) {
            campaignSaveData.setCurrentScenarioInfo( { Campaign::ARCHIBALD_CAMPAIGN, 0 } );
            break;
        }

        size_t highlightCampaignId = campaignRoi.size();

        for ( size_t i = 0; i < campaignRoi.size(); ++i ) {
            if ( le.isMouseCursorPosInArea( campaignRoi[i] ) ) {
                highlightCampaignId = i;
                break;
            }
        }

        if ( Game::validateCustomAnimationDelay( customDelay ) ) {
            fheroes2::Rect frameRoi( roiOffset.x, roiOffset.y, 0, 0 );
            video->getNextFrame( display, frameRoi.x, frameRoi.y, frameRoi.width, frameRoi.height, palette );

            if ( highlightCampaignId < campaignRoi.size() ) {
                const fheroes2::Rect & roi = campaignRoi[highlightCampaignId];
                fheroes2::DrawRect( display, roi, 51 );
                fheroes2::DrawRect( display, { roi.x - 1, roi.y - 1, roi.width + 2, roi.height + 2 }, 51 );
            }

            display.render( frameRoi );

            if ( video->frameCount() <= video->getCurrentFrame() ) {
                video->resetFrame();
            }
        }
    }

    screenRestorer.changePalette( nullptr );

    // Update the frame but do not render it.
    display.fill( 0 );
    display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );

    // Set the fade-in for the Campaign scenario info.
    setDisplayFadeIn();

    return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
}

fheroes2::GameMode Game::NewPriceOfLoyaltyCampaign()
{
    // TODO: Properly choose the campaign instead of this hackish way
    Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
    campaignSaveData.reset();
    campaignSaveData.setCurrentScenarioInfo( { Campaign::PRICE_OF_LOYALTY_CAMPAIGN, 0 } );

    std::array<std::unique_ptr<SMKVideoSequence>, 4> videos{ getVideo( "IVYPOL.SMK" ), getVideo( "IVYVOY.SMK" ), getVideo( "IVYWIZ.SMK" ), getVideo( "IVYDES.SMK" ) };

    if ( !videos[0] ) {
        // File doesn't exist. Fallback to PoL campaign.
        showMissingVideoFilesWindow();
        return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
    }

    // Fade-out display before playing video.
    fheroes2::fadeOutDisplay();

    outputPriceOfLoyaltyCampaignInTextSupportMode();

    const fheroes2::ScreenPaletteRestorer screenRestorer;

    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::vector<uint8_t> palette = videos[0]->getCurrentPalette();
    screenRestorer.changePalette( palette.data() );

    Cursor::Get().setVideoPlaybackCursor();

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point roiOffset( ( display.width() - display.DEFAULT_WIDTH ) / 2, ( display.height() - display.DEFAULT_HEIGHT ) / 2 );

    const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::X_IVY, 1 );
    fheroes2::Blit( background, 0, 0, display, roiOffset.x, roiOffset.y, background.width(), background.height() );

    const fheroes2::Sprite & campaignChoice = fheroes2::AGG::GetICN( ICN::X_IVY, 0 );
    fheroes2::Blit( campaignChoice, 0, 0, display, roiOffset.x + campaignChoice.x(), roiOffset.y + campaignChoice.y(), campaignChoice.width(), campaignChoice.height() );

    display.render();

    const std::array<fheroes2::Rect, 4> activeCampaignArea{ fheroes2::Rect( roiOffset.x + 192, roiOffset.y + 23, 248, 163 ),
                                                            fheroes2::Rect( roiOffset.x + 19, roiOffset.y + 120, 166, 193 ),
                                                            fheroes2::Rect( roiOffset.x + 450, roiOffset.y + 120, 166, 193 ),
                                                            fheroes2::Rect( roiOffset.x + 192, roiOffset.y + 240, 248, 163 ) };

    const std::array<fheroes2::Rect, 4> renderCampaignArea{ fheroes2::Rect( roiOffset.x + 214, roiOffset.y + 47, 248, 163 ),
                                                            fheroes2::Rect( roiOffset.x + 41, roiOffset.y + 140, 166, 193 ),
                                                            fheroes2::Rect( roiOffset.x + 472, roiOffset.y + 131, 166, 193 ),
                                                            fheroes2::Rect( roiOffset.x + 214, roiOffset.y + 273, 248, 163 ) };

    size_t highlightCampaignId = videos.size();

    fheroes2::GameMode gameChoice = fheroes2::GameMode::NEW_CAMPAIGN_SELECTION;
    uint64_t customDelay = 0;

    // Immediately indicate that the delay has passed to render first frame immediately.
    Game::passCustomAnimationDelay( customDelay );
    // Make sure that the first run is passed immediately.
    assert( !Game::isCustomDelayNeeded( customDelay ) );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents( highlightCampaignId < videos.size() ? Game::isCustomDelayNeeded( customDelay ) : true ) ) {
        if ( le.MouseClickLeft( activeCampaignArea[0] ) || HotKeyPressEvent( HotKeyEvent::CAMPAIGN_PRICE_OF_LOYALTY ) ) {
            campaignSaveData.setCurrentScenarioInfo( { Campaign::PRICE_OF_LOYALTY_CAMPAIGN, 0 } );
            gameChoice = fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            break;
        }
        if ( le.MouseClickLeft( activeCampaignArea[1] ) || HotKeyPressEvent( HotKeyEvent::CAMPAIGN_VOYAGE_HOME ) ) {
            campaignSaveData.setCurrentScenarioInfo( { Campaign::VOYAGE_HOME_CAMPAIGN, 0 } );
            gameChoice = fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            break;
        }
        if ( le.MouseClickLeft( activeCampaignArea[2] ) || HotKeyPressEvent( HotKeyEvent::CAMPAIGN_WIZARDS_ISLE ) ) {
            campaignSaveData.setCurrentScenarioInfo( { Campaign::WIZARDS_ISLE_CAMPAIGN, 0 } );
            gameChoice = fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            break;
        }
        if ( le.MouseClickLeft( activeCampaignArea[3] ) || HotKeyPressEvent( HotKeyEvent::CAMPAIGN_DESCENDANTS ) ) {
            campaignSaveData.setCurrentScenarioInfo( { Campaign::DESCENDANTS_CAMPAIGN, 0 } );
            gameChoice = fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            break;
        }

        const size_t beforeCampaignId = highlightCampaignId;

        highlightCampaignId = videos.size();

        for ( size_t i = 0; i < activeCampaignArea.size(); ++i ) {
            if ( le.isMouseCursorPosInArea( activeCampaignArea[i] ) && videos[i] ) {
                highlightCampaignId = i;
                customDelay = static_cast<uint64_t>( std::lround( 1000.0 / videos[highlightCampaignId]->fps() ) );
                break;
            }
        }

        if ( highlightCampaignId != beforeCampaignId ) {
            fheroes2::Blit( background, 0, 0, display, roiOffset.x, roiOffset.y, background.width(), background.height() );
            fheroes2::Blit( campaignChoice, 0, 0, display, roiOffset.x + campaignChoice.x(), roiOffset.y + campaignChoice.y(), campaignChoice.width(),
                            campaignChoice.height() );
            if ( highlightCampaignId >= videos.size() ) {
                display.render();
            }
        }

        if ( highlightCampaignId < videos.size() && Game::validateCustomAnimationDelay( customDelay ) ) {
            fheroes2::Rect frameRoi( renderCampaignArea[highlightCampaignId].x, renderCampaignArea[highlightCampaignId].y, 0, 0 );
            videos[highlightCampaignId]->getNextFrame( display, frameRoi.x, frameRoi.y, frameRoi.width, frameRoi.height, palette );

            fheroes2::Blit( background, frameRoi.x - roiOffset.x, frameRoi.y - roiOffset.y, display, frameRoi.x, frameRoi.y, frameRoi.width, frameRoi.height );

            display.render( frameRoi );

            if ( videos[highlightCampaignId]->frameCount() <= videos[highlightCampaignId]->getCurrentFrame() ) {
                videos[highlightCampaignId]->resetFrame();
            }
        }
    }

    // Update the frame but do not render it.
    display.fill( 0 );
    display.updateNextRenderRoi( { 0, 0, display.width(), display.height() } );

    // Set the fade-in for the Campaign scenario info.
    setDisplayFadeIn();

    return gameChoice;
}

fheroes2::GameMode Game::NewNetwork()
{
    Settings & conf = Settings::Get();
    conf.SetGameType( conf.GameType() | Game::TYPE_NETWORK );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawMainMenuScreen();
    const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

    fheroes2::Button buttonHost( buttonPos.x, buttonPos.y, ICN::BTNNET, 0, 1 );
    fheroes2::Button buttonGuest( buttonPos.x, buttonPos.y + buttonYStep, ICN::BTNNET, 2, 3 );
    fheroes2::Button buttonCancelGame( buttonPos.x, buttonPos.y + buttonYStep * 2, ICN::BTNMP, 8, 9 );

    buttonHost.draw();
    buttonGuest.draw();
    buttonCancelGame.draw();

    fheroes2::Display::instance().render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonHost.area() ) ? buttonHost.drawOnPress() : buttonHost.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonGuest.area() ) ? buttonGuest.drawOnPress() : buttonGuest.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        if ( HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancelGame.area() ) )
            return fheroes2::GameMode::MAIN_MENU;

        // right info
        if ( le.isMouseRightButtonPressedInArea( buttonHost.area() ) )
            fheroes2::showStandardTextMessage( _( "Host" ), _( "The host sets up the game options. There can only be one host per network game." ), Dialog::ZERO );
        if ( le.isMouseRightButtonPressedInArea( buttonGuest.area() ) )
            fheroes2::showStandardTextMessage(
                _( "Guest" ), _( "The guest waits for the host to set up the game, then is automatically added in. There can be multiple guests for TCP/IP games." ),
                Dialog::ZERO );
        if ( le.isMouseRightButtonPressedInArea( buttonCancelGame.area() ) )
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the main menu." ), Dialog::ZERO );
    }

    return fheroes2::GameMode::MAIN_MENU;
}

fheroes2::GameMode Game::NewGame( const bool isProbablyDemoVersion )
{
    outputNewMenuInTextSupportMode();

    // Stop all sounds, but not the music
    AudioManager::stopSounds();

    AudioManager::PlayMusicAsync( MUS::MAINMENU, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

    // reset last save name
    Game::SetLastSaveName( "" );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawMainMenuScreen();
    const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

    fheroes2::Button buttonStandardGame( buttonPos.x, buttonPos.y, ICN::BUTTON_STANDARD_GAME, 0, 1 );
    fheroes2::ButtonSprite buttonCampaignGame( buttonPos.x, buttonPos.y + buttonYStep * 1, fheroes2::AGG::GetICN( ICN::BUTTON_CAMPAIGN_GAME, 0 ),
                                               fheroes2::AGG::GetICN( ICN::BUTTON_CAMPAIGN_GAME, 1 ), fheroes2::AGG::GetICN( ICN::NEW_CAMPAIGN_DISABLED_BUTTON, 0 ) );
    fheroes2::Button buttonMultiGame( buttonPos.x, buttonPos.y + buttonYStep * 2, ICN::BUTTON_MULTIPLAYER_GAME, 0, 1 );
    fheroes2::Button buttonBattleGame( buttonPos.x, buttonPos.y + buttonYStep * 3, ICN::BTNBATTLEONLY, 0, 1 );
    fheroes2::Button buttonSettings( buttonPos.x, buttonPos.y + buttonYStep * 4, ICN::BUTTON_LARGE_CONFIG, 0, 1 );
    fheroes2::Button buttonCancelGame( buttonPos.x, buttonPos.y + buttonYStep * 5, ICN::BUTTON_LARGE_CANCEL, 0, 1 );

    if ( !isSuccessionWarsCampaignPresent() ) {
        buttonCampaignGame.disable();
    }

    buttonStandardGame.draw();
    buttonCampaignGame.draw();
    buttonMultiGame.draw();
    buttonBattleGame.draw();
    buttonSettings.draw();
    buttonCancelGame.draw();

    fheroes2::validateFadeInAndRender();

    LocalEvent & le = LocalEvent::Get();

    if ( isProbablyDemoVersion ) {
        fheroes2::showStandardTextMessage( _( "Warning!" ),
                                           _( "fheroes2 needs data files from the original Heroes of Might and Magic II to operate. "
                                              "You appear to be using the demo version of Heroes of Might and Magic II for this purpose. "
                                              "Please note that only one scenario will be available in this setup." ),
                                           Dialog::OK );
    }

    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonStandardGame.area() ) ? buttonStandardGame.drawOnPress() : buttonStandardGame.drawOnRelease();

        if ( buttonCampaignGame.isEnabled() ) {
            le.isMouseLeftButtonPressedInArea( buttonCampaignGame.area() ) ? buttonCampaignGame.drawOnPress() : buttonCampaignGame.drawOnRelease();
        }
        le.isMouseLeftButtonPressedInArea( buttonMultiGame.area() ) ? buttonMultiGame.drawOnPress() : buttonMultiGame.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonBattleGame.area() ) ? buttonBattleGame.drawOnPress() : buttonBattleGame.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonSettings.area() ) ? buttonSettings.drawOnPress() : buttonSettings.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_STANDARD ) || le.MouseClickLeft( buttonStandardGame.area() ) )
            return fheroes2::GameMode::NEW_STANDARD;
        if ( buttonCampaignGame.isEnabled() && ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_CAMPAIGN ) || le.MouseClickLeft( buttonCampaignGame.area() ) ) )
            return fheroes2::GameMode::NEW_CAMPAIGN_SELECTION;
        if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_MULTI ) || le.MouseClickLeft( buttonMultiGame.area() ) )
            return fheroes2::GameMode::NEW_MULTI;
        if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_SETTINGS ) || le.MouseClickLeft( buttonSettings.area() ) ) {
            fheroes2::openGameSettings();
            return fheroes2::GameMode::MAIN_MENU;
        }
        if ( HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancelGame.area() ) )
            return fheroes2::GameMode::MAIN_MENU;

        if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_BATTLEONLY ) || le.MouseClickLeft( buttonBattleGame.area() ) )
            return fheroes2::GameMode::NEW_BATTLE_ONLY;

        if ( le.isMouseRightButtonPressedInArea( buttonStandardGame.area() ) )
            fheroes2::showStandardTextMessage( _( "Standard Game" ), _( "A single player game playing out a single map." ), Dialog::ZERO );
        else if ( le.isMouseRightButtonPressedInArea( buttonCampaignGame.area() ) )
            fheroes2::showStandardTextMessage( _( "Campaign Game" ), _( "A single player game playing through a series of maps." ), Dialog::ZERO );
        else if ( le.isMouseRightButtonPressedInArea( buttonMultiGame.area() ) )
            fheroes2::showStandardTextMessage( _( "Multi-Player Game" ),
                                               _( "A multi-player game, with several human players competing against each other on a single map." ), Dialog::ZERO );
        else if ( le.isMouseRightButtonPressedInArea( buttonBattleGame.area() ) )
            fheroes2::showStandardTextMessage( _( "Battle Only" ), _( "Setup and play a battle without loading any map." ), Dialog::ZERO );
        else if ( le.isMouseRightButtonPressedInArea( buttonSettings.area() ) )
            fheroes2::showStandardTextMessage( _( "Game Settings" ), _( "Change language, resolution and settings of the game." ), Dialog::ZERO );
        else if ( le.isMouseRightButtonPressedInArea( buttonCancelGame.area() ) )
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the main menu." ), Dialog::ZERO );
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
    const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

    fheroes2::Button buttonHotSeat( buttonPos.x, buttonPos.y, ICN::BUTTON_HOT_SEAT, 0, 1 );
    fheroes2::Button buttonNetwork( buttonPos.x, buttonPos.y + buttonYStep * 1, ICN::BTNMP, 2, 3 );
    fheroes2::Button buttonCancelGame( buttonPos.x, buttonPos.y + buttonYStep * 5, ICN::BUTTON_LARGE_CANCEL, 0, 1 );

    buttonHotSeat.draw();
    buttonCancelGame.draw();
    buttonNetwork.disable();

    fheroes2::Display::instance().render();

    LocalEvent & le = LocalEvent::Get();
    // newgame loop
    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonHotSeat.area() ) ? buttonHotSeat.drawOnPress() : buttonHotSeat.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        if ( le.MouseClickLeft( buttonHotSeat.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_HOTSEAT ) )
            return fheroes2::GameMode::NEW_HOT_SEAT;
        if ( HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancelGame.area() ) )
            return fheroes2::GameMode::MAIN_MENU;

        // right info
        if ( le.isMouseRightButtonPressedInArea( buttonHotSeat.area() ) )
            fheroes2::
                showStandardTextMessage( _( "Hot Seat" ),
                                         _( "Play a Hot Seat game, where 2 to 6 players play on the same device, switching into the 'Hot Seat' when it is their turn." ),
                                         Dialog::ZERO );
        if ( le.isMouseRightButtonPressedInArea( buttonCancelGame.area() ) )
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the main menu." ), Dialog::ZERO );
    }

    return fheroes2::GameMode::QUIT_GAME;
}

uint8_t Game::SelectCountPlayers()
{
    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawMainMenuScreen();
    const fheroes2::Point buttonPos = fheroes2::drawButtonPanel();

    fheroes2::Button button2Players( buttonPos.x, buttonPos.y, ICN::BUTTON_2_PLAYERS, 0, 1 );
    fheroes2::Button button3Players( buttonPos.x, buttonPos.y + buttonYStep * 1, ICN::BUTTON_3_PLAYERS, 0, 1 );
    fheroes2::Button button4Players( buttonPos.x, buttonPos.y + buttonYStep * 2, ICN::BUTTON_4_PLAYERS, 0, 1 );
    fheroes2::Button button5Players( buttonPos.x, buttonPos.y + buttonYStep * 3, ICN::BUTTON_5_PLAYERS, 0, 1 );
    fheroes2::Button button6Players( buttonPos.x, buttonPos.y + buttonYStep * 4, ICN::BUTTON_6_PLAYERS, 0, 1 );
    fheroes2::Button buttonCancel( buttonPos.x, buttonPos.y + buttonYStep * 5, ICN::BUTTON_LARGE_CANCEL, 0, 1 );

    button2Players.draw();
    button3Players.draw();
    button4Players.draw();
    button5Players.draw();
    button6Players.draw();
    buttonCancel.draw();

    fheroes2::Display::instance().render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( button2Players.area() ) ? button2Players.drawOnPress() : button2Players.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( button3Players.area() ) ? button3Players.drawOnPress() : button3Players.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( button4Players.area() ) ? button4Players.drawOnPress() : button4Players.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( button5Players.area() ) ? button5Players.drawOnPress() : button5Players.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( button6Players.area() ) ? button6Players.drawOnPress() : button6Players.drawOnRelease();

        le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        if ( le.MouseClickLeft( button2Players.area() ) || le.isKeyPressed( fheroes2::Key::KEY_2 ) )
            return 2;
        if ( le.MouseClickLeft( button3Players.area() ) || le.isKeyPressed( fheroes2::Key::KEY_3 ) )
            return 3;
        if ( le.MouseClickLeft( button4Players.area() ) || le.isKeyPressed( fheroes2::Key::KEY_4 ) )
            return 4;
        if ( le.MouseClickLeft( button5Players.area() ) || le.isKeyPressed( fheroes2::Key::KEY_5 ) )
            return 5;
        if ( le.MouseClickLeft( button6Players.area() ) || le.isKeyPressed( fheroes2::Key::KEY_6 ) )
            return 6;

        if ( HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancel.area() ) )
            return 0;

        // right info
        if ( le.isMouseRightButtonPressedInArea( button2Players.area() ) )
            fheroes2::showStandardTextMessage( _( "2 Players" ), _( "Play with 2 human players, and optionally, up to 4 additional computer players." ), Dialog::ZERO );
        if ( le.isMouseRightButtonPressedInArea( button3Players.area() ) )
            fheroes2::showStandardTextMessage( _( "3 Players" ), _( "Play with 3 human players, and optionally, up to 3 additional computer players." ), Dialog::ZERO );
        if ( le.isMouseRightButtonPressedInArea( button4Players.area() ) )
            fheroes2::showStandardTextMessage( _( "4 Players" ), _( "Play with 4 human players, and optionally, up to 2 additional computer players." ), Dialog::ZERO );
        if ( le.isMouseRightButtonPressedInArea( button5Players.area() ) )
            fheroes2::showStandardTextMessage( _( "5 Players" ), _( "Play with 5 human players, and optionally, up to 1 additional computer player." ), Dialog::ZERO );
        if ( le.isMouseRightButtonPressedInArea( button6Players.area() ) )
            fheroes2::showStandardTextMessage( _( "6 Players" ), _( "Play with 6 human players." ), Dialog::ZERO );
        if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) )
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the main menu." ), Dialog::ZERO );
    }

    return 0;
}
