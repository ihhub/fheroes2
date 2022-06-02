/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2022                                             *
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

#include <array>
#include <cassert>

#include "agg_image.h"
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
#include "game_over.h"
#include "game_video.h"
#include "icn.h"
#include "logging.h"
#include "race.h"
#include "settings.h"
#include "text.h"
#include "translations.h"
#include "ui_campaign.h"
#include "ui_dialog.h"
#include "ui_text.h"
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

        Text campaignMapId( std::to_string( scenarioId ), Font::BIG );
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

    void SetScenarioBonus( const Campaign::ScenarioInfoId & scenarioInfoId, const Campaign::ScenarioBonusData & scenarioBonus )
    {
        const Players & sortedPlayers = Settings::Get().GetPlayers();
        for ( const Player * player : sortedPlayers ) {
            if ( player == nullptr ) {
                continue;
            }

            if ( !player->isControlHuman() )
                continue;

            Kingdom & kingdom = world.GetKingdom( player->GetColor() );
            Heroes * bestHero = kingdom.GetBestHero();

            switch ( scenarioBonus._type ) {
            case Campaign::ScenarioBonusData::RESOURCES:
                kingdom.AddFundsResource( Funds( scenarioBonus._subType, scenarioBonus._amount ) );
                break;
            case Campaign::ScenarioBonusData::ARTIFACT: {
                assert( bestHero != nullptr );
                if ( bestHero != nullptr ) {
                    bestHero->PickupArtifact( Artifact( scenarioBonus._subType ) );
                }
                break;
            }
            case Campaign::ScenarioBonusData::TROOP:
                assert( bestHero != nullptr );
                if ( bestHero != nullptr ) {
                    bestHero->GetArmy().JoinTroop( Troop( Monster( scenarioBonus._subType ), scenarioBonus._amount ) );
                }
                break;
            case Campaign::ScenarioBonusData::SPELL: {
                KingdomHeroes & heroes = kingdom.GetHeroes();
                assert( !heroes.empty() );
                if ( !heroes.empty() ) {
                    // TODO: make sure that the correct hero receives the spell. Right now it's a semi-hacky way to do this.
                    heroes.back()->AppendSpellToBook( scenarioBonus._subType, true );
                }
                break;
            }
            case Campaign::ScenarioBonusData::STARTING_RACE:
                Players::SetPlayerRace( player->GetColor(), scenarioBonus._subType );
                break;
            case Campaign::ScenarioBonusData::STARTING_RACE_AND_ARMY:
                assert( bestHero != nullptr );
                if ( bestHero != nullptr ) {
                    setHeroAndArmyBonus( bestHero, scenarioInfoId );
                }
                break;
            case Campaign::ScenarioBonusData::SKILL_PRIMARY:
                assert( bestHero != nullptr );
                if ( bestHero != nullptr ) {
                    for ( int32_t i = 0; i < scenarioBonus._amount; ++i )
                        bestHero->IncreasePrimarySkill( scenarioBonus._subType );
                }
                break;
            case Campaign::ScenarioBonusData::SKILL_SECONDARY:
                assert( bestHero != nullptr );
                if ( bestHero != nullptr ) {
                    bestHero->LearnSkill( Skill::Secondary( scenarioBonus._subType, scenarioBonus._amount ) );
                }
                break;
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
        Kingdom & humanKingdom = world.GetKingdom( Players::HumanColors() );

        for ( size_t i = 0; i < awards.size(); ++i ) {
            if ( currentScenarioInfoId.scenarioId < awards[i]._startScenarioID )
                continue;

            switch ( awards[i]._type ) {
            case Campaign::CampaignAwardData::TYPE_GET_ARTIFACT:
                humanKingdom.GetBestHero()->PickupArtifact( Artifact( awards[i]._subType ) );
                break;
            case Campaign::CampaignAwardData::TYPE_GET_SPELL:
                humanKingdom.GetBestHero()->AppendSpellToBook( awards[i]._subType, true );
                break;
            case Campaign::CampaignAwardData::TYPE_DEFEAT_ENEMY_HERO:
                for ( const Player * player : sortedPlayers ) {
                    Kingdom & kingdom = world.GetKingdom( player->GetColor() );
                    const KingdomHeroes & heroes = kingdom.GetHeroes();

                    for ( size_t j = 0; j < heroes.size(); ++j ) {
                        if ( heroes[j]->GetID() == awards[i]._subType ) {
                            heroes[j]->SetFreeman( Battle::RESULT_LOSS );
                            break;
                        }
                    }
                }
                break;
            case Campaign::CampaignAwardData::TYPE_CARRY_OVER_FORCES:
                replaceArmy( humanKingdom.GetBestHero()->GetArmy(), Campaign::CampaignSaveData::Get().getCarryOverTroops() );
                break;
            default:
                break;
            }
        }
    }

    void playPreviosScenarioVideo()
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
            fheroes2::ResetAudio();

            for ( const Campaign::ScenarioIntroVideoInfo & videoInfo : completedScenario.getEndScenarioVideoPlayback() ) {
                Video::ShowVideo( videoInfo.fileName, videoInfo.action );
            }

            fheroes2::ResetAudio();
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
            fheroes2::ResetAudio();

            for ( const Campaign::ScenarioIntroVideoInfo & videoInfo : scenario.getStartScenarioVideoPlayback() ) {
                Video::ShowVideo( videoInfo.fileName, videoInfo.action );
            }

            fheroes2::ResetAudio();
        }
    }

    int getCampaignButtonId( const int campaignId )
    {
        switch ( campaignId ) {
        case Campaign::ROLAND_CAMPAIGN:
            return ICN::CAMPXTRG;
        case Campaign::ARCHIBALD_CAMPAIGN:
            return ICN::CAMPXTRE;
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
            fheroes2::PlayMusic( MUS::ROLAND_CAMPAIGN_SCREEN, true );
            break;
        case Campaign::ARCHIBALD_CAMPAIGN:
            fheroes2::PlayMusic( MUS::ARCHIBALD_CAMPAIGN_SCREEN, true );
            break;
        default:
            // Implementing a new campaign? Add a new case!
            assert( 0 );
            break;
        }
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
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_OKAY ) << " to Restart scenario." )
        }
        else {
            COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::DEFAULT_OKAY ) << " to Start scenario." )
        }

        COUT( "Press " << Game::getHotKeyNameByEventId( Game::HotKeyEvent::CAMPAIGN_VIEW_INTRO ) << " to View Intro Video." )

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
    for ( size_t i = 0; i < obtainableAwards.size(); ++i ) {
        const uint32_t awardType = obtainableAwards[i]._type;

        if ( awardType == Campaign::CampaignAwardData::AwardType::TYPE_CARRY_OVER_FORCES ) {
            const Kingdom & humanKingdom = world.GetKingdom( Players::HumanColors() );

            const Heroes * lastBattleWinHero = humanKingdom.GetLastBattleWinHero();

            if ( lastBattleWinHero )
                saveData.setCarryOverTroops( lastBattleWinHero->GetArmy() );
        }

        saveData.addCampaignAward( obtainableAwards[i]._id );

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

    playPreviosScenarioVideo();

    if ( campaignData.isLastScenario( lastCompletedScenarioInfo ) ) {
        Game::ShowCredits();

        fheroes2::ResetAudio();
        Video::ShowVideo( "WIN.SMK", Video::VideoAction::WAIT_FOR_USER_INPUT );
        // TODO : Implement function that displays the last frame of win.smk with score
        // and a dialog for name entry. AGG:PlayMusic is run here in order to start
        // playing before displaying the high score.
        fheroes2::PlayMusic( MUS::VICTORY, true, true );
        return fheroes2::GameMode::HIGHSCORES_CAMPAIGN;
    }

    const Campaign::ScenarioInfoId firstNextMap = Campaign::CampaignData::getScenariosAfter( lastCompletedScenarioInfo ).front();
    saveData.setCurrentScenarioInfoId( firstNextMap );
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

    fheroes2::GameInterfaceTypeRestorer gameInterfaceRestorer( chosenCampaignID != Campaign::ROLAND_CAMPAIGN );

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
    fheroes2::Button buttonRestart( top.x + 195, top.y + 431, buttonIconID, 2, 3 );
    fheroes2::Button buttonOk( top.x + 367, top.y + 431, buttonIconID, 4, 5 );
    fheroes2::Button buttonCancel( top.x + 511, top.y + 431, buttonIconID, 6, 7 );

    // create scenario bonus choice buttons
    fheroes2::ButtonGroup buttonChoices;
    fheroes2::OptionButtonGroup optionButtonGroup;

    Campaign::ScenarioBonusData scenarioBonus;
    const std::vector<Campaign::ScenarioBonusData> & bonusChoices = scenario.getBonuses();

    const fheroes2::Point optionButtonOffset( 590, 199 );
    const int32_t optionButtonStep = 22;

    const fheroes2::Sprite & pressedButton = fheroes2::AGG::GetICN( ICN::CAMPXTRG, 8 );
    fheroes2::Sprite releaseButton( pressedButton.width(), pressedButton.height(), pressedButton.x(), pressedButton.y() );
    fheroes2::Copy( backgroundImage, optionButtonOffset.x + pressedButton.x(), optionButtonOffset.y + pressedButton.y(), releaseButton, 0, 0, releaseButton.width(),
                    releaseButton.height() );

    const uint32_t bonusChoiceCount = static_cast<uint32_t>( scenario.getBonuses().size() );
    for ( uint32_t i = 0; i < bonusChoiceCount; ++i ) {
        buttonChoices.createButton( optionButtonOffset.x + top.x, optionButtonOffset.y + optionButtonStep * i + top.y, releaseButton, pressedButton, i );
        optionButtonGroup.addButton( &buttonChoices.button( i ) );
    }

    // in case there's no bonus for the map
    if ( bonusChoiceCount > 0 ) {
        scenarioBonus = bonusChoices[0];
        buttonChoices.button( 0 ).press();
    }

    buttonViewIntro.draw();

    if ( !scenario.isMapFilePresent() ) {
        buttonOk.disable();
    }

    if ( allowToRestart ) {
        buttonOk.disable();
        buttonOk.hide();
        buttonRestart.draw();
    }
    else {
        buttonRestart.disable();
        buttonRestart.hide();
        buttonOk.draw();
    }

    buttonCancel.draw();

    for ( uint32_t i = 0; i < bonusChoiceCount; ++i )
        buttonChoices.button( i ).draw();

    Text textDaysSpent( std::to_string( campaignSaveData.getDaysPassed() ), Font::BIG );
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

    display.render();

    std::vector<fheroes2::Rect> choiceArea( bonusChoiceCount );
    for ( uint32_t i = 0; i < bonusChoiceCount; ++i ) {
        choiceArea[i] = buttonChoices.button( i ).area();
        choiceArea[i].x -= 170;
        choiceArea[i].width += 170;
    }

    const std::array<Game::HotKeyEvent, 3> hotKeyBonusChoice{ Game::HotKeyEvent::CAMPAIGN_SELECT_FIRST_BONUS, Game::HotKeyEvent::CAMPAIGN_SELECT_SECOND_BONUS,
                                                              Game::HotKeyEvent::CAMPAIGN_SELECT_THIRD_BONUS };

    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();
        le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.MousePressLeft( buttonViewIntro.area() ) ? buttonViewIntro.drawOnPress() : buttonViewIntro.drawOnRelease();

        if ( allowToRestart ) {
            le.MousePressLeft( buttonRestart.area() ) ? buttonRestart.drawOnPress() : buttonRestart.drawOnRelease();
        }

        for ( uint32_t i = 0; i < bonusChoiceCount; ++i ) {
            if ( le.MousePressLeft( choiceArea[i] ) || ( i < hotKeyBonusChoice.size() && HotKeyPressEvent( hotKeyBonusChoice[i] ) ) ) {
                buttonChoices.button( i ).press();
                optionButtonGroup.draw();
                scenarioBonus = bonusChoices[i];
                display.render();

                break;
            }
        }

        for ( uint32_t i = 0; i < selectableScenariosCount; ++i ) {
            if ( currentScenarioInfoId != selectableScenarios[i] && le.MouseClickLeft( selectableScenarioButtons.button( i ).area() ) ) {
                campaignSaveData.setCurrentScenarioInfoId( selectableScenarios[i] );
                return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            }
        }

        if ( le.MouseClickLeft( buttonCancel.area() ) || HotKeyPressEvent( HotKeyEvent::DEFAULT_CANCEL ) ) {
            if ( !allowToRestart ) {
                // Make sure to reset a state of the game if a user does not want to load it.
                GameOver::Result::Get().Reset();
            }
            return prevMode;
        }

        displayScenarioAwardsPopupWindow( campaignSaveData, top ) || displayScenarioBonusPopupWindow( scenario, top );

        const bool restartButtonClicked = ( buttonRestart.isEnabled() && le.MouseClickLeft( buttonRestart.area() ) );

        if ( le.MousePressRight( buttonCancel.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Cancel" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Exit this menu without doing anything." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( !allowToRestart && le.MousePressRight( buttonOk.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Okay" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Start the selected scenario." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( buttonViewIntro.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "View Intro" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "View Intro videos for the current state of the campaign." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( allowToRestart && le.MousePressRight( buttonRestart.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Restart" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Restart the current scenario." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
        else if ( ( buttonOk.isEnabled() && ( le.MouseClickLeft( buttonOk.area() ) || HotKeyPressEvent( HotKeyEvent::DEFAULT_OKAY ) ) ) || restartButtonClicked ) {
            if ( restartButtonClicked
                 && Dialog::Message( _( "Restart" ), _( "Are you sure you want to restart this scenario?" ), Font::BIG, Dialog::YES | Dialog::NO ) == Dialog::NO ) {
                continue;
            }

            const Maps::FileInfo mapInfo = scenario.loadMap();
            conf.SetCurrentFileInfo( mapInfo );

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
            if ( Settings::ExtGameUseFade() )
                fheroes2::FadeDisplay();

            conf.SetGameType( Game::TYPE_CAMPAIGN );

            if ( !world.LoadMapMP2( mapInfo.file ) ) {
                Dialog::Message( _( "Campaign Scenario loading failure" ), _( "Please make sure that campaign files are correct and present." ), Font::BIG, Dialog::OK );
                conf.SetCurrentFileInfo( Maps::FileInfo() );
                continue;
            }

            // meanwhile, the others should be called after players.SetStartGame()
            if ( scenarioBonus._type != Campaign::ScenarioBonusData::STARTING_RACE ) {
                SetScenarioBonus( currentScenarioInfoId, scenarioBonus );
            }

            applyObtainedCampaignAwards( currentScenarioInfoId, campaignSaveData.getObtainedCampaignAwards() );

            campaignSaveData.setCurrentScenarioBonus( scenarioBonus );
            campaignSaveData.setCurrentScenarioInfoId( currentScenarioInfoId );

            return fheroes2::GameMode::START_GAME;
        }
        else if ( le.MouseClickLeft( buttonViewIntro.area() ) || HotKeyPressEvent( HotKeyEvent::CAMPAIGN_VIEW_INTRO ) ) {
            fheroes2::ResetAudio();
            fheroes2::ImageRestorer restorer( display, top.x, top.y, backgroundImage.width(), backgroundImage.height() );
            playPreviosScenarioVideo();
            playCurrentScenarioVideo();

            playCampaignMusic( chosenCampaignID );
        }
    }

    return prevMode;
}
