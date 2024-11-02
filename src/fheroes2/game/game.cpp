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

#include "game.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "army.h"
#include "audio.h"
#include "audio_manager.h"
#include "campaign_savedata.h"
#include "castle.h"
#include "cursor.h"
#include "difficulty.h"
#include "game_credits.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_static.h"
#include "heroes.h"
#include "localevent.h"
#include "m82.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mus.h"
#include "players.h"
#include "rand.h"
#include "settings.h"
#include "tools.h"
#include "ui_constants.h"
#include "world.h"

namespace
{
    std::string lastMapFileName;
    std::vector<Player> savedPlayers;

    bool updateSoundsOnFocusUpdate = true;
    bool needFadeIn{ true };

    uint32_t maps_animation_frame = 0;
}

namespace Game
{
    void AnimateDelaysInitialize();
}

bool Game::isCampaign()
{
    return Settings::Get().isCampaignGameType();
}

int Game::getDifficulty()
{
    const Settings & configuration = Settings::Get();

    // Difficulty of non-campaign games depends only on the difficulty settings set by the player
    if ( !configuration.isCampaignGameType() ) {
        return configuration.GameDifficulty();
    }

    // Difficulty of campaign games depends on both the difficulty of a particular campaign map and the difficulty settings set by the player
    int difficulty = Campaign::getCurrentScenarioDifficultyLevel().value_or( configuration.getCurrentMapInfo().difficulty );
    const int difficultyAdjustment = Campaign::CampaignSaveData::Get().getDifficulty();

    difficulty += difficultyAdjustment;

    return std::clamp( difficulty, static_cast<int>( Difficulty::EASY ), static_cast<int>( Difficulty::IMPOSSIBLE ) );
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
        player->setHandicapStatus( p.getHandicapStatus() );

        players.push_back( player );

        Players::Set( p.GetColor(), player );
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
        assert( p != nullptr );

        Player player( p->GetColor() );

        player.SetRace( p->GetRace() );
        player.SetControl( p->GetControl() );
        player.SetFriends( p->GetFriends() );
        player.SetName( p->GetName() );
        player.setHandicapStatus( p->getHandicapStatus() );

        savedPlayers.push_back( player );
    }
}

fheroes2::GameMode Game::Credits()
{
    ShowCredits( true );

    return fheroes2::GameMode::MAIN_MENU;
}

bool Game::UpdateSoundsOnFocusUpdate()
{
    return updateSoundsOnFocusUpdate;
}

void Game::SetUpdateSoundsOnFocusUpdate( const bool update )
{
    updateSoundsOnFocusUpdate = update;
}

bool Game::isFadeInNeeded()
{
    return needFadeIn;
}

void Game::setDisplayFadeIn()
{
    needFadeIn = true;
}
bool Game::validateDisplayFadeIn()
{
    if ( needFadeIn ) {
        needFadeIn = false;
        return true;
    }

    return false;
}

void Game::Init()
{
    // set global events
    LocalEvent & le = LocalEvent::Get();
    le.setGlobalMouseMotionEventHook( Cursor::updateCursorPosition );
    le.setGlobalKeyDownEventHook( Game::globalKeyDownEvent );

    Game::AnimateDelaysInitialize();

    Game::HotKeysLoad( Settings::GetLastFile( "", "fheroes2.key" ) );
}

uint32_t Game::getAdventureMapAnimationIndex()
{
    return maps_animation_frame;
}

void Game::updateAdventureMapAnimationIndex()
{
    ++maps_animation_frame;
}

void Game::EnvironmentSoundMixer()
{
    int availableChannels = Mixer::getChannelCount();
    if ( availableChannels <= 2 ) {
        // 2 channels are left for hero's movement.
        return;
    }

    availableChannels -= 2;

    fheroes2::Point center;
    fheroes2::Point tilePixelOffset;

    Player * player = Settings::Get().GetPlayers().GetCurrent();
    if ( player != nullptr ) {
        Focus & focus = player->GetFocus();

        const Heroes * hero = focus.GetHeroes();
        if ( hero != nullptr ) {
            center = hero->GetCenter();
            tilePixelOffset = hero->getCurrentPixelOffset();
        }
        else if ( focus.GetCastle() ) {
            center = focus.GetCastle()->GetCenter();
        }
        else {
            center = { world.w() / 2, world.h() / 2 };
        }
    }
    else {
        center = { world.w() / 2, world.h() / 2 };
    }

    std::map<M82::SoundType, std::vector<AudioManager::AudioLoopEffectInfo>> soundEffects;

    const int32_t maxOffset = 3;

    // Usual area of getting object sounds around a center is 7 x 7 pixel. However, in case of a moving hero we need to expand the area to make sound transition smooth.
    int32_t scanningOffset = maxOffset;
    if ( tilePixelOffset != fheroes2::Point() ) {
        ++scanningOffset;
    }

    std::vector<fheroes2::Point> positions;
    positions.reserve( 2 * 2 * scanningOffset * scanningOffset );

    for ( int32_t y = -scanningOffset; y <= scanningOffset; ++y ) {
        const int32_t posY = y + center.y;
        for ( int32_t x = -scanningOffset; x <= scanningOffset; ++x ) {
            if ( Maps::isValidAbsPoint( x + center.x, posY ) ) {
                positions.emplace_back( x, y );
            }
        }
    }

    // Sort positions by distance to the center.
    std::stable_sort( positions.begin(), positions.end(),
                      []( const fheroes2::Point & p1, const fheroes2::Point & p2 ) { return p1.x * p1.x + p1.y * p1.y < p2.x * p2.x + p2.y * p2.y; } );

    const double maxDistance = std::sqrt( ( maxOffset * maxOffset + maxOffset * maxOffset ) * fheroes2::tileWidthPx * fheroes2::tileWidthPx );

    const bool is3DAudioEnabled = Settings::Get().is3DAudioEnabled();

    for ( const fheroes2::Point & pos : positions ) {
        const M82::SoundType soundType = M82::getAdventureMapTileSound( world.getTile( pos.x + center.x, pos.y + center.y ) );
        if ( soundType == M82::UNKNOWN ) {
            continue;
        }

        fheroes2::Point actualPosition = pos;
        actualPosition.x *= fheroes2::tileWidthPx;
        actualPosition.y *= fheroes2::tileWidthPx;

        actualPosition -= tilePixelOffset;

        const double dblDistance = std::sqrt( actualPosition.x * actualPosition.x + actualPosition.y * actualPosition.y );
        if ( dblDistance >= maxDistance ) {
            continue;
        }

        const uint8_t distance = [maxDistance, dblDistance]() {
            const long dist = std::lround( dblDistance * 255 / maxDistance );
            assert( dist >= 0 && dist <= 255 );

            return static_cast<uint8_t>( dist );
        }();

        int16_t angle = 0;

        if ( is3DAudioEnabled ) {
            // This is a schema how the direction of sound looks like:
            // |      0     |
            // | 270     90 |
            // |     180    |
            // so the direction to an object on the top is 0 degrees, on the right side - 90, bottom - 180 and left side - 270 degrees.

            // We need to swap X and Y axes and invert Y axis as on screen Y axis goes from top to bottom.
            angle = static_cast<int16_t>( std::atan2( actualPosition.x, -actualPosition.y ) * 180 / M_PI );
            // It is extremely important to normalize the angle.
            if ( angle < 0 ) {
                angle = 360 + angle;
            }
        }

        std::vector<AudioManager::AudioLoopEffectInfo> & effects = soundEffects[soundType];

        // If there is already a source of the same sound in this direction, then choose the one that is closer.
        if ( std::find_if( effects.begin(), effects.end(),
                           [distance, angle]( AudioManager::AudioLoopEffectInfo & info ) {
                               if ( info.angle != angle ) {
                                   return false;
                               }

                               info.distance = std::min( distance, info.distance );

                               return true;
                           } )
             != effects.end() ) {
            continue;
        }

        // Otherwise, use the current one for now.
        effects.emplace_back( angle, distance );

        --availableChannels;
        if ( availableChannels == 0 ) {
            break;
        }
    }

    AudioManager::playLoopSoundsAsync( std::move( soundEffects ) );
}

void Game::restoreSoundsForCurrentFocus()
{
    AudioManager::ResetAudio();

    switch ( Interface::GetFocusType() ) {
    case GameFocus::HEROES: {
        const Heroes * focusedHero = Interface::GetFocusHeroes();
        assert( focusedHero != nullptr );

        const int heroIndexPos = focusedHero->GetIndex();
        if ( heroIndexPos >= 0 ) {
            Game::EnvironmentSoundMixer();
            AudioManager::PlayMusicAsync( MUS::FromGround( world.getTile( heroIndexPos ).GetGround() ), Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );
        }
        break;
    }

    case GameFocus::CASTLE: {
        const Castle * focusedCastle = Interface::GetFocusCastle();
        assert( focusedCastle != nullptr );

        Game::EnvironmentSoundMixer();
        AudioManager::PlayMusicAsync( MUS::FromGround( world.getTile( focusedCastle->GetIndex() ).GetGround() ), Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );
        break;
    }

    default:
        break;
    }
}

uint32_t Game::GetRating()
{
    uint32_t rating = 50;

    switch ( Settings::Get().getCurrentMapInfo().difficulty ) {
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

uint32_t Game::getGameOverScoreFactor()
{
    uint32_t mapSizeFactor = 0;

    switch ( Settings::Get().getCurrentMapInfo().width ) {
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

    return ( 200 - daysScore );
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

std::string Game::formatMonsterCount( const uint32_t count, const bool isDetailedView, const bool abbreviateNumber /* = false */ )
{
    if ( isDetailedView ) {
        return ( abbreviateNumber ? fheroes2::abbreviateNumber( count ) : std::to_string( count ) );
    }

    return Army::SizeString( count );
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

    AudioManager::PlaySound( wav );
}
