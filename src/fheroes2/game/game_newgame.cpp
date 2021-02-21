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

#include <sstream>

#include "agg.h"
#include "assert.h"
#include "audio_mixer.h"
#include "audio_music.h"
#include "campaign_savedata.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_video.h"
#include "gamedefs.h"
#include "mus.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"
#include "ui_tool.h"
#include "world.h"

int Game::NewStandard( void )
{
    Settings & conf = Settings::Get();
    if ( conf.GameType() == Game::TYPE_CAMPAIGN )
        conf.SetCurrentFileInfo( Maps::FileInfo() );
    conf.SetGameType( Game::TYPE_STANDARD );
    conf.SetPreferablyCountPlayers( 0 );
    return Game::SELECTSCENARIO;
}

int Game::NewBattleOnly( void )
{
    Settings & conf = Settings::Get();
    conf.SetGameType( Game::TYPE_BATTLEONLY );

    return Game::NEWMULTI;
}

int Game::NewHotSeat( void )
{
    Settings & conf = Settings::Get();
    if ( conf.GameType() == Game::TYPE_CAMPAIGN )
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
            return Game::SELECTSCENARIO;
        }
    }
    return Game::MAINMENU;
}

int Game::NewCampaign()
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

    Video::ShowVideo( "INTRO.SMK", false );
    Video::ShowVideo( "CHOOSEW.SMK", false );
    const size_t chosenCampaign = Video::ShowVideo( "CHOOSE.SMK", true, campaignRoi );

    Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
    campaignSaveData.reset();
    campaignSaveData.setCampaignID( chosenCampaign );
    campaignSaveData.setCurrentScenarioID( 0 );

    AGG::PlayMusic( MUS::VICTORY, true, true );

    return Game::SELECT_CAMPAIGN_SCENARIO;
}

#ifdef NETWORK_ENABLE
int Game::NewNetwork( void )
{
    Settings & conf = Settings::Get();
    conf.SetGameType( conf.GameType() | Game::TYPE_NETWORK );

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    // image background
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HEROES, 0 );
    fheroes2::Blit( back, display );
    const uint32_t backgroundWidth = back.width();

    const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REDBACK, 0 );
    fheroes2::Blit( panel, display, backgroundWidth - 235, 5 );

    LocalEvent & le = LocalEvent::Get();

    fheroes2::Button buttonHost( backgroundWidth - 185, 45, ICN::BTNNET, 0, 1 );
    fheroes2::Button buttonGuest( backgroundWidth - 185, 110, ICN::BTNNET, 2, 3 );
    fheroes2::Button buttonCancelGame( backgroundWidth - 185, 375, ICN::BTNMP, 8, 9 );

    buttonHost.draw();
    buttonGuest.draw();
    buttonCancelGame.draw();

    cursor.Show();
    display.render();

    // newgame loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonHost.area() ) ? buttonHost.drawOnPress() : buttonHost.drawOnRelease();
        le.MousePressLeft( buttonGuest.area() ) ? buttonGuest.drawOnPress() : buttonGuest.drawOnRelease();
        le.MousePressLeft( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        // if(le.MouseClickLeft(buttonHost) || HotKeyPressEvent(EVENT_BUTTON_HOST)) return NetworkHost();
        // if(le.MouseClickLeft(buttonGuest) || HotKeyPressEvent(EVENT_BUTTON_GUEST)) return NetworkGuest();
        if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancelGame.area() ) )
            return MAINMENU;

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

    return Game::MAINMENU;
}
#endif

int Game::NewGame( void )
{
    Mixer::Pause();
    AGG::PlayMusic( MUS::MAINMENU, true, true );
    Settings & conf = Settings::Get();

    // reset last save name
    Game::SetLastSavename( "" );

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    // load game settings
    conf.BinaryLoad();

    // image background
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HEROES, 0 );
    fheroes2::Copy( back, display );

    const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REDBACK, 0 );
    const uint32_t panelOffset = fheroes2::Display::DEFAULT_HEIGHT - panel.height();
    const uint32_t panelXPos = back.width() - ( panel.width() + panelOffset );
    fheroes2::Blit( panel, display, panelXPos, panelOffset );

    const uint32_t buttonMiddlePos = panelXPos + SHADOWWIDTH + ( panel.width() - SHADOWWIDTH ) / 2;
    const fheroes2::Sprite & buttonSample = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 0 );
    const uint32_t buttonWidth = buttonSample.width();
    const uint32_t buttonXPos = buttonMiddlePos - buttonWidth / 2 - 3; // 3 is button shadow
    const uint32_t buttonYPos = 46;
    const uint32_t buttonYStep = 66;
    fheroes2::Button buttonStandartGame( buttonXPos, buttonYPos, ICN::BTNNEWGM, 0, 1 );
    fheroes2::Button buttonCampainGame( buttonXPos, buttonYPos + buttonYStep * 1, ICN::BTNNEWGM, 2, 3 );
    fheroes2::Button buttonMultiGame( buttonXPos, buttonYPos + buttonYStep * 2, ICN::BTNNEWGM, 4, 5 );
    fheroes2::Button buttonBattleGame( buttonXPos, buttonYPos + buttonYStep * 3, ICN::BTNBATTLEONLY, 0, 1 );
    fheroes2::Button buttonSettings( buttonXPos, buttonYPos + buttonYStep * 4, ICN::BTNDCCFG, 4, 5 );
    fheroes2::Button buttonCancelGame( buttonXPos, buttonYPos + buttonYStep * 5, ICN::BTNNEWGM, 6, 7 );

    if ( !IsOriginalCampaignPresent() ) {
        buttonCampainGame.disable();
    }

    buttonStandartGame.draw();
    buttonCampainGame.draw();
    buttonMultiGame.draw();
    buttonBattleGame.draw();
    buttonSettings.draw();
    buttonCancelGame.draw();

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) { // new game loop
        le.MousePressLeft( buttonStandartGame.area() ) ? buttonStandartGame.drawOnPress() : buttonStandartGame.drawOnRelease();

        if ( buttonCampainGame.isEnabled() ) {
            le.MousePressLeft( buttonCampainGame.area() ) ? buttonCampainGame.drawOnPress() : buttonCampainGame.drawOnRelease();
        }
        le.MousePressLeft( buttonMultiGame.area() ) ? buttonMultiGame.drawOnPress() : buttonMultiGame.drawOnRelease();
        le.MousePressLeft( buttonBattleGame.area() ) ? buttonBattleGame.drawOnPress() : buttonBattleGame.drawOnRelease();
        le.MousePressLeft( buttonSettings.area() ) ? buttonSettings.drawOnPress() : buttonSettings.drawOnRelease();
        le.MousePressLeft( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        if ( HotKeyPressEvent( EVENT_BUTTON_STANDARD ) || le.MouseClickLeft( buttonStandartGame.area() ) )
            return NEWSTANDARD;
        if ( buttonCampainGame.isEnabled() && ( HotKeyPressEvent( EVENT_BUTTON_CAMPAIGN ) || le.MouseClickLeft( buttonCampainGame.area() ) ) )
            return NEWCAMPAIGN;
        if ( HotKeyPressEvent( EVENT_BUTTON_MULTI ) || le.MouseClickLeft( buttonMultiGame.area() ) )
            return NEWMULTI;
        if ( HotKeyPressEvent( EVENT_BUTTON_SETTINGS ) || le.MouseClickLeft( buttonSettings.area() ) ) {
            Dialog::ExtSettings( false );
            cursor.Show();
            display.render();
        }
        if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancelGame.area() ) )
            return MAINMENU;

        if ( HotKeyPressEvent( EVENT_BUTTON_BATTLEONLY ) || le.MouseClickLeft( buttonBattleGame.area() ) )
            return NEWBATTLEONLY;

        if ( le.MousePressRight( buttonStandartGame.area() ) )
            Dialog::Message( _( "Standard Game" ), _( "A single player game playing out a single map." ), Font::BIG );
        if ( le.MousePressRight( buttonCampainGame.area() ) )
            Dialog::Message( _( "Campaign Game" ), _( "A single player game playing through a series of maps." ), Font::BIG );
        if ( le.MousePressRight( buttonMultiGame.area() ) )
            Dialog::Message( _( "Multi-Player Game" ), _( "A multi-player game, with several human players completing against each other on a single map." ), Font::BIG );
        if ( le.MousePressRight( buttonSettings.area() ) )
            Dialog::Message( _( "Settings" ), _( "Experimental game settings." ), Font::BIG );
        if ( le.MousePressRight( buttonCancelGame.area() ) )
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );
    }

    return QUITGAME;
}

int Game::NewMulti( void )
{
    Settings & conf = Settings::Get();

    if ( !( conf.IsGameType( Game::TYPE_BATTLEONLY ) ) )
        conf.SetGameType( Game::TYPE_STANDARD );

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    // image background
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HEROES, 0 );
    fheroes2::Copy( back, display );

    const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REDBACK, 0 );
    const uint32_t panelOffset = fheroes2::Display::DEFAULT_HEIGHT - panel.height();
    const uint32_t panelXPos = back.width() - ( panel.width() + panelOffset );
    fheroes2::Blit( panel, display, panelXPos, panelOffset );

    LocalEvent & le = LocalEvent::Get();

    const uint32_t buttonMiddlePos = panelXPos + SHADOWWIDTH + ( panel.width() - SHADOWWIDTH ) / 2;
    const fheroes2::Sprite & buttonSample = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 0 );
    const uint32_t buttonWidth = buttonSample.width();
    const uint32_t buttonXPos = buttonMiddlePos - buttonWidth / 2 - 3; // 3 is button shadow
    const uint32_t buttonYPos = 46;
    const uint32_t buttonYStep = 66;
    fheroes2::Button buttonHotSeat( buttonXPos, buttonYPos, ICN::BTNMP, 0, 1 );
    fheroes2::Button buttonNetwork( buttonXPos, buttonYPos + buttonYStep * 1, ICN::BTNMP, 2, 3 );
    fheroes2::Button buttonCancelGame( buttonXPos, buttonYPos + buttonYStep * 5, ICN::BTNMP, 8, 9 );

    buttonHotSeat.draw();
    buttonCancelGame.draw();
    buttonNetwork.disable();

    cursor.Show();
    display.render();

    // newgame loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonHotSeat.area() ) ? buttonHotSeat.drawOnPress() : buttonHotSeat.drawOnRelease();
        le.MousePressLeft( buttonCancelGame.area() ) ? buttonCancelGame.drawOnPress() : buttonCancelGame.drawOnRelease();

        if ( le.MouseClickLeft( buttonHotSeat.area() ) || HotKeyPressEvent( EVENT_BUTTON_HOTSEAT ) )
            return NEWHOTSEAT;
        if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancelGame.area() ) )
            return MAINMENU;

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
                return NEWNETWORK;
            if ( le.MousePressRight( buttonNetwork.area() ) )
                Dialog::Message( _( "Network" ), _( "Play a network game, where 2 players use their own computers connected through a LAN (Local Area Network)." ),
                                 Font::BIG );
        }
#endif
    }

    return QUITGAME;
}

u32 Game::SelectCountPlayers( void )
{
    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    // image background
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HEROES, 0 );
    fheroes2::Copy( back, display );

    const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REDBACK, 0 );
    const uint32_t panelOffset = fheroes2::Display::DEFAULT_HEIGHT - panel.height();
    const uint32_t panelXPos = back.width() - ( panel.width() + panelOffset );
    fheroes2::Blit( panel, display, panelXPos, panelOffset );

    LocalEvent & le = LocalEvent::Get();

    const uint32_t buttonMiddlePos = panelXPos + SHADOWWIDTH + ( panel.width() - SHADOWWIDTH ) / 2;
    const fheroes2::Sprite & buttonSample = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 0 );
    const uint32_t buttonWidth = buttonSample.width();
    const uint32_t buttonXPos = buttonMiddlePos - buttonWidth / 2 - 3; // 3 is button shadow
    const uint32_t buttonYPos = 46;
    const uint32_t buttonYStep = 66;
    fheroes2::Button button2Players( buttonXPos, buttonYPos, ICN::BTNHOTST, 0, 1 );
    fheroes2::Button button3Players( buttonXPos, buttonYPos + buttonYStep * 1, ICN::BTNHOTST, 2, 3 );
    fheroes2::Button button4Players( buttonXPos, buttonYPos + buttonYStep * 2, ICN::BTNHOTST, 4, 5 );
    fheroes2::Button button5Players( buttonXPos, buttonYPos + buttonYStep * 3, ICN::BTNHOTST, 6, 7 );
    fheroes2::Button button6Players( buttonXPos, buttonYPos + buttonYStep * 4, ICN::BTNHOTST, 8, 9 );
    fheroes2::Button buttonCancel( buttonXPos, buttonYPos + buttonYStep * 5, ICN::BTNNEWGM, 6, 7 );

    button2Players.draw();
    button3Players.draw();
    button4Players.draw();
    button5Players.draw();
    button6Players.draw();
    buttonCancel.draw();

    cursor.Show();
    display.render();

    // newgame loop
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
