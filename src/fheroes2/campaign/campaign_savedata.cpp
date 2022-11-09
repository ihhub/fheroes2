/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2022                                             *
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

#include "campaign_savedata.h"
#include "army.h"
#include "campaign_data.h"
#include "game.h"
#include "save_format_version.h"
#include "serialize.h"
#include <algorithm>
#include <cassert>

namespace Campaign
{
    CampaignSaveData & Campaign::CampaignSaveData::Get()
    {
        static CampaignSaveData instance;
        return instance;
    }

    void CampaignSaveData::addCampaignAward( const int awardID )
    {
        _obtainedCampaignAwards.emplace_back( awardID );
    }

    void CampaignSaveData::removeCampaignAward( const int awardID )
    {
        _obtainedCampaignAwards.erase( std::remove( _obtainedCampaignAwards.begin(), _obtainedCampaignAwards.end(), awardID ), _obtainedCampaignAwards.end() );
    }

    void CampaignSaveData::setCurrentScenarioBonus( const ScenarioBonusData & bonus )
    {
        _currentScenarioBonus = bonus;
    }

    void CampaignSaveData::setCurrentScenarioInfoId( const ScenarioInfoId & scenarioInfoId )
    {
        assert( scenarioInfoId.campaignId >= 0 && scenarioInfoId.scenarioId >= 0 );
        _currentScenarioInfoId = scenarioInfoId;
    }

    void CampaignSaveData::addCurrentMapToFinished()
    {
        const bool isNotDuplicate = std::find( _finishedMaps.begin(), _finishedMaps.end(), _currentScenarioInfoId ) == _finishedMaps.end();
        if ( isNotDuplicate )
            _finishedMaps.emplace_back( _currentScenarioInfoId );
    }

    void CampaignSaveData::addDaysPassed( const uint32_t days )
    {
        _daysPassed += days;
    }

    void CampaignSaveData::reset()
    {
        _finishedMaps.clear();
        _obtainedCampaignAwards.clear();
        _carryOverTroops.clear();
        _currentScenarioInfoId = { -1, -1 };
        _daysPassed = 0;
        _difficulty = CampaignDifficulty::Normal;
    }

    void CampaignSaveData::setCarryOverTroops( const Troops & troops )
    {
        _carryOverTroops.clear();

        for ( size_t i = 0; i < troops.Size(); ++i ) {
            _carryOverTroops.emplace_back( *troops.GetTroop( i ) );
        }
    }

    const ScenarioInfoId & CampaignSaveData::getLastCompletedScenarioInfoID() const
    {
        assert( !_finishedMaps.empty() );
        return _finishedMaps.back();
    }

    std::vector<Campaign::CampaignAwardData> CampaignSaveData::getObtainedCampaignAwards() const
    {
        std::vector<Campaign::CampaignAwardData> obtainedAwards;

        for ( size_t i = 0; i < _finishedMaps.size(); ++i ) {
            const std::vector<Campaign::CampaignAwardData> awards = Campaign::CampaignAwardData::getCampaignAwardData( _finishedMaps[i] );

            for ( size_t j = 0; j < awards.size(); ++j ) {
                if ( std::find( _obtainedCampaignAwards.begin(), _obtainedCampaignAwards.end(), awards[j]._id ) != _obtainedCampaignAwards.end() )
                    obtainedAwards.emplace_back( awards[j] );
            }
        }

        const std::vector<Campaign::CampaignAwardData> extraAwards = Campaign::CampaignAwardData::getExtraCampaignAwardData( _currentScenarioInfoId.campaignId );
        for ( const Campaign::CampaignAwardData & award : extraAwards ) {
            if ( std::find( _obtainedCampaignAwards.begin(), _obtainedCampaignAwards.end(), award._id ) != _obtainedCampaignAwards.end() )
                obtainedAwards.emplace_back( award );
        }

        return obtainedAwards;
    }

    StreamBase & operator<<( StreamBase & msg, const CampaignSaveData & data )
    {
        return msg << data._currentScenarioInfoId.campaignId << data._currentScenarioInfoId.scenarioId << data._currentScenarioBonus << data._finishedMaps
                   << data._daysPassed << data._obtainedCampaignAwards << data._carryOverTroops << data._difficulty;
    }

    StreamBase & operator>>( StreamBase & msg, CampaignSaveData & data )
    {
        return msg >> data._currentScenarioInfoId.campaignId >> data._currentScenarioInfoId.scenarioId >> data._currentScenarioBonus >> data._finishedMaps
               >> data._daysPassed >> data._obtainedCampaignAwards >> data._carryOverTroops >> data._difficulty;
    }

    ScenarioVictoryCondition getCurrentScenarioVictoryCondition()
    {
        const CampaignSaveData & campaignData = CampaignSaveData::Get();

        const std::vector<ScenarioData> & scenarios = CampaignData::getCampaignData( campaignData.getCampaignID() ).getAllScenarios();
        const int scenarioId = campaignData.getCurrentScenarioID();
        assert( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() );

        if ( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() ) {
            return scenarios[scenarioId].getVictoryCondition();
        }

        return ScenarioVictoryCondition::STANDARD;
    }

    ScenarioLossCondition getCurrentScenarioLossCondition()
    {
        const CampaignSaveData & campaignData = CampaignSaveData::Get();

        const std::vector<ScenarioData> & scenarios = CampaignData::getCampaignData( campaignData.getCampaignID() ).getAllScenarios();
        const int scenarioId = campaignData.getCurrentScenarioID();
        assert( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() );

        if ( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() ) {
            return scenarios[scenarioId].getLossCondition();
        }

        return ScenarioLossCondition::STANDARD;
    }
}
