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

#ifdef BUILD_RELEASE
#define NDEBUG
#endif
#include <assert.h>

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

    TimeDelay delays[] = {20, // SCROLL_DELAY
                          250, // MAIN_MENU_DELAY
                          250, // MAPS_DELAY
                          200, // CASTLE_TAVERN_DELAY
                          150, // CASTLE_AROUND_DELAY
                          130, // CASTLE_BUYHERO_DELAY
                          130, // CASTLE_BUILD_DELAY
                          40, // HEROES_FADE_DELAY
                          40, // HEROES_PICKUP_DELAY
                          50, // PUZZLE_FADE_DELAY
                          100, // BATTLE_DIALOG_DELAY
                          120, // BATTLE_FRAME_DELAY
                          40, // BATTLE_MISSILE_DELAY
                          90, // BATTLE_SPELL_DELAY
                          20, // BATTLE_DISRUPTING_DELAY
                          90, // BATTLE_CATAPULT_DELAY  // catapult
                          40, // BATTLE_CATAPULT2_DELAY // boulder
                          40, // BATTLE_CATAPULT3_DELAY // cloud
                          90, // BATTLE_BRIDGE_DELAY
                          150, // BATTLE_IDLE_DELAY
                          500, // BATTLE_OPPONENTS_DELAY
                          300, // BATTLE_FLAGS_DELAY
                          800, // BATTLE_POPUP_DELAY
                          300, // AUTOHIDE_STATUS_DELAY
                          40, // CURRENT_HERO_DELAY
                          40, // CURRENT_AI_DELAY
                          0, // CUSTOM_DELAY
                          0};
}

void Game::AnimateDelaysInitialize( void )
{
    std::for_each( &delays[0], &delays[LAST_DELAY], std::mem_fun_ref( &TimeDelay::Reset ) );
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

    const int heroSpeed = conf.HeroesMoveSpeed() - DEFAULT_SPEED_DELAY;
    const int aiSpeed = conf.AIMoveSpeed() - DEFAULT_SPEED_DELAY;
    const int battleSpeed = conf.BattleSpeed() - DEFAULT_SPEED_DELAY;

    // assert to make sure we won't overflow
    assert( heroSpeed <= DEFAULT_SPEED_DELAY );
    assert( aiSpeed <= DEFAULT_SPEED_DELAY );
    assert( battleSpeed <= DEFAULT_SPEED_DELAY );

    delays[CURRENT_HERO_DELAY] = 40 - heroSpeed * 8;
    delays[CURRENT_AI_DELAY] = 40 - aiSpeed * 8;

    delays[BATTLE_FRAME_DELAY] = 120 - battleSpeed * 20;
    delays[BATTLE_MISSILE_DELAY] = 40 - battleSpeed * 7;
    delays[BATTLE_SPELL_DELAY] = 90 - battleSpeed * 17;
    delays[BATTLE_IDLE_DELAY] = 150 - battleSpeed * 20;
    delays[BATTLE_DISRUPTING_DELAY] = 20 - battleSpeed * 3;
    delays[BATTLE_CATAPULT_DELAY] = 90 - battleSpeed * 17;
    delays[BATTLE_CATAPULT2_DELAY] = 40 - battleSpeed * 7;
    delays[BATTLE_CATAPULT3_DELAY] = 40 - battleSpeed * 7;
    delays[BATTLE_BRIDGE_DELAY] = 90 - battleSpeed * 17;
}

uint32_t Game::ApplyBattleSpeed( uint32_t delay )
{
    return static_cast<uint32_t>( 10 - Settings::Get().BattleSpeed() ) * ( delay / DEFAULT_SPEED_DELAY );
}
