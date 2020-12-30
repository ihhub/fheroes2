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

#include "game.h"
#include "game_delays.h"
#include "gamedefs.h"
#include "settings.h"

TimeDelay::TimeDelay( uint32_t dl )
{
    second = dl;
}

uint32_t TimeDelay::operator()( void ) const
{
    return second;
}

TimeDelay & TimeDelay::operator=( uint32_t dl )
{
    second = dl;
    return *this;
}

void TimeDelay::Reset( void )
{
    first.Start();
}

bool TimeDelay::Trigger( uint32_t customDelay )
{
    first.Stop();
    const uint32_t expected = ( customDelay > 0 ) ? customDelay : second;
    if ( first.Get() < expected )
        return false;

    first.Start();
    return true;
}

namespace Game
{
    void AnimateDelaysInitialize( void );

    static const double battleSpeedAdjustment = 1.0 / static_cast<double>( 10 - DEFAULT_BATTLE_SPEED );

    TimeDelay delays[] = {20, // SCROLL_DELAY
                          250, // MAIN_MENU_DELAY
                          250, // MAPS_DELAY
                          200, // CASTLE_TAVERN_DELAY
                          200, // CASTLE_AROUND_DELAY
                          130, // CASTLE_BUYHERO_DELAY
                          130, // CASTLE_BUILD_DELAY
                          150, // CASTLE_UNIT_DELAY
                          32, // HEROES_FADE_DELAY
                          40, // HEROES_PICKUP_DELAY
                          50, // PUZZLE_FADE_DELAY
                          75, // BATTLE_DIALOG_DELAY
                          120, // BATTLE_FRAME_DELAY
                          40, // BATTLE_MISSILE_DELAY
                          90, // BATTLE_SPELL_DELAY
                          20, // BATTLE_DISRUPTING_DELAY
                          90, // BATTLE_CATAPULT_DELAY  // catapult
                          40, // BATTLE_CATAPULT2_DELAY // boulder
                          40, // BATTLE_CATAPULT3_DELAY // cloud
                          90, // BATTLE_BRIDGE_DELAY
                          150, // BATTLE_IDLE_DELAY
                          350, // BATTLE_OPPONENTS_DELAY
                          250, // BATTLE_FLAGS_DELAY
                          800, // BATTLE_POPUP_DELAY
                          220, // BATTLE_COLOR_CYCLE_DELAY
                          160, // BATTLE_SELECTED_UNIT_DELAY
                          10, // CURRENT_HERO_DELAY
                          10, // CURRENT_AI_DELAY
                          0, // CUSTOM_DELAY
                          0};

    int humanHeroMultiplier = 1;
    int aiHeroMultiplier = 1;

    void SetupHeroMovement( const int speed, TimeDelay & delay, int & multiplier )
    {
        switch ( speed ) {
        case 1:
            delay = 18;
            multiplier = 1;
            break;
        case 2:
            delay = 16;
            multiplier = 1;
            break;
        case 3:
            delay = 14;
            multiplier = 1;
            break;
        case 4:
            delay = 12;
            multiplier = 1;
            break;
        case 5:
            delay = 10;
            multiplier = 1;
            break;
        case 6:
            delay = 16;
            multiplier = 2;
            break;
        case 7:
            delay = 12;
            multiplier = 2;
            break;
        case 8:
            delay = 16;
            multiplier = 4;
            break;
        case 9:
            delay = 8;
            multiplier = 4;
            break;
        default:
            delay = 0;
            multiplier = 4;
        }
    }
}

void Game::AnimateDelaysInitialize( void )
{
    for ( size_t id = 0; id < LAST_DELAY; ++id ) {
        delays[id].Reset();
    }
    UpdateGameSpeed();
}

void Game::AnimateResetDelay( int dl )
{
    if ( dl < LAST_DELAY )
        delays[dl].Reset();
}

bool Game::AnimateCustomDelay( uint32_t delay )
{
    return delays[CUSTOM_DELAY].Trigger( delay );
}

bool Game::AnimateInfrequentDelay( int dl )
{
    return dl < LAST_DELAY && 0 < delays[dl]() ? delays[dl].Trigger() : true;
}

void Game::UpdateGameSpeed( void )
{
    const Settings & conf = Settings::Get();

    SetupHeroMovement( conf.HeroesMoveSpeed(), delays[CURRENT_HERO_DELAY], humanHeroMultiplier );
    SetupHeroMovement( conf.AIMoveSpeed(), delays[CURRENT_AI_DELAY], aiHeroMultiplier );

    const double adjustedBattleSpeed = ( 10 - conf.BattleSpeed() ) * battleSpeedAdjustment;
    delays[BATTLE_FRAME_DELAY] = static_cast<uint32_t>( 120 * adjustedBattleSpeed );
    delays[BATTLE_MISSILE_DELAY] = static_cast<uint32_t>( 40 * adjustedBattleSpeed );
    delays[BATTLE_SPELL_DELAY] = static_cast<uint32_t>( 75 * adjustedBattleSpeed );
    delays[BATTLE_IDLE_DELAY] = static_cast<uint32_t>( 150 * adjustedBattleSpeed );
    delays[BATTLE_DISRUPTING_DELAY] = static_cast<uint32_t>( 25 * adjustedBattleSpeed );
    delays[BATTLE_CATAPULT_DELAY] = static_cast<uint32_t>( 90 * adjustedBattleSpeed );
    delays[BATTLE_CATAPULT2_DELAY] = static_cast<uint32_t>( 40 * adjustedBattleSpeed );
    delays[BATTLE_CATAPULT3_DELAY] = static_cast<uint32_t>( 40 * adjustedBattleSpeed );
    delays[BATTLE_BRIDGE_DELAY] = static_cast<uint32_t>( 90 * adjustedBattleSpeed );
    delays[BATTLE_OPPONENTS_DELAY] = static_cast<uint32_t>( 350 * adjustedBattleSpeed );

    delays[BATTLE_FLAGS_DELAY] = static_cast<uint32_t>( ( adjustedBattleSpeed < 0.1 ) ? 25 : 250 * adjustedBattleSpeed );
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
