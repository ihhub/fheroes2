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

#include <cassert>

#include "agg.h"
#include "agg_image.h"
#include "battle.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_credits.h"
#include "game_io.h"
#include "game_video.h"
#include "icn.h"
#include "race.h"
#include "settings.h"
#include "text.h"
#include "translations.h"
#include "world.h"

namespace
{
    std::vector<fheroes2::Point> getCampaignIconOffsets( const int campaignId )
    {
        switch ( campaignId ) {
        case Campaign::ROLAND_CAMPAIGN:
            return { { 0, 1 }, { 2, 1 }, { 3, 0 }, { 4, 1 }, { 6, 1 }, { 8, 1 }, { 10, 2 }, { 10, 0 }, { 12, 1 }, { 14, 1 } };
        case Campaign::ARCHIBALD_CAMPAIGN:
            return { { 0, 1 }, { 2, 1 }, { 4, 0 }, { 4, 2 }, { 6, 1 }, { 8, 1 }, { 9, 0 }, { 10, 1 }, { 12, 0 }, { 12, 2 }, { 14, 1 } };
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

    enum ScenarioIcon : uint32_t
    {
        SCENARIO_ICON_CLEARED = 0,
        SCENARIO_ICON_AVAILABLE = 1,
        SCENARIO_ICON_UNAVAILABLE = 2,
    };

    void DrawCampaignScenarioIcon( const int icnId, const int iconIdx, const fheroes2::Point & offset, const int posX, const int posY )
    {
        const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( icnId, iconIdx );
        fheroes2::Blit( icon, fheroes2::Display::instance(), offset.x + posX, offset.y + posY );
    }

    void DrawCampaignScenarioIcons( fheroes2::ButtonGroup & buttonGroup, const Campaign::CampaignData & campaignData, const fheroes2::Point & top,
                                    const int chosenScenarioId )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        int campaignTrack = ICN::UNKNOWN;
        int iconsId = ICN::UNKNOWN;
        uint32_t iconStatusOffset = 0;
        uint32_t selectedIconIdx = 0;

        switch ( campaignData.getCampaignID() ) {
        case Campaign::ROLAND_CAMPAIGN:
            campaignTrack = ICN::CTRACK00;
            iconsId = ICN::CAMPXTRG;
            iconStatusOffset = 10;
            selectedIconIdx = 14;
            break;
        case Campaign::ARCHIBALD_CAMPAIGN:
            campaignTrack = ICN::CTRACK03;
            iconsId = ICN::CAMPXTRE;
            iconStatusOffset = 10;
            selectedIconIdx = 17;
            break;
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
            campaignTrack = ICN::X_TRACK1;
            iconsId = ICN::X_CMPEXT;
            iconStatusOffset = 0;
            selectedIconIdx = 4;
            break;
        case Campaign::DESCENDANTS_CAMPAIGN:
            campaignTrack = ICN::X_TRACK2;
            iconsId = ICN::X_CMPEXT;
            iconStatusOffset = 0;
            selectedIconIdx = 7;
            break;
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
            campaignTrack = ICN::X_TRACK3;
            iconsId = ICN::X_CMPEXT;
            iconStatusOffset = 0;
            selectedIconIdx = 10;
            break;
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            campaignTrack = ICN::X_TRACK4;
            iconsId = ICN::X_CMPEXT;
            iconStatusOffset = 0;
            selectedIconIdx = 13;
            break;
        default:
            // Implementing a new campaign? Add a new case!
            assert( 0 );
            break;
        }

        const fheroes2::Sprite & track = fheroes2::AGG::GetICN( campaignTrack, 0 );
        const fheroes2::Point trackOffset( top.x + track.x(), top.y + track.y() );
        fheroes2::Blit( track, display, trackOffset.x, trackOffset.y );

        const std::vector<fheroes2::Point> & iconOffsets = getCampaignIconOffsets( campaignData.getCampaignID() );
        const int deltaY = 42;
        const int deltaX = 37;

        const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
        const Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();

        std::vector<int> prevScenarioNextMaps;
        const std::vector<int> & clearedMaps = saveData.getFinishedMaps();
        std::vector<int> availableMaps;
        if ( chosenScenarioId >= 0 ) {
            availableMaps.emplace_back( chosenScenarioId );
        }
        else {
            availableMaps = saveData.isStarting() ? campaignData.getStartingScenarios() : campaignData.getScenariosAfter( saveData.getLastCompletedScenarioID() );
        }

        assert( iconOffsets.size() == scenarios.size() );

        for ( size_t i = 0; i < scenarios.size(); ++i ) {
            const int scenarioID = scenarios[i].getScenarioID();

            assert( scenarioID >= 0 && static_cast<size_t>( scenarioID ) < iconOffsets.size() );
            if ( scenarioID < 0 || static_cast<size_t>( scenarioID ) >= iconOffsets.size() ) {
                continue;
            }

            fheroes2::Point offset = iconOffsets[scenarioID];
            offset.x *= deltaX;
            offset.y *= deltaY;

            offset.x -= 2;
            offset.y -= 2;

            // available scenario (one of which should be selected)
            if ( std::find( availableMaps.begin(), availableMaps.end(), scenarioID ) != availableMaps.end() ) {
                const fheroes2::Sprite & availableIcon = fheroes2::AGG::GetICN( iconsId, iconStatusOffset + SCENARIO_ICON_AVAILABLE );
                const fheroes2::Sprite & selectedIcon = fheroes2::AGG::GetICN( iconsId, selectedIconIdx );
                buttonGroup.createButton( trackOffset.x + offset.x, trackOffset.y + offset.y, availableIcon, selectedIcon, static_cast<int>( i ) );
            }
            // cleared scenario
            else if ( std::find( clearedMaps.begin(), clearedMaps.end(), static_cast<int>( i ) ) != clearedMaps.end() ) {
                DrawCampaignScenarioIcon( iconsId, iconStatusOffset + SCENARIO_ICON_CLEARED, trackOffset, offset.x, offset.y );
            }
            else {
                DrawCampaignScenarioIcon( iconsId, iconStatusOffset + SCENARIO_ICON_UNAVAILABLE, trackOffset, offset.x, offset.y );
            }
        }
    }

    void DrawCampaignScenarioDescription( const Campaign::ScenarioData & scenario, const fheroes2::Point & top )
    {
        const std::vector<Campaign::ScenarioBonusData> & bonuses = scenario.getBonuses();
        TextBox mapName( scenario.getScenarioName(), Font::BIG, 200 );
        mapName.Blit( top.x + 197, top.y + 97 - mapName.h() / 2 );

        Text campaignMapId( std::to_string( scenario.getScenarioID() + 1 ), Font::BIG );
        campaignMapId.Blit( top.x + 172 - campaignMapId.w() / 2, top.y + 97 - campaignMapId.h() / 2 );

        TextBox mapDescription( scenario.getDescription(), Font::BIG, 356 );
        mapDescription.Blit( top.x + 34, top.y + 132 );

        const int textChoiceWidth = 150;
        for ( size_t i = 0; i < bonuses.size(); ++i ) {
            Text choice( bonuses[i].ToString(), Font::BIG );
            choice.Blit( top.x + 425, top.y + 209 + 22 * static_cast<int>( i ) - choice.h() / 2, textChoiceWidth );
        }
    }

    void drawObtainedCampaignAwards( const std::vector<Campaign::CampaignAwardData> & obtainedAwards, const fheroes2::Point & top )
    {
        const int textAwardWidth = 180;

        // if there are more than 3 awards, we need to reduce the offset between text so that it doesn't overflow out of the text box
        const size_t awardCount = obtainedAwards.size();
        const size_t indexEnd = awardCount <= 4 ? awardCount : 4;
        const int yOffset = awardCount > 3 ? 16 : 22;

        Text award;
        for ( size_t i = 0; i < indexEnd; ++i ) {
            if ( i < 3 )
                award.Set( obtainedAwards[i].ToString(), Font::BIG );
            else // if we have exactly 4 obtained awards, display the fourth award, otherwise show "and more..."
                award.Set( awardCount == 4 ? obtainedAwards[i].ToString() : std::string( _( "and more..." ) ), Font::BIG );

            if ( award.w() > textAwardWidth ) {
                award.Blit( top.x + 425, top.y + 100 + yOffset * static_cast<int>( i ) - award.h() / 2, textAwardWidth );
            }
            else {
                award.Blit( top.x + 425 + ( textAwardWidth - award.w() ) / 2, top.y + 100 + yOffset * static_cast<int>( i ) - award.h() / 2 );
            }
        }
    }

    void replaceArmy( Army & army, const std::vector<Troop> & troops )
    {
        army.Clean();
        for ( size_t i = 0; i < troops.size(); ++i )
            army.GetTroop( i )->Set( troops[i] );
    }

    void setHeroAndArmyBonus( Heroes * hero, const int campaignID, const uint32_t currentScenarioID )
    {
        switch ( campaignID ) {
        case Campaign::ARCHIBALD_CAMPAIGN: {
            if ( currentScenarioID != 6 ) {
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

    void SetScenarioBonus( const int campaignID, const uint32_t currentScenarioID, const Campaign::ScenarioBonusData & scenarioBonus )
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
                    setHeroAndArmyBonus( bestHero, campaignID, currentScenarioID );
                }
                break;
            case Campaign::ScenarioBonusData::SKILL_PRIMARY:
                assert( bestHero != nullptr );
                if ( bestHero != nullptr ) {
                    for ( uint32_t i = 0; i < scenarioBonus._amount; ++i )
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
    void applyObtainedCampaignAwards( const uint32_t currentScenarioID, const std::vector<Campaign::CampaignAwardData> & awards )
    {
        const Players & sortedPlayers = Settings::Get().GetPlayers();
        Kingdom & humanKingdom = world.GetKingdom( Players::HumanColors() );

        for ( size_t i = 0; i < awards.size(); ++i ) {
            if ( currentScenarioID < awards[i]._startScenarioID )
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
                        if ( heroes[j]->GetID() == static_cast<int>( awards[i]._subType ) ) {
                            heroes[j]->SetKillerColor( humanKingdom.GetColor() );
                            heroes[j]->SetFreeman( Battle::RESULT_LOSS );
                            break;
                        }
                    }
                }
                break;
            case Campaign::CampaignAwardData::TYPE_CARRY_OVER_FORCES:
                replaceArmy( humanKingdom.GetBestHero()->GetArmy(), Campaign::CampaignSaveData::Get().getCarryOverTroops() );
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

        const int lastCompletedScenarioID = saveData.getLastCompletedScenarioID();
        const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( saveData.getCampaignID() );

        const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
        assert( lastCompletedScenarioID >= 0 && static_cast<size_t>( lastCompletedScenarioID ) < scenarios.size() );
        const Campaign::ScenarioData & completedScenario = scenarios[lastCompletedScenarioID];

        if ( !completedScenario.getEndScenarioVideoPlayback().empty() ) {
            AGG::ResetMixer();

            for ( const Campaign::ScenarioIntroVideoInfo & videoInfo : completedScenario.getEndScenarioVideoPlayback() ) {
                Video::ShowVideo( videoInfo.fileName, videoInfo.action );
            }

            AGG::ResetMixer();
        }
    }

    void playCurrentScenarioVideo()
    {
        const Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();

        const int chosenScenarioID = saveData.getCurrentScenarioID();
        const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( saveData.getCampaignID() );

        const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
        assert( chosenScenarioID >= 0 && static_cast<size_t>( chosenScenarioID ) < scenarios.size() );
        const Campaign::ScenarioData & scenario = scenarios[chosenScenarioID];

        if ( !scenario.getStartScenarioVideoPlayback().empty() ) {
            AGG::ResetMixer();

            for ( const Campaign::ScenarioIntroVideoInfo & videoInfo : scenario.getStartScenarioVideoPlayback() ) {
                Video::ShowVideo( videoInfo.fileName, videoInfo.action );
            }

            AGG::ResetMixer();
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
            AGG::PlayMusic( MUS::ROLAND_CAMPAIGN_SCREEN, true );
            break;
        case Campaign::ARCHIBALD_CAMPAIGN:
            AGG::PlayMusic( MUS::ARCHIBALD_CAMPAIGN_SCREEN, true );
            break;
        default:
            // Implementing a new campaign? Add a new case!
            assert( 0 );
            break;
        }
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

    if ( !isLoadingSaveFile ) {
        saveData.addCurrentMapToFinished();
        saveData.addDaysPassed( world.CountDay() );
        Game::SaveCompletedCampaignScenario();
    }

    const int lastCompletedScenarioID = saveData.getLastCompletedScenarioID();
    const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( saveData.getCampaignID() );

    const std::vector<Campaign::CampaignAwardData> obtainableAwards
        = Campaign::CampaignAwardData::getCampaignAwardData( saveData.getCampaignID(), lastCompletedScenarioID );

    // TODO: Check for awards that have to be obtained with 'freak' conditions
    for ( size_t i = 0; i < obtainableAwards.size(); ++i ) {
        const uint32_t awardType = obtainableAwards[i]._type;

        if ( awardType == Campaign::CampaignAwardData::AwardType::TYPE_CARRY_OVER_FORCES ) {
            Kingdom & humanKingdom = world.GetKingdom( Players::HumanColors() );

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

    // TODO: do proper calc based on all scenarios cleared?
    if ( campaignData.isLastScenario( lastCompletedScenarioID ) ) {
        Game::ShowCredits();

        AGG::ResetMixer();
        Video::ShowVideo( "WIN.SMK", Video::VideoAction::WAIT_FOR_USER_INPUT );
        return fheroes2::GameMode::HIGHSCORES;
    }

    const int firstNextMap = campaignData.getScenariosAfter( lastCompletedScenarioID ).front();
    saveData.setCurrentScenarioID( firstNextMap );
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

    const int chosenScenarioID = campaignSaveData.getCurrentScenarioID();
    const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
    const Campaign::ScenarioData & scenario = scenarios[chosenScenarioID];

    if ( !allowToRestart ) {
        playCurrentScenarioVideo();
    }

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

    const fheroes2::Sprite & pressedButton = fheroes2::AGG::GetICN( ICN::CAMPXTRG, allowToRestart ? 9 : 8 );
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
    drawObtainedCampaignAwards( campaignSaveData.getObtainedCampaignAwards(), top );

    std::vector<int> selectableScenarios;
    if ( allowToRestart ) {
        selectableScenarios.emplace_back( chosenScenarioID );
    }
    else {
        selectableScenarios
            = campaignSaveData.isStarting() ? campaignData.getStartingScenarios() : campaignData.getScenariosAfter( campaignSaveData.getLastCompletedScenarioID() );
    }

    const uint32_t selectableScenariosCount = static_cast<uint32_t>( selectableScenarios.size() );

    fheroes2::ButtonGroup selectableScenarioButtons;

    const int highlightedScenarioId = allowToRestart ? chosenScenarioID : -1;
    DrawCampaignScenarioIcons( selectableScenarioButtons, campaignData, top, highlightedScenarioId );

    for ( uint32_t i = 0; i < selectableScenariosCount; ++i ) {
        if ( chosenScenarioID == selectableScenarios[i] )
            selectableScenarioButtons.button( i ).press();

        selectableScenarioButtons.button( i ).draw();
    }

    LocalEvent & le = LocalEvent::Get();

    display.render();

    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();
        le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.MousePressLeft( buttonViewIntro.area() ) ? buttonViewIntro.drawOnPress() : buttonViewIntro.drawOnRelease();

        if ( allowToRestart ) {
            le.MousePressLeft( buttonRestart.area() ) ? buttonRestart.drawOnPress() : buttonRestart.drawOnRelease();
        }
        else {
            for ( uint32_t i = 0; i < bonusChoiceCount; ++i ) {
                if ( le.MousePressLeft( buttonChoices.button( i ).area() ) ) {
                    buttonChoices.button( i ).press();
                    optionButtonGroup.draw();
                    scenarioBonus = bonusChoices[i];

                    break;
                }
            }
        }

        for ( uint32_t i = 0; i < selectableScenariosCount; ++i ) {
            if ( chosenScenarioID != selectableScenarios[i] && le.MousePressLeft( selectableScenarioButtons.button( i ).area() ) ) {
                campaignSaveData.setCurrentScenarioID( selectableScenarios[i] );
                return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            }
        }

        if ( le.MouseClickLeft( buttonCancel.area() ) || HotKeyPressEvent( EVENT_DEFAULT_EXIT ) ) {
            return prevMode;
        }
        if ( ( buttonOk.isEnabled() && ( le.MouseClickLeft( buttonOk.area() ) || HotKeyPressEvent( EVENT_DEFAULT_READY ) ) )
             || ( buttonRestart.isEnabled() && le.MouseClickLeft( buttonRestart.area() ) ) ) {
            const Maps::FileInfo mapInfo = scenario.loadMap();
            conf.SetCurrentFileInfo( mapInfo );

            // starting faction scenario bonus has to be called before players.SetStartGame()
            if ( scenarioBonus._type == Campaign::ScenarioBonusData::STARTING_RACE || scenarioBonus._type == Campaign::ScenarioBonusData::STARTING_RACE_AND_ARMY ) {
                // but the army has to be set after starting the game, so first only set the race
                SetScenarioBonus( campaignSaveData.getCampaignID(), chosenScenarioID,
                                  { Campaign::ScenarioBonusData::STARTING_RACE, scenarioBonus._subType, scenarioBonus._amount } );
            }

            Players & players = conf.GetPlayers();
            players.SetStartGame();
            if ( conf.ExtGameUseFade() )
                fheroes2::FadeDisplay();

            fheroes2::ImageRestorer restorer( display );
            Game::ShowMapLoadingText();
            conf.SetGameType( Game::TYPE_CAMPAIGN );

            if ( !world.LoadMapMP2( mapInfo.file ) ) {
                Dialog::Message( _( "Campaign Scenario loading failure" ), _( "Please make sure that campaign files are correct and present." ), Font::BIG, Dialog::OK );
                conf.SetCurrentFileInfo( Maps::FileInfo() );
                continue;
            }

            restorer.reset();

            // meanwhile, the others should be called after players.SetStartGame()
            if ( scenarioBonus._type != Campaign::ScenarioBonusData::STARTING_RACE ) {
                SetScenarioBonus( campaignSaveData.getCampaignID(), chosenScenarioID, scenarioBonus );
            }

            applyObtainedCampaignAwards( chosenScenarioID, campaignSaveData.getObtainedCampaignAwards() );

            campaignSaveData.setCurrentScenarioBonus( scenarioBonus );
            campaignSaveData.setCurrentScenarioID( chosenScenarioID );

            return fheroes2::GameMode::START_GAME;
        }
        else if ( le.MouseClickLeft( buttonViewIntro.area() ) ) {
            AGG::ResetMixer();
            fheroes2::ImageRestorer restorer( display, top.x, top.y, backgroundImage.width(), backgroundImage.height() );
            playPreviosScenarioVideo();
            playCurrentScenarioVideo();

            playCampaignMusic( chosenCampaignID );
        }
    }

    return prevMode;
}
