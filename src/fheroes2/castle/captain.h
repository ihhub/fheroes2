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
#ifndef H2CAPTAIN_H
#define H2CAPTAIN_H

#include "gamedefs.h"
#include "heroes_base.h"

class Castle;

class Captain : public HeroBase
{
  public:
    Captain(Castle &);

    bool	isValid(void) const;
    int		GetAttack(void) const;
    int		GetDefense(void) const;
    int		GetPower(void) const;
    int		GetKnowledge(void) const;
    int		GetMorale(void) const;
    int		GetLuck(void) const;
    int		GetRace(void) const;
    int		GetColor(void) const;
    int		GetType(void) const;
    int		GetControl(void) const;
    s32		GetIndex(void) const;

    const std::string &
		GetName(void) const;

    const Castle*
		inCastle(void) const;

    int		GetLevelSkill(int) const;
    u32		GetSecondaryValues(int) const;

    const Army &
		GetArmy(void) const;
    Army &	GetArmy(void);

    u32		GetMaxSpellPoints(void) const;

    void	ActionPreBattle(void);
    void	ActionAfterBattle(void);

    void	PortraitRedraw(s32, s32, int type, Surface &) const;
    Surface	GetPortrait(int type) const;

  private:
    Castle &	home;
};

#endif
