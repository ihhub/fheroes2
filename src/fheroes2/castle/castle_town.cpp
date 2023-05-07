/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <cstdint>
#include <functional>
#include <string>

#include "agg_image.h"
#include "army.h"
#include "artifact.h"
#include "buildinginfo.h"
#include "captain.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "heroes.h"
#include "heroes_base.h"
#include "heroes_recruits.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "localevent.h"
#include "math_base.h"
#include "payment.h"
#include "race.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "statusbar.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_castle.h"
#include "ui_kingdom.h"
#include "ui_tool.h"
#include "world.h"

int Castle::DialogBuyHero( const Heroes * hero ) const
{
    if ( !hero )
        return Dialog::CANCEL;

    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const int spacer = 10;
    const fheroes2::Sprite & portrait_frame = fheroes2::AGG::GetICN( ICN::SURRENDR, 4 );

    TextBox recruitHeroText( _( "Recruit Hero" ), Font::YELLOW_BIG, BOXAREA_WIDTH );

    uint32_t count = hero->GetCountArtifacts();
    if ( hero->hasArtifact( Artifact::MAGIC_BOOK ) )
        --count;

    std::string str = _( "%{name} is a level %{value} %{race} " );

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

    Resource::BoxSprite rbs( PaymentConditions::RecruitHero(), BOXAREA_WIDTH );

    Dialog::FrameBox box( recruitHeroText.h() + spacer + portrait_frame.height() + spacer + heroDescriptionText.h() + spacer + rbs.GetArea().height, true );
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

    const fheroes2::Rect heroPortraitArea( dst_pt.x, dst_pt.y, portrait_frame.width(), portrait_frame.height() );
    dst_pt.x = dst_pt.x + 5;
    dst_pt.y = dst_pt.y + 6;
    hero->PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );

    dst_pt.x = box_rt.x;
    dst_pt.y = dst_pt.y + portrait_frame.height() + spacer;
    heroDescriptionText.Blit( dst_pt.x, dst_pt.y );

    rbs.SetPos( dst_pt.x, dst_pt.y + heroDescriptionText.h() + spacer );
    rbs.Redraw();

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int okayButtonIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_OKAY_BUTTON : ICN::UNIFORM_GOOD_OKAY_BUTTON;

    dst_pt.x = box_rt.x;
    dst_pt.y = box_rt.y + box_rt.height - fheroes2::AGG::GetICN( okayButtonIcnID, 0 ).height();
    fheroes2::Button button1( dst_pt.x, dst_pt.y, okayButtonIcnID, 0, 1 );

    if ( !AllowBuyHero() ) {
        button1.disable();
    }

    const int cancelButtonIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_CANCEL_BUTTON : ICN::UNIFORM_GOOD_CANCEL_BUTTON;

    dst_pt.x = box_rt.x + box_rt.width - fheroes2::AGG::GetICN( cancelButtonIcnID, 0 ).width();
    dst_pt.y = box_rt.y + box_rt.height - fheroes2::AGG::GetICN( cancelButtonIcnID, 0 ).height();
    fheroes2::Button button2( dst_pt.x, dst_pt.y, cancelButtonIcnID, 0, 1 );

    button1.draw();
    button2.draw();

    display.render();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( button1.area() ) ? button1.drawOnPress() : button1.drawOnRelease();
        le.MousePressLeft( button2.area() ) ? button2.drawOnPress() : button2.drawOnRelease();

        if ( button1.isEnabled() && ( le.MouseClickLeft( button1.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) )
            return Dialog::OK;

        if ( le.MouseClickLeft( button2.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) )
            break;

        if ( le.MousePressRight( heroPortraitArea ) ) {
            Dialog::QuickInfo( *hero );
        }
    }

    return Dialog::CANCEL;
}

int Castle::DialogBuyCastle( bool buttons ) const
{
    BuildingInfo info( *this, BUILD_CASTLE );
    return info.DialogBuyBuilding( buttons ) ? Dialog::OK : Dialog::CANCEL;
}

Castle::ConstructionDialogResult Castle::openConstructionDialog( uint32_t & dwellingTobuild )
{
    if ( !isBuild( BUILD_CASTLE ) ) {
        // It is not possible to open this dialog without a built castle!
        assert( 0 );
        return ConstructionDialogResult::DoNothing;
    }

    dwellingTobuild = BUILD_NOTHING;

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::ImageRestorer restorer( display, ( display.width() - fheroes2::Display::DEFAULT_WIDTH ) / 2,
                                            ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2, fheroes2::Display::DEFAULT_WIDTH,
                                            fheroes2::Display::DEFAULT_HEIGHT );

    const fheroes2::Point cur_pt( restorer.x(), restorer.y() );
    fheroes2::Point dst_pt( cur_pt );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    fheroes2::Blit( fheroes2::AGG::GetICN( isEvilInterface ? ICN::CASLWIND_EVIL : ICN::CASLWIND, 0 ), display, dst_pt.x, dst_pt.y );

    // hide captain options
    if ( !( building & BUILD_CAPTAIN ) ) {
        dst_pt.x = 530;
        dst_pt.y = 163;
        const fheroes2::Rect rect( dst_pt.x, dst_pt.y, 110, 84 );
        dst_pt.x += cur_pt.x;
        dst_pt.y += cur_pt.y;

        const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 );
        fheroes2::Blit( backgroundImage, rect.x, rect.y, display, dst_pt.x, dst_pt.y, rect.width, rect.height );
    }

    // draw castle sprite
    dst_pt.x = cur_pt.x + 459;
    dst_pt.y = cur_pt.y + 5;
    DrawImageCastle( dst_pt );

    // castle name
    Text text( GetName(), Font::SMALL );
    text.Blit( cur_pt.x + 538 - text.w() / 2, cur_pt.y + 1 );

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
    const bool isSkipTavernInteraction = ( Race::NECR == race ) && !Settings::Get().isCurrentMapPriceOfLoyalty();
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
    const std::string descriptionGroupedArmyFormat( _( "'Grouped' combat formation bunches your army together in the center of your side of the battlefield." ) );
    const fheroes2::Point pointSpreadArmyFormat( rectSpreadArmyFormat.x - 1, rectSpreadArmyFormat.y - 1 );
    const fheroes2::Point pointGroupedArmyFormat( rectGroupedArmyFormat.x - 1, rectGroupedArmyFormat.y - 1 );

    fheroes2::MovableSprite cursorFormat( fheroes2::AGG::GetICN( ICN::HSICONS, 11 ) );

    if ( isBuild( BUILD_CAPTAIN ) ) {
        text.Set( Skill::Primary::String( Skill::Primary::ATTACK ) + std::string( " " ), Font::SMALL );
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y = cur_pt.y + 168;
        text.Blit( dst_pt );

        text.Set( std::to_string( captain.GetAttack() ) );
        dst_pt.x += 90;
        text.Blit( dst_pt );

        text.Set( Skill::Primary::String( Skill::Primary::DEFENSE ) + std::string( " " ) );
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y += 12;
        text.Blit( dst_pt );

        text.Set( std::to_string( captain.GetDefense() ) );
        dst_pt.x += 90;
        text.Blit( dst_pt );

        text.Set( Skill::Primary::String( Skill::Primary::POWER ) + std::string( " " ) );
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y += 12;
        text.Blit( dst_pt );

        text.Set( std::to_string( captain.GetPower() ) );
        dst_pt.x += 90;
        text.Blit( dst_pt );

        text.Set( Skill::Primary::String( Skill::Primary::KNOWLEDGE ) + std::string( " " ) );
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y += 12;
        text.Blit( dst_pt );

        text.Set( std::to_string( captain.GetKnowledge() ) );
        dst_pt.x += 90;
        text.Blit( dst_pt );

        fheroes2::Blit( spriteSpreadArmyFormat, display, rectSpreadArmyFormat.x, rectSpreadArmyFormat.y );
        fheroes2::Blit( spriteGroupedArmyFormat, display, rectGroupedArmyFormat.x, rectGroupedArmyFormat.y );

        if ( army.isSpreadFormation() )
            cursorFormat.setPosition( pointSpreadArmyFormat.x, pointSpreadArmyFormat.y );
        else
            cursorFormat.setPosition( pointGroupedArmyFormat.x, pointGroupedArmyFormat.y );
    }

    Kingdom & kingdom = GetKingdom();

    Heroes * hero1 = kingdom.GetRecruits().GetHero1();
    Heroes * hero2 = kingdom.GetRecruits().GetHero2();

    std::string not_allow1_msg;
    std::string not_allow2_msg;
    const bool allow_buy_hero1 = hero1 ? AllowBuyHero( &not_allow1_msg ) : false;
    const bool allow_buy_hero2 = hero2 ? AllowBuyHero( &not_allow2_msg ) : false;

    // first hero
    dst_pt.x = cur_pt.x + 443;
    dst_pt.y = cur_pt.y + 260;
    const fheroes2::Rect rectHero1( dst_pt.x, dst_pt.y, 102, 93 );

    fheroes2::Image noHeroPortrait( rectHero1.width, rectHero1.height );
    noHeroPortrait.fill( 0 );

    if ( hero1 ) {
        hero1->PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );
    }
    else {
        fheroes2::Blit( noHeroPortrait, display, rectHero1.x, rectHero1.y );
    }

    // indicator
    if ( !allow_buy_hero1 ) {
        const fheroes2::Sprite & spriteDeny = fheroes2::AGG::GetICN( ICN::TOWNWIND, 12 );
        fheroes2::Blit( spriteDeny, display, dst_pt.x + 102 - 4 + 1 - spriteDeny.width(), dst_pt.y + 93 - 2 - spriteDeny.height() );
    }

    // second hero
    dst_pt.x = cur_pt.x + 443;
    dst_pt.y = cur_pt.y + 363;
    const fheroes2::Rect rectHero2( dst_pt.x, dst_pt.y, 102, 94 );
    if ( hero2 ) {
        hero2->PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );
    }
    else {
        fheroes2::Blit( noHeroPortrait, display, rectHero2.x, rectHero2.y );
    }

    // indicator
    if ( !allow_buy_hero2 ) {
        const fheroes2::Sprite & spriteDeny = fheroes2::AGG::GetICN( ICN::TOWNWIND, 12 );
        fheroes2::Blit( spriteDeny, display, dst_pt.x + 102 - 4 + 1 - spriteDeny.width(), dst_pt.y + 93 - 2 - spriteDeny.height() );
    }

    const int32_t statusBarOffsetY = 480 - 19;

    fheroes2::Button buttonPrevCastle( cur_pt.x, cur_pt.y + statusBarOffsetY, ICN::SMALLBAR, 1, 2 );
    fheroes2::TimedEventValidator timedButtonPrevCastle( [&buttonPrevCastle]() { return buttonPrevCastle.isPressed(); } );
    buttonPrevCastle.subscribe( &timedButtonPrevCastle );

    // bottom small bar
    const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( ICN::SMALLBAR, 0 );
    fheroes2::Blit( bar, display, cur_pt.x + buttonPrevCastle.area().width, cur_pt.y + statusBarOffsetY );

    StatusBar statusBar;
    statusBar.SetFont( Font::BIG );
    statusBar.SetCenter( cur_pt.x + buttonPrevCastle.area().width + bar.width() / 2, cur_pt.y + statusBarOffsetY + 12 );

    // button next castle
    fheroes2::Button buttonNextCastle( cur_pt.x + buttonPrevCastle.area().width + bar.width(), cur_pt.y + statusBarOffsetY, ICN::SMALLBAR, 3, 4 );
    fheroes2::TimedEventValidator timedButtonNextCastle( [&buttonNextCastle]() { return buttonNextCastle.isPressed(); } );
    buttonNextCastle.subscribe( &timedButtonNextCastle );

    // button exit
    dst_pt.x = cur_pt.x + 553;
    dst_pt.y = cur_pt.y + 428;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::BUTTON_SMALLER_EXIT, 0, 1 );

    if ( GetKingdom().GetCastles().size() < 2 ) {
        buttonPrevCastle.disable();
        buttonNextCastle.disable();
    }

    buttonPrevCastle.draw();
    buttonNextCastle.draw();
    buttonExit.draw();

    // redraw resource panel
    const fheroes2::Rect & rectResource = fheroes2::drawResourcePanel( GetKingdom().GetFunds(), display, cur_pt );
    const fheroes2::Rect resActiveArea( rectResource.x, rectResource.y, rectResource.width, buttonExit.area().y - rectResource.y - 3 );

    display.render();

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( buttonPrevCastle.isEnabled() ) {
            le.MousePressLeft( buttonPrevCastle.area() ) ? buttonPrevCastle.drawOnPress() : buttonPrevCastle.drawOnRelease();
        }
        if ( buttonNextCastle.isEnabled() ) {
            le.MousePressLeft( buttonNextCastle.area() ) ? buttonNextCastle.drawOnPress() : buttonNextCastle.drawOnRelease();
        }

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() )
            break;

        if ( buttonPrevCastle.isEnabled()
             && ( le.MouseClickLeft( buttonPrevCastle.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_LEFT ) || timedButtonPrevCastle.isDelayPassed() ) ) {
            return ConstructionDialogResult::PrevConstructionWindow;
        }
        if ( buttonNextCastle.isEnabled()
             && ( le.MouseClickLeft( buttonNextCastle.area() ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_RIGHT ) || timedButtonNextCastle.isDelayPassed() ) ) {
            return ConstructionDialogResult::NextConstructionWindow;
        }

        if ( le.MouseClickLeft( resActiveArea ) ) {
            fheroes2::ButtonRestorer exitRestorer( buttonExit );
            fheroes2::showKingdomIncome( world.GetKingdom( GetColor() ), Dialog::OK );
        }
        else if ( le.MousePressRight( resActiveArea ) ) {
            fheroes2::showKingdomIncome( world.GetKingdom( GetColor() ), 0 );
        }
        else if ( le.MousePressRight( buttonExit.area() ) ) {
            Dialog::Message( _( "Exit" ), _( "Exit this menu." ), Font::BIG );
        }

        // click left
        if ( le.MouseCursor( dwelling1.GetArea() ) && dwelling1.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling1.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( dwelling2.GetArea() ) && dwelling2.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling2.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( dwelling3.GetArea() ) && dwelling3.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling3.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( dwelling4.GetArea() ) && dwelling4.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling4.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( dwelling5.GetArea() ) && dwelling5.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling5.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( dwelling6.GetArea() ) && dwelling6.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling6.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingMageGuild.GetArea() ) && buildingMageGuild.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = buildingMageGuild.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( !isSkipTavernInteraction && le.MouseCursor( buildingTavern.GetArea() ) && buildingTavern.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = ( Race::NECR == race ? BUILD_SHRINE : BUILD_TAVERN );
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingThievesGuild.GetArea() ) && buildingThievesGuild.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_THIEVESGUILD;
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingShipyard.GetArea() ) && buildingShipyard.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_SHIPYARD;
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingStatue.GetArea() ) && buildingStatue.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_STATUE;
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingMarketplace.GetArea() ) && buildingMarketplace.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_MARKETPLACE;
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingWell.GetArea() ) && buildingWell.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_WELL;
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingWel2.GetArea() ) && buildingWel2.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_WEL2;
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingSpec.GetArea() ) && buildingSpec.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_SPEC;
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingLTurret.GetArea() ) && buildingLTurret.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_LEFTTURRET;
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingRTurret.GetArea() ) && buildingRTurret.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_RIGHTTURRET;
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingMoat.GetArea() ) && buildingMoat.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_MOAT;
            return ConstructionDialogResult::Build;
        }
        if ( le.MouseCursor( buildingCaptain.GetArea() ) && buildingCaptain.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_CAPTAIN;
            return ConstructionDialogResult::Build;
        }
        else if ( hero1 && le.MouseClickLeft( rectHero1 ) ) {
            fheroes2::ButtonRestorer exitRestorer( buttonExit );
            if ( Dialog::OK == DialogBuyHero( hero1 ) ) {
                RecruitHero( hero1 );

                return ConstructionDialogResult::RecruitHero;
            }
        }
        else if ( hero2 && le.MouseClickLeft( rectHero2 ) ) {
            fheroes2::ButtonRestorer exitRestorer( buttonExit );
            if ( Dialog::OK == DialogBuyHero( hero2 ) ) {
                RecruitHero( hero2 );

                return ConstructionDialogResult::RecruitHero;
            }
        }
        else if ( isBuild( BUILD_CAPTAIN ) ) {
            if ( le.MouseClickLeft( rectSpreadArmyFormat ) && !army.isSpreadFormation() ) {
                cursorFormat.setPosition( pointSpreadArmyFormat.x, pointSpreadArmyFormat.y );
                display.render();
                army.SetSpreadFormation( true );
            }
            else if ( le.MouseClickLeft( rectGroupedArmyFormat ) && army.isSpreadFormation() ) {
                cursorFormat.setPosition( pointGroupedArmyFormat.x, pointGroupedArmyFormat.y );
                display.render();
                army.SetSpreadFormation( false );
            }
        }

        const bool isCaptainBuilt = isBuild( BUILD_CAPTAIN );

        // Right click
        if ( isCaptainBuilt && le.MousePressRight( rectSpreadArmyFormat ) )
            Dialog::Message( _( "Spread Formation" ), descriptionSpreadArmyFormat, Font::BIG );
        else if ( isCaptainBuilt && le.MousePressRight( rectGroupedArmyFormat ) )
            Dialog::Message( _( "Grouped Formation" ), descriptionGroupedArmyFormat, Font::BIG );
        else if ( hero1 && le.MousePressRight( rectHero1 ) ) {
            LocalEvent::GetClean();
            hero1->OpenDialog( true, true, false, false );

            if ( Settings::isFadeEffectEnabled() ) {
                // Use half fade if game resolution is not 640x480.
                fheroes2::fadeInDisplay( restorer.rect(), !display.isDefaultSize() );
            }
            else {
                // This dialog currently does not have borders so its ROI is the same as fade ROI.
                display.render( restorer.rect() );
            }
        }
        else if ( hero2 && le.MousePressRight( rectHero2 ) ) {
            LocalEvent::GetClean();
            hero2->OpenDialog( true, true, false, false );

            if ( Settings::isFadeEffectEnabled() ) {
                // Use half fade if game resolution is not 640x480.
                fheroes2::fadeInDisplay( restorer.rect(), !display.isDefaultSize() );
            }
            else {
                // This dialog currently does not have borders so its ROI is the same as fade ROI.
                display.render( restorer.rect() );
            }
        }
        else if ( le.MousePressRight( buttonNextCastle.area() ) ) {
            Dialog::Message( _( "Show next town" ), _( "Click to show next town." ), Font::BIG );
        }
        else if ( le.MousePressRight( buttonPrevCastle.area() ) ) {
            Dialog::Message( _( "Show previous town" ), _( "Click to show previous town." ), Font::BIG );
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
        else if ( buttonPrevCastle.isEnabled() && le.MouseCursor( buttonPrevCastle.area() ) ) {
            statusBar.ShowMessage( _( "Show previous town" ) );
        }
        else if ( buttonNextCastle.isEnabled() && le.MouseCursor( buttonNextCastle.area() ) ) {
            statusBar.ShowMessage( _( "Show next town" ) );
        }
        else {
            statusBar.ShowMessage( _( "Castle Options" ) );
        }
    }

    return ConstructionDialogResult::DoNothing;
}
