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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <numeric>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "heroes_base.h"
#include "race.h"
#include "rand.h"
#include "serialize.h"
#include "spell.h"
#include "tools.h"

namespace
{
    constexpr std::array<int32_t, 5> guildMaxSpellsCount = { 3, 3, 2, 2, 1 };

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

    struct MageGuildLevelProps final
    {
        MageGuildLevelProps( const int spellLevel, const bool hasLibraryCapability )
            : freeSlots( MageGuild::getMaxSpellsCount( spellLevel, hasLibraryCapability ) )
        {
            // Do nothing.
        }

        int32_t freeSlots{ 0 };
        bool hasAdventureSpell{ false };
    };
}

void MageGuild::initialize( const int race, const bool hasLibrary )
{
    assert( CountBits( race ) == 1 && ( race & Race::ALL ) );

    _general.clear();
    _library.clear();

    _general.reserve( getMaxSpellsCount( -1, false ) );
    if ( hasLibrary ) {
        // Reserve for all 5 spell levels.
        _library.reserve( 5 );
    }

    std::set<Spell> spellsInUse;

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

void MageGuild::initialize( const int race, const bool hasLibrary, const std::map<uint8_t, int32_t> & mustHaveSpells, const std::vector<int32_t> & bannedSpells )
{
    if ( mustHaveSpells.empty() && bannedSpells.empty() ) {
        // Fallback to original Mage Guild initialization.
        initialize( race, hasLibrary );

        return;
    }

    assert( CountBits( race ) == 1 && ( race & Race::ALL ) );

    _general.clear();
    _library.clear();

    _general.resize( getMaxSpellsCount( -1, false ), Spell::NONE );
    if ( hasLibrary ) {
        // Library may have only 5 spell, one of each level.
        _library.resize( 5, Spell::NONE );
    }

    std::set<int32_t> spellsInUse;

    // Skip "banned" spells during random spells initialization.
    for ( auto spellId : bannedSpells ) {
        spellsInUse.insert( spellId );
    }

    std::array<MageGuildLevelProps, 5> mageGuildLevels = { { { 1, hasLibrary }, { 2, hasLibrary }, { 3, hasLibrary }, { 4, hasLibrary }, { 5, hasLibrary } } };

    bool hasGuaranteedDamageSpell = false;
    bool hasGuaranteedCancellationSpell = false;

    std::vector<int32_t> guaranteedDamageSpells;
    std::vector<int32_t> guaranteedCancellationSpells;

    // Make the "guaranteed" spells vectors excluding the banned spells.
    auto addSpellsToVector = [&spellsInUse]( const std::vector<int32_t> & spells, std::vector<int32_t> & output ) {
        for ( const int32_t spellId : spells ) {
            if ( spellsInUse.count( spellId ) == 0 ) {
                output.push_back( spellId );
            }
        }
    };
    addSpellsToVector( { Spell::ARROW, Spell::LIGHTNINGBOLT, Spell::FIREBALL, Spell::COLDRAY, Spell::COLDRING }, guaranteedDamageSpells );
    addSpellsToVector( { Spell::DISPEL, Spell::MASSDISPEL, Spell::ANTIMAGIC, Spell::CURE }, guaranteedCancellationSpells );

    // Place the custom spells.
    for ( const auto & [place, spellId] : mustHaveSpells ) {
        const uint8_t levelIndex = place / 10;
        const int32_t pos = place % 10;

        assert( levelIndex < 5 );

        if ( pos == guildMaxSpellsCount[levelIndex] && !hasLibrary ) {
            // This may happen when a spell for a library was defined for Random town and the randomly selected race has no Library capability.
            continue;
        }

        if ( pos < guildMaxSpellsCount[levelIndex] ) {
            // This is a spell store in the Mage Guild.
            _general[std::accumulate( guildMaxSpellsCount.cbegin(), guildMaxSpellsCount.cbegin() + levelIndex, pos )] = spellId;
        }
        else {
            assert( pos == guildMaxSpellsCount[levelIndex] );

            // This is a spell stored in the Library.
            _library[levelIndex] = spellId;
        }

        --mageGuildLevels[levelIndex].freeSlots;

        if ( Spell( spellId ).isAdventure() ) {
            mageGuildLevels[levelIndex].hasAdventureSpell = true;
        }
        else if ( std::find( guaranteedDamageSpells.cbegin(), guaranteedDamageSpells.cend(), spellId ) != guaranteedDamageSpells.cend() ) {
            hasGuaranteedDamageSpell = true;
        }
        else if ( std::find( guaranteedCancellationSpells.cbegin(), guaranteedCancellationSpells.cend(), spellId ) != guaranteedCancellationSpells.cend() ) {
            hasGuaranteedCancellationSpell = true;
        }

        if ( const auto [dummy, inserted] = spellsInUse.insert( spellId ); !inserted ) {
            // The `mustHaveSpells` should not have duplicates.
            assert( 0 );
        }
    }

    const auto addSpell = [this, hasLibrary, &spellsInUse, &mageGuildLevels]( const Spell & spell ) {
        const int spellLevel = spell.Level();
        assert( spellLevel > 0 );

        const size_t levelIndex = fheroes2::checkedCast<size_t>( spellLevel - 1 ).value();
        assert( levelIndex < mageGuildLevels.size() );

        auto & [freeSlots, hasAdventureSpell] = mageGuildLevels[levelIndex];
        if ( freeSlots < 1 ) {
            return false;
        }

        // Check for possible duplicates
        if ( const auto [dummy, inserted] = spellsInUse.insert( spell.GetID() ); !inserted ) {
            return false;
        }

        if ( hasLibrary && _library[levelIndex] == Spell::NONE ) {
            _library[levelIndex] = spell;
        }
        else {
            const int32_t pos = std::accumulate( guildMaxSpellsCount.cbegin(), guildMaxSpellsCount.cbegin() + levelIndex, 0 );
            for ( int32_t offset = 0; offset < guildMaxSpellsCount[levelIndex]; ++offset ) {
                if ( _general[pos + offset] == Spell::NONE ) {
                    _general[pos + offset] = spell;
                    break;
                }
            }
        }

        --freeSlots;

        if ( !hasAdventureSpell && spell.isAdventure() ) {
            hasAdventureSpell = true;
        }

        return true;
    };

    // All spells in the vector have equal probability.
    auto randomlyAddAnySpellFromVector = [&addSpell]( std::vector<int32_t> & spells ) {
        while ( !spells.empty() ) {
            const uint32_t randomIndex = Rand::Get( 0, static_cast<uint32_t>( spells.size() ) - 1 );
            if ( addSpell( spells[randomIndex] ) ) {
                // The spell is successfully added.
                break;
            }

            // Remove the spell that addition failed.
            spells.erase( spells.begin() + randomIndex );
        }
    };

    // Mage Guild must always have one of the specific damage spells...
    if ( !hasGuaranteedDamageSpell ) {
        randomlyAddAnySpellFromVector( guaranteedDamageSpells );
    }
    // ... as well as one of the specific "spell cancellation" spells
    if ( !hasGuaranteedCancellationSpell ) {
        randomlyAddAnySpellFromVector( guaranteedCancellationSpells );
    }

    // Initialize random spells.
    for ( size_t level = 1; level <= mageGuildLevels.size(); ++level ) {
        const auto & [freeSlots, hasAdventureSpell] = mageGuildLevels[level - 1];

        std::vector<int> allSpellsOfLevel = Spell::getAllSpellIdsSuitableForSpellBook( fheroes2::checkedCast<int>( level ).value(), spellsInUse );

        while ( freeSlots > 0 ) {
            assert( !allSpellsOfLevel.empty() );

            const uint32_t spellIdx = Rand::Get( 0, fheroes2::checkedCast<uint32_t>( allSpellsOfLevel.size() - 1 ).value() );
            const Spell spell( allSpellsOfLevel[spellIdx] );

            // Do not skip spells if we don't have enough spells to fill all the slots.
            const bool moreSpellsThanSlots = static_cast<int32_t>( allSpellsOfLevel.size() ) > freeSlots;

            // Some spells may occur less frequently in Mage Guilds than others, depending on race.
            if ( moreSpellsThanSlots && Rand::Get( 0, 10 ) > spell.weightForRace( race ) ) {
                continue;
            }

            // There can only be one adventure spell at each level of the Mage Guild
            if ( !moreSpellsThanSlots || !hasAdventureSpell || !spell.isAdventure() ) {
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
        // Total number of spells of all levels.
        return guildMaxSpellsCount[0] + guildMaxSpellsCount[1] + guildMaxSpellsCount[2] + guildMaxSpellsCount[3] + guildMaxSpellsCount[4] + ( hasLibrary ? 1 : 0 ) * 5;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        return guildMaxSpellsCount[spellLevel - 1] + ( hasLibrary ? 1 : 0 );
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
