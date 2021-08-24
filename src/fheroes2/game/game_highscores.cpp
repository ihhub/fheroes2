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
#include <sstream>
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
    HGSData() = default;

    bool Load( const std::string & );
    bool Save( const std::string & ) const;
    void ScoreRegistry( const std::string &, const std::string &, u32, u32 );
    void RedrawList( int32_t ox, int32_t oy );
    bool validateAnimationDelay( void );

private:
    std::vector<hgs_t> list;

    static Monster getMonsterByRatingStandardGame( const size_t rating );

    std::chrono::time_point<std::chrono::steady_clock> _prevTime = std::chrono::steady_clock::now();    
    u32 hgs_animation_frame = 0;
};

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
    fheroes2::Display & display = fheroes2::Display::instance();

    // image background
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HSBKG, 0 ), display, ox, oy );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::HISCORE, 6 ), display, ox + 50, oy + 31 );

    std::sort( list.begin(), list.end(), RatingSort );

    std::vector<hgs_t>::const_iterator it1 = list.begin();
    std::vector<hgs_t>::const_iterator it2 = list.end();

    Text text;
    text.Set( Font::BIG );

    for ( ; it1 != it2 && ( it1 - list.begin() < HGS_MAX ); ++it1 ) {
        const hgs_t & hgs = *it1;

        text.Set( hgs.player );
        text.Blit( ox + 79 + 9, oy + 70 );

        text.Set( hgs.land );
        text.Blit( ox + 79 + 165 , oy + 70 );

        text.Set( std::to_string( hgs.days ) );
        text.Blit( ox + 79 + 324, oy + 70 );

        text.Set( std::to_string( hgs.rating ) );
        text.Blit( ox + 79 + 405, oy + 70 );

        const Monster monster = HGSData::getMonsterByRatingStandardGame( hgs.rating );
        const uint32_t spriteIndex = monster.GetSpriteIndex() * 9;
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINIMON, spriteIndex );
        fheroes2::Blit( sprite, display, sprite.x() + ox + 79 + 475, sprite.y() + oy + 70 + 21 );

        const std::array<uint8_t, 15> monsterAnimationSequence = fheroes2::MonsterAnimationSequence();

        //+ox+oy+hgs.days - to start from a "random" animation frame when a monster occurs more than once in the high score list
        const uint32_t animateIndex = spriteIndex + 1
              + monsterAnimationSequence[( ox + oy + hgs.days + hgs_animation_frame ) % monsterAnimationSequence.size()];
        const fheroes2::Sprite & animatedSprite = fheroes2::AGG::GetICN( ICN::MINIMON, animateIndex );
        fheroes2::Blit( animatedSprite, display, animatedSprite.x() + ox + 79 + 475, animatedSprite.y() + oy + 70 + 21 );

        oy += 40;
    }
}

Monster HGSData::getMonsterByRatingStandardGame( const size_t rating )
{
    const std::array<Monster::monster_t, 229> rating2monster = { Monster::PEASANT,
                                                                 Monster::PEASANT,
                                                                 Monster::PEASANT,
                                                                 Monster::PEASANT,
                                                                 Monster::GOBLIN,
                                                                 Monster::GOBLIN,
                                                                 Monster::GOBLIN,
                                                                 Monster::GOBLIN,
                                                                 Monster::SPRITE,
                                                                 Monster::SPRITE,
                                                                 Monster::SPRITE,
                                                                 Monster::SPRITE,
                                                                 Monster::HALFLING,
                                                                 Monster::HALFLING,
                                                                 Monster::HALFLING,
                                                                 Monster::HALFLING,
                                                                 Monster::CENTAUR,
                                                                 Monster::CENTAUR,
                                                                 Monster::CENTAUR,
                                                                 Monster::CENTAUR,
                                                                 Monster::ROGUE,
                                                                 Monster::ROGUE,
                                                                 Monster::ROGUE,
                                                                 Monster::ROGUE,
                                                                 Monster::SKELETON,
                                                                 Monster::SKELETON,
                                                                 Monster::SKELETON,
                                                                 Monster::SKELETON,
                                                                 Monster::ORC,
                                                                 Monster::ORC,
                                                                 Monster::ORC,
                                                                 Monster::ORC,
                                                                 Monster::ZOMBIE,
                                                                 Monster::ZOMBIE,
                                                                 Monster::ZOMBIE,
                                                                 Monster::ZOMBIE,
                                                                 Monster::ARCHER,
                                                                 Monster::ARCHER,
                                                                 Monster::ARCHER,
                                                                 Monster::ARCHER,
                                                                 Monster::RANGER,
                                                                 Monster::RANGER,
                                                                 Monster::RANGER,
                                                                 Monster::RANGER,
                                                                 Monster::BOAR,
                                                                 Monster::BOAR,
                                                                 Monster::BOAR,
                                                                 Monster::BOAR,
                                                                 Monster::DWARF,
                                                                 Monster::DWARF,
                                                                 Monster::DWARF,
                                                                 Monster::DWARF,
                                                                 Monster::MUTANT_ZOMBIE,
                                                                 Monster::MUTANT_ZOMBIE,
                                                                 Monster::MUTANT_ZOMBIE,
                                                                 Monster::MUTANT_ZOMBIE,
                                                                 Monster::ORC_CHIEF,
                                                                 Monster::ORC_CHIEF,
                                                                 Monster::ORC_CHIEF,
                                                                 Monster::ORC_CHIEF,
                                                                 Monster::ELF,
                                                                 Monster::ELF,
                                                                 Monster::ELF,
                                                                 Monster::ELF,
                                                                 Monster::GARGOYLE,
                                                                 Monster::GARGOYLE,
                                                                 Monster::GARGOYLE,
                                                                 Monster::GARGOYLE,
                                                                 Monster::PIKEMAN,
                                                                 Monster::PIKEMAN,
                                                                 Monster::PIKEMAN,
                                                                 Monster::PIKEMAN,
                                                                 Monster::GRAND_ELF,
                                                                 Monster::GRAND_ELF,
                                                                 Monster::GRAND_ELF,
                                                                 Monster::GRAND_ELF,
                                                                 Monster::BATTLE_DWARF,
                                                                 Monster::BATTLE_DWARF,
                                                                 Monster::BATTLE_DWARF,
                                                                 Monster::BATTLE_DWARF,
                                                                 Monster::NOMAD,
                                                                 Monster::NOMAD,
                                                                 Monster::NOMAD,
                                                                 Monster::NOMAD,
                                                                 Monster::VETERAN_PIKEMAN,
                                                                 Monster::VETERAN_PIKEMAN,
                                                                 Monster::VETERAN_PIKEMAN,
                                                                 Monster::VETERAN_PIKEMAN,
                                                                 Monster::WOLF,
                                                                 Monster::WOLF,
                                                                 Monster::WOLF,
                                                                 Monster::WOLF,
                                                                 Monster::MUMMY,
                                                                 Monster::MUMMY,
                                                                 Monster::MUMMY,
                                                                 Monster::MUMMY,
                                                                 Monster::IRON_GOLEM,
                                                                 Monster::IRON_GOLEM,
                                                                 Monster::IRON_GOLEM,
                                                                 Monster::IRON_GOLEM,
                                                                 Monster::ROYAL_MUMMY,
                                                                 Monster::ROYAL_MUMMY,
                                                                 Monster::ROYAL_MUMMY,
                                                                 Monster::ROYAL_MUMMY,
                                                                 Monster::OGRE,
                                                                 Monster::OGRE,
                                                                 Monster::OGRE,
                                                                 Monster::OGRE,
                                                                 Monster::GRIFFIN,
                                                                 Monster::GRIFFIN,
                                                                 Monster::GRIFFIN,
                                                                 Monster::GRIFFIN,
                                                                 Monster::SWORDSMAN,
                                                                 Monster::SWORDSMAN,
                                                                 Monster::SWORDSMAN,
                                                                 Monster::SWORDSMAN,
                                                                 Monster::DRUID,
                                                                 Monster::DRUID,
                                                                 Monster::DRUID,
                                                                 Monster::DRUID,
                                                                 Monster::STEEL_GOLEM,
                                                                 Monster::STEEL_GOLEM,
                                                                 Monster::STEEL_GOLEM,
                                                                 Monster::STEEL_GOLEM,
                                                                 Monster::MASTER_SWORDSMAN,
                                                                 Monster::MASTER_SWORDSMAN,
                                                                 Monster::MASTER_SWORDSMAN,
                                                                 Monster::MASTER_SWORDSMAN,
                                                                 Monster::AIR_ELEMENT,
                                                                 Monster::AIR_ELEMENT,
                                                                 Monster::AIR_ELEMENT,
                                                                 Monster::AIR_ELEMENT,
                                                                 Monster::GREATER_DRUID,
                                                                 Monster::GREATER_DRUID,
                                                                 Monster::GREATER_DRUID,
                                                                 Monster::FIRE_ELEMENT,
                                                                 Monster::FIRE_ELEMENT,
                                                                 Monster::FIRE_ELEMENT,
                                                                 Monster::GHOST,
                                                                 Monster::GHOST,
                                                                 Monster::GHOST,
                                                                 Monster::VAMPIRE,
                                                                 Monster::VAMPIRE,
                                                                 Monster::VAMPIRE,
                                                                 Monster::WATER_ELEMENT,
                                                                 Monster::WATER_ELEMENT,
                                                                 Monster::WATER_ELEMENT,
                                                                 Monster::EARTH_ELEMENT,
                                                                 Monster::EARTH_ELEMENT,
                                                                 Monster::EARTH_ELEMENT,
                                                                 Monster::ROC,
                                                                 Monster::ROC,
                                                                 Monster::ROC,
                                                                 Monster::MINOTAUR,
                                                                 Monster::MINOTAUR,
                                                                 Monster::MINOTAUR,
                                                                 Monster::CAVALRY,
                                                                 Monster::CAVALRY,
                                                                 Monster::CAVALRY,
                                                                 Monster::TROLL,
                                                                 Monster::TROLL,
                                                                 Monster::TROLL,
                                                                 Monster::MAGE,
                                                                 Monster::MAGE,
                                                                 Monster::MAGE,
                                                                 Monster::MEDUSA,
                                                                 Monster::MEDUSA,
                                                                 Monster::MEDUSA,
                                                                 Monster::LICH,
                                                                 Monster::LICH,
                                                                 Monster::LICH,
                                                                 Monster::OGRE_LORD,
                                                                 Monster::OGRE_LORD,
                                                                 Monster::OGRE_LORD,
                                                                 Monster::MINOTAUR_KING,
                                                                 Monster::MINOTAUR_KING,
                                                                 Monster::MINOTAUR_KING,
                                                                 Monster::CHAMPION,
                                                                 Monster::CHAMPION,
                                                                 Monster::CHAMPION,
                                                                 Monster::WAR_TROLL,
                                                                 Monster::WAR_TROLL,
                                                                 Monster::WAR_TROLL,
                                                                 Monster::VAMPIRE_LORD,
                                                                 Monster::VAMPIRE_LORD,
                                                                 Monster::VAMPIRE_LORD,
                                                                 Monster::ARCHMAGE,
                                                                 Monster::ARCHMAGE,
                                                                 Monster::ARCHMAGE,
                                                                 Monster::POWER_LICH,
                                                                 Monster::POWER_LICH,
                                                                 Monster::POWER_LICH,
                                                                 Monster::UNICORN,
                                                                 Monster::UNICORN,
                                                                 Monster::UNICORN,
                                                                 Monster::HYDRA,
                                                                 Monster::HYDRA,
                                                                 Monster::HYDRA,
                                                                 Monster::PALADIN,
                                                                 Monster::PALADIN,
                                                                 Monster::PALADIN,
                                                                 Monster::GENIE,
                                                                 Monster::GENIE,
                                                                 Monster::GENIE,
                                                                 Monster::CRUSADER,
                                                                 Monster::CRUSADER,
                                                                 Monster::CRUSADER,
                                                                 Monster::CYCLOPS,
                                                                 Monster::CYCLOPS,
                                                                 Monster::CYCLOPS,
                                                                 Monster::GIANT,
                                                                 Monster::GIANT,
                                                                 Monster::GIANT,
                                                                 Monster::PHOENIX,
                                                                 Monster::PHOENIX,
                                                                 Monster::PHOENIX,
                                                                 Monster::BONE_DRAGON,
                                                                 Monster::BONE_DRAGON,
                                                                 Monster::BONE_DRAGON,
                                                                 Monster::GREEN_DRAGON,
                                                                 Monster::GREEN_DRAGON,
                                                                 Monster::GREEN_DRAGON,
                                                                 Monster::RED_DRAGON,
                                                                 Monster::RED_DRAGON,
                                                                 Monster::RED_DRAGON,
                                                                 Monster::TITAN,
                                                                 Monster::TITAN,
                                                                 Monster::TITAN,
                                                                 Monster::BLACK_DRAGON };

    return Monster( rating2monster[std::min( rating, rating2monster.size() - 1 )] );
}

bool HGSData::validateAnimationDelay()
{
    const std::chrono::duration<double> time = std::chrono::steady_clock::now() - HGSData::_prevTime;
    const uint64_t passedMs = static_cast<uint64_t>( time.count() * 1000 + 0.5 );
    if ( passedMs >= 200 ) {
        _prevTime = std::chrono::steady_clock::now();
        hgs_animation_frame++;
        return true;
    }
    return false;
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

    std::ostringstream stream;
    stream << System::ConcatePath( GetSaveDir(), "fheroes2.hgs" );

    Mixer::Pause();
    AGG::PlayMusic( MUS::MAINMENU, true, true );
    hgs.Load( stream.str() );

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
        hgs.Save( stream.str() );
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

        if ( hgs.validateAnimationDelay() ) {
            hgs.RedrawList( top.x, top.y );
            display.render();
        }
    }

    return fheroes2::GameMode::QUIT_GAME;
}
