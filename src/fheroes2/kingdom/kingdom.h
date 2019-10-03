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
#ifndef H2KINGDOM_H
#define H2KINGDOM_H

#include <vector>
#include <map>
#include "payment.h"
#include "puzzle.h"
#include "mp2.h"
#include "pairs.h"
#include "heroes.h"
#include "castle.h"
#include "heroes_recruits.h"

class Player;
class Castle;
class Heroes;
struct AllHeroes;
struct VecHeroes;
struct AllCastles;
struct VecCastles;
struct CapturedObjects;

struct LastLoseHero : std::pair<int, u32> /* Heroes, date */
{
    LastLoseHero() : std::pair<int, u32>(Heroes::UNKNOWN, 0) {}
};

StreamBase & operator>> (StreamBase &, LastLoseHero &);

struct KingdomCastles : public VecCastles
{
};

struct KingdomHeroes : public VecHeroes
{
};

class Kingdom : public BitModes, public Control
{
public:
    enum
    {
	//UNDEF	     = 0x0001,
	IDENTIFYHERO = 0x0002,
	DISABLEHIRES = 0x0004,
	OVERVIEWCSTL = 0x0008
    };

    Kingdom();
    virtual ~Kingdom() {}

    void	Init(int color);
    void	clear();

    void	OverviewDialog();

    void	UpdateStartingResource();
    bool	isPlay() const;
    bool	isLoss() const;
    bool	AllowPayment(const Funds &) const;
    bool	AllowRecruitHero(bool check_payment, int level) const;

    void	SetLastLostHero(Heroes &);
    void	ResetLastLostHero();
    void	AddHeroStartCondLoss(Heroes*);
    std::string	GetNamesHeroStartCondLoss() const;

    Heroes*	GetLastLostHero() const;

    const Heroes*
		GetFirstHeroStartCondLoss() const;
    const Heroes*
		GetBestHero() const;

    int		GetControl() const;
    int		GetColor() const;
    int		GetRace() const;

    const Funds &
		GetFunds() const{ return resource; }
    Funds	GetIncome(int = INCOME_ALL) const;

    u32		GetArmiesStrength() const;

    void	AddFundsResource(const Funds &);
    void	OddFundsResource(const Funds &);

    u32		GetCountCastle() const;
    u32		GetCountTown() const;
    u32		GetCountMarketplace() const;
    u32		GetCountCapital() const;
    u32		GetLostTownDays() const;
    u32		GetCountNecromancyShrineBuild() const;
    u32		GetCountBuilding(u32) const;

    Recruits &	GetRecruits();

    const KingdomHeroes &
		GetHeroes() const{ return heroes; }
    const KingdomCastles &
		GetCastles() const{ return castles; }

    KingdomHeroes &
		GetHeroes() { return heroes; }
    KingdomCastles &
		GetCastles() { return castles; }

    void	AddHeroes(Heroes*);
    void	RemoveHeroes(const Heroes*);
    void	ApplyPlayWithStartingHero();
    void	HeroesActionNewPosition();

    void	AddCastle(const Castle*);
    void	RemoveCastle(const Castle*);
    
    void	ActionBeforeTurn();
    void	ActionNewDay();
    void	ActionNewWeek();
    void	ActionNewMonth();

    void	SetVisited(s32 index, int object = MP2::OBJ_ZERO);
    u32		CountVisitedObjects(int object) const;
    bool	isVisited(int object) const;
    bool	isVisited(const Maps::Tiles &) const;
    bool	isVisited(s32, int obj) const;

    bool	HeroesMayStillMove() const;

    const Puzzle &
		PuzzleMaps() const;
    Puzzle &	PuzzleMaps();

    void	SetVisitTravelersTent(int);
    bool	IsVisitTravelersTent(int) const;

    void	UpdateRecruits();
    void	LossPostActions();

    static u32	GetMaxHeroes();

private:
    friend StreamBase & operator<< (StreamBase &, const Kingdom &);
    friend StreamBase & operator>> (StreamBase &, Kingdom &);

    int			color;
    Funds		resource;

    u32			lost_town_days;

    KingdomCastles	castles;
    KingdomHeroes	heroes;

    Recruits		recruits;
    LastLoseHero	lost_hero;

    std::list<IndexObject>
			visit_object;

    Puzzle		puzzle_maps;
    u32			visited_tents_colors;

    KingdomHeroes	heroes_cond_loss;
};

class Kingdoms
{
public:
    Kingdoms();

    void	Init();
    void	clear();

    void	ApplyPlayWithStartingHero();

    void	NewDay();
    void	NewWeek();
    void	NewMonth();

    Kingdom &	GetKingdom(int color);
    const Kingdom &
		GetKingdom(int color) const;

    int		GetLossColors() const;
    int		GetNotLossColors() const;
    int		FindWins(int) const;

    void	AddHeroes(const AllHeroes &);
    void	AddCastles(const AllCastles &);

    void	AddCondLossHeroes(const AllHeroes &);
    void	AddTributeEvents(CapturedObjects &, u32 day, int obj);

    u32		size() const;

private:
    friend StreamBase & operator<< (StreamBase &, const Kingdoms &);
    friend StreamBase & operator>> (StreamBase &, Kingdoms &);

    Kingdom kingdoms[KINGDOMMAX + 1];
};

StreamBase & operator<< (StreamBase &, const Kingdom &);
StreamBase & operator>> (StreamBase &, Kingdom &);

StreamBase & operator<< (StreamBase &, const Kingdoms &);
StreamBase & operator>> (StreamBase &, Kingdoms &);

#endif
