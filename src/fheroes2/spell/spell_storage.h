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

#ifndef H2SPELLSTORAGE_H
#define H2SPELLSTORAGE_H

#include <vector>
#include "spell.h"
#include "gamedefs.h"

class Artifact;
class BagArtifacts;

class SpellStorage : public std::vector<Spell>
{
public:
	SpellStorage();

	u32 Size(int lvl = 0) const;

	SpellStorage GetSpells(int) const;
	void Append(const SpellStorage &);
	void Append(const Spell &);
	void Append(const BagArtifacts &);
	void Append(const Artifact &);
	bool isPresentSpell(const Spell &) const;
	std::string String(void) const;
};

#endif
