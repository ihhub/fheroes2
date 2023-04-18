/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "agg_image.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_io.h"
#include "game_mode.h"
#include "game_over.h"
#include "highscores.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "monster.h"
#include "monster_anim.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "ui_text.h"
#include "ui_window.h"
#include "world.h"

#ifdef WITH_DEBUG
#include "logging.h"
#endif

namespace
{
    const std::string highScoreFileName = "fheroes2.hgs";
    fheroes2::HighScoreDataContainer highScoreDataContainer;
    const int32_t initialHighScoreEntryOffsetY = 72;
    const int32_t highScoreEntryStepY = 40;

    const int32_t playerNameOffset = 88;
    const int32_t scenarioNameOffset = 244;
    const int32_t dayCountOffset = 403;
    const int32_t ratingOffset = 484;

    void redrawHighScoreScreen( int32_t offsetX, int32_t offsetY, uint32_t & monsterAnimationFrameId, const int32_t selectedScoreIndex,
                                const std::vector<fheroes2::HighscoreData> & highScores, const uint32_t titleImageIndex,
                                const std::function<Monster( size_t )> & getMonster )
    {
        ++monsterAnimationFrameId;

        fheroes2::Display & display = fheroes2::Display::instance();

        // Draw background.
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HSBKG, 0 ), display, offsetX, offsetY );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HISCORE, titleImageIndex ), display, offsetX + 50, offsetY + 31 );

        fheroes2::Text text( "", fheroes2::FontType::normalWhite() );

        const std::array<uint8_t, 15> & monsterAnimationSequence = fheroes2::getMonsterAnimationSequence();

        int32_t scoreIndex = 0;

        for ( const fheroes2::HighscoreData & data : highScores ) {
            const fheroes2::FontType font = ( scoreIndex == selectedScoreIndex ) ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite();

            const fheroes2::LanguageSwitcher languageSwitcher( fheroes2::getLanguageFromAbbreviation( data.languageAbbreviation ) );

            text.set( data.playerName, font );
            text.draw( offsetX + playerNameOffset, offsetY + initialHighScoreEntryOffsetY, display );

            text.set( data.scenarioName, font );
            text.draw( offsetX + scenarioNameOffset, offsetY + initialHighScoreEntryOffsetY, display );

            text.set( std::to_string( data.dayCount ), font );
            text.draw( offsetX + dayCountOffset, offsetY + initialHighScoreEntryOffsetY, display );

            text.set( std::to_string( data.rating ), font );
            text.draw( offsetX + ratingOffset, offsetY + initialHighScoreEntryOffsetY, display );

            const Monster monster = getMonster( data.rating );
            const uint32_t baseMonsterAnimationIndex = monster.GetSpriteIndex() * 9;
            const fheroes2::Sprite & baseMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, baseMonsterAnimationIndex );
            fheroes2::Blit( baseMonsterSprite, display, baseMonsterSprite.x() + offsetX + 554, baseMonsterSprite.y() + offsetY + 91 );

            // Animation frame of a creature is based on its position on screen and common animation frame ID.
            const uint32_t monsterAnimationId
                = monsterAnimationSequence[( offsetX + offsetY + data.dayCount + monsterAnimationFrameId ) % monsterAnimationSequence.size()];
            const uint32_t secondaryMonsterAnimationIndex = baseMonsterAnimationIndex + 1 + monsterAnimationId;
            const fheroes2::Sprite & secondaryMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, secondaryMonsterAnimationIndex );
            fheroes2::Blit( secondaryMonsterSprite, display, secondaryMonsterSprite.x() + offsetX + 554, secondaryMonsterSprite.y() + offsetY + 91 );

            offsetY += highScoreEntryStepY;
            ++scoreIndex;
        }
    }

    void RedrawHighScoresStandard( int32_t ox, int32_t oy, uint32_t & monsterAnimationFrameId, const int32_t selectedScoreIndex )
    {
        redrawHighScoreScreen( ox, oy, monsterAnimationFrameId, selectedScoreIndex, highScoreDataContainer.getHighScoresStandard(), 6,
                               fheroes2::HighScoreDataContainer::getMonsterByRating );
    }

    void RedrawHighScoresCampaign( int32_t ox, int32_t oy, uint32_t & monsterAnimationFrameId, const int32_t selectedScoreIndex )
    {
        redrawHighScoreScreen( ox, oy, monsterAnimationFrameId, selectedScoreIndex, highScoreDataContainer.getHighScoresCampaign(), 7,
                               fheroes2::HighScoreDataContainer::getMonsterByDay );
    }

    uint32_t getCampaignRating( const Campaign::CampaignSaveData & campaignSaveData )
    {
        const uint32_t daysPassed = campaignSaveData.getDaysPassed();

        // Rating is calculated based on difficulty of campaign.
        const int32_t difficulty = campaignSaveData.getDifficulty();
        switch ( difficulty ) {
        case Campaign::CampaignDifficulty::Easy:
            return daysPassed * 125 / 100;
        case Campaign::CampaignDifficulty::Normal:
            // Nothing we need to do here.
            return daysPassed;
        case Campaign::CampaignDifficulty::Hard:
            return daysPassed * 75 / 100;
        default:
            // Did you add a new campaign difficulty? Add the logic above!
            assert( 0 );
            break;
        }

        return daysPassed;
    }
}

fheroes2::GameMode Game::DisplayHighScores( const bool isCampaign )
{
    GameOver::Result & gameResult = GameOver::Result::Get();

#ifdef WITH_DEBUG
    if ( IS_DEVEL() && ( gameResult.GetResult() & GameOver::WINS ) ) {
        std::string msg = "Developer mode is active, the result will not be saved! \n \n Your result: ";
        if ( isCampaign ) {
            msg += std::to_string( Campaign::CampaignSaveData::Get().getDaysPassed() );
        }
        else {
            msg += std::to_string( GetGameOverScores() );
        }

        fheroes2::showMessage( fheroes2::Text( "High Scores", fheroes2::FontType::normalYellow() ), fheroes2::Text( msg, fheroes2::FontType::normalWhite() ),
                               Dialog::OK );

        gameResult.ResetResult();

        return fheroes2::GameMode::MAIN_MENU;
    }
#endif

    const std::string highScoreDataPath = System::concatPath( GetSaveDir(), highScoreFileName );

    if ( !highScoreDataContainer.load( highScoreDataPath ) ) {
        // Unable to load the file. Let's populate with the default values.
        highScoreDataContainer.clear();
        highScoreDataContainer.populateStandardDefaultHighScores();
        highScoreDataContainer.populateCampaignDefaultHighScores();
    }

    const bool isAfterGameCompletion = ( ( gameResult.GetResult() & GameOver::WINS ) != 0 );
    if ( isAfterGameCompletion ) {
        // Check whether the game result is good enough to be put on high score board. If not then just skip showing the player name dialog.
        if ( isCampaign ) {
            const Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
            const uint32_t rating = getCampaignRating( campaignSaveData );
            const auto & campaignHighscoreData = highScoreDataContainer.getHighScoresCampaign();
            assert( !campaignHighscoreData.empty() );

            if ( campaignHighscoreData.back().rating < rating ) {
                gameResult.ResetResult();
                return fheroes2::GameMode::MAIN_MENU;
            }
        }
        else {
            const uint32_t rating = GetGameOverScores();
            const auto & standardHighscoreData = highScoreDataContainer.getHighScoresStandard();
            assert( !standardHighscoreData.empty() );

            if ( standardHighscoreData.back().rating > rating ) {
                gameResult.ResetResult();
                return fheroes2::GameMode::MAIN_MENU;
            }
        }
    }

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HSBKG, 0 );

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point top{ ( display.width() - back.width() ) / 2, ( display.height() - back.height() ) / 2 };
    const fheroes2::StandardWindow border( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT, false );

    int32_t selectedEntryIndex = -1;
    uint32_t monsterAnimationFrameId = 0;
    if ( isCampaign )
        RedrawHighScoresCampaign( top.x, top.y, monsterAnimationFrameId, selectedEntryIndex );
    else
        RedrawHighScoresStandard( top.x, top.y, monsterAnimationFrameId, selectedEntryIndex );

    // button that goes to standard: releasedIndex == 2, pressedIndex == 3
    // button that goes to campaign: releasedIndex == 0, pressedIndex == 1
    fheroes2::Button buttonOtherHighScore( top.x + 8, top.y + 315, ICN::HISCORE, isCampaign ? 2 : 0, isCampaign ? 3 : 1 );
    fheroes2::Button buttonExit( top.x + back.width() - 36, top.y + 315, ICN::HISCORE, 4, 5 );

    buttonOtherHighScore.draw();
    buttonExit.draw();

    display.render();

    if ( isAfterGameCompletion ) {
        std::string player( _( "Unknown Hero" ) );
        Dialog::InputString( _( "Your Name" ), player, std::string(), 15 );
        if ( player.empty() )
            player = _( "Unknown Hero" );

        const uint32_t completionTime = fheroes2::HighscoreData::generateCompletionTime();

        std::string lang = fheroes2::getLanguageAbbreviation( fheroes2::getCurrentLanguage() );

        if ( isCampaign ) {
            const Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
            const uint32_t rating = getCampaignRating( campaignSaveData );

            selectedEntryIndex = highScoreDataContainer.registerScoreCampaign( { std::move( lang ), player, Campaign::getCampaignName( campaignSaveData.getCampaignID() ),
                                                                                 completionTime, campaignSaveData.getDaysPassed(), rating, world.GetMapSeed() } );
        }
        else {
            const uint32_t rating = GetGameOverScores();
            const uint32_t days = world.CountDay();
            selectedEntryIndex = highScoreDataContainer.registerScoreStandard(
                { std::move( lang ), player, Settings::Get().CurrentFileInfo().name, completionTime, days, rating, world.GetMapSeed() } );
        }

        highScoreDataContainer.save( highScoreDataPath );

        if ( isCampaign )
            RedrawHighScoresCampaign( top.x, top.y, monsterAnimationFrameId, selectedEntryIndex );
        else
            RedrawHighScoresStandard( top.x, top.y, monsterAnimationFrameId, selectedEntryIndex );

        buttonOtherHighScore.draw();
        buttonExit.draw();
        display.render();

        gameResult.ResetResult();
    }

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::MAPS_DELAY } ) ) ) {
        le.MousePressLeft( buttonOtherHighScore.area() ) ? buttonOtherHighScore.drawOnPress() : buttonOtherHighScore.drawOnRelease();
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow() )
            return fheroes2::GameMode::MAIN_MENU;
        if ( le.MouseClickLeft( buttonOtherHighScore.area() ) )
            return isCampaign ? fheroes2::GameMode::HIGHSCORES_STANDARD : fheroes2::GameMode::HIGHSCORES_CAMPAIGN;

        if ( le.MousePressRight( buttonExit.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Exit" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Exit this menu." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( buttonOtherHighScore.area() ) ) {
            if ( isCampaign ) {
                fheroes2::showMessage( fheroes2::Text( _( "Standard" ), fheroes2::FontType::normalYellow() ),
                                       fheroes2::Text( _( "View High Scores for Standard Maps." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
            }
            else {
                fheroes2::showMessage( fheroes2::Text( _( "Campaign" ), fheroes2::FontType::normalYellow() ),
                                       fheroes2::Text( _( "View High Scores for Campaigns." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
            }
        }

        if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
            // TODO: avoid redrawing the whole screen when only monster animation update is required.
            if ( isCampaign )
                RedrawHighScoresCampaign( top.x, top.y, monsterAnimationFrameId, selectedEntryIndex );
            else
                RedrawHighScoresStandard( top.x, top.y, monsterAnimationFrameId, selectedEntryIndex );

            buttonOtherHighScore.draw();
            buttonExit.draw();
            display.render();
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}
