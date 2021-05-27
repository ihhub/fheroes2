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
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_io.h"
#include "game_video.h"
#include "icn.h"
#include "race.h"
#include "settings.h"
#include "text.h"
#include "world.h"

namespace
{
    void DrawCampaignScenarioIcon( const int icnId, const int iconIdx, const fheroes2::Point & offset, const int posX, const int posY )
    {
        fheroes2::Blit( fheroes2::AGG::GetICN( icnId, iconIdx ), fheroes2::Display::instance(), offset.x + posX, offset.y + posY );
    }

    void DrawCampaignScenarioIcons( fheroes2::ButtonGroup & buttonGroup, const Campaign::CampaignData & campaignData, const fheroes2::Point & top )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const bool isGoodCampaign = campaignData.isGoodCampaign();
        const fheroes2::Point trackOffset( top.x + 39, top.y + 294 );

        int campaignTrack = ICN::CTRACK00;
        switch ( campaignData.getCampaignID() ) {
        case Campaign::ROLAND_CAMPAIGN:
            campaignTrack = ICN::CTRACK00;
            break;
        case Campaign::ARCHIBALD_CAMPAIGN:
            campaignTrack = ICN::CTRACK03;
            break;
        case Campaign::PRICE_OF_LOYALTY_CAMPAIGN:
            campaignTrack = ICN::X_TRACK0;
            break;
        case Campaign::DESCENDANTS_CAMPAIGN:
            campaignTrack = ICN::X_TRACK1;
            break;
        case Campaign::WIZARDS_ISLE_CAMPAIGN:
            campaignTrack = ICN::X_TRACK2;
            break;
        case Campaign::VOYAGE_HOME_CAMPAIGN:
            campaignTrack = ICN::X_TRACK3;
            break;
        }

        fheroes2::Blit( fheroes2::AGG::GetICN( campaignTrack, 0 ), display, top.x + 39, top.y + 294 );

        const int iconsId = isGoodCampaign ? ICN::CAMPXTRG : ICN::CAMPXTRE;
        const int selectedIconIdx = isGoodCampaign ? Campaign::SCENARIOICON_GOOD_SELECTED : Campaign::SCENARIOICON_EVIL_SELECTED;

        const int middleY = 40;
        const int deltaY = 42;
        const int startX = -2;
        const int deltaX = 74;

        const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
        const Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();

        int currentX = startX;
        std::vector<int> prevScenarioNextMaps;
        const std::vector<int> & clearedMaps = saveData.getFinishedMaps();
        const std::vector<int> & availableMaps
            = saveData.isStarting() ? campaignData.getStartingScenarios() : campaignData.getScenariosAfter( saveData.getLastCompletedScenarioID() );

        size_t drawnBranchMapCount = 0;

        for ( size_t i = 0; i < scenarios.size(); ++i ) {
            const std::vector<int> & nextMaps = scenarios[i].getNextMaps();
            const int scenarioID = scenarios[i].getScenarioID();

            // sub scenario -> this scenario's next map is one of the prev scenario's next map
            // an example in original campaign would be Save/Slay the Dwarves
            bool isSubScenario = false;
            int x = currentX;
            int y = middleY;

            for ( size_t j = 0; j < prevScenarioNextMaps.size(); ++j ) {
                if ( std::find( nextMaps.begin(), nextMaps.end(), prevScenarioNextMaps[j] ) == nextMaps.end() )
                    continue;

                isSubScenario = true;
                x -= deltaX / 2;
                y -= deltaY;
                break;
            }

            // if it's not a sub-scenario, try to check whether it's a branching scenario
            bool isBranching = false;
            bool isFinalBranch = false;

            const size_t branchCount = prevScenarioNextMaps.size();
            if ( !isSubScenario && branchCount > 1 ) {
                isBranching = true;
                isFinalBranch = drawnBranchMapCount == branchCount - 1;

                y += isFinalBranch ? deltaY : -deltaY;
                ++drawnBranchMapCount;
            }

            // available scenario (one of which should be selected)
            if ( std::find( availableMaps.begin(), availableMaps.end(), scenarioID ) != availableMaps.end() ) {
                buttonGroup.createButton( trackOffset.x + x, trackOffset.y + y, fheroes2::AGG::GetICN( iconsId, Campaign::SCENARIOICON_AVAILABLE ),
                                          fheroes2::AGG::GetICN( iconsId, selectedIconIdx ), static_cast<int>( i ) );
            }
            // cleared scenario
            else if ( std::find( clearedMaps.begin(), clearedMaps.end(), static_cast<int>( i ) ) != clearedMaps.end() ) {
                DrawCampaignScenarioIcon( iconsId, Campaign::SCENARIOICON_CLEARED, trackOffset, x, y );
            }
            else {
                DrawCampaignScenarioIcon( iconsId, Campaign::SCENARIOICON_UNAVAILABLE, trackOffset, x, y );
            }

            if ( !isBranching || isFinalBranch ) {
                prevScenarioNextMaps = nextMaps;
                drawnBranchMapCount = 0;
            }

            if ( !isSubScenario && ( !isBranching || isFinalBranch ) )
                currentX += deltaX;
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

            award.Blit( top.x + 425, top.y + 100 + yOffset * static_cast<int>( i ) - award.h() / 2, textAwardWidth );
        }
    }

    void SetScenarioBonus( const Campaign::ScenarioBonusData & scenarioBonus )
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
                Heroes * hero = kingdom.GetBestHero();
                assert( hero != nullptr );
                if ( hero != nullptr ) {
                    hero->PickupArtifact( Artifact( scenarioBonus._subType ) );
                }
            } break;
            case Campaign::ScenarioBonusData::TROOP:
                kingdom.GetBestHero()->GetArmy().JoinTroop( Troop( Monster( scenarioBonus._subType ), scenarioBonus._amount ) );
                break;
            case Campaign::ScenarioBonusData::SPELL: {
                KingdomHeroes & heroes = kingdom.GetHeroes();
                assert( !heroes.empty() );
                if ( !heroes.empty() ) {
                    // TODO: make sure that the correct hero receives the spell. Right now it's a semi-hacky way to do this.
                    heroes.back()->AppendSpellToBook( scenarioBonus._subType, true );
                }
            } break;
            case Campaign::ScenarioBonusData::STARTING_RACE:
                Players::SetPlayerRace( player->GetColor(), scenarioBonus._subType );
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
        Kingdom & humanKingdom = world.GetKingdom( sortedPlayers.HumanColors() );

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
                            kingdom.RemoveHeroes( heroes[j] );
                            break;
                        }
                    }
                }
                break;
            case Campaign::CampaignAwardData::TYPE_CARRY_OVER_FORCES:
                const std::vector<Troop> & carryOverTroops = Campaign::CampaignSaveData::Get().getCarryOverTroops();
                Army & bestHeroArmy = humanKingdom.GetBestHero()->GetArmy();
                bestHeroArmy.Clean();

                for ( uint32_t troopID = 0; troopID < carryOverTroops.size(); ++troopID )
                    bestHeroArmy.GetTroop( troopID )->Set( carryOverTroops[troopID] );

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

fheroes2::GameMode Game::CompleteCampaignScenario()
{
    Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();

    saveData.addCurrentMapToFinished();
    saveData.addDaysPassed( world.CountDay() );

    const int lastCompletedScenarioID = saveData.getLastCompletedScenarioID();
    const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( saveData.getCampaignID() );

    Game::SaveCompletedCampaignScenario();

    const std::vector<Campaign::CampaignAwardData> obtainableAwards
        = Campaign::CampaignAwardData::getCampaignAwardData( saveData.getCampaignID(), lastCompletedScenarioID );

    // TODO: Check for awards that have to be obtained with 'freak' conditions
    for ( size_t i = 0; i < obtainableAwards.size(); ++i ) {
        saveData.addCampaignAward( obtainableAwards[i]._id );

        if ( obtainableAwards[i]._type == Campaign::CampaignAwardData::AwardType::TYPE_CARRY_OVER_FORCES ) {
            Kingdom & humanKingdom = world.GetKingdom( Settings::Get().GetPlayers().HumanColors() );

            const Heroes * lastBattleWinHero = humanKingdom.GetLastBattleWinHero();

            if ( lastBattleWinHero )
                saveData.setCarryOverTroops( lastBattleWinHero->GetArmy() );
        }
    }

    playPreviosScenarioVideo();

    // TODO: do proper calc based on all scenarios cleared?
    if ( campaignData.isLastScenario( lastCompletedScenarioID ) ) {
        AGG::ResetMixer();
        Video::ShowVideo( "WIN.SMK", Video::VideoAction::WAIT_FOR_USER_INPUT );
        return fheroes2::GameMode::HIGHSCORES;
    }

    const int firstNextMap = campaignData.getScenariosAfter( lastCompletedScenarioID ).front();
    saveData.setCurrentScenarioID( firstNextMap );
    return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
}

fheroes2::GameMode Game::SelectCampaignScenario( const fheroes2::GameMode prevMode )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    display.fill( 0 );
    Settings & conf = Settings::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
    const int chosenCampaignID = campaignSaveData.getCampaignID();

    const Campaign::CampaignData & campaignData = Campaign::CampaignData::getCampaignData( chosenCampaignID );
    const bool goodCampaign = campaignData.isGoodCampaign();

    const int chosenScenarioID = campaignSaveData.getCurrentScenarioID();
    const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
    const Campaign::ScenarioData & scenario = scenarios[chosenScenarioID];

    playCurrentScenarioVideo();

    int backgroundIconID = 0;

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
        backgroundIconID = ICN::CAMPBKGG;
        break;
    }

    const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( backgroundIconID, 0 );
    const fheroes2::Point top( ( display.width() - backgroundImage.width() ) / 2, ( display.height() - backgroundImage.height() ) / 2 );

    fheroes2::Blit( backgroundImage, display, top.x, top.y );

    const int buttonIconID = goodCampaign ? ICN::CAMPXTRG : ICN::CAMPXTRE;
    fheroes2::Button buttonViewIntro( top.x + 22, top.y + 431, buttonIconID, 0, 1 );
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

    buttonOk.draw();
    buttonCancel.draw();

    for ( uint32_t i = 0; i < bonusChoiceCount; ++i )
        buttonChoices.button( i ).draw();

    Text textDaysSpent( std::to_string( campaignSaveData.getDaysPassed() ), Font::BIG );
    textDaysSpent.Blit( top.x + 574 + textDaysSpent.w() / 2, top.y + 31 );

    DrawCampaignScenarioDescription( scenario, top );
    drawObtainedCampaignAwards( campaignSaveData.getObtainedCampaignAwards(), top );

    const std::vector<int> & selectableScenarios
        = campaignSaveData.isStarting() ? campaignData.getStartingScenarios() : campaignData.getScenariosAfter( campaignSaveData.getLastCompletedScenarioID() );
    const uint32_t selectableScenariosCount = static_cast<uint32_t>( selectableScenarios.size() );

    fheroes2::ButtonGroup selectableScenarioButtons;
    DrawCampaignScenarioIcons( selectableScenarioButtons, campaignData, top );

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

        for ( uint32_t i = 0; i < bonusChoiceCount; ++i ) {
            if ( le.MousePressLeft( buttonChoices.button( i ).area() ) ) {
                buttonChoices.button( i ).press();
                optionButtonGroup.draw();
                scenarioBonus = bonusChoices[i];

                break;
            }
        }

        for ( uint32_t i = 0; i < selectableScenariosCount; ++i ) {
            if ( le.MousePressLeft( selectableScenarioButtons.button( i ).area() ) ) {
                campaignSaveData.setCurrentScenarioID( selectableScenarios[i] );
                return fheroes2::GameMode::SELECT_CAMPAIGN_SCENARIO;
            }
        }

        if ( le.MouseClickLeft( buttonCancel.area() ) ) {
            return prevMode;
        }
        else if ( le.MouseClickLeft( buttonOk.area() ) ) {
            const Maps::FileInfo mapInfo = scenario.loadMap();
            conf.SetCurrentFileInfo( mapInfo );

            // starting faction scenario bonus has to be called before players.SetStartGame()
            if ( scenarioBonus._type == Campaign::ScenarioBonusData::STARTING_RACE )
                SetScenarioBonus( scenarioBonus );

            Players & players = conf.GetPlayers();
            players.SetStartGame();
            if ( conf.ExtGameUseFade() )
                fheroes2::FadeDisplay();
            Game::ShowMapLoadingText();
            conf.SetGameType( Game::TYPE_CAMPAIGN );

            if ( !world.LoadMapMP2( mapInfo.file ) ) {
                Dialog::Message( _( "Campaign Game loading failure" ), _( "Please make sure that campaign files are correct and present" ), Font::SMALL, Dialog::OK );
                conf.SetCurrentFileInfo( Maps::FileInfo() );
                continue;
            }

            // meanwhile, the others should be called after players.SetStartGame()
            if ( scenarioBonus._type != Campaign::ScenarioBonusData::STARTING_RACE )
                SetScenarioBonus( scenarioBonus );

            applyObtainedCampaignAwards( chosenScenarioID, campaignSaveData.getObtainedCampaignAwards() );

            campaignSaveData.setCurrentScenarioBonus( scenarioBonus );
            campaignSaveData.setCurrentScenarioID( chosenScenarioID );

            return fheroes2::GameMode::START_GAME;
        }
        else if ( le.MouseClickLeft( buttonViewIntro.area() ) ) {
            fheroes2::ImageRestorer restorer( display, top.x, top.y, backgroundImage.width(), backgroundImage.height() );
            playPreviosScenarioVideo();
            playCurrentScenarioVideo();
        }
    }

    return prevMode;
}
