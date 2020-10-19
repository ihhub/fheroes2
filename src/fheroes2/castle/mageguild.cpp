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
#include "mageguild.h"
#include "race.h"
#include "settings.h"

Spell GetUniqueCombatSpellCompatibility( const SpellStorage &, int race, int level );
Spell GetCombatSpellCompatibility( int race, int level );
Spell GetUniqueSpellCompatibility( const SpellStorage &, const int race, const int level );
Spell GetGuaranteedDamageSpellForMageGuild();
Spell GetGuaranteedNonDamageSpellForMageGuild();

void MageGuild::Builds( int race, bool libraryCap )
{
    general.clear();
    library.clear();

    const Spell guaranteedDamageSpell = GetGuaranteedDamageSpellForMageGuild();
    const int guaranteedDamageSpellLevel = guaranteedDamageSpell.Level();

    const Spell guaranteedNonDamageSpell = GetGuaranteedNonDamageSpellForMageGuild();
    const int guaranteedNonDamageSpellLevel = guaranteedNonDamageSpell.Level();

    general.Append( guaranteedDamageSpell );
    general.Append( guaranteedNonDamageSpell );

    int level1SpellCount = 3;
    int level2SpellCount = 3;
    int level3SpellCount = 2;
    int level4SpellCount = 2;
    int level5SpellCount = 1;

    if ( libraryCap ) {
        level1SpellCount++;
        level2SpellCount++;
        level3SpellCount++;
        level4SpellCount++;
        level5SpellCount++;
    }

    switch ( guaranteedDamageSpellLevel ) {
    case 1:
        level1SpellCount--;
        break;
    case 2:
        level2SpellCount--;
        break;
    case 3:
        level3SpellCount--;
        break;
    case 4:
        level4SpellCount--;
        break;
    case 5:
        level5SpellCount--;
        break;
    }

    switch ( guaranteedNonDamageSpellLevel ) {
    case 1:
        level1SpellCount--;
        break;
    case 2:
        level2SpellCount--;
        break;
    case 3:
        level3SpellCount--;
        break;
    case 4:
        level4SpellCount--;
        break;
    case 5:
        level5SpellCount--;
        break;
    }

    for ( int i = 0; i < level5SpellCount; ++i )
        general.Append( GetUniqueSpellCompatibility( general, race, 5 ) );

    for ( int i = 0; i < level4SpellCount; ++i )
        general.Append( GetUniqueSpellCompatibility( general, race, 4 ) );

    for ( int i = 0; i < level3SpellCount; ++i )
        general.Append( GetUniqueSpellCompatibility( general, race, 3 ) );

    for ( int i = 0; i < level2SpellCount; ++i )
        general.Append( GetUniqueSpellCompatibility( general, race, 2 ) );

    for ( int i = 0; i < level1SpellCount; ++i )
        general.Append( GetUniqueSpellCompatibility( general, race, 1 ) );
}

SpellStorage MageGuild::GetSpells( int lvlmage, bool islibrary, int level ) const
{
    SpellStorage result;

    if ( lvlmage >= level ) {
        result = general.GetSpells( level );
        if ( islibrary )
            result.Append( library.GetSpells( level ) );
    }

    return result;
}

void MageGuild::EducateHero( HeroBase & hero, int lvlmage, bool isLibraryBuild ) const
{
    if ( hero.HaveSpellBook() && lvlmage ) {
        SpellStorage spells;

        for ( s32 level = 1; level <= 5; ++level )
            if ( level <= lvlmage ) {
                spells.Append( general.GetSpells( level ) );
                if ( isLibraryBuild )
                    spells.Append( library.GetSpells( level ) );
            }

        hero.AppendSpellsToBook( spells );
    }
}

Spell GetUniqueCombatSpellCompatibility( const SpellStorage & spells, int race, int lvl )
{
    Spell spell = GetCombatSpellCompatibility( race, lvl );
    while ( spells.isPresentSpell( spell ) )
        spell = GetCombatSpellCompatibility( race, lvl );
    return spell;
}

Spell GetUniqueSpellCompatibility( const SpellStorage & spells, const int race, const int lvl )
{
    Spell spell = Spell::Rand( lvl, Rand::Get( 0, 1 ) == 0 ? true : false );

    while ( spells.isPresentSpell( spell ) || !spell.isRaceCompatible( race ) )
        spell = Spell::Rand( lvl, Rand::Get( 0, 1 ) == 0 ? true : false );

    return spell;
}

Spell GetCombatSpellCompatibility( int race, int lvl )
{
    Spell spell = Spell::RandCombat( lvl );
    while ( !spell.isRaceCompatible( race ) )
        spell = Spell::RandCombat( lvl );
    return spell;
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
