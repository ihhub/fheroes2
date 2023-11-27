/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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
    const uint32_t primaryMaxValue = 20;
    const int32_t primarySkillIconSize{ 33 };

    const std::array<int32_t, 2> playerColor{ Color::BLUE, Color::RED };
    const std::array<int32_t, 2> moraleAndLuckOffsetX{ 34, 566 };
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

    const std::array<fheroes2::Rect, 2> attackRoi{ fheroes2::Rect( cur_pt.x + 215, cur_pt.y + 50, primarySkillIconSize, primarySkillIconSize ),
                                                   fheroes2::Rect( cur_pt.x + 390, cur_pt.y + 50, primarySkillIconSize, primarySkillIconSize ) };
    const std::array<fheroes2::Rect, 2> defenseRoi{ fheroes2::Rect( cur_pt.x + 215, cur_pt.y + 83, primarySkillIconSize, primarySkillIconSize ),
                                                    fheroes2::Rect( cur_pt.x + 390, cur_pt.y + 83, primarySkillIconSize, primarySkillIconSize ) };
    const std::array<fheroes2::Rect, 2> powerRoi{ fheroes2::Rect( cur_pt.x + 215, cur_pt.y + 116, primarySkillIconSize, primarySkillIconSize ),
                                                  fheroes2::Rect( cur_pt.x + 390, cur_pt.y + 116, primarySkillIconSize, primarySkillIconSize ) };
    const std::array<fheroes2::Rect, 2> knowledgeRoi{ fheroes2::Rect( cur_pt.x + 215, cur_pt.y + 149, primarySkillIconSize, primarySkillIconSize ),
                                                      fheroes2::Rect( cur_pt.x + 390, cur_pt.y + 149, primarySkillIconSize, primarySkillIconSize ) };

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

    RedrawBaseInfo( cur_pt );

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
        cinfo2 = std::make_unique<ControlInfo>( fheroes2::Point{ cur_pt.x + 500, cur_pt.y + 425 }, armyInfo[1].player.GetControl() );
    }

    for ( const auto & info : armyInfo ) {
        info.ui.redraw( display );
    }

    if ( cinfo2 ) {
        cinfo2->Redraw();
    }

    bool exit = false;
    bool redraw = false;
    bool result = false;

    // hide the shadow from the original EXIT button
    const fheroes2::Sprite buttonOverride = fheroes2::Crop( fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 ), 122, 428, 84, 32 );
    fheroes2::Blit( buttonOverride, display, cur_pt.x + 276, cur_pt.y + 428 );

    fheroes2::ButtonSprite buttonOkay = fheroes2::makeButtonWithShadow( cur_pt.x + 178, cur_pt.y + 428, fheroes2::AGG::GetICN( ICN::BUTTON_SMALL_OKAY_GOOD, 0 ),
                                                                        fheroes2::AGG::GetICN( ICN::BUTTON_SMALL_OKAY_GOOD, 1 ), display );
    fheroes2::ButtonSprite buttonCancel = fheroes2::makeButtonWithShadow( cur_pt.x + 366, cur_pt.y + 428, fheroes2::AGG::GetICN( ICN::BUTTON_SMALL_CANCEL_GOOD, 0 ),
                                                                          fheroes2::AGG::GetICN( ICN::BUTTON_SMALL_CANCEL_GOOD, 1 ), display );

    fheroes2::ButtonSprite buttonReset = fheroes2::makeButtonWithShadow( cur_pt.x + 30, cur_pt.y + 428, fheroes2::AGG::GetICN( ICN::BUTTON_RESET_GOOD, 0 ),
                                                                         fheroes2::AGG::GetICN( ICN::BUTTON_RESET_GOOD, 1 ), display );

    buttonOkay.draw();
    buttonCancel.draw();
    buttonReset.draw();

    display.render();

    while ( !exit && le.HandleEvents() ) {
        buttonOkay.isEnabled() && le.MousePressLeft( buttonOkay.area() ) ? buttonOkay.drawOnPress() : buttonOkay.drawOnRelease();
        buttonCancel.isEnabled() && le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();
        buttonReset.isEnabled() && le.MousePressLeft( buttonReset.area() ) ? buttonReset.drawOnPress() : buttonReset.drawOnRelease();

        if ( ( buttonOkay.isEnabled() && le.MouseClickLeft( buttonOkay.area() ) ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
            result = true;
            exit = true;
        }
        if ( le.MouseClickLeft( buttonReset.area() ) ) {
            reset = true;
            result = true;
            break;
        }

        if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            exit = true;
        }

        for ( const auto & ids : { std::pair<int32_t, int32_t>( 0, 1 ), std::pair<int32_t, int32_t>( 1, 0 ) } ) {
            ArmyInfo & first = armyInfo[ids.first];
            const ArmyInfo & second = armyInfo[ids.second];

            if ( le.MouseClickLeft( first.portraitRoi ) ) {
                int hid = Dialog::selectHeroes( first.hero ? first.hero->GetID() : Heroes::UNKNOWN );
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

                redraw = true;
            }
        }

        if ( cinfo2 == nullptr && armyInfo[1].hero != nullptr ) {
            cinfo2 = std::make_unique<ControlInfo>( fheroes2::Point{ cur_pt.x + 500, cur_pt.y + 425 }, armyInfo[1].player.GetControl() );
        }

        for ( const auto & [hero, index] : { std::pair<Heroes *, size_t>( armyInfo[0].hero, 0 ), std::pair<Heroes *, size_t>( armyInfo[1].hero, 1 ) } ) {
            if ( hero == nullptr ) {
                continue;
            }

            if ( le.MouseClickLeft( attackRoi[index] ) ) {
                uint32_t value = hero->attack;
                if ( Dialog::SelectCount( _( "Set Attack Skill" ), 0, primaryMaxValue, value ) ) {
                    hero->attack = value;
                    redraw = true;
                }
            }
            else if ( le.MouseClickLeft( defenseRoi[index] ) ) {
                uint32_t value = hero->defense;
                if ( Dialog::SelectCount( _( "Set Defense Skill" ), 0, primaryMaxValue, value ) ) {
                    hero->defense = value;
                    redraw = true;
                }
            }
            else if ( le.MouseClickLeft( powerRoi[index] ) ) {
                uint32_t value = hero->power;
                if ( Dialog::SelectCount( _( "Set Power Skill" ), 0, primaryMaxValue, value ) ) {
                    hero->power = value;
                    redraw = true;
                }
            }
            else if ( le.MouseClickLeft( knowledgeRoi[index] ) ) {
                uint32_t value = hero->knowledge;
                if ( Dialog::SelectCount( _( "Set Knowledge Skill" ), 0, primaryMaxValue, value ) ) {
                    hero->knowledge = value;
                    hero->SetSpellPoints( hero->knowledge * 10 );
                    redraw = true;
                }
            }
        }

        for ( const auto & ids : { std::pair<int32_t, int32_t>{ 0, 1 }, std::pair<int32_t, int32_t>{ 1, 0 } } ) {
            ArmyUI & firstUI = armyInfo[ids.first].ui;
            ArmyUI & secondUI = armyInfo[ids.second].ui;

            if ( firstUI.army != nullptr && le.MouseCursor( firstUI.army->GetArea() ) && firstUI.army->QueueEventProcessing() ) {
                if ( firstUI.artifact != nullptr && firstUI.artifact->isSelected() ) {
                    firstUI.artifact->ResetSelected();
                }

                if ( secondUI.artifact != nullptr && secondUI.artifact->isSelected() ) {
                    secondUI.artifact->ResetSelected();
                }

                if ( secondUI.army != nullptr && secondUI.army->isSelected() ) {
                    secondUI.army->ResetSelected();
                }

                redraw = true;
            }
            else if ( firstUI.artifact != nullptr && le.MouseCursor( firstUI.artifact->GetArea() ) && firstUI.artifact->QueueEventProcessing() ) {
                if ( firstUI.army != nullptr && firstUI.army->isSelected() ) {
                    firstUI.army->ResetSelected();
                }

                if ( secondUI.artifact != nullptr && secondUI.artifact->isSelected() ) {
                    secondUI.artifact->ResetSelected();
                }

                if ( secondUI.army != nullptr && secondUI.army->isSelected() ) {
                    secondUI.army->ResetSelected();
                }

                redraw = true;
            }
            else if ( firstUI.morale != nullptr && le.MouseCursor( firstUI.morale->GetArea() ) ) {
                MoraleIndicator::QueueEventProcessing( *firstUI.morale );
            }
            else if ( firstUI.luck != nullptr && le.MouseCursor( firstUI.luck->GetArea() ) ) {
                LuckIndicator::QueueEventProcessing( *firstUI.luck );
            }
            else if ( firstUI.primarySkill != nullptr && le.MouseCursor( firstUI.primarySkill->GetArea() ) ) {
                firstUI.primarySkill->QueueEventProcessing();
                redraw = true;
            }
            else if ( firstUI.secondarySkill != nullptr && le.MouseCursor( firstUI.secondarySkill->GetArea() ) ) {
                firstUI.secondarySkill->QueueEventProcessing();
                redraw = true;
            }
        }

        if ( cinfo2 ) {
            if ( armyInfo[1].hero && le.MouseClickLeft( cinfo2->rtLocal ) && armyInfo[1].player.isControlAI() ) {
                cinfo2->result = CONTROL_HUMAN;
                armyInfo[1].player.SetControl( CONTROL_HUMAN );
                redraw = true;
            }
            else if ( le.MouseClickLeft( cinfo2->rtAI ) && armyInfo[1].player.isControlHuman() ) {
                cinfo2->result = CONTROL_AI;
                armyInfo[1].player.SetControl( CONTROL_AI );
                redraw = true;
            }
        }

        if ( !redraw ) {
            continue;
        }

        RedrawBaseInfo( cur_pt );

        armyInfo[0].ui.redraw( display );
        armyInfo[1].ui.redraw( display );

        if ( cinfo2 ) {
            cinfo2->Redraw();
        }

        fheroes2::Blit( buttonOverride, display, cur_pt.x + 276, cur_pt.y + 428 );

        buttonOkay.draw();
        buttonCancel.draw();
        buttonReset.draw();
        display.render();

        redraw = false;
    }

    armyInfo[0].ui = {};
    armyInfo[1].ui = {};
    cinfo2.reset();

    return result;
}

void Battle::Only::updateHero( ArmyInfo & info, const fheroes2::Point & offset )
{
    info.ui = {};
    if ( info.hero == nullptr ) {
        return;
    }

    updateArmyUI( info.ui, info.hero, offset, info.armyId );
}

void Battle::Only::RedrawBaseInfo( const fheroes2::Point & top ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 ), display, top.x, top.y );

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
    text.draw( top.x + 320 - text.width() / 2, top.y + 29, display );

    for ( size_t idx : { 0, 1 } ) {
        if ( armyInfo[idx].hero ) {
            const fheroes2::Sprite & port1 = armyInfo[idx].hero->GetPortrait( PORT_BIG );
            if ( !port1.empty() )
                fheroes2::Blit( port1, display, armyInfo[idx].portraitRoi.x, armyInfo[idx].portraitRoi.y );
        }
        else {
            fheroes2::Fill( display, armyInfo[idx].portraitRoi.x, armyInfo[idx].portraitRoi.y, armyInfo[idx].portraitRoi.width, armyInfo[idx].portraitRoi.height, 0 );
            text.set( _( "N/A" ), fheroes2::FontType::normalWhite() );
            text.draw( armyInfo[idx].portraitRoi.x + ( armyInfo[idx].portraitRoi.width - text.width() ) / 2,
                       armyInfo[idx].portraitRoi.y + armyInfo[idx].portraitRoi.height / 2 - 8, display );
        }
    }

    fheroes2::RedrawPrimarySkillInfo( top, armyInfo[0].ui.primarySkill.get(), armyInfo[1].ui.primarySkill.get() );
}

void Battle::Only::StartBattle()
{
    assert( armyInfo[0].hero != nullptr );

    Settings & conf = Settings::Get();

    conf.GetPlayers().Init( armyInfo[0].player.GetColor() | armyInfo[1].player.GetColor() );
    world.InitKingdoms();

    conf.SetCurrentColor( armyInfo[0].player.GetColor() );

    for ( int32_t idx : { 0, 1 } ) {
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

    cinfo2.reset();
}

void Battle::Only::copyHero( Heroes & in, Heroes & out )
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
}

void Battle::Only::updateArmyUI( ArmyUI & ui, Heroes * hero, const fheroes2::Point & offset, const uint8_t armyId )
{
    assert( hero != nullptr );

    ui.morale = std::make_unique<MoraleIndicator>( hero );
    ui.morale->SetPos( { offset.x + moraleAndLuckOffsetX[armyId], offset.y + 75 } );

    ui.luck = std::make_unique<LuckIndicator>( hero );
    ui.luck->SetPos( { offset.x + moraleAndLuckOffsetX[armyId], offset.y + 115 } );

    ui.primarySkill = std::make_unique<PrimarySkillsBar>( hero, true );
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

void Battle::Only::ArmyInfo::reset()
{
    ui = {};
    hero = nullptr;

    monster.Reset();
    monster.GetTroop( 0 )->Set( Monster::PEASANT, 100 );
}
