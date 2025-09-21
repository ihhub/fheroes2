/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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
#include "maps_fileinfo.h"
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
#include "ui_window.h"

namespace
{
    const size_t playerCountOptions = 5;
    const std::array<fheroes2::Key, playerCountOptions> playerCountHotkeys
        = { fheroes2::Key::KEY_2, fheroes2::Key::KEY_3, fheroes2::Key::KEY_4, fheroes2::Key::KEY_5, fheroes2::Key::KEY_6 };
    const std::array<fheroes2::GameMode, playerCountOptions> playerCountModes
        = { fheroes2::GameMode::SELECT_SCENARIO_TWO_HUMAN_PLAYERS, fheroes2::GameMode::SELECT_SCENARIO_THREE_HUMAN_PLAYERS,
            fheroes2::GameMode::SELECT_SCENARIO_FOUR_HUMAN_PLAYERS, fheroes2::GameMode::SELECT_SCENARIO_FIVE_HUMAN_PLAYERS,
            fheroes2::GameMode::SELECT_SCENARIO_SIX_HUMAN_PLAYERS };

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
        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::MAIN_MENU_BATTLEONLY ) << " to choose Battle Only Game." )
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
        fheroes2::showStandardTextMessage( _( "Warning" ),
                                           _( "The required video files for the campaign selection window are missing. "
                                              "Please make sure that all necessary files are present in the system." ),
                                           Dialog::OK );
    }
}

fheroes2::GameMode Game::NewStandard()
{
    Settings & conf = Settings::Get();
    if ( conf.isCampaignGameType() ) {
        conf.setCurrentMapInfo( {} );
    }
    conf.SetGameType( Game::TYPE_STANDARD );
    return fheroes2::GameMode::SELECT_SCENARIO_ONE_HUMAN_PLAYER;
}

fheroes2::GameMode Game::NewBattleOnly()
{
    Settings & conf = Settings::Get();
    conf.SetGameType( Game::TYPE_BATTLEONLY );
    // Redraw the empty main menu screen to show it after the battle using screen restorer.
    fheroes2::drawMainMenuScreen();
    return fheroes2::GameMode::START_BATTLE_ONLY_MODE;
}

fheroes2::GameMode Game::NewHotSeat( const size_t playerCountOptionIndex )
{
    assert( playerCountOptionIndex < playerCountModes.size() );
    Settings & conf = Settings::Get();
    if ( conf.isCampaignGameType() ) {
        conf.setCurrentMapInfo( {} );
    }
    conf.SetGameType( Game::TYPE_HOTSEAT );
    return playerCountModes[playerCountOptionIndex];
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

    const uint64_t customDelay = static_cast<uint64_t>( std::lround( video->microsecondsPerFrame() / 1000 ) );

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

    fheroes2::GameMode gameChoice = fheroes2::GameMode::NEW_GAME;
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
                customDelay = static_cast<uint64_t>( std::lround( videos[highlightCampaignId]->microsecondsPerFrame() / 1000 ) );
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

fheroes2::GameMode Game::NewGame( const bool isProbablyDemoVersion )
{
    outputNewMenuInTextSupportMode();

    // Stop all sounds, but not the music
    AudioManager::stopSounds();

    AudioManager::PlayMusicAsync( MUS::MAINMENU, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

    // Reset last save name
    Game::SetLastSaveName( "" );

    // Setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::drawMainMenuScreen();

    // Setup dialog from buttons.
    const int menuButtonsIcnIndex = Settings::Get().isEvilInterfaceEnabled() ? ICN::BUTTONS_NEW_GAME_MENU_EVIL : ICN::BUTTONS_NEW_GAME_MENU_GOOD;
    fheroes2::ButtonGroup mainModeButtons;
    // Only add the buttons needed for the initial state of the dialog.
    for ( int32_t i = 0; i < 5; ++i ) {
        mainModeButtons.createButton( 0, 0, menuButtonsIcnIndex, i * 2, i * 2 + 1, i );
    }

    const fheroes2::ButtonBase & buttonStandardGame = mainModeButtons.button( 0 );
    fheroes2::ButtonBase & buttonCampaignGame = mainModeButtons.button( 1 );
    const fheroes2::ButtonBase & buttonMultiGame = mainModeButtons.button( 2 );
    const fheroes2::ButtonBase & buttonBattleGame = mainModeButtons.button( 3 );
    const fheroes2::ButtonBase & buttonSettings = mainModeButtons.button( 4 );

    // Generate dialog background with extra space added for the cancel button.
    const int32_t spaceBetweenButtons = 10;
    fheroes2::StandardWindow background( mainModeButtons, true, buttonStandardGame.area().height + spaceBetweenButtons );

    // Make corners like in the original game.
    background.applyGemDecoratedCorners();

    // We don't need to restore the cancel button area because every state of the dialog has this button.
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::ImageRestorer emptyDialog( display, background.activeArea().x, background.activeArea().y, background.activeArea().width,
                                         background.activeArea().height - buttonStandardGame.area().height - spaceBetweenButtons * 2 - 2 );

    if ( !isSuccessionWarsCampaignPresent() ) {
        buttonCampaignGame.disable();
    }

    background.renderSymmetricButtons( mainModeButtons, 0, true );

    // Add the cancel button at the bottom of the dialog.
    fheroes2::Button buttonCancel( buttonStandardGame.area().x,
                                   background.activeArea().y * 2 + background.activeArea().height - buttonStandardGame.area().y - buttonStandardGame.area().height,
                                   menuButtonsIcnIndex, 10, 11 );
    buttonCancel.draw();
    buttonCancel.drawShadow( display );

    // Add extra buttons in disabled state.
    fheroes2::Button buttonHotSeat( buttonStandardGame.area().x, buttonStandardGame.area().y, menuButtonsIcnIndex, 12, 13 );
    buttonHotSeat.disable();

    fheroes2::ButtonGroup playerCountButtons;

    for ( int32_t i = 0; i < 5; ++i ) {
        playerCountButtons.createButton( buttonStandardGame.area().x, buttonStandardGame.area().y + i * ( buttonStandardGame.area().height + spaceBetweenButtons ),
                                         menuButtonsIcnIndex, ( i + 7 ) * 2, ( i + 7 ) * 2 + 1, i );
        playerCountButtons.button( i ).disable();
    }

    fheroes2::Button buttonSuccessionWars;
    fheroes2::Button buttonPriceOfLoyalty;
    buttonSuccessionWars.disable();
    buttonPriceOfLoyalty.disable();

    const bool isPriceOfLoyaltyPresent = isPriceOfLoyaltyCampaignPresent();

    if ( isPriceOfLoyaltyPresent ) {
        buttonSuccessionWars.setICNInfo( menuButtonsIcnIndex, 24, 25 );
        buttonPriceOfLoyalty.setICNInfo( menuButtonsIcnIndex, 26, 27 );
        buttonSuccessionWars.setPosition( mainModeButtons.button( 0 ).area().x, mainModeButtons.button( 0 ).area().y );
        buttonPriceOfLoyalty.setPosition( mainModeButtons.button( 1 ).area().x, mainModeButtons.button( 1 ).area().y );
    }

    fheroes2::validateFadeInAndRender();

    LocalEvent & le = LocalEvent::Get();

    if ( isProbablyDemoVersion ) {
        fheroes2::showStandardTextMessage( _( "Warning" ),
                                           _( "fheroes2 needs data files from the original Heroes of Might and Magic II to operate. "
                                              "You appear to be using the demo version of Heroes of Might and Magic II for this purpose. "
                                              "Please note that only one scenario will be available in this setup." ),
                                           Dialog::OK );
    }

    while ( le.HandleEvents() ) {
        if ( buttonStandardGame.isEnabled() ) {
            mainModeButtons.drawOnState( le );

            if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_STANDARD ) || le.MouseClickLeft( buttonStandardGame.area() ) ) {
                return fheroes2::GameMode::NEW_STANDARD;
            }
            if ( buttonCampaignGame.isEnabled() && ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_CAMPAIGN ) || le.MouseClickLeft( buttonCampaignGame.area() ) ) ) {
                if ( !isPriceOfLoyaltyCampaignPresent() ) {
                    return fheroes2::GameMode::NEW_SUCCESSION_WARS_CAMPAIGN;
                }
                mainModeButtons.disable();
                emptyDialog.restore();
                buttonSuccessionWars.enable();
                buttonPriceOfLoyalty.enable();
                buttonSuccessionWars.draw();
                buttonPriceOfLoyalty.draw();
                buttonSuccessionWars.drawShadow( display );
                buttonPriceOfLoyalty.drawShadow( display );

                outputNewCampaignSelectionInTextSupportMode();

                display.render( emptyDialog.rect() );
            }
            if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_SETTINGS ) || le.MouseClickLeft( buttonSettings.area() ) ) {
                fheroes2::openGameSettings();
                return fheroes2::GameMode::MAIN_MENU;
            }
            if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_BATTLEONLY ) || le.MouseClickLeft( buttonBattleGame.area() ) ) {
                return fheroes2::GameMode::NEW_BATTLE_ONLY;
            }
            if ( HotKeyPressEvent( HotKeyEvent::MAIN_MENU_MULTI ) || le.MouseClickLeft( buttonMultiGame.area() ) ) {
                mainModeButtons.disable();
                emptyDialog.restore();
                buttonHotSeat.enable();
                buttonHotSeat.draw();
                buttonHotSeat.drawShadow( display );
                display.render( emptyDialog.rect() );
            }

            if ( le.isMouseRightButtonPressedInArea( buttonStandardGame.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Standard Game" ), _( "A single player game playing out a single map." ), Dialog::ZERO );
            }
            else if ( buttonCampaignGame.isEnabled() && le.isMouseRightButtonPressedInArea( buttonCampaignGame.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Campaign Game" ), _( "A single player game playing through a series of maps." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonMultiGame.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Multi-Player Game" ),
                                                   _( "A multi-player game, with several human players competing against each other on a single map." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonBattleGame.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Battle Only" ), _( "Setup and play a battle without loading any map." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonSettings.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Game Settings" ), _( "Change language, resolution and settings of the game." ), Dialog::ZERO );
            }
        }
        else if ( playerCountButtons.button( 0 ).isEnabled() ) {
            playerCountButtons.drawOnState( le );

            // Loop through all player count buttons.
            for ( size_t i = 0; i < playerCountOptions; ++i ) {
                assert( i < playerCountHotkeys.size() );

                if ( le.MouseClickLeft( playerCountButtons.button( i ).area() ) || le.isKeyPressed( playerCountHotkeys[i] ) ) {
                    return NewHotSeat( i );
                }
            }
            if ( le.isMouseRightButtonPressedInArea( playerCountButtons.button( 0 ).area() ) ) {
                fheroes2::showStandardTextMessage( _( "2 Players" ), _( "Play with 2 human players, and optionally, up to 4 additional computer players." ),
                                                   Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( playerCountButtons.button( 1 ).area() ) ) {
                fheroes2::showStandardTextMessage( _( "3 Players" ), _( "Play with 3 human players, and optionally, up to 3 additional computer players." ),
                                                   Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( playerCountButtons.button( 2 ).area() ) ) {
                fheroes2::showStandardTextMessage( _( "4 Players" ), _( "Play with 4 human players, and optionally, up to 2 additional computer players." ),
                                                   Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( playerCountButtons.button( 3 ).area() ) ) {
                fheroes2::showStandardTextMessage( _( "5 Players" ), _( "Play with 5 human players, and optionally, up to 1 additional computer player." ),
                                                   Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( playerCountButtons.button( 4 ).area() ) ) {
                fheroes2::showStandardTextMessage( _( "6 Players" ), _( "Play with 6 human players." ), Dialog::ZERO );
            }
        }
        else if ( buttonSuccessionWars.isEnabled() ) {
            buttonSuccessionWars.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonSuccessionWars.area() ) );
            buttonPriceOfLoyalty.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonPriceOfLoyalty.area() ) );

            if ( le.MouseClickLeft( buttonSuccessionWars.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_NEW_ORIGINAL_CAMPAIGN ) ) {
                return fheroes2::GameMode::NEW_SUCCESSION_WARS_CAMPAIGN;
            }
            if ( le.MouseClickLeft( buttonPriceOfLoyalty.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_NEW_EXPANSION_CAMPAIGN ) ) {
                return fheroes2::GameMode::NEW_PRICE_OF_LOYALTY_CAMPAIGN;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonSuccessionWars.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Original Campaign" ),
                                                   _( "Either Roland's or Archibald's campaign from the original Heroes of Might and Magic II." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonPriceOfLoyalty.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Expansion Campaign" ), _( "One of the four new campaigns from the Price of Loyalty expansion set." ),
                                                   Dialog::ZERO );
            }
        }
        else {
            buttonHotSeat.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonHotSeat.area() ) );
            if ( le.MouseClickLeft( buttonHotSeat.area() ) || HotKeyPressEvent( HotKeyEvent::MAIN_MENU_HOTSEAT ) ) {
                buttonHotSeat.disable();
                emptyDialog.restore();
                playerCountButtons.enable();
                playerCountButtons.draw();
                playerCountButtons.drawShadows( display );
                display.render( emptyDialog.rect() );
                continue;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonHotSeat.area() ) ) {
                fheroes2::showStandardTextMessage(
                    _( "Hot Seat" ), _( "Play a Hot Seat game, where 2 to 6 players play on the same device, switching into the 'Hot Seat' when it is their turn." ),
                    Dialog::ZERO );
            }
        }
        buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

        if ( HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancel.area() ) ) {
            return fheroes2::GameMode::MAIN_MENU;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Cancel back to the main menu." ), Dialog::ZERO );
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}
