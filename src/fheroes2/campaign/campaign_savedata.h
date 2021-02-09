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

        const std::vector<std::string> & getEarnedCampaignAwards() const
        {
            return _earnedCampaignAwards;
        }

        int getCampaignID() const
        {
            return _campaignID;
        }

        int getCurrentScenarioID() const
        {
            return _currentScenarioID;
        }

        int getLastCompletedScenarioID() const
        {
            return _finishedMaps.back();
        }

        bool isStarting() const
        {
            return _finishedMaps.empty();
        }

        void setCurrentScenarioBonus( const ScenarioBonusData & bonus );
        void setCurrentScenarioID( const int scenarioID );
        void setCampaignID( const int campaignID );
        void addCurrentMapToFinished();
        void addCampaignAward( const std::string & award );
        void reset();

        static CampaignSaveData & Get();

    private:
        friend StreamBase & operator<<( StreamBase & msg, const CampaignSaveData & data );
        friend StreamBase & operator>>( StreamBase & msg, CampaignSaveData & data );

        std::vector<int> _finishedMaps;
        std::vector<std::string> _earnedCampaignAwards; // should have its own data format
        int _currentScenarioID;
        int _campaignID;
        ScenarioBonusData _currentScenarioBonus;
    };
}

#endif
