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

#include "kingdom.h"
#include "castle.h"
#include "heroes.h"
#include "dialog.h"
#include "battle.h"
#include "battle_arena.h"
#include "battle_command.h"
#include "battle_troop.h"
#include "ai.h"
#include "settings.h"
#include "agg.h"
#include "game_interface.h"

const char* AI::Type(void)
{
    return "empty";
}

const char* AI::License(void)
{
    return "GPL";
}

void AI::Init(void)
{
}

void AI::CastlePreBattle(Castle &)
{
}

void AI::CastleAfterBattle(Castle &, bool attacker_wins)
{
}

void AI::CastleTurn(Castle &)
{
}

void AI::CastleAdd(const Castle &)
{
}

void AI::CastleRemove(const Castle &)
{
}

void AI::HeroesAdd(const Heroes &)
{
}

void AI::HeroesRemove(const Heroes &)
{
}

void AI::HeroesPreBattle(HeroBase &)
{
}

void AI::HeroesAfterBattle(HeroBase &)
{
}

void AI::HeroesActionNewPosition(Heroes &)
{
}

void AI::HeroesClearTask(const Heroes &)
{
}

std::string AI::HeroesString(const Heroes &)
{
    return "";
}

void AI::HeroesActionComplete(Heroes &, s32)
{
}

void AI::HeroesLevelUp(Heroes &)
{
}

void AI::HeroesPostLoad(Heroes &)
{
}

bool AI::HeroesSkipFog(void)
{
    return false;
}

bool AI::HeroesGetTask(Heroes & hero)
{
    // stop hero
    hero.GetPath().Reset();
    return false;
}

bool AI::HeroesCanMove(const Heroes & hero)
{
    return hero.MayStillMove() && ! hero.Modes(HEROES_MOVED);
}

void AI::HeroesTurn(Heroes & hero)
{
   Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();

    hero.ResetModes(HEROES_MOVED);

    while(AI::HeroesCanMove(hero))
    {
	// turn indicator
        status.RedrawTurnProgress(3);
        status.RedrawTurnProgress(4);

        // get task for heroes
        AI::HeroesGetTask(hero);

        // turn indicator
        status.RedrawTurnProgress(5);
        status.RedrawTurnProgress(6);

        // heroes AI turn
        AI::HeroesMove(hero);
	hero.SetModes(HEROES_MOVED);

        // turn indicator
        status.RedrawTurnProgress(7);
        status.RedrawTurnProgress(8);
    }

    DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", end");
}

void AI::KingdomTurn(Kingdom & kingdom)
{
    KingdomHeroes & heroes = kingdom.GetHeroes();
    KingdomCastles & castles = kingdom.GetCastles();

    const int color = kingdom.GetColor();

    if(kingdom.isLoss() || color == Color::NONE)
    {
        kingdom.LossPostActions();
        return;
    }

    if(! Settings::Get().MusicMIDI()) AGG::PlayMusic(MUS::COMPUTER);

    Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();

    // indicator
    status.RedrawTurnProgress(0);

    status.RedrawTurnProgress(1);

    // castles AI turn
    for(KingdomCastles::iterator
	it = castles.begin(); it != castles.end(); ++it)
	if(*it) CastleTurn(**it);

    status.RedrawTurnProgress(3);

    // heroes turns
    for(KingdomHeroes::iterator
	it = heroes.begin(); it != heroes.end(); ++it)
	if(*it) HeroesTurn(**it);

    status.RedrawTurnProgress(6);
    status.RedrawTurnProgress(7);
    status.RedrawTurnProgress(8);
    status.RedrawTurnProgress(9);

    DEBUG(DBG_AI, DBG_INFO, Color::String(color) << " moved");
}

void AI::BattleTurn(Battle::Arena &, const Battle::Unit & b, Battle::Actions & a)
{
    // end action
    a.push_back(Battle::Command(Battle::MSG_BATTLE_END_TURN, b.GetUID()));
}

bool AI::BattleMagicTurn(Battle::Arena &, const Battle::Unit &, Battle::Actions &, const Battle::Unit*)
{
    return false;
}
