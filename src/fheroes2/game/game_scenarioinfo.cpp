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
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"
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

    void DrawScenarioStaticInfo( const fheroes2::Rect & rt )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::FontType normalWhiteFont = fheroes2::FontType::normalWhite();

        // text scenario
        fheroes2::Text text( _( "Scenario:" ), normalWhiteFont );
        text.draw( rt.x, rt.y + 9, rt.width, display );

        // text game difficulty
        text.set( _( "Game Difficulty:" ), normalWhiteFont );
        text.draw( rt.x, rt.y + 59, rt.width, display );

        // text opponents
        text.set( _( "Opponents:" ), normalWhiteFont );
        text.draw( rt.x, rt.y + 164, rt.width, display );

        // text class
        text.set( _( "Class:" ), normalWhiteFont );
        text.draw( rt.x, rt.y + 248, rt.width, display );
    }

    void RedrawMapTitle( const fheroes2::Rect & roi )
    {
        const fheroes2::Text text( Settings::Get().getCurrentMapInfo().name, fheroes2::FontType::normalWhite() );
        text.draw( roi.x, roi.y + 8, roi.width, fheroes2::Display::instance() );
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
        const int32_t y = offset.y + 372;
        text.draw( offset.x, y, width_, fheroes2::Display::instance() );

        const int32_t textX = ( width_ > text.width() ) ? offset.x + ( width_ - text.width() ) / 2 : 0;

        return { textX, y, text.width(), text.height() };
    }

    fheroes2::GameMode ChooseNewMap( const MapsFileInfoList & lists, const int humanPlayerCount )
    {
        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        fheroes2::drawMainMenuScreen();

        fheroes2::StandardWindow background( 388, 397, true, display );

        const fheroes2::Rect roi( background.activeArea() );

        const fheroes2::Point pointDifficultyInfo( roi.x + 8, roi.y + 79 );
        const fheroes2::Point pointOpponentInfo( roi.x + 8, roi.y + 181 );
        const fheroes2::Point pointClassInfo( roi.x + 8, roi.y + 265 );

        const fheroes2::Sprite & scenarioBox = fheroes2::AGG::GetICN( isEvilInterface ? ICN::METALLIC_BORDERED_TEXTBOX_EVIL : ICN::METALLIC_BORDERED_TEXTBOX_GOOD, 0 );

        const fheroes2::Rect scenarioBoxRoi( roi.x + ( roi.width - scenarioBox.width() ) / 2, roi.y + 24, scenarioBox.width(), scenarioBox.height() );

        fheroes2::Copy( scenarioBox, 0, 0, display, scenarioBoxRoi );
        fheroes2::addGradientShadow( scenarioBox, display, scenarioBoxRoi.getPosition(), { -5, 5 } );

        const fheroes2::Sprite & difficultyCursor = fheroes2::AGG::GetICN( ICN::NGEXTRA, 62 );

        const int32_t difficultyCursorWidth = difficultyCursor.width();
        const int32_t difficultyCursorHeight = difficultyCursor.height();

        // vector coord difficulty
        std::vector<fheroes2::Rect> coordDifficulty;
        coordDifficulty.reserve( 5 );

        coordDifficulty.emplace_back( roi.x + 8, roi.y + 78, difficultyCursorWidth, difficultyCursorHeight );
        coordDifficulty.emplace_back( roi.x + 85, roi.y + 78, difficultyCursorWidth, difficultyCursorHeight );
        coordDifficulty.emplace_back( roi.x + 161, roi.y + 78, difficultyCursorWidth, difficultyCursorHeight );
        coordDifficulty.emplace_back( roi.x + 238, roi.y + 78, difficultyCursorWidth, difficultyCursorHeight );
        coordDifficulty.emplace_back( roi.x + 315, roi.y + 78, difficultyCursorWidth, difficultyCursorHeight );

        const int32_t buttonSelectWidth = fheroes2::AGG::GetICN( ICN::BUTTON_MAP_SELECT_GOOD, 0 ).width();

        fheroes2::Button buttonSelectMaps( scenarioBoxRoi.x + scenarioBoxRoi.width - 6 - buttonSelectWidth, scenarioBoxRoi.y + 5,
                                           isEvilInterface ? ICN::BUTTON_MAP_SELECT_EVIL : ICN::BUTTON_MAP_SELECT_GOOD, 0, 1 );
        buttonSelectMaps.draw();

        fheroes2::Button buttonOk;
        fheroes2::Button buttonCancel;

        const fheroes2::Point buttonOffset( 20, 6 );

        const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        background.renderButton( buttonOk, buttonOkIcn, 0, 1, buttonOffset, fheroes2::StandardWindow::Padding::BOTTOM_LEFT );

        const int buttonCancelIcn = isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD;
        background.renderButton( buttonCancel, buttonCancelIcn, 0, 1, buttonOffset, fheroes2::StandardWindow::Padding::BOTTOM_RIGHT );

        bool resetStartingSettings = conf.getCurrentMapInfo().filename.empty();
        Players & players = conf.GetPlayers();
        Interface::PlayersInfo playersInfo;

        if ( !resetStartingSettings ) { // verify that current map really exists in map's list
            resetStartingSettings = true;
            const std::string & mapName = conf.getCurrentMapInfo().name;
            const std::string & mapFileName = System::GetBasename( conf.getCurrentMapInfo().filename );
            for ( const Maps::FileInfo & mapInfo : lists ) {
                if ( ( mapInfo.name == mapName ) && ( System::GetBasename( mapInfo.filename ) == mapFileName ) ) {
                    if ( mapInfo.filename == conf.getCurrentMapInfo().filename ) {
                        conf.SetCurrentFileInfo( mapInfo );
                        updatePlayers( players, humanPlayerCount );
                        Game::LoadPlayers( mapInfo.filename, players );
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
            Game::LoadPlayers( lists.front().filename, players );
        }

        playersInfo.UpdateInfo( players, pointOpponentInfo, pointClassInfo );

        DrawScenarioStaticInfo( roi );
        RedrawDifficultyInfo( pointDifficultyInfo );

        const int icnIndex = isEvilInterface ? 1 : 0;

        // Draw difficulty icons.
        for ( int i = 0; i < 5; ++i ) {
            const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::DIFFICULTY_ICON_EASY + i, icnIndex );
            fheroes2::Copy( icon, 0, 0, display, coordDifficulty[i] );
            fheroes2::addGradientShadow( icon, display, { coordDifficulty[i].x, coordDifficulty[i].y }, { -5, 5 } );
        }

        // Set up restorers.
        fheroes2::ImageRestorer mapTitleArea( display, scenarioBoxRoi.x + 113, scenarioBoxRoi.y + 5, 141, scenarioBoxRoi.height );
        fheroes2::ImageRestorer opponentsArea( display, roi.x, pointOpponentInfo.y, roi.width, 65 );
        fheroes2::ImageRestorer classArea( display, roi.x, pointClassInfo.y, roi.width, 69 );
        fheroes2::ImageRestorer handicapArea( display, roi.x, pointClassInfo.y + 69, roi.width, 31 );
        fheroes2::ImageRestorer ratingArea( display, buttonOk.area().x + buttonOk.area().width, buttonOk.area().y,
                                            roi.width - buttonOk.area().width - buttonCancel.area().width - 20 * 2, buttonOk.area().height );

        // Map name
        RedrawMapTitle( scenarioBoxRoi );

        playersInfo.RedrawInfo( false );

        fheroes2::Rect ratingRoi = RedrawRatingInfo( roi.getPosition(), roi.width );

        fheroes2::MovableSprite levelCursor( difficultyCursor );
        const int32_t levelCursorOffset = 3;

        switch ( Game::getDifficulty() ) {
        case Difficulty::EASY:
            levelCursor.setPosition( coordDifficulty[0].x - levelCursorOffset, coordDifficulty[0].y - levelCursorOffset );
            break;
        case Difficulty::NORMAL:
            levelCursor.setPosition( coordDifficulty[1].x - levelCursorOffset, coordDifficulty[1].y - levelCursorOffset );
            break;
        case Difficulty::HARD:
            levelCursor.setPosition( coordDifficulty[2].x - levelCursorOffset, coordDifficulty[2].y - levelCursorOffset );
            break;
        case Difficulty::EXPERT:
            levelCursor.setPosition( coordDifficulty[3].x - levelCursorOffset, coordDifficulty[3].y - levelCursorOffset );
            break;
        case Difficulty::IMPOSSIBLE:
            levelCursor.setPosition( coordDifficulty[4].x - levelCursorOffset, coordDifficulty[4].y - levelCursorOffset );
            break;
        default:
            // Did you add a new difficulty mode? Add the corresponding case above!
            assert( 0 );
            break;
        }
        levelCursor.redraw();

        display.render();

        fheroes2::GameMode result = fheroes2::GameMode::QUIT_GAME;

        LocalEvent & le = LocalEvent::Get();

        while ( true ) {
            if ( !le.HandleEvents( true, true ) ) {
                if ( Interface::AdventureMap::EventExit() == fheroes2::GameMode::QUIT_GAME ) {
                    fheroes2::fadeOutDisplay();

                    return fheroes2::GameMode::QUIT_GAME;
                }

                continue;
            }

            // press button
            le.isMouseLeftButtonPressedInArea( buttonSelectMaps.area() ) ? buttonSelectMaps.drawOnPress() : buttonSelectMaps.drawOnRelease();
            le.isMouseLeftButtonPressedInArea( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
            le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

            // click select
            if ( HotKeyPressEvent( Game::HotKeyEvent::MAIN_MENU_SELECT_MAP ) || le.MouseClickLeft( buttonSelectMaps.area() ) ) {
                const Maps::FileInfo * fi = Dialog::SelectScenario( lists, false );
                const std::string currentMapName = conf.getCurrentMapInfo().filename;

                if ( fi && fi->filename != currentMapName ) {
                    Game::SavePlayers( currentMapName, conf.GetPlayers() );
                    conf.SetCurrentFileInfo( *fi );

                    mapTitleArea.restore();
                    RedrawMapTitle( scenarioBoxRoi );
                    Game::LoadPlayers( fi->filename, players );

                    opponentsArea.restore();
                    classArea.restore();
                    handicapArea.restore();
                    ratingArea.restore();

                    updatePlayers( players, humanPlayerCount );
                    playersInfo.UpdateInfo( players, pointOpponentInfo, pointClassInfo );

                    playersInfo.resetSelection();
                    playersInfo.RedrawInfo( false );

                    ratingRoi = RedrawRatingInfo( roi.getPosition(), roi.width );
                    levelCursor.setPosition( coordDifficulty[Game::getDifficulty()].x - levelCursorOffset,
                                             coordDifficulty[Game::getDifficulty()].y - levelCursorOffset ); // From 0 to 4, see: Difficulty enum
                }
                display.render();
            }
            else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancel.area() ) ) {
                result = fheroes2::GameMode::MAIN_MENU;
                break;
            }
            else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOk.area() ) ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "select maps: " << conf.getCurrentMapInfo().filename << ", difficulty: " << Difficulty::String( Game::getDifficulty() ) )
                result = fheroes2::GameMode::START_GAME;

                // Fade-out screen before starting a scenario.
                fheroes2::fadeOutDisplay();
                break;
            }
            else if ( le.MouseClickLeft( roi ) ) {
                const int32_t index = GetRectIndex( coordDifficulty, le.getMouseCursorPos() );

                // select difficulty
                if ( 0 <= index ) {
                    levelCursor.setPosition( coordDifficulty[index].x - levelCursorOffset, coordDifficulty[index].y - levelCursorOffset );
                    levelCursor.redraw();
                    Game::saveDifficulty( index );
                    ratingArea.restore();
                    ratingRoi = RedrawRatingInfo( roi.getPosition(), roi.width );

                    display.render( roi );
                }
                // playersInfo
                else if ( playersInfo.QueueEventProcessing() ) {
                    opponentsArea.restore();
                    classArea.restore();
                    handicapArea.restore();
                    playersInfo.RedrawInfo( false );

                    display.render( roi );
                }
            }
            else if ( ( le.isMouseWheelUp() || le.isMouseWheelDown() ) && playersInfo.QueueEventProcessing() ) {
                playersInfo.resetSelection();
                opponentsArea.restore();
                classArea.restore();
                handicapArea.restore();
                playersInfo.RedrawInfo( false );

                display.render( roi );
            }

            if ( le.isMouseRightButtonPressedInArea( roi ) ) {
                if ( le.isMouseRightButtonPressedInArea( buttonSelectMaps.area() ) ) {
                    fheroes2::showStandardTextMessage( _( "Scenario" ), _( "Click here to select which scenario to play." ), Dialog::ZERO );
                }
                else if ( 0 <= GetRectIndex( coordDifficulty, le.getMouseCursorPos() ) ) {
                    fheroes2::showStandardTextMessage(
                        _( "Game Difficulty" ),
                        _( "This lets you change the starting difficulty at which you will play. Higher difficulty levels start you off with fewer resources, and at the higher settings, give extra resources to the computer." ),
                        Dialog::ZERO );
                }
                else if ( le.isMouseRightButtonPressedInArea( ratingRoi ) ) {
                    fheroes2::showStandardTextMessage(
                        _( "Difficulty Rating" ),
                        _( "The difficulty rating reflects a combination of various settings for your game. This number will be applied to your final score." ),
                        Dialog::ZERO );
                }
                else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                    fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to accept these settings and start a new game." ), Dialog::ZERO );
                }
                else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                    fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Click to return to the main menu." ), Dialog::ZERO );
                }
                else {
                    playersInfo.QueueEventProcessing();
                }
            }
        }

        Game::SavePlayers( conf.getCurrentMapInfo().filename, conf.GetPlayers() );

        return result;
    }

    fheroes2::GameMode LoadNewMap()
    {
        Settings & conf = Settings::Get();

        conf.GetPlayers().SetStartGame();

        const Maps::FileInfo & mapInfo = conf.getCurrentMapInfo();

        if ( mapInfo.version == GameVersion::SUCCESSION_WARS || mapInfo.version == GameVersion::PRICE_OF_LOYALTY ) {
            if ( world.LoadMapMP2( mapInfo.filename, ( mapInfo.version == GameVersion::SUCCESSION_WARS ) ) ) {
                return fheroes2::GameMode::START_GAME;
            }

            return fheroes2::GameMode::MAIN_MENU;
        }

        assert( mapInfo.version == GameVersion::RESURRECTION );
        if ( world.loadResurrectionMap( mapInfo.filename ) ) {
            return fheroes2::GameMode::START_GAME;
        }

        return fheroes2::GameMode::MAIN_MENU;
    }
}

fheroes2::GameMode Game::SelectScenario( const uint8_t humanPlayerCount )
{
    assert( humanPlayerCount >= 1 && humanPlayerCount <= 6 );

    AudioManager::PlayMusicAsync( MUS::MAINMENU, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

    const MapsFileInfoList maps = Maps::getAllMapFileInfos( false, humanPlayerCount );
    if ( maps.empty() ) {
        fheroes2::showStandardTextMessage( _( "Warning" ), _( "No maps available!" ), Dialog::OK );
        return fheroes2::GameMode::MAIN_MENU;
    }

    // We must release UI resources for this window before loading a new map. That's why all UI logic is in a separate function.
    const fheroes2::GameMode result = ChooseNewMap( maps, humanPlayerCount );
    if ( result != fheroes2::GameMode::START_GAME ) {
        return result;
    }

    return LoadNewMap();
}

int32_t Game::GetStep4Player( const int32_t currentId, const int32_t width, const int32_t totalCount )
{
    return currentId * width * KINGDOMMAX / totalCount + ( width * ( KINGDOMMAX - totalCount ) / ( 2 * totalCount ) );
}
