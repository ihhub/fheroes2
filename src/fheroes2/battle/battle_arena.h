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

#ifndef H2BATTLE_ARENA_H
#define H2BATTLE_ARENA_H

#include <list>

#include "gamedefs.h"
#include "ai.h"
#include "spell_storage.h"
#include "battle_board.h"
#include "battle_grave.h"

#define ARENAW 11
#define ARENAH 9
#define ARENASIZE ARENAW * ARENAH

class Castle;
class HeroBase;

namespace Battle
{
    class Force;
    class Units;
    class Unit;
    class Command;

    class Actions : public std::list<Command>
    {
    public:
	bool		HaveCommand(u32) const;
    };

    class Arena
    {
    public:
	Arena(Army &, Army &, s32, bool);
	~Arena();

	void		Turns();
	bool		NetworkTurn();
	bool		BattleValid() const;

	bool		CanBreakAutoBattle() const;
	void		BreakAutoBattle();

	u32		GetCurrentTurn() const;
	Result &	GetResult();

	const HeroBase*	GetCommander(int color, bool invert = false) const;
	const HeroBase*	GetCommander1() const;
	const HeroBase*	GetCommander2() const;
	const HeroBase* GetCurrentCommander() const;

	Force &		GetForce1();
	Force &		GetForce2();
	Force &		GetForce(int color, bool invert = false);
	Force &		GetCurrentForce();

	int		GetArmyColor1() const;
	int		GetArmyColor2() const;
	int		GetCurrentColor() const;
	int		GetOppositeColor(int) const;

	Unit*		GetTroopBoard(s32);
	const Unit*	GetTroopBoard(s32) const;

	Unit*		GetTroopUID(u32);
	const Unit*	GetTroopUID(u32) const;

	const Unit*	GetEnemyMaxQuality(int) const;

	const SpellStorage &
			GetUsageSpells() const;

	void		DialogBattleSummary(const Result &) const;
	int		DialogBattleHero(const HeroBase &, bool) const;

	void		FadeArena() const;

	Indexes		GetPath(const Unit &, const Position &);
	void		ApplyAction(Command &);

	TargetsInfo	GetTargetsForDamage(Unit &, Unit &, s32);
	void		TargetsApplyDamage(Unit &, Unit &, TargetsInfo &);
	TargetsInfo	GetTargetsForSpells(const HeroBase*, const Spell &, s32);
	void		TargetsApplySpell(const HeroBase*, const Spell &, TargetsInfo &);

	bool		isDisableCastSpell(const Spell &, std::string *msg);

	bool		GraveyardAllowResurrect(s32, const Spell &) const;
	const Unit*	GraveyardLastTroop(s32) const;
	Indexes		GraveyardClosedCells() const;

	bool		CanSurrenderOpponent(int color) const;
	bool		CanRetreatOpponent(int color) const;

	void		ApplyActionSpellSummonElemental(Command &, const Spell &);
	void		ApplyActionSpellMirrorImage(Command &);
	void		ApplyActionSpellTeleport(Command &);
	void		ApplyActionSpellEarthQuake(Command &);
	void		ApplyActionSpellDefaults(Command &, const Spell &);

	u32		GetObstaclesPenalty(const Unit &, const Unit &) const;
	int		GetICNCovr() const;

        u32		GetCastleTargetValue(int) const;

	static Board*		GetBoard();
	static Tower*		GetTower(int);
	static Bridge*		GetBridge();
	static const Castle*	GetCastle();
	static Interface*	GetInterface();
	static Graveyard*	GetGraveyard();

    private:
	friend StreamBase & operator<< (StreamBase &, const Arena &);
	friend StreamBase & operator>> (StreamBase &, Arena &);

    	void		RemoteTurn(const Unit &, Actions &);
	void		HumanTurn(const Unit &, Actions &);

	void		TurnTroop(Unit*);
	void		TowerAction(const Tower &);

        void		SetCastleTargetValue(int, u32);
	void		CatapultAction();

	s32		GetFreePositionNearHero(int) const;
	std::vector<int>
			GetCastleTargets() const;

	void		ApplyActionRetreat(Command &);
	void		ApplyActionSurrender(Command &);
	void		ApplyActionAttack(Command &);
	void		ApplyActionMove(Command &);
	void		ApplyActionEnd(Command &);
	void		ApplyActionSkip(Command &);
	void		ApplyActionMorale(Command &);
	void		ApplyActionLuck(Command &);
	void		ApplyActionSpellCast(Command &);
	void		ApplyActionTower(Command &);
	void		ApplyActionCatapult(Command &);
	void		ApplyActionAutoBattle(Command &);

	void		BattleProcess(Unit &, Unit & b2, s32 = -1, int = 0);

	Unit*		CreateElemental(const Spell &);
	Unit*		CreateMirrorImage(Unit &, s32);

	Force*		army1;
        Force*		army2;
	Units*		armies_order;

	const Castle*	castle;
	int		current_color;

	Tower*		towers[3];
	Catapult*	catapult;
	Bridge*		bridge;

	Interface*	interface;
	Result		result_game;

	Graveyard	graveyard;
	SpellStorage	usage_spells;

	Board		board;
	int		icn_covr;

	u32		current_turn;
	int		auto_battle;

	bool		end_turn;
    };

    Arena*	GetArena();
    StreamBase &	operator<< (StreamBase &, const Arena &);
    StreamBase &	operator>> (StreamBase &, Arena &);
}

#endif
