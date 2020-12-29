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

#ifndef H2CAMPAIGN_DATA_H
#define H2CAMPAIGN_DATA_H

#include "gamedefs.h"
#include "heroes_base.h"

namespace Campaign
{
    struct ScenarioBonusData
    {
    public:
        enum
        {
            RESOURCES,
            ARTIFACT,
            TROOP
        };

        uint32_t _type;
        uint32_t _subType;
        uint32_t _amount;

        ScenarioBonusData();
        ScenarioBonusData( uint32_t type, uint32_t subType, uint32_t amount );

        friend StreamBase & operator<<( StreamBase & msg, const ScenarioBonusData & data );
        friend StreamBase & operator>>( StreamBase & msg, ScenarioBonusData & data );

        const std::string ToString() const;
    };

    class CampaignData
    {
    public:
        CampaignData();

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

        void setCurrentScenarioBonus( const ScenarioBonusData & bonus );
        void setCurrentScenarioID( const int scenarioID );
        void setCampaignID( const int campaignID );
        void addCurrentMapToFinished();
        void addCampaignAward( const std::string & award );

    private:
        friend StreamBase & operator<<( StreamBase & msg, const CampaignData & data );
        friend StreamBase & operator>>( StreamBase & msg, CampaignData & data );

        std::vector<int> _finishedMaps;
        std::vector<std::string> _earnedCampaignAwards; // should have its own data format
        int _currentScenarioID;
        int _campaignID;
        ScenarioBonusData _currentScenarioBonus;
    };
}

#endif
