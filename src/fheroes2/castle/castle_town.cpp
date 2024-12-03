/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <utility>

#include "agg_image.h"
#include "army.h"
#include "artifact.h"
#include "buildinginfo.h"
#include "captain.h"
#include "castle.h" // IWYU pragma: associated
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
#include "maps_fileinfo.h"
#include "math_base.h"
#include "payment.h"
#include "race.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "statusbar.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_castle.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_kingdom.h"
#include "ui_language.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "world.h"

int Castle::DialogBuyHero( const Heroes * hero ) const
{
    if ( !hero ) {
        return Dialog::CANCEL;
    }

    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const int32_t spacer = 10;
    const fheroes2::Sprite & portrait_frame = fheroes2::AGG::GetICN( ICN::SURRENDR, 4 );

    const fheroes2::Text recruitHeroText( _( "Recruit Hero" ), fheroes2::FontType::normalYellow() );

    uint32_t count = hero->GetCountArtifacts();
    if ( hero->hasArtifact( Artifact::MAGIC_BOOK ) ) {
        --count;
    }

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

    const fheroes2::Text heroDescriptionText( std::move( str ), fheroes2::FontType::normalWhite() );

    Resource::BoxSprite rbs( PaymentConditions::RecruitHero(), fheroes2::boxAreaWidthPx );

    const int32_t dialogHeight = recruitHeroText.height( fheroes2::boxAreaWidthPx ) + spacer + portrait_frame.height() + spacer
                                 + heroDescriptionText.height( fheroes2::boxAreaWidthPx ) + spacer + rbs.GetArea().height;

    const Dialog::FrameBox box( dialogHeight, true );
    const fheroes2::Rect & dialogRoi = box.GetArea();

    recruitHeroText.draw( dialogRoi.x, dialogRoi.y + 2, fheroes2::boxAreaWidthPx, display );

    // portrait and frame
    fheroes2::Point pos{ dialogRoi.x + ( dialogRoi.width - portrait_frame.width() ) / 2, dialogRoi.y + recruitHeroText.height( fheroes2::boxAreaWidthPx ) + spacer };
    fheroes2::Blit( portrait_frame, display, pos.x, pos.y );

    const fheroes2::Rect heroPortraitArea( pos.x, pos.y, portrait_frame.width(), portrait_frame.height() );
    pos.x = pos.x + 5;
    pos.y = pos.y + 6;
    hero->PortraitRedraw( pos.x, pos.y, PORT_BIG, display );

    pos.y += portrait_frame.height() + spacer;
    heroDescriptionText.draw( dialogRoi.x, pos.y + 2, fheroes2::boxAreaWidthPx, display );

    rbs.SetPos( dialogRoi.x, pos.y + heroDescriptionText.height( fheroes2::boxAreaWidthPx ) + spacer );
    rbs.Redraw();

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int okayButtonIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_OKAY_BUTTON : ICN::UNIFORM_GOOD_OKAY_BUTTON;

    pos.y = dialogRoi.y + dialogRoi.height - fheroes2::AGG::GetICN( okayButtonIcnID, 0 ).height();
    fheroes2::Button buttonOkay( dialogRoi.x, pos.y, okayButtonIcnID, 0, 1 );

    if ( !AllowBuyHero() ) {
        buttonOkay.disable();
    }

    const int cancelButtonIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_CANCEL_BUTTON : ICN::UNIFORM_GOOD_CANCEL_BUTTON;

    pos.x = dialogRoi.x + dialogRoi.width - fheroes2::AGG::GetICN( cancelButtonIcnID, 0 ).width();
    pos.y = dialogRoi.y + dialogRoi.height - fheroes2::AGG::GetICN( cancelButtonIcnID, 0 ).height();
    fheroes2::Button buttonCancel( pos.x, pos.y, cancelButtonIcnID, 0, 1 );

    buttonOkay.draw();
    buttonCancel.draw();

    display.render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonOkay.area() ) ? buttonOkay.drawOnPress() : buttonOkay.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        if ( buttonOkay.isEnabled() && ( le.MouseClickLeft( buttonOkay.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) ) {
            return Dialog::OK;
        }

        if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            break;
        }

        if ( le.isMouseRightButtonPressedInArea( heroPortraitArea ) ) {
            Dialog::QuickInfo( *hero );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonOkay.area() ) ) {
            std::string recruitHero = _( "Recruit %{name} the %{race}" );
            StringReplace( recruitHero, "%{name}", hero->GetName() );
            StringReplace( recruitHero, "%{race}", Race::String( hero->GetRace() ) );
            recruitHero += '.';
            fheroes2::showStandardTextMessage( _( "Okay" ), std::move( recruitHero ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
        }
    }

    return Dialog::CANCEL;
}

int Castle::DialogBuyCastle( const bool hasButtons /* = true */ ) const
{
    const BuildingInfo info( *this, BUILD_CASTLE );
    return info.DialogBuyBuilding( hasButtons ) ? Dialog::OK : Dialog::CANCEL;
}

Castle::ConstructionDialogResult Castle::_openConstructionDialog( uint32_t & dwellingTobuild )
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

    const Settings & conf = Settings::Get();
    const bool isEvilInterface = conf.isEvilInterfaceEnabled();

    fheroes2::Blit( fheroes2::AGG::GetICN( isEvilInterface ? ICN::CASLWIND_EVIL : ICN::CASLWIND, 0 ), display, dst_pt.x, dst_pt.y );

    // hide captain options
    if ( !( _constructedBuildings & BUILD_CAPTAIN ) ) {
        dst_pt.x = 530;
        dst_pt.y = 163;
        const fheroes2::Rect rect( dst_pt.x, dst_pt.y, 110, 84 );
        dst_pt.x += cur_pt.x;
        dst_pt.y += cur_pt.y;

        const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 );
        fheroes2::Copy( backgroundImage, rect.x, rect.y, display, dst_pt.x, dst_pt.y, rect.width, rect.height );
    }

    // draw castle sprite
    dst_pt.x = cur_pt.x + 459;
    dst_pt.y = cur_pt.y + 5;
    DrawImageCastle( dst_pt );

    // castle name
    const fheroes2::Text castleName( GetName(), fheroes2::FontType::smallWhite() );
    castleName.draw( cur_pt.x + 538 - castleName.width() / 2, cur_pt.y + 3, display );

    dst_pt.y = cur_pt.y + 2;

    BuildingInfo dwelling1( *this, DWELLING_MONSTER1 );
    dwelling1.SetPos( cur_pt.x + 5, dst_pt.y );
    dwelling1.Redraw();

    BuildingInfo dwelling2( *this, DWELLING_MONSTER2 );
    dwelling2.SetPos( cur_pt.x + 149, dst_pt.y );
    dwelling2.Redraw();

    BuildingInfo dwelling3( *this, DWELLING_MONSTER3 );
    dwelling3.SetPos( cur_pt.x + 293, dst_pt.y );
    dwelling3.Redraw();

    dst_pt.y = cur_pt.y + 77;

    BuildingInfo dwelling4( *this, DWELLING_MONSTER4 );
    dwelling4.SetPos( cur_pt.x + 5, dst_pt.y );
    dwelling4.Redraw();

    BuildingInfo dwelling5( *this, DWELLING_MONSTER5 );
    dwelling5.SetPos( cur_pt.x + 149, dst_pt.y );
    dwelling5.Redraw();

    BuildingInfo dwelling6( *this, DWELLING_MONSTER6 );
    dwelling6.SetPos( cur_pt.x + 293, dst_pt.y );
    dwelling6.Redraw();

    // mage guild
    BuildingType level = BUILD_NOTHING;
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

    dst_pt.y = cur_pt.y + 157;

    BuildingInfo buildingMageGuild( *this, level );
    buildingMageGuild.SetPos( cur_pt.x + 5, dst_pt.y );
    buildingMageGuild.Redraw();

    // tavern
    const bool isSkipTavernInteraction = ( Race::NECR == _race ) && ( conf.getCurrentMapInfo().version == GameVersion::SUCCESSION_WARS );
    BuildingInfo buildingTavern( *this, BUILD_TAVERN );
    buildingTavern.SetPos( cur_pt.x + 149, dst_pt.y );
    buildingTavern.Redraw();

    // thieves guild
    BuildingInfo buildingThievesGuild( *this, BUILD_THIEVESGUILD );
    buildingThievesGuild.SetPos( cur_pt.x + 293, dst_pt.y );
    buildingThievesGuild.Redraw();

    dst_pt.y = cur_pt.y + 232;

    // shipyard
    BuildingInfo buildingShipyard( *this, BUILD_SHIPYARD );
    buildingShipyard.SetPos( cur_pt.x + 5, dst_pt.y );
    buildingShipyard.Redraw();

    // statue
    BuildingInfo buildingStatue( *this, BUILD_STATUE );
    buildingStatue.SetPos( cur_pt.x + 149, dst_pt.y );
    buildingStatue.Redraw();

    // marketplace
    BuildingInfo buildingMarketplace( *this, BUILD_MARKETPLACE );
    buildingMarketplace.SetPos( cur_pt.x + 293, dst_pt.y );
    buildingMarketplace.Redraw();

    dst_pt.y = cur_pt.y + 307;

    // well
    BuildingInfo buildingWell( *this, BUILD_WELL );
    buildingWell.SetPos( cur_pt.x + 5, dst_pt.y );
    buildingWell.Redraw();

    // wel2
    BuildingInfo buildingWel2( *this, BUILD_WEL2 );
    buildingWel2.SetPos( cur_pt.x + 149, dst_pt.y );
    buildingWel2.Redraw();

    // spec
    BuildingInfo buildingSpec( *this, BUILD_SPEC );
    buildingSpec.SetPos( cur_pt.x + 293, dst_pt.y );
    buildingSpec.Redraw();

    dst_pt.y = cur_pt.y + 387;

    // left turret
    BuildingInfo buildingLTurret( *this, BUILD_LEFTTURRET );
    buildingLTurret.SetPos( cur_pt.x + 5, dst_pt.y );
    buildingLTurret.Redraw();

    // right turret
    BuildingInfo buildingRTurret( *this, BUILD_RIGHTTURRET );
    buildingRTurret.SetPos( cur_pt.x + 149, dst_pt.y );
    buildingRTurret.Redraw();

    // moat
    BuildingInfo buildingMoat( *this, BUILD_MOAT );
    buildingMoat.SetPos( cur_pt.x + 293, dst_pt.y );
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
        _( "'Spread' combat formation spreads the castle's units from the top to the bottom of the battlefield, with at least one empty space between each unit." ) );
    const std::string descriptionGroupedArmyFormat(
        _( "'Grouped' combat formation bunches the castle's units together in the center of the castle's side of the battlefield." ) );
    const fheroes2::Point pointSpreadArmyFormat( rectSpreadArmyFormat.x - 1, rectSpreadArmyFormat.y - 1 );
    const fheroes2::Point pointGroupedArmyFormat( rectGroupedArmyFormat.x - 1, rectGroupedArmyFormat.y - 1 );
    const fheroes2::Rect armyFormatRenderRect( pointSpreadArmyFormat, { rectGroupedArmyFormat.x + rectGroupedArmyFormat.width - pointSpreadArmyFormat.x + 1,
                                                                        rectGroupedArmyFormat.y + rectGroupedArmyFormat.height - pointSpreadArmyFormat.y + 1 } );

    fheroes2::MovableSprite cursorFormat( fheroes2::AGG::GetICN( ICN::HSICONS, 11 ) );

    if ( isBuild( BUILD_CAPTAIN ) ) {
        const int32_t skillValueOffsetX = 90;
        const int32_t skillOffsetY = 12;

        fheroes2::Text text( Skill::Primary::String( Skill::Primary::ATTACK ), fheroes2::FontType::smallWhite() );
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y = cur_pt.y + 170;
        text.draw( dst_pt.x, dst_pt.y, display );

        text.set( std::to_string( _captain.GetAttack() ), fheroes2::FontType::smallWhite() );
        text.draw( dst_pt.x + skillValueOffsetX, dst_pt.y, display );

        dst_pt.y += skillOffsetY;
        text.set( Skill::Primary::String( Skill::Primary::DEFENSE ), fheroes2::FontType::smallWhite() );
        text.draw( dst_pt.x, dst_pt.y, display );

        text.set( std::to_string( _captain.GetDefense() ), fheroes2::FontType::smallWhite() );
        text.draw( dst_pt.x + skillValueOffsetX, dst_pt.y, display );

        dst_pt.y += skillOffsetY;
        text.set( Skill::Primary::String( Skill::Primary::POWER ), fheroes2::FontType::smallWhite() );
        text.draw( dst_pt.x, dst_pt.y, display );

        text.set( std::to_string( _captain.GetPower() ), fheroes2::FontType::smallWhite() );
        text.draw( dst_pt.x + skillValueOffsetX, dst_pt.y, display );

        dst_pt.y += skillOffsetY;
        text.set( Skill::Primary::String( Skill::Primary::KNOWLEDGE ), fheroes2::FontType::smallWhite() );
        text.draw( dst_pt.x, dst_pt.y, display );

        text.set( std::to_string( _captain.GetKnowledge() ), fheroes2::FontType::smallWhite() );
        text.draw( dst_pt.x + skillValueOffsetX, dst_pt.y, display );

        fheroes2::Copy( spriteSpreadArmyFormat, 0, 0, display, rectSpreadArmyFormat.x, rectSpreadArmyFormat.y, rectSpreadArmyFormat.width, rectSpreadArmyFormat.height );
        fheroes2::Copy( spriteGroupedArmyFormat, 0, 0, display, rectGroupedArmyFormat.x, rectGroupedArmyFormat.y, rectGroupedArmyFormat.width,
                        rectGroupedArmyFormat.height );

        if ( _army.isSpreadFormation() ) {
            cursorFormat.setPosition( pointSpreadArmyFormat.x, pointSpreadArmyFormat.y );
        }
        else {
            cursorFormat.setPosition( pointGroupedArmyFormat.x, pointGroupedArmyFormat.y );
        }
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

    if ( hero1 ) {
        hero1->PortraitRedraw( dst_pt.x, dst_pt.y, PORT_BIG, display );
    }
    else {
        fheroes2::Image noHeroPortrait;
        noHeroPortrait._disableTransformLayer();
        noHeroPortrait.resize( rectHero1.width, rectHero1.height );
        noHeroPortrait.fill( 0 );
        fheroes2::Copy( noHeroPortrait, 0, 0, display, rectHero1.x, rectHero1.y, rectHero1.width, rectHero1.height );
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
        fheroes2::Image noHeroPortrait;
        noHeroPortrait._disableTransformLayer();
        noHeroPortrait.resize( rectHero2.width, rectHero2.height );
        noHeroPortrait.fill( 0 );
        fheroes2::Copy( noHeroPortrait, 0, 0, display, rectHero2.x, rectHero2.y, rectHero2.width, rectHero2.height );
    }

    // indicator
    if ( !allow_buy_hero2 ) {
        const fheroes2::Sprite & spriteDeny = fheroes2::AGG::GetICN( ICN::TOWNWIND, 12 );
        fheroes2::Blit( spriteDeny, display, dst_pt.x + 102 - 4 + 1 - spriteDeny.width(), dst_pt.y + 93 - 2 - spriteDeny.height() );
    }

    dst_pt.y = cur_pt.y + 480 - 19;
    fheroes2::Button buttonPrevCastle( cur_pt.x, dst_pt.y, ICN::SMALLBAR, 1, 2 );
    fheroes2::TimedEventValidator timedButtonPrevCastle( [&buttonPrevCastle]() { return buttonPrevCastle.isPressed(); } );
    buttonPrevCastle.subscribe( &timedButtonPrevCastle );
    const fheroes2::Rect buttonPrevCastleArea( buttonPrevCastle.area() );

    // bottom small bar
    const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( ICN::SMALLBAR, 0 );
    const int32_t statusBarWidth = bar.width();
    dst_pt.x = cur_pt.x + buttonPrevCastleArea.width;
    fheroes2::Copy( bar, 0, 0, display, dst_pt.x, dst_pt.y, statusBarWidth, bar.height() );

    StatusBar statusBar;
    // Status bar must be smaller due to extra art on both sides.
    statusBar.setRoi( { dst_pt.x + 16, dst_pt.y + 3, statusBarWidth - 16 * 2, 0 } );

    // button next castle
    fheroes2::Button buttonNextCastle( dst_pt.x + statusBarWidth, dst_pt.y, ICN::SMALLBAR, 3, 4 );
    fheroes2::TimedEventValidator timedButtonNextCastle( [&buttonNextCastle]() { return buttonNextCastle.isPressed(); } );
    buttonNextCastle.subscribe( &timedButtonNextCastle );
    const fheroes2::Rect buttonNextCastleArea( buttonNextCastle.area() );

    // button exit
    dst_pt.x = cur_pt.x + 553;
    dst_pt.y = cur_pt.y + 428;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::BUTTON_EXIT_TOWN, 0, 1 );
    const fheroes2::Rect buttonExitArea( buttonExit.area() );

    if ( GetKingdom().GetCastles().size() < 2 ) {
        buttonPrevCastle.disable();
        buttonNextCastle.disable();
    }

    buttonPrevCastle.draw();
    buttonNextCastle.draw();
    buttonExit.draw();

    // redraw resource panel
    const fheroes2::Rect & rectResource = fheroes2::drawResourcePanel( GetKingdom().GetFunds(), display, cur_pt );
    const fheroes2::Rect resActiveArea( rectResource.x, rectResource.y, rectResource.width, buttonExitArea.y - rectResource.y - 3 );

    auto recruitHeroDialog = [this, &buttonExit]( Heroes * hero ) {
        const fheroes2::ButtonRestorer exitRestorer( buttonExit );
        if ( Dialog::OK == DialogBuyHero( hero ) ) {
            RecruitHero( hero );
            return true;
        }
        return false;
    };

    display.render( restorer.rect() );

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonExitArea ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExitArea ) || Game::HotKeyCloseWindow() ) {
            break;
        }

        if ( buttonPrevCastle.isEnabled() ) {
            le.isMouseLeftButtonPressedInArea( buttonPrevCastleArea ) ? buttonPrevCastle.drawOnPress() : buttonPrevCastle.drawOnRelease();

            if ( le.MouseClickLeft( buttonPrevCastleArea ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_LEFT ) || timedButtonPrevCastle.isDelayPassed() ) {
                return ConstructionDialogResult::PrevConstructionWindow;
            }
        }
        if ( buttonNextCastle.isEnabled() ) {
            le.isMouseLeftButtonPressedInArea( buttonNextCastleArea ) ? buttonNextCastle.drawOnPress() : buttonNextCastle.drawOnRelease();

            if ( le.MouseClickLeft( buttonNextCastleArea ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_RIGHT ) || timedButtonNextCastle.isDelayPassed() ) {
                return ConstructionDialogResult::NextConstructionWindow;
            }
        }

        if ( le.MouseClickLeft( resActiveArea ) ) {
            const fheroes2::ButtonRestorer exitRestorer( buttonExit );
            fheroes2::showKingdomIncome( world.GetKingdom( GetColor() ), Dialog::OK );
        }
        else if ( le.isMouseRightButtonPressedInArea( resActiveArea ) ) {
            fheroes2::showKingdomIncome( world.GetKingdom( GetColor() ), 0 );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonExitArea ) ) {
            fheroes2::showStandardTextMessage( _( "Exit" ), _( "Exit this menu." ), Dialog::ZERO );
        }
        else if ( le.isMouseCursorPosInArea( dwelling1.GetArea() ) && dwelling1.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling1.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( dwelling2.GetArea() ) && dwelling2.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling2.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( dwelling3.GetArea() ) && dwelling3.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling3.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( dwelling4.GetArea() ) && dwelling4.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling4.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( dwelling5.GetArea() ) && dwelling5.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling5.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( dwelling6.GetArea() ) && dwelling6.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = dwelling6.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingMageGuild.GetArea() ) && buildingMageGuild.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = buildingMageGuild.getBuilding();
            return ConstructionDialogResult::Build;
        }
        if ( !isSkipTavernInteraction && le.isMouseCursorPosInArea( buildingTavern.GetArea() ) && buildingTavern.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = ( Race::NECR == _race ? BUILD_SHRINE : BUILD_TAVERN );
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingThievesGuild.GetArea() ) && buildingThievesGuild.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_THIEVESGUILD;
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingShipyard.GetArea() ) && buildingShipyard.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_SHIPYARD;
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingStatue.GetArea() ) && buildingStatue.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_STATUE;
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingMarketplace.GetArea() ) && buildingMarketplace.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_MARKETPLACE;
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingWell.GetArea() ) && buildingWell.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_WELL;
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingWel2.GetArea() ) && buildingWel2.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_WEL2;
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingSpec.GetArea() ) && buildingSpec.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_SPEC;
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingLTurret.GetArea() ) && buildingLTurret.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_LEFTTURRET;
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingRTurret.GetArea() ) && buildingRTurret.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_RIGHTTURRET;
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingMoat.GetArea() ) && buildingMoat.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_MOAT;
            return ConstructionDialogResult::Build;
        }
        if ( le.isMouseCursorPosInArea( buildingCaptain.GetArea() ) && buildingCaptain.QueueEventProcessing( buttonExit ) ) {
            dwellingTobuild = BUILD_CAPTAIN;
            return ConstructionDialogResult::Build;
        }
        if ( hero1 && le.MouseClickLeft( rectHero1 ) && recruitHeroDialog( hero1 ) ) {
            return ConstructionDialogResult::RecruitHero;
        }
        if ( hero2 && le.MouseClickLeft( rectHero2 ) && recruitHeroDialog( hero2 ) ) {
            return ConstructionDialogResult::RecruitHero;
        }

        const bool isCaptainBuilt = isBuild( BUILD_CAPTAIN );

        if ( isCaptainBuilt ) {
            if ( le.MouseClickLeft( rectSpreadArmyFormat ) && !_army.isSpreadFormation() ) {
                cursorFormat.setPosition( pointSpreadArmyFormat.x, pointSpreadArmyFormat.y );
                display.render( armyFormatRenderRect );
                _army.SetSpreadFormation( true );
            }
            else if ( le.MouseClickLeft( rectGroupedArmyFormat ) && _army.isSpreadFormation() ) {
                cursorFormat.setPosition( pointGroupedArmyFormat.x, pointGroupedArmyFormat.y );
                display.render( armyFormatRenderRect );
                _army.SetSpreadFormation( false );
            }
            else if ( le.isMouseRightButtonPressedInArea( rectSpreadArmyFormat ) ) {
                fheroes2::showStandardTextMessage( _( "Spread Formation" ), descriptionSpreadArmyFormat, Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( rectGroupedArmyFormat ) ) {
                fheroes2::showStandardTextMessage( _( "Grouped Formation" ), descriptionGroupedArmyFormat, Dialog::ZERO );
            }
        }

        // Right click
        if ( hero1 && le.isMouseRightButtonPressedInArea( rectHero1 ) ) {
            LocalEvent::Get().reset();
            hero1->OpenDialog( true, true, false, false, false, false, fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() ) );

            // Use half fade if game resolution is not 640x480.
            fheroes2::fadeInDisplay( restorer.rect(), !display.isDefaultSize() );
        }
        else if ( hero2 && le.isMouseRightButtonPressedInArea( rectHero2 ) ) {
            LocalEvent::Get().reset();
            hero2->OpenDialog( true, true, false, false, false, false, fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() ) );

            // Use half fade if game resolution is not 640x480.
            fheroes2::fadeInDisplay( restorer.rect(), !display.isDefaultSize() );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonNextCastleArea ) ) {
            fheroes2::showStandardTextMessage( _( "Show next town" ), _( "Click to show the next town." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonPrevCastleArea ) ) {
            fheroes2::showStandardTextMessage( _( "Show previous town" ), _( "Click to show the previous town." ), Dialog::ZERO );
        }

        // status info
        if ( le.isMouseCursorPosInArea( dwelling1.GetArea() ) )
            dwelling1.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( dwelling2.GetArea() ) )
            dwelling2.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( dwelling3.GetArea() ) )
            dwelling3.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( dwelling4.GetArea() ) )
            dwelling4.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( dwelling5.GetArea() ) )
            dwelling5.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( dwelling6.GetArea() ) )
            dwelling6.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingMageGuild.GetArea() ) )
            buildingMageGuild.SetStatusMessage( statusBar );
        else if ( !isSkipTavernInteraction && le.isMouseCursorPosInArea( buildingTavern.GetArea() ) )
            buildingTavern.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingThievesGuild.GetArea() ) )
            buildingThievesGuild.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingShipyard.GetArea() ) )
            buildingShipyard.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingStatue.GetArea() ) )
            buildingStatue.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingMarketplace.GetArea() ) )
            buildingMarketplace.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingWell.GetArea() ) )
            buildingWell.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingWel2.GetArea() ) )
            buildingWel2.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingSpec.GetArea() ) )
            buildingSpec.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingLTurret.GetArea() ) )
            buildingLTurret.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingRTurret.GetArea() ) )
            buildingRTurret.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingMoat.GetArea() ) )
            buildingMoat.SetStatusMessage( statusBar );
        else if ( le.isMouseCursorPosInArea( buildingCaptain.GetArea() ) )
            buildingCaptain.SetStatusMessage( statusBar );
        else if ( hero1 && le.isMouseCursorPosInArea( rectHero1 ) ) {
            if ( !allow_buy_hero1 )
                statusBar.ShowMessage( not_allow1_msg );
            else {
                std::string str = _( "Recruit %{name} the %{race}" );
                StringReplace( str, "%{name}", hero1->GetName() );
                StringReplace( str, "%{race}", Race::String( hero1->GetRace() ) );
                statusBar.ShowMessage( str );
            }
        }
        else if ( hero2 && le.isMouseCursorPosInArea( rectHero2 ) ) {
            if ( !allow_buy_hero2 )
                statusBar.ShowMessage( not_allow2_msg );
            else {
                std::string str = _( "Recruit %{name} the %{race}" );
                StringReplace( str, "%{name}", hero2->GetName() );
                StringReplace( str, "%{race}", Race::String( hero2->GetRace() ) );
                statusBar.ShowMessage( str );
            }
        }
        else if ( isCaptainBuilt && le.isMouseCursorPosInArea( rectSpreadArmyFormat ) )
            statusBar.ShowMessage( _( "Set garrison combat formation to 'Spread'" ) );
        else if ( isCaptainBuilt && le.isMouseCursorPosInArea( rectGroupedArmyFormat ) )
            statusBar.ShowMessage( _( "Set garrison combat formation to 'Grouped'" ) );
        else if ( le.isMouseCursorPosInArea( buttonExitArea ) )
            statusBar.ShowMessage( _( "Exit Castle Options" ) );
        else if ( le.isMouseCursorPosInArea( resActiveArea ) )
            statusBar.ShowMessage( _( "Show Income" ) );
        else if ( buttonPrevCastle.isEnabled() && le.isMouseCursorPosInArea( buttonPrevCastleArea ) ) {
            statusBar.ShowMessage( _( "Show previous town" ) );
        }
        else if ( buttonNextCastle.isEnabled() && le.isMouseCursorPosInArea( buttonNextCastleArea ) ) {
            statusBar.ShowMessage( _( "Show next town" ) );
        }
        else {
            statusBar.ShowMessage( _( "Castle Options" ) );
        }
    }

    return ConstructionDialogResult::DoNothing;
}
