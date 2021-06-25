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
#include <cassert>
#include <cmath>

#include "agg.h"
#include "audio_mixer.h"
#include "cursor.h"
#include "difficulty.h"
#include "game.h"
#include "game_credits.h"
#include "game_delays.h"
#include "game_interface.h"
#include "game_static.h"
#include "icn.h"
#include "m82.h"
#include "maps_tiles.h"
#include "monster.h"
#include "mp2.h"
#include "mus.h"
#include "rand.h"
#include "save_format_version.h"
#include "settings.h"
#include "skill.h"
#include "text.h"
#include "tools.h"
#include "world.h"

namespace Game
{
    u32 GetMixerChannelFromObject( const Maps::Tiles & );
    void AnimateDelaysInitialize( void );
    void KeyboardGlobalFilter( int, int );

    void HotKeysDefaults( void );
    void HotKeysLoad( const std::string & );

    bool disable_change_music = false;
    int current_music = MUS::UNKNOWN;
    u32 castle_animation_frame = 0;
    u32 maps_animation_frame = 0;
    std::string last_name;
    int save_version = CURRENT_FORMAT_VERSION;
    std::vector<int> reserved_vols( LOOPXX_COUNT, 0 );
    std::string lastMapFileName;
    std::vector<Player> savedPlayers;

    namespace ObjectFadeAnimation
    {
        FadeTask::FadeTask( int object_, uint32_t objectIndex_, uint32_t animationIndex_, int32_t fromIndex_, int32_t toIndex_, uint8_t alpha_, bool fadeOut_,
                            bool fadeIn_, uint8_t objectTileset_ )
            : object( object_ )
            , objectIndex( objectIndex_ )
            , animationIndex( animationIndex_ )
            , fromIndex( fromIndex_ )
            , toIndex( toIndex_ )
            , alpha( alpha_ )
            , fadeOut( fadeOut_ )
            , fadeIn( fadeIn_ )
            , objectTileset( objectTileset_ )
        {}

        FadeTask::FadeTask()
            : object( MP2::OBJ_ZERO )
            , objectIndex( 0 )
            , animationIndex( 0 )
            , fromIndex( 0 )
            , toIndex( 0 )
            , alpha( 0 )
            , fadeOut( false )
            , fadeIn( false )
            , objectTileset( 0 )

        {}

        // Single instance of FadeTask.
        FadeTask fadeTask;
    }
}

// Returns the difficulty level based on the type of game.
int Game::getDifficulty()
{
    Settings & configuration = Settings::Get();

    return ( configuration.isCampaignGameType() ? configuration.CurrentFileInfo().difficulty : configuration.GameDifficulty() );
}

void Game::LoadPlayers( const std::string & mapFileName, Players & players )
{
    if ( lastMapFileName != mapFileName || savedPlayers.size() != players.size() ) {
        return;
    }

    const auto newHumanCount = std::count_if( players.begin(), players.end(), []( const Player * player ) { return player->GetControl() == CONTROL_HUMAN; } );
    const auto savedHumanCount = std::count_if( savedPlayers.begin(), savedPlayers.end(), []( const Player & player ) { return player.GetControl() == CONTROL_HUMAN; } );

    if ( newHumanCount != savedHumanCount ) {
        return;
    }

    players.clear();
    for ( const Player & p : savedPlayers ) {
        Player * player = new Player( p.GetColor() );
        player->SetRace( p.GetRace() );
        player->SetControl( p.GetControl() );
        player->SetFriends( p.GetFriends() );
        player->SetName( p.GetName() );
        players.push_back( player );
        Players::Set( Color::GetIndex( p.GetColor() ), player );
    }
}

void Game::saveDifficulty( const int difficulty )
{
    Settings::Get().SetGameDifficulty( difficulty );
}

void Game::SavePlayers( const std::string & mapFileName, const Players & players )
{
    lastMapFileName = mapFileName;
    savedPlayers.clear();
    for ( const Player * p : players ) {
        Player player( p->GetColor() );
        player.SetRace( p->GetRace() );
        player.SetControl( p->GetControl() );
        player.SetFriends( p->GetFriends() );
        player.SetName( p->GetName() );
        savedPlayers.push_back( player );
    }
}

void Game::SetLoadVersion( int ver )
{
    save_version = ver;
}

int Game::GetLoadVersion( void )
{
    return save_version;
}

const std::string & Game::GetLastSavename( void )
{
    return last_name;
}

void Game::SetLastSavename( const std::string & name )
{
    last_name = name;
}

fheroes2::GameMode Game::Credits()
{
    ShowCredits();

    return fheroes2::GameMode::MAIN_MENU;
}

bool Game::ChangeMusicDisabled( void )
{
    return disable_change_music;
}

void Game::DisableChangeMusic( bool /*f*/ )
{
    // disable_change_music = f;
}

void Game::Init( void )
{
    // default events
    LocalEvent::SetStateDefaults();

    // set global events
    LocalEvent & le = LocalEvent::Get();
    le.SetGlobalFilterMouseEvents( Cursor::Redraw );
    le.SetGlobalFilterKeysEvents( Game::KeyboardGlobalFilter );

    Game::AnimateDelaysInitialize();

    HotKeysDefaults();

    const std::string hotkeys = Settings::GetLastFile( "", "fheroes2.key" );
    Game::HotKeysLoad( hotkeys );
}

int Game::CurrentMusic( void )
{
    return current_music;
}

void Game::SetCurrentMusic( int mus )
{
    current_music = mus;
}

void Game::ObjectFadeAnimation::PrepareFadeTask( int object, int32_t fromIndex, int32_t toIndex, bool fadeOut, bool fadeIn )
{
    const uint8_t alpha = fadeOut ? 255u : 0;
    const Maps::Tiles & fromTile = world.GetTiles( fromIndex );

    if ( object == MP2::OBJ_ZERO ) {
        fadeTask = FadeTask();
    }
    else if ( object == MP2::OBJ_MONSTER ) {
        const auto & spriteIndicies = Maps::Tiles::GetMonsterSpriteIndices( fromTile, fromTile.QuantityMonster().GetSpriteIndex() );

        fadeTask = FadeTask( object, spriteIndicies.first, spriteIndicies.second, fromIndex, toIndex, alpha, fadeOut, fadeIn, 0 );
    }
    else if ( object == MP2::OBJ_BOAT ) {
        fadeTask = FadeTask( object, fromTile.GetObjectSpriteIndex(), 0, fromIndex, toIndex, alpha, fadeOut, fadeIn, 0 );
    }
    else {
        const int icn = MP2::GetICNObject( object );
        const uint32_t animationIndex = ICN::AnimationFrame( icn, fromTile.GetObjectSpriteIndex(), Game::MapsAnimationFrame(), fromTile.GetQuantity2() != 0 );

        fadeTask = FadeTask( object, fromTile.GetObjectSpriteIndex(), animationIndex, fromIndex, toIndex, alpha, fadeOut, fadeIn, fromTile.GetObjectTileset() );
    }
}

void Game::ObjectFadeAnimation::PerformFadeTask()
{
    auto removeObject = []() {
        Maps::Tiles & tile = world.GetTiles( fadeTask.fromIndex );

        if ( tile.GetObject() == fadeTask.object ) {
            tile.RemoveObjectSprite();
            tile.setAsEmpty();
        }
    };
    auto addObject = []() {
        Maps::Tiles & tile = world.GetTiles( fadeTask.toIndex );

        if ( tile.GetObject() != fadeTask.object && fadeTask.object == MP2::OBJ_BOAT ) {
            tile.setBoat( Direction::RIGHT );
        }
    };
    auto redrawGameArea = []() {
        fheroes2::Display & display = fheroes2::Display::instance();
        Interface::GameArea & gameArea = Interface::Basic::Get().GetGameArea();

        gameArea.Redraw( display, Interface::LEVEL_ALL );

        display.render();
    };

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() && ( fadeTask.fadeOut || fadeTask.fadeIn ) ) {
        if ( Game::validateAnimationDelay( Game::HEROES_PICKUP_DELAY ) ) {
            if ( fadeTask.fadeOut ) {
                if ( fadeTask.alpha > 20 ) {
                    fadeTask.alpha -= 20;
                }
                else {
                    removeObject();

                    if ( fadeTask.fadeIn ) {
                        fadeTask.fadeOut = false;
                        fadeTask.alpha = 0;
                    }
                    else {
                        fadeTask = FadeTask();
                    }
                }
            }
            else if ( fadeTask.fadeIn ) {
                if ( fadeTask.alpha == 0 ) {
                    addObject();
                }

                if ( fadeTask.alpha < 235 ) {
                    fadeTask.alpha += 20;
                }
                else {
                    fadeTask = FadeTask();
                }
            }

            redrawGameArea();
        }
    }

    if ( fadeTask.fadeOut ) {
        removeObject();
    }

    if ( fadeTask.fadeIn ) {
        addObject();
    }

    fadeTask = FadeTask();

    redrawGameArea();
}

const Game::ObjectFadeAnimation::FadeTask & Game::ObjectFadeAnimation::GetFadeTask()
{
    return fadeTask;
}

u32 & Game::MapsAnimationFrame( void )
{
    return maps_animation_frame;
}

u32 & Game::CastleAnimationFrame( void )
{
    return castle_animation_frame;
}

/* play all sound from focus area game */
void Game::EnvironmentSoundMixer( void )
{
    if ( !Settings::Get().Sound() ) {
        return;
    }

    const fheroes2::Point abs_pt( Interface::GetFocusCenter() );
    std::fill( reserved_vols.begin(), reserved_vols.end(), 0 );

    // scan 4x4 square from focus
    for ( s32 yy = abs_pt.y - 3; yy <= abs_pt.y + 3; ++yy ) {
        for ( s32 xx = abs_pt.x - 3; xx <= abs_pt.x + 3; ++xx ) {
            if ( Maps::isValidAbsPoint( xx, yy ) ) {
                const u32 channel = GetMixerChannelFromObject( world.GetTiles( xx, yy ) );
                if ( channel < reserved_vols.size() ) {
                    // calculation volume
                    const int length = std::max( std::abs( xx - abs_pt.x ), std::abs( yy - abs_pt.y ) );
                    const int volume = ( 2 < length ? 4 : ( 1 < length ? 8 : ( 0 < length ? 12 : 16 ) ) ) * Mixer::MaxVolume() / 16;

                    if ( volume > reserved_vols[channel] )
                        reserved_vols[channel] = volume;
                }
            }
        }
    }

    AGG::LoadLOOPXXSounds( reserved_vols, true );
}

u32 Game::GetMixerChannelFromObject( const Maps::Tiles & tile )
{
    // force: check stream
    if ( tile.isStream() )
        return 13;

    return M82::GetIndexLOOP00XXFromObject( tile.GetObject( false ) );
}

u32 Game::GetRating( void )
{
    const Settings & conf = Settings::Get();
    u32 rating = 50;

    switch ( conf.MapsDifficulty() ) {
    case Difficulty::NORMAL:
        rating += 20;
        break;
    case Difficulty::HARD:
        rating += 40;
        break;
    case Difficulty::EXPERT:
    case Difficulty::IMPOSSIBLE:
        rating += 80;
        break;
    default:
        break;
    }

    switch ( Game::getDifficulty() ) {
    case Difficulty::NORMAL:
        rating += 30;
        break;
    case Difficulty::HARD:
        rating += 50;
        break;
    case Difficulty::EXPERT:
        rating += 70;
        break;
    case Difficulty::IMPOSSIBLE:
        rating += 90;
        break;
    default:
        break;
    }

    return rating;
}

u32 Game::GetGameOverScores( void )
{
    const Settings & conf = Settings::Get();

    uint32_t mapSizeFactor = 0;

    switch ( conf.MapsSize().width ) {
    case Maps::SMALL:
        mapSizeFactor = 140;
        break;
    case Maps::MEDIUM:
        mapSizeFactor = 100;
        break;
    case Maps::LARGE:
        mapSizeFactor = 80;
        break;
    case Maps::XLARGE:
        mapSizeFactor = 60;
        break;
    }

    const uint32_t daysFactor = world.CountDay() * mapSizeFactor / 100;

    uint32_t daysScore = 0;

    if ( daysFactor <= 60 ) {
        daysScore = daysFactor;
    }
    else if ( daysFactor <= 120 ) {
        daysScore = daysFactor / 2 + 30;
    }
    else if ( daysFactor <= 360 ) {
        daysScore = daysFactor / 4 + 60;
    }
    else if ( daysFactor <= 600 ) {
        daysScore = daysFactor / 8 + 105;
    }
    else {
        daysScore = 180;
    }

    return GetRating() * ( 200 - daysScore ) / 100;
}

void Game::ShowMapLoadingText( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Rect pos( 0, display.height() / 2, display.width(), display.height() / 2 );
    TextBox text( _( "Map is loading..." ), Font::BIG, pos.width );

    // blit test
    display.fill( 0 );
    text.Blit( pos.x, pos.y );
    display.render();
}

u32 Game::GetLostTownDays( void )
{
    return GameStatic::GetGameOverLostDays();
}

u32 Game::GetViewDistance( u32 d )
{
    return GameStatic::GetOverViewDistance( d );
}

u32 Game::GetWhirlpoolPercent( void )
{
    return GameStatic::GetLostOnWhirlpoolPercent();
}

std::string Game::GetEncodeString( const std::string & str1 )
{
    const Settings & conf = Settings::Get();

    // encode name
    if ( conf.Unicode() && conf.MapsCharset().size() )
        return EncodeString( str1.c_str(), conf.MapsCharset().c_str() );

    return str1;
}

int Game::GetKingdomColors( void )
{
    return Settings::Get().GetPlayers().GetColors();
}

int Game::GetActualKingdomColors( void )
{
    return Settings::Get().GetPlayers().GetActualColors();
}

std::string Game::CountScoute( uint32_t count, int scoute, bool shorts )
{
    double infelicity = 0;
    std::string res;

    switch ( scoute ) {
    case Skill::Level::BASIC:
        infelicity = count * 30 / 100.0;
        break;

    case Skill::Level::ADVANCED:
        infelicity = count * 15 / 100.0;
        break;

    case Skill::Level::EXPERT:
        res = shorts ? GetStringShort( count ) : std::to_string( count );
        break;

    default:
        return Army::SizeString( count );
    }

    if ( res.empty() ) {
        uint32_t min = Rand::Get( static_cast<uint32_t>( std::floor( count - infelicity + 0.5 ) ), static_cast<uint32_t>( std::floor( count + infelicity + 0.5 ) ) );
        uint32_t max = 0;

        if ( min > count ) {
            max = min;
            min = static_cast<uint32_t>( std::floor( count - infelicity + 0.5 ) );
        }
        else
            max = static_cast<uint32_t>( std::floor( count + infelicity + 0.5 ) );

        res = std::to_string( min );

        if ( min != max ) {
            res.append( "-" );
            res.append( std::to_string( max ) );
        }
    }

    return res;
}

std::string Game::CountThievesGuild( uint32_t monsterCount, int guildCount )
{
    assert( guildCount > 0 );
    return guildCount == 1 ? "???" : Army::SizeString( monsterCount );
}

void Game::PlayPickupSound( void )
{
    int wav = M82::UNKNOWN;

    switch ( Rand::Get( 1, 7 ) ) {
    case 1:
        wav = M82::PICKUP01;
        break;
    case 2:
        wav = M82::PICKUP02;
        break;
    case 3:
        wav = M82::PICKUP03;
        break;
    case 4:
        wav = M82::PICKUP04;
        break;
    case 5:
        wav = M82::PICKUP05;
        break;
    case 6:
        wav = M82::PICKUP06;
        break;
    case 7:
        wav = M82::PICKUP07;
        break;

    default:
        return;
    }

    AGG::PlaySound( wav );
}
