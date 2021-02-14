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
#include "race.h"
#include "settings.h"
#include "text.h"
#include "world.h"

namespace
{
    // TODO: Implement bonus data for each scenario
    std::vector<Campaign::ScenarioBonusData> getRolandCampaignBonusData( const int scenarioID )
    {
        std::vector<Campaign::ScenarioBonusData> bonus;

        switch ( scenarioID ) {
        case 0:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::THUNDER_MACE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        case 1:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WZRD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::SORC, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::KNGT, 1 );
            break;
        case 2:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WZRD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::SORC, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::KNGT, 1 );
            break;
        case 3:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WZRD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::SORC, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::KNGT, 1 );
            break;
        case 4:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WZRD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::SORC, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::KNGT, 1 );
            break;
        case 5:
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::MIRRORIMAGE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::SUMMONEELEMENT, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::RESURRECT, 1 );
            break;
        case 6:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::BLACK_PEARL, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::DRAGON_SWORD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::DIVINE_BREASTPLATE, 1 );
            break;
        case 7:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WZRD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::SORC, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::KNGT, 1 );
            break;
        case 8:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::CRYSTAL, 20 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GEMS, 20 );
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::MERCURY, 20 );
            break;
        case 9:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::TAX_LIEN, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::HIDEOUS_MASK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::FIZBIN_MISFORTUNE, 1 );
            break;
        }

        return bonus;
    }

    std::vector<Campaign::ScenarioBonusData> getArchibaldCampaignBonusData( const int scenarioID )
    {
        std::vector<Campaign::ScenarioBonusData> bonus;

        switch ( scenarioID ) {
        case 0:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MAGE_RING, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::MINOR_SCROLL, 1 );
            break;
        case 1:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 2:
            bonus.emplace_back( Campaign::ScenarioBonusData::RESOURCES, Resource::GOLD, 2000 );
            bonus.emplace_back( Campaign::ScenarioBonusData::SPELL, Spell::MASSCURSE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::DEFENDER_HELM, 1 );
            break;
        case 3:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 4:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 5:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 6:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 7:
            bonus.emplace_back( Campaign::ScenarioBonusData::SKILL, Skill::Secondary::LOGISTICS, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::POWER_AXE, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::WHITE_PEARL, 1 );
            break;
        case 8:
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::NECR, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::WRLK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::STARTING_RACE, Race::BARB, 1 );
            break;
        case 9:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::BLACK_PEARL, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::DRAGON_SWORD, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::DIVINE_BREASTPLATE, 1 );
            break;
        case 10:
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::TAX_LIEN, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::HIDEOUS_MASK, 1 );
            bonus.emplace_back( Campaign::ScenarioBonusData::ARTIFACT, Artifact::FIZBIN_MISFORTUNE, 1 );
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

    const std::string rolandCampaignDescription[10] = {
        _( "Roland needs you to defeat the lords near his castle to begin his war of rebellion against his brother.  They are not allied with each other, so they will spend"
           " most of their time fighting with one another.  Victory is yours when you have defeated all of their castles and heroes." ),
        _( "The local lords refuse to swear allegiance to Roland, and must be subdued. They are wealthy and powerful, so be prepared for a tough fight. Capture all enemy castles to win." ),
        _( "Your task is to defend the Dwarves against Archibald's forces. Capture all of the enemy towns and castles to win, and be sure not to lose all of the dwarf towns at once, or the enemy will have won." ),
        _( "You will face four allied enemies in a straightforward fight for resource and treasure. Capture all of the enemy castles for victory." ),
        _( "Your enemies are allied against you and start close by, so be ready to come out fighting. You will need to own all four castles in this small valley to win." ),
        _( "The Sorceress' guild of Noraston has requested Roland's aid against an attack from Archibald's allies. Capture all of the enemy castles to win, and don't lose Noraston, or you'll lose the scenario. (Hint: There is an enemy castle on an island in the ocean.)" ),
        _( "Find the Crown before Archibald's heroes find it. Roland will need the Crown for the final battle against Archibald." ),
        _( "Gather as large an army as possible and capture the enemy castle within 8 weeks. You are opposed by only one enemy, but must travel a long way to get to the enemy castle. Any troops you have in your army at the end of this scenario will be with you in the final battle." ),
        _( "Three allied enemies stand before you and victory, including Lord Corlagon. Roland is in a castle to the northwest, and you will lose if he falls to the enemy. Remember that capturing Lord Corlagon will ensure that he will not fight against you in the final scenario." ),
        _( "This is the final battle. Both you and your enemy are armed to the teeth, and all are allied against you. Capture Archibald to end the war!" )};

    const std::string archibaldCampaignDescription[11] = {
        _( "King Archibald requires you to defeat the three enemies in this region.  They are not allied with one another, so they will spend most of their energy fighting"
           " amongst themselves.  You will win when you own all of the enemy castles and there are no more heroes left to fight." ),
        _( "You must unify the barbarian tribes of the north by conquering them. As in the previous mission, the enemy is not allied against you, but they have more resources at their disposal. You will win when you own all of the enemy castles and there are no more heroes left to fight." ),
        _( "Do-gooder wizards have taken the Necromancers' castle. You must retake it to achieve victory. Remember that while you start with a powerful army, you have no castle and must take one within 7 days, or lose this battle. (Hint: The nearest castle is to the southeast.)" ),
        _( "The dwarves need conquering before they can interfere in King Archibald's plans. Roland's forces have more than one hero and many towns to start with, so be ready for attack from multiple directions. You must capture all of the enemy towns and castles to claim victory." ),
        _( "Your enemies are allied against you and start close by, so be ready to come out fighting. You will need to own all four castles in this small valley to win." ),
        _( "You must put down a peasant revolt led by Roland's forces. All are allied against you, but you have Lord Corlagon, an experienced hero, to help you. Capture all enemy castles to win." ),
        _( "There are two enemies allied against you in this mission. Both are well armed and seek to evict you from their island. Avoid them and capture Dragon City to win" ),
        _( "Your orders are to conquer the country lords that have sworn to serve Roland. All of the enemy castles are unified against you. Since you start without a castle, you must hurry to capture one before the end of the week. Capture all enemy castles for victory." ),
        _( "Find the Crown before Roland's heroes find it. Archibald will need the Crown for the final battle against Roland." ),
        _( "Gather as large an army as possible and capture the enemy castle within 8 weeks. You are opposed by only one enemy, but must travel a long way to get to the enemy castle. Any troops you have in your army at the end of this scenario will be with you in the final battle." ),
        _( "This is the final battle. Both you and your enemy are armed to the teeth, and all are allied against you. Capture Roland to win the war, and be sure not to lose Archibald in the fight!" )};

    void DrawCampaignScenarioIcon( const int icnId, const int iconIdx, const fheroes2::Point & offset, const int posX, const int posY )
    {
        fheroes2::Blit( fheroes2::AGG::GetICN( icnId, iconIdx ), fheroes2::Display::instance(), offset.x + posX, offset.y + posY );
    }

    void DrawCampaignScenarioIcons( fheroes2::ButtonGroup & buttonGroup, const Campaign::CampaignData campaignData, const fheroes2::Point & top )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const bool isGoodCampaign = campaignData.isGoodCampaign();
        const fheroes2::Point trackOffset( top.x + 39, top.y + 294 );

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

            if ( !isSubScenario && prevScenarioNextMaps.size() > 1 ) {
                isBranching = true;
                isFinalBranch = prevScenarioNextMaps.back() == scenarioID;

                y += isFinalBranch ? deltaY : -deltaY;
            }

            // available scenario (one of which should be selected)
            if ( std::find( availableMaps.begin(), availableMaps.end(), scenarioID ) != availableMaps.end() ) {
                buttonGroup.createButton( trackOffset.x + x, trackOffset.y + y, fheroes2::AGG::GetICN( iconsId, Campaign::SCENARIOICON_AVAILABLE ),
                                          fheroes2::AGG::GetICN( iconsId, selectedIconIdx ), i );
            }
            // cleared scenario
            else if ( std::find( clearedMaps.begin(), clearedMaps.end(), i ) != clearedMaps.end() ) {
                DrawCampaignScenarioIcon( iconsId, Campaign::SCENARIOICON_CLEARED, trackOffset, x, y );
            }
            else {
                DrawCampaignScenarioIcon( iconsId, Campaign::SCENARIOICON_UNAVAILABLE, trackOffset, x, y );
            }

            if ( !isBranching || isFinalBranch )
                prevScenarioNextMaps = nextMaps;

            if ( !isSubScenario && ( !isBranching || isFinalBranch ) )
                currentX += deltaX;
        }
    }

    void DrawCampaignScenarioDescription( const Campaign::ScenarioData & scenario, const fheroes2::Point & top )
    {
        const Maps::FileInfo & mapInfo = scenario.loadMap();
        const std::vector<Campaign::ScenarioBonusData> & bonuses = scenario.getBonuses();

        const std::string & mapNameStr = mapInfo.description;
        TextBox mapName( mapNameStr.substr( 1, mapNameStr.length() - 2 ), Font::BIG, 200 );
        mapName.Blit( top.x + 197, top.y + 97 - mapName.h() / 2 );

        Text campaignMapId( std::to_string( scenario.getScenarioID() + 1 ), Font::BIG );
        campaignMapId.Blit( top.x + 172 - campaignMapId.w() / 2, top.y + 97 - campaignMapId.h() / 2 );

        TextBox mapDescription( scenario.getDescription(), Font::BIG, 356 );
        mapDescription.Blit( top.x + 34, top.y + 132 );

        const int textChoiceWidth = 150;
        for ( size_t i = 0; i < bonuses.size(); ++i ) {
            Text choice( bonuses[i].ToString(), Font::BIG );
            choice.Blit( top.x + 425, top.y + 209 + 22 * i - choice.h() / 2, textChoiceWidth );
        }
    }

    Campaign::CampaignData GetRolandCampaignData()
    {
        std::vector<Campaign::ScenarioData> scenarioDatas;
        scenarioDatas.reserve( 10 );
        scenarioDatas.emplace_back( 0, std::vector<int>{1}, getCampaignBonusData( 0, 0 ), std::string( "CAMPG01.H2C" ), rolandCampaignDescription[0] );
        scenarioDatas.emplace_back( 1, std::vector<int>{2, 3}, getCampaignBonusData( 0, 1 ), std::string( "CAMPG02.H2C" ), rolandCampaignDescription[1] );
        scenarioDatas.emplace_back( 2, std::vector<int>{3}, getCampaignBonusData( 0, 2 ), std::string( "CAMPG03.H2C" ), rolandCampaignDescription[2] );
        scenarioDatas.emplace_back( 3, std::vector<int>{4}, getCampaignBonusData( 0, 3 ), std::string( "CAMPG04.H2C" ), rolandCampaignDescription[3] );
        scenarioDatas.emplace_back( 4, std::vector<int>{5}, getCampaignBonusData( 0, 4 ), std::string( "CAMPG05.H2C" ), rolandCampaignDescription[4] );
        scenarioDatas.emplace_back( 5, std::vector<int>{6, 7}, getCampaignBonusData( 0, 5 ), std::string( "CAMPG06.H2C" ), rolandCampaignDescription[5] );
        scenarioDatas.emplace_back( 6, std::vector<int>{8}, getCampaignBonusData( 0, 6 ), std::string( "CAMPG07.H2C" ), rolandCampaignDescription[6] );
        scenarioDatas.emplace_back( 7, std::vector<int>{8}, getCampaignBonusData( 0, 7 ), std::string( "CAMPG08.H2C" ), rolandCampaignDescription[7] );
        scenarioDatas.emplace_back( 8, std::vector<int>{9}, getCampaignBonusData( 0, 8 ), std::string( "CAMPG09.H2C" ), rolandCampaignDescription[8] );
        scenarioDatas.emplace_back( 9, std::vector<int>{}, getCampaignBonusData( 0, 9 ), std::string( "CAMPG10.H2C" ), rolandCampaignDescription[9] );

        Campaign::CampaignData campaignData;
        campaignData.setCampaignID( 0 );
        campaignData.setCampaignDescription( "Roland Campaign" );
        campaignData.setCampaignAlignment( true );
        campaignData.setCampaignScenarios( scenarioDatas );

        return campaignData;
    }

    Campaign::CampaignData GetArchibaldCampaignData()
    {
        std::vector<Campaign::ScenarioData> scenarioDatas;
        scenarioDatas.reserve( 11 );
        scenarioDatas.emplace_back( 0, std::vector<int>{1}, getCampaignBonusData( 1, 0 ), std::string( "CAMPE01.H2C" ), archibaldCampaignDescription[0] );
        scenarioDatas.emplace_back( 1, std::vector<int>{2, 3}, getCampaignBonusData( 1, 1 ), std::string( "CAMPE02.H2C" ), archibaldCampaignDescription[1] );
        scenarioDatas.emplace_back( 2, std::vector<int>{4}, getCampaignBonusData( 1, 2 ), std::string( "CAMPE03.H2C" ), archibaldCampaignDescription[2] );
        scenarioDatas.emplace_back( 3, std::vector<int>{4}, getCampaignBonusData( 1, 3 ), std::string( "CAMPE04.H2C" ), archibaldCampaignDescription[3] );
        scenarioDatas.emplace_back( 4, std::vector<int>{5}, getCampaignBonusData( 1, 4 ), std::string( "CAMPE05.H2C" ), archibaldCampaignDescription[4] );
        scenarioDatas.emplace_back( 5, std::vector<int>{6, 7}, getCampaignBonusData( 1, 5 ), std::string( "CAMPE06.H2C" ), archibaldCampaignDescription[5] );
        scenarioDatas.emplace_back( 6, std::vector<int>{7}, getCampaignBonusData( 1, 6 ), std::string( "CAMPE07.H2C" ), archibaldCampaignDescription[6] );
        scenarioDatas.emplace_back( 7, std::vector<int>{8, 9}, getCampaignBonusData( 1, 7 ), std::string( "CAMPE08.H2C" ), archibaldCampaignDescription[7] );
        scenarioDatas.emplace_back( 8, std::vector<int>{10}, getCampaignBonusData( 1, 8 ), std::string( "CAMPE09.H2C" ), archibaldCampaignDescription[8] );
        scenarioDatas.emplace_back( 9, std::vector<int>{10}, getCampaignBonusData( 1, 9 ), std::string( "CAMPE10.H2C" ), archibaldCampaignDescription[9] );
        scenarioDatas.emplace_back( 10, std::vector<int>{}, getCampaignBonusData( 1, 10 ), std::string( "CAMPE11.H2C" ), archibaldCampaignDescription[10] );

        Campaign::CampaignData campaignData;
        campaignData.setCampaignID( 1 );
        campaignData.setCampaignDescription( "Archibald Campaign" );
        campaignData.setCampaignAlignment( false );
        campaignData.setCampaignScenarios( scenarioDatas );

        return campaignData;
    }

    Campaign::CampaignData GetCampaignData( int campaignID )
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
            case Campaign::ScenarioBonusData::SPELL:
                kingdom.GetBestHero()->AppendSpellToBook( scenarioBonus._subType, true );
                break;
            case Campaign::ScenarioBonusData::STARTING_RACE:
                Players::SetPlayerRace( player.GetColor(), scenarioBonus._subType );
                break;
            case Campaign::ScenarioBonusData::SKILL:
                kingdom.GetBestHero()->LearnSkill( Skill::Secondary( scenarioBonus._subType, scenarioBonus._amount ) );
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
    if ( campaignData.isLastScenario( lastCompletedScenarioID ) )
        return Game::HIGHSCORES;

    const int firstNextMap = campaignData.getScenariosAfter( lastCompletedScenarioID ).front();
    saveData.setCurrentScenarioID( firstNextMap );
    return Game::SELECT_CAMPAIGN_SCENARIO;
}

int Game::SelectCampaignScenario()
{
    fheroes2::Display & display = fheroes2::Display::instance();
    display.fill( 0 );
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

    const int buttonIconID = goodCampaign ? ICN::CAMPXTRG : ICN::CAMPXTRE;
    fheroes2::Button buttonViewIntro( top.x + 22, top.y + 431, buttonIconID, 0, 1 );
    fheroes2::Button buttonOk( top.x + 367, top.y + 431, buttonIconID, 4, 5 );
    fheroes2::Button buttonCancel( top.x + 511, top.y + 431, buttonIconID, 6, 7 );

    const int chosenScenarioID = campaignSaveData.getCurrentScenarioID();
    const std::vector<Campaign::ScenarioData> & scenarios = campaignData.getAllScenarios();
    const Campaign::ScenarioData & scenario = scenarios[chosenScenarioID];

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

    buttonViewIntro.disable();
    buttonViewIntro.draw();

    buttonOk.draw();
    buttonCancel.draw();

    for ( uint32_t i = 0; i < bonusChoiceCount; ++i )
        buttonChoices.button( i ).draw();

    Text textDaysSpent( "0", Font::BIG );
    textDaysSpent.Blit( top.x + 574 + textDaysSpent.w() / 2, top.y + 31 );

    DrawCampaignScenarioDescription( scenario, top );

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

        for ( uint32_t i = 0; i < selectableScenariosCount; ++i ) {
            if ( le.MousePressLeft( selectableScenarioButtons.button( i ).area() ) ) {
                campaignSaveData.setCurrentScenarioID( selectableScenarios[i] );
                return Game::SELECT_CAMPAIGN_SCENARIO;
            }
        }

        if ( le.MouseClickLeft( buttonCancel.area() ) )
            return Game::NEWGAME;
        else if ( !buttonOk.isDisabled() && le.MouseClickLeft( buttonOk.area() ) ) {
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
                Dialog::Message( "Campaign Game loading failure", "Please make sure that campaign files are correct and present", Font::SMALL, Dialog::OK );
                conf.SetCurrentFileInfo( Maps::FileInfo() );
                continue;
            }

            // meanwhile, the others should be called after players.SetStartGame()
            if ( scenarioBonus._type != Campaign::ScenarioBonusData::STARTING_RACE )
                SetScenarioBonus( scenarioBonus );

            campaignSaveData.setCurrentScenarioBonus( scenarioBonus );
            campaignSaveData.setCurrentScenarioID( chosenScenarioID );

            return Game::STARTGAME;
        }
    }

    return Game::NEWGAME;
}
