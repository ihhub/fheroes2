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

#ifndef H2AI_H
#define H2AI_H

#include "gamedefs.h"

class Castle;
class HeroBase;
class Heroes;
class Kingdom;
namespace Battle { class Arena; class Unit; class Actions; }

namespace AI
{
    enum modes_t
    {
	HEROES_MOVED	= 0x08000000,
	HEROES_SCOUTER	= 0x10000000,
	HEROES_HUNTER	= 0x20000000,
	HEROES_WAITING	= 0x40000000,
	HEROES_STUPID	= 0x80000000
    };

    void Init(void);

    void KingdomTurn(Kingdom &);
    void BattleTurn(Battle::Arena &, const Battle::Unit &, Battle::Actions &);
    bool BattleMagicTurn(Battle::Arena &, const Battle::Unit &, Battle::Actions &, const Battle::Unit*);

    void HeroesAdd(const Heroes &);
    void HeroesRemove(const Heroes &);
    void HeroesTurn(Heroes &);
    void HeroesMove(Heroes &);
    bool HeroesGetTask(Heroes &);
    void HeroesPreBattle(HeroBase &);
    void HeroesAfterBattle(HeroBase &);
    void HeroesPostLoad(Heroes &);
    bool HeroesValidObject(const Heroes &, s32);
    bool HeroesCanMove(const Heroes &);
    void HeroesAction(Heroes &, s32);
    void HeroesActionComplete(Heroes &, s32);
    void HeroesActionNewPosition(Heroes &);
    void HeroesLevelUp(Heroes &);
    void HeroesClearTask(const Heroes &);
    bool HeroesSkipFog(void);
    std::string HeroesString(const Heroes &);

    void CastleAdd(const Castle &);
    void CastleRemove(const Castle &);
    void CastleTurn(Castle &);
    void CastlePreBattle(Castle &);
    void CastleAfterBattle(Castle &, bool attacker_wins);

    const char* Type(void);
    const char* License(void);
}

#endif
