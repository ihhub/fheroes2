/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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
#include "artifact.h"
#include "maps_fileinfo.h"
#include "monster.h"
#include "race.h"
#include "resource.h"
#include "skill.h"
#include "spell.h"
#include "translations.h"
#include <cassert>

namespace
{
    std::vector<Campaign::ScenarioBonusData> getRolandCampaignBonusData( const int scenarioID )
    {
        std::vector<Campaign::ScenarioBonusData> bonus;

        switch ( scenarioID ) {
        case 0:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        case 1:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WZRD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::SORC, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::KNGT, 1 );
            break;
        case 2:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WZRD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::SORC, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::KNGT, 1 );
            break;
        case 3:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WZRD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::SORC, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::KNGT, 1 );
            break;
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
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 4:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 5:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 6:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
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
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::ANIMATEDEAD, 1 );
            break;
        case 7:
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::VIEWHEROES, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::TROOP, Monster::MAGE, 5 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::LOGISTICS, Skill::Level::ADVANCED );
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
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 1000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::WOOD, 10 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::MERCURY, 2 );
            break;
        case 6:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 1000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL_SECONDARY, Skill::Secondary::SCOUTING, 1 );
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

    const char * getArtifactCampaignName( const int32_t artifactId )
    {
        switch ( artifactId ) {
        case Artifact::THUNDER_MACE:
            return _( "Thunder Mace" );
        case Artifact::MINOR_SCROLL:
            return _( "Minor Scroll" );
        case Artifact::DRAGON_SWORD:
            return _( "Dragon Sword" );
        case Artifact::DIVINE_BREASTPLATE:
            return _( "Breastplate" );
        case Artifact::FIZBIN_MISFORTUNE:
            return _( "Fibzin Medal" );
        case Artifact::MAGE_RING:
            return _( "Mage's ring" );
        case Artifact::DEFENDER_HELM:
            return _( "Defender Helm" );
        case Artifact::POWER_AXE:
            return _( "Power Axe" );
        default:
            return Artifact( artifactId ).GetName();
        }
    }

    const char * getSpellCampaignName( const Uint32 spellId )
    {
        switch ( spellId ) {
        case Spell::SUMMONEELEMENT:
            return _( "Summon Earth" );
        default:
            return Spell( spellId ).GetName();
        }
    }
}

namespace Campaign
{
    bool isCampaignMap( const std::string & fullMapPath, const std::string & scenarioMapName )
    {
        if ( fullMapPath.size() < scenarioMapName.size() ) {
            return false;
        }
        const std::string lowerFullPath = StringLower( fullMapPath );
        return lowerFullPath.compare( lowerFullPath.size() - scenarioMapName.size(), scenarioMapName.size(), scenarioMapName ) == 0;
    }

    bool tryGetMatchingFile( const std::string & fileName, std::string & matchingFilePath )
    {
        const std::string fileExtension = fileName.substr( fileName.rfind( '.' ) + 1 );
        const ListFiles files = Settings::GetListFiles( "maps", fileExtension );

        const auto iterator = std::find_if( files.begin(), files.end(), [&fileName]( const std::string & filePath ) { return isCampaignMap( filePath, fileName ); } );

        if ( iterator != files.end() ) {
            matchingFilePath = *iterator;
            return true;
        }

        return false;
    }

    ScenarioBonusData::ScenarioBonusData()
        : _type( 0 )
        , _subType( 0 )
        , _amount( 0 )
    {}

    ScenarioBonusData::ScenarioBonusData( uint32_t type, uint32_t subType, uint32_t amount )
        : _type( type )
        , _subType( subType )
        , _amount( amount )
    {}

    std::string ScenarioBonusData::ToString() const
    {
        const std::vector<uint32_t> useAmountTypes
            = { ScenarioBonusData::ARTIFACT, ScenarioBonusData::RESOURCES, ScenarioBonusData::TROOP, ScenarioBonusData::SKILL_PRIMARY };

        std::string objectName;

        switch ( _type ) {
        case ScenarioBonusData::ARTIFACT:
            objectName = getArtifactCampaignName( _subType );
            break;
        case ScenarioBonusData::RESOURCES:
            objectName = Resource::String( _subType );
            break;
        case ScenarioBonusData::TROOP:
            objectName = Monster( _subType ).GetPluralName( _amount );
            break;
        case ScenarioBonusData::SPELL:
            objectName = getSpellCampaignName( _subType );
            break;
        case ScenarioBonusData::STARTING_RACE:
            objectName = Race::String( _subType );
            break;
        case ScenarioBonusData::SKILL_PRIMARY:
            objectName = Skill::Primary::String( _subType );
            break;
        case ScenarioBonusData::SKILL_SECONDARY:
            objectName = Skill::Secondary( _subType, _amount ).GetName();
            break;
        default:
            assert( 0 ); // some new bonus?
        }

        const bool useAmount = std::find( useAmountTypes.begin(), useAmountTypes.end(), _type ) != useAmountTypes.end() && _amount > 1;
        return useAmount ? std::to_string( _amount ) + " " + objectName : objectName;
    }

    std::vector<ScenarioBonusData> ScenarioBonusData::getCampaignBonusData( const int campaignID, const int scenarioID )
    {
        assert( scenarioID >= 0 );
        switch ( campaignID ) {
        case Campaign::ROLAND_CAMPAIGN:
            return getRolandCampaignBonusData( scenarioID );
        case Campaign::ARCHIBALD_CAMPAIGN:
            return getArchibaldCampaignBonusData( scenarioID );
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
            return getPriceOfLoyaltyCampaignBonusData( scenarioID );
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            return getVoyageHomeCampaignBonusData( scenarioID );
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
            return getWizardsIsleCampaignBonusData( scenarioID );
        case Campaign::DESCENDANTS_CAMPAIGN:
            return getDescendantsCampaignBonusData( scenarioID );
        }

        // shouldn't be here unless we get an unsupported campaign
        return std::vector<Campaign::ScenarioBonusData>();
    }

    StreamBase & operator<<( StreamBase & msg, const Campaign::ScenarioBonusData & data )
    {
        return msg << data._type << data._subType << data._amount;
    }

    StreamBase & operator>>( StreamBase & msg, Campaign::ScenarioBonusData & data )
    {
        return msg >> data._type >> data._subType >> data._amount;
    }

    ScenarioData::ScenarioData( int scenarioID, const std::vector<int> & nextMaps, const std::vector<ScenarioBonusData> & bonuses, const std::string & fileName,
                                const std::string & scenarioName, const std::string & description, const VideoSequence & startScenarioVideoPlayback,
                                const VideoSequence & endScenarioVideoPlayback, const ScenarioVictoryCondition victoryCondition,
                                const ScenarioLossCondition lossCondition )
        : _scenarioID( scenarioID )
        , _nextMaps( nextMaps )
        , _bonuses( bonuses )
        , _fileName( fileName )
        , _scenarioName( scenarioName )
        , _description( description )
        , _victoryCondition( victoryCondition )
        , _lossCondition( lossCondition )
        , _startScenarioVideoPlayback( startScenarioVideoPlayback )
        , _endScenarioVideoPlayback( endScenarioVideoPlayback )
    {}

    bool Campaign::ScenarioData::isMapFilePresent() const
    {
        std::string matchingFilePath;
        return tryGetMatchingFile( _fileName, matchingFilePath );
    }

    Maps::FileInfo Campaign::ScenarioData::loadMap() const
    {
        std::string matchingFilePath;

        Maps::FileInfo fi;
        if ( tryGetMatchingFile( _fileName, matchingFilePath ) )
            fi.ReadMP2( matchingFilePath );

        return fi;
    }
}
