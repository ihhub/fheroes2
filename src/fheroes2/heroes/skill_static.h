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

#ifndef H2SKILL_STATIC_H
#define H2SKILL_STATIC_H

#include "gamedefs.h"

namespace Skill
{
    struct level_t
    {
	u16 basic;
	u16 advanced;
	u16 expert;
    };

    struct primary_t
    {
	u8 attack;
	u8 defense;
	u8 power;
	u8 knowledge;
    };

    struct secondary_t
    {
	u8 archery;
	u8 ballistics;
	u8 diplomacy;
	u8 eagleeye;
	u8 estates;
	u8 leadership;
	u8 logistics;
	u8 luck;
	u8 mysticism;
	u8 navigation;
	u8 necromancy;
	u8 pathfinding;
	u8 scouting;
	u8 wisdom;
    };

    struct stats_t
    {
	const char* id;
	primary_t   captain_primary;
	primary_t   initial_primary;
	u8          initial_book;
	u8          initial_spell;
	secondary_t initial_secondary;
	u8          over_level;
	primary_t   mature_primary_under;
	primary_t   mature_primary_over;
	secondary_t mature_secondary;
    };

    struct values_t
    {
	const char *id;
	level_t values;
    };
}

#endif
