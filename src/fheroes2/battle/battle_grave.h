/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2BATTLE_GRAVEYARD_H
#define H2BATTLE_GRAVEYARD_H

#include <vector>
#include <map>

#include "gamedefs.h"

namespace Battle
{
    class Unit;

    struct TroopUIDs : public std::vector<u32>
    {
	TroopUIDs(){ reserve(4); }
    };

    class Graveyard : public std::map<s32, TroopUIDs>
    {
    public:
	Graveyard() {}

	Indexes		GetClosedCells(void) const;
	void		AddTroop(const Unit &);
	void		RemoveTroop(const Unit &);
	u32		GetLastTroopUID(s32) const;
    };
}

#endif
