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

#ifndef H2ARMY_H
#define H2ARMY_H

#include <utility>
#include <string>
#include <vector>
#include "bitmodes.h"
#include "players.h"
#include "army_troop.h"

class Castle;
class HeroBase;
class Heroes;

namespace Maps { class Tiles; }

class Troops : protected std::vector<Troop*>
{
public:
    Troops();
    virtual ~Troops();

    void		Assign(const Troop*, const Troop*);
    void		Assign(const Troops &);
    void		Insert(const Troops &);
    void		PushBack(const Monster &, u32);
    void		PopBack(void);

    size_t		Size(void) const;

    Troop*		GetTroop(size_t);
    const Troop*	GetTroop(size_t) const;

    void		UpgradeMonsters(const Monster &);
    u32			GetCountMonsters(const Monster &) const;

    u32			GetCount(void) const;
    bool		isValid(void) const;
    bool		HasMonster(const Monster &) const;

    bool		AllTroopsIsRace(int) const;
    u32			GetUniqueCount(void) const;

    bool		JoinTroop(const Troop &);
    bool		JoinTroop(const Monster &, u32);
    bool		CanJoinTroop(const Monster &) const;

    void 		JoinTroops(Troops &);
    bool 		CanJoinTroops(const Troops &) const;

    Troops		GetOptimized(void) const;

    virtual u32		GetAttack(void) const;
    virtual u32		GetDefense(void) const;

    u32			GetHitPoints(void) const;
    u32			GetDamageMin(void) const;
    u32			GetDamageMax(void) const;
    u32			GetStrength(void) const;

    void		Clean(void);
    void		UpgradeTroops(const Castle &);

    Troop*		GetFirstValid(void);
    Troop*		GetWeakestTroop(void);
    Troop*		GetSlowestTroop(void);

    void		ArrangeForBattle(bool = false);

    void		JoinStrongest(Troops &, bool);
    void		KeepOnlyWeakest(Troops &, bool);

    void		DrawMons32LineWithScoute(s32, s32, u32, u32, u32, u32, bool) const;
    void		SplitTroopIntoFreeSlots(const Troop &, u32 slots);
};

enum { JOIN_NONE, JOIN_FREE, JOIN_COST, JOIN_FLEE };

struct JoinCount : std::pair<int, u32>
{
    JoinCount() : std::pair<int, u32>(JOIN_NONE, 0) {}
    JoinCount(int reason, u32 count) : std::pair<int, u32>(reason, count) {}
};

class Army : public Troops, public Control
{
public:
    static std::string	SizeString(u32);
    static std::string	TroopSizeString(const Troop &);

    // compare
    static bool		WeakestTroop(const Troop*, const Troop*);
    static bool		StrongestTroop(const Troop*, const Troop*);
    static bool		SlowestTroop(const Troop*, const Troop*);
    static bool		FastestTroop(const Troop*, const Troop*);
    static void		SwapTroops(Troop &, Troop &);
    static u32		UniqueCount(const Army &);

    // 0: fight, 1: free join, 2: join with gold, 3: flee
    static JoinCount	GetJoinSolution(const Heroes &, const Maps::Tiles &, const Troop &);
    static bool		TroopsStrongerEnemyTroops(const Troops &, const Troops &);

    static void		DrawMons32Line(const Troops &, s32, s32, u32, u32 = 0, u32 = 0);
    static void		DrawMons32LineWithScoute(const Troops &, s32, s32, u32, u32, u32, u32);
    static void		DrawMons32LineShort(const Troops &, s32, s32, u32, u32, u32);

    Army(HeroBase* s = NULL);
    Army(const Maps::Tiles &);
    ~Army();

    void		Reset(bool = false);	// reset: soft or hard

    int			GetRace(void) const;
    int			GetColor(void) const;
    int			GetControl(void) const;
    u32			GetAttack(void) const;
    u32			GetDefense(void) const;

    void        	SetColor(int);

    int			GetMorale(void) const;
    int			GetLuck(void) const;
    int			GetMoraleModificator(std::string*) const;
    int			GetLuckModificator(std::string*) const;
    u32			ActionToSirens(void);

    const HeroBase*	GetCommander(void) const;
    HeroBase*		GetCommander(void);
    void		SetCommander(HeroBase*);

    const Castle*	inCastle(void) const;

    std::string		String(void) const;

    void		JoinStrongestFromArmy(Army &);
    void		KeepOnlyWeakestTroops(Army &);

    void		SetSpreadFormat(bool);
    bool		isSpreadFormat(void) const;

    bool		isFullHouse(void) const;
    bool		SaveLastTroop(void) const;

protected:
    friend StreamBase & operator<< (StreamBase &, const Army &);
    friend StreamBase & operator>> (StreamBase &, Army &);
#ifdef WITH_XML
    friend TiXmlElement & operator>> (TiXmlElement &, Army &);
#endif

    HeroBase*		commander;
    bool		combat_format;
    int			color;

private:
    Army &		operator= (const Army &) { return *this; }
};

StreamBase & operator<< (StreamBase &, const Army &);
StreamBase & operator>> (StreamBase &, Army &);

#endif
