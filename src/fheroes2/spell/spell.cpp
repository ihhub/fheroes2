/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#include "spell.h"

#include <algorithm>
#include <cassert>
#include <vector>

#include "artifact.h"
#include "artifact_info.h"
#include "heroes_base.h"
#include "monster.h"
#include "race.h"
#include "rand.h"
#include "serialize.h"
#include "translations.h"

struct SpellStats
{
    const char * name;
    uint8_t spellPoints; // The number of spell points consumed/required by this spell
    uint16_t movePoints; // The number of movement points consumed by this spell
    uint16_t minMovePoints; // The minimum number of movement points required to cast this spell
    uint32_t imageId;
    uint8_t extraValue;
    const char * description;
};

// The original resources don't have most of sprites for Mass Spells
// so we made some tricks in AGG source file. All modified sprite IDs start from 60

const SpellStats spells[Spell::SPELL_COUNT] = {
    //  name | spell points | movement points | min movement points | image id | extra value | description
    { "Unknown", 0, 0, 0, 0, 0, "Unknown spell." },
    { gettext_noop( "Fireball" ), 9, 0, 0, 8, 10, gettext_noop( "Causes a giant fireball to strike the selected area, damaging all nearby creatures." ) },
    { gettext_noop( "Fireblast" ), 15, 0, 0, 9, 10,
      gettext_noop( "An improved version of fireball, fireblast affects two hexes around the center point of the spell, rather than one." ) },
    { gettext_noop( "Lightning Bolt" ), 7, 0, 0, 4, 25, gettext_noop( "Causes a bolt of electrical energy to strike the selected creature." ) },
    { gettext_noop( "Chain Lightning" ), 15, 0, 0, 5, 40,
      gettext_noop(
          "Causes a bolt of electrical energy to strike a selected creature, then strike the nearest creature with half damage, then strike the NEXT nearest creature with half again damage, and so on, until it becomes too weak to be harmful. Warning: This spell can hit your own creatures!" ) },
    { gettext_noop( "Teleport" ), 9, 0, 0, 10, 0, gettext_noop( "Teleports the creature you select to any open position on the battlefield." ) },
    { gettext_noop( "Cure" ), 6, 0, 0, 6, 5,
      gettext_noop( "Removes all negative spells cast upon one of your units, and restores up to %{count} HP per level of spell power." ) },
    { gettext_noop( "Mass Cure" ), 15, 0, 0, 60, 5,
      gettext_noop( "Removes all negative spells cast upon your forces, and restores up to %{count} HP per level of spell power, per creature." ) },
    { gettext_noop( "Resurrect" ), 12, 0, 0, 13, 50, gettext_noop( "Resurrects creatures from a damaged or dead unit until end of combat." ) },
    { gettext_noop( "Resurrect True" ), 15, 0, 0, 12, 50, gettext_noop( "Resurrects creatures from a damaged or dead unit permanently." ) },
    { gettext_noop( "Haste" ), 3, 0, 0, 14, 2, gettext_noop( "Increases the speed of any creature by %{count}." ) },
    { gettext_noop( "Mass Haste" ), 10, 0, 0, 61, 2, gettext_noop( "Increases the speed of all of your creatures by %{count}." ) },
    { gettext_noop( "spell|Slow" ), 3, 0, 0, 1, 0, gettext_noop( "Slows target to half movement rate." ) },
    { gettext_noop( "Mass Slow" ), 15, 0, 0, 62, 0, gettext_noop( "Slows all enemies to half movement rate." ) },
    { gettext_noop( "spell|Blind" ), 6, 0, 0, 21, 0, gettext_noop( "Clouds the affected creatures' eyes, preventing them from moving." ) },
    { gettext_noop( "Bless" ), 3, 0, 0, 7, 0, gettext_noop( "Causes the selected creatures to inflict maximum damage." ) },
    { gettext_noop( "Mass Bless" ), 12, 0, 0, 63, 0, gettext_noop( "Causes all of your units to inflict maximum damage." ) },
    { gettext_noop( "Stoneskin" ), 3, 0, 0, 31, 3, gettext_noop( "Magically increases the defense skill of the selected creatures." ) },
    { gettext_noop( "Steelskin" ), 6, 0, 0, 30, 5, gettext_noop( "Increases the defense skill of the targeted creatures. This is an improved version of Stoneskin." ) },
    { gettext_noop( "Curse" ), 3, 0, 0, 3, 0, gettext_noop( "Causes the selected creatures to inflict minimum damage." ) },
    { gettext_noop( "Mass Curse" ), 12, 0, 0, 64, 0, gettext_noop( "Causes all enemy troops to inflict minimum damage." ) },
    { gettext_noop( "Holy Word" ), 9, 0, 0, 22, 10, gettext_noop( "Damages all undead in the battle." ) },
    { gettext_noop( "Holy Shout" ), 12, 0, 0, 23, 20, gettext_noop( "Damages all undead in the battle. This is an improved version of Holy Word." ) },
    { gettext_noop( "Anti-Magic" ), 7, 0, 0, 17, 0, gettext_noop( "Prevents any magic against the selected creatures." ) },
    { gettext_noop( "Dispel Magic" ), 5, 0, 0, 18, 0, gettext_noop( "Removes all magic spells from a single target." ) },
    { gettext_noop( "Mass Dispel" ), 12, 0, 0, 18, 0, gettext_noop( "Removes all magic spells from all creatures." ) },
    { gettext_noop( "Magic Arrow" ), 3, 0, 0, 38, 10, gettext_noop( "Causes a magic arrow to strike the selected target." ) },
    { gettext_noop( "Berserker" ), 12, 0, 0, 19, 0, gettext_noop( "Causes a creature to attack its nearest neighbor." ) },
    { gettext_noop( "Armageddon" ), 20, 0, 0, 16, 50, gettext_noop( "Holy terror strikes the battlefield, causing severe damage to all creatures." ) },
    { gettext_noop( "Elemental Storm" ), 15, 0, 0, 11, 25, gettext_noop( "Magical elements pour down on the battlefield, damaging all creatures." ) },
    { gettext_noop( "Meteor Shower" ), 15, 0, 0, 24, 25, gettext_noop( "A rain of rocks strikes an area of the battlefield, damaging all nearby creatures." ) },
    { gettext_noop( "Paralyze" ), 9, 0, 0, 20, 0, gettext_noop( "The targeted creatures are paralyzed, unable to move or retaliate." ) },
    { gettext_noop( "Hypnotize" ), 15, 0, 0, 37, 25,
      gettext_noop( "Brings a single enemy unit under your control if its hits are less than %{count} times the caster's spell power." ) },
    { gettext_noop( "Cold Ray" ), 6, 0, 0, 36, 20, gettext_noop( "Drains body heat from a single enemy unit." ) },
    { gettext_noop( "Cold Ring" ), 9, 0, 0, 35, 10, gettext_noop( "Drains body heat from all units surrounding the center point, but not including the center point." ) },
    { gettext_noop( "Disrupting Ray" ), 7, 0, 0, 34, 3, gettext_noop( "Reduces the defense rating of an enemy unit by three." ) },
    { gettext_noop( "Death Ripple" ), 6, 0, 0, 29, 5, gettext_noop( "Damages all living (non-undead) units in the battle." ) },
    { gettext_noop( "Death Wave" ), 10, 0, 0, 28, 10,
      gettext_noop( "Damages all living (non-undead) units in the battle. This spell is an improved version of Death Ripple." ) },
    { gettext_noop( "Dragon Slayer" ), 6, 0, 0, 32, 5, gettext_noop( "Greatly increases a unit's attack skill vs. Dragons." ) },
    { gettext_noop( "Blood Lust" ), 3, 0, 0, 27, 3, gettext_noop( "Increases a unit's attack skill." ) },
    { gettext_noop( "Animate Dead" ), 10, 0, 0, 25, 50, gettext_noop( "Resurrects creatures from a damaged or dead undead unit permanently." ) },
    { gettext_noop( "Mirror Image" ), 25, 0, 0, 26, 0,
      gettext_noop(
          "Creates an illusionary unit that duplicates one of your existing units. This illusionary unit does the same damages as the original, but will vanish if it takes any damage." ) },
    { gettext_noop( "Shield" ), 3, 0, 0, 15, 2,
      gettext_noop( "Halves damage received from ranged attacks for a single unit. Does not affect damage received from Turrets or Ballistae." ) },
    { gettext_noop( "Mass Shield" ), 7, 0, 0, 65, 0,
      gettext_noop( "Halves damage received from ranged attacks for all of your units. Does not affect damage received from Turrets or Ballistae." ) },
    { gettext_noop( "Summon Earth Elemental" ), 30, 0, 0, 56, 3, gettext_noop( "Summons Earth Elementals to fight for your army." ) },
    { gettext_noop( "Summon Air Elemental" ), 30, 0, 0, 57, 3, gettext_noop( "Summons Air Elementals to fight for your army." ) },
    { gettext_noop( "Summon Fire Elemental" ), 30, 0, 0, 58, 3, gettext_noop( "Summons Fire Elementals to fight for your army." ) },
    { gettext_noop( "Summon Water Elemental" ), 30, 0, 0, 59, 3, gettext_noop( "Summons Water Elementals to fight for your army." ) },
    { gettext_noop( "Earthquake" ), 15, 0, 0, 33, 0, gettext_noop( "Damages castle walls." ) },
    { gettext_noop( "View Mines" ), 1, 0, 0, 39, 0, gettext_noop( "Causes all mines across the land to become visible." ) },
    { gettext_noop( "View Resources" ), 1, 0, 0, 40, 0, gettext_noop( "Causes all resources across the land to become visible." ) },
    { gettext_noop( "View Artifacts" ), 2, 0, 0, 41, 0, gettext_noop( "Causes all artifacts across the land to become visible." ) },
    { gettext_noop( "View Towns" ), 2, 0, 0, 42, 0, gettext_noop( "Causes all towns and castles across the land to become visible." ) },
    { gettext_noop( "View Heroes" ), 2, 0, 0, 43, 0, gettext_noop( "Causes all Heroes across the land to become visible." ) },
    { gettext_noop( "View All" ), 3, 0, 0, 44, 0, gettext_noop( "Causes the entire land to become visible." ) },
    { gettext_noop( "Identify Hero" ), 3, 0, 0, 45, 0, gettext_noop( "Allows the caster to view detailed information on enemy Heroes." ) },
    { gettext_noop( "Summon Boat" ), 5, 0, 0, 46, 0,
      gettext_noop(
          "Summons the nearest unoccupied, friendly boat to an adjacent shore location. A friendly boat is one which you just built or were the most recent player to occupy." ) },
    { gettext_noop( "Dimension Door" ), 10, 225, 69, 47, 0, gettext_noop( "Allows the caster to magically transport to a nearby location." ) },
    { gettext_noop( "Town Gate" ), 10, 225, 69, 48, 0, gettext_noop( "Returns the caster to any town or castle currently owned." ) },
    { gettext_noop( "Town Portal" ), 20, 225, 69, 49, 0, gettext_noop( "Returns the hero to the town or castle of choice, provided it is controlled by you." ) },
    { gettext_noop( "Visions" ), 6, 0, 0, 50, 3, gettext_noop( "Visions predicts the likely outcome of an encounter with a neutral army camp." ) },
    { gettext_noop( "Haunt" ), 8, 0, 0, 51, 4,
      gettext_noop( "Haunts a mine you control with Ghosts. This mine stops producing resources. (If I can't keep it, nobody will!)" ) },
    { gettext_noop( "Set Earth Guardian" ), 15, 0, 0, 52, 4, gettext_noop( "Sets Earth Elementals to guard a mine against enemy armies." ) },
    { gettext_noop( "Set Air Guardian" ), 15, 0, 0, 53, 4, gettext_noop( "Sets Air Elementals to guard a mine against enemy armies." ) },
    { gettext_noop( "Set Fire Guardian" ), 15, 0, 0, 54, 4, gettext_noop( "Sets Fire Elementals to guard a mine against enemy armies." ) },
    { gettext_noop( "Set Water Guardian" ), 15, 0, 0, 55, 4, gettext_noop( "Sets Water Elementals to guard a mine against enemy armies." ) },
    { gettext_noop( "Random Spell" ), 0, 0, 0, 67, 0, gettext_noop( "Randomly selected spell of any level." ) },
    { gettext_noop( "Random 1st Level Spell" ), 0, 0, 0, 68, 0, gettext_noop( "Randomly selected 1st level spell." ) },
    { gettext_noop( "Random 2nd Level Spell" ), 0, 0, 0, 69, 0, gettext_noop( "Randomly selected 2nd level spell." ) },
    { gettext_noop( "Random 3rd Level Spell" ), 0, 0, 0, 70, 0, gettext_noop( "Randomly selected 3rd level spell." ) },
    { gettext_noop( "Random 4th Level Spell" ), 0, 0, 0, 71, 0, gettext_noop( "Randomly selected 4th level spell." ) },
    { gettext_noop( "Random 5th Level Spell" ), 0, 0, 0, 72, 0, gettext_noop( "Randomly selected 5th level spell." ) },
    { gettext_noop( "Petrification" ), 0, 0, 0, 66, 0,
      gettext_noop( "Turns the affected creature into stone. A petrified creature receives half damage from a direct attack." ) },
};

const char * Spell::GetName() const
{
    return _( spells[id].name );
}

const char * Spell::GetDescription() const
{
    return _( spells[id].description );
}

uint32_t Spell::movePoints() const
{
    return spells[id].movePoints;
}

uint32_t Spell::minMovePoints() const
{
    return spells[id].minMovePoints;
}

uint32_t Spell::spellPoints( const HeroBase * hero ) const
{
    if ( hero == nullptr ) {
        return spells[id].spellPoints;
    }

    fheroes2::ArtifactBonusType type = fheroes2::ArtifactBonusType::NONE;
    switch ( id ) {
    case BLESS:
    case MASSBLESS:
        type = fheroes2::ArtifactBonusType::BLESS_SPELL_COST_REDUCTION_PERCENT;
        break;
    case SUMMONEELEMENT:
    case SUMMONAELEMENT:
    case SUMMONFELEMENT:
    case SUMMONWELEMENT:
        type = fheroes2::ArtifactBonusType::SUMMONING_SPELL_COST_REDUCTION_PERCENT;
        break;
    case CURSE:
    case MASSCURSE:
        type = fheroes2::ArtifactBonusType::CURSE_SPELL_COST_REDUCTION_PERCENT;
        break;
    default:
        if ( isMindInfluence() ) {
            type = fheroes2::ArtifactBonusType::MIND_INFLUENCE_SPELL_COST_REDUCTION_PERCENT;
        }
        break;
    }

    if ( type == fheroes2::ArtifactBonusType::NONE ) {
        return spells[id].spellPoints;
    }

    int32_t spellCost = spells[id].spellPoints;

    const std::vector<int32_t> spellReductionPercentage = hero->GetBagArtifacts().getTotalArtifactMultipliedPercent( type );
    for ( const int32_t value : spellReductionPercentage ) {
        assert( value >= 0 && value <= 100 );
        spellCost = spellCost * ( 100 - value ) / 100;
    }

    if ( spellCost < 1 ) {
        return 1;
    }

    return static_cast<uint32_t>( spellCost );
}

double Spell::getStrategicValue( double armyStrength, uint32_t currentSpellPoints, int spellPower ) const
{
    const uint32_t spellCost = spellPoints();
    const uint32_t casts = spellCost ? std::min( 10U, currentSpellPoints / spellCost ) : 0;

    // use quadratic formula to diminish returns from subsequent spell casts, (up to x5 when spell has 10 uses)
    const double amountModifier = ( casts == 1 ) ? 1 : casts - ( 0.05 * casts * casts );

    if ( isAdventure() ) {
        // TODO: update this logic if you add support for more Adventure Map spells.
        switch ( id ) {
        case Spell::DIMENSIONDOOR:
            return 500 * amountModifier;
        case Spell::TOWNGATE:
        case Spell::TOWNPORTAL:
            return 250 * amountModifier;
        case Spell::VIEWALL:
            return 500;
        default:
            break;
        }

        return 0;
    }

    if ( isDamage() ) {
        // Benchmark for Lightning for 20 power * 20 knowledge (maximum uses) is 2500.0
        return amountModifier * Damage() * spellPower;
    }

    // These high impact spells can turn tide of battle
    if ( isResurrect() || isMassActions() || id == Spell::BLIND || id == Spell::PARALYZE ) {
        return armyStrength * 0.1 * amountModifier;
    }

    if ( isSummon() ) {
        // Summoning spells can be effective only per single turn as a summoned stack of monster could be killed within the same turn.
        // Also if the opponent targets the army's monsters and kill all of them the battle would be lost for this hero.
        return Monster( id ).GetMonsterStrength() * ExtraValue() * spellPower;
    }

    return armyStrength * 0.04 * amountModifier;
}

int Spell::Level() const
{
    switch ( id ) {
    case BLESS:
    case BLOODLUST:
    case CURE:
    case CURSE:
    case DISPEL:
    case HASTE:
    case ARROW:
    case SHIELD:
    case SLOW:
    case STONESKIN:

    case VIEWMINES:
    case VIEWRESOURCES:
        return 1;

    case BLIND:
    case COLDRAY:
    case DEATHRIPPLE:
    case DISRUPTINGRAY:
    case DRAGONSLAYER:
    case LIGHTNINGBOLT:
    case STEELSKIN:

    case HAUNT:
    case SUMMONBOAT:
    case VIEWARTIFACTS:
    case VISIONS:
        return 2;

    case ANIMATEDEAD:
    case ANTIMAGIC:
    case COLDRING:
    case DEATHWAVE:
    case EARTHQUAKE:
    case FIREBALL:
    case HOLYWORD:
    case MASSBLESS:
    case MASSCURSE:
    case MASSDISPEL:
    case MASSHASTE:
    case PARALYZE:
    case TELEPORT:

    case IDENTIFYHERO:
    case VIEWHEROES:
    case VIEWTOWNS:
        return 3;

    case BERSERKER:
    case CHAINLIGHTNING:
    case ELEMENTALSTORM:
    case FIREBLAST:
    case HOLYSHOUT:
    case MASSCURE:
    case MASSSHIELD:
    case MASSSLOW:
    case METEORSHOWER:
    case RESURRECT:

    case SETEGUARDIAN:
    case SETAGUARDIAN:
    case SETFGUARDIAN:
    case SETWGUARDIAN:
    case TOWNGATE:
    case VIEWALL:
        return 4;

    case ARMAGEDDON:
    case HYPNOTIZE:
    case MIRRORIMAGE:
    case RESURRECTTRUE:
    case SUMMONEELEMENT:
    case SUMMONAELEMENT:
    case SUMMONFELEMENT:
    case SUMMONWELEMENT:

    case DIMENSIONDOOR:
    case TOWNPORTAL:
        return 5;

    default:
        break;
    }

    return 0;
}

bool Spell::isCombat() const
{
    switch ( id ) {
    case NONE:
    case VIEWMINES:
    case VIEWRESOURCES:
    case VIEWARTIFACTS:
    case VIEWTOWNS:
    case VIEWHEROES:
    case VIEWALL:
    case IDENTIFYHERO:
    case SUMMONBOAT:
    case DIMENSIONDOOR:
    case TOWNGATE:
    case TOWNPORTAL:
    case VISIONS:
    case HAUNT:
    case SETEGUARDIAN:
    case SETAGUARDIAN:
    case SETFGUARDIAN:
    case SETWGUARDIAN:
        return false;
    default:
        break;
    }
    return true;
}

bool Spell::isGuardianType() const
{
    switch ( id ) {
    case HAUNT:
    case SETEGUARDIAN:
    case SETAGUARDIAN:
    case SETFGUARDIAN:
    case SETWGUARDIAN:
        return true;
    default:
        break;
    }

    return false;
}

uint32_t Spell::Damage() const
{
    switch ( id ) {
    case ARROW:
    case FIREBALL:
    case FIREBLAST:
    case LIGHTNINGBOLT:
    case COLDRING:
    case DEATHWAVE:
    case HOLYWORD:
    case CHAINLIGHTNING:
    case ARMAGEDDON:
    case ELEMENTALSTORM:
    case METEORSHOWER:
    case COLDRAY:
    case HOLYSHOUT:
    case DEATHRIPPLE:
        return spells[id].extraValue;

    default:
        break;
    }

    return 0;
}

bool Spell::isMindInfluence() const
{
    switch ( id ) {
    case BLIND:
    case PARALYZE:
    case BERSERKER:
    case HYPNOTIZE:
        return true;

    default:
        break;
    }

    return false;
}

uint32_t Spell::IndexSprite() const
{
    return spells[id].imageId;
}

uint32_t Spell::Restore() const
{
    switch ( id ) {
    case Spell::CURE:
    case Spell::MASSCURE:
        return spells[id].extraValue;

    default:
        break;
    }

    return 0;
}

uint32_t Spell::Resurrect() const
{
    switch ( id ) {
    case Spell::ANIMATEDEAD:
    case Spell::RESURRECT:
    case Spell::RESURRECTTRUE:
        return spells[id].extraValue;

    default:
        break;
    }

    return 0;
}

uint32_t Spell::ExtraValue() const
{
    return spells[id].extraValue;
}

uint32_t Spell::weightForRace( const int race ) const
{
    switch ( id ) {
    case Spell::HOLYWORD:
    case Spell::HOLYSHOUT:
        if ( race == Race::NECR ) {
            return 0;
        }
        break;

    case Spell::DEATHRIPPLE:
    case Spell::DEATHWAVE:
        if ( race != Race::NECR ) {
            return 0;
        }
        break;

    case Spell::SUMMONEELEMENT:
    case Spell::SUMMONAELEMENT:
    case Spell::SUMMONFELEMENT:
    case Spell::SUMMONWELEMENT:
    case Spell::TOWNPORTAL:
    case Spell::VISIONS:
    case Spell::HAUNT:
    case Spell::SETEGUARDIAN:
    case Spell::SETAGUARDIAN:
    case Spell::SETFGUARDIAN:
    case Spell::SETWGUARDIAN:
        return 0;

    default:
        break;
    }

    return 10;
}

Spell Spell::Rand( const int level, const bool isAdventure )
{
    std::vector<Spell> vec;
    vec.reserve( 15 );

    for ( int32_t spellId = NONE; spellId < PETRIFY; ++spellId ) {
        const Spell spell( spellId );

        if ( level != spell.Level() ) {
            continue;
        }

        if ( isAdventure ) {
            if ( !spell.isAdventure() ) {
                continue;
            }
        }
        else {
            if ( !spell.isCombat() ) {
                continue;
            }
        }

        vec.push_back( spell );
    }

    return !vec.empty() ? Rand::Get( vec ) : Spell::NONE;
}

Spell Spell::RandCombat( const int level )
{
    return Rand( level, false );
}

Spell Spell::RandAdventure( const int level )
{
    const Spell spell = Rand( level, true );

    return spell.isValid() ? spell : RandCombat( level );
}

std::vector<int> Spell::getAllSpellIdsSuitableForSpellBook( const int spellLevel /* = -1 */ )
{
    std::vector<int> result;
    result.reserve( SPELL_COUNT );

    for ( int spellId = 0; spellId < SPELL_COUNT; ++spellId ) {
        if ( spellId == NONE || ( spellId >= RANDOM && spellId <= PETRIFY ) ) {
            continue;
        }

        if ( spellLevel > 0 ) {
            const Spell spell( spellId );

            if ( spell.Level() != spellLevel ) {
                continue;
            }
        }

        result.push_back( spellId );
    }

    return result;
}

bool Spell::isUndeadOnly() const
{
    switch ( id ) {
    case ANIMATEDEAD:
    case HOLYWORD:
    case HOLYSHOUT:
        return true;

    default:
        break;
    }

    return false;
}

bool Spell::isAliveOnly() const
{
    switch ( id ) {
    case BLESS:
    case MASSBLESS:
    case CURSE:
    case MASSCURSE:
    case DEATHRIPPLE:
    case DEATHWAVE:
    case RESURRECT:
    case RESURRECTTRUE:
        return true;

    default:
        break;
    }

    return false;
}

bool Spell::isSingleTarget() const
{
    switch ( id ) {
    case LIGHTNINGBOLT:
    case TELEPORT:
    case CURE:
    case RESURRECT:
    case RESURRECTTRUE:
    case HASTE:
    case SLOW:
    case BLIND:
    case BLESS:
    case STONESKIN:
    case STEELSKIN:
    case CURSE:
    case ANTIMAGIC:
    case DISPEL:
    case ARROW:
    case BERSERKER:
    case PARALYZE:
    case HYPNOTIZE:
    case COLDRAY:
    case DISRUPTINGRAY:
    case DRAGONSLAYER:
    case BLOODLUST:
    case ANIMATEDEAD:
    case MIRRORIMAGE:
    case SHIELD:
        return true;

    default:
        break;
    }

    return false;
}

bool Spell::isApplyWithoutFocusObject() const
{
    if ( isMassActions() || isSummon() )
        return true;
    else
        switch ( id ) {
        case DEATHRIPPLE:
        case DEATHWAVE:
        case EARTHQUAKE:
        case HOLYWORD:
        case HOLYSHOUT:
        case ARMAGEDDON:
        case ELEMENTALSTORM:
            return true;

        default:
            break;
        }

    return false;
}

bool Spell::isSummon() const
{
    switch ( id ) {
    case SUMMONEELEMENT:
    case SUMMONAELEMENT:
    case SUMMONFELEMENT:
    case SUMMONWELEMENT:
        return true;

    default:
        break;
    }

    return false;
}

bool Spell::isEffectDispel() const
{
    switch ( id ) {
    case CURE:
    case MASSCURE:
    case DISPEL:
    case MASSDISPEL:
        return true;

    default:
        break;
    }

    return false;
}

bool Spell::isApplyToAnyTroops() const
{
    switch ( id ) {
    case DISPEL:
    case MASSDISPEL:
        return true;

    default:
        break;
    }

    return false;
}

bool Spell::isApplyToFriends() const
{
    switch ( id ) {
    case BLESS:
    case BLOODLUST:
    case CURE:
    case HASTE:
    case SHIELD:
    case STONESKIN:
    case DRAGONSLAYER:
    case STEELSKIN:
    case ANIMATEDEAD:
    case ANTIMAGIC:
    case TELEPORT:
    case RESURRECT:
    case MIRRORIMAGE:
    case RESURRECTTRUE:

    case MASSBLESS:
    case MASSCURE:
    case MASSHASTE:
    case MASSSHIELD:
        return true;

    default:
        break;
    }

    return false;
}

bool Spell::isMassActions() const
{
    switch ( id ) {
    case MASSCURE:
    case MASSHASTE:
    case MASSSLOW:
    case MASSBLESS:
    case MASSCURSE:
    case MASSDISPEL:
    case MASSSHIELD:
        return true;

    default:
        break;
    }

    return false;
}

bool Spell::isApplyToEnemies() const
{
    switch ( id ) {
    case MASSSLOW:
    case MASSCURSE:

    case CURSE:
    case ARROW:
    case SLOW:
    case BLIND:
    case COLDRAY:
    case DISRUPTINGRAY:
    case LIGHTNINGBOLT:
    case CHAINLIGHTNING:
    case PARALYZE:
    case BERSERKER:
    case HYPNOTIZE:
        return true;

    default:
        break;
    }

    return false;
}

int32_t Spell::CalculateDimensionDoorDistance()
{
    // original h2 variant
    return 14;
}

StreamBase & operator<<( StreamBase & msg, const Spell & spell )
{
    return msg << spell.id;
}

StreamBase & operator>>( StreamBase & msg, Spell & spell )
{
    return msg >> spell.id;
}
