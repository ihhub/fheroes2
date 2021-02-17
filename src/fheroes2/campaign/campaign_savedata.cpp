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
#include "game.h"
#include "serialize.h"
#include "settings.h"
#include <algorithm>
#include <cassert>

namespace Campaign
{
    CampaignSaveData::CampaignSaveData()
        : _finishedMaps()
        , _earnedCampaignAwards()
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

    void CampaignSaveData::addCampaignAward( const std::string & award )
    {
        _earnedCampaignAwards.emplace_back( award );
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
        _earnedCampaignAwards.clear();
        _currentScenarioID = 0;
        _campaignID = 0;
        _daysPassed = 0;
    }

    StreamBase & operator<<( StreamBase & msg, const Campaign::CampaignSaveData & data )
    {
        return msg << data._earnedCampaignAwards << data._currentScenarioID << data._currentScenarioBonus << data._finishedMaps << data._campaignID << data._daysPassed;
    }

    StreamBase & operator>>( StreamBase & msg, Campaign::CampaignSaveData & data )
    {
        msg >> data._earnedCampaignAwards >> data._currentScenarioID >> data._currentScenarioBonus >> data._finishedMaps;

        if ( Game::GetLoadVersion() >= FORMAT_VERSION_091_RELEASE )
            msg >> data._campaignID >> data._daysPassed;

        return msg;
    }
}
