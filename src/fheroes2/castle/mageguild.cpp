/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "mageguild.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>

#include "heroes_base.h"
#include "race.h"
#include "rand.h"
#include "serialize.h"
#include "spell.h"
#include "tools.h"

namespace
{
    Spell getGuaranteedDamageSpell()
    {
        const uint32_t rand = Rand::Get( 0, 100 );

        if ( rand < 20 ) {
            return Spell::ARROW;
        }

        if ( rand < 40 ) {
            return Spell::LIGHTNINGBOLT;
        }

        if ( rand < 60 ) {
            return Spell::FIREBALL;
        }

        if ( rand < 80 ) {
            return Spell::COLDRAY;
        }

        return Spell::COLDRING;
    }

    Spell getGuaranteedRestorationSpell()
    {
        const uint32_t rand = Rand::Get( 0, 100 );

        if ( rand < 25 ) {
            return Spell::DISPEL;
        }

        if ( rand < 50 ) {
            return Spell::MASSDISPEL;
        }

        if ( rand < 75 ) {
            return Spell::ANTIMAGIC;
        }

        return Spell::CURE;
    }
}

void MageGuild::initialize( const int race, const bool hasLibrary )
{
    assert( CountBits( race ) == 1 && ( race & Race::ALL ) );

    general.clear();
    library.clear();

    struct MageGuildLevelProps
    {
        int freeSlots = 0;
        bool hasAdventureSpell = false;
    };

    std::array<MageGuildLevelProps, 5> mageGuildLevels = { { { 3, false }, { 3, false }, { 2, false }, { 2, false }, { 1, false } } };
    if ( hasLibrary ) {
        for ( auto & [freeSlots, dummy] : mageGuildLevels ) {
            ++freeSlots;
        }
    }

    std::set<Spell> allSpells;

    auto addSpell = [this, race, hasLibrary, &mageGuildLevels, &allSpells]( const Spell & spell ) {
        const size_t spellLevel = fheroes2::checkedCast<size_t>( spell.Level() ).value();
        assert( spellLevel > 0 && spellLevel <= mageGuildLevels.size() );

        auto & [freeSlots, hasAdventureSpell] = mageGuildLevels[spellLevel - 1];
        assert( freeSlots > 0 );

        if ( hasAdventureSpell && spell.isAdventure() ) {
            return false;
        }

        if ( Rand::Get( 0, 10 ) > spell.weightForRace( race ) ) {
            return false;
        }

        const auto [dummy, inserted] = allSpells.insert( spell );
        if ( !inserted ) {
            return false;
        }

        if ( hasLibrary && freeSlots == 1 ) {
            library.Append( spell );
        }
        else {
            general.Append( spell );
        }

        --freeSlots;

        if ( spell.isAdventure() ) {
            hasAdventureSpell = true;
        }

        return true;
    };

    if ( !addSpell( getGuaranteedDamageSpell() ) ) {
        assert( 0 );
    }
    if ( !addSpell( getGuaranteedRestorationSpell() ) ) {
        assert( 0 );
    }

    for ( size_t level = 1; level <= mageGuildLevels.size(); ++level ) {
        const auto & [freeSlots, hasAdventureSpell] = mageGuildLevels[level - 1];

        while ( freeSlots > 0 ) {
            const Spell spell = Spell::Rand();

            if ( fheroes2::checkedCast<size_t>( spell.Level() ).value() != level ) {
                continue;
            }

            addSpell( spell );
        }
    }
}

SpellStorage MageGuild::GetSpells( int guildLevel, bool hasLibrary, int spellLevel ) const
{
    SpellStorage result;

    if ( spellLevel == -1 ) {
        // Get all available spells
        for ( int level = 1; level <= guildLevel; ++level ) {
            result.Append( general.GetSpells( level ) );

            if ( hasLibrary ) {
                result.Append( library.GetSpells( level ) );
            }
        }
    }
    else if ( spellLevel <= guildLevel ) {
        result = general.GetSpells( spellLevel );

        if ( hasLibrary ) {
            result.Append( library.GetSpells( spellLevel ) );
        }
    }

    return result;
}

void MageGuild::educateHero( HeroBase & hero, int guildLevel, bool hasLibrary ) const
{
    if ( hero.HaveSpellBook() && guildLevel > 0 ) {
        // This method will test the hero for compliance with the wisdom requirements
        hero.AppendSpellsToBook( MageGuild::GetSpells( guildLevel, hasLibrary ) );
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
