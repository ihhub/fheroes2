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

#include "gamedefs.h"
#include "settings.h"
#include "game.h"

struct TimeDelay : std::pair<SDL::Time, int>
{
    TimeDelay(int dl)
    {
	second = dl;
    }

    int operator() (void) const
    {
	return second;
    }

    TimeDelay & operator= (int dl)
    {
	second = dl;
	return *this;
    }

    void Reset(void)
    {
	first.Start();
    }

    bool Trigger(void)
    {
	first.Stop();
	if(first.Get() < static_cast<u32>(second)) return false;

	first.Start();
	return true;
    }
};

namespace Game
{
    void AnimateDelaysInitialize(void);

    TimeDelay delays[] = {
	20,	// SCROLL_DELAY
	400,	// MAIN_MENU_DELAY
	700,	// MAPS_DELAY
	200,	// CASTLE_TAVERN_DELAY
	400,	// CASTLE_AROUND_DELAY
	130,	// CASTLE_BUYHERO_DELAY
	130,	// CASTLE_BUILD_DELAY
	60,	// HEROES_MOVE_DELAY
	40,	// HEROES_FADE_DELAY
	40,	// HEROES_PICKUP_DELAY
	50,	// PUZZLE_FADE_DELAY
	300,	// BATTLE_DIALOG_DELAY
	80,	// BATTLE_FRAME_DELAY
	40,	// BATTLE_MISSILE_DELAY
	90,	// BATTLE_SPELL_DELAY
	20,	// BATTLE_DISRUPTING_DELAY
	90,	// BATTLE_CATAPULT_DELAY  // catapult
	40,	// BATTLE_CATAPULT2_DELAY // boulder
	40,	// BATTLE_CATAPULT3_DELAY // cloud
	90,	// BATTLE_BRIDGE_DELAY
	3000,	// BATTLE_IDLE_DELAY
	200,	// BATTLE_IDLE2_DELAY
	500,	// BATTLE_OPPONENTS_DELAY
	300,	// BATTLE_FLAGS_DELAY
	800,	// BATTLE_POPUP_DELAY
	300,	// AUTOHIDE_STATUS_DELAY
	0,	// CURRENT_HERO_DELAY
	0,	// CURRENT_AI_DELAY
	0
    };
}

void Game::AnimateDelaysInitialize(void)
{
    std::for_each(&delays[0], &delays[LAST_DELAY], std::mem_fun_ref(&TimeDelay::Reset));
    UpdateHeroesMoveSpeed();
    UpdateBattleSpeed();
}

void Game::AnimateResetDelay(int dl)
{
    if(dl < LAST_DELAY)
	delays[dl].Reset();
}

bool Game::AnimateInfrequentDelay(int dl)
{
    return dl < LAST_DELAY && 0 < delays[dl]() ?
	delays[dl].Trigger() : true;
}

void Game::UpdateHeroesMoveSpeed(void)
{
    const Settings & conf = Settings::Get();

    const TimeDelay & td_etalon = delays[HEROES_MOVE_DELAY];
    TimeDelay & td_hero = delays[CURRENT_HERO_DELAY];
    TimeDelay & td_ai   = delays[CURRENT_AI_DELAY];

    const int hr_value = conf.HeroesMoveSpeed() ?
	((conf.HeroesMoveSpeed() - DEFAULT_SPEED_DELAY) * td_etalon()) / DEFAULT_SPEED_DELAY :
	td_etalon();

    const int ai_value = conf.AIMoveSpeed() ?
	((conf.AIMoveSpeed() - DEFAULT_SPEED_DELAY) * td_etalon()) / DEFAULT_SPEED_DELAY :
	td_etalon();

    if(conf.HeroesMoveSpeed() == DEFAULT_SPEED_DELAY)
	td_hero = td_etalon();
    else
	td_hero = td_etalon() - hr_value;

    if(conf.AIMoveSpeed() == DEFAULT_SPEED_DELAY)
	td_ai   = td_etalon();
    else
	td_ai   = td_etalon() - ai_value;
}

void Game::UpdateBattleSpeed(void)
{
    const Settings & conf = Settings::Get();
    const int value = 5;

    delays[BATTLE_FRAME_DELAY] = 80 - (conf.BattleSpeed() - value) * 15;
    delays[BATTLE_MISSILE_DELAY] = 40 - (conf.BattleSpeed() - value) * 7;
    delays[BATTLE_SPELL_DELAY] = 90 - (conf.BattleSpeed() - value) * 17;
    delays[BATTLE_DISRUPTING_DELAY] = 20 - (conf.BattleSpeed() - value) * 3;
    delays[BATTLE_CATAPULT_DELAY] = 90 - (conf.BattleSpeed() - value) * 17;
    delays[BATTLE_CATAPULT2_DELAY] = 40 - (conf.BattleSpeed() - value) * 7;
    delays[BATTLE_CATAPULT3_DELAY] = 40 - (conf.BattleSpeed() - value) * 7;
    delays[BATTLE_BRIDGE_DELAY] = 90 - (conf.BattleSpeed() - value) * 17;

    if(0 < conf.BlitSpeed())
    {
	const float etalon = 15.0;
	const float div = conf.BlitSpeed() / etalon;
	const int ids[] = { BATTLE_FRAME_DELAY, BATTLE_MISSILE_DELAY, BATTLE_SPELL_DELAY, BATTLE_DISRUPTING_DELAY, BATTLE_CATAPULT_DELAY, BATTLE_CATAPULT2_DELAY, BATTLE_CATAPULT3_DELAY, BATTLE_BRIDGE_DELAY };

	DEBUG(DBG_GAME, DBG_INFO, "set battle speed: " << conf.BattleSpeed());
	std::ostringstream os;

	for(u32 it = 0; it < ARRAY_COUNT(ids); ++it)
	{
	    float tmp = delays[ids[it]]();
	    tmp /= div;
	    delays[ids[it]] = static_cast<int>(tmp);
	    os << static_cast<int>(tmp) << ", ";
	}

	DEBUG(DBG_GAME, DBG_INFO, "set battle delays: " << os.str());
    }
}
