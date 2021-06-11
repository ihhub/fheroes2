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

#include "campaign_savedata.h"
#include "artifact.h"
#include "game.h"
#include "heroes.h"
#include "serialize.h"
#include "settings.h"
#include "translations.h"
#include <algorithm>
#include <cassert>

namespace Campaign
{
    CampaignSaveData::CampaignSaveData()
        : _finishedMaps()
        , _obtainedCampaignAwards()
        , _currentScenarioID( 0 )
        , _campaignID( 0 )
        , _daysPassed( 0 )
        , _currentScenarioBonus()
    {}

    CampaignSaveData & Campaign::CampaignSaveData::Get()
    {
        static CampaignSaveData instance;
        return instance;
    }

    void CampaignSaveData::addCampaignAward( const int awardID )
    {
        _obtainedCampaignAwards.emplace_back( awardID );
    }

    void CampaignSaveData::setCurrentScenarioBonus( const ScenarioBonusData & bonus )
    {
        _currentScenarioBonus = bonus;
    }

    void CampaignSaveData::setCurrentScenarioID( const int scenarioID )
    {
        _currentScenarioID = scenarioID;
    }

    void CampaignSaveData::setCampaignID( const int campaignID )
    {
        _campaignID = campaignID;
    }

    void CampaignSaveData::addCurrentMapToFinished()
    {
        const bool isNotDuplicate = std::find( _finishedMaps.begin(), _finishedMaps.end(), _currentScenarioID ) == _finishedMaps.end();
        if ( isNotDuplicate )
            _finishedMaps.emplace_back( _currentScenarioID );
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
        _currentScenarioID = 0;
        _campaignID = 0;
        _daysPassed = 0;
    }

    void CampaignSaveData::setCarryOverTroops( const Troops & troops )
    {
        _carryOverTroops.clear();

        for ( size_t i = 0; i < troops.Size(); ++i ) {
            _carryOverTroops.emplace_back( *troops.GetTroop( i ) );
        }
    }

    int CampaignSaveData::getLastCompletedScenarioID() const
    {
        assert( !_finishedMaps.empty() );
        return _finishedMaps.back();
    }

    const std::vector<Campaign::CampaignAwardData> CampaignSaveData::getObtainedCampaignAwards() const
    {
        std::vector<Campaign::CampaignAwardData> obtainedAwards;

        for ( size_t i = 0; i < _finishedMaps.size(); ++i ) {
            const std::vector<Campaign::CampaignAwardData> awards = Campaign::CampaignAwardData::getCampaignAwardData( _campaignID, _finishedMaps[i] );

            for ( size_t j = 0; j < awards.size(); ++j ) {
                if ( std::find( _obtainedCampaignAwards.begin(), _obtainedCampaignAwards.end(), awards[j]._id ) != _obtainedCampaignAwards.end() )
                    obtainedAwards.emplace_back( awards[j] );
            }
        }

        return obtainedAwards;
    }

    StreamBase & operator<<( StreamBase & msg, const Campaign::CampaignSaveData & data )
    {
        return msg << data._currentScenarioID << data._currentScenarioBonus << data._finishedMaps << data._campaignID << data._daysPassed << data._obtainedCampaignAwards
                   << data._carryOverTroops;
    }

    StreamBase & operator>>( StreamBase & msg, Campaign::CampaignSaveData & data )
    {
        const int loadVersion = Game::GetLoadVersion();

        if ( loadVersion < FORMAT_VERSION_093_RELEASE ) {
            std::vector<std::string> tempOldObtainedCampaignAwards;
            msg >> tempOldObtainedCampaignAwards;
        }

        msg >> data._currentScenarioID >> data._currentScenarioBonus >> data._finishedMaps;

        if ( loadVersion >= FORMAT_VERSION_091_RELEASE )
            msg >> data._campaignID >> data._daysPassed;

        if ( loadVersion >= FORMAT_VERSION_093_RELEASE )
            msg >> data._obtainedCampaignAwards >> data._carryOverTroops;

        return msg;
    }
}
