/***************************************************************************
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2CASTLE_HEROES_H
#define H2CASTLE_HEROES_H

#include <utility>
#include <algorithm>
#include "gamedefs.h"
#include "heroes.h"

class CastleHeroes : protected std::pair<Heroes*, Heroes*>
{
public:
    CastleHeroes(Heroes* guest, Heroes* guard) : std::pair<Heroes*, Heroes*>(guest, guard) {};

    Heroes* Guest(void) { return first; };
    Heroes* Guard(void) { return second; };
    const Heroes* Guest(void) const { return first; };
    const Heroes* Guard(void) const { return second; };
    Heroes* GuestFirst(void) { return first ? first : second; };
    Heroes* GuardFirst(void) { return second ? second : first; };

    bool operator== (const Heroes* hero) const { return first == hero || second == hero; };

    void Swap(void) { std::swap(first, second); };
    bool FullHouse(void) const { return first && second; };
    bool IsValid(void) const { return first || second; };
};

#endif
