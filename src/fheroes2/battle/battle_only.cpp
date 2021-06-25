/***************************************************************************
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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
#include "agg_image.h"
#include "army_bar.h"
#include "battle.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game.h"
#include "heroes.h"
#include "heroes_indicator.h"
#include "icn.h"
#include "race.h"
#include "settings.h"
#include "skill_bar.h"
#include "text.h"
#include "tools.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    const uint32_t primaryMaxValue = 20;
}

void Battle::ControlInfo::Redraw( void ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Sprite & cell = fheroes2::AGG::GetICN( ICN::CELLWIN, 1 );
    const fheroes2::Sprite & mark = fheroes2::AGG::GetICN( ICN::CELLWIN, 2 );

    fheroes2::Blit( cell, display, rtLocal.x, rtLocal.y );
    if ( result & CONTROL_HUMAN )
        fheroes2::Blit( mark, display, rtLocal.x + 3, rtLocal.y + 2 );
    Text text( "Human", Font::SMALL );
    text.Blit( rtLocal.x + cell.width() + 5, rtLocal.y + 5 );

    fheroes2::Blit( cell, display, rtAI.x, rtAI.y );
    if ( result & CONTROL_AI )
        fheroes2::Blit( mark, display, rtAI.x + 3, rtAI.y + 2 );
    text.Set( "AI" );
    text.Blit( rtAI.x + cell.width() + 5, rtAI.y + 5 );
}

Battle::Only::Only()
    : hero1( nullptr )
    , hero2( nullptr )
    , player1( Color::BLUE )
    , player2( Color::NONE )
    , army1( nullptr )
    , army2( nullptr )
    , moraleIndicator1( nullptr )
    , moraleIndicator2( nullptr )
    , luckIndicator1( nullptr )
    , luckIndicator2( nullptr )
    , primskill_bar1( nullptr )
    , primskill_bar2( nullptr )
    , secskill_bar1( nullptr )
    , secskill_bar2( nullptr )
    , selectArmy1( nullptr )
    , selectArmy2( nullptr )
    , selectArtifacts1( nullptr )
    , selectArtifacts2( nullptr )
    , cinfo2( nullptr )
{
    player1.SetControl( CONTROL_HUMAN );
    player2.SetControl( CONTROL_AI );
}

bool Battle::Only::ChangeSettings( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::StandardWindow frameborder( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );

    const fheroes2::Point cur_pt( frameborder.activeArea().x, frameborder.activeArea().y );

    rtPortrait1 = fheroes2::Rect( cur_pt.x + 93, cur_pt.y + 72, 101, 93 );
    rtPortrait2 = fheroes2::Rect( cur_pt.x + 445, cur_pt.y + 72, 101, 93 );

    const fheroes2::Rect rtAttack1( cur_pt.x + 215, cur_pt.y + 50, 33, 33 );
    const fheroes2::Rect rtAttack2( cur_pt.x + 390, cur_pt.y + 50, 33, 33 );

    const fheroes2::Rect rtDefense1( cur_pt.x + 215, cur_pt.y + 83, 33, 33 );
    const fheroes2::Rect rtDefense2( cur_pt.x + 390, cur_pt.y + 83, 33, 33 );

    const fheroes2::Rect rtPower1( cur_pt.x + 215, cur_pt.y + 116, 33, 33 );
    const fheroes2::Rect rtPower2( cur_pt.x + 390, cur_pt.y + 116, 33, 33 );

    const fheroes2::Rect rtKnowledge1( cur_pt.x + 215, cur_pt.y + 149, 33, 33 );
    const fheroes2::Rect rtKnowledge2( cur_pt.x + 390, cur_pt.y + 149, 33, 33 );

    if ( conf.IsGameType( Game::TYPE_NETWORK ) ) {
        player2.SetColor( Color::RED );

        player1.SetControl( CONTROL_REMOTE );
        player2.SetControl( CONTROL_REMOTE );

        hero2 = world.GetHeroes( Heroes::ZOM );
    }

    hero1 = world.GetHeroes( Heroes::LORDKILBURN );
    hero1->GetSecondarySkills().FillMax( Skill::Secondary() );
    army1 = &hero1->GetArmy();

    RedrawBaseInfo( cur_pt );

    UpdateHero1( cur_pt );

    moraleIndicator1->Redraw();
    luckIndicator1->Redraw();
    primskill_bar1->Redraw();
    secskill_bar1->Redraw();
    selectArtifacts1->Redraw();

    selectArmy1.reset( new ArmyBar( army1, true, false, true ) );
    selectArmy1->SetColRows( 5, 1 );
    selectArmy1->SetPos( cur_pt.x + 36, cur_pt.y + 267 );
    selectArmy1->SetHSpace( 2 );
    selectArmy1->Redraw();

    if ( hero2 ) {
        hero2->GetSecondarySkills().FillMax( Skill::Secondary() );
        UpdateHero2( cur_pt );

        moraleIndicator2->Redraw();
        luckIndicator2->Redraw();
        secskill_bar2->Redraw();
        selectArtifacts2->Redraw();
        selectArmy2->Redraw();
    }

    monsters.GetTroop( 0 )->Set( Monster::PEASANT, 100 );
    army2 = hero2 ? &hero2->GetArmy() : &monsters;

    selectArmy2.reset( new ArmyBar( army2, true, false, true ) );
    selectArmy2->SetColRows( 5, 1 );
    selectArmy2->SetPos( cur_pt.x + 381, cur_pt.y + 267 );
    selectArmy2->SetHSpace( 2 );
    selectArmy2->Redraw();

    bool exit = false;
    bool redraw = false;
    bool result = false;
    bool allow1 = true;
    bool allow2 = true;

    fheroes2::Button buttonStart( cur_pt.x + 280, cur_pt.y + 428, ICN::SYSTEM, 1, 2 );
    buttonStart.draw();

    display.render();

    // message loop
    while ( !exit && le.HandleEvents() ) {
        buttonStart.isEnabled() && le.MousePressLeft( buttonStart.area() ) ? buttonStart.drawOnPress() : buttonStart.drawOnRelease();

        if ( ( buttonStart.isEnabled() && le.MouseClickLeft( buttonStart.area() ) ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) ) {
            result = true;
            exit = true;
        }
        else if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) )
            exit = true;

        if ( allow1 && le.MouseClickLeft( rtPortrait1 ) ) {
            int hid = Dialog::SelectHeroes( hero1 ? hero1->GetID() : Heroes::UNKNOWN );
            if ( hero2 && hid == hero2->GetID() ) {
                Dialog::Message( "Error", "Please, select other hero.", Font::BIG, Dialog::OK );
            }
            else if ( Heroes::UNKNOWN != hid ) {
                hero1 = world.GetHeroes( hid );
                if ( hero1 )
                    hero1->GetSecondarySkills().FillMax( Skill::Secondary() );
                UpdateHero1( cur_pt );
                redraw = true;
            }
        }
        else if ( allow2 && le.MouseClickLeft( rtPortrait2 ) ) {
            int hid = Dialog::SelectHeroes( hero2 ? hero2->GetID() : Heroes::UNKNOWN );
            if ( hero1 && hid == hero1->GetID() ) {
                Dialog::Message( "Error", "Please, select other hero.", Font::BIG, Dialog::OK );
            }
            else if ( Heroes::UNKNOWN != hid ) {
                hero2 = world.GetHeroes( hid );
                if ( hero2 )
                    hero2->GetSecondarySkills().FillMax( Skill::Secondary() );
                UpdateHero2( cur_pt );
                if ( player2.isControlLocal() && nullptr == cinfo2 ) {
                    cinfo2.reset( new ControlInfo( fheroes2::Point( cur_pt.x + 500, cur_pt.y + 425 ), player2.GetControl() ) );
                }
                redraw = true;
            }
        }

        if ( hero1 && allow1 ) {
            if ( le.MouseClickLeft( rtAttack1 ) ) {
                u32 value = hero1->attack;
                if ( Dialog::SelectCount( "Set Attack Skill", 0, primaryMaxValue, value ) ) {
                    hero1->attack = value;
                    redraw = true;
                }
            }
            else if ( le.MouseClickLeft( rtDefense1 ) ) {
                u32 value = hero1->defense;
                if ( Dialog::SelectCount( "Set Defense Skill", 0, primaryMaxValue, value ) ) {
                    hero1->defense = value;
                    redraw = true;
                }
            }
            else if ( le.MouseClickLeft( rtPower1 ) ) {
                u32 value = hero1->power;
                if ( Dialog::SelectCount( "Set Power Skill", 0, primaryMaxValue, value ) ) {
                    hero1->power = value;
                    redraw = true;
                }
            }
            else if ( le.MouseClickLeft( rtKnowledge1 ) ) {
                u32 value = hero1->knowledge;
                if ( Dialog::SelectCount( "Set Knowledge Skill", 0, primaryMaxValue, value ) ) {
                    hero1->knowledge = value;
                    redraw = true;
                }
            }
        }

        if ( hero2 && allow2 ) {
            if ( le.MouseClickLeft( rtAttack2 ) ) {
                u32 value = hero2->attack;
                if ( Dialog::SelectCount( "Set Attack Skill", 0, primaryMaxValue, value ) ) {
                    hero2->attack = value;
                    redraw = true;
                }
            }
            else if ( le.MouseClickLeft( rtDefense2 ) ) {
                u32 value = hero2->defense;
                if ( Dialog::SelectCount( "Set Defense Skill", 0, primaryMaxValue, value ) ) {
                    hero2->defense = value;
                    redraw = true;
                }
            }
            else if ( le.MouseClickLeft( rtPower2 ) ) {
                u32 value = hero2->power;
                if ( Dialog::SelectCount( "Set Power Skill", 0, primaryMaxValue, value ) ) {
                    hero2->power = value;
                    redraw = true;
                }
            }
            else if ( le.MouseClickLeft( rtKnowledge2 ) ) {
                u32 value = hero2->knowledge;
                if ( Dialog::SelectCount( "Set Knowledge Skill", 0, primaryMaxValue, value ) ) {
                    hero2->knowledge = value;
                    redraw = true;
                }
            }
        }

        if ( allow1 && le.MouseCursor( selectArmy1->GetArea() ) && selectArmy1->QueueEventProcessing() ) {
            if ( selectArtifacts1->isSelected() )
                selectArtifacts1->ResetSelected();
            else if ( selectArtifacts2 && selectArtifacts2->isSelected() )
                selectArtifacts2->ResetSelected();

            if ( selectArmy2->isSelected() )
                selectArmy2->ResetSelected();

            redraw = true;
        }

        if ( allow2 && le.MouseCursor( selectArmy2->GetArea() ) && selectArmy2->QueueEventProcessing() ) {
            if ( selectArtifacts1->isSelected() )
                selectArtifacts1->ResetSelected();
            else if ( selectArtifacts2 && selectArtifacts2->isSelected() )
                selectArtifacts2->ResetSelected();

            if ( selectArmy1->isSelected() )
                selectArmy1->ResetSelected();

            redraw = true;
        }

        if ( allow1 && le.MouseCursor( selectArtifacts1->GetArea() ) && selectArtifacts1->QueueEventProcessing() ) {
            if ( selectArmy1->isSelected() )
                selectArmy1->ResetSelected();
            else if ( selectArmy2->isSelected() )
                selectArmy2->ResetSelected();

            if ( selectArtifacts2 && selectArtifacts2->isSelected() )
                selectArtifacts2->ResetSelected();

            redraw = true;
        }

        if ( allow2 && selectArtifacts2 && le.MouseCursor( selectArtifacts2->GetArea() ) && selectArtifacts2->QueueEventProcessing() ) {
            if ( selectArmy1->isSelected() )
                selectArmy1->ResetSelected();
            else if ( selectArmy2->isSelected() )
                selectArmy2->ResetSelected();

            if ( selectArtifacts1->isSelected() )
                selectArtifacts1->ResetSelected();

            redraw = true;
        }

        if ( hero1 && allow1 ) {
            if ( le.MouseCursor( moraleIndicator1->GetArea() ) )
                MoraleIndicator::QueueEventProcessing( *moraleIndicator1 );
            else if ( le.MouseCursor( luckIndicator1->GetArea() ) )
                LuckIndicator::QueueEventProcessing( *luckIndicator1 );
            else if ( le.MouseCursor( primskill_bar1->GetArea() ) && primskill_bar1->QueueEventProcessing() )
                redraw = true;
            else if ( le.MouseCursor( secskill_bar1->GetArea() ) && secskill_bar1->QueueEventProcessing() )
                redraw = true;
        }

        if ( hero2 && allow2 ) {
            if ( le.MouseCursor( moraleIndicator2->GetArea() ) )
                MoraleIndicator::QueueEventProcessing( *moraleIndicator2 );
            else if ( le.MouseCursor( luckIndicator2->GetArea() ) )
                LuckIndicator::QueueEventProcessing( *luckIndicator2 );
            else if ( le.MouseCursor( primskill_bar2->GetArea() ) && primskill_bar2->QueueEventProcessing() )
                redraw = true;
            else if ( le.MouseCursor( secskill_bar2->GetArea() ) && secskill_bar2->QueueEventProcessing() )
                redraw = true;
        }

        if ( cinfo2 && allow1 ) {
            if ( hero2 && le.MouseClickLeft( cinfo2->rtLocal ) && player2.isControlAI() ) {
                cinfo2->result = CONTROL_HUMAN;
                player2.SetControl( CONTROL_HUMAN );
                redraw = true;
            }
            else if ( le.MouseClickLeft( cinfo2->rtAI ) && player2.isControlHuman() ) {
                cinfo2->result = CONTROL_AI;
                player2.SetControl( CONTROL_AI );
                redraw = true;
            }
        }

        if ( !redraw ) {
            continue;
        }

        RedrawBaseInfo( cur_pt );
        moraleIndicator1->Redraw();
        luckIndicator1->Redraw();
        secskill_bar1->Redraw();
        selectArtifacts1->Redraw();
        selectArmy1->Redraw();

        if ( hero2 ) {
            moraleIndicator2->Redraw();
            luckIndicator2->Redraw();
            secskill_bar2->Redraw();
            selectArtifacts2->Redraw();
        }

        selectArmy2->Redraw();

        if ( cinfo2 ) {
            cinfo2->Redraw();
        }

        buttonStart.draw();
        display.render();

        redraw = false;
    }

    moraleIndicator1.reset();
    luckIndicator1.reset();
    primskill_bar1.reset();
    secskill_bar1.reset();
    selectArtifacts1.reset();
    selectArmy1.reset();

    moraleIndicator2.reset();
    luckIndicator2.reset();
    primskill_bar2.reset();
    secskill_bar2.reset();
    selectArtifacts2.reset();
    selectArmy2.reset();

    cinfo2.reset();

    return result;
}

void Battle::Only::UpdateHero1( const fheroes2::Point & cur_pt )
{
    primskill_bar1.reset();
    secskill_bar1.reset();
    selectArtifacts1.reset();
    selectArmy1.reset();

    if ( hero1 ) {
        player1.SetColor( Color::BLUE );
        player1.SetRace( hero1->GetRace() );

        if ( moraleIndicator1 == nullptr ) {
            moraleIndicator1.reset( new MoraleIndicator( hero1 ) );
            moraleIndicator1->SetPos( fheroes2::Point( cur_pt.x + 34, cur_pt.y + 75 ) );
        }
        else {
            moraleIndicator1->SetHero( hero1 );
        }

        if ( luckIndicator1 == nullptr ) {
            luckIndicator1.reset( new LuckIndicator( hero1 ) );
            luckIndicator1->SetPos( fheroes2::Point( cur_pt.x + 34, cur_pt.y + 115 ) );
        }
        else {
            luckIndicator1->SetHero( hero1 );
        }

        primskill_bar1.reset( new PrimarySkillsBar( hero1, true ) );
        primskill_bar1->SetColRows( 1, 4 );
        primskill_bar1->SetVSpace( -1 );
        primskill_bar1->SetTextOff( 70, -25 );
        primskill_bar1->SetPos( cur_pt.x + 216, cur_pt.y + 51 );

        secskill_bar1.reset( new SecondarySkillsBar( *hero1, true, true ) );
        secskill_bar1->SetColRows( 8, 1 );
        secskill_bar1->SetHSpace( -1 );
        secskill_bar1->SetContent( hero1->GetSecondarySkills().ToVector() );
        secskill_bar1->SetPos( cur_pt.x + 22, cur_pt.y + 199 );

        selectArtifacts1.reset( new ArtifactsBar( hero1, true, false, true, true, nullptr ) );
        selectArtifacts1->SetColRows( 7, 2 );
        selectArtifacts1->SetHSpace( 2 );
        selectArtifacts1->SetVSpace( 2 );
        selectArtifacts1->SetContent( hero1->GetBagArtifacts() );
        selectArtifacts1->SetPos( cur_pt.x + 23, cur_pt.y + 347 );

        army1 = &hero1->GetArmy();

        selectArmy1.reset( new ArmyBar( army1, true, false, true ) );
        selectArmy1->SetColRows( 5, 1 );
        selectArmy1->SetPos( cur_pt.x + 36, cur_pt.y + 267 );
        selectArmy1->SetHSpace( 2 );
    }
}

void Battle::Only::UpdateHero2( const fheroes2::Point & cur_pt )
{
    primskill_bar2.reset();
    secskill_bar2.reset();
    selectArtifacts2.reset();
    selectArmy2.reset();

    if ( hero2 ) {
        player2.SetColor( Color::RED );
        player2.SetRace( hero2->GetRace() );

        if ( moraleIndicator2 == nullptr ) {
            moraleIndicator2.reset( new MoraleIndicator( hero2 ) );
            moraleIndicator2->SetPos( fheroes2::Point( cur_pt.x + 566, cur_pt.y + 75 ) );
        }
        else {
            moraleIndicator2->SetHero( hero2 );
        }

        if ( luckIndicator2 == nullptr ) {
            luckIndicator2.reset( new LuckIndicator( hero2 ) );
            luckIndicator2->SetPos( fheroes2::Point( cur_pt.x + 566, cur_pt.y + 115 ) );
        }
        else {
            luckIndicator2->SetHero( hero2 );
        }

        primskill_bar2.reset( new PrimarySkillsBar( hero2, true ) );
        primskill_bar2->SetColRows( 1, 4 );
        primskill_bar2->SetVSpace( -1 );
        primskill_bar2->SetTextOff( -70, -25 );
        primskill_bar2->SetPos( cur_pt.x + 389, cur_pt.y + 51 );

        secskill_bar2.reset( new SecondarySkillsBar( *hero2, true, true ) );
        secskill_bar2->SetColRows( 8, 1 );
        secskill_bar2->SetHSpace( -1 );
        secskill_bar2->SetContent( hero2->GetSecondarySkills().ToVector() );
        secskill_bar2->SetPos( cur_pt.x + 353, cur_pt.y + 199 );

        selectArtifacts2.reset( new ArtifactsBar( hero2, true, false, true, true, nullptr ) );
        selectArtifacts2->SetColRows( 7, 2 );
        selectArtifacts2->SetHSpace( 2 );
        selectArtifacts2->SetVSpace( 2 );
        selectArtifacts2->SetContent( hero2->GetBagArtifacts() );
        selectArtifacts2->SetPos( cur_pt.x + 367, cur_pt.y + 347 );

        army2 = &hero2->GetArmy();

        selectArmy2.reset( new ArmyBar( army2, true, false, true ) );
        selectArmy2->SetColRows( 5, 1 );
        selectArmy2->SetPos( cur_pt.x + 381, cur_pt.y + 267 );
        selectArmy2->SetHSpace( 2 );
    }
}

void Battle::Only::RedrawBaseInfo( const fheroes2::Point & top ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 ), display, top.x, top.y );

    // header
    std::string message = "%{name1} vs %{name2}";

    StringReplace( message, "%{name1}", ( hero1 ? std::string( Race::String( hero1->GetRace() ) ) + " " + hero1->GetName() : "Monsters" ) );
    StringReplace( message, "%{name2}", ( hero2 ? std::string( Race::String( hero2->GetRace() ) ) + " " + hero2->GetName() : "Monsters" ) );

    Text text( message, Font::BIG );
    text.Blit( top.x + 320 - text.w() / 2, top.y + 26 );

    // portrait
    if ( hero1 ) {
        const fheroes2::Sprite & port1 = hero1->GetPortrait( PORT_BIG );
        if ( !port1.empty() )
            fheroes2::Blit( port1, display, rtPortrait1.x, rtPortrait1.y );
    }
    else {
        fheroes2::Fill( display, rtPortrait1.x, rtPortrait1.y, rtPortrait1.width, rtPortrait1.height, 0 );
        text.Set( "N/A", Font::BIG );
        text.Blit( rtPortrait1.x + ( rtPortrait1.width - text.w() ) / 2, rtPortrait1.y + rtPortrait1.height / 2 - 8 );
    }

    if ( hero2 ) {
        const fheroes2::Sprite & port2 = hero2->GetPortrait( PORT_BIG );
        if ( !port2.empty() )
            fheroes2::Blit( port2, display, rtPortrait2.x, rtPortrait2.y );
    }
    else {
        fheroes2::Fill( display, rtPortrait2.x, rtPortrait2.y, rtPortrait2.width, rtPortrait2.height, 0 );
        text.Set( "N/A", Font::BIG );
        text.Blit( rtPortrait2.x + ( rtPortrait2.width - text.w() ) / 2, rtPortrait2.y + rtPortrait2.height / 2 - 8 );
    }

    // primary skill
    fheroes2::RedrawPrimarySkillInfo( top, primskill_bar1.get(), primskill_bar2.get() );
}

void Battle::Only::StartBattle( void )
{
    Settings & conf = Settings::Get();

    conf.GetPlayers().Init( player1.GetColor() | player2.GetColor() );
    world.InitKingdoms();

    Players::SetPlayerRace( player1.GetColor(), player1.GetRace() );
    Players::SetPlayerRace( player2.GetColor(), player2.GetRace() );

    conf.SetCurrentColor( player1.GetColor() );

    Players::SetPlayerControl( player1.GetColor(), CONTROL_AI );
    Players::SetPlayerControl( player2.GetColor(), CONTROL_AI );

    if ( hero1 ) {
        hero1->SetSpellPoints( hero1->GetMaxSpellPoints() );
        hero1->Recruit( player1.GetColor(), fheroes2::Point( 5, 5 ) );

        if ( hero2 ) {
            hero2->SetSpellPoints( hero2->GetMaxSpellPoints() );
            hero2->Recruit( player2.GetColor(), fheroes2::Point( 5, 6 ) );
        }

        Players::SetPlayerControl( player1.GetColor(), player1.GetControl() );
        Players::SetPlayerControl( player2.GetColor(), player2.GetControl() );

        Battle::Loader( hero1->GetArmy(), ( hero2 ? hero2->GetArmy() : monsters ), hero1->GetIndex() + 1 );
    }
}
