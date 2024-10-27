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

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h" // IWYU pragma: associated
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
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

#ifdef WITH_DEBUG
#include "logging.h"
#endif

namespace fheroes2
{
    enum class SupportedLanguage : uint8_t;
}

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
    const int32_t monsterOffsetX = 554;
    const int32_t monsterOffsetY = 91;

    // Calculate the mini monster sprites ROI.
    fheroes2::Rect getAnimationRoi( const fheroes2::Point & position, const bool isCampaign )
    {
        fheroes2::Rect roi;
        std::vector<uint32_t> animationIndex;

        if ( isCampaign ) {
            const std::vector<fheroes2::HighscoreData> & highScoreData = highScoreDataContainer.getHighScoresCampaign();

            animationIndex.reserve( highScoreData.size() );

            for ( const fheroes2::HighscoreData & data : highScoreData ) {
                animationIndex.push_back( fheroes2::HighScoreDataContainer::getMonsterByDay( data.rating ).GetSpriteIndex() * 9 );
            }
        }
        else {
            const std::vector<fheroes2::HighscoreData> & highScoreData = highScoreDataContainer.getHighScoresStandard();

            animationIndex.reserve( highScoreData.size() );

            for ( const fheroes2::HighscoreData & data : highScoreData ) {
                animationIndex.push_back( fheroes2::HighScoreDataContainer::getMonsterByRating( data.rating ).GetSpriteIndex() * 9 );
            }
        }

        // Animation frames: 0 - static part, 1-6 - animation, 7 and 8 - attacking sprite. We analyze and render here only 0-6 frames.
        for ( uint32_t i = 0; i < 7; ++i ) {
            const fheroes2::Sprite & topMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, animationIndex.front() + i );
            const fheroes2::Sprite & bottomMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, animationIndex.back() + i );

            // We search for the most top sprite offset of the top monster sprite, the most bottom offset of the bottom monster.
            int32_t offset = topMonsterSprite.y();
            if ( offset < roi.y ) {
                roi.y = offset;
            }

            // The offset of the sprite right border from the render point.
            offset = bottomMonsterSprite.y() + bottomMonsterSprite.height();
            if ( offset > roi.height ) {
                roi.height = offset;
            }

            for ( const uint32_t index : animationIndex ) {
                const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, index + i );

                offset = monsterSprite.x();
                if ( offset < roi.x ) {
                    roi.x = offset;
                }

                offset += monsterSprite.width();
                if ( offset > roi.width ) {
                    roi.width = offset;
                }
            }
        }

        // We have 10 high score records and should take into account 9 intervals between each record.
        roi.height += highScoreEntryStepY * 9 - roi.y;
        roi.width -= roi.x;

        // We take into account the High Scores windows position and first monster sprite offset.
        roi.x += position.x + monsterOffsetX;
        roi.y += position.y + monsterOffsetY;

        return roi;
    }

    void redrawMonstersAnimation( const fheroes2::Point & position, uint32_t & monsterAnimationFrameId, const std::vector<fheroes2::HighscoreData> & highScores,
                                  const std::function<Monster( size_t )> & getMonster )
    {
        ++monsterAnimationFrameId;

        fheroes2::Display & display = fheroes2::Display::instance();

        const std::array<uint8_t, 15> & monsterAnimationSequence = fheroes2::getMonsterAnimationSequence();
        int32_t offsetY = position.y;

        for ( const fheroes2::HighscoreData & data : highScores ) {
            // Animation frame of a creature is based on its position on screen and common animation frame ID.
            const uint32_t monsterAnimationId
                = monsterAnimationSequence[( position.x + offsetY + data.dayCount + monsterAnimationFrameId ) % monsterAnimationSequence.size()];
            const uint32_t secondaryMonsterAnimationIndex = getMonster( data.rating ).GetSpriteIndex() * 9 + 1 + monsterAnimationId;
            const fheroes2::Sprite & secondaryMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, secondaryMonsterAnimationIndex );
            fheroes2::Blit( secondaryMonsterSprite, display, secondaryMonsterSprite.x() + position.x + monsterOffsetX,
                            secondaryMonsterSprite.y() + offsetY + monsterOffsetY );

            offsetY += highScoreEntryStepY;
        }
    }

    void redrawHighScoreAnimation( const fheroes2::Point & position, uint32_t & monsterAnimationFrameId, const bool isCampaign )
    {
        if ( isCampaign ) {
            redrawMonstersAnimation( position, monsterAnimationFrameId, highScoreDataContainer.getHighScoresCampaign(),
                                     fheroes2::HighScoreDataContainer::getMonsterByDay );
        }
        else {
            redrawMonstersAnimation( position, monsterAnimationFrameId, highScoreDataContainer.getHighScoresStandard(),
                                     fheroes2::HighScoreDataContainer::getMonsterByRating );
        }
    }

    void redrawHighScoreScreen( const fheroes2::Point & position, const int32_t selectedScoreIndex, const std::vector<fheroes2::HighscoreData> & highScores,
                                const uint32_t titleImageIndex, const std::function<Monster( size_t )> & getMonster )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        // Draw background.
        const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::HSBKG, 0 );
        fheroes2::Copy( background, 0, 0, display, position.x, position.y, background.width(), background.height() );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HISCORE, titleImageIndex ), display, position.x + 50, position.y + 31 );

        fheroes2::Text text( "", fheroes2::FontType::normalWhite() );

        int32_t scoreIndex = 0;
        int32_t offsetY = position.y;

        for ( const fheroes2::HighscoreData & data : highScores ) {
            const fheroes2::FontType font = ( scoreIndex == selectedScoreIndex ) ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite();

            // TODO: scenario name can have its own independent language.
            const fheroes2::SupportedLanguage language = fheroes2::getLanguageFromAbbreviation( data.languageAbbreviation );

            text.set( data.playerName, font, language );
            text.fitToOneRow( scenarioNameOffset - playerNameOffset );
            text.draw( position.x + playerNameOffset, offsetY + initialHighScoreEntryOffsetY, display );

            text.set( data.scenarioName, font, language );
            text.fitToOneRow( dayCountOffset - scenarioNameOffset );
            text.draw( position.x + scenarioNameOffset, offsetY + initialHighScoreEntryOffsetY, display );

            text.set( std::to_string( data.dayCount ), font );
            text.draw( position.x + dayCountOffset, offsetY + initialHighScoreEntryOffsetY, display );

            text.set( std::to_string( data.rating ), font );
            text.draw( position.x + ratingOffset, offsetY + initialHighScoreEntryOffsetY, display );

            // Render static part of monster animation.
            const Monster monster = getMonster( data.rating );
            const uint32_t baseMonsterAnimationIndex = monster.GetSpriteIndex() * 9;
            const fheroes2::Sprite & baseMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, baseMonsterAnimationIndex );
            fheroes2::Blit( baseMonsterSprite, display, baseMonsterSprite.x() + position.x + 554, baseMonsterSprite.y() + offsetY + 91 );

            offsetY += highScoreEntryStepY;
            ++scoreIndex;
        }
    }
}

fheroes2::GameMode Game::DisplayHighScores( const bool isCampaign )
{
    GameOver::Result & gameResult = GameOver::Result::Get();

#ifdef WITH_DEBUG
    if ( IS_DEVEL() && ( gameResult.GetResult() & GameOver::WINS ) ) {
        std::string msg = "Developer mode is active, the result will not be saved! \n\n Your result: ";
        if ( isCampaign ) {
            msg += std::to_string( Campaign::CampaignSaveData::Get().getDaysPassed() );
        }
        else {
            msg += std::to_string( GetRating() * getGameOverScoreFactor() / 100 );
        }

        fheroes2::showStandardTextMessage( _( "High Scores" ), std::move( msg ), Dialog::OK );

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

    int32_t selectedEntryIndex = -1;
    const bool isAfterGameCompletion = ( ( gameResult.GetResult() & GameOver::WINS ) != 0 );

    fheroes2::Display & display = fheroes2::Display::instance();
    const bool isDefaultScreenSize = display.isDefaultSize();

    if ( isAfterGameCompletion ) {
        const auto inputPlayerName = []( std::string & playerName ) {
            Dialog::inputString( fheroes2::Text{}, fheroes2::Text{ _( "Your Name" ), fheroes2::FontType::normalWhite() }, playerName, 15, false, {} );
            if ( playerName.empty() ) {
                playerName = _( "Unknown Hero" );
            }
        };

        const uint32_t completionTime = fheroes2::HighscoreData::generateCompletionTime();
        std::string lang = fheroes2::getLanguageAbbreviation( fheroes2::getCurrentLanguage() );

        // Check whether the game result is good enough to be put on high score board. If not then just skip showing the player name dialog.
        if ( isCampaign ) {
            const Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
            const uint32_t daysPassed = campaignSaveData.getDaysPassed();
            // Rating is calculated based on difficulty of campaign.
            const uint32_t rating = daysPassed * campaignSaveData.getCampaignDifficultyPercent() / 100;

            const auto & campaignHighscoreData = highScoreDataContainer.getHighScoresCampaign();
            assert( !campaignHighscoreData.empty() );

            if ( campaignHighscoreData.back().rating < rating ) {
                gameResult.ResetResult();
                return fheroes2::GameMode::MAIN_MENU;
            }

            std::string playerName;
            inputPlayerName( playerName );

            selectedEntryIndex
                = highScoreDataContainer.registerScoreCampaign( { std::move( lang ), playerName, Campaign::getCampaignName( campaignSaveData.getCampaignID() ),
                                                                  completionTime, daysPassed, rating, world.GetMapSeed() } );
        }
        else {
            const uint32_t rating = GetRating() * getGameOverScoreFactor() / 100;

            const auto & standardHighscoreData = highScoreDataContainer.getHighScoresStandard();
            assert( !standardHighscoreData.empty() );

            if ( standardHighscoreData.back().rating > rating ) {
                gameResult.ResetResult();
                return fheroes2::GameMode::MAIN_MENU;
            }

            const uint32_t daysPassed = world.CountDay();
            std::string playerName;
            inputPlayerName( playerName );

            selectedEntryIndex = highScoreDataContainer.registerScoreStandard(
                { std::move( lang ), playerName, Settings::Get().getCurrentMapInfo().name, completionTime, daysPassed, rating, world.GetMapSeed() } );
        }

        highScoreDataContainer.save( highScoreDataPath );

        gameResult.ResetResult();

        // Fade-out game screen.
        fheroes2::fadeOutDisplay();
    }
    else if ( isDefaultScreenSize ) {
        fheroes2::fadeOutDisplay();
    }

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HSBKG, 0 );

    const fheroes2::Point top{ ( display.width() - back.width() ) / 2, ( display.height() - back.height() ) / 2 };
    const fheroes2::StandardWindow border( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT, false );

    uint32_t monsterAnimationFrameId = 0;
    if ( isCampaign ) {
        redrawHighScoreScreen( top, selectedEntryIndex, highScoreDataContainer.getHighScoresCampaign(), 7, fheroes2::HighScoreDataContainer::getMonsterByDay );
    }
    else {
        redrawHighScoreScreen( top, selectedEntryIndex, highScoreDataContainer.getHighScoresStandard(), 6, fheroes2::HighScoreDataContainer::getMonsterByRating );
    }

    const fheroes2::Rect animationRoi = getAnimationRoi( top, isCampaign );

    // Make a copy of static monsters animation part with background to restore it during animations.
    fheroes2::ImageRestorer backgroundCopy( display, animationRoi.x, animationRoi.y, animationRoi.width, animationRoi.height );

    // Render first animation frame.
    redrawHighScoreAnimation( top, monsterAnimationFrameId, isCampaign );

    fheroes2::Button buttonOtherHighScore( top.x + 8, top.y + 315, isCampaign ? ICN::BUTTON_HSCORES_VERTICAL_CAMPAIGN : ICN::BUTTON_HSCORES_VERTICAL_STANDARD, 0, 1 );
    fheroes2::Button buttonExit( top.x + back.width() - 36, top.y + 315, ICN::BUTTON_HSCORES_VERTICAL_EXIT, 0, 1 );

    buttonOtherHighScore.draw();
    buttonExit.draw();

    // Fade-in High Scores screen.
    if ( isAfterGameCompletion ) {
        fheroes2::fadeInDisplay();
    }
    else {
        if ( !isDefaultScreenSize ) {
            // We need to expand the ROI for the next render to properly render window borders and shadow.
            display.updateNextRenderRoi( border.totalArea() );
        }

        fheroes2::fadeInDisplay( border.activeArea(), !isDefaultScreenSize );
    }

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::MAPS_DELAY } ) ) ) {
        le.isMouseLeftButtonPressedInArea( buttonOtherHighScore.area() ) ? buttonOtherHighScore.drawOnPress() : buttonOtherHighScore.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow() ) {
            if ( isAfterGameCompletion || isDefaultScreenSize ) {
                fheroes2::fadeOutDisplay();
                Game::setDisplayFadeIn();
            }
            else {
                fheroes2::fadeOutDisplay( border.activeArea(), true );
            }

            return fheroes2::GameMode::MAIN_MENU;
        }
        if ( le.MouseClickLeft( buttonOtherHighScore.area() ) ) {
            return isCampaign ? fheroes2::GameMode::HIGHSCORES_STANDARD : fheroes2::GameMode::HIGHSCORES_CAMPAIGN;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonExit.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Exit" ), _( "Exit this menu." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonOtherHighScore.area() ) ) {
            if ( isCampaign ) {
                fheroes2::showStandardTextMessage( _( "Standard" ), _( "View High Scores for Standard Maps." ), Dialog::ZERO );
            }
            else {
                fheroes2::showStandardTextMessage( _( "Campaign" ), _( "View High Scores for Campaigns." ), Dialog::ZERO );
            }
        }

        if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
            // Restore background with static monster animation part.
            backgroundCopy.restore();

            redrawHighScoreAnimation( top, monsterAnimationFrameId, isCampaign );

            buttonOtherHighScore.draw();
            buttonExit.draw();
            display.render( animationRoi );
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}
