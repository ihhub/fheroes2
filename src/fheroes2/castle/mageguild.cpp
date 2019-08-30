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

#include <algorithm>
#include "castle.h"
#include "heroes_base.h"
#include "race.h"
#include "settings.h"
#include "mageguild.h"

Spell GetUniqueCombatSpellCompatibility(const SpellStorage &, int race, int level);
Spell GetCombatSpellCompatibility(int race, int level);

void MageGuild::Builds(int race, bool libraryCap)
{
    general.clear();
    library.clear();

    // level 5
    general.Append(7 > Rand::Get(1, 10) ? Spell::RandCombat(5) : Spell::RandAdventure(5));

    // level 4
    general.Append(GetCombatSpellCompatibility(race, 4));
    general.Append(Spell::RandAdventure(4));

    // level 3
    general.Append(GetCombatSpellCompatibility(race, 3));
    general.Append(Spell::RandAdventure(3));

    // level 2
    general.Append(GetCombatSpellCompatibility(race, 2));
    general.Append(GetUniqueCombatSpellCompatibility(general, race, 2));
    general.Append(Spell::RandAdventure(2));

    // level 1
    general.Append(GetCombatSpellCompatibility(race, 1));
    general.Append(GetUniqueCombatSpellCompatibility(general, race, 1));
    general.Append(Spell::RandAdventure(1));

    if(libraryCap)
    {
	library.Append(GetUniqueCombatSpellCompatibility(general, race, 1));
	library.Append(GetUniqueCombatSpellCompatibility(general, race, 2));
	library.Append(GetUniqueCombatSpellCompatibility(general, race, 3));
	library.Append(GetUniqueCombatSpellCompatibility(general, race, 4));
	library.Append(GetUniqueCombatSpellCompatibility(general, race, 5));
    }
}

SpellStorage MageGuild::GetSpells(int lvlmage, bool islibrary, int level) const
{
    SpellStorage result;

    if(lvlmage >= level)
    {
	result = general.GetSpells(level);
	if(islibrary) result.Append(library.GetSpells(level));
    }

    return result;
}

void MageGuild::EducateHero(HeroBase & hero, int lvlmage, bool isLibraryBuild) const
{
    if(hero.HaveSpellBook() && lvlmage)
    {
	SpellStorage spells;

	for(s32 level = 1; level <= 5; ++level) if(level <= lvlmage)
	{
	    spells.Append(general.GetSpells(level));
	    if(isLibraryBuild) spells.Append(library.GetSpells(level));
	}

	hero.AppendSpellsToBook(spells);
    }
}

Spell GetUniqueCombatSpellCompatibility(const SpellStorage & spells, int race, int lvl)
{
    Spell spell = GetCombatSpellCompatibility(race, lvl);
    while(spells.isPresentSpell(spell)) spell = GetCombatSpellCompatibility(race, lvl);
    return spell;
}

Spell GetCombatSpellCompatibility(int race, int lvl)
{
    Spell spell = Spell::RandCombat(lvl);
    while(!spell.isRaceCompatible(race)) spell = Spell::RandCombat(lvl);
    return spell;
}

StreamBase & operator<< (StreamBase & msg, const MageGuild & guild)
{
    return msg << guild.general << guild.library;
}

StreamBase & operator>> (StreamBase & msg, MageGuild & guild)
{
    return msg >> guild.general >> guild.library;
}
