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
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectscenario.h"
#include "difficulty.h"
#include "game.h"
#include "game_interface.h"
#include "gamedefs.h"
#include "kingdom.h"
#include "maps_fileinfo.h"
#include "mus.h"
#include "player_info.h"
#include "race.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"
#include "ui_tool.h"
#include "world.h"

void RedrawScenarioStaticInfo( const Rect & rt, bool firstDraw = false );
void RedrawRatingInfo( TextSprite & );
void RedrawDifficultyInfo( const Point & dst );

int Game::SelectScenario( void )
{
    return SCENARIOINFO;
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
    Settings & conf = Settings::Get();

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

int Game::ScenarioInfo( void )
{
    Settings & conf = Settings::Get();

    AGG::PlayMusic( MUS::MAINMENU );

    MapsFileInfoList lists;
    if ( !PrepareMapsFileInfoList( lists, ( conf.IsGameType( Game::TYPE_MULTI ) ) ) ) {
        Dialog::Message( _( "Warning" ), _( "No maps available!" ), Font::BIG, Dialog::OK );
        return MAINMENU;
    }

    int result = QUITGAME;
    LocalEvent & le = LocalEvent::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    Point pointDifficultyInfo, pointOpponentInfo, pointClassInfo;
    Rect rectPanel;

    // vector coord difficulty
    Rects coordDifficulty;
    coordDifficulty.reserve( 5 );

    const fheroes2::Sprite & ngextra = fheroes2::AGG::GetICN( ICN::NGEXTRA, 62 );
    const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::NGHSBKG, 0 );
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HEROES, 0 );

    rectPanel = Rect( ( display.width() - panel.width() ) / 2, ( display.height() - panel.height() ) / 2, panel.width(), panel.height() );
    pointDifficultyInfo = Point( rectPanel.x + 24, rectPanel.y + 93 );
    pointOpponentInfo = Point( rectPanel.x + 24, rectPanel.y + 202 );
    pointClassInfo = Point( rectPanel.x + 24, rectPanel.y + 282 );

    const uint32_t ngextraWidth = ngextra.width();
    const uint32_t ngextraHeight = ngextra.height();
    coordDifficulty.push_back( Rect( rectPanel.x + 21, rectPanel.y + 91, ngextraWidth, ngextraHeight ) );
    coordDifficulty.push_back( Rect( rectPanel.x + 98, rectPanel.y + 91, ngextraWidth, ngextraHeight ) );
    coordDifficulty.push_back( Rect( rectPanel.x + 174, rectPanel.y + 91, ngextraWidth, ngextraHeight ) );
    coordDifficulty.push_back( Rect( rectPanel.x + 251, rectPanel.y + 91, ngextraWidth, ngextraHeight ) );
    coordDifficulty.push_back( Rect( rectPanel.x + 328, rectPanel.y + 91, ngextraWidth, ngextraHeight ) );

    fheroes2::Button buttonSelectMaps( rectPanel.x + 309, rectPanel.y + 45, ICN::NGEXTRA, 64, 65 );
    fheroes2::Button buttonOk( rectPanel.x + 31, rectPanel.y + 380, ICN::NGEXTRA, 66, 67 );
    fheroes2::Button buttonCancel( rectPanel.x + 287, rectPanel.y + 380, ICN::NGEXTRA, 68, 69 );

    fheroes2::Copy( back, display );

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

    switch ( conf.GameDifficulty() ) {
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

    cursor.Show();
    display.render();

    while ( 1 ) {
        if ( !le.HandleEvents( true, true ) ) {
            if ( Interface::Basic::EventExit() == QUITGAME ) {
                if ( conf.ExtGameUseFade() )
                    fheroes2::FadeDisplay();
                return QUITGAME;
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

                cursor.Hide();
                RedrawScenarioStaticInfo( rectPanel );
                RedrawDifficultyInfo( pointDifficultyInfo );
                playersInfo.resetSelection();
                playersInfo.RedrawInfo();
                RedrawRatingInfo( rating );
                levelCursor.setPosition( coordDifficulty[1].x, coordDifficulty[1].y );
                conf.SetGameDifficulty( Difficulty::NORMAL );
                buttonOk.draw();
                buttonCancel.draw();
            }
            cursor.Show();
            display.render();
        }
        else
            // click cancel
            if ( HotKeyPressEvent( EVENT_DEFAULT_EXIT ) || le.MouseClickLeft( buttonCancel.area() ) ) {
            result = MAINMENU;
            break;
        }
        else
            // click ok
            if ( HotKeyPressEvent( EVENT_DEFAULT_READY ) || le.MouseClickLeft( buttonOk.area() ) ) {
            DEBUG( DBG_GAME, DBG_INFO, "select maps: " << conf.MapsFile() << ", difficulty: " << Difficulty::String( conf.GameDifficulty() ) );
            result = STARTGAME;
            break;
        }
        else if ( le.MouseClickLeft( rectPanel ) ) {
            const s32 index = coordDifficulty.GetIndex( le.GetMouseCursor() );

            // select difficulty
            if ( 0 <= index ) {
                cursor.Hide();
                levelCursor.setPosition( coordDifficulty[index].x, coordDifficulty[index].y );
                levelCursor.redraw();
                conf.SetGameDifficulty( index );
                RedrawRatingInfo( rating );
                cursor.Show();
                display.render();
            }
            else
                // playersInfo
                if ( playersInfo.QueueEventProcessing() ) {
                cursor.Hide();
                RedrawScenarioStaticInfo( rectPanel );
                levelCursor.redraw();
                RedrawDifficultyInfo( pointDifficultyInfo );

                playersInfo.RedrawInfo();
                RedrawRatingInfo( rating );
                buttonOk.draw();
                buttonCancel.draw();
                cursor.Show();
                display.render();
            }
        }

        if ( le.MousePressRight( rectPanel ) ) {
            if ( le.MousePressRight( buttonSelectMaps.area() ) )
                Dialog::Message( _( "Scenario" ), _( "Click here to select which scenario to play." ), Font::BIG );
            else if ( 0 <= coordDifficulty.GetIndex( le.GetMouseCursor() ) )
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

    cursor.Hide();

    if ( result == STARTGAME ) {
        players.SetStartGame();
        if ( conf.ExtGameUseFade() )
            fheroes2::FadeDisplay();
        Game::ShowMapLoadingText();
        // Load maps
        std::string lower = StringLower( conf.MapsFile() );

        if ( lower.size() > 3 ) {
            std::string ext = lower.substr( lower.size() - 3 );

            if ( ext == "mp2" || ext == "mx2" ) {
                result = world.LoadMapMP2( conf.MapsFile() ) ? STARTGAME : MAINMENU;
            }
            else if ( ext == "map" ) {
                result = world.LoadMapMAP( conf.MapsFile() ) ? STARTGAME : MAINMENU;
            }
            else {
                result = MAINMENU;
                DEBUG( DBG_GAME, DBG_WARN,
                       conf.MapsFile() << ", "
                                       << "unknown map format" );
            }
        }
        else {
            result = MAINMENU;
            DEBUG( DBG_GAME, DBG_WARN,
                   conf.MapsFile() << ", "
                                   << "unknown map format" );
        }
    }

    return result;
}

u32 Game::GetStep4Player( u32 current, u32 width, u32 count )
{
    return current * width * KINGDOMMAX / count + ( width * ( KINGDOMMAX - count ) / ( 2 * count ) );
}

void RedrawScenarioStaticInfo( const Rect & rt, bool firstDraw )
{
    Settings & conf = Settings::Get();
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
    text.Blit( rt.x + ( rt.w - text.w() ) / 2, rt.y + 23 );

    // maps name
    text.Set( conf.MapsName() );
    text.Blit( rt.x + ( rt.w - text.w() ) / 2, rt.y + 46 );

    // text game difficulty
    text.Set( _( "Game Difficulty:" ) );
    text.Blit( rt.x + ( rt.w - text.w() ) / 2, rt.y + 75 );

    // text opponents
    text.Set( _( "Opponents:" ), Font::BIG );
    text.Blit( rt.x + ( rt.w - text.w() ) / 2, rt.y + 181 );

    // text class
    text.Set( _( "Class:" ), Font::BIG );
    text.Blit( rt.x + ( rt.w - text.w() ) / 2, rt.y + 262 );
}

void RedrawDifficultyInfo( const Point & dst )
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
