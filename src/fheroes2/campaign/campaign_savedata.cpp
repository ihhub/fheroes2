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

#include "campaign_savedata.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <map>
#include <utility>

#include "army.h"
#include "campaign_data.h"
#include "difficulty.h"
#include "serialize.h"

namespace Campaign
{
    CampaignSaveData & Campaign::CampaignSaveData::Get()
    {
        static CampaignSaveData instance;
        return instance;
    }

    void CampaignSaveData::addCampaignAward( const int awardID )
    {
        _obtainedCampaignAwards.emplace_back( awardID );
    }

    void CampaignSaveData::removeCampaignAward( const int awardID )
    {
        _obtainedCampaignAwards.erase( std::remove( _obtainedCampaignAwards.begin(), _obtainedCampaignAwards.end(), awardID ), _obtainedCampaignAwards.end() );
    }

    void CampaignSaveData::setEnemyDefeatedAward( const int heroId )
    {
        const ScenarioInfoId & currentScenarioInfo = getCurrentScenarioInfoId();
        const std::vector<CampaignAwardData> obtainableAwards = CampaignAwardData::getCampaignAwardData( currentScenarioInfo );

        for ( const auto & obtainableAward : obtainableAwards ) {
            const int32_t awardType = obtainableAward._type;

            if ( awardType == CampaignAwardData::AwardType::TYPE_DEFEAT_ENEMY_HERO ) {
                if ( obtainableAward._subType == heroId ) {
                    addCampaignAward( obtainableAward._id );
                }
                break;
            }
        }
    }

    void CampaignSaveData::setCurrentScenarioInfo( const ScenarioInfoId & scenarioInfoId, const int32_t bonusId /* = -1 */ )
    {
        assert( scenarioInfoId.campaignId >= 0 && scenarioInfoId.scenarioId >= 0 );

        _currentScenarioInfoId = scenarioInfoId;
        _currentScenarioBonusId = bonusId;
    }

    void CampaignSaveData::addCurrentMapToFinished()
    {
        // Check for a duplicate
        if ( std::find( _finishedMaps.begin(), _finishedMaps.end(), _currentScenarioInfoId ) != _finishedMaps.end() ) {
            return;
        }

        _finishedMaps.emplace_back( _currentScenarioInfoId );
        _bonusesForFinishedMaps.emplace_back( _currentScenarioBonusId );

        assert( _finishedMaps.size() == _bonusesForFinishedMaps.size() );
    }

    void CampaignSaveData::addDaysPassed( const uint32_t days )
    {
        _daysPassed += days;
    }

    void CampaignSaveData::reset()
    {
        _finishedMaps.clear();
        _bonusesForFinishedMaps.clear();
        _obtainedCampaignAwards.clear();
        _carryOverTroops.clear();
        _currentScenarioInfoId = { -1, -1 };
        _currentScenarioBonusId = -1;
        _daysPassed = 0;
        _difficulty = CampaignDifficulty::Normal;
    }

    void CampaignSaveData::setCarryOverTroops( const Troops & troops )
    {
        _carryOverTroops.clear();

        for ( size_t i = 0; i < troops.Size(); ++i ) {
            _carryOverTroops.emplace_back( *troops.GetTroop( i ) );
        }
    }

    const ScenarioInfoId & CampaignSaveData::getLastCompletedScenarioInfoID() const
    {
        assert( !_finishedMaps.empty() );
        return _finishedMaps.back();
    }

    uint32_t CampaignSaveData::getCampaignDifficultyPercent() const
    {
        switch ( _difficulty ) {
        case CampaignDifficulty::Easy:
            return 125;
        case CampaignDifficulty::Normal:
            // Original campaign difficulty.
            return 100;
        case CampaignDifficulty::Hard:
            return 75;
        default:
            // Did you add a new campaign difficulty? Add the logic above!
            assert( 0 );
            return 100;
        }
    }

    std::vector<Campaign::CampaignAwardData> CampaignSaveData::getObtainedCampaignAwards() const
    {
        std::vector<Campaign::CampaignAwardData> obtainedAwards;

        for ( size_t i = 0; i < _finishedMaps.size(); ++i ) {
            const std::vector<Campaign::CampaignAwardData> awards = Campaign::CampaignAwardData::getCampaignAwardData( _finishedMaps[i] );

            for ( size_t j = 0; j < awards.size(); ++j ) {
                if ( std::find( _obtainedCampaignAwards.begin(), _obtainedCampaignAwards.end(), awards[j]._id ) != _obtainedCampaignAwards.end() )
                    obtainedAwards.emplace_back( awards[j] );
            }
        }

        const std::vector<Campaign::CampaignAwardData> extraAwards = Campaign::CampaignAwardData::getExtraCampaignAwardData( _currentScenarioInfoId.campaignId );
        for ( const Campaign::CampaignAwardData & award : extraAwards ) {
            if ( std::find( _obtainedCampaignAwards.begin(), _obtainedCampaignAwards.end(), award._id ) != _obtainedCampaignAwards.end() )
                obtainedAwards.emplace_back( award );
        }

        return obtainedAwards;
    }

    OStreamBase & operator<<( OStreamBase & stream, const CampaignSaveData & data )
    {
        return stream << data._currentScenarioInfoId.campaignId << data._currentScenarioInfoId.scenarioId << data._currentScenarioBonusId << data._finishedMaps
                      << data._bonusesForFinishedMaps << data._daysPassed << data._obtainedCampaignAwards << data._carryOverTroops << data._difficulty;
    }

    IStreamBase & operator>>( IStreamBase & stream, CampaignSaveData & data )
    {
        stream >> data._currentScenarioInfoId.campaignId >> data._currentScenarioInfoId.scenarioId >> data._currentScenarioBonusId >> data._finishedMaps
            >> data._bonusesForFinishedMaps;

        // Make sure that the number of elements in the vector of map bonuses matches the number of elements in the vector of finished maps
        data._bonusesForFinishedMaps.resize( data._finishedMaps.size(), -1 );

        return stream >> data._daysPassed >> data._obtainedCampaignAwards >> data._carryOverTroops >> data._difficulty;
    }

    ScenarioVictoryCondition getCurrentScenarioVictoryCondition()
    {
        const CampaignSaveData & campaignData = CampaignSaveData::Get();

        const std::vector<ScenarioData> & scenarios = CampaignData::getCampaignData( campaignData.getCampaignID() ).getAllScenarios();
        const int scenarioId = campaignData.getCurrentScenarioID();
        assert( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() );

        if ( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() ) {
            return scenarios[scenarioId].getVictoryCondition();
        }

        return ScenarioVictoryCondition::STANDARD;
    }

    ScenarioLossCondition getCurrentScenarioLossCondition()
    {
        const CampaignSaveData & campaignData = CampaignSaveData::Get();

        const std::vector<ScenarioData> & scenarios = CampaignData::getCampaignData( campaignData.getCampaignID() ).getAllScenarios();
        const int scenarioId = campaignData.getCurrentScenarioID();
        assert( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() );

        if ( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() ) {
            return scenarios[scenarioId].getLossCondition();
        }

        return ScenarioLossCondition::STANDARD;
    }

    std::optional<int> getCurrentScenarioDifficultyLevel()
    {
        static const std::map<std::pair<int, int>, int> adjustedDifficultyLevels = { // Roland
                                                                                     { { ROLAND_CAMPAIGN, 1 }, Difficulty::EASY },
                                                                                     // Archibald
                                                                                     { { ARCHIBALD_CAMPAIGN, 1 }, Difficulty::EASY },
                                                                                     // Descendants
                                                                                     { { DESCENDANTS_CAMPAIGN, 0 }, Difficulty::EASY },
                                                                                     { { DESCENDANTS_CAMPAIGN, 5 }, Difficulty::HARD },
                                                                                     // Wizard's Isle
                                                                                     { { WIZARDS_ISLE_CAMPAIGN, 3 }, Difficulty::HARD },
                                                                                     // Voyage Home
                                                                                     { { VOYAGE_HOME_CAMPAIGN, 0 }, Difficulty::EASY },
                                                                                     // Price of Loyalty
                                                                                     { { PRICE_OF_LOYALTY_CAMPAIGN, 0 }, Difficulty::EASY },
                                                                                     { { PRICE_OF_LOYALTY_CAMPAIGN, 5 }, Difficulty::HARD },
                                                                                     { { PRICE_OF_LOYALTY_CAMPAIGN, 6 }, Difficulty::HARD },
                                                                                     { { PRICE_OF_LOYALTY_CAMPAIGN, 7 }, Difficulty::EXPERT } };

        const CampaignSaveData & campaignData = CampaignSaveData::Get();

        const auto iter = adjustedDifficultyLevels.find( { campaignData.getCampaignID(), campaignData.getCurrentScenarioID() } );
        if ( iter == adjustedDifficultyLevels.end() ) {
            return {};
        }

        return iter->second;
    }
}
