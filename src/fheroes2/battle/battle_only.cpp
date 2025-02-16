/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "battle_only.h"

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "army_bar.h"
#include "army_troop.h"
#include "battle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game_hotkeys.h"
#include "heroes.h"
#include "heroes_base.h"
#include "heroes_indicator.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "monster.h"
#include "race.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "skill_bar.h"
#include "spell_book.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    const std::array<int32_t, 2> playerColor{ Color::BLUE, Color::RED };
    const std::array<int32_t, 2> moraleAndLuckOffsetX{ 34, 571 };
    const std::array<int32_t, 2> primarySkillOffsetX{ 216, 389 };
    const std::array<int32_t, 2> secondarySkillOffsetX{ 22, 353 };
    const std::array<int32_t, 2> artifactOffsetX{ 23, 367 };
    const std::array<int32_t, 2> armyOffsetX{ 36, 381 };
}

void Battle::ControlInfo::Redraw() const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Sprite & cell = fheroes2::AGG::GetICN( ICN::CELLWIN, 1 );
    const fheroes2::Sprite & mark = fheroes2::AGG::GetICN( ICN::CELLWIN, 2 );

    fheroes2::Blit( cell, display, rtLocal.x, rtLocal.y );
    if ( result & CONTROL_HUMAN )
        fheroes2::Blit( mark, display, rtLocal.x + 3, rtLocal.y + 2 );
    fheroes2::Text text( _( "Human" ), fheroes2::FontType::smallWhite() );
    text.draw( rtLocal.x + cell.width() + 5, rtLocal.y + 5, display );

    fheroes2::Blit( cell, display, rtAI.x, rtAI.y );
    if ( result & CONTROL_AI )
        fheroes2::Blit( mark, display, rtAI.x + 3, rtAI.y + 2 );
    text.set( _( "AI" ), fheroes2::FontType::smallWhite() );
    text.draw( rtAI.x + cell.width() + 5, rtAI.y + 5, display );
}

Battle::Only::Only()
{
    armyInfo[1].armyId = 1;

    for ( auto & info : armyInfo ) {
        info.monster.GetTroop( 0 )->Set( Monster::PEASANT, 100 );
        info.monsterBackup.Assign( info.monster );
    }

    armyInfo[0].controlType = CONTROL_HUMAN;
    armyInfo[0].player.SetControl( armyInfo[0].controlType );
    armyInfo[0].player.SetColor( playerColor[0] );

    armyInfo[1].controlType = CONTROL_AI;
    armyInfo[1].player.SetControl( armyInfo[1].controlType );
    armyInfo[1].player.SetColor( playerColor[1] );
}

bool Battle::Only::setup( const bool allowBackup, bool & reset )
{
    reset = false;

    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::StandardWindow frameborder( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, false );

    const fheroes2::Point cur_pt( frameborder.activeArea().x, frameborder.activeArea().y );

    armyInfo[0].portraitRoi = { cur_pt.x + 93, cur_pt.y + 72, 101, 93 };
    armyInfo[1].portraitRoi = { cur_pt.x + 445, cur_pt.y + 72, 101, 93 };

    armyInfo[0].player.SetControl( armyInfo[0].controlType );
    armyInfo[1].player.SetControl( armyInfo[1].controlType );

    for ( auto & info : armyInfo ) {
        info.hero = nullptr;
        info.monster.Reset();
        info.monster.GetTroop( 0 )->Set( Monster::PEASANT, 100 );

        if ( !_backupCompleted || !allowBackup ) {
            continue;
        }

        if ( info.isHeroPresent ) {
            info.hero = world.GetHeroes( info.heroBackup.GetID() );
            info.hero->GetSecondarySkills().FillMax( Skill::Secondary() );

            copyHero( info.heroBackup, *info.hero );
        }
        else {
            info.monster.Assign( info.monsterBackup );
        }
    }

    if ( !_backupCompleted || !allowBackup ) {
        armyInfo[0].hero = world.GetHeroes( Heroes::LORDKILBURN );
        armyInfo[0].isHeroPresent = true;
    }

    const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
    fheroes2::Copy( background, 0, 0, display, cur_pt.x, cur_pt.y, background.width(), background.height() );

    redrawOpponents( cur_pt );
    redrawOpponentsStats( cur_pt );

    for ( auto & info : armyInfo ) {
        if ( info.hero != nullptr ) {
            info.hero->GetSecondarySkills().FillMax( Skill::Secondary() );

            updateHero( info, cur_pt );
        }
        else {
            info.ui.army = std::make_unique<ArmyBar>( &info.monster, true, false, true );
            info.ui.army->setTableSize( { 5, 1 } );
            info.ui.army->setRenderingOffset( { cur_pt.x + armyOffsetX[info.armyId], cur_pt.y + 267 } );
            info.ui.army->setInBetweenItemsOffset( { 2, 0 } );
        }
    }

    if ( armyInfo[1].hero != nullptr ) {
        attackedArmyControlInfo = std::make_unique<ControlInfo>( fheroes2::Point{ cur_pt.x + 500, cur_pt.y + 425 }, armyInfo[1].player.GetControl() );
    }

    for ( const auto & info : armyInfo ) {
        info.ui.redraw( display );
    }

    if ( attackedArmyControlInfo ) {
        attackedArmyControlInfo->Redraw();
    }

    // hide the swap army/artifact arrows
    const fheroes2::Sprite & stoneBackground = fheroes2::AGG::GetICN( ICN::STONEBAK, 0 );
    fheroes2::Copy( stoneBackground, 292, 270, display, cur_pt.x + 292, cur_pt.y + 270, 48, 44 );
    fheroes2::Copy( stoneBackground, 292, 363, display, cur_pt.x + 292, cur_pt.y + 363, 48, 44 );

    // hide the shadow from the original EXIT button
    const fheroes2::Sprite buttonOverride = fheroes2::Crop( fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 ), 122, 428, 84, 32 );
    fheroes2::Copy( buttonOverride, 0, 0, display, cur_pt.x + 276, cur_pt.y + 428, 84, 32 );

    fheroes2::Button buttonReset( cur_pt.x + 30, cur_pt.y + 428, ICN::BUTTON_RESET_GOOD, 0, 1 );
    fheroes2::Button buttonStart( cur_pt.x + 178, cur_pt.y + 428, ICN::BUTTON_START_GOOD, 0, 1 );
    fheroes2::Button buttonExit( cur_pt.x + 366, cur_pt.y + 428, ICN::BUTTON_EXIT_GOOD, 0, 1 );

    fheroes2::addGradientShadow( fheroes2::AGG::GetICN( ICN::BUTTON_RESET_GOOD, 0 ), display, buttonReset.area().getPosition(), { -5, 5 } );
    fheroes2::addGradientShadow( fheroes2::AGG::GetICN( ICN::BUTTON_START_GOOD, 0 ), display, buttonStart.area().getPosition(), { -5, 5 } );
    fheroes2::addGradientShadow( fheroes2::AGG::GetICN( ICN::BUTTON_EXIT_GOOD, 0 ), display, buttonExit.area().getPosition(), { -5, 5 } );

    buttonStart.draw();
    buttonExit.draw();
    buttonReset.draw();

    display.render();

    bool result = false;

    while ( le.HandleEvents() ) {
        bool updateSpellPoints = false;
        bool needRender = false;
        bool needRedrawOpponentsStats = false;
        bool needRedrawControlInfo = false;

        if ( buttonStart.isEnabled() ) {
            buttonStart.drawOnState( le.isMouseLeftButtonPressedInArea( buttonStart.area() ) );
        }
        if ( buttonExit.isEnabled() ) {
            buttonExit.drawOnState( le.isMouseLeftButtonPressedInArea( buttonExit.area() ) );
        }
        if ( buttonReset.isEnabled() ) {
            buttonReset.drawOnState( le.isMouseLeftButtonPressedInArea( buttonReset.area() ) );
        }

        if ( ( buttonStart.isEnabled() && le.MouseClickLeft( buttonStart.area() ) ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
            result = true;

            break;
        }
        if ( le.MouseClickLeft( buttonReset.area() ) ) {
            reset = true;
            result = true;

            break;
        }

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            break;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonStart.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Start" ), _( "Start the battle." ), 0 );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonExit.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Exit" ), _( "Exit this menu." ), 0 );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonReset.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Reset" ), _( "Reset to default settings." ), 0 );
        }

        for ( const auto & [firstId, secondId] : { std::pair<int32_t, int32_t>( 0, 1 ), std::pair<int32_t, int32_t>( 1, 0 ) } ) {
            ArmyInfo & first = armyInfo[firstId];
            const ArmyInfo & second = armyInfo[secondId];

            if ( le.MouseClickLeft( first.portraitRoi ) ) {
                const int hid = Dialog::selectHeroes( first.hero ? first.hero->GetID() : Heroes::UNKNOWN );
                if ( second.hero && hid == second.hero->GetID() ) {
                    fheroes2::showStandardTextMessage( _( "Error" ), _( "Please select another hero." ), Dialog::OK );
                }
                else if ( Heroes::UNKNOWN != hid ) {
                    first.hero = world.GetHeroes( hid );

                    if ( first.hero ) {
                        first.hero->GetSecondarySkills().FillMax( Skill::Secondary() );
                    }

                    updateHero( first, cur_pt );
                }

                redrawOpponents( cur_pt );

                first.needRedraw = true;
                needRedrawOpponentsStats = true;

                // User can not click two hero portraits at the same time so we can break the loop.
                break;
            }
        }

        if ( attackedArmyControlInfo == nullptr && armyInfo[1].hero != nullptr ) {
            attackedArmyControlInfo = std::make_unique<ControlInfo>( fheroes2::Point{ cur_pt.x + 500, cur_pt.y + 425 }, armyInfo[1].player.GetControl() );
            needRedrawControlInfo = true;
        }

        for ( const auto & [firstId, secondId] : { std::pair<int32_t, int32_t>{ 0, 1 }, std::pair<int32_t, int32_t>{ 1, 0 } } ) {
            const ArmyUI & firstUI = armyInfo[firstId].ui;
            const ArmyUI & secondUI = armyInfo[secondId].ui;

            if ( firstUI.army != nullptr && le.isMouseCursorPosInArea( firstUI.army->GetArea() ) && firstUI.army->QueueEventProcessing() ) {
                if ( firstUI.artifact != nullptr && firstUI.artifact->isSelected() ) {
                    firstUI.artifact->ResetSelected();
                }

                if ( secondUI.artifact != nullptr && secondUI.artifact->isSelected() ) {
                    secondUI.artifact->ResetSelected();
                }

                if ( secondUI.army != nullptr && secondUI.army->isSelected() ) {
                    secondUI.army->ResetSelected();
                }

                armyInfo[firstId].needRedraw = true;
            }
            else if ( firstUI.artifact != nullptr && le.isMouseCursorPosInArea( firstUI.artifact->GetArea() ) && firstUI.artifact->QueueEventProcessing() ) {
                if ( firstUI.army != nullptr && firstUI.army->isSelected() ) {
                    firstUI.army->ResetSelected();
                }

                if ( secondUI.artifact != nullptr && secondUI.artifact->isSelected() ) {
                    secondUI.artifact->ResetSelected();
                }

                if ( secondUI.army != nullptr && secondUI.army->isSelected() ) {
                    secondUI.army->ResetSelected();
                }

                armyInfo[firstId].needRedraw = true;
                updateSpellPoints = true;
                needRedrawOpponentsStats = true;
            }
            else if ( firstUI.morale != nullptr && le.isMouseCursorPosInArea( firstUI.morale->GetArea() ) ) {
                MoraleIndicator::QueueEventProcessing( *firstUI.morale );
            }
            else if ( firstUI.luck != nullptr && le.isMouseCursorPosInArea( firstUI.luck->GetArea() ) ) {
                LuckIndicator::QueueEventProcessing( *firstUI.luck );
            }
            else if ( firstUI.primarySkill != nullptr && le.isMouseCursorPosInArea( firstUI.primarySkill->GetArea() ) ) {
                if ( firstUI.primarySkill->QueueEventProcessing() ) {
                    updateSpellPoints = true;
                    needRedrawOpponentsStats = true;
                }
            }
            else if ( firstUI.secondarySkill != nullptr && le.isMouseCursorPosInArea( firstUI.secondarySkill->GetArea() )
                      && firstUI.secondarySkill->QueueEventProcessing() ) {
                armyInfo[firstId].needRedraw = true;
            }
        }

        if ( attackedArmyControlInfo ) {
            assert( armyInfo[1].hero );

            if ( le.MouseClickLeft( attackedArmyControlInfo->rtLocal ) && armyInfo[1].player.isControlAI() ) {
                attackedArmyControlInfo->result = CONTROL_HUMAN;
                armyInfo[1].player.SetControl( CONTROL_HUMAN );

                needRedrawControlInfo = true;
            }
            else if ( le.MouseClickLeft( attackedArmyControlInfo->rtAI ) && armyInfo[1].player.isControlHuman() ) {
                attackedArmyControlInfo->result = CONTROL_AI;
                armyInfo[1].player.SetControl( CONTROL_AI );

                needRedrawControlInfo = true;
            }
        }

        if ( updateSpellPoints ) {
            for ( Heroes * hero : { armyInfo[0].hero, armyInfo[1].hero } ) {
                if ( hero == nullptr ) {
                    continue;
                }

                hero->SetSpellPoints( hero->GetMaxSpellPoints() );
            }
        }

        if ( needRedrawOpponentsStats ) {
            redrawOpponentsStats( cur_pt );

            needRender = true;
        }

        for ( const int32_t i : { 0, 1 } ) {
            if ( armyInfo[i].needRedraw ) {
                armyInfo[i].ui.redraw( display );
                armyInfo[i].needRedraw = false;

                needRender = true;
            }
        }

        if ( needRedrawControlInfo ) {
            assert( attackedArmyControlInfo != nullptr );
            attackedArmyControlInfo->Redraw();

            needRender = true;
        }

        if ( needRender ) {
            display.render();
        }
    }

    armyInfo[0].ui = {};
    armyInfo[1].ui = {};

    attackedArmyControlInfo.reset();

    return result;
}

void Battle::Only::updateHero( ArmyInfo & info, const fheroes2::Point & offset )
{
    info.ui.resetForNewHero();

    if ( info.hero == nullptr ) {
        return;
    }

    updateArmyUI( info.ui, info.hero, offset, info.armyId );
}

void Battle::Only::redrawOpponents( const fheroes2::Point & top ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );
    const fheroes2::Rect textRoi( top.x + 89, top.y + 27, 462, 17 );
    fheroes2::Copy( background, 89, 27, display, textRoi );

    std::string message = _( "%{race1} %{name1} vs %{race2} %{name2}" );

    if ( armyInfo[0].hero ) {
        StringReplace( message, "%{name1}", armyInfo[0].hero->GetName() );
        StringReplace( message, "%{race1}", std::string( Race::String( armyInfo[0].hero->GetRace() ) ) );
    }
    else {
        StringReplace( message, "%{race1}", "" );
        StringReplace( message, " %{name1}", _( "Monsters" ) );
    }
    if ( armyInfo[1].hero ) {
        StringReplace( message, "%{name2}", armyInfo[1].hero->GetName() );
        StringReplace( message, "%{race2}", std::string( Race::String( armyInfo[1].hero->GetRace() ) ) );
    }
    else {
        StringReplace( message, "%{race2}", "" );
        StringReplace( message, " %{name2}", _( "Monsters" ) );
    }

    fheroes2::Text text( std::move( message ), fheroes2::FontType::normalWhite() );
    text.drawInRoi( top.x + 320 - text.width() / 2, top.y + 29, display, textRoi );

    for ( const size_t idx : { 0, 1 } ) {
        if ( armyInfo[idx].hero ) {
            const fheroes2::Sprite & port1 = armyInfo[idx].hero->GetPortrait( PORT_BIG );
            if ( !port1.empty() ) {
                fheroes2::Copy( port1, 0, 0, display, armyInfo[idx].portraitRoi );
            }
        }
        else {
            fheroes2::Fill( display, armyInfo[idx].portraitRoi.x, armyInfo[idx].portraitRoi.y, armyInfo[idx].portraitRoi.width, armyInfo[idx].portraitRoi.height, 0 );
            text.set( _( "N/A" ), fheroes2::FontType::normalWhite() );
            text.draw( armyInfo[idx].portraitRoi.x + ( armyInfo[idx].portraitRoi.width - text.width() ) / 2,
                       armyInfo[idx].portraitRoi.y + armyInfo[idx].portraitRoi.height / 2 - 8, display );
        }
    }
}

void Battle::Only::redrawOpponentsStats( const fheroes2::Point & top ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );

    fheroes2::Copy( background, 262, 61, display, top.x + 262, top.y + 61, 115, 109 );
    fheroes2::RedrawPrimarySkillInfo( top, armyInfo[0].ui.primarySkill.get(), armyInfo[1].ui.primarySkill.get() );
}

void Battle::Only::StartBattle()
{
    assert( armyInfo[0].hero != nullptr );

    Settings & conf = Settings::Get();

    conf.GetPlayers().Init( armyInfo[0].player.GetColor() | armyInfo[1].player.GetColor() );
    world.InitKingdoms();

    conf.SetCurrentColor( armyInfo[0].player.GetColor() );

    for ( const int32_t idx : { 0, 1 } ) {
        Players::SetPlayerRace( armyInfo[idx].player.GetColor(), armyInfo[idx].player.GetRace() );
        Players::SetPlayerControl( armyInfo[idx].player.GetColor(), armyInfo[idx].player.GetControl() );
        armyInfo[idx].controlType = armyInfo[idx].player.GetControl();

        armyInfo[idx].isHeroPresent = ( armyInfo[idx].hero != nullptr );

        if ( !armyInfo[idx].isHeroPresent ) {
            armyInfo[idx].monsterBackup.Assign( armyInfo[idx].monster );
            continue;
        }

        armyInfo[idx].hero->SetSpellPoints( armyInfo[idx].hero->GetMaxSpellPoints() );
        armyInfo[idx].hero->Recruit( armyInfo[idx].player.GetColor(), { idx, idx } );

        copyHero( *armyInfo[idx].hero, armyInfo[idx].heroBackup );

        armyInfo[idx].monster.Reset();
        armyInfo[idx].monster.GetTroop( 0 )->Set( Monster::PEASANT, 100 );
        armyInfo[idx].monsterBackup.Assign( armyInfo[idx].monster );
    }

    _backupCompleted = true;

    Battle::Loader( ( armyInfo[0].hero ? armyInfo[0].hero->GetArmy() : armyInfo[0].monster ), ( armyInfo[1].hero ? armyInfo[1].hero->GetArmy() : armyInfo[1].monster ),
                    1 );

    conf.SetCurrentColor( Color::NONE );
}

void Battle::Only::reset()
{
    armyInfo[0].reset();
    armyInfo[1].reset();

    attackedArmyControlInfo.reset();
}

void Battle::Only::copyHero( const Heroes & in, Heroes & out )
{
    out.attack = in.attack;
    out.defense = in.defense;
    out.knowledge = in.knowledge;
    out.power = in.power;
    out._id = in._id;
    out.portrait = in.portrait;
    out._race = in._race;

    out.secondary_skills.ToVector() = in.secondary_skills.ToVector();
    out.army.Assign( in.army );

    out.bag_artifacts = in.bag_artifacts;
    out.spell_book = in.spell_book;

    out.SetSpellPoints( out.GetMaxSpellPoints() );
}

void Battle::Only::updateArmyUI( ArmyUI & ui, Heroes * hero, const fheroes2::Point & offset, const uint8_t armyId )
{
    assert( hero != nullptr );

    ui.morale = std::make_unique<MoraleIndicator>( hero );
    ui.morale->SetPos( { offset.x + moraleAndLuckOffsetX[armyId], offset.y + 75 } );

    ui.luck = std::make_unique<LuckIndicator>( hero );
    ui.luck->SetPos( { offset.x + moraleAndLuckOffsetX[armyId], offset.y + 115 } );

    ui.primarySkill = std::make_unique<PrimarySkillsBar>( hero, true, true, false );
    ui.primarySkill->setTableSize( { 1, 4 } );
    ui.primarySkill->setInBetweenItemsOffset( { 0, -1 } );
    ui.primarySkill->SetTextOff( armyId == 0 ? 70 : -70, -25 );
    ui.primarySkill->setRenderingOffset( { offset.x + primarySkillOffsetX[armyId], offset.y + 51 } );

    ui.secondarySkill = std::make_unique<SecondarySkillsBar>( *hero, true, true );
    ui.secondarySkill->setTableSize( { 8, 1 } );
    ui.secondarySkill->setInBetweenItemsOffset( { -1, 0 } );
    ui.secondarySkill->SetContent( hero->GetSecondarySkills().ToVector() );
    ui.secondarySkill->setRenderingOffset( { offset.x + secondarySkillOffsetX[armyId], offset.y + 199 } );

    ui.artifact = std::make_unique<ArtifactsBar>( hero, true, false, true, true, nullptr );
    ui.artifact->setTableSize( { 7, 2 } );
    ui.artifact->setInBetweenItemsOffset( { 2, 2 } );
    ui.artifact->SetContent( hero->GetBagArtifacts() );
    ui.artifact->setRenderingOffset( { offset.x + artifactOffsetX[armyId], offset.y + 347 } );

    ui.army = std::make_unique<ArmyBar>( &hero->GetArmy(), true, false, true );
    ui.army->setTableSize( { 5, 1 } );
    ui.army->setRenderingOffset( { offset.x + armyOffsetX[armyId], offset.y + 267 } );
    ui.army->setInBetweenItemsOffset( { 2, 0 } );
}

void Battle::Only::ArmyUI::redraw( fheroes2::Image & output ) const
{
    if ( morale ) {
        morale->Redraw();
    }

    if ( luck ) {
        luck->Redraw();
    }

    if ( primarySkill ) {
        primarySkill->Redraw( output );
    }

    if ( secondarySkill ) {
        secondarySkill->Redraw( output );
    }

    if ( artifact ) {
        artifact->Redraw( output );
    }

    if ( army ) {
        army->Redraw( output );
    }
}

void Battle::Only::ArmyUI::resetForNewHero()
{
    if ( morale ) {
        morale->redrawOnlyBackground();
    }

    if ( luck ) {
        luck->redrawOnlyBackground();
    }

    *this = {};
}

void Battle::Only::ArmyInfo::reset()
{
    ui = {};
    hero = nullptr;

    monster.Reset();
    monster.GetTroop( 0 )->Set( Monster::PEASANT, 100 );
}
