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

#ifndef H2ARMYTROOP_H
#define H2ARMYTROOP_H

#include <string>
#include "monster.h"

class Army;

class Troop : public Monster
{
public:
    Troop();
    Troop(const Monster &, u32);

    bool		operator== (const Monster &) const;
    Monster		operator() (void) const;

    void		Set(const Troop &);
    void		Set(const Monster &, u32);
    void		SetMonster(const Monster &);
    void		SetCount(u32);
    void		Reset(void);

    bool		isMonster(int) const;
    const char*		GetName(void) const;
    virtual u32		GetCount(void) const;
    u32			GetHitPoints(void) const;
    Monster		GetMonster(void) const;

    u32			GetDamageMin(void) const;
    u32			GetDamageMax(void) const;
    u32			GetStrength(void) const;


    payment_t		GetCost(void) const;
    payment_t		GetUpgradeCost(void) const;

    virtual bool	isValid(void) const;
    virtual bool	isBattle(void) const;
    virtual bool	isModes(u32) const;
    virtual std::string	GetAttackString(void) const;
    virtual std::string	GetDefenseString(void) const;
    virtual std::string	GetShotString(void) const;
    virtual std::string	GetSpeedString(void) const;
    virtual u32		GetHitPointsLeft(void) const;
    virtual u32		GetSpeed(void) const;
    virtual u32		GetAffectedDuration(u32) const;

protected:
    friend StreamBase & operator<< (StreamBase &, const Troop &);
    friend StreamBase & operator>> (StreamBase &, Troop &);

    u32			count;
};

StreamBase & operator<< (StreamBase &, const Troop &);
StreamBase & operator>> (StreamBase &, Troop &);

class ArmyTroop : public Troop
{
public:
    ArmyTroop(Army*);
    ArmyTroop(Army*, const Troop &);

    ArmyTroop &		operator= (const Troop &);

    u32			GetAttack(void) const;
    u32			GetDefense(void) const;
    int			GetColor(void) const;
    int			GetMorale(void) const;
    int			GetLuck(void) const;

    void		SetArmy(const Army &);
    const Army*		GetArmy(void) const;

    std::string		GetAttackString(void) const;
    std::string		GetDefenseString(void) const;

protected:
    const Army*		army;
};

#endif
