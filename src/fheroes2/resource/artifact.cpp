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

#include "artifact.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <ostream>
#include <string>
#include <utility>

#include "agg_image.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "heroes.h"
#include "icn.h"
#include "logging.h"
#include "maps_fileinfo.h"
#include "mp2.h"
#include "rand.h"
#include "serialize.h"
#include "settings.h"
#include "skill.h"
#include "spell.h"
#include "spell_book.h"
#include "spell_storage.h"
#include "statusbar.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"

namespace
{
    const std::map<ArtifactSetData, std::vector<int32_t>> artifactSets
        = { { ArtifactSetData( Artifact::BATTLE_GARB, gettext_noop( "The three Anduran artifacts magically combine into one." ) ),
              { Artifact::HELMET_ANDURAN, Artifact::SWORD_ANDURAN, Artifact::BREASTPLATE_ANDURAN } } };

    // TODO: this array is not used during gameplay but only during new map loading.
    //       If we decide to add objects / events that generate a random artifact after a new game started
    //       then we will have problems.
    std::array<uint8_t, Artifact::ARTIFACT_COUNT> artifactGlobalStatus = { 0 };

    enum
    {
        ART_RNDDISABLED = 0x01,
        ART_RNDUSED = 0x02
    };

    void transferArtifactsByCondition( std::vector<Artifact> & artifacts, BagArtifacts & artifactBag, const std::function<bool( const Artifact & )> & condition )
    {
        for ( auto iter = artifacts.begin(); iter != artifacts.end(); ) {
            if ( condition( *iter ) ) {
                if ( !artifactBag.PushArtifact( *iter ) ) {
                    // The bag is full so no need to proceed further.
                    return;
                }

                iter = artifacts.erase( iter );
                continue;
            }

            ++iter;
        }
    }
}

const char * Artifact::GetName() const
{
    return _( fheroes2::getArtifactData( id ).name );
}

bool Artifact::isUltimate() const
{
    switch ( id ) {
    case ULTIMATE_BOOK:
    case ULTIMATE_SWORD:
    case ULTIMATE_CLOAK:
    case ULTIMATE_WAND:
    case ULTIMATE_SHIELD:
    case ULTIMATE_STAFF:
    case ULTIMATE_CROWN:
    case GOLDEN_GOOSE:
        return true;
    default:
        break;
    }

    return false;
}

int Artifact::LoyaltyLevel() const
{
    switch ( id ) {
    case MASTHEAD:
    case SPADE_NECROMANCY:
    case HEART_FIRE:
    case HEART_ICE:
        return ART_LEVEL_MINOR;

    case ARM_MARTYR:
    case HOLY_HAMMER:
    case LEGENDARY_SCEPTER:
    case STAFF_WIZARDRY:
    case SWORD_BREAKER:
    case CRYSTAL_BALL:
        return ART_LEVEL_MAJOR;

    case SPELL_SCROLL:
    case BROACH_SHIELDING:
    case SWORD_ANDURAN:
    case BREASTPLATE_ANDURAN:
    case BATTLE_GARB:
    case HELMET_ANDURAN:
    case SPHERE_NEGATION:
        return ART_NORANDOM;

    default:
        break;
    }

    return ART_NONE;
}

int Artifact::Level() const
{
    if ( isUltimate() ) {
        return ART_ULTIMATE;
    }

    switch ( id ) {
    case MEDAL_VALOR:
    case MEDAL_COURAGE:
    case MEDAL_HONOR:
    case MEDAL_DISTINCTION:
    case THUNDER_MACE:
    case ARMORED_GAUNTLETS:
    case DEFENDER_HELM:
    case GIANT_FLAIL:
    case RABBIT_FOOT:
    case GOLDEN_HORSESHOE:
    case GAMBLER_LUCKY_COIN:
    case FOUR_LEAF_CLOVER:
    case ENCHANTED_HOURGLASS:
    case ICE_CLOAK:
    case FIRE_CLOAK:
    case LIGHTNING_HELM:
    case SNAKE_RING:
    case HOLY_PENDANT:
    case PENDANT_FREE_WILL:
    case PENDANT_LIFE:
    case SERENITY_PENDANT:
    case SEEING_EYE_PENDANT:
    case KINETIC_PENDANT:
    case PENDANT_DEATH:
    case GOLDEN_BOW:
    case TELESCOPE:
    case STATESMAN_QUILL:
        return ART_LEVEL_TREASURE;

    case CASTER_BRACELET:
    case MAGE_RING:
    case STEALTH_SHIELD:
    case POWER_AXE:
    case MINOR_SCROLL:
    case ENDLESS_PURSE_GOLD:
    case SAILORS_ASTROLABE_MOBILITY:
    case EVIL_EYE:
    case GOLD_WATCH:
    case SKULLCAP:
    case EVERCOLD_ICICLE:
    case EVERHOT_LAVA_ROCK:
    case LIGHTNING_ROD:
    case ANKH:
    case BOOK_ELEMENTS:
    case ELEMENTAL_RING:
    case POWER_RING:
    case AMMO_CART:
    case ENDLESS_CORD_WOOD:
    case ENDLESS_CART_ORE:
    case SPIKED_HELM:
    case WHITE_PEARL:
        return ART_LEVEL_MINOR;

    case ARCANE_NECKLACE:
    case WITCHES_BROACH:
    case BALLISTA:
    case DRAGON_SWORD:
    case DIVINE_BREASTPLATE:
    case MAJOR_SCROLL:
    case SUPERIOR_SCROLL:
    case FOREMOST_SCROLL:
    case ENDLESS_SACK_GOLD:
    case ENDLESS_BAG_GOLD:
    case NOMAD_BOOTS_MOBILITY:
    case TRAVELER_BOOTS_MOBILITY:
    case TRUE_COMPASS_MOBILITY:
    case WAND_NEGATION:
    case WIZARD_HAT:
    case ENDLESS_POUCH_SULFUR:
    case ENDLESS_VIAL_MERCURY:
    case ENDLESS_POUCH_GEMS:
    case ENDLESS_POUCH_CRYSTAL:
    case SPIKED_SHIELD:
    case BLACK_PEARL:
        return ART_LEVEL_MAJOR;

    // no random
    case MAGIC_BOOK:
    case FIZBIN_MISFORTUNE:
    case TAX_LIEN:
    case HIDEOUS_MASK:
        return ART_NORANDOM;

    // price loyalty
    case SPELL_SCROLL:
    case ARM_MARTYR:
    case BREASTPLATE_ANDURAN:
    case BROACH_SHIELDING:
    case BATTLE_GARB:
    case CRYSTAL_BALL:
    case HELMET_ANDURAN:
    case HOLY_HAMMER:
    case LEGENDARY_SCEPTER:
    case MASTHEAD:
    case SPHERE_NEGATION:
    case STAFF_WIZARDRY:
    case SWORD_BREAKER:
    case SWORD_ANDURAN:
    case SPADE_NECROMANCY:
    case HEART_FIRE:
    case HEART_ICE: {
        const GameVersion version = Settings::Get().getCurrentMapInfo().version;
        return ( version == GameVersion::PRICE_OF_LOYALTY || version == GameVersion::RESURRECTION ) ? ART_LOYALTY | LoyaltyLevel() : ART_LOYALTY;
    }

    default:
        break;
    }

    return ART_NONE;
}

double Artifact::getArtifactValue() const
{
    double artifactValue = 0;
    const fheroes2::ArtifactData & data = fheroes2::getArtifactData( id );
    const std::vector<fheroes2::ArtifactBonus> & bonuses = data.bonuses;
    const std::vector<fheroes2::ArtifactCurse> & curses = data.curses;

    for ( const fheroes2::ArtifactBonus & bonus : bonuses ) {
        switch ( bonus.type ) {
        case fheroes2::ArtifactBonusType::GOLD_INCOME:
            artifactValue += static_cast<double>( bonus.value ) / 800.0;
            break;
        case fheroes2::ArtifactBonusType::SEA_MOBILITY:
            artifactValue += static_cast<double>( bonus.value ) / 500.0;
            break;
        case fheroes2::ArtifactBonusType::LAND_MOBILITY:
            artifactValue += static_cast<double>( bonus.value ) / 200.0;
            break;
        case fheroes2::ArtifactBonusType::CURSE_SPELL_COST_REDUCTION_PERCENT:
        case fheroes2::ArtifactBonusType::BLESS_SPELL_COST_REDUCTION_PERCENT:
        case fheroes2::ArtifactBonusType::SUMMONING_SPELL_COST_REDUCTION_PERCENT:
        case fheroes2::ArtifactBonusType::MIND_INFLUENCE_SPELL_COST_REDUCTION_PERCENT:
        case fheroes2::ArtifactBonusType::COLD_SPELL_DAMAGE_REDUCTION_PERCENT:
        case fheroes2::ArtifactBonusType::FIRE_SPELL_DAMAGE_REDUCTION_PERCENT:
        case fheroes2::ArtifactBonusType::LIGHTNING_SPELL_DAMAGE_REDUCTION_PERCENT:
        case fheroes2::ArtifactBonusType::ELEMENTAL_SPELL_DAMAGE_REDUCTION_PERCENT:
        case fheroes2::ArtifactBonusType::HYPNOTIZE_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
        case fheroes2::ArtifactBonusType::COLD_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
        case fheroes2::ArtifactBonusType::FIRE_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
        case fheroes2::ArtifactBonusType::LIGHTNING_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
        case fheroes2::ArtifactBonusType::RESURRECT_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
        case fheroes2::ArtifactBonusType::SUMMONING_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
            artifactValue += static_cast<double>( bonus.value ) / 50.0;
            break;
        case fheroes2::ArtifactBonusType::NECROMANCY_SKILL:
        case fheroes2::ArtifactBonusType::SURRENDER_COST_REDUCTION_PERCENT:
            artifactValue += static_cast<double>( bonus.value ) / 10.0;
            break;
        case fheroes2::ArtifactBonusType::EVERY_COMBAT_SPELL_DURATION:
        case fheroes2::ArtifactBonusType::SPELL_POINTS_DAILY_GENERATION:
            artifactValue += static_cast<double>( bonus.value ) / 2.0;
            break;
        case fheroes2::ArtifactBonusType::CURSE_SPELL_IMMUNITY:
        case fheroes2::ArtifactBonusType::HYPNOTIZE_SPELL_IMMUNITY:
        case fheroes2::ArtifactBonusType::DEATH_SPELL_IMMUNITY:
        case fheroes2::ArtifactBonusType::BERSERK_SPELL_IMMUNITY:
        case fheroes2::ArtifactBonusType::BLIND_SPELL_IMMUNITY:
        case fheroes2::ArtifactBonusType::PARALYZE_SPELL_IMMUNITY:
        case fheroes2::ArtifactBonusType::HOLY_SPELL_IMMUNITY:
        case fheroes2::ArtifactBonusType::DISPEL_SPELL_IMMUNITY:
        case fheroes2::ArtifactBonusType::ENDLESS_AMMUNITION:
        case fheroes2::ArtifactBonusType::NO_SHOOTING_PENALTY:
        case fheroes2::ArtifactBonusType::VIEW_MONSTER_INFORMATION:
        case fheroes2::ArtifactBonusType::ADD_SPELL:
        case fheroes2::ArtifactBonusType::EXTRA_CATAPULT_SHOTS:
            artifactValue += 1;
            break;
        case fheroes2::ArtifactBonusType::MAXIMUM_MORALE:
        case fheroes2::ArtifactBonusType::DISABLE_ALL_SPELL_COMBAT_CASTING:
        case fheroes2::ArtifactBonusType::MAXIMUM_LUCK:
            artifactValue += 3;
            break;
        case fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL:
        case fheroes2::ArtifactBonusType::ATTACK_SKILL:
        case fheroes2::ArtifactBonusType::DEFENCE_SKILL:
        case fheroes2::ArtifactBonusType::SPELL_POWER_SKILL:
        case fheroes2::ArtifactBonusType::MORALE:
        case fheroes2::ArtifactBonusType::LUCK:
        case fheroes2::ArtifactBonusType::AREA_REVEAL_DISTANCE:
        case fheroes2::ArtifactBonusType::CRYSTAL_INCOME:
        case fheroes2::ArtifactBonusType::MERCURY_INCOME:
        case fheroes2::ArtifactBonusType::ORE_INCOME:
        case fheroes2::ArtifactBonusType::GEMS_INCOME:
        case fheroes2::ArtifactBonusType::WOOD_INCOME:
        case fheroes2::ArtifactBonusType::SULFUR_INCOME:
        case fheroes2::ArtifactBonusType::SEA_BATTLE_LUCK_BOOST:
        case fheroes2::ArtifactBonusType::SEA_BATTLE_MORALE_BOOST:
            artifactValue += bonus.value;
            break;
        case fheroes2::ArtifactBonusType::NONE:
            break;
        default:
            // Did you add a new artifact bonus? Add your logic here.
            assert( 0 );
            break;
        }
    }

    for ( const fheroes2::ArtifactCurse & curse : curses ) {
        switch ( curse.type ) {
        case fheroes2::ArtifactCurseType::GOLD_PENALTY:
            artifactValue -= static_cast<double>( curse.value ) / 200.0;
            break;
        case fheroes2::ArtifactCurseType::COLD_SPELL_EXTRA_DAMAGE_PERCENT:
        case fheroes2::ArtifactCurseType::FIRE_SPELL_EXTRA_DAMAGE_PERCENT:
            artifactValue -= static_cast<double>( curse.value ) / 100.0;
            break;
        case fheroes2::ArtifactCurseType::NO_JOINING_ARMIES:
        case fheroes2::ArtifactCurseType::UNDEAD_MORALE_PENALTY:
            artifactValue -= 1;
            break;
        case fheroes2::ArtifactCurseType::MORALE:
        case fheroes2::ArtifactCurseType::SPELL_POWER_SKILL:
            artifactValue -= curse.value;
            break;
        default:
            // Did you add a new artifact curse? Add your logic here.
            assert( 0 );
            break;
        }
    }

    return artifactValue;
}

void Artifact::SetSpell( const int v )
{
    if ( id != SPELL_SCROLL ) {
        // This method must be called only for Spell Scroll artifact.
        assert( 0 );
        return;
    }

    const bool adv = Rand::Get( 1 ) != 0;

    switch ( v ) {
    case Spell::RANDOM:
        ext = Spell::Rand( Rand::Get( 1, 5 ), adv ).GetID();
        break;
    case Spell::RANDOM1:
        ext = Spell::Rand( 1, adv ).GetID();
        break;
    case Spell::RANDOM2:
        ext = Spell::Rand( 2, adv ).GetID();
        break;
    case Spell::RANDOM3:
        ext = Spell::Rand( 3, adv ).GetID();
        break;
    case Spell::RANDOM4:
        ext = Spell::Rand( 4, adv ).GetID();
        break;
    case Spell::RANDOM5:
        ext = Spell::Rand( 5, adv ).GetID();
        break;
    default:
        ext = v;
        break;
    }
}

int32_t Artifact::getSpellId() const
{
    const std::vector<fheroes2::ArtifactBonus> & bonuses = fheroes2::getArtifactData( id ).bonuses;
    for ( const fheroes2::ArtifactBonus & bonus : bonuses ) {
        if ( bonus.type == fheroes2::ArtifactBonusType::ADD_SPELL ) {
            int32_t spellId = bonus.value;
            if ( spellId == Spell::NONE ) {
                spellId = ext;
            }
            assert( spellId > Spell::NONE && spellId < Spell::SPELL_COUNT );
            return spellId;
        }
    }

    return Spell::NONE;
}

int Artifact::Rand( ArtLevel lvl )
{
    std::vector<int> v;
    v.reserve( 25 );

    // if possibly: make unique on map
    for ( int art = UNKNOWN + 1; art < ARTIFACT_COUNT; ++art ) {
        const Artifact artifact{ art };

        if ( artifact.isValid() && ( lvl & artifact.Level() ) && !( artifactGlobalStatus[art] & ART_RNDDISABLED ) && !( artifactGlobalStatus[art] & ART_RNDUSED ) ) {
            v.push_back( art );
        }
    }

    if ( v.empty() ) {
        for ( int art = UNKNOWN + 1; art < ARTIFACT_COUNT; ++art ) {
            const Artifact artifact{ art };

            if ( artifact.isValid() && ( lvl & artifact.Level() ) && !( artifactGlobalStatus[art] & ART_RNDDISABLED ) ) {
                v.push_back( art );
            }
        }
    }

    int res = !v.empty() ? Rand::Get( v ) : Artifact::UNKNOWN;
    artifactGlobalStatus[res] |= ART_RNDUSED;

    return res;
}

Artifact Artifact::getArtifactFromMapSpriteIndex( const uint32_t index )
{
    // Add 1 to all values to properly convert from the old map format.
    if ( ( index < 162 ) || ( Settings::Get().isPriceOfLoyaltySupported() && index > 171 && index < 206 ) ) {
        return { static_cast<int32_t>( index - 1 ) / 2 + 1 };
    }

    // The original game does not have the Magic Book adventure map sprite. But it uses the ID that is taken for "Dummy" sprite.
    // The Resurrection map format allows to place a Magic Book and it has its own sprite that does not correlate with the original Magic Book artifact ID.
    if ( Settings::Get().getCurrentMapInfo().version == GameVersion::RESURRECTION && index == 207 ) {
        return { MAGIC_BOOK };
    }

    if ( index == 163 ) {
        return { Rand( ART_LEVEL_ALL_NORMAL ) };
    }

    if ( index == 164 ) {
        return { Rand( ART_ULTIMATE ) };
    }

    if ( index == 167 ) {
        return { Rand( ART_LEVEL_TREASURE ) };
    }

    if ( index == 169 ) {
        return { Rand( ART_LEVEL_MINOR ) };
    }

    if ( index == 171 ) {
        return { Rand( ART_LEVEL_MAJOR ) };
    }

    DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown Artifact object index: " << index )

    return { UNKNOWN };
}

const char * Artifact::getDiscoveryDescription( const Artifact & art )
{
    return _( fheroes2::getArtifactData( art.GetID() ).discoveryEventDescription );
}

OStreamBase & operator<<( OStreamBase & stream, const Artifact & art )
{
    return stream << art.id << art.ext;
}

IStreamBase & operator>>( IStreamBase & stream, Artifact & art )
{
    return stream >> art.id >> art.ext;
}

BagArtifacts::BagArtifacts()
    : std::vector<Artifact>( maxCapacity, Artifact::UNKNOWN )
{}

bool BagArtifacts::ContainSpell( const int spellId ) const
{
    assert( spellId > Spell::NONE && spellId < Spell::SPELL_COUNT );

    for ( const Artifact & artifact : *this ) {
        if ( artifact.getSpellId() == spellId ) {
            return true;
        }
    }

    return false;
}

bool BagArtifacts::isPresentArtifact( const Artifact & art ) const
{
    return end() != std::find( begin(), end(), art );
}

bool BagArtifacts::isArtifactBonusPresent( const fheroes2::ArtifactBonusType type ) const
{
    for ( const Artifact & artifact : *this ) {
        const std::vector<fheroes2::ArtifactBonus> & bonuses = fheroes2::getArtifactData( artifact.GetID() ).bonuses;
        if ( std::find( bonuses.begin(), bonuses.end(), fheroes2::ArtifactBonus( type ) ) != bonuses.end() ) {
            return true;
        }
    }

    return false;
}

bool BagArtifacts::isArtifactCursePresent( const fheroes2::ArtifactCurseType type ) const
{
    for ( const Artifact & artifact : *this ) {
        const std::vector<fheroes2::ArtifactCurse> & curses = fheroes2::getArtifactData( artifact.GetID() ).curses;
        if ( std::find( curses.begin(), curses.end(), fheroes2::ArtifactCurse( type ) ) != curses.end() ) {
            return true;
        }
    }

    return false;
}

int32_t BagArtifacts::getTotalArtifactEffectValue( const fheroes2::ArtifactBonusType bonus ) const
{
    // If this assertion blows up you're calling the method for a wrong type.
    assert( !fheroes2::isBonusMultiplied( bonus ) && !fheroes2::isBonusUnique( bonus ) );

    int32_t totalValue = 0;

    if ( fheroes2::isBonusCumulative( bonus ) ) {
        for ( const Artifact & artifact : *this ) {
            const std::vector<fheroes2::ArtifactBonus> & bonuses = fheroes2::getArtifactData( artifact.GetID() ).bonuses;
            auto bonusIter = std::find( bonuses.begin(), bonuses.end(), fheroes2::ArtifactBonus( bonus ) );
            if ( bonusIter != bonuses.end() ) {
                totalValue += bonusIter->value;
            }
        }
    }
    else {
        std::set<int> usedArtifactIds;
        for ( const Artifact & artifact : *this ) {
            const int artifactId = artifact.GetID();
            if ( !usedArtifactIds.insert( artifactId ).second ) {
                // The artifact is present in multiple copies.
                continue;
            }

            const std::vector<fheroes2::ArtifactBonus> & bonuses = fheroes2::getArtifactData( artifactId ).bonuses;
            auto bonusIter = std::find( bonuses.begin(), bonuses.end(), fheroes2::ArtifactBonus( bonus ) );
            if ( bonusIter != bonuses.end() ) {
                totalValue += bonusIter->value;
            }
        }
    }

    return totalValue;
}

int32_t BagArtifacts::getTotalArtifactEffectValue( const fheroes2::ArtifactBonusType bonus, std::string & description ) const
{
    // If this assertion blows up you're calling the method for a wrong type.
    assert( !fheroes2::isBonusMultiplied( bonus ) && !fheroes2::isBonusUnique( bonus ) );

    int32_t totalValue = 0;

    if ( fheroes2::isBonusCumulative( bonus ) ) {
        std::map<int, int> artifactValuePerId;
        for ( const Artifact & artifact : *this ) {
            const int artifactId = artifact.GetID();

            const std::vector<fheroes2::ArtifactBonus> & bonuses = fheroes2::getArtifactData( artifactId ).bonuses;
            auto bonusIter = std::find( bonuses.begin(), bonuses.end(), fheroes2::ArtifactBonus( bonus ) );
            if ( bonusIter != bonuses.end() ) {
                totalValue += bonusIter->value;
                artifactValuePerId[artifactId] += bonusIter->value;
            }
        }

        for ( const auto & artifactInfo : artifactValuePerId ) {
            description += Artifact( artifactInfo.first ).GetName();
            description += " +";

            description += std::to_string( artifactInfo.second );
            description += '\n';
        }
    }
    else {
        std::set<int> usedArtifactIds;
        for ( const Artifact & artifact : *this ) {
            const int artifactId = artifact.GetID();
            if ( !usedArtifactIds.insert( artifactId ).second ) {
                // The artifact is present in multiple copies.
                continue;
            }

            const std::vector<fheroes2::ArtifactBonus> & bonuses = fheroes2::getArtifactData( artifactId ).bonuses;
            auto bonusIter = std::find( bonuses.begin(), bonuses.end(), fheroes2::ArtifactBonus( bonus ) );
            if ( bonusIter != bonuses.end() ) {
                totalValue += bonusIter->value;

                description += artifact.GetName();
                description += " +"; // to show a positive value.

                description += std::to_string( bonusIter->value );
                description += '\n';
            }
        }
    }

    return totalValue;
}

int32_t BagArtifacts::getTotalArtifactEffectValue( const fheroes2::ArtifactCurseType curse ) const
{
    // If this assertion blows up you're calling the method for a wrong type.
    assert( !fheroes2::isCurseMultiplied( curse ) && !fheroes2::isCurseUnique( curse ) );

    int32_t totalValue = 0;

    if ( fheroes2::isCurseCumulative( curse ) ) {
        for ( const Artifact & artifact : *this ) {
            const std::vector<fheroes2::ArtifactCurse> & curses = fheroes2::getArtifactData( artifact.GetID() ).curses;
            auto curseIter = std::find( curses.begin(), curses.end(), fheroes2::ArtifactCurse( curse ) );
            if ( curseIter != curses.end() ) {
                totalValue += curseIter->value;
            }
        }
    }
    else {
        std::set<int> usedArtifactIds;
        for ( const Artifact & artifact : *this ) {
            const int artifactId = artifact.GetID();
            if ( !usedArtifactIds.insert( artifactId ).second ) {
                // The artifact is present in multiple copies.
                continue;
            }

            const std::vector<fheroes2::ArtifactCurse> & curses = fheroes2::getArtifactData( artifactId ).curses;
            auto curseIter = std::find( curses.begin(), curses.end(), fheroes2::ArtifactCurse( curse ) );
            if ( curseIter != curses.end() ) {
                totalValue += curseIter->value;
            }
        }
    }

    return totalValue;
}

int32_t BagArtifacts::getTotalArtifactEffectValue( const fheroes2::ArtifactCurseType curse, std::string & description ) const
{
    // If this assertion blows up you're calling the method for a wrong type.
    assert( !fheroes2::isCurseMultiplied( curse ) && !fheroes2::isCurseUnique( curse ) );

    int32_t totalValue = 0;

    if ( fheroes2::isCurseCumulative( curse ) ) {
        std::map<int, int> artifactValuePerId;
        for ( const Artifact & artifact : *this ) {
            const int artifactId = artifact.GetID();

            const std::vector<fheroes2::ArtifactCurse> & curses = fheroes2::getArtifactData( artifactId ).curses;
            auto curseIter = std::find( curses.begin(), curses.end(), fheroes2::ArtifactCurse( curse ) );
            if ( curseIter != curses.end() ) {
                totalValue += curseIter->value;
                artifactValuePerId[artifactId] += curseIter->value;
            }
        }

        for ( const auto & artifactInfo : artifactValuePerId ) {
            description += Artifact( artifactInfo.first ).GetName();
            description += " -";

            description += std::to_string( artifactInfo.second );
            description += '\n';
        }
    }
    else {
        std::set<int> usedArtifactIds;
        for ( const Artifact & artifact : *this ) {
            const int artifactId = artifact.GetID();
            if ( !usedArtifactIds.insert( artifactId ).second ) {
                // The artifact is present in multiple copies.
                continue;
            }

            const std::vector<fheroes2::ArtifactCurse> & curses = fheroes2::getArtifactData( artifactId ).curses;
            auto curseIter = std::find( curses.begin(), curses.end(), fheroes2::ArtifactCurse( curse ) );
            if ( curseIter != curses.end() ) {
                totalValue += curseIter->value;

                description += artifact.GetName();
                description += " -";

                description += std::to_string( curseIter->value );
                description += '\n';
            }
        }
    }

    return totalValue;
}

std::vector<int32_t> BagArtifacts::getTotalArtifactMultipliedPercent( const fheroes2::ArtifactBonusType bonus ) const
{
    if ( !fheroes2::isBonusMultiplied( bonus ) ) {
        // You are calling this method for a wrong bonus type!
        assert( 0 );
        return {};
    }

    std::vector<int32_t> values;

    std::set<int> usedArtifactIds;
    for ( const Artifact & artifact : *this ) {
        const int artifactId = artifact.GetID();
        if ( !usedArtifactIds.insert( artifactId ).second ) {
            // The artifact is present in multiple copies.
            continue;
        }

        const std::vector<fheroes2::ArtifactBonus> & bonuses = fheroes2::getArtifactData( artifactId ).bonuses;
        auto bonusIter = std::find( bonuses.begin(), bonuses.end(), fheroes2::ArtifactBonus( bonus ) );
        if ( bonusIter != bonuses.end() ) {
            values.emplace_back( bonusIter->value );
        }
    }

    return values;
}

std::vector<int32_t> BagArtifacts::getTotalArtifactMultipliedPercent( const fheroes2::ArtifactCurseType curse ) const
{
    if ( !fheroes2::isCurseMultiplied( curse ) ) {
        // You are calling this method for a wrong curse type!
        assert( 0 );
        return {};
    }

    std::vector<int32_t> values;

    std::set<int> usedArtifactIds;
    for ( const Artifact & artifact : *this ) {
        const int artifactId = artifact.GetID();
        if ( !usedArtifactIds.insert( artifactId ).second ) {
            // The artifact is present in multiple copies.
            continue;
        }

        const std::vector<fheroes2::ArtifactCurse> & curses = fheroes2::getArtifactData( artifactId ).curses;
        auto curseIter = std::find( curses.begin(), curses.end(), fheroes2::ArtifactCurse( curse ) );
        if ( curseIter != curses.end() ) {
            values.emplace_back( curseIter->value );
        }
    }

    return values;
}

Artifact BagArtifacts::getFirstArtifactWithBonus( const fheroes2::ArtifactBonusType bonus ) const
{
    for ( const Artifact & artifact : *this ) {
        const std::vector<fheroes2::ArtifactBonus> & bonuses = fheroes2::getArtifactData( artifact.GetID() ).bonuses;
        auto bonusIter = std::find( bonuses.begin(), bonuses.end(), fheroes2::ArtifactBonus( bonus ) );
        if ( bonusIter != bonuses.end() ) {
            return artifact;
        }
    }

    return { Artifact::UNKNOWN };
}

Artifact BagArtifacts::getFirstArtifactWithCurse( const fheroes2::ArtifactCurseType curse ) const
{
    for ( const Artifact & artifact : *this ) {
        const std::vector<fheroes2::ArtifactCurse> & curses = fheroes2::getArtifactData( artifact.GetID() ).curses;
        auto curseIter = std::find( curses.begin(), curses.end(), fheroes2::ArtifactCurse( curse ) );
        if ( curseIter != curses.end() ) {
            return artifact;
        }
    }

    return { Artifact::UNKNOWN };
}

bool BagArtifacts::PushArtifact( const Artifact & art )
{
    if ( !art.isValid() ) {
        // Why an invalid artifact is being pushed?
        assert( 0 );
        return false;
    }

    // There should not be more than one Magic Book in the artifact bag at a time.
    if ( art.GetID() == Artifact::MAGIC_BOOK && isPresentArtifact( art ) ) {
        return false;
    }

    const auto firstEmptySlotIter = std::find( begin(), end(), Artifact( Artifact::UNKNOWN ) );
    if ( firstEmptySlotIter == end() ) {
        return false;
    }

    // If the artifact to add is not a Magic Book, then just use the first empty slot.
    if ( art.GetID() != Artifact::MAGIC_BOOK ) {
        *firstEmptySlotIter = art;

        return true;
    }

    // Otherwise, we should first shift the existing artifacts (if any) from left to right...
    std::move_backward( begin(), firstEmptySlotIter, std::next( firstEmptySlotIter ) );

    // ... and then put the Magic Book to the first slot of the artifact bag.
    front() = art;

    return true;
}

void BagArtifacts::RemoveArtifact( const Artifact & art )
{
    iterator it = std::find( begin(), end(), art );
    if ( it == end() ) {
        return;
    }

    it->Reset();
}

bool BagArtifacts::isFull() const
{
    return end() == std::find( begin(), end(), Artifact( Artifact::UNKNOWN ) );
}

uint32_t BagArtifacts::CountArtifacts() const
{
    // no way that we have more than 4 billion artifacts so static_cast is totally valid
    return static_cast<uint32_t>( std::count_if( begin(), end(), []( const Artifact & art ) { return art.isValid(); } ) );
}

double BagArtifacts::getArtifactValue() const
{
    double result = 0;
    for ( const Artifact & art : *this ) {
        if ( art.isValid() )
            result += art.getArtifactValue();
    }

    return result;
}

void BagArtifacts::exchangeArtifacts( Heroes & taker, Heroes & giver )
{
    BagArtifacts & takerBag = taker.GetBagArtifacts();
    BagArtifacts & giverBag = giver.GetBagArtifacts();

    std::vector<Artifact> combined;

    for ( BagArtifacts & bag : std::array<std::reference_wrapper<BagArtifacts>, 2>{ takerBag, giverBag } ) {
        for ( Artifact & artifact : bag ) {
            if ( !artifact.isValid() || artifact.GetID() == Artifact::MAGIC_BOOK ) {
                continue;
            }

            combined.push_back( artifact );

            artifact.Reset();
        }
    }

    const auto isPureCursedArtifact = []( const Artifact & artifact ) {
        const fheroes2::ArtifactData & data = fheroes2::getArtifactData( artifact.GetID() );
        return !data.curses.empty() && data.bonuses.empty();
    };

    // Pure cursed artifacts (artifacts with no bonuses but only curses) should definitely go to another bag.
    transferArtifactsByCondition( combined, giverBag, isPureCursedArtifact );

    if ( !taker.HasSecondarySkill( Skill::Secondary::NECROMANCY ) && giver.HasSecondarySkill( Skill::Secondary::NECROMANCY ) ) {
        const auto isNecromancyArtifact = []( const Artifact & artifact ) {
            const fheroes2::ArtifactData & data = fheroes2::getArtifactData( artifact.GetID() );
            if ( data.bonuses.empty() ) {
                return false;
            }

            for ( const fheroes2::ArtifactBonus & bonus : data.bonuses ) {
                if ( bonus.type != fheroes2::ArtifactBonusType::NECROMANCY_SKILL ) {
                    return false;
                }
            }

            return true;
        };

        // Giver hero has Necromancy skill so it would be more useful for him to use Necromancy related artifacts.
        transferArtifactsByCondition( combined, giverBag, isNecromancyArtifact );
    }

    // Scrolls are effective if they contain spells which are not present in the book.
    if ( taker.HaveSpellBook() ) {
        const auto isScrollSpellDuplicated = [&taker]( const Artifact & artifact ) {
            const fheroes2::ArtifactData & data = fheroes2::getArtifactData( artifact.GetID() );
            if ( data.bonuses.empty() ) {
                return false;
            }

            for ( const fheroes2::ArtifactBonus & bonus : data.bonuses ) {
                if ( bonus.type != fheroes2::ArtifactBonusType::ADD_SPELL ) {
                    return false;
                }
            }

            const SpellStorage & magicBookSpells = taker.getMagicBookSpells();
            const int32_t spellId = artifact.getSpellId();
            assert( spellId != Spell::NONE );

            return std::find( magicBookSpells.begin(), magicBookSpells.end(), Spell( spellId ) ) != magicBookSpells.end();
        };

        transferArtifactsByCondition( combined, giverBag, isScrollSpellDuplicated );
    }

    // A unique artifact is an artifact with no curses and all its bonuses are unique.
    const auto isUniqueArtifact = []( const Artifact & artifact ) {
        const fheroes2::ArtifactData & data = fheroes2::getArtifactData( artifact.GetID() );
        if ( !data.curses.empty() ) {
            return false;
        }

        for ( const fheroes2::ArtifactBonus & bonus : data.bonuses ) {
            if ( fheroes2::isBonusCumulative( bonus.type ) ) {
                return false;
            }
        }

        return true;
    };

    // Search for copies of unique artifacts. All copies of unique artifacts are useless.
    for ( auto mainIter = combined.begin(); mainIter != combined.end(); ++mainIter ) {
        if ( !isUniqueArtifact( *mainIter ) ) {
            continue;
        }

        for ( auto iter = mainIter + 1; iter != combined.end(); ) {
            if ( *iter != *mainIter ) {
                ++iter;
                continue;
            }

            // Scrolls are considered as unique artifacts but their internal value might be different.
            // If they contain different spells then we should not interpret them as the same.
            if ( ( iter->GetID() == Artifact::SPELL_SCROLL ) && ( iter->getSpellId() != mainIter->getSpellId() ) ) {
                ++iter;
                continue;
            }

            if ( !giverBag.PushArtifact( *iter ) ) {
                // The bag is full. No need to proceed further.
                break;
            }

            iter = combined.erase( iter );
        }
    }

    // Sort artifacts by value from lowest to highest since we pick them from the end of container.
    std::sort( combined.begin(), combined.end(), []( const Artifact & left, const Artifact & right ) { return left.getArtifactValue() < right.getArtifactValue(); } );

    // Taker gets the best artifacts. If he doesn't have a Spell Book yet, we'll try to leave a place for it.
    for ( size_t i = 1; i < takerBag.size(); ++i ) {
        if ( combined.empty() || !takerBag.PushArtifact( combined.back() ) ) {
            break;
        }

        combined.pop_back();
    }

    // If there are more artifacts left than there are free slots in the giver's bag, then this means that we failed to leave room for the Spell Book in the taker's bag.
    {
        const ptrdiff_t emptySlotsCount = std::count_if( giverBag.begin(), giverBag.end(), []( const Artifact & artifact ) { return !artifact.isValid(); } );
        assert( emptySlotsCount >= 0 );

        if ( combined.size() > static_cast<size_t>( emptySlotsCount ) && takerBag.PushArtifact( combined.back() ) ) {
            combined.pop_back();
        }
    }

    // Giver gets the rest of artifacts
    while ( !combined.empty() && giverBag.PushArtifact( combined.back() ) ) {
        combined.pop_back();
    }

    assert( combined.empty() );

    // Assemble artifact sets after the exchange, if possible
    takerBag.assembleArtifactSetIfPossible();
    giverBag.assembleArtifactSetIfPossible();
}

bool BagArtifacts::ContainUltimateArtifact() const
{
    return std::any_of( begin(), end(), []( const Artifact & art ) { return art.isUltimate(); } );
}

std::string BagArtifacts::String() const
{
    std::string output;

    for ( const Artifact & art : *this ) {
        if ( !art.isValid() ) {
            continue;
        }

        output += art.GetName();
        output += ", ";
    }

    return output;
}

uint32_t BagArtifacts::Count( const Artifact & art ) const
{
    return static_cast<uint32_t>( std::count( begin(), end(), art ) ); // no way that we have more than 4 billion artifacts
}

uint32_t GoldInsteadArtifact( const MP2::MapObjectType objectType )
{
    switch ( objectType ) {
    case MP2::OBJ_SKELETON:
    case MP2::OBJ_TREASURE_CHEST:
    case MP2::OBJ_SHIPWRECK_SURVIVOR:
        return 1000;
    case MP2::OBJ_SEA_CHEST:
        return 1500;
    case MP2::OBJ_GRAVEYARD:
        return 2000;
    case MP2::OBJ_SHIPWRECK:
        return 5000;
    default:
        break;
    }
    return 0;
}

void fheroes2::ResetArtifactStats()
{
    std::fill( artifactGlobalStatus.begin(), artifactGlobalStatus.end(), static_cast<uint8_t>( 0 ) );
}

void fheroes2::ExcludeArtifactFromRandom( const int artifactID )
{
    const size_t id = static_cast<size_t>( artifactID );

    assert( id < artifactGlobalStatus.size() );

    artifactGlobalStatus[id] |= ART_RNDDISABLED;
}

bool fheroes2::isPriceOfLoyaltyArtifact( const int artifactID )
{
    return artifactID >= Artifact::SPELL_SCROLL && artifactID <= Artifact::SPADE_NECROMANCY;
}

ArtifactsBar::ArtifactsBar( Heroes * hero, const bool mini, const bool ro, const bool change, const bool allowOpeningMagicBook, StatusBar * bar )
    : _hero( hero )
    , use_mini_sprite( mini )
    , read_only( ro )
    , can_change( change )
    , _allowOpeningMagicBook( allowOpeningMagicBook )
    , _statusBar( bar )
{
    if ( use_mini_sprite ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::HSICONS, 0 );
        const fheroes2::Rect rt( 26, 21, 32, 32 );

        backsf.resize( rt.width + 2, rt.height + 2 );
        backsf.reset();

        fheroes2::DrawBorder( backsf, fheroes2::GetColorId( 0xD0, 0xC0, 0x48 ) );
        fheroes2::Blit( sprite, rt.x, rt.y, backsf, 1, 1, rt.width, rt.height );

        setSingleItemSize( { backsf.width(), backsf.height() } );

        spcursor.resize( backsf.width(), backsf.height() );
        spcursor.reset();
        fheroes2::DrawBorder( spcursor, 214 );
    }
    else {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::ARTIFACT, 0 );
        setSingleItemSize( { sprite.width(), sprite.height() } );

        spcursor.resize( 70, 70 );
        spcursor.reset();
        fheroes2::DrawRect( spcursor, { 0, 0, 70, 70 }, 190 );
        fheroes2::DrawRect( spcursor, { 1, 1, 68, 68 }, 180 );
        fheroes2::DrawRect( spcursor, { 2, 2, 66, 66 }, 190 );
    }
}

void ArtifactsBar::ResetSelected()
{
    spcursor.hide();
    Interface::ItemsActionBar<Artifact>::ResetSelected();
}

void ArtifactsBar::Redraw( fheroes2::Image & dstsf )
{
    spcursor.hide();
    Interface::ItemsActionBar<Artifact>::Redraw( dstsf );
}

void ArtifactsBar::RedrawBackground( const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    if ( use_mini_sprite )
        fheroes2::Blit( backsf, dstsf, pos.x, pos.y );
    else
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::ARTIFACT, 0 ), dstsf, pos.x, pos.y );
}

void ArtifactsBar::RedrawItem( Artifact & art, const fheroes2::Rect & pos, bool selected, fheroes2::Image & dstsf )
{
    if ( art.isValid() ) {
        if ( use_mini_sprite ) {
            const fheroes2::Sprite & artifactSprite = fheroes2::AGG::GetICN( ICN::ARTFX, art.IndexSprite32() );
            fheroes2::Fill( dstsf, pos.x + 1, pos.y + 1, artifactSprite.width(), artifactSprite.height(), 0 );
            fheroes2::Blit( artifactSprite, dstsf, pos.x + 1, pos.y + 1 );
        }
        else {
            const fheroes2::Sprite & artifactSprite = fheroes2::AGG::GetICN( ICN::ARTIFACT, art.IndexSprite64() );
            fheroes2::Fill( dstsf, pos.x, pos.y, artifactSprite.width(), artifactSprite.height(), 0 );
            fheroes2::Blit( artifactSprite, dstsf, pos.x, pos.y );
        }

        if ( selected ) {
            if ( use_mini_sprite )
                spcursor.setPosition( pos.x, pos.y );
            else
                spcursor.setPosition( pos.x - 3, pos.y - 3 );

            spcursor.show();
        }
    }
}

bool ArtifactsBar::ActionBarLeftMouseSingleClick( Artifact & art )
{
    if ( isMagicBook( art ) ) {
        const bool isMbSelected = ( !isSelected() || isMagicBook( *GetSelectedItem() ) );
        if ( isMbSelected ) {
            if ( can_change ) {
                _hero->EditSpellBook();
            }
            else if ( _allowOpeningMagicBook ) {
                _hero->OpenSpellBook( SpellBook::Filter::ALL, false, false,
                                      _statusBar ? [this]( const std::string & status ) { _statusBar->ShowMessage( status ); }
                                                 : std::function<void( const std::string & )>{} );
            }
            else {
                messageMagicBookAbortTrading();
            }
        }

        return false;
    }

    if ( isSelected() ) {
        if ( !read_only ) {
            std::swap( art, *GetSelectedItem() );
        }
        return false;
    }

    if ( art.isValid() ) {
        if ( !read_only ) {
            spcursor.hide();
        }
    }
    else {
        if ( can_change ) {
            art = Dialog::selectArtifact( Artifact::UNKNOWN, false );

            if ( isMagicBook( art ) ) {
                art.Reset();

                if ( _hero->HaveSpellBook() ) {
                    fheroes2::showStandardTextMessage( Artifact( Artifact::MAGIC_BOOK ).GetName(), _( "You cannot have multiple spell books." ), Dialog::OK );
                }
                else {
                    _hero->SpellBookActivate();
                }
            }
            else if ( art.GetID() == Artifact::SPELL_SCROLL ) {
                const int spellId = Dialog::selectSpell( Spell::RANDOM, false ).GetID();

                if ( spellId == Spell::NONE ) {
                    // No spell for the Spell Scroll artifact was selected - cancel the artifact selection.
                    art.Reset();
                }
                else {
                    art.SetSpell( spellId );
                }
            }
        }

        return false;
    }

    return true;
}

bool ArtifactsBar::ActionBarLeftMouseDoubleClick( Artifact & art )
{
    if ( art.isValid() ) {
        fheroes2::ArtifactDialogElement( art ).showPopup( Dialog::OK );
    }

    ResetSelected();

    return true;
}

bool ArtifactsBar::ActionBarRightMouseHold( Artifact & art )
{
    ResetSelected();

    if ( art.isValid() ) {
        if ( can_change ) {
            if ( isMagicBook( art ) ) {
                _hero->SpellBookDeactivate();
            }
            else {
                art.Reset();
            }
        }
        else {
            fheroes2::ArtifactDialogElement( art ).showPopup( Dialog::ZERO );
        }
    }

    return true;
}

bool ArtifactsBar::ActionBarLeftMouseSingleClick( Artifact & art1, Artifact & art2 )
{
    if ( !isMagicBook( art1 ) && !isMagicBook( art2 ) ) {
        std::swap( art1, art2 );
    }
    else {
        messageMagicBookAbortTrading();
    }

    return false;
}

bool ArtifactsBar::ActionBarCursor( Artifact & art )
{
    if ( isSelected() ) {
        const Artifact * art2 = GetSelectedItem();

        if ( &art == art2 ) {
            if ( isMagicBook( art ) )
                msg = _( "View Spells" );
            else {
                msg = _( "View %{name} Info" );
                StringReplace( msg, "%{name}", art.GetName() );
            }
        }
        else if ( !art.isValid() ) {
            if ( !read_only ) {
                msg = _( "Move %{name}" );
                StringReplace( msg, "%{name}", art2->GetName() );
            }
        }
        else if ( !read_only ) {
            if ( isMagicBook( art ) ) {
                msg = _( "Cannot move the Spellbook" );
            }
            else {
                msg = _( "Exchange %{name2} with %{name}" );
                StringReplace( msg, "%{name}", art.GetName() );
                StringReplace( msg, "%{name2}", art2->GetName() );
            }
        }
    }
    else if ( art.isValid() ) {
        if ( isMagicBook( art ) ) {
            msg = _( "View Spells" );
        }
        else {
            msg = _( "Select %{name}" );
            StringReplace( msg, "%{name}", art.GetName() );
        }
    }

    return false;
}

bool ArtifactsBar::ActionBarCursor( Artifact & art1, Artifact & art2 /* selected */ )
{
    if ( isMagicBook( art2 ) || isMagicBook( art1 ) )
        msg = _( "Cannot move the Spellbook" );
    else if ( art1.isValid() ) {
        msg = _( "Exchange %{name2} with %{name}" );
        StringReplace( msg, "%{name}", art1.GetName() );
        StringReplace( msg, "%{name2}", art2.GetName() );
    }
    else {
        msg = _( "Move %{name}" );
        StringReplace( msg, "%{name}", art2.GetName() );
    }

    return false;
}

bool ArtifactsBar::QueueEventProcessing( std::string * str )
{
    msg.clear();
    bool res = Interface::ItemsActionBar<Artifact>::QueueEventProcessing();
    if ( str )
        *str = msg;
    return res;
}

bool ArtifactsBar::QueueEventProcessing( ArtifactsBar & bar, std::string * str )
{
    msg.clear();
    bool res = Interface::ItemsActionBar<Artifact>::QueueEventProcessing( bar );
    if ( str )
        *str = msg;
    return res;
}

bool ArtifactsBar::isMagicBook( const Artifact & artifact )
{
    return artifact.GetID() == Artifact::MAGIC_BOOK;
}

void ArtifactsBar::messageMagicBookAbortTrading() const
{
    fheroes2::showStandardTextMessage( "", _( "This item can't be traded." ), Dialog::OK );
}

std::set<ArtifactSetData> BagArtifacts::assembleArtifactSetIfPossible()
{
    std::set<ArtifactSetData> assembledArtifactSets;

    for ( const auto & setData : artifactSets ) {
        bool foundAllArtifacts = true;
        while ( foundAllArtifacts ) {
            for ( const int artifactId : setData.second ) {
                if ( std::find( begin(), end(), Artifact( artifactId ) ) == end() ) {
                    foundAllArtifacts = false;
                    break;
                }
            }

            if ( !foundAllArtifacts )
                break;

            // At this point, we have confirmed that all the artifact parts are present
            // so remove the parts and then add the assembled artifact to BagArtifacts
            for ( const int artifactId : setData.second )
                RemoveArtifact( artifactId );

            assembledArtifactSets.insert( setData.first );
            PushArtifact( setData.first._assembledArtifactID );
        }
    }

    return assembledArtifactSets;
}

bool ArtifactSetData::operator<( const ArtifactSetData & other ) const
{
    return _assembledArtifactID < other._assembledArtifactID;
}
