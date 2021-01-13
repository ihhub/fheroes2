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
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "settings.h"
#include "text.h"
#include "world.h"

namespace
{
    std::vector<Campaign::ScenarioBonusData> getCampaignBonusData( const int /*campaignID*/, const int scenarioId )
    {
        assert( scenarioId >= 0 );
        std::vector<Campaign::ScenarioBonusData> bonus;

        // TODO: apply use of campaignID
        if ( scenarioId == 0 ) {
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
        }

        return bonus;
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

    bool hasEnding( const std::string & fullString, const std::string & ending )
    {
        if ( fullString.length() >= ending.length() )
            return ( 0 == fullString.compare( fullString.length() - ending.length(), ending.length(), ending ) );
        else
            return false;
    }

    std::vector<Maps::FileInfo> GetCampaignMaps( const std::vector<std::string> & fileNames, const std::string & fileExtension )
    {
        const ListFiles files = Settings::GetListFiles( "maps", fileExtension );
        std::vector<Maps::FileInfo> maps;

        for ( size_t i = 0; i < fileNames.size(); ++i ) {
            for ( ListFiles::const_iterator file = files.begin(); file != files.end(); ++file ) {
                // check if the obtained file's name matches the one we want
                if ( !hasEnding( *file, fileNames[i] ) )
                    continue;

                Maps::FileInfo fi;
                if ( fi.ReadMP2( *file ) ) {
                    maps.push_back( fi );
                }
            }
        }

        return maps;
    }

    const std::vector<Maps::FileInfo> GetRolandCampaign()
    {
        const std::vector<std::string> maps = {"CAMPG01.H2C", "CAMPG02.H2C", "CAMPG03.H2C", "CAMPG04.H2C", "CAMPG05.H2C", "CAMPG05B.H2C",
                                               "CAMPG06.H2C", "CAMPG07.H2C", "CAMPG08.H2C", "CAMPG09.H2C", "CAMPG10.H2C"};

        return GetCampaignMaps( maps, PRICE_OF_LOYALTY_CAMPAIGN_SCENARIO_FILE_EXTENSION );
    }

    const std::vector<Maps::FileInfo> GetArchibaldCampaign()
    {
        const std::vector<std::string> maps = {"CAMPE01.H2C", "CAMPE02.H2C", "CAMPE03.H2C", "CAMPE04.H2C", "CAMPE05.H2C", "CAMPE05B.H2C",
                                               "CAMPE06.H2C", "CAMPE07.H2C", "CAMPE08.H2C", "CAMPE09.H2C", "CAMPE10.H2C", "CAMPE11.H2C"};

        return GetCampaignMaps( maps, PRICE_OF_LOYALTY_CAMPAIGN_SCENARIO_FILE_EXTENSION );
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
    return !GetRolandCampaign().empty() && !GetArchibaldCampaign().empty();
}

int Game::SelectCampaignScenario()
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Settings & conf = Settings::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    Campaign::CampaignData & campaignData = Campaign::CampaignData::Get();
    const size_t chosenCampaign = campaignData.getCampaignID();
    const bool goodCampaign = chosenCampaign == 0;

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

    const int chosenScenarioID = 0;
    const std::vector<Campaign::ScenarioBonusData> bonusChoices = getCampaignBonusData( chosenCampaign, chosenScenarioID );
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

    const std::vector<Maps::FileInfo> & campaignMaps = goodCampaign ? GetRolandCampaign() : GetArchibaldCampaign();

    buttonViewIntro.disable();
    buttonViewIntro.draw();
    if ( campaignMaps.empty() )
        buttonOk.disable();
    buttonOk.draw();
    buttonCancel.draw();

    for ( uint32_t i = 0; i < bonusChoiceCount; ++i )
        buttonChoices.button( i ).draw();

    Text textDaysSpent( "0", Font::BIG );
    textDaysSpent.Blit( top.x + 574 + textDaysSpent.w() / 2, top.y + 31 );

    if ( !campaignMaps.empty() ) {
        const std::string & desc = campaignMaps[chosenScenarioID].description;
        TextBox mapName( desc.substr( 1, desc.length() - 2 ), Font::BIG, 200 );
        mapName.Blit( top.x + 197, top.y + 97 - mapName.h() / 2 );

        Text campaignMapId( "1", Font::BIG );
        campaignMapId.Blit( top.x + 172 - campaignMapId.w() / 2, top.y + 97 - campaignMapId.h() / 2 );

        TextBox mapDescription( goodCampaign ? rolandCampaignDescription[chosenScenarioID] : archibaldCampaignDescription[chosenScenarioID], Font::BIG, 356 );
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

    const int iconsId = goodCampaign ? ICN::CAMPXTRG : ICN::CAMPXTRE;
    const fheroes2::Point trackOffset = fheroes2::Point( top.x + 39, top.y + 294 );
    fheroes2::Blit( fheroes2::AGG::GetICN( goodCampaign ? ICN::CTRACK00 : ICN::CTRACK03, 0 ), display, top.x + 39, top.y + 294 );

    if ( goodCampaign ) {
        DrawCampaignScenarioIcon( iconsId, 14, trackOffset, -2, 40 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 72, 40 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 109, -2 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 146, 40 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 220, 40 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 294, 40 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 368, 82 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 368, -2 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 442, 40 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 516, 40 );
    }
    else {
        DrawCampaignScenarioIcon( iconsId, 17, trackOffset, -2, 40 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 72, 40 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 146, -2 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 146, 82 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 220, 40 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 294, 40 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 331, -2 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 368, 40 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 442, -2 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 442, 82 );
        DrawCampaignScenarioIcon( iconsId, 12, trackOffset, 516, 40 );
    }

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
            conf.SetCurrentFileInfo( campaignMaps[chosenScenarioID] );
            Players & players = conf.GetPlayers();
            players.SetStartGame();
            if ( conf.ExtGameUseFade() )
                fheroes2::FadeDisplay();
            Game::ShowMapLoadingText();
            conf.SetGameType( Game::TYPE_CAMPAIGN );

            if ( !world.LoadMapMP2( campaignMaps[chosenScenarioID].file ) ) {
                Dialog::Message( "Campaign Game loading failure", "Please make sure that campaign files are correct and present", Font::SMALL, Dialog::OK );
                conf.SetCurrentFileInfo( Maps::FileInfo() );
                continue;
            }

            campaignData.setCurrentScenarioBonus( scenarioBonus );
            campaignData.setCurrentScenarioID( chosenScenarioID );

            SetScenarioBonus( scenarioBonus );

            return Game::STARTGAME;
        }
    }

    return Game::NEWGAME;
}
