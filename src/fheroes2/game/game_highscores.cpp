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
#include <array>
#include <ctime>
#include <string>
#include <vector>

#include "agg.h"
#include "agg_image.h"
#include "audio.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_highscores.h"
#include "game_over.h"
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
    const uint16_t HGS_ID1 = 0xF1F3;
    const uint16_t HGS_ID2 = 0xF1F4;
    const uint8_t HGS_MAX = 10;
    const std::string HGS_DATA_PATH = "fheroes2.hgs";

    const std::array<Monster::monster_t, 65> monstersInRanking = { Monster::PEASANT,       Monster::GOBLIN,
                                                                   Monster::SPRITE,        Monster::HALFLING,
                                                                   Monster::CENTAUR,       Monster::ROGUE,
                                                                   Monster::SKELETON,      Monster::ORC,
                                                                   Monster::ZOMBIE,        Monster::ARCHER,
                                                                   Monster::RANGER,        Monster::BOAR,
                                                                   Monster::DWARF,         Monster::MUTANT_ZOMBIE,
                                                                   Monster::ORC_CHIEF,     Monster::ELF,
                                                                   Monster::GARGOYLE,      Monster::PIKEMAN,
                                                                   Monster::GRAND_ELF,     Monster::BATTLE_DWARF,
                                                                   Monster::NOMAD,         Monster::VETERAN_PIKEMAN,
                                                                   Monster::WOLF,          Monster::MUMMY,
                                                                   Monster::IRON_GOLEM,    Monster::ROYAL_MUMMY,
                                                                   Monster::OGRE,          Monster::GRIFFIN,
                                                                   Monster::SWORDSMAN,     Monster::DRUID,
                                                                   Monster::STEEL_GOLEM,   Monster::MASTER_SWORDSMAN,
                                                                   Monster::AIR_ELEMENT,   Monster::GREATER_DRUID,
                                                                   Monster::FIRE_ELEMENT,  Monster::GHOST,
                                                                   Monster::VAMPIRE,       Monster::WATER_ELEMENT,
                                                                   Monster::EARTH_ELEMENT, Monster::ROC,
                                                                   Monster::MINOTAUR,      Monster::CAVALRY,
                                                                   Monster::TROLL,         Monster::MAGE,
                                                                   Monster::MEDUSA,        Monster::LICH,
                                                                   Monster::OGRE_LORD,     Monster::MINOTAUR_KING,
                                                                   Monster::CHAMPION,      Monster::WAR_TROLL,
                                                                   Monster::VAMPIRE_LORD,  Monster::ARCHMAGE,
                                                                   Monster::POWER_LICH,    Monster::UNICORN,
                                                                   Monster::HYDRA,         Monster::PALADIN,
                                                                   Monster::GENIE,         Monster::CRUSADER,
                                                                   Monster::CYCLOPS,       Monster::GIANT,
                                                                   Monster::PHOENIX,       Monster::BONE_DRAGON,
                                                                   Monster::GREEN_DRAGON,  Monster::RED_DRAGON,
                                                                   Monster::TITAN };
}

namespace HighScore
{
    HighScoreDataContainer & HighScoreDataContainer::Get() 
    {
        static HighScoreDataContainer instance;
        return instance;
    }

    StreamBase & operator<<( StreamBase & msg, const HighScoreStandardData & data )
    {
        return msg << data._player << data._scenarioName << data._localTime << data._days << data._rating;
    }

    StreamBase & operator>>( StreamBase & msg, HighScoreStandardData & data )
    {
        return msg >> data._player >> data._scenarioName >> data._localTime >> data._days >> data._rating;
    }

    bool HighScoreStandardData::operator==( const HighScoreStandardData & other ) const
    {
        return _player == other._player && _scenarioName == other._scenarioName && _days == other._days;
    }

    bool RatingSort( const HighScoreStandardData & h1, const HighScoreStandardData & h2 )
    {
        return h1._rating > h2._rating;
    }

    StreamBase & operator<<( StreamBase & msg, const HighScoreCampaignData & data ) 
    {
        return msg << data._player << data._campaignName << data._localTime << data._days;
    }

    StreamBase & operator>>( StreamBase & msg, HighScoreCampaignData & data ) 
    {
        return msg >> data._player >> data._campaignName >> data._localTime >> data._days;
    }

    bool HighScoreCampaignData::operator==( const HighScoreCampaignData & other ) const 
    {
        return _player == other._player && _campaignName == other._campaignName && _days == other._days;
    }

    bool DaysSort( const HighScoreCampaignData & h1, const HighScoreCampaignData & h2 ) 
    {
        return h1._days < h2._days;
    }

    HighScoreDataContainer::HighScoreDataContainer()
        : _monsterAnimationFrameId( 0 )
    {
        uint32_t ratingSoFar = 0;
        uint32_t ratingIncrementCount = 0;

        // need int for reverse-for loop
        const int monstersInRankingCount = static_cast<int>( monstersInRanking.size() );

        for ( int i = 0; i < monstersInRankingCount; ++i ) {
            const Monster::monster_t monster = monstersInRanking[i];

            // 0 to 3
            if ( monster == Monster::PEASANT ) {
                ratingIncrementCount = 3;
            }
            // 4 to 131
            else if ( monster == Monster::GOBLIN ) {
                ratingIncrementCount = 4;
            }
            // 132 to 227
            else if ( monster == Monster::GREATER_DRUID ) {
                ratingIncrementCount = 3;
            }
            // >= 228
            else if ( monster == Monster::BLACK_DRAGON ) {
                ratingIncrementCount = 1;
            }

            ratingSoFar += ratingIncrementCount;
            _monsterRatings.emplace_back( std::make_pair( ratingSoFar, monstersInRanking[i] ) );
        }

        uint32_t daySoFar = 0;
        uint32_t dayIncrementCount = 0;

        for ( int i = monstersInRankingCount - 1; i >= 0; --i ) {
            const Monster::monster_t monster = monstersInRanking[i];

            // 0 to 300
            if ( monster == Monster::BLACK_DRAGON ) {
                dayIncrementCount = 300;
            }
            // 301 to 1000
            else if ( monster == Monster::TITAN ) {
                dayIncrementCount = 20;
            }
            // 1001 to 2000
            else if ( monster == Monster::DRUID ) {
                dayIncrementCount = 100;
            }
            // 2001 to 5800
            else if ( monster == Monster::BATTLE_DWARF ) {
                dayIncrementCount = 200;
            }
            // >= 5801
            else if ( monster == Monster::PEASANT ) {
                dayIncrementCount = 1;
            }

            daySoFar += dayIncrementCount;
            _monsterDays.emplace_back( std::make_pair( daySoFar, monstersInRanking[i] ) );
        }
    }

    bool HighScoreDataContainer::Load( const std::string & fileName )
    {
        ZStreamFile hdata;
        if ( !hdata.read( fileName ) )
            return false;

        hdata.setbigendian( true );
        u16 hgs_id = 0;

        hdata >> hgs_id;

        if ( hgs_id == HGS_ID1 ) {
            hdata >> _highScoresStandard;
            return !hdata.fail();
        }
        if ( hgs_id == HGS_ID2 ) {
            hdata >> _highScoresStandard >> _highScoresCampaign;
            return !hdata.fail();
        }

        return false;
    }

    bool HighScoreDataContainer::Save( const std::string & fileName ) const
    {
        ZStreamFile hdata;
        hdata.setbigendian( true );
        hdata << static_cast<uint16_t>( HGS_ID2 ) << _highScoresStandard << _highScoresCampaign;
        if ( hdata.fail() || !hdata.write( fileName ) )
            return false;

        return true;
    }

    void HighScoreDataContainer::RegisterScoreStandard( const std::string & playerName, const std::string & scenarioName, const uint32_t days, const uint32_t rating )
    {
        HighScoreStandardData highScore;
        std::vector<HighScoreStandardData> & highScores = _highScoresStandard;

        highScore._player = playerName;
        highScore._scenarioName = scenarioName;
        highScore._localTime = std::time( nullptr );
        highScore._days = days;
        highScore._rating = rating;

        // duplicate score
        if ( highScores.end() != std::find( highScores.begin(), highScores.end(), highScore ) )
            return;

        highScores.emplace_back( highScore );
        std::sort( highScores.begin(), highScores.end(), RatingSort );
        if ( highScores.size() > HGS_MAX )
            highScores.resize( HGS_MAX );
    }

    void HighScoreDataContainer::RegisterScoreCampaign( const std::string & playerName, const std::string & campaignName, const uint32_t days )
    {
        HighScoreCampaignData highScore;
        std::vector<HighScoreCampaignData> & highScores = _highScoresCampaign;

        highScore._player = playerName;
        highScore._campaignName = campaignName;
        highScore._localTime = std::time( nullptr );
        highScore._days = days;

        // duplicate score
        if ( highScores.end() != std::find( highScores.begin(), highScores.end(), highScore ) )
            return;

        highScores.emplace_back( highScore );
        std::sort( highScores.begin(), highScores.end(), RatingSort );
        if ( highScores.size() > HGS_MAX )
            highScores.resize( HGS_MAX );
    }

    void HighScoreDataContainer::RedrawListStandard( int32_t ox, int32_t oy )
    {
        ++_monsterAnimationFrameId;

        fheroes2::Display & display = fheroes2::Display::instance();

        // image background
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HSBKG, 0 ), display, ox, oy );

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HISCORE, 6 ), display, ox + 50, oy + 31 );

        std::sort( _highScoresStandard.begin(), _highScoresStandard.end(), RatingSort );

        std::vector<HighScoreStandardData>::const_iterator it1 = _highScoresStandard.begin();
        std::vector<HighScoreStandardData>::const_iterator it2 = _highScoresStandard.end();

        Text text;
        text.Set( Font::BIG );

        const std::array<uint8_t, 15> & monsterAnimationSequence = fheroes2::getMonsterAnimationSequence();

        for ( ; it1 != it2 && ( it1 - _highScoresStandard.begin() < HGS_MAX ); ++it1 ) {
            const HighScoreStandardData & hgs = *it1;

            text.Set( hgs._player );
            text.Blit( ox + 88, oy + 70 );

            text.Set( hgs._scenarioName );
            text.Blit( ox + 244, oy + 70 );

            text.Set( std::to_string( hgs._days ) );
            text.Blit( ox + 403, oy + 70 );

            text.Set( std::to_string( hgs._rating ) );
            text.Blit( ox + 484, oy + 70 );

            const Monster monster = HighScoreDataContainer::getMonsterByRating( hgs._rating );
            const uint32_t baseMonsterAnimationIndex = monster.GetSpriteIndex() * 9;
            const fheroes2::Sprite & baseMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, baseMonsterAnimationIndex );
            fheroes2::Blit( baseMonsterSprite, display, baseMonsterSprite.x() + ox + 554, baseMonsterSprite.y() + oy + 91 );

            // Animation frame of a creature is based on its position on screen and common animation frame ID.
            const uint32_t monsterAnimationId = monsterAnimationSequence[( ox + oy + hgs._days + _monsterAnimationFrameId ) % monsterAnimationSequence.size()];
            const uint32_t secondaryMonsterAnimationIndex = baseMonsterAnimationIndex + 1 + monsterAnimationId;
            const fheroes2::Sprite & secondaryMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, secondaryMonsterAnimationIndex );
            fheroes2::Blit( secondaryMonsterSprite, display, secondaryMonsterSprite.x() + ox + 554, secondaryMonsterSprite.y() + oy + 91 );

            oy += 40;
        }
    }

    void HighScoreDataContainer::RedrawListCampaign( int32_t ox, int32_t oy )
    {
        ++_monsterAnimationFrameId;

        fheroes2::Display & display = fheroes2::Display::instance();

        // image background
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HSBKG, 4 ), display, ox, oy );

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HISCORE, 7 ), display, ox + 50, oy + 31 );

        std::sort( _highScoresCampaign.begin(), _highScoresCampaign.end(), DaysSort );

        std::vector<HighScoreCampaignData>::const_iterator it1 = _highScoresCampaign.begin();
        std::vector<HighScoreCampaignData>::const_iterator it2 = _highScoresCampaign.end();

        Text text;
        text.Set( Font::BIG );

        const std::array<uint8_t, 15> & monsterAnimationSequence = fheroes2::getMonsterAnimationSequence();

        for ( ; it1 != it2 && ( it1 - _highScoresCampaign.begin() < HGS_MAX ); ++it1 ) {
            const HighScoreCampaignData & hgs = *it1;

            text.Set( hgs._player );
            text.Blit( ox + 88, oy + 70 );

            text.Set( hgs._campaignName );
            text.Blit( ox + 244, oy + 70 );

            text.Set( std::to_string( hgs._days ) );
            text.Blit( ox + 403, oy + 70 );

            const Monster monster = HighScoreDataContainer::getMonsterByDay( hgs._days );
            const uint32_t baseMonsterAnimationIndex = monster.GetSpriteIndex() * 9;
            const fheroes2::Sprite & baseMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, baseMonsterAnimationIndex );
            fheroes2::Blit( baseMonsterSprite, display, baseMonsterSprite.x() + ox + 554, baseMonsterSprite.y() + oy + 91 );

            // Animation frame of a creature is based on its position on screen and common animation frame ID.
            const uint32_t monsterAnimationId = monsterAnimationSequence[( ox + oy + hgs._days + _monsterAnimationFrameId ) % monsterAnimationSequence.size()];
            const uint32_t secondaryMonsterAnimationIndex = baseMonsterAnimationIndex + 1 + monsterAnimationId;
            const fheroes2::Sprite & secondaryMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, secondaryMonsterAnimationIndex );
            fheroes2::Blit( secondaryMonsterSprite, display, secondaryMonsterSprite.x() + ox + 554, secondaryMonsterSprite.y() + oy + 91 );

            oy += 40;
        }
    }

    Monster HighScoreDataContainer::getMonsterByRating( const size_t rating ) const
    {
        std::pair<size_t, Monster::monster_t> lastData = _monsterRatings.back();

        if ( rating >= lastData.first )
            return Monster( lastData.second );

        Monster::monster_t monster = Monster::PEASANT;
        for ( size_t i = 0; i < _monsterRatings.size(); ++i ) {
            if ( rating <= _monsterRatings[i].first ) {
                monster = _monsterRatings[i].second;
                break;
            }
        }

        return Monster( monster );
    }

    Monster HighScoreDataContainer::getMonsterByDay( const size_t dayCount ) const
    {
        std::pair<size_t, Monster::monster_t> lastData = _monsterDays.back();

        if ( dayCount >= lastData.first )
            return Monster( lastData.second );

        Monster::monster_t monster = Monster::PEASANT;
        for ( size_t i = 0; i < _monsterDays.size(); ++i ) {
            if ( dayCount <= _monsterDays[i].first ) {
                monster = _monsterDays[i].second;
                break;
            }
        }

        return Monster( monster );
    }
}

fheroes2::GameMode Game::HighScoresStandard()
{
#ifdef WITH_DEBUG
    if ( IS_DEVEL() && world.CountDay() ) {
        std::string msg = std::string( "Developer mode, not save! \n \n Your result: " ) + std::to_string( GetGameOverScores() );
        Dialog::Message( "High Scores", msg, Font::BIG, Dialog::OK );
        return fheroes2::GameMode::MAIN_MENU;
    }
#endif

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    Mixer::Pause();
    AGG::PlayMusic( MUS::MAINMENU, true, true );

    const std::string highScoreDataPath = System::ConcatePath( GetSaveDir(), HGS_DATA_PATH );

    HighScore::HighScoreDataContainer & hgs = HighScore::HighScoreDataContainer::Get();
    hgs.RefreshMonsterAnimationFrameID();
    hgs.Load( highScoreDataPath );

    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HSBKG, 0 );

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point top( ( display.width() - back.width() ) / 2, ( display.height() - back.height() ) / 2 );
    const fheroes2::StandardWindow border( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT );

    hgs.RedrawListStandard( top.x, top.y );

    fheroes2::Button buttonCampaign( top.x + 8, top.y + 315, ICN::HISCORE, 0, 1 );
    fheroes2::Button buttonExit( top.x + back.width() - 36, top.y + 315, ICN::HISCORE, 4, 5 );

    buttonCampaign.draw();
    buttonExit.draw();

    display.render();

    GameOver::Result & gameResult = GameOver::Result::Get();

    if ( gameResult.GetResult() & GameOver::WINS ) {
        std::string player( _( "Unknown Hero" ) );
        Dialog::InputString( _( "Your Name" ), player, std::string(), 15 );
        if ( player.empty() )
            player = _( "Unknown Hero" );

        const uint32_t rating = GetGameOverScores();
        const uint32_t days = world.CountDay();
        hgs.RegisterScoreStandard( player, Settings::Get().CurrentFileInfo().name, days, rating );
        hgs.Save( highScoreDataPath );
        hgs.RedrawListStandard( top.x, top.y );
        buttonCampaign.draw();
        buttonExit.draw();
        display.render();
        gameResult.ResetResult();
    }

    LocalEvent & le = LocalEvent::Get();

    // highscores loop
    while ( le.HandleEvents() ) {
        // key code info
        if ( Settings::Get().Debug() == 0x12 && le.KeyPress() )
            Dialog::Message( "Key Press:", std::to_string( le.KeyValue() ), Font::SMALL, Dialog::OK );
        if ( buttonCampaign.isEnabled() ) {
            le.MousePressLeft( buttonCampaign.area() ) ? buttonCampaign.drawOnPress() : buttonCampaign.drawOnRelease();
        }
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            return fheroes2::GameMode::MAIN_MENU;
        if ( le.MouseClickLeft( buttonCampaign.area() ) )
            return fheroes2::GameMode::HIGHSCORES_CAMPAIGN;

        if ( le.MousePressRight( buttonExit.area() ) ) {
            Dialog::Message( _( "Exit" ), _( "Exit this menu." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonCampaign.area() ) ) {
            Dialog::Message( _( "Campaign" ), _( "View High Scores for Campaigns." ), Font::BIG );
        }

        if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
            hgs.RedrawListStandard( top.x, top.y );
            display.render();
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}

fheroes2::GameMode Game::HighScoresCampaign()
{
#ifdef WITH_DEBUG
    if ( IS_DEVEL() && world.CountDay() ) {
        std::string msg = std::string( "Developer mode, not save! \n \n Your result: " ) + std::to_string( GetGameOverScores() );
        Dialog::Message( "High Scores", msg, Font::BIG, Dialog::OK );
        return fheroes2::GameMode::MAIN_MENU;
    }
#endif

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    Mixer::Pause();
    AGG::PlayMusic( MUS::MAINMENU, true, true );

    const std::string highScoreDataPath = System::ConcatePath( GetSaveDir(), HGS_DATA_PATH );

    HighScore::HighScoreDataContainer & hgs = HighScore::HighScoreDataContainer::Get();
    hgs.RefreshMonsterAnimationFrameID();
    hgs.Load( highScoreDataPath );

    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HSBKG, 0 );

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point top( ( display.width() - back.width() ) / 2, ( display.height() - back.height() ) / 2 );
    const fheroes2::StandardWindow border( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT );

    hgs.RedrawListCampaign( top.x, top.y );

    fheroes2::Button buttonStandard( top.x + 8, top.y + 315, ICN::HISCORE, 2, 1 );
    fheroes2::Button buttonExit( top.x + back.width() - 36, top.y + 315, ICN::HISCORE, 4, 5 );

    buttonStandard.draw();
    buttonExit.draw();

    display.render();

    GameOver::Result & gameResult = GameOver::Result::Get();

    if ( gameResult.GetResult() & GameOver::WINS ) {
        std::string player( _( "Unknown Hero" ) );
        Dialog::InputString( _( "Your Name" ), player, std::string(), 15 );
        if ( player.empty() )
            player = _( "Unknown Hero" );

        const Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
        const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( campaignSaveData.getCampaignID() );

        hgs.RegisterScoreCampaign( player, campaignData.getCampaignName(), campaignSaveData.getDaysPassed() );
        hgs.Save( highScoreDataPath );
        hgs.RedrawListCampaign( top.x, top.y );
        buttonStandard.draw();
        buttonExit.draw();
        display.render();
        gameResult.ResetResult();
    }

    LocalEvent & le = LocalEvent::Get();

    // highscores loop
    while ( le.HandleEvents() ) {
        // key code info
        if ( Settings::Get().Debug() == 0x12 && le.KeyPress() )
            Dialog::Message( "Key Press:", std::to_string( le.KeyValue() ), Font::SMALL, Dialog::OK );
        if ( buttonStandard.isEnabled() ) {
            le.MousePressLeft( buttonStandard.area() ) ? buttonStandard.drawOnPress() : buttonStandard.drawOnRelease();
        }
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            return fheroes2::GameMode::MAIN_MENU;
        if ( le.MouseClickLeft( buttonStandard.area() ) )
            return fheroes2::GameMode::HIGHSCORES_STANDARD;

        if ( le.MousePressRight( buttonExit.area() ) ) {
            Dialog::Message( _( "Exit" ), _( "Exit this menu." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonStandard.area() ) ) {
            Dialog::Message( _( "Standard" ), _( "View High Scores for Standard Maps." ), Font::BIG );
        }

        if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
            hgs.RedrawListCampaign( top.x, top.y );
            display.render();
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}
