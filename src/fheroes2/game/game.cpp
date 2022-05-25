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
#include <atomic>
#include <cassert>
#include <cmath>

#include "agg.h"
#include "audio.h"
#include "cursor.h"
#include "difficulty.h"
#include "game.h"
#include "game_credits.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_static.h"
#include "icn.h"
#include "m82.h"
#include "maps_tiles.h"
#include "monster.h"
#include "mp2.h"
#include "rand.h"
#include "save_format_version.h"
#include "settings.h"
#include "skill.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

namespace
{
    std::string lastMapFileName;
    std::vector<Player> savedPlayers;

    uint16_t save_version = CURRENT_FORMAT_VERSION;

    std::string last_name;

    bool updateSoundsOnFocusUpdate = true;
    std::atomic<int> currentMusic{ MUS::UNKNOWN };

    uint32_t maps_animation_frame = 0;

    std::vector<int> reserved_vols( LOOPXX_COUNT, 0 );

    uint32_t GetMixerChannelFromObject( const Maps::Tiles & tile )
    {
        // check stream first
        if ( tile.isStream() ) {
            return 13;
        }

        return M82::GetIndexLOOP00XXFromObject( tile.GetObject( false ) );
    }
}

namespace Game
{
    void AnimateDelaysInitialize();

    namespace ObjectFadeAnimation
    {
        FadeTask::FadeTask( MP2::MapObjectType object_, uint32_t objectIndex_, uint32_t animationIndex_, int32_t fromIndex_, int32_t toIndex_, uint8_t alpha_,
                            bool fadeOut_, bool fadeIn_, uint8_t objectTileset_ )
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
    const Settings & configuration = Settings::Get();

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

void Game::SetLoadVersion( uint16_t ver )
{
    save_version = ver;
}

uint16_t Game::GetLoadVersion()
{
    return save_version;
}

const std::string & Game::GetLastSavename()
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

bool Game::UpdateSoundsOnFocusUpdate()
{
    return updateSoundsOnFocusUpdate;
}

void Game::SetUpdateSoundsOnFocusUpdate( bool update )
{
    updateSoundsOnFocusUpdate = update;
}

void Game::Init()
{
    // default events
    LocalEvent::SetStateDefaults();

    // set global events
    LocalEvent & le = LocalEvent::Get();
    le.SetGlobalFilterMouseEvents( Cursor::Redraw );
    le.SetGlobalFilterKeysEvents( Game::KeyboardGlobalFilter );

    Game::AnimateDelaysInitialize();

    Game::HotKeysLoad( Settings::GetLastFile( "", "fheroes2.key" ) );
}

int Game::CurrentMusic()
{
    return currentMusic;
}

void Game::SetCurrentMusic( const int mus )
{
    currentMusic = mus;
}

void Game::ObjectFadeAnimation::PrepareFadeTask( const MP2::MapObjectType objectType, int32_t fromIndex, int32_t toIndex, bool fadeOut, bool fadeIn )
{
    const uint8_t alpha = fadeOut ? 255u : 0;
    const Maps::Tiles & fromTile = world.GetTiles( fromIndex );

    if ( objectType == MP2::OBJ_ZERO ) {
        fadeTask = FadeTask();
    }
    else if ( objectType == MP2::OBJ_MONSTER ) {
        const auto & spriteIndicies = Maps::Tiles::GetMonsterSpriteIndices( fromTile, fromTile.QuantityMonster().GetSpriteIndex() );

        fadeTask = FadeTask( objectType, spriteIndicies.first, spriteIndicies.second, fromIndex, toIndex, alpha, fadeOut, fadeIn, 0 );
    }
    else if ( objectType == MP2::OBJ_BOAT ) {
        fadeTask = FadeTask( objectType, fromTile.GetObjectSpriteIndex(), 0, fromIndex, toIndex, alpha, fadeOut, fadeIn, 0 );
    }
    else {
        const int icn = MP2::GetICNObject( fromTile.GetObjectTileset() );
        const uint32_t animationIndex = ICN::AnimationFrame( icn, fromTile.GetObjectSpriteIndex(), Game::MapsAnimationFrame(), fromTile.GetQuantity2() != 0 );

        fadeTask = FadeTask( objectType, fromTile.GetObjectSpriteIndex(), animationIndex, fromIndex, toIndex, alpha, fadeOut, fadeIn, fromTile.GetObjectTileset() );
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
        const Interface::GameArea & gameArea = Interface::Basic::Get().GetGameArea();

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

uint32_t & Game::MapsAnimationFrame()
{
    return maps_animation_frame;
}

// play environment sounds from the game area in focus
void Game::EnvironmentSoundMixer()
{
    size_t availableChannels = Mixer::getChannelCount();
    if ( availableChannels == 0 ) {
        return;
    }

    const fheroes2::Point abs_pt( Interface::GetFocusCenter() );
    std::fill( reserved_vols.begin(), reserved_vols.end(), 0 );

    const int32_t maxOffset = 3;

    // Get all valid positions within 7 x 7 area.
    std::vector<fheroes2::Point> positions;
    for ( int32_t y = -maxOffset; y <= maxOffset; ++y ) {
        for ( int32_t x = -maxOffset; x <= maxOffset; ++x ) {
            if ( Maps::isValidAbsPoint( x + abs_pt.x, y + abs_pt.y ) ) {
                positions.emplace_back( x, y );
            }
        }
    }

    // Sort positions by distance to the center.
    std::stable_sort( positions.begin(), positions.end(),
                      []( const fheroes2::Point & p1, const fheroes2::Point & p2 ) { return p1.x * p1.x + p1.y * p1.y < p2.x * p2.x + p2.y * p2.y; } );

    const double maxDistance = std::sqrt( maxOffset * maxOffset + maxOffset * maxOffset );
    double maxVolume = Mixer::MaxVolume();
    double minVolumeOnMaxDistance = maxVolume * 0.1; // 10% from maximum volume

    maxVolume -= minVolumeOnMaxDistance; // need to remove these 10% from max value as we're going to add it later
    minVolumeOnMaxDistance += 0.5; // this is done to make casting faster. We know that the value is always positive.

    for ( const fheroes2::Point & pos : positions ) {
        const uint32_t channel = GetMixerChannelFromObject( world.GetTiles( pos.x + abs_pt.x, pos.y + abs_pt.y ) );
        if ( channel < reserved_vols.size() ) {
            const double distance = std::sqrt( pos.x * pos.x + pos.y * pos.y );
            const int32_t volume = static_cast<int32_t>( ( ( maxDistance - distance ) / maxDistance ) * maxVolume + minVolumeOnMaxDistance );

            if ( reserved_vols[channel] == 0 ) {
                if ( availableChannels == 0 ) {
                    // No new channel can be added.
                    continue;
                }
                --availableChannels;
            }

            if ( volume > reserved_vols[channel] ) {
                reserved_vols[channel] = volume;
            }
        }
    }

    AGG::LoadLOOPXXSounds( reserved_vols, true );
}

void Game::restoreSoundsForCurrentFocus()
{
    Game::SetCurrentMusic( MUS::UNKNOWN );
    AGG::ResetAudio();

    switch ( Interface::GetFocusType() ) {
    case GameFocus::HEROES: {
        const Heroes * focusedHero = Interface::GetFocusHeroes();
        assert( focusedHero != nullptr );

        const int heroIndexPos = focusedHero->GetIndex();
        if ( heroIndexPos >= 0 ) {
            Game::EnvironmentSoundMixer();
            AGG::PlayMusic( MUS::FromGround( world.GetTiles( heroIndexPos ).GetGround() ), true, true );
        }
        break;
    }

    case GameFocus::CASTLE: {
        const Castle * focusedCastle = Interface::GetFocusCastle();
        assert( focusedCastle != nullptr );

        Game::EnvironmentSoundMixer();
        AGG::PlayMusic( MUS::FromGround( world.GetTiles( focusedCastle->GetIndex() ).GetGround() ), true, true );
        break;
    }

    default:
        break;
    }
}

uint32_t Game::GetRating()
{
    const Settings & conf = Settings::Get();
    uint32_t rating = 50;

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

uint32_t Game::GetGameOverScores()
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
    default:
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

uint32_t Game::GetLostTownDays()
{
    return GameStatic::GetGameOverLostDays();
}

uint32_t Game::GetWhirlpoolPercent()
{
    return GameStatic::GetLostOnWhirlpoolPercent();
}

int Game::GetKingdomColors()
{
    return Settings::Get().GetPlayers().GetColors();
}

int Game::GetActualKingdomColors()
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

void Game::PlayPickupSound()
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
