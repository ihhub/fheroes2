/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include <cstdint>
#include <string>
#include <vector>

#include "maps_fileinfo.h"

class IStreamBase;
class OStreamBase;

namespace Video
{
    enum class VideoAction : int;
}

namespace Campaign
{
    enum CampaignID
    {
        ROLAND_CAMPAIGN = 0,
        ARCHIBALD_CAMPAIGN = 1,
        PRICE_OF_LOYALTY_CAMPAIGN = 2,
        DESCENDANTS_CAMPAIGN = 3,
        WIZARDS_ISLE_CAMPAIGN = 4,
        VOYAGE_HOME_CAMPAIGN = 5
    };

    enum class ScenarioVictoryCondition : int
    {
        STANDARD = 0, // standard map's defined victory condition
        CAPTURE_DRAGON_CITY = 1,
        OBTAIN_ULTIMATE_CROWN = 2,
        OBTAIN_SPHERE_NEGATION = 3
    };

    enum class ScenarioLossCondition : int
    {
        STANDARD = 0, // standard map's defined loss condition
        LOSE_ALL_SORCERESS_VILLAGES = 1
    };

    struct ScenarioInfoId
    {
        ScenarioInfoId() = default;

        ScenarioInfoId( const int campaignId_, const int scenarioId_ )
            : campaignId( campaignId_ )
            , scenarioId( scenarioId_ )
        {
            // Do nothing.
        }

        bool operator==( const ScenarioInfoId & info ) const
        {
            return campaignId == info.campaignId && scenarioId == info.scenarioId;
        }

        bool operator!=( const ScenarioInfoId & info ) const
        {
            return !operator==( info );
        }

        friend OStreamBase & operator<<( OStreamBase & stream, const ScenarioInfoId & data );
        friend IStreamBase & operator>>( IStreamBase & stream, ScenarioInfoId & data );

        int campaignId{ -1 };

        int scenarioId{ -1 };
    };

    struct ScenarioBonusData
    {
    public:
        enum BonusType : int32_t
        {
            RESOURCES = 0,
            ARTIFACT,
            TROOP,
            SPELL,
            STARTING_RACE,
            SKILL_PRIMARY,
            SKILL_SECONDARY,
            STARTING_RACE_AND_ARMY
        };

        int32_t _type;
        int32_t _subType;
        int32_t _amount;
        int32_t _artifactSpellId; // Spell ID of a spell scroll

        ScenarioBonusData();
        ScenarioBonusData( const int32_t type, const int32_t subType, const int32_t amount );
        ScenarioBonusData( const int32_t type, const int32_t subType, const int32_t amount, const int32_t spellId );

        std::string getName() const;

        std::string getDescription() const;

        static std::vector<Campaign::ScenarioBonusData> getCampaignBonusData( const ScenarioInfoId & scenarioInfo );
    };

    struct ScenarioIntroVideoInfo
    {
        std::string fileName;
        Video::VideoAction action;
    };

    using VideoSequence = std::vector<ScenarioIntroVideoInfo>;

    class ScenarioData
    {
    public:
        ScenarioData() = delete;
        ScenarioData( const ScenarioInfoId & scenarioInfo, std::vector<ScenarioInfoId> && nextScenarios, const std::string & fileName, const std::string & scenarioName,
                      const std::string & description, const VideoSequence & startScenarioVideoPlayback, const VideoSequence & endScenarioVideoPlayback,
                      const ScenarioVictoryCondition victoryCondition = ScenarioVictoryCondition::STANDARD,
                      const ScenarioLossCondition lossCondition = ScenarioLossCondition::STANDARD );

        const std::vector<ScenarioInfoId> & getNextScenarios() const
        {
            return _nextScenarios;
        }

        const std::vector<ScenarioBonusData> & getBonuses() const
        {
            return _bonuses;
        }

        int getScenarioID() const
        {
            return _scenarioInfo.scenarioId;
        }

        int getCampaignId() const
        {
            return _scenarioInfo.campaignId;
        }

        const ScenarioInfoId & getScenarioInfoId() const
        {
            return _scenarioInfo;
        }

        const char * getScenarioName() const;

        const char * getDescription() const;

        ScenarioVictoryCondition getVictoryCondition() const
        {
            return _victoryCondition;
        }

        ScenarioLossCondition getLossCondition() const
        {
            return _lossCondition;
        }

        const std::vector<ScenarioIntroVideoInfo> & getStartScenarioVideoPlayback() const
        {
            return _startScenarioVideoPlayback;
        }

        const std::vector<ScenarioIntroVideoInfo> & getEndScenarioVideoPlayback() const
        {
            return _endScenarioVideoPlayback;
        }

        bool isMapFilePresent() const;
        Maps::FileInfo loadMap() const;

    private:
        ScenarioInfoId _scenarioInfo;
        std::vector<ScenarioInfoId> _nextScenarios;
        std::vector<ScenarioBonusData> _bonuses;
        std::string _fileName;
        // Note: There are inconsistencies with the content of the map file in regards to the map name and description, so we'll be getting them from somewhere else
        std::string _scenarioName;
        std::string _description;
        ScenarioVictoryCondition _victoryCondition;
        ScenarioLossCondition _lossCondition;

        VideoSequence _startScenarioVideoPlayback;
        VideoSequence _endScenarioVideoPlayback;
    };

    const char * getCampaignName( const int campaignId );
}

#endif
