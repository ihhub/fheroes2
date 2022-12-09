/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "agg_image.h"
#include "audio.h"
#include "audio_manager.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectscenario.h"
#include "difficulty.h"
#include "game.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_mainmenu_ui.h"
#include "game_mode.h"
#include "gamedefs.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "logging.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "mus.h"
#include "player_info.h"
#include "players.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "world.h"

namespace
{
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

    void RedrawScenarioStaticInfo( const fheroes2::Rect & rt, bool firstDraw = false )
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

        fheroes2::FontType normalWhiteFont = fheroes2::FontType::normalWhite();

        // text scenario
        fheroes2::Text text( _( "Scenario:" ), normalWhiteFont );
        text.draw( rt.x, rt.y + 25, rt.width, display );

        // maps name
        text.set( conf.MapsName(), normalWhiteFont );
        text.draw( rt.x, rt.y + 48, rt.width, display );

        // text game difficulty
        text.set( _( "Game Difficulty:" ), normalWhiteFont );
        text.draw( rt.x, rt.y + 75, rt.width, display );

        // text opponents
        text.set( _( "Opponents:" ), normalWhiteFont );
        text.draw( rt.x, rt.y + 180, rt.width, display );

        // text class
        text.set( _( "Class:" ), normalWhiteFont );
        text.draw( rt.x, rt.y + 264, rt.width, display );
    }

    void RedrawDifficultyInfo( const fheroes2::Point & dst )
    {
        const int32_t width = 77;
        const int32_t height = 69;

        for ( int32_t current = Difficulty::EASY; current <= Difficulty::IMPOSSIBLE; ++current ) {
            const int32_t offset = width * current;
            int32_t normalSpecificOffset = 0;
            // Add offset shift because the original difficulty icons have irregular spacing.
            if ( current == Difficulty::NORMAL ) {
                normalSpecificOffset = 1;
            }

            fheroes2::Text text( Difficulty::String( current ), fheroes2::FontType::smallWhite() );
            text.draw( dst.x + 31 + offset + normalSpecificOffset - ( text.width() / 2 ), dst.y + height, fheroes2::Display::instance() );
        }
    }

    fheroes2::Rect RedrawRatingInfo( const fheroes2::Point & offset, int32_t width_ )
    {
        std::string str( _( "Rating %{rating}%" ) );
        StringReplace( str, "%{rating}", Game::GetRating() );

        const fheroes2::Text text( str, fheroes2::FontType::normalWhite() );
        const int32_t y = offset.y + 385;
        text.draw( offset.x, y, width_, fheroes2::Display::instance() );

        const int32_t textX = ( width_ > text.width() ) ? offset.x + ( width_ - text.width() ) / 2 : 0;

        return { textX, y, text.width(), text.height() };
    }

    fheroes2::GameMode ChooseNewMap( const MapsFileInfoList & lists )
    {
        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::NGHSBKG, 0 );
        const fheroes2::Rect rectPanel( ( display.width() - panel.width() ) / 2, ( display.height() - panel.height() ) / 2, panel.width(), panel.height() );
        const fheroes2::Point pointDifficultyInfo( rectPanel.x + 24, rectPanel.y + 95 );
        const fheroes2::Point pointOpponentInfo( rectPanel.x + 24, rectPanel.y + 197 );
        const fheroes2::Point pointClassInfo( rectPanel.x + 24, rectPanel.y + 281 );

        const fheroes2::Sprite & ngextra = fheroes2::AGG::GetICN( ICN::NGEXTRA, 62 );

        const int32_t ngextraWidth = ngextra.width();
        const int32_t ngextraHeight = ngextra.height();

        // vector coord difficulty
        std::vector<fheroes2::Rect> coordDifficulty;
        coordDifficulty.reserve( 5 );

        coordDifficulty.emplace_back( rectPanel.x + 21, rectPanel.y + 91, ngextraWidth, ngextraHeight );
        coordDifficulty.emplace_back( rectPanel.x + 98, rectPanel.y + 91, ngextraWidth, ngextraHeight );
        coordDifficulty.emplace_back( rectPanel.x + 174, rectPanel.y + 91, ngextraWidth, ngextraHeight );
        coordDifficulty.emplace_back( rectPanel.x + 251, rectPanel.y + 91, ngextraWidth, ngextraHeight );
        coordDifficulty.emplace_back( rectPanel.x + 328, rectPanel.y + 91, ngextraWidth, ngextraHeight );

        fheroes2::Button buttonSelectMaps( rectPanel.x + 309, rectPanel.y + 45, ICN::NGEXTRA, 64, 65 );
        fheroes2::Button buttonOk( rectPanel.x + 31, rectPanel.y + 380, ICN::NGEXTRA, 66, 67 );
        fheroes2::Button buttonCancel( rectPanel.x + 287, rectPanel.y + 380, ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1 );

        fheroes2::drawMainMenuScreen();

        Settings & conf = Settings::Get();
        bool resetStartingSettings = conf.MapsFile().empty();
        Players & players = conf.GetPlayers();
        Interface::PlayersInfo playersInfo;

        const int humanPlayerCount = Settings::Get().PreferablyCountPlayers();

        if ( !resetStartingSettings ) { // verify that current map really exists in map's list
            resetStartingSettings = true;
            const std::string & mapName = conf.CurrentFileInfo().name;
            const std::string & mapFileName = System::GetBasename( conf.CurrentFileInfo().file );
            for ( const Maps::FileInfo & mapInfo : lists ) {
                if ( ( mapInfo.name == mapName ) && ( System::GetBasename( mapInfo.file ) == mapFileName ) ) {
                    if ( mapInfo.file == conf.CurrentFileInfo().file ) {
                        conf.SetCurrentFileInfo( mapInfo );
                        updatePlayers( players, humanPlayerCount );
                        Game::LoadPlayers( mapInfo.file, players );
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
            Game::LoadPlayers( lists.front().file, players );
        }

        playersInfo.UpdateInfo( players, pointOpponentInfo, pointClassInfo );

        RedrawScenarioStaticInfo( rectPanel, true );
        RedrawDifficultyInfo( pointDifficultyInfo );

        playersInfo.RedrawInfo( false );

        fheroes2::Rect ratingRoi = RedrawRatingInfo( rectPanel.getPosition(), rectPanel.width );

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
        default:
            // Did you add a new difficulty mode? Add the corresponding case above!
            assert( 0 );
            break;
        }
        levelCursor.redraw();

        buttonSelectMaps.draw();
        buttonOk.draw();
        buttonCancel.draw();

        display.render();

        fheroes2::GameMode result = fheroes2::GameMode::QUIT_GAME;
        LocalEvent & le = LocalEvent::Get();
        while ( true ) {
            if ( !le.HandleEvents( true, true ) ) {
                if ( Interface::Basic::EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                    if ( Settings::isFadeEffectEnabled() ) {
                        fheroes2::FadeDisplay();
                    }
                    return fheroes2::GameMode::QUIT_GAME;
                }

                continue;
            }

            // press button
            le.MousePressLeft( buttonSelectMaps.area() ) ? buttonSelectMaps.drawOnPress() : buttonSelectMaps.drawOnRelease();
            le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
            le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

            // click select
            if ( HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_SELECT_MAP ) || le.MouseClickLeft( buttonSelectMaps.area() ) ) {
                const Maps::FileInfo * fi = Dialog::SelectScenario( lists );

                if ( fi ) {
                    Game::SavePlayers( conf.CurrentFileInfo().file, conf.GetPlayers() );
                    conf.SetCurrentFileInfo( *fi );
                    Game::LoadPlayers( fi->file, players );

                    updatePlayers( players, humanPlayerCount );
                    playersInfo.UpdateInfo( players, pointOpponentInfo, pointClassInfo );

                    RedrawScenarioStaticInfo( rectPanel );
                    RedrawDifficultyInfo( pointDifficultyInfo );
                    playersInfo.resetSelection();
                    playersInfo.RedrawInfo( false );
                    ratingRoi = RedrawRatingInfo( rectPanel.getPosition(), rectPanel.width );
                    levelCursor.setPosition( coordDifficulty[Game::getDifficulty()].x, coordDifficulty[Game::getDifficulty()].y ); // From 0 to 4, see: Difficulty enum
                    buttonOk.draw();
                    buttonCancel.draw();
                }

                display.render();
            }
            else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancel.area() ) ) {
                result = fheroes2::GameMode::MAIN_MENU;
                break;
            }
            else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOk.area() ) ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "select maps: " << conf.MapsFile() << ", difficulty: " << Difficulty::String( Game::getDifficulty() ) )
                result = fheroes2::GameMode::START_GAME;
                break;
            }
            else if ( le.MouseClickLeft( rectPanel ) ) {
                const int32_t index = GetRectIndex( coordDifficulty, le.GetMouseCursor() );

                // select difficulty
                if ( 0 <= index ) {
                    RedrawScenarioStaticInfo( rectPanel );
                    levelCursor.setPosition( coordDifficulty[index].x, coordDifficulty[index].y );
                    levelCursor.redraw();
                    Game::saveDifficulty( index );
                    RedrawDifficultyInfo( pointDifficultyInfo );
                    playersInfo.RedrawInfo( false );
                    ratingRoi = RedrawRatingInfo( rectPanel.getPosition(), rectPanel.width );
                    buttonOk.draw();
                    buttonCancel.draw();
                    display.render();
                }
                // playersInfo
                else if ( playersInfo.QueueEventProcessing() ) {
                    RedrawScenarioStaticInfo( rectPanel );
                    levelCursor.redraw();
                    RedrawDifficultyInfo( pointDifficultyInfo );

                    playersInfo.RedrawInfo( false );
                    ratingRoi = RedrawRatingInfo( rectPanel.getPosition(), rectPanel.width );
                    buttonOk.draw();
                    buttonCancel.draw();
                    display.render();
                }
            }
            else if ( le.MouseWheelUp() || le.MouseWheelDn() ) {
                if ( playersInfo.QueueEventProcessing() ) {
                    playersInfo.resetSelection();

                    RedrawScenarioStaticInfo( rectPanel );
                    levelCursor.redraw();
                    RedrawDifficultyInfo( pointDifficultyInfo );

                    playersInfo.RedrawInfo( false );
                    ratingRoi = RedrawRatingInfo( rectPanel.getPosition(), rectPanel.width );
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
                else if ( le.MousePressRight( ratingRoi ) )
                    Dialog::
                        Message( _( "Difficulty Rating" ),
                                 _( "The difficulty rating reflects a combination of various settings for your game. This number will be applied to your final score." ),
                                 Font::BIG );
                else if ( le.MousePressRight( buttonOk.area() ) )
                    Dialog::Message( _( "Okay" ), _( "Click to accept these settings and start a new game." ), Font::BIG );
                else if ( le.MousePressRight( buttonCancel.area() ) )
                    Dialog::Message( _( "Cancel" ), _( "Click to return to the main menu." ), Font::BIG );
                else
                    playersInfo.QueueEventProcessing();
            }
        }

        Game::SavePlayers( conf.CurrentFileInfo().file, conf.GetPlayers() );

        return result;
    }

    fheroes2::GameMode LoadNewMap()
    {
        Settings & conf = Settings::Get();

        conf.GetPlayers().SetStartGame();
        if ( Settings::isFadeEffectEnabled() ) {
            fheroes2::FadeDisplay();
        }

        // Load maps
        std::string lower = StringLower( conf.MapsFile() );

        if ( lower.size() > 3 ) {
            std::string ext = lower.substr( lower.size() - 3 );

            if ( ext == "mp2" || ext == "mx2" ) {
                return world.LoadMapMP2( conf.MapsFile() ) ? fheroes2::GameMode::START_GAME : fheroes2::GameMode::MAIN_MENU;
            }

            DEBUG_LOG( DBG_GAME, DBG_WARN,
                       conf.MapsFile() << ", "
                                       << "unknown map format" )
            return fheroes2::GameMode::MAIN_MENU;
        }

        DEBUG_LOG( DBG_GAME, DBG_WARN,
                   conf.MapsFile() << ", "
                                   << "unknown map format" )
        return fheroes2::GameMode::MAIN_MENU;
    }
}

fheroes2::GameMode Game::SelectScenario()
{
    return fheroes2::GameMode::SCENARIO_INFO;
}

fheroes2::GameMode Game::ScenarioInfo()
{
    AudioManager::PlayMusicAsync( MUS::MAINMENU, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

    const MapsFileInfoList lists = Maps::PrepareMapsFileInfoList( Settings::Get().IsGameType( Game::TYPE_MULTI ) );
    if ( lists.empty() ) {
        Dialog::Message( _( "Warning" ), _( "No maps available!" ), Font::BIG, Dialog::OK );
        return fheroes2::GameMode::MAIN_MENU;
    }

    // We must release UI resources for this window before loading a new map. That's why all UI logic is in a separate function.
    const fheroes2::GameMode result = ChooseNewMap( lists );
    if ( result != fheroes2::GameMode::START_GAME ) {
        return result;
    }

    return LoadNewMap();
}

int32_t Game::GetStep4Player( const int32_t currentId, const int32_t width, const int32_t totalCount )
{
    return currentId * width * KINGDOMMAX / totalCount + ( width * ( KINGDOMMAX - totalCount ) / ( 2 * totalCount ) );
}
