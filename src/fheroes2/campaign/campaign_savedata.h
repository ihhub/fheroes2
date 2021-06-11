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

#ifndef H2CAMPAIGN_SAVEDATA_H
#define H2CAMPAIGN_SAVEDATA_H

#include "army.h"
#include "campaign_data.h"
#include "campaign_scenariodata.h"

namespace Campaign
{
    class CampaignSaveData
    {
    public:
        CampaignSaveData();

        const ScenarioBonusData & getCurrentScenarioBonus() const
        {
            return _currentScenarioBonus;
        }

        const std::vector<int> & getFinishedMaps() const
        {
            return _finishedMaps;
        }

        int getCampaignID() const
        {
            return _campaignID;
        }

        int getCurrentScenarioID() const
        {
            return _currentScenarioID;
        }

        // Make sure that this is not the first scenario in the campaign. Please call isStarting to verify this.
        int getLastCompletedScenarioID() const;

        bool isStarting() const
        {
            return _finishedMaps.empty();
        }

        uint32_t getDaysPassed() const
        {
            return _daysPassed;
        }

        const std::vector<Troop> & getCarryOverTroops() const
        {
            return _carryOverTroops;
        }

        const std::vector<Campaign::CampaignAwardData> getObtainedCampaignAwards() const;

        void setCurrentScenarioBonus( const ScenarioBonusData & bonus );
        void setCurrentScenarioID( const int scenarioID );
        void setCampaignID( const int campaignID );
        void addCurrentMapToFinished();
        void addCampaignAward( const int awardID );
        void setCarryOverTroops( const Troops & troops );
        void reset();
        void addDaysPassed( const uint32_t days );

        static CampaignSaveData & Get();

    private:
        friend StreamBase & operator<<( StreamBase & msg, const CampaignSaveData & data );
        friend StreamBase & operator>>( StreamBase & msg, CampaignSaveData & data );

        std::vector<int> _finishedMaps;
        std::vector<int> _obtainedCampaignAwards;
        std::vector<Troop> _carryOverTroops;
        int _currentScenarioID;
        int _campaignID;
        uint32_t _daysPassed;
        ScenarioBonusData _currentScenarioBonus;
    };
}

#endif
