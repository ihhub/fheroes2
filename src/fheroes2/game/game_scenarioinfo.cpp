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

#include <algorithm>
#include <string>
#include <vector>

#include "agg.h"
#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectscenario.h"
#include "difficulty.h"
#include "game.h"
#include "game_interface.h"
#include "game_mainmenu_ui.h"
#include "gamedefs.h"
#include "icn.h"
#include "logging.h"
#include "maps_fileinfo.h"
#include "mus.h"
#include "player_info.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "tools.h"
#include "ui_button.h"
#include "ui_tool.h"
#include "world.h"

void RedrawScenarioStaticInfo( const fheroes2::Rect & rt, bool firstDraw = false );
void RedrawRatingInfo( TextSprite & );
void RedrawDifficultyInfo( const fheroes2::Point & dst );

fheroes2::GameMode Game::SelectScenario( void )
{
    return fheroes2::GameMode::SCENARIO_INFO;
}

void updatePlayers( Players & players, const int humanPlayerCount )
{
    if ( humanPlayerCount < 2 )
        return;

    int foundHumans = 0;

    for ( size_t i = 0; i < players.size(); ++i ) {
        if ( players[i]->isControlHuman() ) {
            ++foundHumans;
            if ( players[i]->isControlAI() )
                players[i]->SetControl( CONTROL_HUMAN );
        }

        if ( foundHumans == humanPlayerCount )
            break;
    }
}

size_t GetSelectedMapId( MapsFileInfoList & lists )
{
    const Settings & conf = Settings::Get();

    const std::string & mapName = conf.CurrentFileInfo().name;
    const std::string & mapFileName = System::GetBasename( conf.CurrentFileInfo().file );
    size_t mapId = 0;
    for ( MapsFileInfoList::const_iterator mapIter = lists.begin(); mapIter != lists.end(); ++mapIter, ++mapId ) {
        if ( ( mapIter->name == mapName ) && ( System::GetBasename( mapIter->file ) == mapFileName ) ) {
            return mapId;
        }
    }

    return 0;
}

fheroes2::GameMode Game::ScenarioInfo()
{
    Settings & conf = Settings::Get();

    AGG::PlayMusic( MUS::MAINMENU, true, true );

    MapsFileInfoList lists;
    if ( !PrepareMapsFileInfoList( lists, ( conf.IsGameType( Game::TYPE_MULTI ) ) ) ) {
        Dialog::Message( _( "Warning" ), _( "No maps available!" ), Font::BIG, Dialog::OK );
        return fheroes2::GameMode::MAIN_MENU;
    }

    fheroes2::GameMode result = fheroes2::GameMode::QUIT_GAME;
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::Point pointDifficultyInfo, pointOpponentInfo, pointClassInfo;
    fheroes2::Rect rectPanel;

    // vector coord difficulty
    std::vector<fheroes2::Rect> coordDifficulty;
    coordDifficulty.reserve( 5 );

    const fheroes2::Sprite & ngextra = fheroes2::AGG::GetICN( ICN::NGEXTRA, 62 );
    const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::NGHSBKG, 0 );

    rectPanel = fheroes2::Rect( ( display.width() - panel.width() ) / 2, ( display.height() - panel.height() ) / 2, panel.width(), panel.height() );
    pointDifficultyInfo = fheroes2::Point( rectPanel.x + 24, rectPanel.y + 93 );
    pointOpponentInfo = fheroes2::Point( rectPanel.x + 24, rectPanel.y + 202 );
    pointClassInfo = fheroes2::Point( rectPanel.x + 24, rectPanel.y + 282 );

    const uint32_t ngextraWidth = ngextra.width();
    const uint32_t ngextraHeight = ngextra.height();
    coordDifficulty.emplace_back( rectPanel.x + 21, rectPanel.y + 91, ngextraWidth, ngextraHeight );
    coordDifficulty.emplace_back( rectPanel.x + 98, rectPanel.y + 91, ngextraWidth, ngextraHeight );
    coordDifficulty.emplace_back( rectPanel.x + 174, rectPanel.y + 91, ngextraWidth, ngextraHeight );
    coordDifficulty.emplace_back( rectPanel.x + 251, rectPanel.y + 91, ngextraWidth, ngextraHeight );
    coordDifficulty.emplace_back( rectPanel.x + 328, rectPanel.y + 91, ngextraWidth, ngextraHeight );

    fheroes2::Button buttonSelectMaps( rectPanel.x + 309, rectPanel.y + 45, ICN::NGEXTRA, 64, 65 );
    fheroes2::Button buttonOk( rectPanel.x + 31, rectPanel.y + 380, ICN::NGEXTRA, 66, 67 );
    fheroes2::Button buttonCancel( rectPanel.x + 287, rectPanel.y + 380, ICN::NGEXTRA, 68, 69 );

    fheroes2::drawMainMenuScreen();

    bool resetStartingSettings = conf.MapsFile().empty();
    Players & players = conf.GetPlayers();
    Interface::PlayersInfo playersInfo( true, true, true );

    const int humanPlayerCount = Settings::Get().PreferablyCountPlayers();

    if ( !resetStartingSettings ) { // verify that current map really exists in map's list
        resetStartingSettings = true;
        const std::string & mapName = conf.CurrentFileInfo().name;
        const std::string & mapFileName = System::GetBasename( conf.CurrentFileInfo().file );
        for ( MapsFileInfoList::const_iterator mapIter = lists.begin(); mapIter != lists.end(); ++mapIter ) {
            if ( ( mapIter->name == mapName ) && ( System::GetBasename( mapIter->file ) == mapFileName ) ) {
                if ( mapIter->file == conf.CurrentFileInfo().file ) {
                    conf.SetCurrentFileInfo( *mapIter );
                    updatePlayers( players, humanPlayerCount );
                    LoadPlayers( mapIter->file, players );
                    resetStartingSettings = false;
                    break;
                }
            }
        }
    }

    // set first map's settings
    if ( resetStartingSettings ) {
        conf.SetCurrentFileInfo( lists.front() );
        updatePlayers( players, humanPlayerCount );
        LoadPlayers( lists.front().file, players );
    }

    playersInfo.UpdateInfo( players, pointOpponentInfo, pointClassInfo );

    RedrawScenarioStaticInfo( rectPanel, true );
    RedrawDifficultyInfo( pointDifficultyInfo );

    playersInfo.RedrawInfo();

    TextSprite rating;
    rating.SetFont( Font::BIG );
    rating.SetPos( rectPanel.x + 166, rectPanel.y + 383 );
    RedrawRatingInfo( rating );

    fheroes2::MovableSprite levelCursor( ngextra );

    switch ( Game::getDifficulty() ) {
    case Difficulty::EASY:
        levelCursor.setPosition( coordDifficulty[0].x, coordDifficulty[0].y );
        break;
    case Difficulty::NORMAL:
        levelCursor.setPosition( coordDifficulty[1].x, coordDifficulty[1].y );
        break;
    case Difficulty::HARD:
        levelCursor.setPosition( coordDifficulty[2].x, coordDifficulty[2].y );
        break;
    case Difficulty::EXPERT:
        levelCursor.setPosition( coordDifficulty[3].x, coordDifficulty[3].y );
        break;
    case Difficulty::IMPOSSIBLE:
        levelCursor.setPosition( coordDifficulty[4].x, coordDifficulty[4].y );
        break;
    }
    levelCursor.redraw();

    buttonSelectMaps.draw();
    buttonOk.draw();
    buttonCancel.draw();

    display.render();

    while ( 1 ) {
        if ( !le.HandleEvents( true, true ) ) {
            if ( Interface::Basic::EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                if ( conf.ExtGameUseFade() )
                    fheroes2::FadeDisplay();
                return fheroes2::GameMode::QUIT_GAME;
            }
            else {
                continue;
            }
        }

        // press button
        le.MousePressLeft( buttonSelectMaps.area() ) ? buttonSelectMaps.drawOnPress() : buttonSelectMaps.drawOnRelease();
        le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        // click select
        if ( HotKeyPressEvent( Game::EVENT_BUTTON_SELECT ) || le.MouseClickLeft( buttonSelectMaps.area() ) ) {
            const Maps::FileInfo * fi = Dialog::SelectScenario( lists, GetSelectedMapId( lists ) );

            if ( fi ) {
                SavePlayers( conf.CurrentFileInfo().file, conf.GetPlayers() );
                conf.SetCurrentFileInfo( *fi );
                LoadPlayers( fi->file, players );

                updatePlayers( players, humanPlayerCount );
                playersInfo.UpdateInfo( players, pointOpponentInfo, pointClassInfo );

                RedrawScenarioStaticInfo( rectPanel );
                RedrawDifficultyInfo( pointDifficultyInfo );
                playersInfo.resetSelection();
                playersInfo.RedrawInfo();
                RedrawRatingInfo( rating );
                levelCursor.setPosition( coordDifficulty[Game::getDifficulty()].x, coordDifficulty[Game::getDifficulty()].y ); // From 0 to 4, see: Difficulty enum
                buttonOk.draw();
                buttonCancel.draw();
            }

            display.render();
        }
        else
            // click cancel
            if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancel.area() ) ) {
            result = fheroes2::GameMode::MAIN_MENU;
            break;
        }
        else
            // click ok
            if ( HotKeyPressEvent( EVENT_DEFAULT_READY ) || le.MouseClickLeft( buttonOk.area() ) ) {
            DEBUG_LOG( DBG_GAME, DBG_INFO, "select maps: " << conf.MapsFile() << ", difficulty: " << Difficulty::String( Game::getDifficulty() ) );
            result = fheroes2::GameMode::START_GAME;
            break;
        }
        else if ( le.MouseClickLeft( rectPanel ) ) {
            const s32 index = GetRectIndex( coordDifficulty, le.GetMouseCursor() );

            // select difficulty
            if ( 0 <= index ) {
                levelCursor.setPosition( coordDifficulty[index].x, coordDifficulty[index].y );
                levelCursor.redraw();
                Game::saveDifficulty( index );
                RedrawRatingInfo( rating );
                display.render();
            }
            // playersInfo
            else if ( playersInfo.QueueEventProcessing() ) {
                RedrawScenarioStaticInfo( rectPanel );
                levelCursor.redraw();
                RedrawDifficultyInfo( pointDifficultyInfo );

                playersInfo.RedrawInfo();
                RedrawRatingInfo( rating );
                buttonOk.draw();
                buttonCancel.draw();
                display.render();
            }
        }

        if ( le.MousePressRight( rectPanel ) ) {
            if ( le.MousePressRight( buttonSelectMaps.area() ) )
                Dialog::Message( _( "Scenario" ), _( "Click here to select which scenario to play." ), Font::BIG );
            else if ( 0 <= GetRectIndex( coordDifficulty, le.GetMouseCursor() ) )
                Dialog::Message(
                    _( "Game Difficulty" ),
                    _( "This lets you change the starting difficulty at which you will play. Higher difficulty levels start you of with fewer resources, and at the higher settings, give extra resources to the computer." ),
                    Font::BIG );
            else if ( le.MousePressRight( rating.GetRect() ) )
                Dialog::Message( _( "Difficulty Rating" ),
                                 _( "The difficulty rating reflects a combination of various settings for your game. This number will be applied to your final score." ),
                                 Font::BIG );
            else if ( le.MousePressRight( buttonOk.area() ) )
                Dialog::Message( _( "OK" ), _( "Click to accept these settings and start a new game." ), Font::BIG );
            else if ( le.MousePressRight( buttonCancel.area() ) )
                Dialog::Message( _( "Cancel" ), _( "Click to return to the main menu." ), Font::BIG );
            else
                playersInfo.QueueEventProcessing();
        }
    }

    SavePlayers( conf.CurrentFileInfo().file, conf.GetPlayers() );

    if ( result == fheroes2::GameMode::START_GAME ) {
        players.SetStartGame();
        if ( conf.ExtGameUseFade() )
            fheroes2::FadeDisplay();
        Game::ShowMapLoadingText();
        // Load maps
        std::string lower = StringLower( conf.MapsFile() );

        if ( lower.size() > 3 ) {
            std::string ext = lower.substr( lower.size() - 3 );

            if ( ext == "mp2" || ext == "mx2" ) {
                result = world.LoadMapMP2( conf.MapsFile() ) ? fheroes2::GameMode::START_GAME : fheroes2::GameMode::MAIN_MENU;
            }
            else {
                result = fheroes2::GameMode::MAIN_MENU;
                DEBUG_LOG( DBG_GAME, DBG_WARN,
                           conf.MapsFile() << ", "
                                           << "unknown map format" );
            }
        }
        else {
            result = fheroes2::GameMode::MAIN_MENU;
            DEBUG_LOG( DBG_GAME, DBG_WARN,
                       conf.MapsFile() << ", "
                                       << "unknown map format" );
        }
    }

    return result;
}

int32_t Game::GetStep4Player( const int32_t currentId, const int32_t width, const int32_t totalCount )
{
    return currentId * width * KINGDOMMAX / totalCount + ( width * ( KINGDOMMAX - totalCount ) / ( 2 * totalCount ) );
}

void RedrawScenarioStaticInfo( const fheroes2::Rect & rt, bool firstDraw )
{
    const Settings & conf = Settings::Get();
    fheroes2::Display & display = fheroes2::Display::instance();

    if ( firstDraw ) {
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::NGHSBKG, 1 ), display, rt.x - BORDERWIDTH, rt.y + BORDERWIDTH );
    }

    // image panel
    const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::NGHSBKG, 0 );
    fheroes2::Blit( panel, display, rt.x, rt.y );

    // Redraw select button as the original image has a wrong position of it
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::NGEXTRA, 64 ), display, rt.x + 309, rt.y + 45 );

    // text scenario
    Text text( _( "Scenario:" ), Font::BIG );
    text.Blit( rt.x + ( rt.width - text.w() ) / 2, rt.y + 23 );

    // maps name
    text.Set( conf.MapsName() );
    text.Blit( rt.x + ( rt.width - text.w() ) / 2, rt.y + 46 );

    // text game difficulty
    text.Set( _( "Game Difficulty:" ) );
    text.Blit( rt.x + ( rt.width - text.w() ) / 2, rt.y + 75 );

    // text opponents
    text.Set( _( "Opponents:" ), Font::BIG );
    text.Blit( rt.x + ( rt.width - text.w() ) / 2, rt.y + 181 );

    // text class
    text.Set( _( "Class:" ), Font::BIG );
    text.Blit( rt.x + ( rt.width - text.w() ) / 2, rt.y + 262 );
}

void RedrawDifficultyInfo( const fheroes2::Point & dst )
{
    const uint32_t width = 65;
    const uint32_t height = 69;

    for ( u32 current = Difficulty::EASY; current <= Difficulty::IMPOSSIBLE; ++current ) {
        const uint32_t offset = current * ( width + 12 );
        Text text( Difficulty::String( current ), Font::SMALL );
        text.Blit( dst.x + offset + ( width - text.w() ) / 2, dst.y + height );
    }
}

void RedrawRatingInfo( TextSprite & sprite )
{
    sprite.Hide();
    std::string str( _( "Rating %{rating}%" ) );
    StringReplace( str, "%{rating}", Game::GetRating() );
    sprite.SetText( str );
    sprite.Show();
}
