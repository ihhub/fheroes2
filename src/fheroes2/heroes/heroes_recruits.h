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

#ifndef H2RECRUITS_H
#define H2RECRUITS_H

#include <utility>
#include "gamedefs.h"

class Heroes;

class Recruits : public std::pair<int, int>
{
public:
    Recruits();

    void Reset();

    int GetID1() const;
    int GetID2() const;

    const Heroes* GetHero1() const;
    const Heroes* GetHero2() const;
    Heroes* GetHero1();
    Heroes* GetHero2();

    void SetHero1(const Heroes*);
    void SetHero2(const Heroes*);
};

StreamBase & operator>> (StreamBase &, Recruits &);

#endif
