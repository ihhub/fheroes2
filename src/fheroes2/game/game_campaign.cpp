/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "army.h"
#include "army_troop.h"
#include "artifact.h"
#include "audio.h"
#include "audio_manager.h"
#include "battle.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_credits.h"
#include "game_hotkeys.h"
#include "game_io.h"
#include "game_mode.h"
#include "game_over.h"
#include "game_video.h"
#include "game_video_type.h"
#include "heroes.h"
#include "highscores.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "monster.h"
#include "mus.h"
#include "pal.h"
#include "players.h"
#include "race.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_campaign.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    enum ScenarioIcon : uint32_t
    {
        SCENARIO_ICON_CLEARED = 0,
        SCENARIO_ICON_AVAILABLE = 1,
        SCENARIO_ICON_UNAVAILABLE = 2,
    };

    const int betrayalScenarioId = 4;

    std::vector<fheroes2::Point> getCampaignIconOffsets( const int campaignId, const bool archibaldLowerBetrayalBranch = false )
    {
        switch ( campaignId ) {
        case Campaign::ROLAND_CAMPAIGN:
            return { { 0, 1 }, { 2, 1 }, { 3, 0 }, { 4, 1 }, { 6, 1 }, { 8, 1 }, { 10, 2 }, { 10, 0 }, { 12, 1 }, { 14, 1 }, { 6, 2 } };
        case Campaign::ARCHIBALD_CAMPAIGN:
            if ( archibaldLowerBetrayalBranch ) {
                return { { 0, 1 }, { 2, 1 }, { 4, 0 }, { 4, 2 }, { 6, 1 }, { 8, 1 }, { 9, 0 }, { 10, 1 }, { 12, 0 }, { 12, 2 }, { 14, 1 }, { 6, 2 } };
            }
            else {
                return { { 0, 1 }, { 2, 1 }, { 4, 0 }, { 4, 2 }, { 6, 1 }, { 8, 1 }, { 9, 0 }, { 10, 1 }, { 12, 0 }, { 12, 2 }, { 14, 1 }, { 6, 0 } };
            }
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
            return { { 0, 0 }, { 2, 0 }, { 4, 1 }, { 4, 0 }, { 6, 1 }, { 7, 0 }, { 9, 1 }, { 10, 0 } };
        case Campaign::DESCENDANTS_CAMPAIGN:
            return { { 0, 1 }, { 2, 1 }, { 4, 0 }, { 4, 2 }, { 6, 1 }, { 8, 2 }, { 8, 0 }, { 10, 1 } };
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
            return { { 0, 0 }, { 2, 0 }, { 4, 1 }, { 6, 0 } };
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            return { { 0, 0 }, { 2, 0 }, { 4, 0 }, { 4, 1 } };
        default:
            // Implementing a new campaign? Add a new case!
            assert( 0 );
            return {};
        }
    }

    bool isBetrayalScenario( const Campaign::ScenarioInfoId & scenarioInfoId )
    {
        // Betrayal scenario is a special scenario of switching between campaigns. Next scenarios after this must have different campaign ID.
        for ( const Campaign::ScenarioInfoId & nextScenario : Campaign::CampaignData::getScenariosAfter( scenarioInfoId ) ) {
            if ( nextScenario.campaignId != scenarioInfoId.campaignId ) {
                return true;
            }
        }

        return false;
    }

    void getScenarioIconId( const Campaign::ScenarioInfoId & scenarioInfoId, int & iconsId, uint32_t & iconStatusOffset, uint32_t & selectedIconIdx )
    {
        iconsId = ICN::UNKNOWN;
        iconStatusOffset = 0;
        selectedIconIdx = 0;

        switch ( scenarioInfoId.campaignId ) {
        case Campaign::ROLAND_CAMPAIGN:
            iconsId = ICN::CAMPXTRG;
            iconStatusOffset = 10;
            selectedIconIdx = isBetrayalScenario( scenarioInfoId ) ? 17 : 14;
            return;
        case Campaign::ARCHIBALD_CAMPAIGN:
            iconsId = ICN::CAMPXTRE;
            iconStatusOffset = 10;
            selectedIconIdx = isBetrayalScenario( scenarioInfoId ) ? 14 : 17;
            return;
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
            iconsId = ICN::X_CMPEXT;
            iconStatusOffset = 0;
            selectedIconIdx = 4;
            return;
        case Campaign::DESCENDANTS_CAMPAIGN:
            iconsId = ICN::X_CMPEXT;
            iconStatusOffset = 0;
            selectedIconIdx = 7;
            return;
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
            iconsId = ICN::X_CMPEXT;
            iconStatusOffset = 0;
            selectedIconIdx = 10;
            return;
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            iconsId = ICN::X_CMPEXT;
            iconStatusOffset = 0;
            selectedIconIdx = 13;
            return;
        default:
            // Implementing a new campaign? Add a new case!
            assert( 0 );
            return;
        }
    }

    void DrawCampaignScenarioIcon( const int icnId, const uint32_t iconIdx, const fheroes2::Point & offset, const int posX, const int posY )
    {
        const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( icnId, iconIdx );
        fheroes2::Blit( icon, fheroes2::Display::instance(), offset.x + posX, offset.y + posY );
    }

    void addScenarioButton( fheroes2::ButtonGroup & buttonGroup, const int buttonId, const Campaign::ScenarioInfoId & scenarioInfo,
                            const std::vector<Campaign::ScenarioInfoId> & availableMaps, const std::vector<Campaign::ScenarioInfoId> & clearedMaps,
                            const fheroes2::Point & trackOffset, const bool archibaldLowerBetrayalBranch )
    {
        const int deltaY = 42;
        const int deltaX = 37;

        const std::vector<fheroes2::Point> & iconOffsets = getCampaignIconOffsets( scenarioInfo.campaignId, archibaldLowerBetrayalBranch );

        assert( scenarioInfo.scenarioId >= 0 && static_cast<size_t>( scenarioInfo.scenarioId ) < iconOffsets.size() );
        if ( scenarioInfo.scenarioId < 0 || static_cast<size_t>( scenarioInfo.scenarioId ) >= iconOffsets.size() ) {
            return;
        }

        fheroes2::Point offset = iconOffsets[scenarioInfo.scenarioId];
        offset.x *= deltaX;
        offset.y *= deltaY;

        // Price of loyalty's track doesn't respect the standard offset between icons.
        if ( scenarioInfo.campaignId == Campaign::CampaignID::PRICE_OF_LOYALTY_CAMPAIGN && scenarioInfo.scenarioId >= 5 ) {
            switch ( scenarioInfo.scenarioId ) {
            case 5:
                offset.x -= 2;
                break;
            case 6:
                offset.x -= 1;
                break;
            case 7:
                offset.x -= 4;
                break;
            default:
                break;
            }
        }

        offset.x -= 2;
        offset.y -= 2;

        int iconsId = -1;
        uint32_t iconStatusOffset = 0;
        uint32_t selectedIconIdx = 0;

        getScenarioIconId( scenarioInfo, iconsId, iconStatusOffset, selectedIconIdx );

        // available scenario (one of which should be selected)
        if ( std::find( availableMaps.begin(), availableMaps.end(), scenarioInfo ) != availableMaps.end() ) {
            const fheroes2::Sprite & availableIcon = fheroes2::AGG::GetICN( iconsId, iconStatusOffset + SCENARIO_ICON_AVAILABLE );
            const fheroes2::Sprite & selectedIcon = fheroes2::AGG::GetICN( iconsId, selectedIconIdx );
            buttonGroup.createButton( trackOffset.x + offset.x, trackOffset.y + offset.y, availableIcon, selectedIcon, buttonId );
        }
        // cleared scenario
        else if ( std::find( clearedMaps.begin(), clearedMaps.end(), scenarioInfo ) != clearedMaps.end() ) {
            if ( isBetrayalScenario( scenarioInfo ) ) {
                assert( static_cast<size_t>( betrayalScenarioId ) < iconOffsets.size() );
                offset = iconOffsets[betrayalScenarioId];
                offset.x *= deltaX;
                offset.y *= deltaY;

                offset.x -= 2;
                offset.y -= 2;
            }

            DrawCampaignScenarioIcon( iconsId, iconStatusOffset + SCENARIO_ICON_CLEARED, trackOffset, offset.x, offset.y );
        }
        else if ( !isBetrayalScenario( scenarioInfo ) ) {
            DrawCampaignScenarioIcon( iconsId, iconStatusOffset + SCENARIO_ICON_UNAVAILABLE, trackOffset, offset.x, offset.y );
        }
    }

    void DrawCampaignScenarioIcons( fheroes2::ButtonGroup & buttonGroup, const Campaign::CampaignData & campaignData, const fheroes2::Point & top,
                                    const int chosenScenarioId )
    {
        std::vector<int> prevScenarioNextMaps;
        const Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();
        const std::vector<Campaign::ScenarioInfoId> & clearedMaps = saveData.getFinishedMaps();

        std::vector<Campaign::ScenarioInfoId> availableMaps;
        if ( chosenScenarioId >= 0 ) {
            availableMaps.emplace_back( saveData.getCampaignID(), chosenScenarioId );
        }
        else {
            availableMaps
                = saveData.isStarting() ? campaignData.getStartingScenarios() : Campaign::CampaignData::getScenariosAfter( saveData.getLastCompletedScenarioInfoID() );
        }

        bool isBetrayalScenarioNext = false;
        for ( const Campaign::ScenarioInfoId & scenarioInfo : availableMaps ) {
            if ( isBetrayalScenario( scenarioInfo ) ) {
                isBetrayalScenarioNext = true;
                break;
            }
        }

        bool isBetrayalScenarioCompleted = false;
        if ( !isBetrayalScenarioNext ) {
            for ( const Campaign::ScenarioInfoId & scenarioInfo : clearedMaps ) {
                if ( isBetrayalScenario( scenarioInfo ) ) {
                    isBetrayalScenarioCompleted = true;
                    break;
                }
            }
        }

        int campaignTrack = ICN::UNKNOWN;

        bool archibaldLowerBetrayalBranch = false;

        switch ( campaignData.getCampaignID() ) {
        case Campaign::ROLAND_CAMPAIGN:
            if ( isBetrayalScenarioNext ) {
                campaignTrack = ICN::CTRACK04;
            }
            else if ( isBetrayalScenarioCompleted ) {
                campaignTrack = ICN::CTRACK02;
            }
            else {
                campaignTrack = ICN::CTRACK00;
            }
            break;
        case Campaign::ARCHIBALD_CAMPAIGN:
            if ( isBetrayalScenarioNext ) {
                // Archibald campaign is unique in terms of scenario branching so this is the only way to make it work.
                assert( !clearedMaps.empty() );
                if ( clearedMaps.back().scenarioId == 2 ) {
                    campaignTrack = ICN::CTRACK05;
                }
                else {
                    assert( clearedMaps.back().scenarioId == 3 );
                    campaignTrack = ICN::CTRACK06;
                    archibaldLowerBetrayalBranch = true;
                }
            }
            else if ( isBetrayalScenarioCompleted ) {
                campaignTrack = ICN::CTRACK01;
            }
            else {
                campaignTrack = ICN::CTRACK03;
            }
            break;
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
            campaignTrack = ICN::X_TRACK1;
            break;
        case Campaign::DESCENDANTS_CAMPAIGN:
            campaignTrack = ICN::X_TRACK2;
            break;
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
            campaignTrack = ICN::X_TRACK3;
            break;
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            campaignTrack = ICN::X_TRACK4;
            break;
        default:
            // Implementing a new campaign? Add a new case!
            assert( 0 );
            return;
        }

        const fheroes2::Sprite & track = fheroes2::AGG::GetICN( campaignTrack, 0 );
        const fheroes2::Point trackOffset( top.x + track.x(), top.y + track.y() );
        fheroes2::Blit( track, fheroes2::Display::instance(), trackOffset.x, trackOffset.y );

        if ( isBetrayalScenarioCompleted ) {
            assert( campaignData.getCampaignID() == Campaign::ROLAND_CAMPAIGN || campaignData.getCampaignID() == Campaign::ARCHIBALD_CAMPAIGN );

            const bool isRolandCurrentCampaign = ( campaignData.getCampaignID() == Campaign::ROLAND_CAMPAIGN );

            const Campaign::CampaignData & beforeCampaignData
                = Campaign::CampaignData::getCampaignData( isRolandCurrentCampaign ? Campaign::ARCHIBALD_CAMPAIGN : Campaign::ROLAND_CAMPAIGN );
            const Campaign::CampaignData & currentCampaignData
                = Campaign::CampaignData::getCampaignData( isRolandCurrentCampaign ? Campaign::ROLAND_CAMPAIGN : Campaign::ARCHIBALD_CAMPAIGN );

            int scenarioCounter = 0;

            for ( const Campaign::ScenarioData & scenarioData : beforeCampaignData.getAllScenarios() ) {
                if ( scenarioData.getScenarioID() <= betrayalScenarioId || isBetrayalScenario( scenarioData.getScenarioInfoId() ) ) {
                    const Campaign::ScenarioInfoId scenarioInfo{ scenarioData.getCampaignId(), scenarioData.getScenarioID() };
                    addScenarioButton( buttonGroup, scenarioCounter, scenarioInfo, availableMaps, clearedMaps, trackOffset, archibaldLowerBetrayalBranch );
                    ++scenarioCounter;
                }
            }

            for ( const Campaign::ScenarioData & scenarioData : currentCampaignData.getAllScenarios() ) {
                if ( scenarioData.getScenarioID() > betrayalScenarioId ) {
                    const Campaign::ScenarioInfoId scenarioInfo{ scenarioData.getCampaignId(), scenarioData.getScenarioID() };
                    addScenarioButton( buttonGroup, scenarioCounter, scenarioInfo, availableMaps, clearedMaps, trackOffset, archibaldLowerBetrayalBranch );
                    ++scenarioCounter;
                }
            }
        }
        else {
            const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
            for ( size_t i = 0; i < scenarios.size(); ++i ) {
                const Campaign::ScenarioInfoId scenarioInfo{ scenarios[i].getCampaignId(), scenarios[i].getScenarioID() };
                addScenarioButton( buttonGroup, static_cast<int>( i ), scenarioInfo, availableMaps, clearedMaps, trackOffset, archibaldLowerBetrayalBranch );
            }
        }
    }

    void DrawCampaignScenarioDescription( const Campaign::ScenarioData & scenario, const fheroes2::Point & top )
    {
        const std::vector<Campaign::ScenarioBonusData> & bonuses = scenario.getBonuses();
        TextBox mapName( scenario.getScenarioName(), Font::BIG, 200 );
        mapName.Blit( top.x + 197, top.y + 97 - mapName.h() / 2 );

        int scenarioId = scenario.getScenarioID() + 1;
        if ( isBetrayalScenario( scenario.getScenarioInfoId() ) ) {
            assert( scenario.getCampaignId() == Campaign::ARCHIBALD_CAMPAIGN || scenario.getCampaignId() == Campaign::ROLAND_CAMPAIGN );
            scenarioId = betrayalScenarioId + 1;
        }

        const Text campaignMapId( std::to_string( scenarioId ), Font::BIG );
        campaignMapId.Blit( top.x + 172 - campaignMapId.w() / 2, top.y + 97 - campaignMapId.h() / 2 );

        TextBox mapDescription( scenario.getDescription(), Font::BIG, 356 );
        mapDescription.Blit( top.x + 34, top.y + 132 );

        const int textChoiceWidth = 160;
        const fheroes2::Point initialOffset{ top.x + 425, top.y + 211 };
        fheroes2::Display & display = fheroes2::Display::instance();

        for ( size_t i = 0; i < bonuses.size(); ++i ) {
            fheroes2::Text choice( bonuses[i].getName(), fheroes2::FontType::normalWhite() );
            choice.fitToOneRow( textChoiceWidth );

            choice.draw( initialOffset.x, initialOffset.y + 22 * static_cast<int>( i ) - choice.height() / 2, display );
        }
    }

    bool displayScenarioBonusPopupWindow( const Campaign::ScenarioData & scenario, const fheroes2::Point & top )
    {
        const std::vector<Campaign::ScenarioBonusData> & bonuses = scenario.getBonuses();
        if ( bonuses.empty() ) {
            // Nothing to process.
            return false;
        }

        const LocalEvent & le = LocalEvent::Get();

        for ( size_t i = 0; i < bonuses.size(); ++i ) {
            if ( le.MousePressRight( { top.x + 414, top.y + 198 + 22 * static_cast<int>( i ), 200, 22 } ) ) {
                fheroes2::showScenarioBonusDataPopupWindow( bonuses[i] );
                return true;
            }
        }

        return false;
    }

    void drawObtainedCampaignAwards( const Campaign::CampaignSaveData & campaignSaveData, const fheroes2::Point & top )
    {
        if ( isBetrayalScenario( campaignSaveData.getCurrentScenarioInfoId() ) ) {
            return;
        }

        const int textAwardWidth = 180;

        // if there are more than 3 awards, we need to reduce the offset between text so that it doesn't overflow out of the text box
        const std::vector<Campaign::CampaignAwardData> obtainedAwards = campaignSaveData.getObtainedCampaignAwards();
        const size_t awardCount = obtainedAwards.size();
        const size_t indexEnd = awardCount <= 4 ? awardCount : 4;
        const int yOffset = awardCount > 3 ? 16 : 22;
        const int initialOffsetY = 102;

        fheroes2::Display & display = fheroes2::Display::instance();

        if ( awardCount == 0 ) {
            const fheroes2::Text text( _( "None" ), fheroes2::FontType::normalWhite() );
            text.draw( top.x + 425, top.y + initialOffsetY - text.height() / 2, textAwardWidth, display );
            return;
        }

        fheroes2::Text award;
        for ( size_t i = 0; i < indexEnd; ++i ) {
            if ( i < 3 ) {
                award.set( obtainedAwards[i].getName(), fheroes2::FontType::normalWhite() );
            }
            else {
                // if we have exactly 4 obtained awards, display the fourth award, otherwise show "and more..."
                award.set( awardCount == 4 ? obtainedAwards[i].getName() : std::string( _( "and more..." ) ), fheroes2::FontType::normalWhite() );
            }

            award.fitToOneRow( textAwardWidth );
            award.draw( top.x + 425 + ( textAwardWidth - award.width() ) / 2, top.y + initialOffsetY + yOffset * static_cast<int>( i ) - award.height() / 2, display );
        }
    }

    bool displayScenarioAwardsPopupWindow( const Campaign::CampaignSaveData & campaignSaveData, const fheroes2::Point & top )
    {
        if ( isBetrayalScenario( campaignSaveData.getCurrentScenarioInfoId() ) ) {
            return false;
        }

        const std::vector<Campaign::CampaignAwardData> obtainedAwards = campaignSaveData.getObtainedCampaignAwards();
        if ( obtainedAwards.empty() ) {
            // Nothing to process.
            return false;
        }

        const size_t awardCount = obtainedAwards.size();
        const size_t indexEnd = awardCount <= 4 ? awardCount : 4;
        const int yOffset = awardCount > 3 ? 16 : 22;

        const LocalEvent & le = LocalEvent::Get();

        for ( size_t i = 0; i < indexEnd; ++i ) {
            if ( le.MousePressRight( { top.x + 414, top.y + 100 - yOffset / 2 + yOffset * static_cast<int>( i ), 200, yOffset } ) ) {
                fheroes2::showAwardDataPopupWindow( obtainedAwards[i] );
                return true;
            }
        }

        return false;
    }

    void replaceArmy( Army & army, const std::vector<Troop> & troops )
    {
        army.Clean();
        for ( size_t i = 0; i < troops.size(); ++i )
            army.GetTroop( i )->Set( troops[i] );
    }

    void setHeroAndArmyBonus( Heroes * hero, const Campaign::ScenarioInfoId & scenarioInfoId )
    {
        switch ( scenarioInfoId.campaignId ) {
        case Campaign::ARCHIBALD_CAMPAIGN: {
            if ( scenarioInfoId.scenarioId != 6 ) {
                assert( 0 ); // no other scenario has this bonus
                return;
            }
            switch ( hero->GetRace() ) {
            case Race::NECR:
                replaceArmy( hero->GetArmy(), { { Monster::SKELETON, 50 }, { Monster::ROYAL_MUMMY, 18 }, { Monster::VAMPIRE_LORD, 8 } } );
                break;
            case Race::WRLK:
                replaceArmy( hero->GetArmy(), { { Monster::CENTAUR, 40 }, { Monster::GARGOYLE, 24 }, { Monster::GRIFFIN, 18 } } );
                break;
            case Race::BARB:
                replaceArmy( hero->GetArmy(), { { Monster::ORC_CHIEF, 12 }, { Monster::OGRE, 18 }, { Monster::GOBLIN, 40 } } );
                break;
            default:
                assert( 0 ); // bonus changed?
            }
            const uint32_t exp = hero->GetExperience();
            if ( exp < 5000 ) {
                hero->IncreaseExperience( 5000 - exp, true );
            }
            break;
        }
        default:
            assert( 0 ); // some new campaign that uses this bonus?
        }
    }

    Heroes * getHeroToApplyBonusOrAwards( const Campaign::ScenarioInfoId & scenarioInfoId, const Kingdom & kingdom )
    {
        {
            static const std::map<std::pair<int, int>, int> targetHeroes = { // Final Justice
                                                                             { { Campaign::ROLAND_CAMPAIGN, 9 }, Heroes::ROLAND },
                                                                             // Apocalypse
                                                                             { { Campaign::ARCHIBALD_CAMPAIGN, 10 }, Heroes::ARCHIBALD },
                                                                             // Blood is Thicker
                                                                             { { Campaign::VOYAGE_HOME_CAMPAIGN, 3 }, Heroes::GALLAVANT } };

            const auto iter = targetHeroes.find( { scenarioInfoId.campaignId, scenarioInfoId.scenarioId } );
            if ( iter != targetHeroes.end() ) {
                // The "special" kingdom heroes may have the ID of another hero, but a custom name and portrait,
                // so the search should be performed by the portrait ID
                for ( Heroes * hero : kingdom.GetHeroes() ) {
                    assert( hero != nullptr );

                    if ( hero->getPortraitId() == iter->second ) {
                        return hero;
                    }
                }

                DEBUG_LOG( DBG_GAME, DBG_WARN,
                           "the hero to whom bonuses or awards should be applied has not been found"
                               << ", campaign id: " << scenarioInfoId.campaignId << ", scenario id: " << scenarioInfoId.scenarioId )
            }
        }

        {
            static const std::map<std::pair<int, int>, int> targetRaces = { // Defender
                                                                            { { Campaign::ROLAND_CAMPAIGN, 5 }, Race::SORC },
                                                                            // The Wayward Son
                                                                            { { Campaign::DESCENDANTS_CAMPAIGN, 2 }, Race::SORC },
                                                                            // The Epic Battle
                                                                            { { Campaign::DESCENDANTS_CAMPAIGN, 7 }, Race::SORC } };

            const auto iter = targetRaces.find( { scenarioInfoId.campaignId, scenarioInfoId.scenarioId } );
            if ( iter != targetRaces.end() ) {
                for ( Heroes * hero : kingdom.GetHeroes() ) {
                    assert( hero != nullptr );

                    if ( hero->GetRace() == iter->second ) {
                        return hero;
                    }
                }

                DEBUG_LOG( DBG_GAME, DBG_WARN,
                           "the hero to whom bonuses or awards should be applied has not been found"
                               << ", campaign id: " << scenarioInfoId.campaignId << ", scenario id: " << scenarioInfoId.scenarioId )
            }
        }

        // By default, bonuses and awards are applied to the best hero of the kingdom
        return kingdom.GetBestHero();
    }

    void SetScenarioBonus( const Campaign::ScenarioInfoId & scenarioInfoId, const Campaign::ScenarioBonusData & scenarioBonus )
    {
        const Players & sortedPlayers = Settings::Get().GetPlayers();
        for ( const Player * player : sortedPlayers ) {
            if ( player == nullptr ) {
                continue;
            }

            if ( !player->isControlHuman() ) {
                continue;
            }

            Kingdom & kingdom = world.GetKingdom( player->GetColor() );

            switch ( scenarioBonus._type ) {
            case Campaign::ScenarioBonusData::RESOURCES:
                kingdom.AddFundsResource( Funds( scenarioBonus._subType, scenarioBonus._amount ) );

                break;
            case Campaign::ScenarioBonusData::ARTIFACT: {
                Heroes * hero = getHeroToApplyBonusOrAwards( scenarioInfoId, kingdom );
                assert( hero != nullptr );

                if ( hero != nullptr ) {
                    Artifact artifact( scenarioBonus._subType );

                    if ( artifact == Artifact::SPELL_SCROLL ) {
                        artifact.SetSpell( scenarioBonus._artifactSpellId );
                    }

                    hero->PickupArtifact( artifact );
                }

                break;
            }
            case Campaign::ScenarioBonusData::TROOP: {
                Heroes * hero = getHeroToApplyBonusOrAwards( scenarioInfoId, kingdom );
                assert( hero != nullptr );

                if ( hero != nullptr ) {
                    hero->GetArmy().JoinTroop( Troop( Monster( scenarioBonus._subType ), scenarioBonus._amount ) );
                }

                break;
            }
            case Campaign::ScenarioBonusData::SPELL: {
                Heroes * hero = getHeroToApplyBonusOrAwards( scenarioInfoId, kingdom );
                assert( hero != nullptr );

                if ( hero != nullptr ) {
                    hero->AppendSpellToBook( scenarioBonus._subType, true );
                }

                break;
            }
            case Campaign::ScenarioBonusData::STARTING_RACE:
                Players::SetPlayerRace( player->GetColor(), scenarioBonus._subType );

                break;
            case Campaign::ScenarioBonusData::STARTING_RACE_AND_ARMY: {
                Heroes * hero = getHeroToApplyBonusOrAwards( scenarioInfoId, kingdom );
                assert( hero != nullptr );

                if ( hero != nullptr ) {
                    setHeroAndArmyBonus( hero, scenarioInfoId );
                }

                break;
            }
            case Campaign::ScenarioBonusData::SKILL_PRIMARY: {
                Heroes * hero = getHeroToApplyBonusOrAwards( scenarioInfoId, kingdom );
                assert( hero != nullptr );

                if ( hero != nullptr ) {
                    for ( int32_t i = 0; i < scenarioBonus._amount; ++i ) {
                        hero->IncreasePrimarySkill( scenarioBonus._subType );
                    }
                }

                break;
            }
            case Campaign::ScenarioBonusData::SKILL_SECONDARY: {
                Heroes * hero = getHeroToApplyBonusOrAwards( scenarioInfoId, kingdom );
                assert( hero != nullptr );

                if ( hero != nullptr ) {
                    hero->LearnSkill( Skill::Secondary( scenarioBonus._subType, scenarioBonus._amount ) );
                }

                break;
            }
            default:
                assert( 0 );
            }
        }
    }

    // apply only the ones that are applied at the start (artifact, spell, carry-over troops)
    // the rest will be applied based on the situation required
    void applyObtainedCampaignAwards( const Campaign::ScenarioInfoId & currentScenarioInfoId, const std::vector<Campaign::CampaignAwardData> & awards )
    {
        const Players & sortedPlayers = Settings::Get().GetPlayers();
        const Kingdom & humanKingdom = world.GetKingdom( Players::HumanColors() );

        for ( size_t i = 0; i < awards.size(); ++i ) {
            if ( currentScenarioInfoId.scenarioId < awards[i]._startScenarioID ) {
                continue;
            }

            switch ( awards[i]._type ) {
            case Campaign::CampaignAwardData::TYPE_GET_ARTIFACT: {
                Heroes * hero = getHeroToApplyBonusOrAwards( currentScenarioInfoId, humanKingdom );
                assert( hero != nullptr );

                if ( hero != nullptr ) {
                    hero->PickupArtifact( Artifact( awards[i]._subType ) );

                    // Some artifacts increase the Spell Power of the hero we have to set spell points to maximum.
                    hero->SetSpellPoints( std::max( hero->GetSpellPoints(), hero->GetMaxSpellPoints() ) );
                }

                break;
            }
            case Campaign::CampaignAwardData::TYPE_GET_SPELL: {
                Heroes * hero = getHeroToApplyBonusOrAwards( currentScenarioInfoId, humanKingdom );
                assert( hero != nullptr );

                if ( hero != nullptr ) {
                    hero->AppendSpellToBook( awards[i]._subType, true );
                }

                break;
            }
            case Campaign::CampaignAwardData::TYPE_DEFEAT_ENEMY_HERO:
                for ( const Player * player : sortedPlayers ) {
                    const Kingdom & kingdom = world.GetKingdom( player->GetColor() );
                    const KingdomHeroes & heroes = kingdom.GetHeroes();

                    for ( size_t j = 0; j < heroes.size(); ++j ) {
                        assert( heroes[j] != nullptr );

                        if ( heroes[j]->GetID() == awards[i]._subType ) {
                            heroes[j]->SetFreeman( Battle::RESULT_LOSS );
                            break;
                        }
                    }
                }

                break;
            case Campaign::CampaignAwardData::TYPE_CARRY_OVER_FORCES: {
                Heroes * hero = getHeroToApplyBonusOrAwards( currentScenarioInfoId, humanKingdom );
                assert( hero != nullptr );

                if ( hero != nullptr ) {
                    replaceArmy( hero->GetArmy(), Campaign::CampaignSaveData::Get().getCarryOverTroops() );
                }

                break;
            }
            default:
                break;
            }
        }
    }

    void playPreviousScenarioVideo()
    {
        const Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();
        if ( saveData.isStarting() ) {
            return;
        }

        const Campaign::ScenarioInfoId & lastCompletedScenarioInfoId = saveData.getLastCompletedScenarioInfoID();

        const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( lastCompletedScenarioInfoId.campaignId );

        const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
        assert( lastCompletedScenarioInfoId.scenarioId >= 0 && static_cast<size_t>( lastCompletedScenarioInfoId.scenarioId ) < scenarios.size() );
        const Campaign::ScenarioData & completedScenario = scenarios[lastCompletedScenarioInfoId.scenarioId];

        if ( !completedScenario.getEndScenarioVideoPlayback().empty() ) {
            AudioManager::ResetAudio();

            for ( const Campaign::ScenarioIntroVideoInfo & videoInfo : completedScenario.getEndScenarioVideoPlayback() ) {
                Video::ShowVideo( videoInfo.fileName, videoInfo.action );
            }

            AudioManager::ResetAudio();
        }
    }

    void playCurrentScenarioVideo()
    {
        const Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();

        const Campaign::ScenarioInfoId & currentScenarioInfoId = saveData.getCurrentScenarioInfoId();

        const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( currentScenarioInfoId.campaignId );

        const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
        assert( currentScenarioInfoId.scenarioId >= 0 && static_cast<size_t>( currentScenarioInfoId.scenarioId ) < scenarios.size() );
        const Campaign::ScenarioData & scenario = scenarios[currentScenarioInfoId.scenarioId];

        if ( !scenario.getStartScenarioVideoPlayback().empty() ) {
            AudioManager::ResetAudio();

            for ( const Campaign::ScenarioIntroVideoInfo & videoInfo : scenario.getStartScenarioVideoPlayback() ) {
                Video::ShowVideo( videoInfo.fileName, videoInfo.action );
            }

            AudioManager::ResetAudio();
        }
    }

    int getCampaignButtonId( const int campaignId )
    {
        switch ( campaignId ) {
        case Campaign::ROLAND_CAMPAIGN:
            return ICN::GOOD_CAMPAIGN_BUTTONS;
        case Campaign::ARCHIBALD_CAMPAIGN:
            return ICN::EVIL_CAMPAIGN_BUTTONS;
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
        case Campaign::DESCENDANTS_CAMPAIGN:
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            return ICN::X_CMPBTN;
        default:
            // Implementing a new campaign? Add a new case!
            assert( 0 );
            return ICN::UNKNOWN;
        }
    }

    void drawCampaignNameHeader( const int campaignId, fheroes2::Image & output, const fheroes2::Point & offset )
    {
        // Add extra image header if supported
        uint32_t campaignNameHeader = ICN::UNKNOWN;

        switch ( campaignId ) {
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
            campaignNameHeader = 15;
            break;
        case Campaign::DESCENDANTS_CAMPAIGN:
            campaignNameHeader = 16;
            break;
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
            campaignNameHeader = 17;
            break;
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            campaignNameHeader = 18;
            break;
        default:
            return;
        }

        const fheroes2::Sprite & header = fheroes2::AGG::GetICN( ICN::X_CMPEXT, campaignNameHeader );
        fheroes2::Blit( header, output, offset.x + 24, offset.y + 25 );
    }

    void playCampaignMusic( const int campaignId )
    {
        switch ( campaignId ) {
        case Campaign::ROLAND_CAMPAIGN:
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
        case Campaign::DESCENDANTS_CAMPAIGN:
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            AudioManager::PlayMusicAsync( MUS::ROLAND_CAMPAIGN_SCREEN, Music::PlaybackMode::REWIND_AND_PLAY_INFINITE );
            break;
        case Campaign::ARCHIBALD_CAMPAIGN:
            AudioManager::PlayMusicAsync( MUS::ARCHIBALD_CAMPAIGN_SCREEN, Music::PlaybackMode::REWIND_AND_PLAY_INFINITE );
            break;
        default:
            // Implementing a new campaign? Add a new case!
            assert( 0 );
            break;
        }
    }

    fheroes2::ButtonSprite getDifficultyButton( const int campaignId, const fheroes2::Point & offset )
    {
        int icnId = ICN::UNKNOWN;

        switch ( campaignId ) {
        case Campaign::ROLAND_CAMPAIGN:
            icnId = ICN::BUTTON_DIFFICULTY_ROLAND;
            break;
        case Campaign::ARCHIBALD_CAMPAIGN:
            icnId = ICN::BUTTON_DIFFICULTY_ARCHIBALD;
            break;
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
        case Campaign::DESCENDANTS_CAMPAIGN:
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            icnId = ICN::BUTTON_DIFFICULTY_POL;
            break;
        default:
            // Implementing a new campaign? Add a new case!
            assert( 0 );
            break;
        }
        return fheroes2::ButtonSprite( offset.x, offset.y, fheroes2::AGG::GetICN( icnId, 0 ), fheroes2::AGG::GetICN( icnId, 1 ) );
    }

    std::string getCampaignDifficultyText( const int32_t difficulty )
    {
        switch ( difficulty ) {
        case Campaign::CampaignDifficulty::Easy:
            return { _( "Easy" ) };
        case Campaign::CampaignDifficulty::Normal:
            // Original campaign difficulty.
            return { _( "Normal" ) };
        case Campaign::CampaignDifficulty::Hard:
            return { _( "Hard" ) };
        default:
            // Did you add a new campaign difficulty? Add the logic above!
            assert( 0 );
            return {};
        }
    }

    int32_t setCampaignDifficulty( int32_t currentDifficulty, const bool isSelectionAllowed )
    {
        const fheroes2::StandardWindow frameborder( 234, 270, true );
        const fheroes2::Rect & windowRoi = frameborder.activeArea();

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        const int buttonIcnId = isEvilInterface ? ICN::SPANBTNE : ICN::SPANBTN;
        const fheroes2::Sprite & buttonSprite = fheroes2::AGG::GetICN( buttonIcnId, 0 );

        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::Rect buttonMaxRoi( windowRoi.x + 5, windowRoi.y, windowRoi.width - 10, windowRoi.height - 5 );
        fheroes2::ButtonSprite buttonOk = fheroes2::makeButtonWithShadow( buttonMaxRoi.x + ( buttonMaxRoi.width - buttonSprite.width() ) / 2,
                                                                          buttonMaxRoi.y + buttonMaxRoi.height - buttonSprite.height(),
                                                                          fheroes2::AGG::GetICN( buttonIcnId, 0 ), fheroes2::AGG::GetICN( buttonIcnId, 1 ), display );

        buttonOk.draw();

        const fheroes2::Text caption( _( "Campaign Difficulty" ), fheroes2::FontType::normalYellow() );
        caption.draw( windowRoi.x + ( windowRoi.width - caption.width() ) / 2, windowRoi.y + 10, display );

        const fheroes2::Rect copyFromArea{ 20, 94, 223, 69 };
        const fheroes2::Point copyToOffset{ windowRoi.x + 4, windowRoi.y + 40 };
        fheroes2::Copy( fheroes2::AGG::GetICN( ICN::NGHSBKG, 0 ), copyFromArea.x, copyFromArea.y, display, copyToOffset.x, copyToOffset.y, copyFromArea.width,
                        copyFromArea.height );

        if ( isEvilInterface ) {
            fheroes2::ApplyPalette( display, copyToOffset.x, copyToOffset.y, display, copyToOffset.x, copyToOffset.y, copyFromArea.width, copyFromArea.height,
                                    PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
        }

        const fheroes2::Sprite & selectionImage = fheroes2::AGG::GetICN( ICN::NGEXTRA, 62 );

        fheroes2::MovableSprite selection( selectionImage );

        const char * easyDescription = _( "Choose this difficulty if you want to prefer game story over challenge. AI is weaker in comparison with normal difficulty." );
        const char * normalDescription = _( "Choose this difficulty to enjoy the campaign as per the original design." );
        const char * hardDescription = _( "Choose this difficulty if you want challenge. AI is stronger in comparison with normal difficulty." );

        const std::array<fheroes2::Rect, 3> difficultyArea{ fheroes2::Rect( windowRoi.x + 5, windowRoi.y + 37, selectionImage.width(), selectionImage.height() ),
                                                            fheroes2::Rect( windowRoi.x + 82, windowRoi.y + 37, selectionImage.width(), selectionImage.height() ),
                                                            fheroes2::Rect( windowRoi.x + 158, windowRoi.y + 37, selectionImage.width(), selectionImage.height() ) };

        const std::array<fheroes2::Rect, 3> iconArea{ fheroes2::Rect( difficultyArea[0].x + 4, difficultyArea[0].y + 4, 63, 63 ),
                                                      fheroes2::Rect( difficultyArea[1].x + 4, difficultyArea[1].y + 4, 63, 63 ),
                                                      fheroes2::Rect( difficultyArea[2].x + 4, difficultyArea[2].y + 4, 63, 63 ) };

        const char * currentDescription = nullptr;
        switch ( currentDifficulty ) {
        case Campaign::CampaignDifficulty::Easy:
            currentDescription = easyDescription;
            selection.setPosition( difficultyArea[0].x, difficultyArea[0].y );
            if ( !isSelectionAllowed ) {
                fheroes2::ApplyPalette( display, iconArea[1].x, iconArea[1].y, display, iconArea[1].x, iconArea[1].y, iconArea[1].width, iconArea[1].height,
                                        PAL::GetPalette( PAL::PaletteType::GRAY ) );
                fheroes2::ApplyPalette( display, iconArea[2].x, iconArea[2].y, display, iconArea[2].x, iconArea[2].y, iconArea[2].width, iconArea[2].height,
                                        PAL::GetPalette( PAL::PaletteType::GRAY ) );
            }
            break;
        case Campaign::CampaignDifficulty::Normal:
            currentDescription = normalDescription;
            selection.setPosition( difficultyArea[1].x, difficultyArea[1].y );
            if ( !isSelectionAllowed ) {
                fheroes2::ApplyPalette( display, iconArea[0].x, iconArea[0].y, display, iconArea[0].x, iconArea[0].y, iconArea[0].width, iconArea[0].height,
                                        PAL::GetPalette( PAL::PaletteType::GRAY ) );
                fheroes2::ApplyPalette( display, iconArea[2].x, iconArea[2].y, display, iconArea[2].x, iconArea[2].y, iconArea[2].width, iconArea[2].height,
                                        PAL::GetPalette( PAL::PaletteType::GRAY ) );
            }
            break;
        case Campaign::CampaignDifficulty::Hard:
            currentDescription = hardDescription;
            selection.setPosition( difficultyArea[2].x, difficultyArea[2].y );
            if ( !isSelectionAllowed ) {
                fheroes2::ApplyPalette( display, iconArea[0].x, iconArea[0].y, display, iconArea[0].x, iconArea[0].y, iconArea[0].width, iconArea[0].height,
                                        PAL::GetPalette( PAL::PaletteType::GRAY ) );
                fheroes2::ApplyPalette( display, iconArea[1].x, iconArea[1].y, display, iconArea[1].x, iconArea[1].y, iconArea[1].width, iconArea[1].height,
                                        PAL::GetPalette( PAL::PaletteType::GRAY ) );
            }
            break;
        default:
            // Did you add a new difficulty level for campaigns? Add the logic above!
            assert( 0 );
            break;
        }

        const int32_t textWidth = windowRoi.width - 16;
        const fheroes2::Point textOffset{ windowRoi.x + 8, windowRoi.y + 135 };

        fheroes2::Text description( currentDescription, fheroes2::FontType::normalWhite() );
        fheroes2::ImageRestorer restorer( display, textOffset.x, textOffset.y, textWidth, description.height( textWidth ) );
        description.draw( textOffset.x, textOffset.y, textWidth, display );

        const fheroes2::Text easyName( getCampaignDifficultyText( Campaign::CampaignDifficulty::Easy ), fheroes2::FontType::normalWhite() );
        const fheroes2::Text normalName( getCampaignDifficultyText( Campaign::CampaignDifficulty::Normal ), fheroes2::FontType::normalWhite() );
        const fheroes2::Text hardName( getCampaignDifficultyText( Campaign::CampaignDifficulty::Hard ), fheroes2::FontType::normalWhite() );

        easyName.draw( difficultyArea[0].x + ( difficultyArea[0].width - easyName.width() ) / 2, difficultyArea[0].y + difficultyArea[0].height + 5, display );
        normalName.draw( difficultyArea[1].x + ( difficultyArea[1].width - normalName.width() ) / 2, difficultyArea[1].y + difficultyArea[1].height + 5, display );
        hardName.draw( difficultyArea[2].x + ( difficultyArea[2].width - hardName.width() ) / 2, difficultyArea[2].y + difficultyArea[2].height + 5, display );

        display.render();

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();

            if ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyCloseWindow() ) {
                break;
            }

            if ( le.MousePressRight( buttonOk.area() ) ) {
                const fheroes2::Text header( _( "Okay" ), fheroes2::FontType::normalYellow() );
                const fheroes2::Text body( _( "Exit this menu." ), fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( difficultyArea[0] ) ) {
                const fheroes2::Text header( getCampaignDifficultyText( Campaign::CampaignDifficulty::Easy ), fheroes2::FontType::normalYellow() );
                const fheroes2::Text body( easyDescription, fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( difficultyArea[1] ) ) {
                const fheroes2::Text header( getCampaignDifficultyText( Campaign::CampaignDifficulty::Normal ), fheroes2::FontType::normalYellow() );
                const fheroes2::Text body( normalDescription, fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }
            else if ( le.MousePressRight( difficultyArea[2] ) ) {
                const fheroes2::Text header( getCampaignDifficultyText( Campaign::CampaignDifficulty::Hard ), fheroes2::FontType::normalYellow() );
                const fheroes2::Text body( hardDescription, fheroes2::FontType::normalWhite() );

                fheroes2::showMessage( header, body, 0 );
            }

            bool updateInfo = false;

            if ( isSelectionAllowed ) {
                if ( le.MouseClickLeft( difficultyArea[0] ) ) {
                    currentDescription = easyDescription;
                    selection.setPosition( difficultyArea[0].x, difficultyArea[0].y );
                    currentDifficulty = Campaign::CampaignDifficulty::Easy;
                    updateInfo = true;
                }
                else if ( le.MouseClickLeft( difficultyArea[1] ) ) {
                    currentDescription = normalDescription;
                    selection.setPosition( difficultyArea[1].x, difficultyArea[1].y );
                    currentDifficulty = Campaign::CampaignDifficulty::Normal;
                    updateInfo = true;
                }
                else if ( le.MouseClickLeft( difficultyArea[2] ) ) {
                    currentDescription = hardDescription;
                    selection.setPosition( difficultyArea[2].x, difficultyArea[2].y );
                    currentDifficulty = Campaign::CampaignDifficulty::Hard;
                    updateInfo = true;
                }
            }

            if ( updateInfo ) {
                restorer.restore();

                description.set( currentDescription, fheroes2::FontType::normalWhite() );
                restorer.reset();
                restorer.update( textOffset.x, textOffset.y, textWidth, description.height( textWidth ) );
                description.draw( textOffset.x, textOffset.y, textWidth, display );

                display.render();
            }
        }

        return currentDifficulty;
    }

    void outputCampaignScenarioInfoInTextSupportMode( const bool allowToRestart )
    {
        START_TEXT_SUPPORT_MODE

        const Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
        const int chosenCampaignID = campaignSaveData.getCampaignID();
        const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( chosenCampaignID );
        const int scenarioId = campaignSaveData.getCurrentScenarioID();
        const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
        const Campaign::ScenarioData & scenario = scenarios[scenarioId];

        COUT( "Scenario Information\n" )
        COUT( "'" << Campaign::getCampaignName( chosenCampaignID ) << "' campaign, scenario " << scenarioId + 1 << ": " << scenario.getScenarioName() )
        COUT( "Description: " << scenario.getDescription() << '\n' )

        const std::vector<Campaign::CampaignAwardData> obtainedAwards = campaignSaveData.getObtainedCampaignAwards();
        if ( obtainedAwards.empty() ) {
            COUT( "Awards: None" )
        }
        else {
            COUT( "Awards:" )
            for ( const Campaign::CampaignAwardData & award : obtainedAwards ) {
                COUT( "- " << award.getName() << " : " << award.getDescription() )
            }
        }

        COUT( "Bonuses:" )
        const std::vector<Campaign::ScenarioBonusData> & bonusChoices = scenario.getBonuses();
        for ( const Campaign::ScenarioBonusData & bonus : bonusChoices ) {
            COUT( "- " << bonus.getName() << " : " << bonus.getDescription() )
        }

        if ( !bonusChoices.empty() ) {
            COUT( "-  Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_SELECT_FIRST_BONUS ) << " to select the first bonus." )
        }
        if ( bonusChoices.size() > 1 ) {
            COUT( "-  Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_SELECT_SECOND_BONUS ) << " to select the second bonus." )
        }
        if ( bonusChoices.size() > 2 ) {
            COUT( "-  Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_SELECT_THIRD_BONUS ) << " to select the third bonus." )
        }

        if ( allowToRestart ) {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_RESTART_SCENARIO ) << " to Restart scenario." )
        }
        else {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_OKAY ) << " to Start scenario." )
        }

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_VIEW_INTRO ) << " to View Intro Video." )

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_SELECT_DIFFICULTY ) << " to Select / View Campaign Difficulty." )

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_CANCEL ) << " to Exit this dialog." )
    }
}

bool Game::isSuccessionWarsCampaignPresent()
{
    return Campaign::CampaignData::getCampaignData( Campaign::ROLAND_CAMPAIGN ).isAllCampaignMapsPresent()
           && Campaign::CampaignData::getCampaignData( Campaign::ARCHIBALD_CAMPAIGN ).isAllCampaignMapsPresent();
}

bool Game::isPriceOfLoyaltyCampaignPresent()
{
    // We need to check game resources as well.
    if ( fheroes2::AGG::GetICN( ICN::X_LOADCM, 0 ).empty() || fheroes2::AGG::GetICN( ICN::X_IVY, 0 ).empty() ) {
        return false;
    }

    return Campaign::CampaignData::getCampaignData( Campaign::PRICE_OF_LOYALTY_CAMPAIGN ).isAllCampaignMapsPresent()
           && Campaign::CampaignData::getCampaignData( Campaign::VOYAGE_HOME_CAMPAIGN ).isAllCampaignMapsPresent()
           && Campaign::CampaignData::getCampaignData( Campaign::WIZARDS_ISLE_CAMPAIGN ).isAllCampaignMapsPresent()
           && Campaign::CampaignData::getCampaignData( Campaign::DESCENDANTS_CAMPAIGN ).isAllCampaignMapsPresent();
}

fheroes2::GameMode Game::CompleteCampaignScenario( const bool isLoadingSaveFile )
{
    Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();
    const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( saveData.getCampaignID() );

    if ( !isLoadingSaveFile ) {
        saveData.addCurrentMapToFinished();
        saveData.addDaysPassed( world.CountDay() );

        if ( !campaignData.isLastScenario( saveData.getLastCompletedScenarioInfoID() ) ) {
            Game::SaveCompletedCampaignScenario();
        }
    }

    const Campaign::ScenarioInfoId & lastCompletedScenarioInfo = saveData.getLastCompletedScenarioInfoID();
    const std::vector<Campaign::CampaignAwardData> obtainableAwards = Campaign::CampaignAwardData::getCampaignAwardData( lastCompletedScenarioInfo );

    // TODO: Check for awards that have to be obtained with 'freak' conditions
    for ( const auto & obtainableAward : obtainableAwards ) {
        const int32_t awardType = obtainableAward._type;

        if ( awardType == Campaign::CampaignAwardData::AwardType::TYPE_DEFEAT_ENEMY_HERO ) {
            // This award must be granted only after defeating a hero in a battle.
            continue;
        }

        if ( awardType == Campaign::CampaignAwardData::AwardType::TYPE_CARRY_OVER_FORCES ) {
            const Kingdom & humanKingdom = world.GetKingdom( Players::HumanColors() );

            const Heroes * lastBattleWinHero = humanKingdom.GetLastBattleWinHero();

            if ( lastBattleWinHero )
                saveData.setCarryOverTroops( lastBattleWinHero->GetArmy() );
        }

        saveData.addCampaignAward( obtainableAward._id );

        // after adding an artifact award, check whether the artifacts can be assembled into something else
        if ( awardType == Campaign::CampaignAwardData::AwardType::TYPE_GET_ARTIFACT ) {
            const std::vector<Campaign::CampaignAwardData> obtainedAwards = saveData.getObtainedCampaignAwards();
            std::map<uint32_t, int> artifactAwardIDs;
            BagArtifacts bagArtifacts;

            for ( const Campaign::CampaignAwardData & awardData : obtainedAwards ) {
                if ( awardData._type != Campaign::CampaignAwardData::AwardType::TYPE_GET_ARTIFACT )
                    continue;

                artifactAwardIDs.emplace( awardData._subType, awardData._id );
                bagArtifacts.PushArtifact( awardData._subType );
                saveData.removeCampaignAward( awardData._id );
            }

            // add the assembled artifact's campaign award to artifactAwards
            for ( const Campaign::CampaignAwardData & awardData : Campaign::CampaignAwardData::getExtraCampaignAwardData( saveData.getCampaignID() ) ) {
                if ( awardData._type != Campaign::CampaignAwardData::AwardType::TYPE_GET_ARTIFACT )
                    continue;

                artifactAwardIDs.emplace( awardData._subType, awardData._id );
            }

            bagArtifacts.assembleArtifactSetIfPossible();

            for ( const Artifact & artifact : bagArtifacts ) {
                if ( !artifact.isValid() )
                    continue;

                const auto foundArtifact = artifactAwardIDs.find( artifact.GetID() );
                if ( foundArtifact != artifactAwardIDs.end() )
                    saveData.addCampaignAward( foundArtifact->second );
            }
        }
    }

    playPreviousScenarioVideo();

    if ( campaignData.isLastScenario( lastCompletedScenarioInfo ) ) {
        Game::ShowCredits( false );

        // Get data for ratings text.
        const Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
        const int32_t daysPassed = static_cast<int32_t>( campaignSaveData.getDaysPassed() );
        // Rating is calculated based on difficulty of campaign.
        const int32_t score = daysPassed * static_cast<int32_t>( campaignSaveData.getCampaignDifficultyPercent() ) / 100;

        // Make ratings text as a subtitle for WIN.SMK.
        fheroes2::MultiFontText ratingText;

        std::string textBody = _( "Congratulations!\n\nDays: %{days}\n" );
        StringReplace( textBody, "%{days}", daysPassed );
        ratingText.add( { textBody, fheroes2::FontType::normalWhite() } );

        textBody = _( "\nDifficulty: %{difficulty}\n\n" );
        StringReplace( textBody, "%{difficulty}", getCampaignDifficultyText( campaignSaveData.getDifficulty() ) );
        ratingText.add( { textBody, fheroes2::FontType::smallWhite() } );

        textBody = _( "Score: %{score}\n\nRating:\n%{rating}" );
        StringReplace( textBody, "%{score}", score );
        StringReplace( textBody, "%{rating}", fheroes2::HighScoreDataContainer::getMonsterByDay( score ).GetName() );
        ratingText.add( { textBody, fheroes2::FontType::normalWhite() } );

        // Show results from the 5th second until end (forever) and set maximum width to 140 to fit the black area.
        // Set subtitles top-center position (475,110) to render results over the black rectangle of burned picture in WIN.SMK video.
        Video::Subtitle ratingSubtitle( ratingText, 5000, UINT32_MAX, { 475, 110 }, 140 );

        AudioManager::ResetAudio();
        Video::ShowVideo( "WIN.SMK", Video::VideoAction::WAIT_FOR_USER_INPUT, { std::move( ratingSubtitle ) }, true );

        // fheroes2::PlayMusic is run here in order to start playing before displaying the high score.
        AudioManager::PlayMusicAsync( MUS::VICTORY, Music::PlaybackMode::REWIND_AND_PLAY_INFINITE );
        return fheroes2::GameMode::HIGHSCORES_CAMPAIGN;
    }

    const Campaign::ScenarioInfoId firstNextMap = Campaign::CampaignData::getScenariosAfter( lastCompletedScenarioInfo ).front();
    saveData.setCurrentScenarioInfo( firstNextMap );
    return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
}

fheroes2::GameMode Game::SelectCampaignScenario( const fheroes2::GameMode prevMode, const bool allowToRestart )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    display.fill( 0 );
    Settings & conf = Settings::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
    const int chosenCampaignID = campaignSaveData.getCampaignID();

    const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( chosenCampaignID );

    const Campaign::ScenarioInfoId & currentScenarioInfoId = campaignSaveData.getCurrentScenarioInfoId();

    const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
    const Campaign::ScenarioData & scenario = scenarios[currentScenarioInfoId.scenarioId];

    const fheroes2::GameInterfaceTypeRestorer gameInterfaceRestorer( chosenCampaignID != Campaign::ROLAND_CAMPAIGN );

    if ( !allowToRestart ) {
        playCurrentScenarioVideo();
    }

    outputCampaignScenarioInfoInTextSupportMode( allowToRestart );

    playCampaignMusic( chosenCampaignID );

    int backgroundIconID = ICN::UNKNOWN;

    switch ( chosenCampaignID ) {
    case Campaign::ROLAND_CAMPAIGN:
        backgroundIconID = ICN::CAMPBKGG;
        break;
    case Campaign::ARCHIBALD_CAMPAIGN:
        backgroundIconID = ICN::CAMPBKGE;
        break;
        // PoL campaigns use the same background, but different headers. TODO: Implement the headers
    case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
    case Campaign::DESCENDANTS_CAMPAIGN:
    case Campaign::WIZARDS_ISLE_CAMPAIGN:
    case Campaign::VOYAGE_HOME_CAMPAIGN:
        backgroundIconID = ICN::X_CMPBKG;
        break;
    default:
        // Implementing a new campaign? Add a new case!
        assert( 0 );
        break;
    }

    const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( backgroundIconID, 0 );
    const fheroes2::Point top( ( display.width() - backgroundImage.width() ) / 2, ( display.height() - backgroundImage.height() ) / 2 );

    fheroes2::Blit( backgroundImage, display, top.x, top.y );
    drawCampaignNameHeader( chosenCampaignID, display, top );

    const int buttonIconID = getCampaignButtonId( chosenCampaignID );
    fheroes2::Button buttonViewIntro( top.x + 22, top.y + 431, buttonIconID, 0, 1 );
    fheroes2::Button buttonRestart( top.x + 367 - 6, top.y + 431, buttonIconID, 2, 3 );
    fheroes2::Button buttonOk( top.x + 367, top.y + 431, buttonIconID, 4, 5 );
    fheroes2::Button buttonCancel( top.x + 511, top.y + 431, buttonIconID, 6, 7 );

    // Difficulty button is not present in the original assets so we need to generate it.
    fheroes2::ButtonSprite buttonDifficulty = getDifficultyButton( chosenCampaignID, top + fheroes2::Point( 195, 431 ) );

    // create scenario bonus choice buttons
    fheroes2::ButtonGroup buttonChoices;
    fheroes2::OptionButtonGroup optionButtonGroup;

    std::optional<int32_t> scenarioBonusId;
    const std::vector<Campaign::ScenarioBonusData> & bonusChoices = scenario.getBonuses();

    const fheroes2::Point optionButtonOffset( 590, 199 );
    const int32_t optionButtonStep = 22;

    const fheroes2::Sprite & pressedButton = fheroes2::AGG::GetICN( ICN::CAMPXTRG, 8 );
    fheroes2::Sprite releaseButton( pressedButton.width(), pressedButton.height(), pressedButton.x(), pressedButton.y() );
    fheroes2::Copy( backgroundImage, optionButtonOffset.x + pressedButton.x(), optionButtonOffset.y + pressedButton.y(), releaseButton, 0, 0, releaseButton.width(),
                    releaseButton.height() );

    const uint32_t bonusChoiceCount = static_cast<uint32_t>( bonusChoices.size() );

    {
        const int32_t saveDataBonusId = campaignSaveData.getCurrentScenarioBonusId();

        for ( uint32_t i = 0; i < bonusChoiceCount; ++i ) {
            buttonChoices.createButton( optionButtonOffset.x + top.x, optionButtonOffset.y + optionButtonStep * i + top.y, releaseButton, pressedButton, i );
            optionButtonGroup.addButton( &buttonChoices.button( i ) );

            if ( allowToRestart && saveDataBonusId >= 0 && static_cast<uint32_t>( saveDataBonusId ) == i ) {
                scenarioBonusId = saveDataBonusId;
                buttonChoices.button( i ).press();
            }
        }
    }

    if ( bonusChoiceCount > 0 ) {
        if ( allowToRestart ) {
            // If the campaign scenario is already in progress, then one of the bonuses should be selected
            assert( scenarioBonusId.has_value() );
        }
        else {
            // If this is the beginning of a new campaign scenario, then just select the first bonus
            scenarioBonusId = 0;
            buttonChoices.button( 0 ).press();
        }
    }

    optionButtonGroup.draw();

    buttonViewIntro.draw();
    buttonDifficulty.draw();

    const bool isDifficultySelectionAllowed = campaignSaveData.isStarting() && !allowToRestart;
    const bool isMapPresent = scenario.isMapFilePresent();

    if ( allowToRestart ) {
        buttonOk.disable();
        buttonOk.hide();

        if ( !isMapPresent ) {
            buttonRestart.disable();
        }

        buttonRestart.draw();
    }
    else {
        buttonRestart.disable();
        buttonRestart.hide();

        if ( !isMapPresent ) {
            buttonOk.disable();
        }

        buttonOk.draw();
    }

    // Only one button can be enabled at the time.
    assert( buttonRestart.isHidden() != buttonOk.isHidden() );

    buttonCancel.draw();

    const Text textDaysSpent( std::to_string( campaignSaveData.getDaysPassed() ), Font::BIG );
    textDaysSpent.Blit( top.x + 582 - textDaysSpent.w() / 2, top.y + 31 );

    DrawCampaignScenarioDescription( scenario, top );
    drawObtainedCampaignAwards( campaignSaveData, top );

    std::vector<Campaign::ScenarioInfoId> selectableScenarios;
    if ( allowToRestart ) {
        selectableScenarios.emplace_back( currentScenarioInfoId );
    }
    else {
        selectableScenarios = campaignSaveData.isStarting() ? campaignData.getStartingScenarios()
                                                            : Campaign::CampaignData::getScenariosAfter( campaignSaveData.getLastCompletedScenarioInfoID() );
    }

    const uint32_t selectableScenariosCount = static_cast<uint32_t>( selectableScenarios.size() );

    fheroes2::ButtonGroup selectableScenarioButtons;

    const int highlightedScenarioId = allowToRestart ? currentScenarioInfoId.scenarioId : -1;
    DrawCampaignScenarioIcons( selectableScenarioButtons, campaignData, top, highlightedScenarioId );

    for ( uint32_t i = 0; i < selectableScenariosCount; ++i ) {
        if ( currentScenarioInfoId == selectableScenarios[i] )
            selectableScenarioButtons.button( i ).press();

        selectableScenarioButtons.button( i ).draw();
    }

    LocalEvent & le = LocalEvent::Get();

    // Fade-in campaign scenario info.
    if ( validateDisplayFadeIn() ) {
        fheroes2::fadeInDisplay( { top.x, top.y, backgroundImage.width(), backgroundImage.height() }, false );
    }

    std::vector<fheroes2::Rect> choiceArea( bonusChoiceCount );
    for ( uint32_t i = 0; i < bonusChoiceCount; ++i ) {
        choiceArea[i] = buttonChoices.button( i ).area();
        choiceArea[i].x -= 170;
        choiceArea[i].width += 170;
    }

    const fheroes2::Rect areaDaysSpent{ top.x + 413, top.y + 27, 201, 25 };

    const std::array<Game::HotKeyEvent, 3> hotKeyBonusChoice{ Game::HotKeyEvent::CAMPAIGN_SELECT_FIRST_BONUS, Game::HotKeyEvent::CAMPAIGN_SELECT_SECOND_BONUS,
                                                              Game::HotKeyEvent::CAMPAIGN_SELECT_THIRD_BONUS };

    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();
        le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.MousePressLeft( buttonViewIntro.area() ) ? buttonViewIntro.drawOnPress() : buttonViewIntro.drawOnRelease();
        le.MousePressLeft( buttonDifficulty.area() ) ? buttonDifficulty.drawOnPress() : buttonDifficulty.drawOnRelease();
        le.MousePressLeft( buttonRestart.area() ) ? buttonRestart.drawOnPress() : buttonRestart.drawOnRelease();

        for ( uint32_t i = 0; i < bonusChoiceCount; ++i ) {
            if ( le.MousePressLeft( choiceArea[i] ) || ( i < hotKeyBonusChoice.size() && HotKeyPressEvent( hotKeyBonusChoice[i] ) ) ) {
                scenarioBonusId = fheroes2::checkedCast<int32_t>( i );
                buttonChoices.button( i ).press();
                optionButtonGroup.draw();
                display.render();

                break;
            }
        }

        for ( uint32_t i = 0; i < selectableScenariosCount; ++i ) {
            if ( currentScenarioInfoId != selectableScenarios[i] && le.MouseClickLeft( selectableScenarioButtons.button( i ).area() ) ) {
                campaignSaveData.setCurrentScenarioInfo( selectableScenarios[i] );
                return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            }
        }

        if ( le.MouseClickLeft( buttonCancel.area() ) || HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) ) {
            if ( !allowToRestart ) {
                // Make sure to reset a state of the game if a user does not want to load it.
                GameOver::Result::Get().Reset();
            }

            fheroes2::fadeOutDisplay( { top.x, top.y, backgroundImage.width(), backgroundImage.height() }, false );

            if ( prevMode == fheroes2::GameMode::LOAD_CAMPAIGN || prevMode == fheroes2::GameMode::MAIN_MENU ) {
                // We are going back to main menu.
                setDisplayFadeIn();
            }

            return prevMode;
        }

        displayScenarioAwardsPopupWindow( campaignSaveData, top ) || displayScenarioBonusPopupWindow( scenario, top );

        const bool restartButtonClicked
            = ( buttonRestart.isEnabled() && ( le.MouseClickLeft( buttonRestart.area() ) || HotKeyPressEvent( HotKeyEvent::CAMPAIGN_RESTART_SCENARIO ) ) );

        if ( le.MousePressRight( buttonCancel.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Cancel" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Exit this menu without doing anything." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( buttonOk.isVisible() && le.MousePressRight( buttonOk.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Okay" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Start the selected scenario." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( buttonViewIntro.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "View Intro" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "View Intro videos for the current state of the campaign." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( buttonDifficulty.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Campaign Difficulty" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Select campaign difficulty. It cannot be changed after." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( buttonRestart.isVisible() && le.MousePressRight( buttonRestart.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Restart" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Restart the current scenario." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( ( buttonOk.isEnabled() && ( le.MouseClickLeft( buttonOk.area() ) || HotKeyPressEvent( HotKeyEvent::DEFAULT_OKAY ) ) ) || restartButtonClicked ) {
            if ( restartButtonClicked
                 && Dialog::Message( _( "Restart" ), _( "Are you sure you want to restart this scenario?" ), Font::BIG, Dialog::YES | Dialog::NO ) == Dialog::NO ) {
                continue;
            }

            Maps::FileInfo mapInfo = scenario.loadMap();
            Campaign::CampaignData::updateScenarioGameplayConditions( currentScenarioInfoId, mapInfo );

            conf.SetCurrentFileInfo( mapInfo );

            assert( !scenarioBonusId || ( scenarioBonusId >= 0 && static_cast<size_t>( *scenarioBonusId ) < bonusChoices.size() ) );

            const Campaign::ScenarioBonusData scenarioBonus = [scenarioBonusId, &bonusChoices = std::as_const( bonusChoices )]() -> Campaign::ScenarioBonusData {
                if ( !scenarioBonusId ) {
                    return {};
                }

                if ( scenarioBonusId < 0 || static_cast<size_t>( *scenarioBonusId ) >= bonusChoices.size() ) {
                    return {};
                }

                return bonusChoices[*scenarioBonusId];
            }();

            // starting faction scenario bonus has to be called before players.SetStartGame()
            if ( scenarioBonus._type == Campaign::ScenarioBonusData::STARTING_RACE || scenarioBonus._type == Campaign::ScenarioBonusData::STARTING_RACE_AND_ARMY ) {
                // but the army has to be set after starting the game, so first only set the race
                SetScenarioBonus( currentScenarioInfoId, { Campaign::ScenarioBonusData::STARTING_RACE, scenarioBonus._subType, scenarioBonus._amount } );
            }

            // Betrayal scenario eliminates all obtained awards.
            if ( isBetrayalScenario( currentScenarioInfoId ) ) {
                campaignSaveData.removeAllAwards();
            }

            Players & players = conf.GetPlayers();
            players.SetStartGame();

            conf.SetGameType( Game::TYPE_CAMPAIGN );

            const bool isSWCampaign = ( chosenCampaignID == Campaign::ROLAND_CAMPAIGN ) || ( chosenCampaignID == Campaign::ARCHIBALD_CAMPAIGN );

            if ( !world.LoadMapMP2( mapInfo.file, isSWCampaign ) ) {
                Dialog::Message( _( "Campaign Scenario loading failure" ), _( "Please make sure that campaign files are correct and present." ), Font::BIG, Dialog::OK );

                // TODO: find a way to restore world for the current game after a failure.
                conf.SetCurrentFileInfo( Maps::FileInfo() );
                continue;
            }

            // Fade-out screen before loading a scenario.
            fheroes2::fadeOutDisplay();

            // meanwhile, the others should be called after players.SetStartGame()
            if ( scenarioBonus._type != Campaign::ScenarioBonusData::STARTING_RACE ) {
                SetScenarioBonus( currentScenarioInfoId, scenarioBonus );
            }

            applyObtainedCampaignAwards( currentScenarioInfoId, campaignSaveData.getObtainedCampaignAwards() );

            campaignSaveData.setCurrentScenarioInfo( currentScenarioInfoId, scenarioBonusId.value_or( -1 ) );

            return fheroes2::GameMode::START_GAME;
        }
        else if ( le.MouseClickLeft( buttonViewIntro.area() ) || HotKeyPressEvent( HotKeyEvent::CAMPAIGN_VIEW_INTRO ) ) {
            AudioManager::ResetAudio();
            fheroes2::ImageRestorer restorer( display, top.x, top.y, backgroundImage.width(), backgroundImage.height() );

            fheroes2::fadeOutDisplay( restorer.rect(), false );

            playPreviousScenarioVideo();
            playCurrentScenarioVideo();

            playCampaignMusic( chosenCampaignID );

            restorer.restore();

            fheroes2::fadeInDisplay( restorer.rect(), false );
        }
        else if ( le.MousePressRight( areaDaysSpent ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Days spent" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "The number of days spent on this campaign." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( le.MouseClickLeft( buttonDifficulty.area() ) || HotKeyPressEvent( HotKeyEvent::CAMPAIGN_SELECT_DIFFICULTY ) ) {
            campaignSaveData.setDifficulty( setCampaignDifficulty( campaignSaveData.getDifficulty(), isDifficultySelectionAllowed ) );
            display.render();
        }
    }

    return prevMode;
}
