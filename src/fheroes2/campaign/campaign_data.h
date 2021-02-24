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

#ifndef H2CAMPAIGN_DATA_H
#define H2CAMPAIGN_DATA_H

#include "campaign_scenariodata.h"
#include "gamedefs.h"
#include "maps_fileinfo.h"
#include "serialize.h"

namespace Campaign
{
    class CampaignData
    {
    public:
        CampaignData();

        const std::string & getCampaignDescription() const
        {
            return _campaignDescription;
        }

        int getCampaignID() const
        {
            return _campaignID;
        }

        bool isGoodCampaign() const
        {
            return _isGoodCampaign;
        }

        const std::vector<ScenarioData> & getAllScenarios() const
        {
            return _scenarios;
        }

        std::vector<int> getScenariosBefore( const int scenarioID ) const;
        const std::vector<int> & getScenariosAfter( const int scenarioID ) const;
        std::vector<int> getStartingScenarios() const;

        bool isAllCampaignMapsPresent() const;
        bool isLastScenario( const int scenarioID ) const;
        bool isStartingScenario( const int scenarioID ) const;

        void setCampaignID( const int campaignID );
        void setCampaignAlignment( const bool isGoodCampaign );
        void setCampaignDescription( const std::string & campaignDescription );
        void setCampaignScenarios( const std::vector<ScenarioData> & scenarios );

    private:
        int _campaignID;
        bool _isGoodCampaign;
        std::string _campaignDescription;
        std::vector<ScenarioData> _scenarios;
    };
}

#endif
