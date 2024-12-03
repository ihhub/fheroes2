/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include "campaign_scenariodata.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <list>
#include <map>
#include <utility>

#include "artifact.h"
#include "dir.h"
#include "maps_fileinfo.h"
#include "monster.h"
#include "race.h"
#include "resource.h"
#include "serialize.h"
#include "settings.h"
#include "skill.h"
#include "spell.h"
#include "system.h"
#include "tools.h"
#include "translations.h"

namespace
{
    std::vector<Campaign::ScenarioBonusData> getRolandCampaignBonusData( const int scenarioID )
    {
        std::vector<Campaign::ScenarioBonusData> bonus;

        switch ( scenarioID ) {
        case 0:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::ARMORED_GAUNTLETS, 1 );
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WZRD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::SORC, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::KNGT, 1 );
            break;
        case 5:
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::MIRRORIMAGE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::SUMMONEELEMENT, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::RESURRECT, 1 );
            break;
        case 6:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::BLACK_PEARL, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::DRAGON_SWORD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::DIVINE_BREASTPLATE, 1 );
            break;
        case 7:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WZRD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::SORC, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::KNGT, 1 );
            break;
        case 8:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::CRYSTAL, 20 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GEMS, 20 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::MERCURY, 20 );
            break;
        case 9:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::TAX_LIEN, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::HIDEOUS_MASK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::FIZBIN_MISFORTUNE, 1 );
            break;
        case 10:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        default:
            assert( 0 );
            break;
        }

        return bonus;
    }

    std::vector<Campaign::ScenarioBonusData> getArchibaldCampaignBonusData( const int scenarioID )
    {
        std::vector<Campaign::ScenarioBonusData> bonus;

        switch ( scenarioID ) {
        case 0:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MAGE_RING, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        case 1:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 2:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::MASSCURSE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::DEFENDER_HELM, 1 );
            break;
        case 3:
        case 4:
        case 5:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 6:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE_AND_ARMY, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE_AND_ARMY, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE_AND_ARMY, Race::BARB, 1 );
            break;
        case 7:
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::LOGISTICS, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::POWER_AXE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::WHITE_PEARL, 1 );
            break;
        case 8:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 9:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::BLACK_PEARL, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::DRAGON_SWORD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::DIVINE_BREASTPLATE, 1 );
            break;
        case 10:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::TAX_LIEN, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::HIDEOUS_MASK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::FIZBIN_MISFORTUNE, 1 );
            break;
        case 11:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WZRD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::SORC, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::KNGT, 1 );
            break;
        default:
            assert( 0 );
            break;
        }

        return bonus;
    }

    std::vector<Campaign::ScenarioBonusData> getPriceOfLoyaltyCampaignBonusData( const int scenarioID )
    {
        std::vector<Campaign::ScenarioBonusData> bonus;

        switch ( scenarioID ) {
        case 0:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MEDAL_VALOR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::STEALTH_SHIELD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        case 1:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::WHITE_PEARL, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::BALLISTA, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            break;
        case 2:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MAGE_RING, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MAJOR_SCROLL, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::LOGISTICS, Skill::Level::ADVANCED );
            break;
        case 3:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MEDAL_HONOR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::CASTER_BRACELET, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::DEFENDER_HELM, 1 );
            break;
        case 4:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::POWER_AXE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_PRIMARY, Skill::Primary::DEFENSE, 2 );
            break;
        case 5:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::TROOP, Monster::CRUSADER, 3 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::TRAVELER_BOOTS_MOBILITY, 1 );
            break;
        case 6:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::SULFUR, 10 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::POWER_AXE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::SPELL_SCROLL, 1, Spell::ANIMATEDEAD );
            break;
        case 7:
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::VIEWHEROES, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::TROOP, Monster::MAGE, 5 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::ESTATES, Skill::Level::ADVANCED );
            break;
        default:
            assert( 0 );
            break;
        }

        return bonus;
    }

    std::vector<Campaign::ScenarioBonusData> getDescendantsCampaignBonusData( const int scenarioID )
    {
        std::vector<Campaign::ScenarioBonusData> bonus;

        switch ( scenarioID ) {
        case 0:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 1000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::WOOD, 50 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::CRYSTAL, 10 );
            break;
        case 1:
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_PRIMARY, Skill::Primary::ATTACK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_PRIMARY, Skill::Primary::DEFENSE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 1000 );
            break;
        case 2:
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::LOGISTICS, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::STEELSKIN, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MAGE_RING, 1 );
            break;
        case 3:
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::LOGISTICS, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::SCOUTING, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::POWER_AXE, 1 );
            break;
        case 4:
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::LUCK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::LEADERSHIP, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::SPIKED_SHIELD, 1 );
            break;
        case 5:
            // These are negative values as they should be. Do NOT change them!
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, -1000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::WOOD, -10 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::MERCURY, -2 );
            break;
        case 6:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 1000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::SCOUTING, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::WOOD, 20 );
            break;
        case 7:
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::LEADERSHIP, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::ARCHERY, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_PRIMARY, Skill::Primary::POWER, 1 );
            break;
        default:
            assert( 0 );
            break;
        }

        return bonus;
    }

    std::vector<Campaign::ScenarioBonusData> getWizardsIsleCampaignBonusData( const int scenarioID )
    {
        std::vector<Campaign::ScenarioBonusData> bonus;

        switch ( scenarioID ) {
        case 0:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::POWER_AXE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::STEALTH_SHIELD, 1 );
            break;
        case 1:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MAGE_RING, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::FOREMOST_SCROLL, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::FIREBLAST, 1 );
            break;
        case 2:
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::MASSHASTE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::SUMMONEELEMENT, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::CHAINLIGHTNING, 1 );
            break;
        case 3:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::WOOD, 5 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::ORE, 5 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 1000 );
            break;
        default:
            assert( 0 );
            break;
        }

        return bonus;
    }

    std::vector<Campaign::ScenarioBonusData> getVoyageHomeCampaignBonusData( const int scenarioID )
    {
        std::vector<Campaign::ScenarioBonusData> bonus;

        switch ( scenarioID ) {
        case 0:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::NOMAD_BOOTS_MOBILITY, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 500 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::SCOUTING, Skill::Level::BASIC );
            break;
        case 1:
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::PATHFINDING, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::NAVIGATION, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::SULFUR, 15 );
            break;
        case 2:
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::ARCHERY, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 3000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_PRIMARY, Skill::Primary::ATTACK, 1 );
            break;
        case 3:
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::NECROMANCY, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::LOGISTICS, Skill::Level::BASIC );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            break;
        default:
            assert( 0 );
            break;
        }

        return bonus;
    }

    const char * getSpellCampaignName( const int32_t spellId )
    {
        switch ( spellId ) {
        case Spell::ANIMATEDEAD:
            return _( "campaignBonus|Animate Dead" );
        case Spell::CHAINLIGHTNING:
            return _( "campaignBonus|Chain Lightning" );
        case Spell::FIREBLAST:
            return _( "campaignBonus|Fireblast" );
        case Spell::MASSCURSE:
            return _( "campaignBonus|Mass Curse" );
        case Spell::MASSHASTE:
            return _( "campaignBonus|Mass Haste" );
        case Spell::MIRRORIMAGE:
            return _( "campaignBonus|Mirror Image" );
        case Spell::RESURRECT:
            return _( "campaignBonus|Resurrect" );
        case Spell::STEELSKIN:
            return _( "campaignBonus|Steelskin" );
        case Spell::SUMMONEELEMENT:
            return _( "campaignBonus|Summon Earth" );
        case Spell::VIEWHEROES:
            return _( "campaignBonus|View Heroes" );
        default:
            return Spell( spellId ).GetName();
        }
    }

    const char * getArtifactCampaignName( const int32_t artifactId, const int32_t spellId )
    {
        switch ( artifactId ) {
        case Artifact::BALLISTA:
            return _( "campaignBonus|Ballista" );
        case Artifact::BLACK_PEARL:
            return _( "campaignBonus|Black Pearl" );
        case Artifact::CASTER_BRACELET:
            return _( "campaignBonus|Caster's Bracelet" );
        case Artifact::DEFENDER_HELM:
            return _( "campaignBonus|Defender Helm" );
        case Artifact::DIVINE_BREASTPLATE:
            return _( "campaignBonus|Breastplate" );
        case Artifact::DRAGON_SWORD:
            return _( "campaignBonus|Dragon Sword" );
        case Artifact::FIZBIN_MISFORTUNE:
            return _( "campaignBonus|Fizbin Medal" );
        case Artifact::FOREMOST_SCROLL:
            return _( "campaignBonus|Foremost Scroll" );
        case Artifact::ARMORED_GAUNTLETS:
            return _( "campaignBonus|Gauntlets" );
        case Artifact::HIDEOUS_MASK:
            return _( "campaignBonus|Hideous Mask" );
        case Artifact::MAGE_RING:
            return _( "campaignBonus|Mage's Ring" );
        case Artifact::MAJOR_SCROLL:
            return _( "campaignBonus|Major Scroll" );
        case Artifact::MEDAL_HONOR:
            return _( "campaignBonus|Medal of Honor" );
        case Artifact::MEDAL_VALOR:
            return _( "campaignBonus|Medal of Valor" );
        case Artifact::MINOR_SCROLL:
            return _( "campaignBonus|Minor Scroll" );
        case Artifact::NOMAD_BOOTS_MOBILITY:
            return _( "campaignBonus|Nomad Boots" );
        case Artifact::POWER_AXE:
            return _( "campaignBonus|Power Axe" );
        case Artifact::SPIKED_SHIELD:
            return _( "campaignBonus|Spiked Shield" );
        case Artifact::STEALTH_SHIELD:
            return _( "campaignBonus|Stealth Shield" );
        case Artifact::TAX_LIEN:
            return _( "campaignBonus|Tax Lien" );
        case Artifact::THUNDER_MACE:
            return _( "campaignBonus|Thunder Mace" );
        case Artifact::TRAVELER_BOOTS_MOBILITY:
            return _( "campaignBonus|Traveler's Boots" );
        case Artifact::WHITE_PEARL:
            return _( "campaignBonus|White Pearl" );
        case Artifact::SPELL_SCROLL:
            return getSpellCampaignName( spellId );
        default:
            return Artifact( artifactId ).GetName();
        }
    }

    std::string getSecondarySkillCampaignName( const uint32_t secondarySkillId, const uint32_t secondarySkillLevel )
    {
        switch ( secondarySkillId ) {
        case Skill::Secondary::ARCHERY:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Archery" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Archery" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Archery" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::BALLISTICS:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Ballistics" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Ballistics" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Ballistics" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::DIPLOMACY:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Diplomacy" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Diplomacy" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Diplomacy" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::EAGLE_EYE:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Eagle Eye" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Eagle Eye" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Eagle Eye" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::ESTATES:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Estates" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Estates" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Estates" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::LEADERSHIP:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Leadership" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Leadership" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Leadership" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::LOGISTICS:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Logistics" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Logistics" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Logistics" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::LUCK:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Luck" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Luck" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Luck" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::MYSTICISM:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Mysticism" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Mysticism" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Mysticism" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::NAVIGATION:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Navigation" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Navigation" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Navigation" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::NECROMANCY:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Necromancy" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Necromancy" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Necromancy" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::PATHFINDING:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Pathfinding" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Pathfinding" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Pathfinding" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::SCOUTING:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Scouting" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Scouting" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Scouting" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        case Skill::Secondary::WISDOM:
            switch ( secondarySkillLevel ) {
            case Skill::Level::BASIC:
                return _( "campaignBonus|Basic Wisdom" );
            case Skill::Level::ADVANCED:
                return _( "campaignBonus|Advanced Wisdom" );
            case Skill::Level::EXPERT:
                return _( "campaignBonus|Expert Wisdom" );
            default:
                // There cannot be other valid values of the skill level
                assert( 0 );
                return {};
            }
        default:
            // There cannot be other valid values of secondary skills
            assert( 0 );
            return {};
        }
    }

    bool tryGetMatchingFile( const std::string & fileName, std::string & matchingFilePath )
    {
        static const auto fileNameToPath = []() {
            std::map<std::string, std::string> result;

            const ListFiles files = Settings::FindFiles( "maps", "", false );

            for ( const std::string & file : files ) {
                result.try_emplace( StringLower( System::GetFileName( file ) ), file );
            }

            return result;
        }();

        const auto result = fileNameToPath.find( fileName );

        if ( result != fileNameToPath.end() ) {
            matchingFilePath = result->second;

            return true;
        }

        return false;
    }
}

namespace Campaign
{
    OStreamBase & operator<<( OStreamBase & stream, const ScenarioInfoId & data )
    {
        return stream << data.campaignId << data.scenarioId;
    }

    IStreamBase & operator>>( IStreamBase & stream, ScenarioInfoId & data )
    {
        return stream >> data.campaignId >> data.scenarioId;
    }

    ScenarioBonusData::ScenarioBonusData()
        : _type( 0 )
        , _subType( 0 )
        , _amount( 0 )
        , _artifactSpellId( Spell::NONE )
    {}

    ScenarioBonusData::ScenarioBonusData( const int32_t type, const int32_t subType, const int32_t amount )
        : _type( type )
        , _subType( subType )
        , _amount( amount )
        , _artifactSpellId( Spell::NONE )
    {}

    ScenarioBonusData::ScenarioBonusData( const int32_t type, const int32_t subType, const int32_t amount, const int32_t spellId )
        : _type( type )
        , _subType( subType )
        , _amount( amount )
        , _artifactSpellId( spellId )
    {}

    std::string ScenarioBonusData::getName() const
    {
        std::string objectName;

        switch ( _type ) {
        case ScenarioBonusData::ARTIFACT:
            objectName = getArtifactCampaignName( _subType, _artifactSpellId );
            break;
        case ScenarioBonusData::RESOURCES:
            objectName = std::to_string( _amount ) + " " + Resource::String( _subType );
            break;
        case ScenarioBonusData::TROOP:
            objectName = Monster( _subType ).GetPluralName( _amount );
            break;
        case ScenarioBonusData::SPELL:
            objectName = getSpellCampaignName( _subType );
            break;
        case ScenarioBonusData::STARTING_RACE:
        case ScenarioBonusData::STARTING_RACE_AND_ARMY:
            objectName = Race::String( _subType );
            break;
        case ScenarioBonusData::SKILL_PRIMARY:
            objectName = Skill::Primary::String( _subType );
            break;
        case ScenarioBonusData::SKILL_SECONDARY:
            objectName = getSecondarySkillCampaignName( _subType, _amount );
            break;
        default:
            assert( 0 ); // some new bonus?
            break;
        }

        const std::vector<int32_t> useAmountTypes = { ScenarioBonusData::ARTIFACT, ScenarioBonusData::TROOP, ScenarioBonusData::SKILL_PRIMARY };
        const bool useAmount = std::find( useAmountTypes.begin(), useAmountTypes.end(), _type ) != useAmountTypes.end() && _amount > 1;

        return useAmount ? std::to_string( _amount ) + " " + objectName : objectName;
    }

    std::string ScenarioBonusData::getDescription() const
    {
        switch ( _type ) {
        case ScenarioBonusData::ARTIFACT: {
            std::string description( _( "The main hero will have the \"%{artifact}\" artifact at the start of the scenario." ) );
            StringReplace( description, "%{artifact}", Artifact( _subType ).GetName() );
            return description;
        }
        case ScenarioBonusData::RESOURCES: {
            std::string description( _amount > 0 ? _( "The kingdom will receive %{amount} additional %{resource} at the start of the scenario." )
                                                 : _( "The kingdom will have %{amount} less %{resource} at the start of the scenario." ) );
            StringReplace( description, "%{amount}", std::to_string( std::abs( _amount ) ) );
            StringReplace( description, "%{resource}", Resource::String( _subType ) );
            return description;
        }
        case ScenarioBonusData::TROOP: {
            std::string description( _( "The main hero will have %{count} %{monster} at the start of the scenario." ) );
            StringReplace( description, "%{count}", std::to_string( _amount ) );
            StringReplaceWithLowercase( description, "%{monster}", Monster( _subType ).GetPluralName( _amount ) );
            return description;
        }
        case ScenarioBonusData::SPELL: {
            std::string description( _( "The main hero will have the \"%{spell}\" spell at the start of the scenario." ) );
            StringReplace( description, "%{spell}", Spell( _subType ).GetName() );
            return description;
        }
        case ScenarioBonusData::STARTING_RACE:
        case ScenarioBonusData::STARTING_RACE_AND_ARMY: {
            std::string description( _( "The starting alignment of the scenario will be %{race}." ) );
            StringReplace( description, "%{race}", Race::String( _subType ) );
            return description;
        }
        case ScenarioBonusData::SKILL_PRIMARY: {
            std::string description( _( "The main hero will receive a +%{count} to their %{skill} at the start of the scenario." ) );
            StringReplace( description, "%{count}", std::to_string( _amount ) );
            StringReplace( description, "%{skill}", Skill::Primary::String( _subType ) );
            return description;
        }
        case ScenarioBonusData::SKILL_SECONDARY: {
            std::string description( _( "The main hero will have %{skill} at the start of the scenario." ) );
            StringReplace( description, "%{skill}", Skill::Secondary( _subType, _amount ).GetName() );
            return description;
        }
        default:
            assert( 0 ); // some new bonus?
            break;
        }

        return {};
    }

    std::vector<ScenarioBonusData> ScenarioBonusData::getCampaignBonusData( const ScenarioInfoId & scenarioInfo )
    {
        assert( scenarioInfo.scenarioId >= 0 );
        switch ( scenarioInfo.campaignId ) {
        case Campaign::ROLAND_CAMPAIGN:
            return getRolandCampaignBonusData( scenarioInfo.scenarioId );
        case Campaign::ARCHIBALD_CAMPAIGN:
            return getArchibaldCampaignBonusData( scenarioInfo.scenarioId );
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
            return getPriceOfLoyaltyCampaignBonusData( scenarioInfo.scenarioId );
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            return getVoyageHomeCampaignBonusData( scenarioInfo.scenarioId );
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
            return getWizardsIsleCampaignBonusData( scenarioInfo.scenarioId );
        case Campaign::DESCENDANTS_CAMPAIGN:
            return getDescendantsCampaignBonusData( scenarioInfo.scenarioId );
        default:
            // Did you add a new campaign? Add the corresponding case above.
            assert( 0 );
            break;
        }

        // shouldn't be here unless we get an unsupported campaign
        return std::vector<Campaign::ScenarioBonusData>();
    }

    ScenarioData::ScenarioData( const ScenarioInfoId & scenarioInfo, std::vector<ScenarioInfoId> && nextScenarios, const std::string & fileName,
                                const std::string & scenarioName, const std::string & description, const VideoSequence & startScenarioVideoPlayback,
                                const VideoSequence & endScenarioVideoPlayback, const ScenarioVictoryCondition victoryCondition,
                                const ScenarioLossCondition lossCondition )
        : _scenarioInfo( scenarioInfo )
        , _nextScenarios( std::move( nextScenarios ) )
        , _bonuses( ScenarioBonusData::getCampaignBonusData( scenarioInfo ) )
        , _fileName( StringLower( fileName ) )
        , _scenarioName( scenarioName )
        , _description( description )
        , _victoryCondition( victoryCondition )
        , _lossCondition( lossCondition )
        , _startScenarioVideoPlayback( startScenarioVideoPlayback )
        , _endScenarioVideoPlayback( endScenarioVideoPlayback )
    {}

    const char * ScenarioData::getScenarioName() const
    {
        return _( _scenarioName );
    }

    const char * ScenarioData::getDescription() const
    {
        return _( _description );
    }

    bool Campaign::ScenarioData::isMapFilePresent() const
    {
        std::string matchingFilePath;
        return tryGetMatchingFile( _fileName, matchingFilePath );
    }

    Maps::FileInfo Campaign::ScenarioData::loadMap() const
    {
        std::string matchingFilePath;

        if ( tryGetMatchingFile( _fileName, matchingFilePath ) ) {
            Maps::FileInfo fi;

            if ( fi.readMP2Map( std::move( matchingFilePath ), false ) ) {
                return fi;
            }
        }

        return {};
    }

    const char * getCampaignName( const int campaignId )
    {
        switch ( campaignId ) {
        case Campaign::ROLAND_CAMPAIGN:
            return _( "Roland" );
        case Campaign::ARCHIBALD_CAMPAIGN:
            return _( "Archibald" );
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
            return _( "Price of Loyalty" );
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            return _( "Voyage Home" );
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
            return _( "Wizard's Isle" );
        case Campaign::DESCENDANTS_CAMPAIGN:
            return _( "Descendants" );
        default:
            // Did you add a new campaign? Add the corresponding case above.
            assert( 0 );
            break;
        }

        return "???";
    }
}
