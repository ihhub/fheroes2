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
#include "button.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "gamedefs.h"
#include "mus.h"
#include "settings.h"
#include "text.h"
#include "world.h"

namespace
{
    const std::string rolandCampaignDescription[] = {_(
        "Roland needs you to defeat the lords near his castle to begin his war of rebellion against his brother. They are not allied with each other, so they will spend most of their time fighting with on another. Victory is yours when you have defeated all of their castles and heroes." )};

    std::string ConvertToString( int value )
    {
        std::ostringstream ostr;
        ostr << value;
        return ostr.str();
    }

    void DrawCampaignScenarioIcon( int id, double offsetXMultipler, double offsetYMultipler, int icnId, Point offset )
    {
        const Sprite & campaignMapIcon = AGG::GetICN( ICN::CAMPXTRG, icnId );
        campaignMapIcon.Blit( offset.x + 40 + 73 * offsetXMultipler, offset.y + 356 + campaignMapIcon.h() * ( offsetYMultipler - 0.5 ) );
        Text campaignMapText( ConvertToString( id ), Font::YELLOW_BIG );
        campaignMapText.Blit( offset.x + 40 + 73 * offsetXMultipler + campaignMapIcon.w(),
                              offset.y + 356 + campaignMapIcon.h() * ( offsetYMultipler + 0.5 ) - campaignMapText.h() );
    }

    bool hasEnding( std::string const & fullString, std::string const & ending )
    {
        if ( fullString.length() >= ending.length() )
            return ( 0 == fullString.compare( fullString.length() - ending.length(), ending.length(), ending ) );
        else
            return false;
    }

    std::vector<Maps::FileInfo> GetRolandCampaign()
    {
        const size_t rolandMapCount = 11;
        static const std::string rolandMap[rolandMapCount] = {"CAMPG01.H2C", "CAMPG02.H2C", "CAMPG03.H2C", "CAMPG04.H2C", "CAMPG05.H2C", "CAMPG05B.H2C",
                                                              "CAMPG06.H2C", "CAMPG07.H2C", "CAMPG08.H2C", "CAMPG09.H2C", "CAMPG10.H2C"};
        const ListFiles files = Settings::Get().GetListFiles( "maps", ".h2c" );

        std::vector<Maps::FileInfo> maps;

        for ( size_t i = 0; i < rolandMapCount; ++i ) {
            bool isPresent = false;
            for ( ListFiles::const_iterator file = files.begin(); file != files.end(); ++file ) {
                if ( hasEnding( *file, rolandMap[i] ) ) {
                    Maps::FileInfo fi;
                    if ( fi.ReadMP2( *file ) ) {
                        maps.push_back( fi );
                        isPresent = true;
                        break;
                    }
                }
            }
            if ( !isPresent )
                return std::vector<Maps::FileInfo>();
        }

        return maps;
    }
}

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

    conf.SetGameType( conf.GameType() | Game::TYPE_HOTSEAT );

    if ( conf.GameType( Game::TYPE_BATTLEONLY ) ) {
        conf.SetPreferablyCountPlayers( 2 );
        world.NewMaps( 10, 10 );
        return StartBattleOnly();
    }
    else {
        const u32 select = conf.QVGA() ? 2 : SelectCountPlayers();
        if ( select ) {
            conf.SetPreferablyCountPlayers( select );
            return Game::SELECTSCENARIO;
        }
    }
    return Game::MAINMENU;
}

int Game::NewCampain( void )
{
    Settings::Get().SetGameType( Game::TYPE_CAMPAIGN );

    Mixer::Pause();
    AGG::PlayMusic( MUS::VICTORY );
    Settings & conf = Settings::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    Display & display = Display::Get();
    display.Fill( ColorBlack );

    const Sprite & backgroundImage = AGG::GetICN( ICN::CAMPBKGG, 0 );
    const Point top( ( display.w() - backgroundImage.w() ) / 2, ( display.h() - backgroundImage.h() ) / 2 );
    backgroundImage.Blit( top );

    Button buttonViewIntro( top.x + 30, top.y + 430, ICN::CAMPXTRG, 0, 1 );
    Button buttonOk( top.x + 380, top.y + 430, ICN::NGEXTRA, 66, 67 );
    Button buttonCancel( top.x + 520, top.y + 430, ICN::NGEXTRA, 68, 69 );

    const std::vector<Maps::FileInfo> & campaignMap = GetRolandCampaign();

    buttonViewIntro.SetDisable( true );
    buttonViewIntro.Draw();
    buttonOk.SetDisable( campaignMap.empty() );
    buttonOk.Draw();
    buttonCancel.Draw();

    Text textDaysSpent( "0", Font::BIG );
    textDaysSpent.Blit( top.x + 570 + textDaysSpent.w() / 2, top.y + 31 );

    if ( !campaignMap.empty() ) {
        TextBox mapName( campaignMap[0].description, Font::BIG, 200 );
        mapName.Blit( top.x + 200, top.y + 97 - mapName.h() / 2 );

        Text campaignMapId( "1", Font::BIG );
        campaignMapId.Blit( top.x + 175 - campaignMapId.w() / 2, top.y + 97 - campaignMapId.h() / 2 );

        TextBox mapDescription( rolandCampaignDescription[0], Font::BIG, 350 );
        mapDescription.Blit( top.x + 40, top.y + 140 );

        TextBox awards( _( "None" ), Font::BIG, 180 );
        awards.Blit( top.x + 425, top.y + 100 );

        Text choice1( _( "2000 Gold" ), Font::BIG );
        choice1.Blit( top.x + 425, top.y + 209 - choice1.h() / 2 );
        Text choice2( _( "Thunder Mace" ), Font::BIG );
        choice2.Blit( top.x + 425, top.y + 209 + 23 - choice2.h() / 2 );
        Text choice3( _( "Gauntlets" ), Font::BIG );
        choice3.Blit( top.x + 425, top.y + 209 + 45 - choice3.h() / 2 );
    }
    else {
        TextBox textCaption( "We are working hard to ensure that the support of Campaign would arrive as soon as possible", Font::YELLOW_BIG, 350 );
        textCaption.Blit( top.x + 40, top.y + 140 );

        TextBox textDescription( "Campaign Game mode is under construction", Font::BIG, 350 );
        textDescription.Blit( top.x + 40, top.y + 200 );
    }

    DrawCampaignScenarioIcon( 1, 0, 0, 14, top );
    DrawCampaignScenarioIcon( 2, 1, 0, 15, top );
    DrawCampaignScenarioIcon( 3, 1.5, -1, 15, top );
    DrawCampaignScenarioIcon( 4, 2, 0, 15, top );
    DrawCampaignScenarioIcon( 5, 3, 0, 15, top );
    DrawCampaignScenarioIcon( 6, 4, 0, 15, top );
    DrawCampaignScenarioIcon( 7, 5, 0, 15, top );
    DrawCampaignScenarioIcon( 8, 6, -1, 15, top );
    DrawCampaignScenarioIcon( 9, 6, 1, 15, top );
    DrawCampaignScenarioIcon( 10, 7, 0, 15, top );

    LocalEvent & le = LocalEvent::Get();

    cursor.Show();
    display.Flip();

    while ( le.HandleEvents() ) {
        if ( !buttonCancel.isDisable() )
            le.MousePressLeft( buttonCancel ) ? buttonCancel.PressDraw() : buttonCancel.ReleaseDraw();
        if ( !buttonOk.isDisable() )
            le.MousePressLeft( buttonOk ) ? buttonOk.PressDraw() : buttonOk.ReleaseDraw();

        if ( le.MouseClickLeft( buttonCancel ) )
            return Game::NEWGAME;
        else if ( !buttonOk.isDisable() && le.MouseClickLeft( buttonOk ) ) {
            conf.SetCurrentFileInfo( campaignMap[0] );
            Players & players = conf.GetPlayers();
            players.SetStartGame();
            if ( conf.ExtGameUseFade() )
                display.Fade();
            Game::ShowLoadMapsText();
            conf.SetGameType( Game::TYPE_CAMPAIGN );

            if ( !world.LoadMapMP2( campaignMap[0].file ) ) {
                Dialog::Message( "Campaign Game loading failure", "Please make sure that campaign files are correct and present", Font::SMALL, Dialog::OK );
                conf.SetCurrentFileInfo( Maps::FileInfo() );
                continue;
            }

            return Game::STARTGAME;
        }
    }

    return Game::NEWGAME;
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

    Display & display = Display::Get();
    // Settings & conf = Settings::Get();

    AGG::RegisterScalableICN( ICN::HEROES );

    // image background
    const Sprite & back = AGG::GetICN( ICN::HEROES, 0 );
    back.Blit( Point( 0, 0 ) );

    const Sprite & panel = AGG::GetICN( ICN::REDBACK, 0 );
    panel.Blit( back.w() - 235, 5 );

    LocalEvent & le = LocalEvent::Get();

    Button buttonHost( back.w() - 185, 45, ICN::BTNNET, 0, 1 );
    Button buttonGuest( back.w() - 185, 110, ICN::BTNNET, 2, 3 );
    Button buttonCancelGame( back.w() - 185, 375, ICN::BTNMP, 8, 9 );

    buttonHost.Draw();
    buttonGuest.Draw();
    buttonCancelGame.Draw();

    cursor.Show();
    display.Flip();

    // newgame loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonHost ) ? buttonHost.PressDraw() : buttonHost.ReleaseDraw();
        le.MousePressLeft( buttonGuest ) ? buttonGuest.PressDraw() : buttonGuest.ReleaseDraw();
        le.MousePressLeft( buttonCancelGame ) ? buttonCancelGame.PressDraw() : buttonCancelGame.ReleaseDraw();

        // if(le.MouseClickLeft(buttonHost) || HotKeyPressEvent(EVENT_BUTTON_HOST)) return NetworkHost();
        // if(le.MouseClickLeft(buttonGuest) || HotKeyPressEvent(EVENT_BUTTON_GUEST)) return NetworkGuest();
        if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancelGame ) )
            return MAINMENU;

        // right info
        if ( le.MousePressRight( buttonHost ) )
            Dialog::Message( _( "Host" ), _( "The host sets up the game options. There can only be one host per network game." ), Font::BIG );
        if ( le.MousePressRight( buttonGuest ) )
            Dialog::Message( _( "Guest" ),
                             _( "The guest waits for the host to set up the game, then is automatically added in. There can be multiple guests for TCP/IP games." ),
                             Font::BIG );
        if ( le.MousePressRight( buttonCancelGame ) )
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );
    }

    return Game::MAINMENU;
}
#endif

int Game::NewGame( void )
{
    Mixer::Pause();
    AGG::PlayMusic( MUS::MAINMENU );
    Settings & conf = Settings::Get();

    // reset last save name
    Game::SetLastSavename( "" );

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    Display & display = Display::Get();

    // load game settings
    conf.BinaryLoad();

    // image background
    const Sprite & back = AGG::GetICN( ICN::HEROES, 0 );
    back.Blit( Point( 0, 0 ) );

    const Sprite & panel = AGG::GetICN( ICN::REDBACK, 0 );
    panel.Blit( back.w() - 235, 5 );

    LocalEvent & le = LocalEvent::Get();

    Button buttonStandartGame( back.w() - 185, 45, ICN::BTNNEWGM, 0, 1 );
    Button buttonCampainGame( back.w() - 185, 110, ICN::BTNNEWGM, 2, 3 );
    Button buttonMultiGame( back.w() - 185, 175, ICN::BTNNEWGM, 4, 5 );
    Button buttonBattleGame( back.w() - 185, 240, ICN::BTNBATTLEONLY, 0, 1 );
    Button buttonSettings( back.w() - 185, 305, ICN::BTNDCCFG, 4, 5 );
    Button buttonCancelGame( back.w() - 185, 375, ICN::BTNNEWGM, 6, 7 );

    buttonStandartGame.Draw();
    buttonCampainGame.Draw();
    buttonMultiGame.Draw();
    if ( conf.QVGA() )
        buttonBattleGame.SetDisable( true );
    else
        buttonBattleGame.Draw();
    buttonSettings.Draw();
    buttonCancelGame.Draw();

    cursor.Show();
    display.Flip();

    while ( le.HandleEvents() ) { // new game loop
        le.MousePressLeft( buttonStandartGame ) ? buttonStandartGame.PressDraw() : buttonStandartGame.ReleaseDraw();
        le.MousePressLeft( buttonCampainGame ) ? buttonCampainGame.PressDraw() : buttonCampainGame.ReleaseDraw();
        le.MousePressLeft( buttonMultiGame ) ? buttonMultiGame.PressDraw() : buttonMultiGame.ReleaseDraw();
        buttonBattleGame.isEnable() && le.MousePressLeft( buttonBattleGame ) ? buttonBattleGame.PressDraw() : buttonBattleGame.ReleaseDraw();
        le.MousePressLeft( buttonSettings ) ? buttonSettings.PressDraw() : buttonSettings.ReleaseDraw();
        le.MousePressLeft( buttonCancelGame ) ? buttonCancelGame.PressDraw() : buttonCancelGame.ReleaseDraw();

        if ( HotKeyPressEvent( EVENT_BUTTON_STANDARD ) || le.MouseClickLeft( buttonStandartGame ) )
            return NEWSTANDARD;
        if ( HotKeyPressEvent( EVENT_BUTTON_CAMPAIN ) || le.MouseClickLeft( buttonCampainGame ) )
            return NEWCAMPAIN;
        if ( HotKeyPressEvent( EVENT_BUTTON_MULTI ) || le.MouseClickLeft( buttonMultiGame ) )
            return NEWMULTI;
        if ( HotKeyPressEvent( EVENT_BUTTON_SETTINGS ) || le.MouseClickLeft( buttonSettings ) ) {
            Dialog::ExtSettings( false );
            cursor.Show();
            display.Flip();
        }
        if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancelGame ) )
            return MAINMENU;

        if ( buttonBattleGame.isEnable() && ( HotKeyPressEvent( EVENT_BUTTON_BATTLEONLY ) || le.MouseClickLeft( buttonBattleGame ) ) )
            return NEWBATTLEONLY;

        if ( le.MousePressRight( buttonStandartGame ) )
            Dialog::Message( _( "Standard Game" ), _( "A single player game playing out a single map." ), Font::BIG );
        if ( le.MousePressRight( buttonCampainGame ) )
            Dialog::Message( _( "Campaign Game" ), _( "A single player game playing through a series of maps." ), Font::BIG );
        if ( le.MousePressRight( buttonMultiGame ) )
            Dialog::Message( _( "Multi-Player Game" ), _( "A multi-player game, with several human players completing against each other on a single map." ), Font::BIG );
        if ( le.MousePressRight( buttonSettings ) )
            Dialog::Message( _( "Settings" ), _( "FHeroes2 game settings." ), Font::BIG );
        if ( le.MousePressRight( buttonCancelGame ) )
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );
    }

    return QUITGAME;
}

int Game::NewMulti( void )
{
    Settings & conf = Settings::Get();

    if ( !( conf.GameType( Game::TYPE_BATTLEONLY ) ) )
        conf.SetGameType( Game::TYPE_STANDARD );

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    Display & display = Display::Get();

    // image background
    const Sprite & back = AGG::GetICN( ICN::HEROES, 0 );
    back.Blit( Point( 0, 0 ) );

    const Sprite & panel = AGG::GetICN( ICN::REDBACK, 0 );
    panel.Blit( back.w() - 235, 5 );

    LocalEvent & le = LocalEvent::Get();

    Button buttonHotSeat( back.w() - 185, 45, ICN::BTNMP, 0, 1 );
    Button buttonNetwork( back.w() - 185, 110, ICN::BTNMP, 2, 3 );
    Button buttonCancelGame( back.w() - 185, 375, ICN::BTNMP, 8, 9 );

    buttonHotSeat.Draw();
    buttonCancelGame.Draw();
    buttonNetwork.SetDisable( true );

    cursor.Show();
    display.Flip();

    // newgame loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonHotSeat ) ? buttonHotSeat.PressDraw() : buttonHotSeat.ReleaseDraw();
        le.MousePressLeft( buttonCancelGame ) ? buttonCancelGame.PressDraw() : buttonCancelGame.ReleaseDraw();

        if ( le.MouseClickLeft( buttonHotSeat ) || HotKeyPressEvent( EVENT_BUTTON_HOTSEAT ) )
            return NEWHOTSEAT;
        if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancelGame ) )
            return MAINMENU;

        // right info
        if ( le.MousePressRight( buttonHotSeat ) )
            Dialog::Message( _( "Hot Seat" ),
                             _( "Play a Hot Seat game, where 2 to 4 players play around the same computer, switching into the 'Hot Seat' when it is their turn." ),
                             Font::BIG );
        if ( le.MousePressRight( buttonCancelGame ) )
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );

#ifdef NETWORK_ENABLE
        if ( buttonNetwork.isEnable() ) {
            le.MousePressLeft( buttonNetwork ) ? buttonNetwork.PressDraw() : buttonNetwork.ReleaseDraw();
            if ( le.MouseClickLeft( buttonNetwork ) || HotKeyPressEvent( EVENT_BUTTON_NETWORK ) )
                return NEWNETWORK;
            if ( le.MousePressRight( buttonNetwork ) )
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

    Display & display = Display::Get();

    // image background
    const Sprite & back = AGG::GetICN( ICN::HEROES, 0 );
    back.Blit( Point( 0, 0 ) );

    const Sprite & panel = AGG::GetICN( ICN::REDBACK, 0 );
    panel.Blit( back.w() - 235, 5 );

    LocalEvent & le = LocalEvent::Get();

    Button button2Players( back.w() - 185, 45, ICN::BTNHOTST, 0, 1 );
    Button button3Players( back.w() - 185, 110, ICN::BTNHOTST, 2, 3 );
    Button button4Players( back.w() - 185, 175, ICN::BTNHOTST, 4, 5 );
    Button button5Players( back.w() - 185, 240, ICN::BTNHOTST, 6, 7 );
    Button button6Players( back.w() - 185, 305, ICN::BTNHOTST, 8, 9 );
    Button buttonCancel( back.w() - 185, 375, ICN::BTNNEWGM, 6, 7 );

    button2Players.Draw();
    button3Players.Draw();
    button4Players.Draw();
    button5Players.Draw();
    button6Players.Draw();
    buttonCancel.Draw();

    cursor.Show();
    display.Flip();

    // newgame loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( button2Players ) ? button2Players.PressDraw() : button2Players.ReleaseDraw();
        le.MousePressLeft( button3Players ) ? button3Players.PressDraw() : button3Players.ReleaseDraw();
        le.MousePressLeft( button4Players ) ? button4Players.PressDraw() : button4Players.ReleaseDraw();
        le.MousePressLeft( button5Players ) ? button5Players.PressDraw() : button5Players.ReleaseDraw();
        le.MousePressLeft( button6Players ) ? button6Players.PressDraw() : button6Players.ReleaseDraw();

        le.MousePressLeft( buttonCancel ) ? buttonCancel.PressDraw() : buttonCancel.ReleaseDraw();

        if ( le.MouseClickLeft( button2Players ) || le.KeyPress( KEY_2 ) )
            return 2;
        if ( le.MouseClickLeft( button3Players ) || le.KeyPress( KEY_3 ) )
            return 3;
        if ( le.MouseClickLeft( button4Players ) || le.KeyPress( KEY_4 ) )
            return 4;
        if ( le.MouseClickLeft( button5Players ) || le.KeyPress( KEY_5 ) )
            return 5;
        if ( le.MouseClickLeft( button6Players ) || le.KeyPress( KEY_6 ) )
            return 6;

        if ( HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancel ) )
            return 0;

        // right info
        if ( le.MousePressRight( button2Players ) )
            Dialog::Message( _( "2 Players" ), _( "Play with 2 human players, and optionally, up, to 4 additional computer players." ), Font::BIG );
        if ( le.MousePressRight( button3Players ) )
            Dialog::Message( _( "3 Players" ), _( "Play with 3 human players, and optionally, up, to 3 additional computer players." ), Font::BIG );
        if ( le.MousePressRight( button4Players ) )
            Dialog::Message( _( "4 Players" ), _( "Play with 4 human players, and optionally, up, to 2 additional computer players." ), Font::BIG );
        if ( le.MousePressRight( button5Players ) )
            Dialog::Message( _( "5 Players" ), _( "Play with 5 human players, and optionally, up, to 1 additional computer players." ), Font::BIG );
        if ( le.MousePressRight( button6Players ) )
            Dialog::Message( _( "6 Players" ), _( "Play with 6 human players." ), Font::BIG );
        if ( le.MousePressRight( buttonCancel ) )
            Dialog::Message( _( "Cancel" ), _( "Cancel back to the main menu." ), Font::BIG );
    }

    return 0;
}
