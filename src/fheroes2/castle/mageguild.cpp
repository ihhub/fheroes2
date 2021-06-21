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

#include "heroes_base.h"
#include "mageguild.h"
#include "rand.h"

Spell GetUniqueSpellCompatibility( const SpellStorage & spells, const int race, const int level );
Spell GetGuaranteedDamageSpellForMageGuild();
Spell GetGuaranteedNonDamageSpellForMageGuild();

void MageGuild::initialize( int race, bool libraryCap )
{
    general.clear();
    library.clear();

    int spellCountByLevel[] = {3, 3, 2, 2, 1};

    const Spell guaranteedDamageSpell = GetGuaranteedDamageSpellForMageGuild();
    const int guaranteedDamageSpellLevel = guaranteedDamageSpell.Level();

    const Spell guaranteedNonDamageSpell = GetGuaranteedNonDamageSpellForMageGuild();
    const int guaranteedNonDamageSpellLevel = guaranteedNonDamageSpell.Level();

    general.Append( guaranteedDamageSpell );
    general.Append( guaranteedNonDamageSpell );

    if ( libraryCap ) {
        for ( int i = 0; i < 5; ++i )
            ++spellCountByLevel[i];
    }

    --spellCountByLevel[guaranteedDamageSpellLevel - 1];
    --spellCountByLevel[guaranteedNonDamageSpellLevel - 1];

    SpellStorage all( general );

    for ( int i = 0; i < 5; ++i ) {
        for ( int j = 0; j < spellCountByLevel[i]; ++j ) {
            const Spell spell = GetUniqueSpellCompatibility( all, race, i + 1 );

            if ( spell == Spell::NONE ) {
                continue;
            }

            if ( libraryCap && j == spellCountByLevel[i] - 1 ) {
                library.Append( spell );
            }
            else {
                general.Append( spell );
            }

            all.Append( spell );
        }
    }
}

SpellStorage MageGuild::GetSpells( int guildLevel, bool hasLibrary, int spellLevel ) const
{
    SpellStorage result;

    if ( spellLevel == -1 ) {
        // get all available spells
        for ( int level = 1; level <= guildLevel; ++level ) {
            result.Append( general.GetSpells( level ) );
            if ( hasLibrary )
                result.Append( library.GetSpells( level ) );
        }
    }
    else if ( spellLevel <= guildLevel ) {
        result = general.GetSpells( spellLevel );
        if ( hasLibrary )
            result.Append( library.GetSpells( spellLevel ) );
    }

    return result;
}

void MageGuild::educateHero( HeroBase & hero, int guildLevel, bool hasLibrary ) const
{
    if ( hero.HaveSpellBook() && guildLevel > 0 ) {
        // this method will check wisdom requirement
        hero.AppendSpellsToBook( MageGuild::GetSpells( guildLevel, hasLibrary ) );
    }
}

Spell GetUniqueSpellCompatibility( const SpellStorage & spells, const int race, const int lvl )
{
    const bool hasAdventureSpell = spells.hasAdventureSpell( lvl );
    const bool lookForAdv = hasAdventureSpell ? false : Rand::Get( 0, 1 ) == 0 ? true : false;

    std::vector<Spell> v;
    v.reserve( 15 );

    for ( int sp = Spell::NONE; sp < Spell::STONE; ++sp ) {
        const Spell spell( sp );

        if ( spells.isPresentSpell( spell ) )
            continue;

        if ( !spell.isRaceCompatible( race ) )
            continue;

        if ( spell.Level() != lvl || !spell.isEnabled() )
            continue;

        if ( lookForAdv != spell.isCombat() )
            v.push_back( spell );
    }

    return v.size() ? Rand::Get( v ) : Spell( Spell::NONE );
}

Spell GetGuaranteedDamageSpellForMageGuild()
{
    switch ( Rand::Get( 0, 4 ) ) {
    case 0:
        return Spell::ARROW;
    case 1:
        return Spell::COLDRAY;
    case 2:
        return Spell::LIGHTNINGBOLT;
    case 3:
        return Spell::COLDRING;
    case 4:
        return Spell::FIREBALL;
    default:
        return Spell::RANDOM;
    }
}

Spell GetGuaranteedNonDamageSpellForMageGuild()
{
    switch ( Rand::Get( 0, 4 ) ) {
    case 0:
        return Spell::DISPEL;
    case 1:
        return Spell::MASSDISPEL;
    case 2:
        return Spell::CURE;
    case 3:
        return Spell::MASSCURE;
    case 4:
        return Spell::ANTIMAGIC;
    default:
        return Spell::RANDOM;
    }
}

StreamBase & operator<<( StreamBase & msg, const MageGuild & guild )
{
    return msg << guild.general << guild.library;
}

StreamBase & operator>>( StreamBase & msg, MageGuild & guild )
{
    return msg >> guild.general >> guild.library;
}
