/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <string>
#include <vector>

#include "agg.h"
#include "buildinginfo.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "heroes.h"
#include "kingdom.h"
#include "payment.h"
#include "race.h"
#include "settings.h"
#include "statusbar.h"
#include "text.h"
#include "ui_button.h"
#include "world.h"

int Castle::DialogBuyHero( const Heroes * hero )
{
    if ( !hero )
        return Dialog::CANCEL;

    const int system = ( Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM );

    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    const int spacer = 10;
    const fheroes2::Sprite & portrait_frame = fheroes2::AGG::GetICN( ICN::SURRENDR, 4 );

    TextBox recruitHeroText( _( "Recruit Hero" ), Font::YELLOW_BIG, BOXAREA_WIDTH );

    u32 count = hero->GetCountArtifacts();
    if ( hero->HasArtifact( Artifact::MAGIC_BOOK ) )
        count--;

    std::string str = _( "%{name} is a level %{value} %{race} " );

    // FIXME: It is necessary to consider locale features for numerals (with getext).
    if ( count ) {
        str += count > 1 ? _( "with %{count} artifacts." ) : _( "with 1 artifact." );
    }
    else {
        str += _( "without artifacts." );
    }

    StringReplace( str, "%{name}", hero->GetName() );
    StringReplace( str, "%{value}", hero->GetLevel() );
    StringReplace( str, "%{race}", Race::String( hero->GetRace() ) );
    StringReplace( str, "%{count}", count );

    TextBox heroDescriptionText( str, Font::BIG, BOXAREA_WIDTH );

    Resource::BoxSprite rbs( PaymentConditions::RecruitHero( hero->GetLevel() ), BOXAREA_WIDTH );

    Dialog::FrameBox box( recruitHeroText.h() + spacer + portrait_frame.height() + spacer + heroDescriptionText.h() + spacer + rbs.GetArea().h, true );
    const fheroes2::Rect & box_rt = box.GetArea();
    LocalEvent & le = LocalEvent::Get();
    fheroes2::Point dst_pt;

    dst_pt.x = box_rt.x + ( box_rt.width - recruitHeroText.w() ) / 2;
    dst_pt.y = box_rt.y;
    recruitHeroText.Blit( dst_pt.x, dst_pt.y );

    // portrait and frame
    dst_pt.x = box_rt.x + ( box_rt.width - portrait_frame.width() ) / 2;
    dst_pt.y = dst_pt.y + recruitHeroText.h() + spacer;
    fheroes2::Blit( portrait_frame, display, dst_pt.x, dst_pt.y );

    dst_pt.x = dst_pt.x + 5;
    dst_pt.y = dst_pt.y + 6;
    hero->PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );

    dst_pt.x = box_rt.x;
    dst_pt.y = dst_pt.y + portrait_frame.height() + spacer;
    heroDescriptionText.Blit( dst_pt.x, dst_pt.y );

    rbs.SetPos( dst_pt.x, dst_pt.y + heroDescriptionText.h() + spacer );
    rbs.Redraw();

    dst_pt.x = box_rt.x;
    dst_pt.y = box_rt.y + box_rt.height - fheroes2::AGG::GetICN( system, 1 ).height();
    fheroes2::Button button1( dst_pt.x, dst_pt.y, system, 1, 2 );

    if ( !AllowBuyHero( *hero ) ) {
        button1.disable();
    }

    dst_pt.x = box_rt.x + box_rt.width - fheroes2::AGG::GetICN( system, 3 ).width();
    dst_pt.y = box_rt.y + box_rt.height - fheroes2::AGG::GetICN( system, 3 ).height();
    fheroes2::Button button2( dst_pt.x, dst_pt.y, system, 3, 4 );

    button1.draw();
    button2.draw();

    cursor.Show();
    display.render();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( button1.area() ) ? button1.drawOnPress() : button1.drawOnRelease();
        le.MousePressLeft( button2.area() ) ? button2.drawOnPress() : button2.drawOnRelease();

        if ( button1.isEnabled() && ( le.MouseClickLeft( button1.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) ) )
            return Dialog::OK;

        if ( le.MouseClickLeft( button2.area() ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) )
            break;
    }

    return Dialog::CANCEL;
}

int Castle::DialogBuyCastle( bool buttons ) const
{
    BuildingInfo info( *this, BUILD_CASTLE );
    return info.DialogBuyBuilding( buttons ) ? Dialog::OK : Dialog::CANCEL;
}

u32 Castle::OpenTown( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    Dialog::FrameBorder background( Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) );

    const Point & cur_pt = background.GetArea();
    Point dst_pt( cur_pt );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CASLWIND, 0 ), display, dst_pt.x, dst_pt.y );

    // hide captain options
    if ( !( building & BUILD_CAPTAIN ) ) {
        dst_pt.x = 530;
        dst_pt.y = 163;
        const Rect rect( dst_pt, 110, 84 );
        dst_pt.x += cur_pt.x;
        dst_pt.y += cur_pt.y;

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STONEBAK, 0 ), rect.x, rect.y, display, dst_pt.x, dst_pt.y, rect.w, rect.h );
    }

    // draw castle sprite
    dst_pt.x = cur_pt.x + 460;
    dst_pt.y = cur_pt.y + 5;
    DrawImageCastle( dst_pt );

    // castle name
    Text text( GetName(), Font::SMALL );
    text.Blit( cur_pt.x + 536 - text.w() / 2, cur_pt.y + 1 );

    //
    BuildingInfo dwelling1( *this, DWELLING_MONSTER1 );
    dwelling1.SetPos( cur_pt.x + 5, cur_pt.y + 2 );
    dwelling1.Redraw();

    BuildingInfo dwelling2( *this, DWELLING_MONSTER2 );
    dwelling2.SetPos( cur_pt.x + 149, cur_pt.y + 2 );
    dwelling2.Redraw();

    BuildingInfo dwelling3( *this, DWELLING_MONSTER3 );
    dwelling3.SetPos( cur_pt.x + 293, cur_pt.y + 2 );
    dwelling3.Redraw();

    BuildingInfo dwelling4( *this, DWELLING_MONSTER4 );
    dwelling4.SetPos( cur_pt.x + 5, cur_pt.y + 77 );
    dwelling4.Redraw();

    BuildingInfo dwelling5( *this, DWELLING_MONSTER5 );
    dwelling5.SetPos( cur_pt.x + 149, cur_pt.y + 77 );
    dwelling5.Redraw();

    BuildingInfo dwelling6( *this, DWELLING_MONSTER6 );
    dwelling6.SetPos( cur_pt.x + 293, cur_pt.y + 77 );
    dwelling6.Redraw();

    // mage guild
    building_t level = BUILD_NOTHING;
    switch ( GetLevelMageGuild() ) {
    case 0:
        level = BUILD_MAGEGUILD1;
        break;
    case 1:
        level = BUILD_MAGEGUILD2;
        break;
    case 2:
        level = BUILD_MAGEGUILD3;
        break;
    case 3:
        level = BUILD_MAGEGUILD4;
        break;
    default:
        level = BUILD_MAGEGUILD5;
        break;
    }
    BuildingInfo buildingMageGuild( *this, level );
    buildingMageGuild.SetPos( cur_pt.x + 5, cur_pt.y + 157 );
    buildingMageGuild.Redraw();

    // tavern
    const bool isSkipTavernInteraction = ( Race::NECR == race ) && !Settings::Get().PriceLoyaltyVersion();
    BuildingInfo buildingTavern( *this, BUILD_TAVERN );
    buildingTavern.SetPos( cur_pt.x + 149, cur_pt.y + 157 );
    buildingTavern.Redraw();

    // thieves guild
    BuildingInfo buildingThievesGuild( *this, BUILD_THIEVESGUILD );
    buildingThievesGuild.SetPos( cur_pt.x + 293, cur_pt.y + 157 );
    buildingThievesGuild.Redraw();

    // shipyard
    BuildingInfo buildingShipyard( *this, BUILD_SHIPYARD );
    buildingShipyard.SetPos( cur_pt.x + 5, cur_pt.y + 232 );
    buildingShipyard.Redraw();

    // statue
    BuildingInfo buildingStatue( *this, BUILD_STATUE );
    buildingStatue.SetPos( cur_pt.x + 149, cur_pt.y + 232 );
    buildingStatue.Redraw();

    // marketplace
    BuildingInfo buildingMarketplace( *this, BUILD_MARKETPLACE );
    buildingMarketplace.SetPos( cur_pt.x + 293, cur_pt.y + 232 );
    buildingMarketplace.Redraw();

    // well
    BuildingInfo buildingWell( *this, BUILD_WELL );
    buildingWell.SetPos( cur_pt.x + 5, cur_pt.y + 307 );
    buildingWell.Redraw();

    // wel2
    BuildingInfo buildingWel2( *this, BUILD_WEL2 );
    buildingWel2.SetPos( cur_pt.x + 149, cur_pt.y + 307 );
    buildingWel2.Redraw();

    // spec
    BuildingInfo buildingSpec( *this, BUILD_SPEC );
    buildingSpec.SetPos( cur_pt.x + 293, cur_pt.y + 307 );
    buildingSpec.Redraw();

    // left turret
    BuildingInfo buildingLTurret( *this, BUILD_LEFTTURRET );
    buildingLTurret.SetPos( cur_pt.x + 5, cur_pt.y + 387 );
    buildingLTurret.Redraw();

    // right turret
    BuildingInfo buildingRTurret( *this, BUILD_RIGHTTURRET );
    buildingRTurret.SetPos( cur_pt.x + 149, cur_pt.y + 387 );
    buildingRTurret.Redraw();

    // moat
    BuildingInfo buildingMoat( *this, BUILD_MOAT );
    buildingMoat.SetPos( cur_pt.x + 293, cur_pt.y + 387 );
    buildingMoat.Redraw();

    // captain
    BuildingInfo buildingCaptain( *this, BUILD_CAPTAIN );
    buildingCaptain.SetPos( cur_pt.x + 444, cur_pt.y + 165 );
    buildingCaptain.Redraw();

    // combat format
    const fheroes2::Sprite & spriteSpreadArmyFormat = fheroes2::AGG::GetICN( ICN::HSICONS, 9 );
    const fheroes2::Sprite & spriteGroupedArmyFormat = fheroes2::AGG::GetICN( ICN::HSICONS, 10 );
    const fheroes2::Rect rectSpreadArmyFormat( cur_pt.x + 550, cur_pt.y + 220, spriteSpreadArmyFormat.width(), spriteSpreadArmyFormat.height() );
    const fheroes2::Rect rectGroupedArmyFormat( cur_pt.x + 585, cur_pt.y + 220, spriteGroupedArmyFormat.width(), spriteGroupedArmyFormat.height() );
    const std::string descriptionSpreadArmyFormat(
        _( "'Spread' combat formation spreads your armies from the top to the bottom of the battlefield, with at least one empty space between each army." ) );
    const std::string descriptionGroupedArmyFormat( _( "'Grouped' combat formation bunches your army toget her in the center of your side of the battlefield." ) );
    const fheroes2::Point pointSpreadArmyFormat( rectSpreadArmyFormat.x - 1, rectSpreadArmyFormat.y - 1 );
    const fheroes2::Point pointGroupedArmyFormat( rectGroupedArmyFormat.x - 1, rectGroupedArmyFormat.y - 1 );

    fheroes2::MovableSprite cursorFormat( fheroes2::AGG::GetICN( ICN::HSICONS, 11 ) );

    if ( isBuild( BUILD_CAPTAIN ) ) {
        text.Set( _( "Attack Skill" ) + std::string( " " ), Font::SMALL );
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y = cur_pt.y + 168;
        text.Blit( dst_pt );

        text.Set( GetString( captain.GetAttack() ) );
        dst_pt.x += 90;
        text.Blit( dst_pt );

        text.Set( _( "Defense Skill" ) + std::string( " " ) );
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y += 12;
        text.Blit( dst_pt );

        text.Set( GetString( captain.GetDefense() ) );
        dst_pt.x += 90;
        text.Blit( dst_pt );

        text.Set( _( "Spell Power" ) + std::string( " " ) );
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y += 12;
        text.Blit( dst_pt );

        text.Set( GetString( captain.GetPower() ) );
        dst_pt.x += 90;
        text.Blit( dst_pt );

        text.Set( _( "Knowledge" ) + std::string( " " ) );
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y += 12;
        text.Blit( dst_pt );

        text.Set( GetString( captain.GetKnowledge() ) );
        dst_pt.x += 90;
        text.Blit( dst_pt );

        fheroes2::Blit( spriteSpreadArmyFormat, display, rectSpreadArmyFormat.x, rectSpreadArmyFormat.y );
        fheroes2::Blit( spriteGroupedArmyFormat, display, rectGroupedArmyFormat.x, rectGroupedArmyFormat.y );

        if ( army.isSpreadFormat() )
            cursorFormat.setPosition( pointSpreadArmyFormat.x, pointSpreadArmyFormat.y );
        else
            cursorFormat.setPosition( pointGroupedArmyFormat.x, pointGroupedArmyFormat.y );
    }

    Kingdom & kingdom = GetKingdom();

    Heroes * hero1 = kingdom.GetRecruits().GetHero1();
    Heroes * hero2 = kingdom.GetLastLostHero() && kingdom.GetLastLostHero() != hero1 ? kingdom.GetLastLostHero() : kingdom.GetRecruits().GetHero2();

    std::string not_allow1_msg, not_allow2_msg;
    const bool allow_buy_hero1 = hero1 ? AllowBuyHero( *hero1, &not_allow1_msg ) : false;
    const bool allow_buy_hero2 = hero2 ? AllowBuyHero( *hero2, &not_allow2_msg ) : false;

    // first hero
    dst_pt.x = cur_pt.x + 443;
    dst_pt.y = cur_pt.y + 260;
    const Rect rectHero1( dst_pt, 102, 93 );

    fheroes2::Image noHeroPortrait( rectHero1.w, rectHero1.h );
    noHeroPortrait.fill( 0 );

    if ( hero1 ) {
        hero1->PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );
    }
    else {
        fheroes2::Blit( noHeroPortrait, display, rectHero1.x, rectHero1.y );
    }

    // indicator
    if ( !allow_buy_hero1 ) {
        dst_pt.x += 83;
        dst_pt.y += 75;
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::TOWNWIND, 12 ), display, dst_pt.x, dst_pt.y );
    }

    // second hero
    dst_pt.x = cur_pt.x + 443;
    dst_pt.y = cur_pt.y + 363;
    const Rect rectHero2( dst_pt, 102, 94 );
    if ( hero2 ) {
        hero2->PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );
    }
    else {
        fheroes2::Blit( noHeroPortrait, display, rectHero2.x, rectHero2.y );
    }

    // indicator
    if ( !allow_buy_hero2 ) {
        dst_pt.x += 83;
        dst_pt.y += 75;
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::TOWNWIND, 12 ), display, dst_pt.x, dst_pt.y );
    }

    // bottom bar
    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y + 461;
    const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( ICN::CASLBAR, 0 );
    fheroes2::Blit( bar, display, dst_pt.x, dst_pt.y );

    StatusBar statusBar;
    statusBar.SetCenter( dst_pt.x + bar.width() / 2, dst_pt.y + 12 );

    // button exit
    dst_pt.x = cur_pt.x + 553;
    dst_pt.y = cur_pt.y + 428;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::TREASURY, 1, 2 );
    buttonExit.draw();

    // redraw resource panel
    const Rect & rectResource = RedrawResourcePanel( cur_pt );
    const fheroes2::Rect resActiveArea( rectResource.x, rectResource.y, rectResource.w, buttonExit.area().y - rectResource.y );

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            break;

        if ( le.MouseClickLeft( resActiveArea ) ) {
            fheroes2::ButtonRestorer exitRestorer( buttonExit );
            Dialog::ResourceInfo( _( "Income" ), "", world.GetKingdom( GetColor() ).GetIncome( INCOME_ALL ), Dialog::OK );
        }
        else if ( le.MousePressRight( resActiveArea ) ) {
            Dialog::ResourceInfo( _( "Income" ), "", world.GetKingdom( GetColor() ).GetIncome( INCOME_ALL ), 0 );
        }

        // click left
        if ( le.MouseCursor( dwelling1.GetArea() ) && dwelling1.QueueEventProcessing( buttonExit ) )
            return dwelling1();
        else if ( le.MouseCursor( dwelling2.GetArea() ) && dwelling2.QueueEventProcessing( buttonExit ) )
            return dwelling2();
        else if ( le.MouseCursor( dwelling3.GetArea() ) && dwelling3.QueueEventProcessing( buttonExit ) )
            return dwelling3();
        else if ( le.MouseCursor( dwelling4.GetArea() ) && dwelling4.QueueEventProcessing( buttonExit ) )
            return dwelling4();
        else if ( le.MouseCursor( dwelling5.GetArea() ) && dwelling5.QueueEventProcessing( buttonExit ) )
            return dwelling5();
        else if ( le.MouseCursor( dwelling6.GetArea() ) && dwelling6.QueueEventProcessing( buttonExit ) )
            return dwelling6();
        else if ( le.MouseCursor( buildingMageGuild.GetArea() ) && buildingMageGuild.QueueEventProcessing( buttonExit ) )
            return buildingMageGuild();
        else if ( !isSkipTavernInteraction && le.MouseCursor( buildingTavern.GetArea() ) && buildingTavern.QueueEventProcessing( buttonExit ) )
            return ( Race::NECR == race ? BUILD_SHRINE : BUILD_TAVERN );
        else if ( le.MouseCursor( buildingThievesGuild.GetArea() ) && buildingThievesGuild.QueueEventProcessing( buttonExit ) )
            return BUILD_THIEVESGUILD;
        else if ( le.MouseCursor( buildingShipyard.GetArea() ) && buildingShipyard.QueueEventProcessing( buttonExit ) )
            return BUILD_SHIPYARD;
        else if ( le.MouseCursor( buildingStatue.GetArea() ) && buildingStatue.QueueEventProcessing( buttonExit ) )
            return BUILD_STATUE;
        else if ( le.MouseCursor( buildingMarketplace.GetArea() ) && buildingMarketplace.QueueEventProcessing( buttonExit ) )
            return BUILD_MARKETPLACE;
        else if ( le.MouseCursor( buildingWell.GetArea() ) && buildingWell.QueueEventProcessing( buttonExit ) )
            return BUILD_WELL;
        else if ( le.MouseCursor( buildingWel2.GetArea() ) && buildingWel2.QueueEventProcessing( buttonExit ) )
            return BUILD_WEL2;
        else if ( le.MouseCursor( buildingSpec.GetArea() ) && buildingSpec.QueueEventProcessing( buttonExit ) )
            return BUILD_SPEC;
        else if ( le.MouseCursor( buildingLTurret.GetArea() ) && buildingLTurret.QueueEventProcessing( buttonExit ) )
            return BUILD_LEFTTURRET;
        else if ( le.MouseCursor( buildingRTurret.GetArea() ) && buildingRTurret.QueueEventProcessing( buttonExit ) )
            return BUILD_RIGHTTURRET;
        else if ( le.MouseCursor( buildingMoat.GetArea() ) && buildingMoat.QueueEventProcessing( buttonExit ) )
            return BUILD_MOAT;
        else if ( le.MouseCursor( buildingCaptain.GetArea() ) && buildingCaptain.QueueEventProcessing( buttonExit ) )
            return BUILD_CAPTAIN;
        else if ( hero1 && le.MouseClickLeft( rectHero1 ) ) {
            fheroes2::ButtonRestorer exitRestorer( buttonExit );
            if ( Dialog::OK == DialogBuyHero( hero1 ) ) {
                RecruitHero( hero1 );

                return BUILD_NOTHING;
            }
        }
        else if ( hero2 && le.MouseClickLeft( rectHero2 ) ) {
            fheroes2::ButtonRestorer exitRestorer( buttonExit );
            if ( Dialog::OK == DialogBuyHero( hero2 ) ) {
                RecruitHero( hero2 );

                return BUILD_NOTHING;
            }
        }
        else if ( isBuild( BUILD_CAPTAIN ) ) {
            if ( le.MouseClickLeft( rectSpreadArmyFormat ) && !army.isSpreadFormat() ) {
                cursor.Hide();
                cursorFormat.setPosition( pointSpreadArmyFormat.x, pointSpreadArmyFormat.y );
                cursor.Show();
                display.render();
                army.SetSpreadFormat( true );
            }
            else if ( le.MouseClickLeft( rectGroupedArmyFormat ) && army.isSpreadFormat() ) {
                cursor.Hide();
                cursorFormat.setPosition( pointGroupedArmyFormat.x, pointGroupedArmyFormat.y );
                cursor.Show();
                display.render();
                army.SetSpreadFormat( false );
            }
        }

        const bool isCaptainBuilt = isBuild( BUILD_CAPTAIN );

        // Right click
        if ( le.MousePressRight( rectSpreadArmyFormat ) && isCaptainBuilt )
            Dialog::Message( _( "Spread Formation" ), descriptionSpreadArmyFormat, Font::BIG );
        else if ( le.MousePressRight( rectGroupedArmyFormat ) && isCaptainBuilt )
            Dialog::Message( _( "Grouped Formation" ), descriptionGroupedArmyFormat, Font::BIG );
        else if ( hero1 && le.MousePressRight( rectHero1 ) ) {
            hero1->OpenDialog( true );
            cursor.Show();
            display.render();
        }
        else if ( hero2 && le.MousePressRight( rectHero2 ) ) {
            hero2->OpenDialog( true );
            cursor.Show();
            display.render();
        }

        // status info
        if ( le.MouseCursor( dwelling1.GetArea() ) )
            dwelling1.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( dwelling2.GetArea() ) )
            dwelling2.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( dwelling3.GetArea() ) )
            dwelling3.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( dwelling4.GetArea() ) )
            dwelling4.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( dwelling5.GetArea() ) )
            dwelling5.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( dwelling6.GetArea() ) )
            dwelling6.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingMageGuild.GetArea() ) )
            buildingMageGuild.SetStatusMessage( statusBar );
        else if ( !isSkipTavernInteraction && le.MouseCursor( buildingTavern.GetArea() ) )
            buildingTavern.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingThievesGuild.GetArea() ) )
            buildingThievesGuild.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingShipyard.GetArea() ) )
            buildingShipyard.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingStatue.GetArea() ) )
            buildingStatue.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingMarketplace.GetArea() ) )
            buildingMarketplace.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingWell.GetArea() ) )
            buildingWell.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingWel2.GetArea() ) )
            buildingWel2.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingSpec.GetArea() ) )
            buildingSpec.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingLTurret.GetArea() ) )
            buildingLTurret.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingRTurret.GetArea() ) )
            buildingRTurret.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingMoat.GetArea() ) )
            buildingMoat.SetStatusMessage( statusBar );
        else if ( le.MouseCursor( buildingCaptain.GetArea() ) )
            buildingCaptain.SetStatusMessage( statusBar );
        else if ( hero1 && le.MouseCursor( rectHero1 ) ) {
            if ( !allow_buy_hero1 )
                statusBar.ShowMessage( not_allow1_msg );
            else {
                std::string str = _( "Recruit %{name} the %{race}" );
                StringReplace( str, "%{name}", hero1->GetName() );
                StringReplace( str, "%{race}", Race::String( hero1->GetRace() ) );
                statusBar.ShowMessage( str );
            }
        }
        else if ( hero2 && le.MouseCursor( rectHero2 ) ) {
            if ( !allow_buy_hero2 )
                statusBar.ShowMessage( not_allow2_msg );
            else {
                std::string str = _( "Recruit %{name} the %{race}" );
                StringReplace( str, "%{name}", hero2->GetName() );
                StringReplace( str, "%{race}", Race::String( hero2->GetRace() ) );
                statusBar.ShowMessage( str );
            }
        }
        else if ( le.MouseCursor( rectSpreadArmyFormat ) && isCaptainBuilt )
            statusBar.ShowMessage( _( "Set garrison combat formation to 'Spread'" ) );
        else if ( le.MouseCursor( rectGroupedArmyFormat ) && isCaptainBuilt )
            statusBar.ShowMessage( _( "Set garrison combat formation to 'Grouped'" ) );
        else if ( le.MouseCursor( buttonExit.area() ) )
            statusBar.ShowMessage( _( "Exit Castle Options" ) );
        else if ( le.MouseCursor( resActiveArea ) )
            statusBar.ShowMessage( _( "Show Income" ) );
        else
            // clear all
            statusBar.ShowMessage( _( "Castle Options" ) );
    }

    return BUILD_NOTHING;
}
