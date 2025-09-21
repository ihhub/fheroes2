/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2025                                             *
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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "campaign_scenariodata.h"

namespace Maps
{
    struct FileInfo;
}

namespace Campaign
{
    class CampaignData
    {
    public:
        int getCampaignID() const
        {
            return _campaignID;
        }

        const std::vector<ScenarioData> & getAllScenarios() const
        {
            return _scenarios;
        }

        static const std::vector<ScenarioInfoId> & getScenariosAfter( const ScenarioInfoId & scenarioInfo );
        std::vector<ScenarioInfoId> getStartingScenarios() const;

        bool isAllCampaignMapsPresent() const;
        bool isLastScenario( const Campaign::ScenarioInfoId & scenarioInfoId ) const;

        void setCampaignID( const int campaignID )
        {
            _campaignID = campaignID;
        }

        void setCampaignScenarios( std::vector<ScenarioData> && scenarios );

        static const CampaignData & getCampaignData( const int campaignID );

        // Some scenarios have different gameplay conditions like unions which are not specified within the map file itself.
        static void updateScenarioGameplayConditions( const Campaign::ScenarioInfoId & scenarioInfoId, Maps::FileInfo & mapInfo );

    private:
        int _campaignID{ 0 };
        std::string _campaignName;
        std::vector<ScenarioData> _scenarios;

        bool isStartingScenario( const ScenarioInfoId & scenarioInfo ) const;
    };

    struct CampaignAwardData
    {
    public:
        enum AwardType : int32_t
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
        int32_t _id{ 0 };
        int32_t _type{ 0 };
        int32_t _subType{ 0 };
        int32_t _amount{ 0 };
        int32_t _startScenarioID{ 0 };
        std::string _customName;

        CampaignAwardData( const int32_t id, const int32_t type, const int32_t subType );
        CampaignAwardData( const int32_t id, const int32_t type, const int32_t subType, const int32_t amount );
        CampaignAwardData( const int32_t id, const int32_t type, const int32_t subType, std::string customName );
        CampaignAwardData( const int32_t id, const int32_t type, const int32_t subType, const int32_t amount, const int32_t startScenarioID,
                           std::string customName = std::string() );

        std::string getName() const;

        std::string getDescription() const;

        static std::vector<Campaign::CampaignAwardData> getCampaignAwardData( const ScenarioInfoId & scenarioInfo );
        static std::vector<Campaign::CampaignAwardData> getExtraCampaignAwardData( const int campaignID );

        static const char * getAllianceJoiningMessage( const int monsterId );
        static const char * getAllianceFleeingMessage( const int monsterId );
        static const char * getBaneFleeingMessage( const int monsterId );
    };
}
