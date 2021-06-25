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
        void setCampaignDescription( const std::string & campaignDescription );
        void setCampaignScenarios( const std::vector<ScenarioData> & scenarios );

        static const CampaignData & getCampaignData( const int campaignID );

    private:
        int _campaignID;
        std::string _campaignDescription;
        std::vector<ScenarioData> _scenarios;
    };

    struct CampaignAwardData
    {
    public:
        enum AwardType : int
        {
            TYPE_CREATURE_CURSE, // eg: dwarf bane
            TYPE_CREATURE_ALLIANCE, // eg: dwarf alliance
            TYPE_GET_ARTIFACT, // eg: ultimate crown
            TYPE_GET_SPELL, // eg: guardian spell in wizard's isle
            TYPE_CARRY_OVER_FORCES, // eg: the gauntlet
            TYPE_HIREABLE_HERO, // eg: sorceress guild
            TYPE_DEFEAT_ENEMY_HERO, // eg: corlagon defeated
            TYPE_RESOURCE_BONUS, // eg: wood bonus in price of loyalty
        };

        // NOTE: Carry over forces shouldn't use these other than id, type and startScenarioID
        // IDs are here so that we just have to store an int instead of the entire award data in a campaign save data
        // also usable when we have to remove specific awards when completing a mission (PoL campaign)
        int _id;
        uint32_t _type;
        uint32_t _subType;
        uint32_t _amount;
        uint32_t _startScenarioID;
        std::string _customName;

        CampaignAwardData();
        CampaignAwardData( int id, uint32_t type, uint32_t subType );
        CampaignAwardData( int id, uint32_t type, uint32_t subType, uint32_t amount );
        CampaignAwardData( int id, uint32_t type, uint32_t subType, const std::string & customName );
        CampaignAwardData( int id, uint32_t type, uint32_t subType, uint32_t amount, int startScenarioID, const std::string & customName = std::string() );

        std::string ToString() const;

        static std::vector<Campaign::CampaignAwardData> getCampaignAwardData( const int campaignID, const int scenarioID );
    };
}

#endif
