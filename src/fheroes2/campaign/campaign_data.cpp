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

#include "campaign_data.h"
#include "artifact.h"
#include "heroes.h"
#include "monster.h"
#include "resource.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"
#include <cassert>

namespace
{
    const std::vector<Campaign::ScenarioIntroVideoInfo> emptyPlayback;

    std::vector<Campaign::CampaignAwardData> getRolandCampaignAwardData( const int scenarioID )
    {
        std::vector<Campaign::CampaignAwardData> obtainableAwards;

        switch ( scenarioID ) {
        case 2:
            obtainableAwards.emplace_back( 0, Campaign::CampaignAwardData::TYPE_CREATURE_ALLIANCE, Monster::DWARF );
            break;
        case 5:
            obtainableAwards.emplace_back( 1, Campaign::CampaignAwardData::TYPE_HIREABLE_HERO, Heroes::ELIZA, 0, 0, _( "Sorceress Guild" ) );
            break;
        case 6:
            obtainableAwards.emplace_back( 2, Campaign::CampaignAwardData::TYPE_CARRY_OVER_FORCES, 0, 0, 9 );
            break;
        case 7:
            obtainableAwards.emplace_back( 3, Campaign::CampaignAwardData::TYPE_GET_ARTIFACT, Artifact::ULTIMATE_CROWN, 1, 9 );
            break;
        case 8:
            obtainableAwards.emplace_back( 4, Campaign::CampaignAwardData::TYPE_DEFEAT_ENEMY_HERO, Heroes::CORLAGON, 0, 9 );
            break;
        }

        return obtainableAwards;
    }

    std::vector<Campaign::CampaignAwardData> getArchibaldCampaignAwardData( const int scenarioID )
    {
        std::vector<Campaign::CampaignAwardData> obtainableAwards;

        switch ( scenarioID ) {
        case 2:
            obtainableAwards.emplace_back( 1, Campaign::CampaignAwardData::TYPE_HIREABLE_HERO, Heroes::BAX, 0, 0, _( "Necromancer Guild" ) );
            break;
        case 3:
            obtainableAwards.emplace_back( 2, Campaign::CampaignAwardData::TYPE_CREATURE_ALLIANCE, Monster::OGRE );
            obtainableAwards.emplace_back( 3, Campaign::CampaignAwardData::TYPE_CREATURE_CURSE, Monster::DWARF );
            break;
        case 6:
            obtainableAwards.emplace_back( 4, Campaign::CampaignAwardData::TYPE_CREATURE_ALLIANCE, Monster::GREEN_DRAGON, _( "Dragon Alliance" ) );
            break;
        case 8:
            obtainableAwards.emplace_back( 5, Campaign::CampaignAwardData::TYPE_GET_ARTIFACT, Artifact::ULTIMATE_CROWN );
            break;
        case 9:
            obtainableAwards.emplace_back( 6, Campaign::CampaignAwardData::TYPE_CARRY_OVER_FORCES, 0 );
            break;
        }

        return obtainableAwards;
    }

    std::vector<Campaign::CampaignAwardData> getPriceOfLoyaltyCampaignAwardData( const int scenarioID )
    {
        std::vector<Campaign::CampaignAwardData> obtainableAwards;

        switch ( scenarioID ) {
        case 1:
            obtainableAwards.emplace_back( 0, Campaign::CampaignAwardData::TYPE_GET_ARTIFACT, Artifact::BREASTPLATE_ANDURAN );
            break;
        case 2:
            obtainableAwards.emplace_back( 1, Campaign::CampaignAwardData::TYPE_CREATURE_ALLIANCE, Monster::ELF, _( "Elven Alliance" ) );
            obtainableAwards.emplace_back( 2, Campaign::CampaignAwardData::TYPE_RESOURCE_BONUS, Resource::WOOD, 2 );
            break;
        case 5:
            obtainableAwards.emplace_back( 3, Campaign::CampaignAwardData::TYPE_GET_ARTIFACT, Artifact::HELMET_ANDURAN );
            break;
        case 6:
            // will assemble Battle Garb of Anduran along with the previous anduran set pieces
            obtainableAwards.emplace_back( 4, Campaign::CampaignAwardData::TYPE_GET_ARTIFACT, Artifact::SWORD_ANDURAN );
            obtainableAwards.emplace_back( 5, Campaign::CampaignAwardData::TYPE_DEFEAT_ENEMY_HERO, Heroes::DAINWIN );
            break;
        }

        return obtainableAwards;
    }

    std::vector<Campaign::CampaignAwardData> getWizardsIsleCampaignAwardData( const int scenarioID )
    {
        std::vector<Campaign::CampaignAwardData> obtainableAwards;

        switch ( scenarioID ) {
        case 1:
            obtainableAwards.emplace_back( 0, Campaign::CampaignAwardData::TYPE_GET_SPELL, Spell::SETEGUARDIAN );
            break;
        case 2:
            obtainableAwards.emplace_back( 1, Campaign::CampaignAwardData::TYPE_GET_ARTIFACT, Artifact::SPHERE_NEGATION );
            break;
        }

        return obtainableAwards;
    }

    std::vector<Campaign::CampaignAwardData> getDescendantsCampaignAwardData( const int scenarioID )
    {
        std::vector<Campaign::CampaignAwardData> obtainableAwards;

        switch ( scenarioID ) {
        case 2:
            obtainableAwards.emplace_back( 0, Campaign::CampaignAwardData::TYPE_HIREABLE_HERO, Heroes::JOSEPH, 0, 0, _( "Wayward Son" ) );
            break;
        case 3:
            obtainableAwards.emplace_back( 1, Campaign::CampaignAwardData::TYPE_HIREABLE_HERO, Heroes::UNCLEIVAN, 0, 0, _( "Uncle Ivan" ) );
            break;
        case 5:
            obtainableAwards.emplace_back( 2, Campaign::CampaignAwardData::TYPE_GET_ARTIFACT, Artifact::LEGENDARY_SCEPTER );
            break;
        case 6:
            obtainableAwards.emplace_back( 3, Campaign::CampaignAwardData::TYPE_CREATURE_ALLIANCE, Monster::ELF, _( "Elven Alliance" ) );
            break;
        }

        return obtainableAwards;
    }

    const std::string rolandCampaignScenarioNames[10]
        = { _( "Force of Arms" ), _( "Annexation" ),   _( "Save the Dwarves" ), _( "Carator Mines" ),      _( "Turning Point" ),
            _( "Defender" ),      _( "The Gauntlet" ), _( "The Crown" ),        _( "Corlagon's Defense" ), _( "Final Justice" ) };

    const std::string archibaldCampaignScenarioNames[11]
        = { _( "First Blood" ),   _( "Barbarian Wars" ), _( "Necromancers" ), _( "Slay the Dwarves" ), _( "Turning Point" ), _( "Rebellion" ),
            _( "Dragon Master" ), _( "Country Lords" ),  _( "The Crown" ),    _( "Greater Glory" ),    _( "Apocalypse" ) };

    const std::string priceOfLoyaltyCampaignScenarioNames[8] = { _( "Uprising" ),         _( "Island of Chaos" ), _( "Arrow's Flight" ), _( "The Abyss" ),
                                                                 _( "The Giant's Pass" ), _( "Aurora Borealis" ), _( "Betrayal's End" ), _( "Corruption's Heart" ) };

    const std::string descendantsCampaignScenarioNames[8] = { _( "Conquer and Unify" ), _( "Border Towns" ), _( "The Wayward Son" ), _( "Crazy Uncle Ivan" ),
                                                              _( "The Southern War" ),  _( "Ivory Gates" ),  _( "The Elven Lands" ), _( "The Epic Battle" ) };

    const std::string wizardsIsleCampaignScenarioNames[4] = { _( "The Shrouded Isles" ), _( "The Eternal Scrolls" ), _( "Power's End" ), _( "Fount of Wizardry" ) };

    const std::string voyageHomeCampaignScenarioNames[4] = { _( "Stranded" ), _( "Pirate Isles" ), _( "King and Country" ), _( "Blood is Thicker" ) };

    const std::string rolandCampaignDescription[10] = {
        _( "Roland needs you to defeat the lords near his castle to begin his war of rebellion against his brother.  They are not allied with each other, so they will spend"
           " most of their time fighting with one another.  Victory is yours when you have defeated all of their castles and heroes." ),
        _( "The local lords refuse to swear allegiance to Roland, and must be subdued. They are wealthy and powerful, so be prepared for a tough fight. Capture all enemy castles to win." ),
        _( "Your task is to defend the Dwarves against Archibald's forces. Capture all of the enemy towns and castles to win, and be sure not to lose all of the dwarf towns at once, or the enemy will have won." ),
        _( "You will face four allied enemies in a straightforward fight for resource and treasure. Capture all of the enemy castles for victory." ),
        _( "Your enemies are allied against you and start close by, so be ready to come out fighting. You will need to own all four castles in this small valley to win." ),
        _( "The Sorceress' guild of Noraston has requested Roland's aid against an attack from Archibald's allies. Capture all of the enemy castles to win, and don't lose Noraston, or you'll lose the scenario. (Hint: There is an enemy castle on an island in the ocean.)" ),
        _( "Gather as large an army as possible and capture the enemy castle within 8 weeks. You are opposed by only one enemy, but must travel a long way to get to the enemy castle. Any troops you have in your army at the end of this scenario will be with you in the final battle." ),
        _( "Find the Crown before Archibald's heroes find it. Roland will need the Crown for the final battle against Archibald." ),
        _( "Three allied enemies stand before you and victory, including Lord Corlagon. Roland is in a castle to the northwest, and you will lose if he falls to the enemy. Remember that capturing Lord Corlagon will ensure that he will not fight against you in the final scenario." ),
        _( "This is the final battle. Both you and your enemy are armed to the teeth, and all are allied against you. Capture Archibald to end the war!" ) };

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
        _( "This is the final battle. Both you and your enemy are armed to the teeth, and all are allied against you. Capture Roland to win the war, and be sure not to lose Archibald in the fight!" ) };

    const std::string priceOfLoyaltyCampaignDescription[8] = {
        _( "Subdue the unruly local lords in order to provide the Empire with facilities to operate in this region." ),
        _( "Eliminate all oposition in this area. Then the first piece of the artifact will be yours." ),
        _( "The sorceresses to the northeast are rebelling! For the good of the empire you must quash their feeble uprising on your way to the mountains." ),
        _( "Having prepared for your arrival, Kraeger has arranged for a force of necromancers to thwart your quest. You must capture the castle of Scabsdale before the first day of the third week, or the Necromancers will be too strong for you." ),
        _( "The barbarian despot in this area is, as yet, ignorant of your presence. Quickly, build up your forces before you are discovered and attacked! Secure the region by subduing all enemy forces." ),
        _( "The Empire is weak in this region. You will be unable to completely subdue all forces in this area, so take what you can before reprisal strikes. Remember, your true goal is to claim the Helmet of Anduran." ),
        _( "For the good of the Empire, eliminate Kraeger." ),
        _( "At last, you have the opportunity and the facilities to rid the Empire of the necromancer's evil. Eradicate them completely, and you will be sung as a hero for all time." ) };

    const std::string descendantsCampaignDescription[8] = {
        _( "Conquer and unite all the enemy tribes. Don't lose the hero Jarkonas, the forefather of all descendants." ),
        _( "Your rival, the Kingdom of Harondale, is attacking weak towns on your border! Recover from their first strike and crush them completely!" ),
        _( "Find your wayward son Joseph who is rumored to be living in the desolate lands. Do it before the first day of the third month or it will be of no help to your family." ),
        _( "Rescue your crazy uncle Ivan. Find him before the first day of the fourth month or it will be no help to your kingdom." ),
        _( "Destroy the barbarians who are attacking the southern border of your kingdom! Recover your fallen towns, and then invade the jungle kingdom. Leave no enemy standing." ),
        _( "Retake the castle of Ivory Gates, which has fallen due to treachery." ),
        _( "Gain the favor of the elves. They will not allow trees to be chopped down, so they will send you wood every 2 weeks. You must complete your mission before the first day of the seventh month, or the kingdom will surely fall." ),
        _( "This is the final battle against your rival kingdom of Harondale. Eliminate everyone, and don't lose the hero Jarkonas VI." ) };

    const std::string wizardsIsleCampaignDescription[4] = {
        _( "Your mission is to vanquish the warring mages in the magical Shrouded Isles. The completion of this task will give you a fighting chance against your rivals." ),
        _( "The location of the great library has been dicovered! You must make your way to it, and reclaim the city of Chronos in which it lies." ),
        _( "Find the Orb of negation, which is said to be buried in this land. There are clues inscribed on stone obelisks which will help lead you to your price. Find the orb before the first day of the sixth month, or your rivals will surely have gotten to the fount before you." ),
        _( "You must take control of the castle of Magic, where the fount of wizardry lies. Do this and your victory will be supreme." ) };

    const std::string voyageHomeCampaignDescription[4] = {
        _( "Capture the town on the island off the southeast shore in order to construct a boat and travel back towards the mainland. Do not lose the hero Gallavant." ),
        _( "Find and defeat Martine, the pirate leader, who resides in Pirates Cove. Do not lose Gallavant or your quest will be over." ),
        _( "Eliminate all the other forces who oppose the rule of Lord Alberon. Gallavant must not die." ),
        _( "Overthrow the entrenched monarchy of Lord Alberon, and claim all the land in your name. Gallavant must not die." ) };

    Campaign::CampaignData getRolandCampaignData()
    {
        std::vector<Campaign::ScenarioData> scenarioDatas;
        scenarioDatas.reserve( 10 );

        scenarioDatas.emplace_back( 0, std::vector<int>{ 1 }, Campaign::ScenarioBonusData::getCampaignBonusData( 0, 0 ), "CAMPG01.H2C", rolandCampaignScenarioNames[0],
                                    rolandCampaignDescription[0],
                                    Campaign::VideoSequence{ { "GOOD01V.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "GOOD01.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    emptyPlayback );
        scenarioDatas.emplace_back( 1, std::vector<int>{ 2, 3 }, Campaign::ScenarioBonusData::getCampaignBonusData( 0, 1 ), "CAMPG02.H2C", rolandCampaignScenarioNames[1],
                                    rolandCampaignDescription[1],
                                    Campaign::VideoSequence{ { "GOOD02W.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "GOOD02.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "GOOD03QW.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "GOOD03.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 2, std::vector<int>{ 3 }, Campaign::ScenarioBonusData::getCampaignBonusData( 0, 2 ), "CAMPG03.H2C", rolandCampaignScenarioNames[2],
                                    rolandCampaignDescription[2], emptyPlayback,
                                    Campaign::VideoSequence{ { "GOOD04W.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "GOOD04.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::ScenarioVictoryCondition::STANDARD, Campaign::ScenarioLossCondition::LOSE_ALL_SORCERESS_VILLAGES );
        scenarioDatas.emplace_back( 3, std::vector<int>{ 4 }, Campaign::ScenarioBonusData::getCampaignBonusData( 0, 3 ), "CAMPG04.H2C", rolandCampaignScenarioNames[3],
                                    rolandCampaignDescription[3], emptyPlayback,
                                    Campaign::VideoSequence{ { "GOOD05V.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "GOOD05.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 4, std::vector<int>{ 5 }, Campaign::ScenarioBonusData::getCampaignBonusData( 0, 4 ), "CAMPG05.H2C", rolandCampaignScenarioNames[4],
                                    rolandCampaignDescription[4], emptyPlayback, emptyPlayback );
        scenarioDatas.emplace_back( 5, std::vector<int>{ 6, 7 }, Campaign::ScenarioBonusData::getCampaignBonusData( 0, 5 ), "CAMPG06.H2C", rolandCampaignScenarioNames[5],
                                    rolandCampaignDescription[5],
                                    Campaign::VideoSequence{ { "GOOD06AV.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "GOOD06.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "GOOD07QW.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "GOOD07.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        // NOTE: In Roland's Campaign, scenario 8 is drawn above scenario 7, so we emplace_back scenario 8 first
        scenarioDatas.emplace_back( 7, std::vector<int>{ 8 }, Campaign::ScenarioBonusData::getCampaignBonusData( 0, 7 ), "CAMPG08.H2C", rolandCampaignScenarioNames[7],
                                    rolandCampaignDescription[7], emptyPlayback, emptyPlayback, Campaign::ScenarioVictoryCondition::OBTAIN_ULTIMATE_CROWN );
        scenarioDatas.emplace_back( 6, std::vector<int>{ 8 }, Campaign::ScenarioBonusData::getCampaignBonusData( 0, 6 ), "CAMPG07.H2C", rolandCampaignScenarioNames[6],
                                    rolandCampaignDescription[6], emptyPlayback, emptyPlayback );
        scenarioDatas.emplace_back( 8, std::vector<int>{ 9 }, Campaign::ScenarioBonusData::getCampaignBonusData( 0, 8 ), "CAMPG09.H2C", rolandCampaignScenarioNames[8],
                                    rolandCampaignDescription[8],
                                    Campaign::VideoSequence{ { "GOOD09W.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "GOOD09.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    emptyPlayback );
        scenarioDatas.emplace_back( 9, std::vector<int>{}, Campaign::ScenarioBonusData::getCampaignBonusData( 0, 9 ), "CAMPG10.H2C", rolandCampaignScenarioNames[9],
                                    rolandCampaignDescription[9],
                                    Campaign::VideoSequence{ { "GOOD10W.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "GOOD10.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "LIBRARYW.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "LIBRARY.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );

        Campaign::CampaignData campaignData;
        campaignData.setCampaignID( Campaign::ROLAND_CAMPAIGN );
        campaignData.setCampaignDescription( "Roland Campaign" );
        campaignData.setCampaignScenarios( scenarioDatas );

        return campaignData;
    }

    Campaign::CampaignData getArchibaldCampaignData()
    {
        std::vector<Campaign::ScenarioData> scenarioDatas;
        scenarioDatas.reserve( 11 );

        scenarioDatas.emplace_back( 0, std::vector<int>{ 1 }, Campaign::ScenarioBonusData::getCampaignBonusData( 1, 0 ), "CAMPE01.H2C", archibaldCampaignScenarioNames[0],
                                    archibaldCampaignDescription[0],
                                    Campaign::VideoSequence{ { "EVIL01V.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "EVIL01.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    emptyPlayback );
        scenarioDatas.emplace_back( 1, std::vector<int>{ 2, 3 }, Campaign::ScenarioBonusData::getCampaignBonusData( 1, 1 ), "CAMPE02.H2C",
                                    archibaldCampaignScenarioNames[1], archibaldCampaignDescription[1],
                                    Campaign::VideoSequence{ { "EVIL02W.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "EVIL02.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "EVIL03QW.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "EVIL03.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 2, std::vector<int>{ 4 }, Campaign::ScenarioBonusData::getCampaignBonusData( 1, 2 ), "CAMPE03.H2C", archibaldCampaignScenarioNames[2],
                                    archibaldCampaignDescription[2], emptyPlayback,
                                    Campaign::VideoSequence{ { "EVIL05AV.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "EVIL05.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 3, std::vector<int>{ 4 }, Campaign::ScenarioBonusData::getCampaignBonusData( 1, 3 ), "CAMPE04.H2C", archibaldCampaignScenarioNames[3],
                                    archibaldCampaignDescription[3], emptyPlayback,
                                    Campaign::VideoSequence{ { "EVIL05AV.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "EVIL05.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 4, std::vector<int>{ 5 }, Campaign::ScenarioBonusData::getCampaignBonusData( 1, 4 ), "CAMPE05.H2C", archibaldCampaignScenarioNames[4],
                                    archibaldCampaignDescription[4], emptyPlayback, emptyPlayback );
        scenarioDatas.emplace_back( 5, std::vector<int>{ 6, 7 }, Campaign::ScenarioBonusData::getCampaignBonusData( 1, 5 ), "CAMPE06.H2C",
                                    archibaldCampaignScenarioNames[5], archibaldCampaignDescription[5],
                                    Campaign::VideoSequence{ { "EVIL06AW.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "EVIL06.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "EVIL07W.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "EVIL07.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 6, std::vector<int>{ 7 }, Campaign::ScenarioBonusData::getCampaignBonusData( 1, 6 ), "CAMPE07.H2C", archibaldCampaignScenarioNames[6],
                                    archibaldCampaignDescription[6], emptyPlayback, Campaign::VideoSequence{ { "EVIL08.SMK", Video::VideoAction::PLAY_TILL_VIDEO_END } },
                                    Campaign::ScenarioVictoryCondition::CAPTURE_DRAGON_CITY );
        scenarioDatas.emplace_back( 7, std::vector<int>{ 8, 9 }, Campaign::ScenarioBonusData::getCampaignBonusData( 1, 7 ), "CAMPE08.H2C",
                                    archibaldCampaignScenarioNames[7], archibaldCampaignDescription[7], emptyPlayback,
                                    Campaign::VideoSequence{ { "EVIL09W.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "EVIL09.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 8, std::vector<int>{ 10 }, Campaign::ScenarioBonusData::getCampaignBonusData( 1, 8 ), "CAMPE09.H2C",
                                    archibaldCampaignScenarioNames[8], archibaldCampaignDescription[8], emptyPlayback, emptyPlayback,
                                    Campaign::ScenarioVictoryCondition::OBTAIN_ULTIMATE_CROWN );
        scenarioDatas.emplace_back( 9, std::vector<int>{ 10 }, Campaign::ScenarioBonusData::getCampaignBonusData( 1, 9 ), "CAMPE10.H2C",
                                    archibaldCampaignScenarioNames[9], archibaldCampaignDescription[9], emptyPlayback, emptyPlayback );
        scenarioDatas.emplace_back( 10, std::vector<int>{}, Campaign::ScenarioBonusData::getCampaignBonusData( 1, 10 ), "CAMPE11.H2C", archibaldCampaignScenarioNames[10],
                                    archibaldCampaignDescription[10],
                                    Campaign::VideoSequence{ { "EVIL11W.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "EVIL10.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "PRISON.SMK", Video::VideoAction::PLAY_TILL_VIDEO_END } } );

        Campaign::CampaignData campaignData;
        campaignData.setCampaignID( Campaign::ARCHIBALD_CAMPAIGN );
        campaignData.setCampaignDescription( "Archibald Campaign" );
        campaignData.setCampaignScenarios( scenarioDatas );

        return campaignData;
    }

    Campaign::CampaignData getPriceOfLoyaltyCampaignData()
    {
        std::vector<Campaign::ScenarioData> scenarioDatas;
        scenarioDatas.reserve( 8 );

        const int campaignID = Campaign::PRICE_OF_LOYALTY_CAMPAIGN;
        scenarioDatas.emplace_back( 0, std::vector<int>{ 1 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 0 ), "CAMP1_01.HXC",
                                    priceOfLoyaltyCampaignScenarioNames[0], priceOfLoyaltyCampaignDescription[0],
                                    Campaign::VideoSequence{ { "MIXPOL1.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "POL1.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    emptyPlayback );
        scenarioDatas.emplace_back( 1, std::vector<int>{ 2, 3 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 1 ), "CAMP1_02.HXC",
                                    priceOfLoyaltyCampaignScenarioNames[1], priceOfLoyaltyCampaignDescription[1],
                                    Campaign::VideoSequence{ { "MIXPOL2.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "POL2.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "MIXPOL3.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "POL3.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 2, std::vector<int>{ 4 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 2 ), "CAMP1_03.HXC",
                                    priceOfLoyaltyCampaignScenarioNames[2], priceOfLoyaltyCampaignDescription[2], emptyPlayback, emptyPlayback );
        scenarioDatas.emplace_back( 3, std::vector<int>{ 5 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 3 ), "CAMP1_04.HXC",
                                    priceOfLoyaltyCampaignScenarioNames[3], priceOfLoyaltyCampaignDescription[3], emptyPlayback, emptyPlayback );
        scenarioDatas.emplace_back( 4, std::vector<int>{ 5 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 4 ), "CAMP1_05.HXC",
                                    priceOfLoyaltyCampaignScenarioNames[4], priceOfLoyaltyCampaignDescription[4],
                                    Campaign::VideoSequence{ { "MIXPOL4.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "POL4.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    emptyPlayback );
        scenarioDatas.emplace_back( 5, std::vector<int>{ 6, 7 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 5 ), "CAMP1_06.HXC",
                                    priceOfLoyaltyCampaignScenarioNames[5], priceOfLoyaltyCampaignDescription[5],
                                    Campaign::VideoSequence{ { "MIXPOL5.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "POL5.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "MIXPOL6.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "POL6.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 6, std::vector<int>{ 7 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 6 ), "CAMP1_07.HXC",
                                    priceOfLoyaltyCampaignScenarioNames[6], priceOfLoyaltyCampaignDescription[6], emptyPlayback,
                                    Campaign::VideoSequence{ { "MIXPOL7.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "POL7.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 7, std::vector<int>{}, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 7 ), "CAMP1_08.HXC",
                                    priceOfLoyaltyCampaignScenarioNames[7], priceOfLoyaltyCampaignDescription[7], emptyPlayback,
                                    Campaign::VideoSequence{ { "MIXPOL8.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "POL8.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );

        Campaign::CampaignData campaignData;
        campaignData.setCampaignID( campaignID );
        campaignData.setCampaignScenarios( scenarioDatas );

        return campaignData;
    }

    Campaign::CampaignData getDescendantsCampaignData()
    {
        std::vector<Campaign::ScenarioData> scenarioDatas;
        scenarioDatas.reserve( 8 );

        const int campaignID = Campaign::DESCENDANTS_CAMPAIGN;
        scenarioDatas.emplace_back( 0, std::vector<int>{ 1 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 0 ), "CAMP2_01.HXC",
                                    descendantsCampaignScenarioNames[0], descendantsCampaignDescription[0],
                                    Campaign::VideoSequence{ { "MIXDES9.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "DES9.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    emptyPlayback );
        scenarioDatas.emplace_back( 1, std::vector<int>{ 2, 3 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 1 ), "CAMP2_02.HXC",
                                    descendantsCampaignScenarioNames[1], descendantsCampaignDescription[1],
                                    Campaign::VideoSequence{ { "MIXDES10.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "DES10.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "MIXDES11.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "DES11.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 2, std::vector<int>{ 4 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 2 ), "CAMP2_03.HXC",
                                    descendantsCampaignScenarioNames[2], descendantsCampaignDescription[2], emptyPlayback, emptyPlayback );
        scenarioDatas.emplace_back( 3, std::vector<int>{ 4 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 3 ), "CAMP2_04.HXC",
                                    descendantsCampaignScenarioNames[3], descendantsCampaignDescription[3], emptyPlayback, emptyPlayback );
        scenarioDatas.emplace_back( 4, std::vector<int>{ 5, 6 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 4 ), "CAMP2_05.HXC",
                                    descendantsCampaignScenarioNames[4], descendantsCampaignDescription[4],
                                    Campaign::VideoSequence{ { "MIXDES12.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "DES12.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    emptyPlayback );
        scenarioDatas.emplace_back( 5, std::vector<int>{ 7 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 5 ), "CAMP2_06.HXC",
                                    descendantsCampaignScenarioNames[5], descendantsCampaignDescription[5],
                                    Campaign::VideoSequence{ { "MIXDES13.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "DES13.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    emptyPlayback );
        scenarioDatas.emplace_back( 6, std::vector<int>{ 7 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 6 ), "CAMP2_07.HXC",
                                    descendantsCampaignScenarioNames[6], descendantsCampaignDescription[6], emptyPlayback, emptyPlayback );
        scenarioDatas.emplace_back( 7, std::vector<int>{}, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 7 ), "CAMP2_08.HXC",
                                    descendantsCampaignScenarioNames[7], descendantsCampaignDescription[7],
                                    Campaign::VideoSequence{ { "MIXDES14.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "DES14.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "MIXDES15.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "DES15.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );

        Campaign::CampaignData campaignData;
        campaignData.setCampaignID( campaignID );
        campaignData.setCampaignScenarios( scenarioDatas );

        return campaignData;
    }

    Campaign::CampaignData getWizardsIsleCampaignData()
    {
        std::vector<Campaign::ScenarioData> scenarioDatas;
        scenarioDatas.reserve( 4 );

        const int campaignID = Campaign::WIZARDS_ISLE_CAMPAIGN;
        scenarioDatas.emplace_back( 0, std::vector<int>{ 1 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 0 ), "CAMP3_01.HXC",
                                    wizardsIsleCampaignScenarioNames[0], wizardsIsleCampaignDescription[0],
                                    Campaign::VideoSequence{ { "MIXWIZ16.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "WIZ16.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    emptyPlayback );
        scenarioDatas.emplace_back( 1, std::vector<int>{ 2, 3 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 1 ), "CAMP3_02.HXC",
                                    wizardsIsleCampaignScenarioNames[1], wizardsIsleCampaignDescription[1],
                                    Campaign::VideoSequence{ { "MIXWIZ17.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "WIZ17.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "MIXWIZ18.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "WIZ18.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 2, std::vector<int>{}, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 2 ), "CAMP3_03.HXC",
                                    wizardsIsleCampaignScenarioNames[2], wizardsIsleCampaignDescription[2], emptyPlayback,
                                    Campaign::VideoSequence{ { "MIXWIZ19.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "WIZ19.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 3, std::vector<int>{}, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 3 ), "CAMP3_04.HXC",
                                    wizardsIsleCampaignScenarioNames[3], wizardsIsleCampaignDescription[3], emptyPlayback,
                                    Campaign::VideoSequence{ { "MIXWIZ20.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "WIZ20.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );

        Campaign::CampaignData campaignData;
        campaignData.setCampaignID( campaignID );
        campaignData.setCampaignScenarios( scenarioDatas );

        return campaignData;
    }

    Campaign::CampaignData getVoyageHomeCampaignData()
    {
        std::vector<Campaign::ScenarioData> scenarioDatas;
        scenarioDatas.reserve( 4 );

        const int campaignID = Campaign::VOYAGE_HOME_CAMPAIGN;
        scenarioDatas.emplace_back( 0, std::vector<int>{ 1 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 0 ), "CAMP4_01.HXC",
                                    voyageHomeCampaignScenarioNames[0], voyageHomeCampaignDescription[0],
                                    Campaign::VideoSequence{ { "MIXVOY21.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "VOY21.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    emptyPlayback );
        scenarioDatas.emplace_back( 1, std::vector<int>{ 2, 3 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 1 ), "CAMP4_02.HXC",
                                    voyageHomeCampaignScenarioNames[1], voyageHomeCampaignDescription[1],
                                    Campaign::VideoSequence{ { "MIXVOY22.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "VOY22.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } },
                                    Campaign::VideoSequence{ { "MIXVOY23.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "VOY23.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 2, std::vector<int>{ 3 }, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 2 ), "CAMP4_03.HXC",
                                    voyageHomeCampaignScenarioNames[2], voyageHomeCampaignDescription[2], emptyPlayback,
                                    Campaign::VideoSequence{ { "MIXVOY24.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "VOY24.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );
        scenarioDatas.emplace_back( 3, std::vector<int>{}, Campaign::ScenarioBonusData::getCampaignBonusData( campaignID, 3 ), "CAMP4_04.HXC",
                                    voyageHomeCampaignScenarioNames[3], voyageHomeCampaignDescription[3], emptyPlayback,
                                    Campaign::VideoSequence{ { "MIXVOY25.SMK", Video::VideoAction::IGNORE_VIDEO },
                                                             { "VOY25.SMK", Video::VideoAction::PLAY_TILL_AUDIO_END } } );

        Campaign::CampaignData campaignData;
        campaignData.setCampaignID( campaignID );
        campaignData.setCampaignScenarios( scenarioDatas );

        return campaignData;
    }
}

namespace Campaign
{
    CampaignData::CampaignData()
        : _campaignID( 0 )
        , _campaignDescription()
        , _scenarios()
    {}

    std::vector<Campaign::CampaignAwardData> CampaignAwardData::getCampaignAwardData( const int campaignID, const int scenarioID )
    {
        assert( campaignID >= 0 && scenarioID >= 0 );

        switch ( campaignID ) {
        case ROLAND_CAMPAIGN:
            return getRolandCampaignAwardData( scenarioID );
        case ARCHIBALD_CAMPAIGN:
            return getArchibaldCampaignAwardData( scenarioID );
        case PRICE_OF_LOYALTY_CAMPAIGN:
            return getPriceOfLoyaltyCampaignAwardData( scenarioID );
        case DESCENDANTS_CAMPAIGN:
            return getDescendantsCampaignAwardData( scenarioID );
        case WIZARDS_ISLE_CAMPAIGN:
            return getWizardsIsleCampaignAwardData( scenarioID );
            // no campaign award for voyage home!
        case VOYAGE_HOME_CAMPAIGN:
            break;
        }

        return std::vector<Campaign::CampaignAwardData>();
    }

    std::vector<int> CampaignData::getScenariosBefore( const int scenarioID ) const
    {
        std::vector<int> scenarioIDs;

        for ( size_t i = 0; i < _scenarios.size(); ++i ) {
            if ( _scenarios[i].getScenarioID() >= scenarioID )
                break;

            const std::vector<int> & nextMaps = _scenarios[i].getNextMaps();

            // if any of this scenario's next maps is the one passed as param, then this scenario is a previous scenario
            if ( std::find( nextMaps.begin(), nextMaps.end(), scenarioID ) != nextMaps.end() )
                scenarioIDs.emplace_back( _scenarios[i].getScenarioID() );
        }

        return scenarioIDs;
    }

    const std::vector<int> & CampaignData::getScenariosAfter( const int scenarioID ) const
    {
        for ( size_t i = 0; i < _scenarios.size(); ++i ) {
            if ( _scenarios[i].getScenarioID() == scenarioID )
                return _scenarios[i].getNextMaps();
        }

        return _scenarios[scenarioID].getNextMaps();
    }

    std::vector<int> CampaignData::getStartingScenarios() const
    {
        std::vector<int> startingScenarios;

        for ( size_t i = 0; i < _scenarios.size(); ++i ) {
            const int scenarioID = _scenarios[i].getScenarioID();
            if ( isStartingScenario( scenarioID ) )
                startingScenarios.emplace_back( scenarioID );
        }

        return startingScenarios;
    }

    bool CampaignData::isStartingScenario( const int scenarioID ) const
    {
        // starting scenario = a scenario that is never included as a nextMap
        for ( size_t i = 0; i < _scenarios.size(); ++i ) {
            const std::vector<int> & nextMaps = _scenarios[i].getNextMaps();

            if ( std::find( nextMaps.begin(), nextMaps.end(), scenarioID ) != nextMaps.end() )
                return false;
        }

        return true;
    }

    bool CampaignData::isAllCampaignMapsPresent() const
    {
        for ( size_t i = 0; i < _scenarios.size(); ++i ) {
            if ( !_scenarios[i].isMapFilePresent() )
                return false;
        }

        return true;
    }

    bool CampaignData::isLastScenario( const int scenarioID ) const
    {
        assert( !_scenarios.empty() );
        return scenarioID == _scenarios.back().getScenarioID();
    }

    void CampaignData::setCampaignID( const int campaignID )
    {
        _campaignID = campaignID;
    }

    void CampaignData::setCampaignScenarios( const std::vector<ScenarioData> & scenarios )
    {
        _scenarios = scenarios;
    }

    void CampaignData::setCampaignDescription( const std::string & campaignDescription )
    {
        _campaignDescription = campaignDescription;
    }

    const CampaignData & CampaignData::getCampaignData( const int campaignID )
    {
        switch ( campaignID ) {
        case ROLAND_CAMPAIGN: {
            static const Campaign::CampaignData campaign( getRolandCampaignData() );
            return campaign;
        }
        case ARCHIBALD_CAMPAIGN: {
            static const Campaign::CampaignData campaign( getArchibaldCampaignData() );
            return campaign;
        }
        case PRICE_OF_LOYALTY_CAMPAIGN: {
            static const Campaign::CampaignData campaign( getPriceOfLoyaltyCampaignData() );
            return campaign;
        }
        case DESCENDANTS_CAMPAIGN: {
            static const Campaign::CampaignData campaign( getDescendantsCampaignData() );
            return campaign;
        }
        case VOYAGE_HOME_CAMPAIGN: {
            static const Campaign::CampaignData campaign( getVoyageHomeCampaignData() );
            return campaign;
        }
        case WIZARDS_ISLE_CAMPAIGN: {
            static const Campaign::CampaignData campaign( getWizardsIsleCampaignData() );
            return campaign;
        }
        default: {
            static const Campaign::CampaignData noCampaign;
            return noCampaign;
        }
        }
    }

    CampaignAwardData::CampaignAwardData()
        : _id( 0 )
        , _type( 0 )
        , _subType( 0 )
        , _amount( 0 )
        , _startScenarioID( 0 )
        , _customName()
    {}

    // default amount to 1 for initialized campaign award data
    CampaignAwardData::CampaignAwardData( int id, uint32_t type, uint32_t subType )
        : CampaignAwardData( id, type, subType, 1, 0 )
    {}

    CampaignAwardData::CampaignAwardData( int id, uint32_t type, uint32_t subType, uint32_t amount )
        : CampaignAwardData( id, type, subType, amount, 0 )
    {}

    CampaignAwardData::CampaignAwardData( int id, uint32_t type, uint32_t subType, const std::string & customName )
        : CampaignAwardData( id, type, subType, 1, 0, customName )
    {}

    CampaignAwardData::CampaignAwardData( int id, uint32_t type, uint32_t subType, uint32_t amount, int startScenarioID, const std::string & customName )
        : _id( id )
        , _type( type )
        , _subType( subType )
        , _amount( amount )
        , _startScenarioID( startScenarioID )
        , _customName( customName )
    {}

    std::string CampaignAwardData::ToString() const
    {
        if ( !_customName.empty() )
            return _customName;

        switch ( _type ) {
        case CampaignAwardData::TYPE_CREATURE_CURSE:
            return Monster( _subType ).GetName() + std::string( _( " bane" ) );
        case CampaignAwardData::TYPE_CREATURE_ALLIANCE:
            return Monster( _subType ).GetName() + std::string( _( " alliance" ) );
        case CampaignAwardData::TYPE_GET_ARTIFACT:
            return Artifact( _subType ).GetName();
        case CampaignAwardData::TYPE_CARRY_OVER_FORCES:
            return _( "Carry-over forces" );
        case CampaignAwardData::TYPE_RESOURCE_BONUS:
            return Resource::String( _subType ) + std::string( _( " bonus" ) );
        case CampaignAwardData::TYPE_GET_SPELL:
            return Spell( _subType ).GetName();
        case CampaignAwardData::TYPE_HIREABLE_HERO:
            return Heroes( _subType, 0 ).GetName();
        case CampaignAwardData::TYPE_DEFEAT_ENEMY_HERO:
            return Heroes( _subType, 0 ).GetName() + std::string( _( " defeated" ) );
        default:
            assert( 0 ); // some new/unhandled award
            return "";
        }
    }
}
