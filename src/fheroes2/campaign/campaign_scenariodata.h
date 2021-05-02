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

#ifndef H2CAMPAIGN_SCENARIODATA_H
#define H2CAMPAIGN_SCENARIODATA_H

#include "maps_fileinfo.h"

namespace Campaign
{
    enum ScenarioIcon
    {
        SCENARIOICON_CLEARED = 10,
        SCENARIOICON_AVAILABLE = 11,
        SCENARIOICON_UNAVAILABLE = 12,
        SCENARIOICON_GOOD_SELECTED = 14,
        SCENARIOICON_EVIL_SELECTED = 17,
    };

    enum CampaignID
    {
        ROLAND_CAMPAIGN = 0,
        ARCHIBALD_CAMPAIGN = 1
    };

    enum class ScenarioVictoryCondition : int
    {
        STANDARD = 0, // standard map's defined victory condition
        CAPTURE_DRAGON_CITY = 1
    };

    struct ScenarioBonusData
    {
    public:
        enum BonusType
        {
            RESOURCES = 0,
            ARTIFACT,
            TROOP,
            SPELL,
            STARTING_RACE,
            SKILL
        };

        uint32_t _type;
        uint32_t _subType;
        uint32_t _amount;

        ScenarioBonusData();
        ScenarioBonusData( uint32_t type, uint32_t subType, uint32_t amount );

        friend StreamBase & operator<<( StreamBase & msg, const ScenarioBonusData & data );
        friend StreamBase & operator>>( StreamBase & msg, ScenarioBonusData & data );

        std::string ToString() const;

        static std::vector<Campaign::ScenarioBonusData> getCampaignBonusData( const int campaignID, const int scenarioID );
    };

    class ScenarioData
    {
    public:
        ScenarioData() = delete;
        ScenarioData( int scenarioID, const std::vector<int> & nextMaps, const std::vector<Campaign::ScenarioBonusData> & bonuses, const std::string & fileName,
                      const std::string & description, const ScenarioVictoryCondition victoryCondition = ScenarioVictoryCondition::STANDARD );

        const std::vector<int> & getNextMaps() const
        {
            return _nextMaps;
        }

        const std::vector<ScenarioBonusData> & getBonuses() const
        {
            return _bonuses;
        }

        const std::string & getFileName() const
        {
            return _fileName;
        }

        int getScenarioID() const
        {
            return _scenarioID;
        }

        const std::string & getDescription() const
        {
            return _description;
        }

        ScenarioVictoryCondition getVictoryCondition() const
        {
            return _victoryCondition;
        }

        bool isMapFilePresent() const;
        Maps::FileInfo loadMap() const;

    private:
        int _scenarioID;
        std::vector<int> _nextMaps;
        std::vector<ScenarioBonusData> _bonuses;
        std::string _fileName;
        std::string _description; // at least for campaign maps, the description isn't obtained from the map's description, so we have to write one manually
        ScenarioVictoryCondition _victoryCondition;
    };
}

#endif
