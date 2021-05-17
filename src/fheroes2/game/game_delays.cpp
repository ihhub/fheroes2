/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "game_delays.h"
#include "settings.h"
#include "timing.h"

namespace
{
    std::vector<fheroes2::TimeDelay> delays( Game::LAST_DELAY + 1, fheroes2::TimeDelay( 0 ) );

    const double battleSpeedAdjustment = 1.0 / static_cast<double>( 10 - DEFAULT_BATTLE_SPEED );

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
            delay.setDelay( 0 );
            multiplier = 4;
        }
    }
}

namespace Game
{
    void AnimateDelaysInitialize();
}

void Game::AnimateDelaysInitialize()
{
    delays[SCROLL_DELAY].setDelay( 20 );
    delays[SCROLL_START_DELAY].setDelay( 20 );
    delays[MAIN_MENU_DELAY].setDelay( 250 );
    delays[MAPS_DELAY].setDelay( 250 );
    delays[CASTLE_TAVERN_DELAY].setDelay( 200 );
    delays[CASTLE_AROUND_DELAY].setDelay( 200 );
    delays[CASTLE_BUYHERO_DELAY].setDelay( 130 );
    delays[CASTLE_BUILD_DELAY].setDelay( 130 );
    delays[CASTLE_UNIT_DELAY].setDelay( 150 );
    delays[HEROES_FADE_DELAY].setDelay( 32 );
    delays[HEROES_PICKUP_DELAY].setDelay( 40 );
    delays[PUZZLE_FADE_DELAY].setDelay( 50 );
    delays[BATTLE_DIALOG_DELAY].setDelay( 75 );
    delays[BATTLE_FRAME_DELAY].setDelay( 120 );
    delays[BATTLE_MISSILE_DELAY].setDelay( 40 );
    delays[BATTLE_SPELL_DELAY].setDelay( 90 );
    delays[BATTLE_DISRUPTING_DELAY].setDelay( 20 );
    delays[BATTLE_CATAPULT_DELAY].setDelay( 90 );
    delays[BATTLE_CATAPULT_BOULDER_DELAY].setDelay( 40 );
    delays[BATTLE_CATAPULT_CLOUD_DELAY].setDelay( 40 );
    delays[BATTLE_BRIDGE_DELAY].setDelay( 90 );
    delays[BATTLE_IDLE_DELAY].setDelay( 150 );
    delays[BATTLE_OPPONENTS_DELAY].setDelay( 350 );
    delays[BATTLE_FLAGS_DELAY].setDelay( 250 );
    delays[BATTLE_POPUP_DELAY].setDelay( 800 );
    delays[BATTLE_COLOR_CYCLE_DELAY].setDelay( 220 );
    delays[BATTLE_SELECTED_UNIT_DELAY].setDelay( 160 );
    delays[CURRENT_HERO_DELAY].setDelay( 10 );
    delays[CURRENT_AI_DELAY].setDelay( 10 );

    for ( fheroes2::TimeDelay & delay : delays ) {
        delay.reset();
    }

    UpdateGameSpeed();
}

void Game::AnimateResetDelay( const DelayType delayType )
{
    delays[delayType].reset();
}

bool Game::validateCustomAnimationDelay( const uint64_t delayMs )
{
    if ( delays[Game::DelayType::CUSTOM_DELAY].isPassed( delayMs ) ) {
        delays[Game::DelayType::CUSTOM_DELAY].reset();
        return true;
    }

    return false;
}

bool Game::validateAnimationDelay( const DelayType delayType )
{
    assert( delayType != Game::DelayType::CUSTOM_DELAY );

    if ( delays[delayType].isPassed() ) {
        delays[delayType].reset();
        return true;
    }

    return false;
}

void Game::passAnimationDelay( const DelayType delayType )
{
    delays[delayType].pass();
}

void Game::UpdateGameSpeed()
{
    const Settings & conf = Settings::Get();

    SetupHeroMovement( conf.HeroesMoveSpeed(), delays[CURRENT_HERO_DELAY], humanHeroMultiplier );
    SetupHeroMovement( conf.AIMoveSpeed(), delays[CURRENT_AI_DELAY], aiHeroMultiplier );

    const double adjustedBattleSpeed = ( 10 - conf.BattleSpeed() ) * battleSpeedAdjustment;
    delays[BATTLE_FRAME_DELAY].setDelay( static_cast<uint64_t>( 120 * adjustedBattleSpeed ) );
    delays[BATTLE_MISSILE_DELAY].setDelay( static_cast<uint64_t>( 40 * adjustedBattleSpeed ) );
    delays[BATTLE_SPELL_DELAY].setDelay( static_cast<uint64_t>( 75 * adjustedBattleSpeed ) );
    delays[BATTLE_IDLE_DELAY].setDelay( static_cast<uint64_t>( 150 * adjustedBattleSpeed ) );
    delays[BATTLE_DISRUPTING_DELAY].setDelay( static_cast<uint64_t>( 25 * adjustedBattleSpeed ) );
    delays[BATTLE_CATAPULT_DELAY].setDelay( static_cast<uint64_t>( 90 * adjustedBattleSpeed ) );
    delays[BATTLE_CATAPULT_BOULDER_DELAY].setDelay( static_cast<uint64_t>( 40 * adjustedBattleSpeed ) );
    delays[BATTLE_CATAPULT_CLOUD_DELAY].setDelay( static_cast<uint64_t>( 40 * adjustedBattleSpeed ) );
    delays[BATTLE_BRIDGE_DELAY].setDelay( static_cast<uint64_t>( 90 * adjustedBattleSpeed ) );
    delays[BATTLE_OPPONENTS_DELAY].setDelay( static_cast<uint64_t>( 350 * adjustedBattleSpeed ) );

    delays[BATTLE_FLAGS_DELAY].setDelay( static_cast<uint64_t>( ( adjustedBattleSpeed < 0.1 ) ? 25 : 250 * adjustedBattleSpeed ) );
}

int Game::HumanHeroAnimSkip()
{
    return humanHeroMultiplier;
}

int Game::AIHeroAnimSkip()
{
    return aiHeroMultiplier;
}

uint32_t Game::ApplyBattleSpeed( uint32_t delay )
{
    return static_cast<uint32_t>( battleSpeedAdjustment * ( 10 - Settings::Get().BattleSpeed() ) * delay );
}

bool Game::isDelayNeeded( const std::vector<Game::DelayType> & delayTypes )
{
    if ( delayTypes.empty() )
        return true;

    for ( const Game::DelayType type : delayTypes ) {
        assert( type != Game::DelayType::CUSTOM_DELAY );

        if ( delays[type].isPassed() ) {
            return false;
        }
    }

    return true;
}

bool Game::isCustomDelayNeeded( const uint64_t delayMs )
{
    return !delays[Game::DelayType::CUSTOM_DELAY].isPassed( delayMs );
}
