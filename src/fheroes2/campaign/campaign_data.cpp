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
#include "assert.h"
#include "maps_fileinfo.h"
#include "monster.h"
#include "resource.h"

namespace Campaign
{
    CampaignData::CampaignData()
        : _campaignID( 0 )
        , _isGoodCampaign( false )
        , _campaignDescription()
        , _scenarios()
    {}

    const std::vector<int> CampaignData::getScenariosBefore( const int scenarioID ) const
    {
        std::vector<int> scenarioIDs;

        for ( size_t i = 0, count = _scenarios.size(); i < count; ++i ) {
            if ( _scenarios[i].getScenarioID() >= scenarioID )
                break;

            const std::vector<int> nextMaps = _scenarios[i].getNextMaps();
            for ( size_t j = 0, nextMapCount = nextMaps.size(); j < nextMapCount; ++j ) {
                if ( nextMaps[i] == scenarioID )
                    scenarioIDs.emplace_back( nextMaps[i] );
            }
        }

        return scenarioIDs;
    }

    const std::vector<int> & CampaignData::getScenariosAfter( const int scenarioID ) const
    {
        return _scenarios[scenarioID].getNextMaps();
    }

    const std::vector<int> CampaignData::getStartingScenarios() const
    {
        std::vector<int> startingScenarios;

        for ( size_t i = 0, count = _scenarios.size(); i < count; ++i ) {
            if ( isStartingScenario( _scenarios[i].getScenarioID() ) )
                startingScenarios.emplace_back( _scenarios[i].getScenarioID() );
        }

        return startingScenarios;
    }

    bool CampaignData::isStartingScenario( const int scenarioID ) const
    {
        // starting scenario = a scenario that is never included as a nextMap
        for ( size_t i = 0, count = _scenarios.size(); i < count; ++i ) {
            const std::vector<int> & nextMaps = _scenarios[i].getNextMaps();

            if ( std::any_of( nextMaps.begin(), nextMaps.end(), [&]( const int nextMap ) { return nextMap == scenarioID; } ) )
                return false;
        }

        return true;
    }

    bool CampaignData::isAllCampaignMapsPresent() const
    {
        for ( int i = 0, count = _scenarios.size(); i < count; ++i ) {
            if ( !_scenarios[i].isMapFilePresent() )
                return false;
        }

        return true;
    }

    bool CampaignData::isLastScenario( const int scenarioID ) const
    {
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
}
