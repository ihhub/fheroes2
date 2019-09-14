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

    bool	isValid() const;
    int		GetAttack() const;
    int		GetDefense() const;
    int		GetPower() const;
    int		GetKnowledge() const;
    int		GetMorale() const;
    int		GetLuck() const;
    int		GetRace() const;
    int		GetColor() const;
    int		GetType() const;
    int		GetControl() const;
    s32		GetIndex() const;

    const std::string &
		GetName() const;

    const Castle*
		inCastle() const;

    int		GetLevelSkill(int) const;
    u32		GetSecondaryValues(int) const;

    const Army &
		GetArmy() const;
    Army &	GetArmy();

    u32		GetMaxSpellPoints() const;

    void	ActionPreBattle();
    void	ActionAfterBattle();

    void	PortraitRedraw(s32, s32, int type, Surface &) const;
    Surface	GetPortrait(int type) const;

  private:
    Castle &	home;
};

#endif
