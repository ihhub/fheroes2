/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2024                                             *
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

#include "ui_campaign.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "army_troop.h"
#include "artifact.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "dialog.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "math_base.h"
#include "monster.h"
#include "skill.h"
#include "spell.h"
#include "spell_info.h"
#include "ui_dialog.h"
#include "ui_monster.h"
#include "ui_text.h"

namespace
{
    fheroes2::Sprite getMonsterFrame( const Monster & monster, const int32_t count )
    {
        const fheroes2::Point frameOffset = { 6, 6 };

        fheroes2::Sprite output = fheroes2::AGG::GetICN( ICN::STRIP, 12 );
        renderMonsterFrame( monster, output, frameOffset );

        if ( count > 0 ) {
            const fheroes2::Text monsterCountText( "+" + std::to_string( count ), fheroes2::FontType::normalWhite() );
            monsterCountText.draw( output.width() - frameOffset.x - monsterCountText.width() - 3, output.height() - frameOffset.y - monsterCountText.height() + 2,
                                   output );
        }

        return output;
    }
}

namespace fheroes2
{
    void showScenarioBonusDataPopupWindow( const Campaign::ScenarioBonusData & bonusData )
    {
        switch ( bonusData._type ) {
        case Campaign::ScenarioBonusData::ARTIFACT: {
            Artifact artifact( bonusData._subType );

            if ( artifact == Artifact::SPELL_SCROLL ) {
                artifact.SetSpell( bonusData._artifactSpellId );
            }

            const ArtifactDialogElement artifactUI( artifact );
            const TextDialogElement artifactDescriptionUI( std::make_shared<Text>( artifact.GetDescription(), FontType::normalWhite() ) );

            showStandardTextMessage( bonusData.getName(), bonusData.getDescription(), Dialog::ZERO, { &artifactUI, &artifactDescriptionUI } );
            break;
        }
        case Campaign::ScenarioBonusData::RESOURCES: {
            const std::string resourceAmount = std::to_string( bonusData._amount );
            const ResourceDialogElement resourceUI( bonusData._subType, ( bonusData._amount > 0 ? "+" + resourceAmount : resourceAmount ) );

            showStandardTextMessage( bonusData.getName(), bonusData.getDescription(), Dialog::ZERO, { &resourceUI } );
            break;
        }
        case Campaign::ScenarioBonusData::TROOP: {
            const Monster monster( bonusData._subType );
            const CustomImageDialogElement monsterUI( getMonsterFrame( monster, bonusData._amount ) );

            showStandardTextMessage( bonusData.getName(), bonusData.getDescription(), Dialog::ZERO, { &monsterUI } );
            break;
        }
        case Campaign::ScenarioBonusData::SPELL: {
            const Spell spell( bonusData._subType );
            const SpellDialogElement spellUI( spell, nullptr );
            const TextDialogElement spellDescriptionUI( std::make_shared<Text>( getSpellDescription( spell, nullptr ), FontType::normalWhite() ) );

            showStandardTextMessage( bonusData.getName(), bonusData.getDescription(), Dialog::ZERO, { &spellUI, &spellDescriptionUI } );
            break;
        }
        case Campaign::ScenarioBonusData::STARTING_RACE:
        case Campaign::ScenarioBonusData::STARTING_RACE_AND_ARMY: {
            const CustomImageDialogElement raceUI( AGG::GetICN( ICN::Get4Captain( bonusData._subType ), 1 ) );

            showStandardTextMessage( bonusData.getName(), bonusData.getDescription(), Dialog::ZERO, { &raceUI } );
            break;
        }
        case Campaign::ScenarioBonusData::SKILL_PRIMARY: {
            const PrimarySkillDialogElement primarySkillUI( bonusData._subType, "+" + std::to_string( bonusData._amount ) );
            const TextDialogElement skillDescriptionUI(
                std::make_shared<Text>( Skill::Primary::StringDescription( bonusData._subType, nullptr ), FontType::normalWhite() ) );

            showStandardTextMessage( bonusData.getName(), bonusData.getDescription(), Dialog::ZERO, { &primarySkillUI, &skillDescriptionUI } );
            break;
        }
        case Campaign::ScenarioBonusData::SKILL_SECONDARY: {
            Heroes fakeHero;
            Skill::Secondary skill( bonusData._subType, bonusData._amount );
            const SecondarySkillDialogElement secondarySkillUI( skill, fakeHero );
            const TextDialogElement skillDescriptionUI( std::make_shared<Text>( skill.GetDescription( fakeHero ), FontType::normalWhite() ) );

            showStandardTextMessage( bonusData.getName(), bonusData.getDescription(), Dialog::ZERO, { &secondarySkillUI, &skillDescriptionUI } );
            break;
        }
        default:
            assert( 0 ); // some new bonus?
            break;
        }
    }

    void showAwardDataPopupWindow( const Campaign::CampaignAwardData & awardData )
    {
        switch ( awardData._type ) {
        case Campaign::CampaignAwardData::TYPE_CREATURE_CURSE:
        case Campaign::CampaignAwardData::TYPE_CREATURE_ALLIANCE: {
            std::vector<Monster> monsters;

            std::vector<const DialogElement *> uiElements;
            std::vector<std::unique_ptr<CustomImageDialogElement>> monsterUI;
            monsters.emplace_back( awardData._subType );
            monsterUI.emplace_back( new CustomImageDialogElement( getMonsterFrame( monsters.back(), 0 ) ) );
            uiElements.emplace_back( monsterUI.back().get() );

            while ( monsters.back() != monsters.back().GetUpgrade() ) {
                monsters.emplace_back( monsters.back().GetUpgrade() );
                monsterUI.emplace_back( new CustomImageDialogElement( getMonsterFrame( monsters.back(), 0 ) ) );
                uiElements.emplace_back( monsterUI.back().get() );
            }

            showStandardTextMessage( awardData.getName(), awardData.getDescription(), Dialog::ZERO, uiElements );
            break;
        }
        case Campaign::CampaignAwardData::TYPE_GET_ARTIFACT: {
            const Artifact artifact( awardData._subType );
            const ArtifactDialogElement artifactUI( artifact );
            const TextDialogElement artifactDescriptionUI( std::make_shared<Text>( artifact.GetDescription(), FontType::normalWhite() ) );

            showStandardTextMessage( awardData.getName(), awardData.getDescription(), Dialog::ZERO, { &artifactUI, &artifactDescriptionUI } );
            break;
        }
        case Campaign::CampaignAwardData::TYPE_CARRY_OVER_FORCES: {
            const std::vector<Troop> & troops = Campaign::CampaignSaveData::Get().getCarryOverTroops();
            std::vector<const DialogElement *> uiElements;
            std::vector<std::unique_ptr<CustomImageDialogElement>> monsterUI;
            for ( const Troop & troop : troops ) {
                if ( !troop.isValid() ) {
                    continue;
                }

                monsterUI.emplace_back( new CustomImageDialogElement( getMonsterFrame( troop, troop.GetCount() ) ) );
                uiElements.emplace_back( monsterUI.back().get() );
            }

            showStandardTextMessage( awardData.getName(), awardData.getDescription(), Dialog::ZERO, uiElements );
            break;
        }
        case Campaign::CampaignAwardData::TYPE_RESOURCE_BONUS: {
            const int32_t resourceType = awardData._subType;
            const std::string resourceAmount = std::to_string( awardData._amount );
            const ResourceDialogElement resourceUI( resourceType, ( awardData._amount > 0 ? "+" + resourceAmount : resourceAmount ) );

            showStandardTextMessage( awardData.getName(), awardData.getDescription(), Dialog::ZERO, { &resourceUI } );
            break;
        }
        case Campaign::CampaignAwardData::TYPE_GET_SPELL: {
            const Spell spell( awardData._subType );
            const SpellDialogElement spellUI( spell, nullptr );
            const TextDialogElement spellDescriptionUI( std::make_shared<Text>( getSpellDescription( spell, nullptr ), FontType::normalWhite() ) );

            showStandardTextMessage( awardData.getName(), awardData.getDescription(), Dialog::ZERO, { &spellUI, &spellDescriptionUI } );
            break;
        }
        case Campaign::CampaignAwardData::TYPE_HIREABLE_HERO:
        case Campaign::CampaignAwardData::TYPE_DEFEAT_ENEMY_HERO: {
            Sprite output = Crop( AGG::GetICN( ICN::HEROBKG, 0 ), 47, 29, 105, 98 );
            const Sprite & heroPortrait = AGG::GetICN( ICN::PORTxxxx( awardData._subType ), 0 );
            Blit( heroPortrait, 0, 0, output, 2, 2, heroPortrait.width(), heroPortrait.height() );

            const CustomImageDialogElement heroUI( std::move( output ) );

            showStandardTextMessage( awardData.getName(), awardData.getDescription(), Dialog::ZERO, { &heroUI } );
            break;
        }
        default:
            // Did you add a new award? Add the logic above!
            assert( 0 );
            break;
        }
    }
}
