/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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
#include <vector>

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

    Spell getGuaranteedCancellationSpell()
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

    _general.clear();
    _library.clear();

    _general.reserve( getMaxSpellsCount( -1, false ) );
    if ( hasLibrary ) {
        _library.reserve( 1 );
    }

    std::set<Spell> spellsInUse;

    struct MageGuildLevelProps
    {
        MageGuildLevelProps( const int spellLevel, const bool hasLibraryCapability )
            : freeSlots( getMaxSpellsCount( spellLevel, hasLibraryCapability ) )
        {
            // Do nothing.
        }

        int32_t freeSlots;
        bool hasAdventureSpell{ false };
    };

    std::array<MageGuildLevelProps, 5> mageGuildLevels = { { { 1, hasLibrary }, { 2, hasLibrary }, { 3, hasLibrary }, { 4, hasLibrary }, { 5, hasLibrary } } };

    const auto addSpell = [this, hasLibrary, &spellsInUse, &mageGuildLevels]( const Spell & spell ) {
        const size_t spellLevel = fheroes2::checkedCast<size_t>( spell.Level() ).value();
        assert( spellLevel > 0 && spellLevel <= mageGuildLevels.size() );

        auto & [freeSlots, hasAdventureSpell] = mageGuildLevels[spellLevel - 1];
        assert( freeSlots > 0 );

        // Check for possible duplicates
        if ( const auto [dummy, inserted] = spellsInUse.insert( spell ); !inserted ) {
            return;
        }

        if ( hasLibrary && freeSlots == 1 ) {
            _library.Append( spell );
        }
        else {
            _general.Append( spell );
        }

        --freeSlots;

        if ( spell.isAdventure() ) {
            assert( !hasAdventureSpell );

            hasAdventureSpell = true;
        }
    };

    // Mage Guild must always have one of the specific damage spells...
    addSpell( getGuaranteedDamageSpell() );
    // ... as well as one of the specific "spell cancellation" spells
    addSpell( getGuaranteedCancellationSpell() );

    for ( size_t level = 1; level <= mageGuildLevels.size(); ++level ) {
        const auto & [freeSlots, hasAdventureSpell] = mageGuildLevels[level - 1];

        std::vector<int> allSpellsOfLevel = Spell::getAllSpellIdsSuitableForSpellBook( fheroes2::checkedCast<int>( level ).value() );

        while ( freeSlots > 0 ) {
            assert( !allSpellsOfLevel.empty() );

            const uint32_t spellIdx = Rand::Get( 0, fheroes2::checkedCast<uint32_t>( allSpellsOfLevel.size() - 1 ).value() );
            const Spell spell( allSpellsOfLevel[spellIdx] );

            // Some spells may occur less frequently in Mage Guilds than others, depending on race
            if ( Rand::Get( 0, 10 ) > spell.weightForRace( race ) ) {
                continue;
            }

            // There can only be one adventure spell at each level of the Mage Guild
            if ( !hasAdventureSpell || !spell.isAdventure() ) {
                addSpell( spell );
            }

            allSpellsOfLevel.erase( allSpellsOfLevel.begin() + spellIdx );
        }
    }
}

SpellStorage MageGuild::GetSpells( const int guildLevel, const bool hasLibrary, const int spellLevel /* = -1 */ ) const
{
    SpellStorage result;

    if ( spellLevel == -1 ) {
        // Get all available spells
        if ( guildLevel == 5 ) {
            // Get spells for all 1-5 levels.
            result = _general.GetSpells();

            if ( hasLibrary ) {
                result.Append( _library.GetSpells() );
            }
        }
        else {
            for ( int level = 1; level <= guildLevel; ++level ) {
                result.Append( _general.GetSpells( level ) );

                if ( hasLibrary ) {
                    result.Append( _library.GetSpells( level ) );
                }
            }
        }
    }
    else if ( spellLevel <= guildLevel ) {
        result = _general.GetSpells( spellLevel );

        if ( hasLibrary ) {
            result.Append( _library.GetSpells( spellLevel ) );
        }
    }

    return result;
}

int32_t MageGuild::getMaxSpellsCount( const int spellLevel, const bool hasLibrary )
{
    switch ( spellLevel ) {
    case -1:
        // Total number of spells in Mage Guild and Library.
        return 3 + 3 + 2 + 2 + 1 + 1 * 5;
    case 1:
    case 2:
        return 3 + ( hasLibrary ? 1 : 0 );
    case 3:
    case 4:
        return 2 + ( hasLibrary ? 1 : 0 );
    case 5:
        return 1 + ( hasLibrary ? 1 : 0 );
    default:
        return 0;
    }
}

void MageGuild::trainHero( HeroBase & hero, const int guildLevel, const bool hasLibrary ) const
{
    if ( !hero.HaveSpellBook() ) {
        return;
    }

    if ( guildLevel < 1 ) {
        return;
    }

    // This method will test the hero for compliance with the wisdom requirements
    hero.AppendSpellsToBook( GetSpells( guildLevel, hasLibrary ) );
}

OStreamBase & operator<<( OStreamBase & stream, const MageGuild & guild )
{
    return stream << guild._general << guild._library;
}

IStreamBase & operator>>( IStreamBase & stream, MageGuild & guild )
{
    return stream >> guild._general >> guild._library;
}
