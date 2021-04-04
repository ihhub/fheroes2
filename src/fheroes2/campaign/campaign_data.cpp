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

#include "campaign_data.h"
#include "artifact.h"
#include "campaign_scenariodata.h"
#include "heroes.h"
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
    std::vector<Campaign::CampaignAwardData> getRolandCampaignAwardData( const int scenarioID )
    {
        std::vector<Campaign::CampaignAwardData> obtainableAwards;

        switch ( scenarioID ) {
        case 2:
            obtainableAwards.emplace_back( 0, Campaign::CampaignAwardData::TYPE_CREATURE_ALLIANCE, Monster::DWARF );
            break;
        case 5:
            obtainableAwards.emplace_back( 1, Campaign::CampaignAwardData::TYPE_HIREABLE_HERO, Heroes::ELIZA, 0, 0, _( "Sorceress Guild" ) );
            break;
        case 6:
            obtainableAwards.emplace_back( 2, Campaign::CampaignAwardData::TYPE_CARRY_OVER_FORCES, 0, 0, 9 );
            break;
        case 7:
            obtainableAwards.emplace_back( 3, Campaign::CampaignAwardData::TYPE_GET_ARTIFACT, Artifact::ULTIMATE_CROWN, 1, 9 );
            break;
        case 8:
            obtainableAwards.emplace_back( 4, Campaign::CampaignAwardData::TYPE_REMOVE_ENEMY_HERO, Heroes::CORLAGON, 0, 9 );
            break;
        }

        return obtainableAwards;
    }

    std::vector<Campaign::CampaignAwardData> getArchibaldCampaignAwardData( const int scenarioID )
    {
        std::vector<Campaign::CampaignAwardData> obtainableAwards;

        switch ( scenarioID ) {
        case 2:
            obtainableAwards.emplace_back( 1, Campaign::CampaignAwardData::TYPE_HIREABLE_HERO, Heroes::BAX, 0, 0, _( "Necromancer Guild" ) );
            break;
        case 3:
            obtainableAwards.emplace_back( 2, Campaign::CampaignAwardData::TYPE_CREATURE_ALLIANCE, Monster::OGRE );
            obtainableAwards.emplace_back( 3, Campaign::CampaignAwardData::TYPE_CREATURE_CURSE, Monster::DWARF );
            break;
        case 6:
            obtainableAwards.emplace_back( 4, Campaign::CampaignAwardData::TYPE_CREATURE_ALLIANCE, Monster::GREEN_DRAGON, _( "Dragon Alliance" ) );
            break;
        case 8:
            obtainableAwards.emplace_back( 5, Campaign::CampaignAwardData::TYPE_GET_ARTIFACT, Artifact::ULTIMATE_CROWN );
            break;
        }

        return obtainableAwards;
    }
}

namespace Campaign
{
    CampaignData::CampaignData()
        : _campaignID( 0 )
        , _isGoodCampaign( false )
        , _campaignDescription()
        , _scenarios()
    {}

    std::vector<Campaign::CampaignAwardData> CampaignAwardData::getCampaignAwardData( const int campaignID, const int scenarioID )
    {
        assert( campaignID >= 0 && scenarioID >= 0 );

        switch ( campaignID ) {
        case 0:
            return getRolandCampaignAwardData( scenarioID );
        case 1:
            return getArchibaldCampaignAwardData( scenarioID );
        }

        return std::vector<Campaign::CampaignAwardData>();
    }

    std::vector<int> CampaignData::getScenariosBefore( const int scenarioID ) const
    {
        std::vector<int> scenarioIDs;

        for ( size_t i = 0; i < _scenarios.size(); ++i ) {
            if ( _scenarios[i].getScenarioID() >= scenarioID )
                break;

            const std::vector<int> & nextMaps = _scenarios[i].getNextMaps();

            // if any of this scenario's next maps is the one passed as param, then this scenario is a previous scenario
            if ( std::find( nextMaps.begin(), nextMaps.end(), scenarioID ) != nextMaps.end() )
                scenarioIDs.emplace_back( _scenarios[i].getScenarioID() );
        }

        return scenarioIDs;
    }

    const std::vector<int> & CampaignData::getScenariosAfter( const int scenarioID ) const
    {
        for ( size_t i = 0; i < _scenarios.size(); ++i ) {
            if ( _scenarios[i].getScenarioID() == scenarioID )
                return _scenarios[i].getNextMaps();
        }

        return _scenarios[scenarioID].getNextMaps();
    }

    std::vector<int> CampaignData::getStartingScenarios() const
    {
        std::vector<int> startingScenarios;

        for ( size_t i = 0; i < _scenarios.size(); ++i ) {
            const int scenarioID = _scenarios[i].getScenarioID();
            if ( isStartingScenario( scenarioID ) )
                startingScenarios.emplace_back( scenarioID );
        }

        return startingScenarios;
    }

    bool CampaignData::isStartingScenario( const int scenarioID ) const
    {
        // starting scenario = a scenario that is never included as a nextMap
        for ( size_t i = 0; i < _scenarios.size(); ++i ) {
            const std::vector<int> & nextMaps = _scenarios[i].getNextMaps();

            if ( std::find( nextMaps.begin(), nextMaps.end(), scenarioID ) != nextMaps.end() )
                return false;
        }

        return true;
    }

    bool CampaignData::isAllCampaignMapsPresent() const
    {
        for ( size_t i = 0; i < _scenarios.size(); ++i ) {
            if ( !_scenarios[i].isMapFilePresent() )
                return false;
        }

        return true;
    }

    bool CampaignData::isLastScenario( const int scenarioID ) const
    {
        assert( !_scenarios.empty() );
        return scenarioID == _scenarios.back().getScenarioID();
    }

    void CampaignData::setCampaignID( const int campaignID )
    {
        _campaignID = campaignID;
    }

    void CampaignData::setCampaignAlignment( const bool isGoodCampaign )
    {
        _isGoodCampaign = isGoodCampaign;
    }

    void CampaignData::setCampaignScenarios( const std::vector<ScenarioData> & scenarios )
    {
        _scenarios = scenarios;
    }

    void CampaignData::setCampaignDescription( const std::string & campaignDescription )
    {
        _campaignDescription = campaignDescription;
    }

    CampaignAwardData::CampaignAwardData()
        : _id( 0 )
        , _type( 0 )
        , _subType( 0 )
        , _amount( 0 )
        , _startScenarioID( 0 )
        , _customName()
    {}

    // default amount to 1 for initialized campaign award data
    CampaignAwardData::CampaignAwardData( int id, uint32_t type, uint32_t subType )
        : CampaignAwardData( id, type, subType, 1, 0 )
    {}

    CampaignAwardData::CampaignAwardData( int id, uint32_t type, uint32_t subType, uint32_t amount )
        : CampaignAwardData( id, type, subType, amount, 0 )
    {}

    CampaignAwardData::CampaignAwardData( int id, uint32_t type, uint32_t subType, const std::string & customName )
        : CampaignAwardData( id, type, subType, 1, 0, customName )
    {}

    CampaignAwardData::CampaignAwardData( int id, uint32_t type, uint32_t subType, uint32_t amount, int startScenarioID, const std::string & customName )
        : _id( id )
        , _type( type )
        , _subType( subType )
        , _amount( amount )
        , _startScenarioID( startScenarioID )
        , _customName( customName )
    {}

    std::string CampaignAwardData::ToString() const
    {
        if ( !_customName.empty() )
            return _customName;

        switch ( _type ) {
        case CampaignAwardData::TYPE_CREATURE_CURSE:
            return Monster( _subType ).GetName() + std::string( _( " bane" ) );
        case CampaignAwardData::TYPE_CREATURE_ALLIANCE:
            return Monster( _subType ).GetName() + std::string( _( " alliance" ) );
        case CampaignAwardData::TYPE_GET_ARTIFACT:
            return Artifact( _subType ).GetName();
        case CampaignAwardData::TYPE_CARRY_OVER_FORCES:
            return _( "Carry-over forces" );
        case CampaignAwardData::TYPE_RESOURCE_BONUS:
            return Resource::String( _subType ) + std::string( _( " bonus" ) );
        case CampaignAwardData::TYPE_GET_SPELL:
            return Spell( _subType ).GetName();
        case CampaignAwardData::TYPE_HIREABLE_HERO:
            return Heroes( _subType, 0 ).GetName();
        case CampaignAwardData::TYPE_REMOVE_ENEMY_HERO:
            return Heroes( _subType, 0 ).GetName() + std::string( _( " defeated" ) );
        default:
            assert( 0 ); // some new/unhandled award
        }
    }
}
