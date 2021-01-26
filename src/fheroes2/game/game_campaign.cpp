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

#include "agg.h"
#include "assert.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "settings.h"
#include "text.h"
#include "world.h"

namespace
{
    std::vector<Campaign::ScenarioBonusData> getRolandCampaignBonusData( const int scenarioID )
    {
        assert( scenarioID >= 0 && scenarioID <= 10 );
        std::vector<Campaign::ScenarioBonusData> bonus;

        switch ( scenarioID ) {
        case 0:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        case 1:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        case 2:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        case 3:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        }

        return bonus;
    }

    std::vector<Campaign::ScenarioBonusData> getArchibaldCampaignBonusData( const int scenarioID )
    {
        assert( scenarioID >= 0 && scenarioID <= 10 );
        std::vector<Campaign::ScenarioBonusData> bonus;

        switch ( scenarioID ) {
        case 0:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        case 1:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        case 2:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        }

        return bonus;
    }

    std::vector<Campaign::ScenarioBonusData> getCampaignBonusData( const int campaignID, const int scenarioID )
    {
        assert( scenarioID >= 0 );
        switch ( campaignID ) {
        case 0:
            return getRolandCampaignBonusData( scenarioID );
        case 1:
            return getArchibaldCampaignBonusData( scenarioID );
        }

        // shouldn't be here unless we get an unsupported campaign
        return std::vector<Campaign::ScenarioBonusData>();
    }

    const std::string rolandCampaignDescription[] = {_(
        "Roland needs you to defeat the lords near his castle to begin his war of rebellion against his brother.  They are not allied with each other, so they will spend"
        " most of their time fighting with one another.  Victory is yours when you have defeated all of their castles and heroes." )};

    const std::string archibaldCampaignDescription[] = {_(
        "King Archibald requires you to defeat the three enemies in this region.  They are not allied with one another, so they will spend most of their energy fighting"
        " amongst themselves.  You will win when you own all of the enemy castles and there are no more heroes left to fight." )};

    void DrawCampaignScenarioIcon( const int icnId, int const iconIdx, const fheroes2::Point & offset, const int posX, const int posY )
    {
        fheroes2::Blit( fheroes2::AGG::GetICN( icnId, iconIdx ), fheroes2::Display::instance(), offset.x + posX, offset.y + posY );
    }

    // TODO: return buttonGroup?
    void DrawCampaignScenarioIcons( const Campaign::CampaignData campaignData, const fheroes2::Point & top )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const bool isGoodCampaign = campaignData.isGoodCampaign();
        const fheroes2::Point trackOffset = fheroes2::Point( top.x + 39, top.y + 294 );

        int campaignTrack = ICN::CTRACK00;
        switch ( campaignData.getCampaignID() ) {
        case 0:
            campaignTrack = ICN::CTRACK00;
            break; // roland
        case 1:
            campaignTrack = ICN::CTRACK03;
            break; // archibald
        }

        fheroes2::Blit( fheroes2::AGG::GetICN( campaignTrack, 0 ), display, top.x + 39, top.y + 294 );

        const int iconsId = isGoodCampaign ? ICN::CAMPXTRG : ICN::CAMPXTRE;
        const int selectedIconIdx = isGoodCampaign ? Campaign::SCENARIOICON_GOOD_SELECTED : Campaign::SCENARIOICON_EVIL_SELECTED;
        // TODO: find icon Idx for done (yellow ring border)

        const int middleY = 40, deltaY = 42;
        const int startX = -2, deltaX = 74;

        const std::vector<Campaign::ScenarioData> scenarios = campaignData.getAllScenarios();
        const Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();

        int currentX = startX;
        std::vector<int> prevScenarioNextMaps;
        const std::vector<int> & completedScenarioNextMaps = campaignData.getScenariosAfter( saveData.getCurrentScenarioID() );
        const std::vector<int> & clearedMaps = saveData.getFinishedMaps();
        for ( int i = 0, scenarioCount = scenarios.size(); i < scenarioCount; ++i ) {
            const std::vector<int> nextMaps = scenarios[i].getNextMaps();

            // sub scenario -> this scenario's next map is one of the prev scenario's next map
            bool isSubScenario = false;
            int x = currentX, y = middleY;

            for ( int j = 0, prevScenarioNextMapsCount = prevScenarioNextMaps.size(); j < prevScenarioNextMapsCount; ++j ) {
                if ( std::none_of( nextMaps.begin(), nextMaps.end(), [&]( int scenarioID ) { return scenarioID == prevScenarioNextMaps[j]; } ) )
                    continue;

                isSubScenario = true;
                x -= deltaX / 2;
                y -= deltaY;
                break;
            }

            // if it's not a sub-scenario, try to check whether it's a branching scenario
            bool isBranching = !isSubScenario && prevScenarioNextMaps.size() > 1;
            bool isFinalBranch = false;

            if ( !isSubScenario && prevScenarioNextMaps.size() > 1 ) {
                isBranching = true;
                isFinalBranch = prevScenarioNextMaps.back() == i;

                y += isFinalBranch ? deltaY : -deltaY;
            }

            int iconIdx = 0;

            // currently selected scenario
            if ( saveData.getCurrentScenarioID() == i )
                iconIdx = selectedIconIdx;
            // another scenario you can select
            else if ( std::any_of( completedScenarioNextMaps.begin(), completedScenarioNextMaps.end(), [&]( const int scenarioID ) { return scenarioID == i; } ) )
                iconIdx = Campaign::SCENARIOICON_AVAILABLE;
            // cleared scenario
            else if ( std::any_of( clearedMaps.begin(), clearedMaps.end(), [&]( const int scenarioID ) { return scenarioID == i; } ) )
                iconIdx = Campaign::SCENARIOICON_CLEARED;
            else
                iconIdx = Campaign::SCENARIOICON_UNAVAILABLE;

            DrawCampaignScenarioIcon( iconsId, iconIdx, trackOffset, x, y );

            if ( !isBranching || isFinalBranch )
                prevScenarioNextMaps = nextMaps;

            if ( !isSubScenario && ( !isBranching || isFinalBranch ) )
                currentX += deltaX;
        }
    }

    const Campaign::CampaignData GetRolandCampaignData()
    {
        // TODO: Do all campaign data until CAMPG10.H2C, for now we'll test until mission 4 (in which mission 3 Save The Dwarves is optional)
        std::vector<Campaign::ScenarioData> scenarioDatas;
        scenarioDatas.emplace_back(
            Campaign::ScenarioData( 0, std::vector<int>{1}, getCampaignBonusData( 0, 0 ), std::string( "CAMPG01.H2C" ), rolandCampaignDescription[0] ) );
        scenarioDatas.emplace_back(
            Campaign::ScenarioData( 1, std::vector<int>{2, 3}, getCampaignBonusData( 0, 1 ), std::string( "CAMPG02.H2C" ), rolandCampaignDescription[0] ) );
        scenarioDatas.emplace_back(
            Campaign::ScenarioData( 2, std::vector<int>{3}, getCampaignBonusData( 0, 2 ), std::string( "CAMPG03.H2C" ), rolandCampaignDescription[0] ) );
        scenarioDatas.emplace_back(
            Campaign::ScenarioData( 3, std::vector<int>{4}, getCampaignBonusData( 0, 3 ), std::string( "CAMPG04.H2C" ), rolandCampaignDescription[0] ) );

        Campaign::CampaignData campaignData = Campaign::CampaignData();
        campaignData.setCampaignID( 0 );
        campaignData.setCampaignDescription( "Roland Campaign" );
        campaignData.setCampaignAlignment( true );
        campaignData.setCampaignScenarios( scenarioDatas );

        return campaignData;
    }

    const Campaign::CampaignData GetArchibaldCampaignData()
    {
        // TODO: Do all campaign data until CAMPE10.H2C
        std::vector<Campaign::ScenarioBonusData> bonus;
        bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
        bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
        bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );

        std::vector<Campaign::ScenarioData> scenarioDatas;
        scenarioDatas.emplace_back(
            Campaign::ScenarioData( 0, std::vector<int>{1}, getCampaignBonusData( 1, 0 ), std::string( "CAMPE01.H2C" ), archibaldCampaignDescription[0] ) );
        scenarioDatas.emplace_back(
            Campaign::ScenarioData( 1, std::vector<int>{2}, getCampaignBonusData( 1, 1 ), std::string( "CAMPE02.H2C" ), archibaldCampaignDescription[0] ) );
        scenarioDatas.emplace_back(
            Campaign::ScenarioData( 2, std::vector<int>{}, getCampaignBonusData( 1, 2 ), std::string( "CAMPE03.H2C" ), archibaldCampaignDescription[0] ) );

        Campaign::CampaignData campaignData = Campaign::CampaignData();
        campaignData.setCampaignID( 1 );
        campaignData.setCampaignDescription( "Archibald Campaign" );
        campaignData.setCampaignAlignment( false );
        campaignData.setCampaignScenarios( scenarioDatas );

        return campaignData;
    }

    const Campaign::CampaignData GetCampaignData( int campaignID )
    {
        switch ( campaignID ) {
        case 0:
            return GetRolandCampaignData();
        case 1:
            return GetArchibaldCampaignData();
        default:
            return Campaign::CampaignData();
        }
    }

    void SetScenarioBonus( const Campaign::ScenarioBonusData & scenarioBonus )
    {
        const Players & sortedPlayers = Settings::Get().GetPlayers();
        for ( Players::const_iterator it = sortedPlayers.begin(); it != sortedPlayers.end(); ++it ) {
            if ( !*it )
                continue;

            const Player & player = ( **it );
            if ( !player.isControlHuman() )
                continue;

            Kingdom & kingdom = world.GetKingdom( player.GetColor() );

            switch ( scenarioBonus._type ) {
            case Campaign::ScenarioBonusData::RESOURCES:
                kingdom.AddFundsResource( Funds( scenarioBonus._subType, scenarioBonus._amount ) );
                break;
            case Campaign::ScenarioBonusData::ARTIFACT:
                kingdom.GetBestHero()->PickupArtifact( Artifact( scenarioBonus._subType ) );
                break;
            case Campaign::ScenarioBonusData::TROOP:
                kingdom.GetBestHero()->GetArmy().JoinTroop( Troop( Monster( scenarioBonus._subType ), scenarioBonus._amount ) );
                break;
            default:
                assert( 0 );
            }
        }
    }
}

bool Game::IsOriginalCampaignPresent()
{
    return GetRolandCampaignData().isAllCampaignMapsPresent() && GetArchibaldCampaignData().isAllCampaignMapsPresent();
}

int Game::CompleteCampaignScenario()
{
    Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();

    saveData.addCurrentMapToFinished();
    const int lastCompletedScenarioID = saveData.getLastCompletedScenarioID();
    const Campaign::CampaignData & campaignData = GetCampaignData( saveData.getCampaignID() );

    // TODO: do proper calc based on all scenarios cleared?
    if ( campaignData.isLastScenario( lastCompletedScenarioID ) ) {
        return Game::HIGHSCORES;
    }
    // if not, eventually call SelectCampaignScenario() again
    else {
        const int firstNextMap = campaignData.getScenariosAfter( lastCompletedScenarioID ).front();
        saveData.setCurrentScenarioID( firstNextMap );
        return Game::SELECTCAMPAIGNSCENARIO;
    }
}

int Game::SelectCampaignScenario()
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Settings & conf = Settings::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    Campaign::CampaignSaveData & campaignSaveData = Campaign::CampaignSaveData::Get();
    const size_t chosenCampaignID = campaignSaveData.getCampaignID();

    const Campaign::CampaignData & campaignData = GetCampaignData( chosenCampaignID );
    const bool goodCampaign = campaignData.isGoodCampaign();

    const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( goodCampaign ? ICN::CAMPBKGG : ICN::CAMPBKGE, 0 );
    const fheroes2::Point top( ( display.width() - backgroundImage.width() ) / 2, ( display.height() - backgroundImage.height() ) / 2 );

    fheroes2::Blit( backgroundImage, display, top.x, top.y );

    fheroes2::Button buttonViewIntro( top.x + 22, top.y + 431, goodCampaign ? ICN::CAMPXTRG : ICN::CAMPXTRE, 0, 1 );
    fheroes2::Button buttonOk( top.x + 367, top.y + 431, goodCampaign ? ICN::CAMPXTRG : ICN::CAMPXTRE, 4, 5 );
    fheroes2::Button buttonCancel( top.x + 511, top.y + 431, goodCampaign ? ICN::CAMPXTRG : ICN::CAMPXTRE, 6, 7 );

    const fheroes2::Point optionButtonOffset( 590, 199 );
    const int32_t optionButtonStep = 22;

    const fheroes2::Sprite & pressedButton = fheroes2::AGG::GetICN( ICN::CAMPXTRG, 8 );
    fheroes2::Sprite releaseButton( pressedButton.width(), pressedButton.height(), pressedButton.x(), pressedButton.y() );
    fheroes2::Copy( backgroundImage, optionButtonOffset.x + pressedButton.x(), optionButtonOffset.y + pressedButton.y(), releaseButton, 0, 0, releaseButton.width(),
                    releaseButton.height() );

    const int chosenScenarioID = campaignSaveData.getCurrentScenarioID();
    const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
    const Campaign::ScenarioData scenario = scenarios[chosenScenarioID];

    const std::vector<Campaign::ScenarioBonusData> & bonusChoices = scenario.getBonuses();
    const uint32_t bonusChoiceCount = static_cast<uint32_t>( bonusChoices.size() );

    fheroes2::ButtonGroup buttonChoices;
    fheroes2::OptionButtonGroup optionButtonGroup;

    for ( uint32_t i = 0; i < bonusChoiceCount; ++i ) {
        buttonChoices.createButton( optionButtonOffset.x + top.x, optionButtonOffset.y + optionButtonStep * i + top.y, releaseButton, pressedButton, i );
        optionButtonGroup.addButton( &buttonChoices.button( i ) );
    }

    Campaign::ScenarioBonusData scenarioBonus;

    // in case there's no bonus for the map
    if ( bonusChoiceCount > 0 ) {
        scenarioBonus = bonusChoices[0];
        buttonChoices.button( 0 ).press();
    }

    buttonViewIntro.disable();
    buttonViewIntro.draw();

    if ( scenarios.empty() )
        buttonOk.disable();
    buttonOk.draw();
    buttonCancel.draw();

    for ( uint32_t i = 0; i < bonusChoiceCount; ++i )
        buttonChoices.button( i ).draw();

    Text textDaysSpent( "0", Font::BIG );
    textDaysSpent.Blit( top.x + 574 + textDaysSpent.w() / 2, top.y + 31 );

    const auto mapInfo = scenario.loadMap();
    if ( !scenarios.empty() ) {
        const std::string & mapNameStr = mapInfo.description;
        TextBox mapName( mapNameStr.substr( 1, mapNameStr.length() - 2 ), Font::BIG, 200 );
        mapName.Blit( top.x + 197, top.y + 97 - mapName.h() / 2 );

        Text campaignMapId( "1", Font::BIG );
        campaignMapId.Blit( top.x + 172 - campaignMapId.w() / 2, top.y + 97 - campaignMapId.h() / 2 );

        TextBox mapDescription( scenario.getDescription(), Font::BIG, 356 );
        mapDescription.Blit( top.x + 34, top.y + 132 );

        const int textChoiceWidth = 150;
        for ( uint32_t i = 0; i < bonusChoiceCount; ++i ) {
            Text choice( bonusChoices[i].ToString(), Font::BIG );
            choice.Blit( top.x + 425, top.y + 209 + 22 * i - choice.h() / 2, textChoiceWidth );
        }
    }
    else {
        TextBox textCaption( "We are working hard to ensure that the support of Campaign would arrive as soon as possible", Font::YELLOW_BIG, 350 );
        textCaption.Blit( top.x + 40, top.y + 140 );

        TextBox textDescription( "Campaign Game mode is under construction", Font::BIG, 350 );
        textDescription.Blit( top.x + 40, top.y + 200 );
    }

    DrawCampaignScenarioIcons( campaignData, top );
    LocalEvent & le = LocalEvent::Get();

    cursor.Show();
    display.render();

    while ( le.HandleEvents() ) {
        if ( !buttonCancel.isDisabled() )
            le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();
        if ( !buttonOk.isDisabled() )
            le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();

        for ( uint32_t i = 0; i < bonusChoiceCount; ++i ) {
            if ( le.MousePressLeft( buttonChoices.button( i ).area() ) ) {
                buttonChoices.button( i ).press();
                optionButtonGroup.draw();
                scenarioBonus = bonusChoices[i];

                break;
            }
        }

        if ( le.MouseClickLeft( buttonCancel.area() ) )
            return Game::NEWGAME;
        else if ( !buttonOk.isDisabled() && le.MouseClickLeft( buttonOk.area() ) ) {
            conf.SetCurrentFileInfo( mapInfo );
            Players & players = conf.GetPlayers();
            players.SetStartGame();
            if ( conf.ExtGameUseFade() )
                fheroes2::FadeDisplay();
            Game::ShowMapLoadingText();
            conf.SetGameType( Game::TYPE_CAMPAIGN );

            if ( !world.LoadMapMP2( mapInfo.file ) ) {
                Dialog::Message( "Campaign Game loading failure", "Please make sure that campaign files are correct and present", Font::SMALL, Dialog::OK );
                conf.SetCurrentFileInfo( Maps::FileInfo() );
                continue;
            }

            campaignSaveData.setCurrentScenarioBonus( scenarioBonus );
            campaignSaveData.setCurrentScenarioID( chosenScenarioID );

            SetScenarioBonus( scenarioBonus );

            return Game::STARTGAME;
        }
    }

    return Game::NEWGAME;
}
