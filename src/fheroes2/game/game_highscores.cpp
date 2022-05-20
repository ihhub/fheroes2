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
#include <array>
#include <ctime>
#include <string>
#include <vector>

#include "agg.h"
#include "agg_image.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_over.h"
#include "highscores.h"
#include "icn.h"
#ifdef WITH_DEBUG
#include "logging.h"
#endif
#include "monster_anim.h"
#include "mus.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_window.h"
#include "world.h"
#include "zzlib.h"

namespace
{
    const std::string highScoreFileName = "fheroes2.hgs";
    HighScore::HighScoreDataContainer highScoreDataContainer;

    void RedrawHighScoresStandard( int32_t ox, int32_t oy, uint32_t & monsterAnimationFrameId )
    {
        ++monsterAnimationFrameId;

        fheroes2::Display & display = fheroes2::Display::instance();

        // image background
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HSBKG, 0 ), display, ox, oy );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HISCORE, 6 ), display, ox + 50, oy + 31 );

        Text text;
        text.Set( Font::BIG );

        const std::array<uint8_t, 15> & monsterAnimationSequence = fheroes2::getMonsterAnimationSequence();
        const std::vector<HighScore::HighScoreStandardData> & highScores = highScoreDataContainer.getHighScoresStandard();

        for ( size_t i = 0; i < highScores.size(); ++i ) {
            const HighScore::HighScoreStandardData & hgs = highScores[i];

            text.Set( hgs._player );
            text.Blit( ox + 88, oy + 70 );

            text.Set( hgs._scenarioName );
            text.Blit( ox + 244, oy + 70 );

            text.Set( std::to_string( hgs._days ) );
            text.Blit( ox + 403, oy + 70 );

            text.Set( std::to_string( hgs._rating ) );
            text.Blit( ox + 484, oy + 70 );

            const Monster monster = HighScore::HighScoreDataContainer::getMonsterByRating( hgs._rating );
            const uint32_t baseMonsterAnimationIndex = monster.GetSpriteIndex() * 9;
            const fheroes2::Sprite & baseMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, baseMonsterAnimationIndex );
            fheroes2::Blit( baseMonsterSprite, display, baseMonsterSprite.x() + ox + 554, baseMonsterSprite.y() + oy + 91 );

            // Animation frame of a creature is based on its position on screen and common animation frame ID.
            const uint32_t monsterAnimationId = monsterAnimationSequence[( ox + oy + hgs._days + monsterAnimationFrameId ) % monsterAnimationSequence.size()];
            const uint32_t secondaryMonsterAnimationIndex = baseMonsterAnimationIndex + 1 + monsterAnimationId;
            const fheroes2::Sprite & secondaryMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, secondaryMonsterAnimationIndex );
            fheroes2::Blit( secondaryMonsterSprite, display, secondaryMonsterSprite.x() + ox + 554, secondaryMonsterSprite.y() + oy + 91 );

            oy += 40;
        }
    }

    void RedrawHighScoresCampaign( int32_t ox, int32_t oy, uint32_t & monsterAnimationFrameId )
    {
        ++monsterAnimationFrameId;

        fheroes2::Display & display = fheroes2::Display::instance();

        // image background
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HSBKG, 0 ), display, ox, oy );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HISCORE, 7 ), display, ox + 50, oy + 31 );

        Text text;
        text.Set( Font::BIG );

        const std::array<uint8_t, 15> & monsterAnimationSequence = fheroes2::getMonsterAnimationSequence();
        const std::vector<HighScore::HighScoreCampaignData> & highScores = highScoreDataContainer.getHighScoresCampaign();

        for ( size_t i = 0; i < highScores.size(); ++i ) {
            const HighScore::HighScoreCampaignData & hgs = highScores[i];

            text.Set( hgs._player );
            text.Blit( ox + 88, oy + 70 );

            text.Set( hgs._campaignName );
            text.Blit( ox + 250, oy + 70 );

            text.Set( std::to_string( hgs._days ) );
            text.Blit( ox + 455, oy + 70 );

            const Monster monster = HighScore::HighScoreDataContainer::getMonsterByDay( hgs._days );
            const uint32_t baseMonsterAnimationIndex = monster.GetSpriteIndex() * 9;
            const fheroes2::Sprite & baseMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, baseMonsterAnimationIndex );
            fheroes2::Blit( baseMonsterSprite, display, baseMonsterSprite.x() + ox + 554, baseMonsterSprite.y() + oy + 91 );

            // Animation frame of a creature is based on its position on screen and common animation frame ID.
            uint32_t monsterAnimationId = monsterAnimationSequence[( ox + oy + hgs._days + monsterAnimationFrameId ) % monsterAnimationSequence.size()];
            const uint32_t secondaryMonsterAnimationIndex = baseMonsterAnimationIndex + 1 + monsterAnimationId;
            const fheroes2::Sprite & secondaryMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, secondaryMonsterAnimationIndex );
            fheroes2::Blit( secondaryMonsterSprite, display, secondaryMonsterSprite.x() + ox + 554, secondaryMonsterSprite.y() + oy + 91 );

            oy += 40;
        }
    }
}

fheroes2::GameMode Game::HighScoresStandard()
{
    return DisplayHighScores( false );
}

fheroes2::GameMode Game::HighScoresCampaign()
{
    return DisplayHighScores( true );
}

fheroes2::GameMode Game::DisplayHighScores( const bool isCampaign )
{
#ifdef WITH_DEBUG
    if ( IS_DEVEL() && world.CountDay() ) {
        std::string msg = std::string( "Developer mode, not save! \n \n Your result: " ) + std::to_string( Campaign::CampaignSaveData::Get().getDaysPassed() );
        Dialog::Message( "High Scores", msg, Font::BIG, Dialog::OK );
        return fheroes2::GameMode::MAIN_MENU;
    }
#endif

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const std::string highScoreDataPath = System::ConcatePath( GetSaveDir(), highScoreFileName );

    if ( !highScoreDataContainer.load( highScoreDataPath ) ) {
        // Unable to load the file. Let's populate with the default values.
        highScoreDataContainer.populateDefaultHighScoresStandard();
        highScoreDataContainer.save( highScoreDataPath );
    }

    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HSBKG, 0 );

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point top( ( display.width() - back.width() ) / 2, ( display.height() - back.height() ) / 2 );
    const fheroes2::StandardWindow border( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT );

    uint32_t monsterAnimationFrameId = 0;
    if ( isCampaign )
        RedrawHighScoresCampaign( top.x, top.y, monsterAnimationFrameId );
    else
        RedrawHighScoresStandard( top.x, top.y, monsterAnimationFrameId );

    // button that goes to standard: releasedIndex == 2, pressedIndex == 3
    // button that goes to campaign: releasedIndex == 0, pressedIndex == 1
    fheroes2::Button buttonOtherHighScore( top.x + 8, top.y + 315, ICN::HISCORE, isCampaign ? 2 : 0, isCampaign ? 3 : 1 );
    fheroes2::Button buttonExit( top.x + back.width() - 36, top.y + 315, ICN::HISCORE, 4, 5 );

    buttonOtherHighScore.draw();
    buttonExit.draw();

    display.render();

    GameOver::Result & gameResult = GameOver::Result::Get();

    if ( gameResult.GetResult() & GameOver::WINS ) {
        std::string player( _( "Unknown Hero" ) );
        Dialog::InputString( _( "Your Name" ), player, std::string(), 15 );
        if ( player.empty() )
            player = _( "Unknown Hero" );

        if ( isCampaign ) {
            const Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
            highScoreDataContainer.registerScoreCampaign( player, Campaign::getCampaignName( campaignSaveData.getCampaignID() ), campaignSaveData.getDaysPassed() );
        }
        else {
            const uint32_t rating = GetGameOverScores();
            const uint32_t days = world.CountDay();
            highScoreDataContainer.registerScoreStandard( player, Settings::Get().CurrentFileInfo().name, days, rating );
        }

        highScoreDataContainer.save( highScoreDataPath );

        if ( isCampaign )
            RedrawHighScoresCampaign( top.x, top.y, monsterAnimationFrameId );
        else
            RedrawHighScoresStandard( top.x, top.y, monsterAnimationFrameId );

        buttonOtherHighScore.draw();
        buttonExit.draw();
        display.render();
        gameResult.ResetResult();
    }

    LocalEvent & le = LocalEvent::Get();

    // highscores loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonOtherHighScore.area() ) ? buttonOtherHighScore.drawOnPress() : buttonOtherHighScore.drawOnRelease();
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow() )
            return fheroes2::GameMode::MAIN_MENU;
        if ( le.MouseClickLeft( buttonOtherHighScore.area() ) )
            return isCampaign ? fheroes2::GameMode::HIGHSCORES_STANDARD : fheroes2::GameMode::HIGHSCORES_CAMPAIGN;

        if ( le.MousePressRight( buttonExit.area() ) ) {
            Dialog::Message( _( "Exit" ), _( "Exit this menu." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonOtherHighScore.area() ) ) {
            Dialog::Message( _( "Standard" ), _( isCampaign ? "View High Scores for Standard Maps." : "View High Scores for Campaigns." ), Font::BIG );
        }

        if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
            if ( isCampaign )
                RedrawHighScoresCampaign( top.x, top.y, monsterAnimationFrameId );
            else
                RedrawHighScoresStandard( top.x, top.y, monsterAnimationFrameId );

            buttonOtherHighScore.draw();
            buttonExit.draw();
            display.render();
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}
