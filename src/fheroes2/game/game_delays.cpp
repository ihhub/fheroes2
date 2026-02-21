/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2026                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "game_delays.h"

#include <cassert>
#include <cstddef>

#include "settings.h"
#include "timing.h"

namespace
{
    std::vector<fheroes2::TimeDelay> delays( static_cast<size_t>( Game::DelayType::LAST_DELAY ), fheroes2::TimeDelay( 0 ) );

    static_assert( ( defaultBattleSpeed >= 0 ) && ( defaultBattleSpeed < 10 ) );

    constexpr double battleSpeedAdjustment = 1.0 / static_cast<double>( 10 - defaultBattleSpeed );

    int humanHeroMultiplier = 1;
    int aiHeroMultiplier = 1;

    void SetupHeroMovement( const int speed, fheroes2::TimeDelay & delay, int & multiplier )
    {
        switch ( speed ) {
        case 1:
            delay.setDelay( 18 );
            multiplier = 1;
            break;
        case 2:
            delay.setDelay( 16 );
            multiplier = 1;
            break;
        case 3:
            delay.setDelay( 14 );
            multiplier = 1;
            break;
        case 4:
            delay.setDelay( 12 );
            multiplier = 1;
            break;
        case 5:
            delay.setDelay( 10 );
            multiplier = 1;
            break;
        case 6:
            delay.setDelay( 16 );
            multiplier = 2;
            break;
        case 7:
            delay.setDelay( 12 );
            multiplier = 2;
            break;
        case 8:
            delay.setDelay( 16 );
            multiplier = 4;
            break;
        case 9:
            delay.setDelay( 8 );
            multiplier = 4;
            break;
        default:
            delay.setDelay( 1 );
            multiplier = 4;
        }
    }

    inline void setDelay( Game::DelayType type, const uint64_t delayMs )
    {
        delays[static_cast<size_t>( type )].setDelay( delayMs );
    }
}

namespace Game
{
    void AnimateDelaysInitialize();
}

void Game::AnimateDelaysInitialize()
{
    setDelay( DelayType::SCROLL_DELAY, 20 );
    setDelay( DelayType::SCROLL_START_DELAY, 20 );
    setDelay( DelayType::CURSOR_BLINK_DELAY, 440 );
    setDelay( DelayType::MAIN_MENU_DELAY, 250 );
    setDelay( DelayType::MAPS_DELAY, 250 );
    setDelay( DelayType::CASTLE_TAVERN_DELAY, 75 );
    setDelay( DelayType::CASTLE_AROUND_DELAY, 200 );
    setDelay( DelayType::CASTLE_BUYHERO_DELAY, 130 );
    setDelay( DelayType::CASTLE_BUILD_DELAY, 130 );
    setDelay( DelayType::CASTLE_UNIT_DELAY, 150 );
    setDelay( DelayType::HEROES_FADE_DELAY, 32 );
    setDelay( DelayType::HEROES_PICKUP_DELAY, 40 );
    setDelay( DelayType::PUZZLE_FADE_DELAY, 50 );
    setDelay( DelayType::BATTLE_DIALOG_DELAY, 75 );
    setDelay( DelayType::BATTLE_FRAME_DELAY, 120 );
    setDelay( DelayType::BATTLE_MISSILE_DELAY, 40 );
    setDelay( DelayType::BATTLE_SPELL_DELAY, 75 );
    setDelay( DelayType::BATTLE_DISRUPTING_DELAY, 25 );
    setDelay( DelayType::BATTLE_CATAPULT_DELAY, 90 );
    setDelay( DelayType::BATTLE_CATAPULT_BOULDER_DELAY, 40 );
    setDelay( DelayType::BATTLE_CATAPULT_CLOUD_DELAY, 40 );
    setDelay( DelayType::BATTLE_BRIDGE_DELAY, 90 );
    setDelay( DelayType::BATTLE_IDLE_DELAY, 150 );
    setDelay( DelayType::BATTLE_OPPONENTS_DELAY, 75 );
    setDelay( DelayType::BATTLE_FLAGS_DELAY, 250 );
    setDelay( DelayType::BATTLE_POPUP_DELAY, 800 );
    setDelay( DelayType::BATTLE_COLOR_CYCLE_DELAY, 220 );
    setDelay( DelayType::BATTLE_SELECTED_UNIT_DELAY, 160 );
    setDelay( DelayType::CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY, 10 );
    setDelay( DelayType::BATTLEFIELD_BACKGROUND_ANIMATION_DELAY, 125 );
    setDelay( DelayType::CURRENT_HERO_DELAY, 10 );
    setDelay( DelayType::CURRENT_AI_DELAY, 10 );

    UpdateGameSpeed();

    for ( fheroes2::TimeDelay & delay : delays ) {
        delay.reset();
    }
}

void Game::AnimateResetDelay( const DelayType delayType )
{
    delays[static_cast<size_t>( delayType )].reset();
}

bool Game::validateCustomAnimationDelay( const uint64_t delayMs )
{
    if ( delays[static_cast<size_t>( DelayType::CUSTOM_DELAY )].isPassed( delayMs ) ) {
        delays[static_cast<size_t>( DelayType::CUSTOM_DELAY )].reset();
        return true;
    }

    return false;
}

bool Game::validateAnimationDelay( const DelayType delayType )
{
    assert( delayType != DelayType::CUSTOM_DELAY );

    if ( delays[static_cast<size_t>( delayType )].isPassed() ) {
        delays[static_cast<size_t>( delayType )].reset();
        return true;
    }

    return false;
}

void Game::passAnimationDelay( const DelayType delayType )
{
    assert( delayType != DelayType::CUSTOM_DELAY );

    delays[static_cast<size_t>( delayType )].pass();
}

void Game::passCustomAnimationDelay( const uint64_t delayMs )
{
    setDelay( DelayType::CUSTOM_DELAY, delayMs );
    delays[static_cast<size_t>( DelayType::CUSTOM_DELAY )].pass();
}

void Game::setCustomUnitMovementDelay( const uint64_t delayMs )
{
    setDelay( DelayType::CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY, delayMs > 0 ? delayMs : 1 );
}

void Game::UpdateGameSpeed()
{
    const Settings & conf = Settings::Get();

    SetupHeroMovement( conf.HeroesMoveSpeed(), delays[static_cast<size_t>( DelayType::CURRENT_HERO_DELAY )], humanHeroMultiplier );
    SetupHeroMovement( conf.AIMoveSpeed(), delays[static_cast<size_t>( DelayType::CURRENT_AI_DELAY )], aiHeroMultiplier );

    const int32_t battleSpeed = conf.BattleSpeed();
    // For the battle speed = 10 avoid the zero delay and set animation speed to the 1/3 of battleSpeedAdjustment step.
    const double adjustedBattleSpeed = ( battleSpeed < 10 ) ? ( ( 10 - battleSpeed ) * battleSpeedAdjustment ) : ( battleSpeedAdjustment / 3 );
    // Reduce the Idle animation adjustment interval to: 1.2 for speed 1 ... 0.8 for speed 10.
    const double adjustedIdleAnimationSpeed = ( 28 - battleSpeed ) / 22.5;

    setDelay( DelayType::BATTLE_FRAME_DELAY, static_cast<uint64_t>( 120 * adjustedBattleSpeed ) );
    setDelay( DelayType::BATTLE_MISSILE_DELAY, static_cast<uint64_t>( 40 * adjustedBattleSpeed ) );
    setDelay( DelayType::BATTLE_SPELL_DELAY, static_cast<uint64_t>( 75 * adjustedBattleSpeed ) );
    setDelay( DelayType::BATTLE_DISRUPTING_DELAY, static_cast<uint64_t>( 25 * adjustedBattleSpeed ) );
    setDelay( DelayType::BATTLE_CATAPULT_DELAY, static_cast<uint64_t>( 90 * adjustedBattleSpeed ) );
    setDelay( DelayType::BATTLE_CATAPULT_BOULDER_DELAY, static_cast<uint64_t>( 40 * adjustedBattleSpeed ) );
    setDelay( DelayType::BATTLE_CATAPULT_CLOUD_DELAY, static_cast<uint64_t>( 40 * adjustedBattleSpeed ) );
    setDelay( DelayType::BATTLE_BRIDGE_DELAY, static_cast<uint64_t>( 90 * adjustedBattleSpeed ) );
    setDelay( DelayType::BATTLE_IDLE_DELAY, static_cast<uint64_t>( 150 * adjustedIdleAnimationSpeed ) );
    setDelay( DelayType::BATTLE_OPPONENTS_DELAY, static_cast<uint64_t>( 75 * adjustedIdleAnimationSpeed ) );
    setDelay( DelayType::BATTLE_FLAGS_DELAY, static_cast<uint64_t>( 250 * adjustedIdleAnimationSpeed ) );
}

int Game::HumanHeroAnimSpeedMultiplier()
{
    assert( humanHeroMultiplier > 0 );

    return humanHeroMultiplier;
}

int Game::AIHeroAnimSpeedMultiplier()
{
    assert( aiHeroMultiplier > 0 );

    return aiHeroMultiplier;
}

uint32_t Game::ApplyBattleSpeed( const uint32_t delay )
{
    const uint32_t battleSpeed = static_cast<uint32_t>( battleSpeedAdjustment * ( 10 - Settings::Get().BattleSpeed() ) * delay );
    return battleSpeed == 0 ? 1 : battleSpeed;
}

bool Game::hasEveryDelayPassed( const std::vector<DelayType> & delayTypes )
{
    for ( const DelayType type : delayTypes ) {
        if ( !delays[static_cast<size_t>( type )].isPassed() ) {
            return false;
        }
    }

    return true;
}

bool Game::isDelayNeeded( const std::vector<DelayType> & delayTypes )
{
    for ( const DelayType type : delayTypes ) {
        assert( type != DelayType::CUSTOM_DELAY );

        if ( delays[static_cast<size_t>( type )].isPassed() ) {
            return false;
        }
    }

    return true;
}

bool Game::isCustomDelayNeeded( const uint64_t delayMs )
{
    return !delays[static_cast<size_t>( DelayType::CUSTOM_DELAY )].isPassed( delayMs );
}

uint64_t Game::getAnimationDelayValue( const DelayType delayType )
{
    assert( delayType != DelayType::CUSTOM_DELAY );
    return delays[static_cast<size_t>( delayType )].getDelay();
}
