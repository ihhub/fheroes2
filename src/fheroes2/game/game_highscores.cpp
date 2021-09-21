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
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
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

#define HGS_ID 0xF1F3
#define HGS_MAX 10

struct hgs_t
{
    hgs_t()
        : localtime( 0 )
        , days( 0 )
        , rating( 0 )
    {}

    bool operator==( const hgs_t & ) const;

    std::string player;
    std::string land;
    u32 localtime;
    u32 days;
    u32 rating;
};

StreamBase & operator<<( StreamBase & msg, const hgs_t & hgs )
{
    return msg << hgs.player << hgs.land << hgs.localtime << hgs.days << hgs.rating;
}

StreamBase & operator>>( StreamBase & msg, hgs_t & hgs )
{
    return msg >> hgs.player >> hgs.land >> hgs.localtime >> hgs.days >> hgs.rating;
}

bool hgs_t::operator==( const hgs_t & h ) const
{
    return player == h.player && land == h.land && days == h.days;
}

bool RatingSort( const hgs_t & h1, const hgs_t & h2 )
{
    return h1.rating > h2.rating;
}

class HGSData
{
public:
    HGSData();

    bool Load( const std::string & );
    bool Save( const std::string & ) const;
    void ScoreRegistry( const std::string &, const std::string &, u32, u32 );
    void RedrawList( int32_t ox, int32_t oy );

private:
    uint32_t _monsterAnimationFrameId;
    std::vector<hgs_t> list;
    std::vector<std::pair<size_t, Monster::monster_t>> _monsterRatings;
    std::vector<std::pair<size_t, Monster::monster_t>> _monsterDays;

    Monster getMonsterByRatingStandardGame( const size_t rating ) const
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

    Monster getMonsterByRatingCampaignGame( const size_t dayCount ) const 
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
};

HGSData::HGSData()
    : _monsterAnimationFrameId( 0 )
{
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
        _monsterRatings.emplace_back( std::make_pair( ratingSoFar, monstersInRanking[i] ) );
    }
}

bool HGSData::Load( const std::string & fn )
{
    ZStreamFile hdata;
    if ( !hdata.read( fn ) )
        return false;

    hdata.setbigendian( true );
    u16 hgs_id = 0;

    hdata >> hgs_id;

    if ( hgs_id == HGS_ID ) {
        hdata >> list;
        return !hdata.fail();
    }

    return false;
}

bool HGSData::Save( const std::string & fn ) const
{
    ZStreamFile hdata;
    hdata.setbigendian( true );
    hdata << static_cast<u16>( HGS_ID ) << list;
    if ( hdata.fail() || !hdata.write( fn ) )
        return false;

    return true;
}

void HGSData::ScoreRegistry( const std::string & p, const std::string & m, u32 r, u32 s )
{
    hgs_t h;

    h.player = p;
    h.land = m;
    h.localtime = std::time( nullptr );
    h.days = r;
    h.rating = s;

    if ( list.end() == std::find( list.begin(), list.end(), h ) ) {
        list.push_back( h );
        std::sort( list.begin(), list.end(), RatingSort );
        if ( list.size() > HGS_MAX )
            list.resize( HGS_MAX );
    }
}

void HGSData::RedrawList( int32_t ox, int32_t oy )
{
    ++_monsterAnimationFrameId;

    fheroes2::Display & display = fheroes2::Display::instance();

    // image background
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HSBKG, 0 ), display, ox, oy );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HISCORE, 6 ), display, ox + 50, oy + 31 );

    std::sort( list.begin(), list.end(), RatingSort );

    std::vector<hgs_t>::const_iterator it1 = list.begin();
    std::vector<hgs_t>::const_iterator it2 = list.end();

    Text text;
    text.Set( Font::BIG );

    const std::array<uint8_t, 15> & monsterAnimationSequence = fheroes2::getMonsterAnimationSequence();

    for ( ; it1 != it2 && ( it1 - list.begin() < HGS_MAX ); ++it1 ) {
        const hgs_t & hgs = *it1;

        text.Set( hgs.player );
        text.Blit( ox + 88, oy + 70 );

        text.Set( hgs.land );
        text.Blit( ox + 244, oy + 70 );

        text.Set( std::to_string( hgs.days ) );
        text.Blit( ox + 403, oy + 70 );

        text.Set( std::to_string( hgs.rating ) );
        text.Blit( ox + 484, oy + 70 );

        const Monster monster = HGSData::getMonsterByRatingStandardGame( hgs.rating );
        const uint32_t baseMonsterAnimationIndex = monster.GetSpriteIndex() * 9;
        const fheroes2::Sprite & baseMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, baseMonsterAnimationIndex );
        fheroes2::Blit( baseMonsterSprite, display, baseMonsterSprite.x() + ox + 554, baseMonsterSprite.y() + oy + 91 );

        // Animation frame of a creature is based on its position on screen and common animation frame ID.
        const uint32_t monsterAnimationId = monsterAnimationSequence[( ox + oy + hgs.days + _monsterAnimationFrameId ) % monsterAnimationSequence.size()];
        const uint32_t secondaryMonsterAnimationIndex = baseMonsterAnimationIndex + 1 + monsterAnimationId;
        const fheroes2::Sprite & secondaryMonsterSprite = fheroes2::AGG::GetICN( ICN::MINIMON, secondaryMonsterAnimationIndex );
        fheroes2::Blit( secondaryMonsterSprite, display, secondaryMonsterSprite.x() + ox + 554, secondaryMonsterSprite.y() + oy + 91 );

        oy += 40;
    }
}

fheroes2::GameMode Game::HighScores()
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

    HGSData hgs;

    const std::string highScoreDataPath = System::ConcatePath( GetSaveDir(), "fheroes2.hgs" );

    Mixer::Pause();
    AGG::PlayMusic( MUS::MAINMENU, true, true );
    hgs.Load( highScoreDataPath );

    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HSBKG, 0 );

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Point top( ( display.width() - back.width() ) / 2, ( display.height() - back.height() ) / 2 );
    const fheroes2::StandardWindow border( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT );

    hgs.RedrawList( top.x, top.y );

    fheroes2::Button buttonCampain( top.x + 8, top.y + 315, ICN::HISCORE, 0, 1 );
    fheroes2::Button buttonExit( top.x + back.width() - 36, top.y + 315, ICN::HISCORE, 4, 5 );

    buttonCampain.disable(); // disable for now till full campaign support

    buttonCampain.draw();
    buttonExit.draw();

    display.render();

    const u32 rating = GetGameOverScores();
    const u32 days = world.CountDay();
    GameOver::Result & gameResult = GameOver::Result::Get();

    if ( rating && ( gameResult.GetResult() & GameOver::WINS ) ) {
        std::string player( _( "Unknown Hero" ) );
        Dialog::InputString( _( "Your Name" ), player );
        if ( player.empty() )
            player = _( "Unknown Hero" );
        hgs.ScoreRegistry( player, Settings::Get().CurrentFileInfo().name, days, rating );
        hgs.Save( highScoreDataPath );
        hgs.RedrawList( top.x, top.y );
        buttonCampain.draw();
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
        if ( buttonCampain.isEnabled() ) {
            le.MousePressLeft( buttonCampain.area() ) ? buttonCampain.drawOnPress() : buttonCampain.drawOnRelease();
        }
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            return fheroes2::GameMode::MAIN_MENU;

        if ( le.MousePressRight( buttonExit.area() ) ) {
            Dialog::Message( _( "Exit" ), _( "Exit this menu." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonCampain.area() ) ) {
            Dialog::Message( _( "Campaign" ), _( "View High Scores for Campaigns." ), Font::BIG );
        }

        if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
            hgs.RedrawList( top.x, top.y );
            display.render();
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}
