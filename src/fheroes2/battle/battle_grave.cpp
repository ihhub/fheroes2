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

#include <algorithm>
#include "battle_troop.h"
#include "battle_board.h"
#include "battle_grave.h"

Battle::Indexes Battle::Graveyard::GetClosedCells(void) const
{
    Indexes res;
    res.reserve(size());

    for(const_iterator it = begin(); it != end(); ++it)
        res.push_back((*it).first);

    return res;
}

void Battle::Graveyard::AddTroop(const Unit & b)
{
    Graveyard & map = *this;

    map[b.GetHeadIndex()].push_back(b.GetUID());

    if(b.isWide())
        map[b.GetTailIndex()].push_back(b.GetUID());
}

void Battle::Graveyard::RemoveTroop(const Unit & b)
{
    Graveyard & map = *this;
    TroopUIDs & ids = map[b.GetHeadIndex()];

    TroopUIDs::iterator it = std::find(ids.begin(), ids.end(), b.GetUID());
    if(it != ids.end()) ids.erase(it);

    if(b.isWide())
    {
        TroopUIDs & ids2 = map[b.GetTailIndex()];

        it = std::find(ids2.begin(), ids2.end(), b.GetUID());
        if(it != ids2.end()) ids2.erase(it);
    }
}

u32 Battle::Graveyard::GetLastTroopUID(s32 index) const
{
    for(const_iterator it = begin(); it != end(); ++it)
        if(index == (*it).first && (*it).second.size())
        return (*it).second.back();

    return 0;
}
