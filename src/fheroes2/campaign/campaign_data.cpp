/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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
#include "assert.h"
#include "monster.h"
#include "resource.h"

namespace Campaign
{
    void CampaignData::addCampaignAward( const std::string & award )
    {
        _earnedCampaignAwards.emplace_back( award );
    }

    void CampaignData::setCurrentScenarioBonus( const ScenarioBonusData & bonus )
    {
        _currentScenarioBonus = bonus;
    }

    void CampaignData::setCurrentScenarioID( const int scenarioID )
    {
        _currentScenarioID = scenarioID;
    }

    void CampaignData::setCampaignID( const int campaignID )
    {
        _campaignID = campaignID;
    }

    void CampaignData::addCurrentMapToFinished()
    {
        _finishedMaps.emplace_back( _currentScenarioID );
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

    CampaignData::CampaignData()
        : _finishedMaps()
        , _earnedCampaignAwards()
        , _currentScenarioID( 0 )
        , _campaignID( 0 )
        , _currentScenarioBonus()
    {}

    const std::string ScenarioBonusData::ToString() const
    {
        std::string objectName;

        switch ( _type ) {
        case ScenarioBonusData::ARTIFACT:
            objectName = Artifact( _subType ).GetName();
            break;
        case ScenarioBonusData::RESOURCES:
            objectName = Resource::String( _subType );
            break;
        case ScenarioBonusData::TROOP:
            objectName = Monster( _subType ).GetPluralName( _amount );
            break;
        default:
            assert( 0 ); // some new bonus?
        }

        const bool useAmount = _amount > 1;
        return useAmount ? std::to_string( _amount ) + " " + objectName : objectName;
    }

    StreamBase & operator<<( StreamBase & msg, const Campaign::CampaignData & data )
    {
        return msg << data._earnedCampaignAwards << data._currentScenarioID << data._currentScenarioBonus << data._finishedMaps;
    }

    StreamBase & operator>>( StreamBase & msg, Campaign::CampaignData & data )
    {
        return msg >> data._earnedCampaignAwards >> data._currentScenarioID >> data._currentScenarioBonus >> data._finishedMaps;
    }

    StreamBase & operator<<( StreamBase & msg, const Campaign::ScenarioBonusData & data )
    {
        return msg << data._type << data._subType << data._amount;
    }

    StreamBase & operator>>( StreamBase & msg, Campaign::ScenarioBonusData & data )
    {
        return msg >> data._type >> data._subType >> data._amount;
    }
}
