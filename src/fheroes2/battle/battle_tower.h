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

#ifndef H2BATTLE_TOWER_H
#define H2BATTLE_TOWER_H

#include "gamedefs.h"
#include "battle_command.h"
#include "battle_troop.h"

class Castle;

namespace Battle
{
    enum { TWR_LEFT = 0x01, TWR_CENTER = 0x02, TWR_RIGHT = 0x04 };

    class Tower : public Unit
    {
    public:
	Tower(const Castle &, int);

	bool	isValid(void) const;
	int	GetColor(void) const;
	u32	GetType(void) const;
	u32	GetBonus(void) const;
	u32	GetAttack(void) const;

	const char* GetName(void) const;

	void	SetDestroy(void);
	Point	GetPortPosition(void) const;

	static std::string GetInfo(const Castle &);

    private:
	int	type;
	int	color;
	u32	bonus;
	bool	valid;
    };
}

#endif
