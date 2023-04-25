/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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

#ifndef H2CAMPAIGN_SAVEDATA_H
#define H2CAMPAIGN_SAVEDATA_H

#include <cstdint>
#include <vector>

#include "army_troop.h"
#include "campaign_scenariodata.h"

class StreamBase;

class Troops;

namespace Campaign
{
    struct CampaignAwardData;

    enum CampaignDifficulty : int32_t
    {
        Easy = -1,
        Normal = 0,
        Hard = 1
    };

    class CampaignSaveData
    {
    public:
        const std::vector<ScenarioInfoId> & getFinishedMaps() const
        {
            return _finishedMaps;
        }

        int getCampaignID() const
        {
            return _currentScenarioInfoId.campaignId;
        }

        int getCurrentScenarioID() const
        {
            return _currentScenarioInfoId.scenarioId;
        }

        const ScenarioInfoId & getCurrentScenarioInfoId() const
        {
            return _currentScenarioInfoId;
        }

        // Make sure that this is not the first scenario in the campaign. Please call isStarting to verify this.
        const ScenarioInfoId & getLastCompletedScenarioInfoID() const;

        bool isStarting() const
        {
            return _finishedMaps.empty();
        }

        uint32_t getDaysPassed() const
        {
            return _daysPassed;
        }

        int32_t getDifficulty() const
        {
            return _difficulty;
        }

        void setDifficulty( const int32_t difficulty )
        {
            _difficulty = difficulty;
        }

        // Get the campaign difficulty in percents for rating calculations.
        uint32_t getCampaignDifficultyPercent() const;

        const std::vector<Troop> & getCarryOverTroops() const
        {
            return _carryOverTroops;
        }

        std::vector<Campaign::CampaignAwardData> getObtainedCampaignAwards() const;

        void setCurrentScenarioBonus( const ScenarioBonusData & bonus );
        void setCurrentScenarioInfoId( const ScenarioInfoId & scenarioInfoId );
        void addCurrentMapToFinished();
        void addCampaignAward( const int awardID );
        void setCarryOverTroops( const Troops & troops );
        void reset();
        void addDaysPassed( const uint32_t days );
        void removeCampaignAward( const int awardID );

        void removeAllAwards()
        {
            _obtainedCampaignAwards.clear();
        }

        static CampaignSaveData & Get();

    private:
        friend StreamBase & operator<<( StreamBase & msg, const CampaignSaveData & data );
        friend StreamBase & operator>>( StreamBase & msg, CampaignSaveData & data );

        CampaignSaveData() = default;

        std::vector<ScenarioInfoId> _finishedMaps;
        std::vector<int> _obtainedCampaignAwards;
        std::vector<Troop> _carryOverTroops;

        ScenarioInfoId _currentScenarioInfoId;

        uint32_t _daysPassed{ 0 };
        int32_t _difficulty{ CampaignDifficulty::Normal };

        ScenarioBonusData _currentScenarioBonus;
    };

    // Call this function only when playing campaign scenario.
    ScenarioVictoryCondition getCurrentScenarioVictoryCondition();

    // Call this function only when playing campaign scenario.
    ScenarioLossCondition getCurrentScenarioLossCondition();
}

#endif
