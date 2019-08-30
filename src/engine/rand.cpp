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

#include <cstdlib>
#include <ctime>
#include <iterator>

#include "system.h"
#include "rand.h"


void Rand::Init(void){ std::srand((u32) std::time(0)); }

u32 Rand::Get(u32 min, u32 max)
{
    if(max)
    {
	if(min > max) std::swap(min, max);

	return min + Get(max - min);
    }

    return static_cast<u32>((min + 1) * (std::rand() / (RAND_MAX + 1.0)));
}

Rand::Queue::Queue(u32 size)
{
    reserve(size);
}

void Rand::Queue::Reset(void)
{
    clear();
}

void Rand::Queue::Push(s32 value, u32 percent)
{
    if(percent)
	push_back(std::make_pair(value, percent));
}

size_t Rand::Queue::Size(void) const
{
    return size();
}

s32 Rand::Queue::Get(void)
{
    std::vector<ValuePercent>::iterator it;

    // get max
    it = begin();
    u32 max = 0;
    for(; it != end(); ++it) max += (*it).second;

    // set weight (from 100)
    it = begin();
    for(; it != end(); ++it) (*it).second = 100 * (*it).second / max;

    // get max
    max = 0;
    it = begin();
    for(; it != end(); ++it) max += (*it).second;

    u8 rand = Rand::Get(max);
    u8 amount = 0;

    it = begin();
    for(; it != end(); ++it)
    {
        amount += (*it).second;
        if(rand <= amount) return (*it).first;
    }

    ERROR("weight not found, return 0");
    return 0;
}
