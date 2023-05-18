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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "agg_image.h"
#include "army_troop.h"
#include "bin_info.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "localevent.h"
#include "math_base.h"
#include "monster.h"
#include "pal.h"
#include "payment.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    void drawRecruitWindow( fheroes2::Image & output, const fheroes2::Point & offset )
    {
        const fheroes2::Sprite & originalBackground = fheroes2::AGG::GetICN( ICN::RECRBKG, 0 );
        const fheroes2::Sprite & recruitWindowTitle = fheroes2::AGG::GetICN( ICN::BLDGXTRA, 0 );

        fheroes2::Image recruitWindow( 132, 67 );
        recruitWindow.reset();
        fheroes2::Copy( recruitWindowTitle, 3, 58, recruitWindow, 3, 0, 63, 14 );
        fheroes2::Copy( recruitWindowTitle, recruitWindowTitle.width() - 66, 58, recruitWindow, 66, 0, 63, 14 );
        fheroes2::Copy( originalBackground, 138, 54, recruitWindow, 0, 0, 3, 63 );
        fheroes2::Copy( originalBackground, 267, 54, recruitWindow, 129, 0, 3, 63 );
        fheroes2::Copy( originalBackground, 138, 117, recruitWindow, 0, 63, 132, 4 );

        const fheroes2::Text recruitWindowText( _( "Cost per troop:" ), fheroes2::FontType::smallWhite() );
        recruitWindowText.draw( ( recruitWindow.width() - recruitWindowText.width() ) / 2, 3, recruitWindow );

        if ( Settings::Get().isEvilInterfaceEnabled() ) {
            fheroes2::ApplyPalette( recruitWindow, PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
        }

        fheroes2::Blit( recruitWindow, output, offset.x + 138, offset.y + 54 );
        fheroes2::addSoftShadow( recruitWindow, output, { offset.x + 138, offset.y + 54 }, { -5, 5 } );
    }
}

void RedrawCurrentInfo( const fheroes2::Point & pos, const uint32_t result, const payment_t & paymentMonster, const payment_t & paymentCosts, const Funds & funds,
                        const std::string & label, const fheroes2::Image & background = {} )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::Text text( std::to_string( result ), fheroes2::FontType::normalWhite() );

    // Restore the background of the text before rendering it.
    fheroes2::Copy( background, 118, 147, display, pos.x + 118, pos.y + 147, 68, text.height() );

    text.draw( pos.x + 151 - text.width() / 2, pos.y + 147, display );

    const std::string sgold = std::to_string( paymentCosts.gold ) + " " + "(" + std::to_string( funds.gold - paymentCosts.gold ) + ")";
    const int rsext = paymentMonster.GetValidItems() & ~Resource::GOLD;

    text.set( sgold, fheroes2::FontType::smallWhite() );

    // Restore the background of the text before rendering it.
    fheroes2::Copy( background, 0, 214, display, pos.x, pos.y + 214, background.width(), text.height() );

    if ( rsext ) {
        text.draw( pos.x + 117 - text.width() / 2, pos.y + 214, display );

        text.set( std::to_string( paymentCosts.Get( rsext ) ) + " " + "(" + std::to_string( funds.Get( rsext ) - paymentCosts.Get( rsext ) ) + ")",
                  fheroes2::FontType::smallWhite() );
        text.draw( pos.x + 179 - text.width() / 2, pos.y + 214, display );
    }
    else {
        text.draw( pos.x + 144 - text.width() / 2, pos.y + 214, display );
    }

    // Restore the background of the text before rendering it or to leave it blank if there is no text.
    fheroes2::Copy( background, 0, 166, display, pos.x, pos.y + 166, background.width(), text.height() );

    if ( !label.empty() ) {
        text.set( label, fheroes2::FontType::smallWhite() );
        text.draw( pos.x + 151 - text.width() / 2, pos.y + 166, display );
    }
}

void RedrawResourceInfo( const int resourceIcnIndex, const fheroes2::Point & pos, const int32_t value, const int32_t px1, const int32_t py1, const int32_t px2,
                         const int32_t py2, const bool showTotalSum )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Sprite & sres = fheroes2::AGG::GetICN( ICN::RESOURCE, Resource::getIconIcnIndex( resourceIcnIndex ) );
    fheroes2::Blit( sres, fheroes2::Display::instance(), pos.x + px1, pos.y + py1 );

    const fheroes2::Text text( std::to_string( value ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + px2 - text.width() / 2, pos.y + py2, display );

    if ( showTotalSum ) {
        fheroes2::Blit( sres, display, pos.x + px1 - 45, pos.y + py1 + 125 );
    }
}

void RedrawMonsterInfo( const fheroes2::Rect & pos, const Monster & monster, const uint32_t available, const bool showTotalSum )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const payment_t paymentMonster = monster.GetCost();
    const bool needExtraResources = 2 == paymentMonster.GetValidItemsCount();

    // Recruit monster text.
    std::string str = _( "Recruit %{name}" );
    StringReplace( str, "%{name}", monster.GetMultiName() );
    fheroes2::Text text( str, fheroes2::FontType::normalYellow() );
    fheroes2::Point dst_pt( pos.x + ( pos.width - text.width() ) / 2, pos.y + 27 );
    text.draw( dst_pt.x, dst_pt.y, display );

    // Monster sprite.
    const int monsterId = monster.GetID();
    const Bin_Info::MonsterAnimInfo & monsterInfo = Bin_Info::GetMonsterInfo( monsterId );
    assert( !monsterInfo.animationFrames[Bin_Info::MonsterAnimInfo::STATIC].empty() );

    const fheroes2::Sprite & smon = fheroes2::AGG::GetICN( monster.GetMonsterSprite(), monsterInfo.animationFrames[Bin_Info::MonsterAnimInfo::STATIC][0] );
    dst_pt.x = pos.x + 80 + smon.x() - ( monster.isWide() ? 22 : 0 );
    dst_pt.y = pos.y + 135 - smon.height();

    if ( monsterId == Monster::CHAMPION ) {
        ++dst_pt.x;
    }

    fheroes2::Blit( smon, display, dst_pt.x, dst_pt.y );

    // Resources needed to buy monster.
    if ( needExtraResources ) {
        RedrawResourceInfo( Resource::GOLD, pos.getPosition(), paymentMonster.gold, 150, 75, 183, 105, showTotalSum );

        if ( paymentMonster.crystal > 0 ) {
            RedrawResourceInfo( Resource::CRYSTAL, pos.getPosition(), paymentMonster.crystal, 222, 69, 240, 105, showTotalSum );
        }
        else if ( paymentMonster.mercury > 0 ) {
            RedrawResourceInfo( Resource::MERCURY, pos.getPosition(), paymentMonster.mercury, 225, 72, 240, 105, showTotalSum );
        }
        else if ( paymentMonster.wood > 0 ) {
            RedrawResourceInfo( Resource::WOOD, pos.getPosition(), paymentMonster.wood, 225, 72, 240, 105, showTotalSum );
        }
        else if ( paymentMonster.ore > 0 ) {
            RedrawResourceInfo( Resource::ORE, pos.getPosition(), paymentMonster.ore, 225, 72, 240, 105, showTotalSum );
        }
        else if ( paymentMonster.sulfur > 0 ) {
            RedrawResourceInfo( Resource::SULFUR, pos.getPosition(), paymentMonster.sulfur, 225, 75, 240, 105, showTotalSum );
        }
        else if ( paymentMonster.gems > 0 ) {
            RedrawResourceInfo( Resource::GEMS, pos.getPosition(), paymentMonster.gems, 225, 75, 240, 105, showTotalSum );
        }
    }
    else {
        // Only gold is needed.
        RedrawResourceInfo( Resource::GOLD, pos.getPosition(), paymentMonster.gold, 175, 75, 205, 105, showTotalSum );
    }

    str = _( "Available: %{count}" );
    StringReplace( str, "%{count}", available );
    text.set( str, fheroes2::FontType::smallWhite() );
    text.draw( pos.x + 80 - text.width() / 2, pos.y + 137, display );

    if ( showTotalSum ) {
        text.set( _( "Number to buy:" ), fheroes2::FontType::smallWhite() );
        text.draw( pos.x + 29, pos.y + 165, display );
    }
}

const char * SwitchMaxMinButtons( fheroes2::ButtonBase & btnMax, fheroes2::ButtonBase & btnMin, bool max )
{
    if ( btnMax.isEnabled() || btnMin.isEnabled() ) {
        if ( max ) {
            btnMax.disable();
            btnMin.enable();

            return _( "Max" );
        }

        btnMin.disable();
        btnMax.enable();

        return _( "Min" );
    }

    return "";
}

uint32_t CalculateMax( const Monster & monster, const Kingdom & kingdom, const uint32_t available )
{
    uint32_t max = 0;
    while ( kingdom.AllowPayment( monster.GetCost() * ( max + 1 ) ) && ( max + 1 ) <= available ) {
        ++max;
    }

    return max;
}

Troop Dialog::RecruitMonster( const Monster & monster0, const uint32_t available, const bool allowDowngradedMonster, const int32_t windowOffsetY )
{
    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // Set cursor.
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    // Calculate max count.
    Monster monster = monster0;
    payment_t paymentMonster = monster.GetCost();
    const Kingdom & kingdom = world.GetKingdom( Settings::Get().CurrentColor() );

    uint32_t max = CalculateMax( monster, kingdom, available );
    uint32_t result = max;

    payment_t paymentCosts( paymentMonster * result );

    const fheroes2::Size windowSize{ 299, 272 };
    const fheroes2::Point dialogOffset( ( display.width() - windowSize.width ) / 2, ( display.height() - windowSize.height ) / 2 + windowOffsetY );
    const fheroes2::Rect pos( dialogOffset.x - BORDERWIDTH, dialogOffset.y - BORDERWIDTH, windowSize.width + 2 * BORDERWIDTH, windowSize.height + 2 * BORDERWIDTH );

    const fheroes2::StandardWindow window( dialogOffset.x, dialogOffset.y, windowSize.width, windowSize.height, true, display );

    const fheroes2::Sprite & originalBackground = fheroes2::AGG::GetICN( ICN::RECRBKG, 0 );

    // Render the recruit count background from original recruit dialog ICN.
    fheroes2::Image background( 68, 19 );
    fheroes2::Copy( originalBackground, 134, 159, background, 0, 0, 68, 19 );

    if ( isEvilInterface ) {
        fheroes2::ApplyPalette( background, PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
    }

    fheroes2::Copy( background, 0, 0, display, pos.x + 134, pos.y + 159, 68, 19 );
    fheroes2::addSoftShadow( background, display, { pos.x + 134, pos.y + 159 }, { -5, 5 } );

    drawRecruitWindow( display, { pos.x, pos.y } );

    // Prepare buttons.
    fheroes2::Button buttonOk( pos.x + 34, pos.y + 249, isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD, 0, 1 );
    fheroes2::Button buttonCancel( pos.x + 197, pos.y + 249, isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1 );

    fheroes2::Point dst_pt( pos.x + 234, pos.y + 156 );

    fheroes2::Button buttonMax( dst_pt.x, dst_pt.y, isEvilInterface ? ICN::BUTTON_SMALL_MAX_EVIL : ICN::BUTTON_SMALL_MAX_GOOD, 0, 1 );
    fheroes2::Button buttonMin( dst_pt.x, dst_pt.y, isEvilInterface ? ICN::BUTTON_SMALL_MIN_EVIL : ICN::BUTTON_SMALL_MIN_GOOD, 0, 1 );

    dst_pt.x = pos.x + 205;
    dst_pt.y = pos.y + 154;
    fheroes2::Button buttonUp( dst_pt.x, dst_pt.y, ICN::RECRUIT, 0, 1 );

    dst_pt.y = pos.y + 169;
    fheroes2::Button buttonDn( dst_pt.x, dst_pt.y, ICN::RECRUIT, 2, 3 );

    fheroes2::TimedEventValidator timedButtonUp( [&buttonUp]() { return buttonUp.isPressed(); } );
    fheroes2::TimedEventValidator timedButtonDn( [&buttonDn]() { return buttonDn.isPressed(); } );

    buttonDn.subscribe( &timedButtonDn );
    buttonUp.subscribe( &timedButtonUp );

    const fheroes2::Rect rtWheel( pos.x + 130, pos.y + 155, 100, 30 );

    // Create monster switching arrows
    fheroes2::ButtonSprite monsterSwitchLeft;
    fheroes2::ButtonSprite monsterSwitchRight;

    if ( allowDowngradedMonster && monster0.GetDowngrade() != monster0 ) {
        monsterSwitchLeft.setSprite( fheroes2::AGG::GetICN( ICN::MONSTER_SWITCH_LEFT_ARROW, 0 ), fheroes2::AGG::GetICN( ICN::MONSTER_SWITCH_LEFT_ARROW, 1 ) );
        monsterSwitchRight.setSprite( fheroes2::AGG::GetICN( ICN::MONSTER_SWITCH_RIGHT_ARROW, 0 ), fheroes2::AGG::GetICN( ICN::MONSTER_SWITCH_RIGHT_ARROW, 1 ) );

        dst_pt.x = pos.x + 24;
        dst_pt.y = pos.y + 80;
        monsterSwitchLeft.setPosition( dst_pt.x, dst_pt.y );
        dst_pt.x = pos.x + 121;
        monsterSwitchRight.setPosition( dst_pt.x, dst_pt.y );
    }
    else {
        monsterSwitchLeft.hide();
        monsterSwitchRight.hide();

        monsterSwitchLeft.disable();
        monsterSwitchRight.disable();
    }

    // Render shadows for UI elements.
    buttonOk.drawShadow();
    buttonCancel.drawShadow();
    buttonMax.drawShadow();
    buttonUp.drawShadow();
    buttonDn.drawShadow();
    monsterSwitchLeft.drawShadow();
    monsterSwitchRight.drawShadow();

    // Render Up and Down buttons to restore their initial state later.
    buttonUp.draw();
    buttonDn.draw();

    // Make a copy of background dialog to restore its parts before updating some dialog elements.
    background.resize( windowSize.width, windowSize.height );
    fheroes2::Copy( display, dialogOffset.x, dialogOffset.y, background, 0, 0, windowSize.width, windowSize.height );

    RedrawMonsterInfo( pos, monster, available, true );

    if ( 0 == result ) {
        buttonOk.disable();
        buttonMax.disable();
        buttonMin.disable();
        buttonMax.draw();
    }

    const Funds & funds = kingdom.GetFunds();
    std::string maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, true );
    RedrawCurrentInfo( dialogOffset, result, paymentMonster, paymentCosts, funds, maxmin );

    buttonOk.draw();
    buttonCancel.draw();

    if ( buttonMax.isEnabled() ) {
        buttonMax.draw();
    }
    if ( buttonMin.isEnabled() ) {
        buttonMin.draw();
    }
    monsterSwitchLeft.draw();
    monsterSwitchRight.draw();

    display.render( pos );

    const fheroes2::Rect monsterArea( pos.x + 40, pos.y + 35, 75, 95 );

    auto buttonUpDownRelease = [&display, &background, &dialogOffset]( fheroes2::Button & button ) {
        // When the "Up"/"Down" button is pressed it is shifted 1 pixel down so we need to properly restore the background.
        const fheroes2::Rect roi = button.area();
        button.release();

        fheroes2::Copy( background, roi.x - dialogOffset.x, roi.y - dialogOffset.y, display, roi.x, roi.y, roi.width, roi.height );
        // A the non-pressed button is already on the "background" copy so we do not render the button.
        display.render( roi );
    };

    std::vector<Monster> upgrades = { monster0 };
    while ( upgrades.back().GetDowngrade() != upgrades.back() ) {
        upgrades.emplace_back( upgrades.back().GetDowngrade() );
    }

    // str loop
    while ( le.HandleEvents() ) {
        bool redraw = false;

        if ( buttonOk.isEnabled() ) {
            le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        }
        le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        if ( le.MousePressLeft( buttonUp.area() ) ) {
            buttonUp.drawOnPress();
        }
        else if ( buttonUp.isPressed() ) {
            buttonUpDownRelease( buttonUp );
        }

        if ( le.MousePressLeft( buttonDn.area() ) ) {
            buttonDn.drawOnPress();
        }
        else if ( buttonDn.isPressed() ) {
            buttonUpDownRelease( buttonDn );
        }

        le.MousePressLeft( monsterSwitchLeft.area() ) ? monsterSwitchLeft.drawOnPress() : monsterSwitchLeft.drawOnRelease();
        le.MousePressLeft( monsterSwitchRight.area() ) ? monsterSwitchRight.drawOnPress() : monsterSwitchRight.drawOnRelease();

        if ( buttonMax.isEnabled() ) {
            le.MousePressLeft( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();
        }
        if ( buttonMin.isEnabled() ) {
            le.MousePressLeft( buttonMin.area() ) ? buttonMin.drawOnPress() : buttonMin.drawOnRelease();
        }

        bool updateCost = false;

        if ( allowDowngradedMonster && upgrades.size() > 1 ) {
            if ( le.MouseClickLeft( monsterSwitchLeft.area() ) || le.KeyPress( fheroes2::Key::KEY_LEFT ) ) {
                for ( size_t i = 0; i < upgrades.size(); ++i ) {
                    if ( upgrades[i] == monster ) {
                        if ( i < upgrades.size() - 1 ) {
                            monster = upgrades[i + 1];
                        }
                        else {
                            monster = upgrades[0];
                        }
                        break;
                    }
                }
                updateCost = true;
            }
            else if ( le.MouseClickLeft( monsterSwitchRight.area() ) || le.KeyPress( fheroes2::Key::KEY_RIGHT ) ) {
                for ( size_t i = 0; i < upgrades.size(); ++i ) {
                    if ( upgrades[i] == monster ) {
                        if ( i > 0 ) {
                            monster = upgrades[i - 1];
                        }
                        else {
                            monster = upgrades.back();
                        }
                        break;
                    }
                }
                updateCost = true;
            }
        }

        if ( updateCost ) {
            // Restore the recruit dialog background.
            fheroes2::Copy( background, 0, 0, display, dialogOffset.x, dialogOffset.y, windowSize.width, windowSize.height );

            max = CalculateMax( monster, kingdom, available );
            result = max;
            paymentMonster = monster.GetCost();
            paymentCosts = paymentMonster * result;
            redraw = true;
            maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, true );

            RedrawMonsterInfo( pos, monster, available, true );
        }

        if ( le.MousePressRight( monsterArea ) ) {
            ArmyInfo( Troop( monster, available ), ZERO );

            // Perform a full rendering to properly restore the parts of the screen outside of this dialog
            display.render();
            continue;
        }

        if ( le.MouseClickLeft( monsterArea ) ) {
            ArmyInfo( Troop( monster, available ), BUTTONS );

            // Perform a full rendering to properly restore the parts of the screen outside of this dialog
            display.render();
            continue;
        }

        if ( fheroes2::PressIntKey( max, result ) ) {
            paymentCosts = paymentMonster * result;
            redraw = true;
            maxmin.clear();

            if ( result == max ) {
                maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, true );
            }
            else if ( result == 1 ) {
                maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, false );
            }
        }

        if ( ( le.MouseWheelUp( rtWheel ) || le.MouseClickLeft( buttonUp.area() ) || le.KeyPress( fheroes2::Key::KEY_UP ) || timedButtonUp.isDelayPassed() )
             && result < max ) {
            ++result;
            paymentCosts += paymentMonster;
            redraw = true;
            maxmin.clear();

            if ( result == max ) {
                maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, true );
            }
            else if ( result == 1 ) {
                maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, false );
            }
        }
        else if ( ( le.MouseWheelDn( rtWheel ) || le.MouseClickLeft( buttonDn.area() ) || le.KeyPress( fheroes2::Key::KEY_DOWN ) || timedButtonDn.isDelayPassed() )
                  && result ) {
            --result;
            paymentCosts -= paymentMonster;
            redraw = true;
            maxmin.clear();

            if ( result == max ) {
                maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, true );
            }
            else if ( result == 1 ) {
                maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, false );
            }
        }
        else if ( buttonMax.isEnabled() && le.MouseClickLeft( buttonMax.area() ) && result != max ) {
            maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, true );
            result = max;
            paymentCosts = paymentMonster * max;
            redraw = true;
        }
        else if ( buttonMin.isEnabled() && le.MouseClickLeft( buttonMin.area() ) && result != 1 ) {
            maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, false );
            result = 1;
            paymentCosts = paymentMonster;
            redraw = true;
        }
        else if ( le.MousePressRight( buttonOk.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Okay" ), _( "Recruit selected monsters." ), 0 );
        }
        else if ( le.MousePressRight( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), 0 );
        }
        else if ( buttonMax.isEnabled() && le.MousePressRight( buttonMax.area() ) ) {
            fheroes2::showStandardTextMessage( _( "MAX" ), _( "Select maximum monsters to be recruited." ), 0 );
        }
        else if ( buttonMin.isEnabled() && le.MousePressRight( buttonMin.area() ) ) {
            fheroes2::showStandardTextMessage( _( "MIN" ), _( "Select only 1 monster to be recruited." ), 0 );
        }

        if ( redraw ) {
            RedrawCurrentInfo( dialogOffset, result, paymentMonster, paymentCosts, funds, maxmin, background );

            if ( 0 == result ) {
                buttonOk.disable();
                buttonOk.draw();
            }
            else {
                buttonOk.enable();
                buttonOk.draw();
            }

            buttonCancel.draw();

            if ( buttonMax.isEnabled() || max == 0 ) {
                buttonMax.draw();
            }
            else if ( buttonMin.isEnabled() ) {
                buttonMin.draw();
            }

            monsterSwitchLeft.draw();
            monsterSwitchRight.draw();

            display.render( pos );
        }

        if ( buttonOk.isEnabled() && ( le.MouseClickLeft( buttonOk.area() ) || ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) ) ) {
            break;
        }

        if ( le.MouseClickLeft( buttonCancel.area() ) || ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) ) {
            result = 0;
            break;
        }
    }

    return { monster, result };
}

void Dialog::DwellingInfo( const Monster & monster, const uint32_t available )
{
    const fheroes2::Size window{ 289, 139 };

    fheroes2::Display & display = fheroes2::Display::instance();

    // Set cursor.
    const CursorRestorer cursorRestorer( false, Cursor::POINTER );

    const fheroes2::Point dialogOffset( ( display.width() - window.width ) / 2, display.height() / 2 - display.DEFAULT_HEIGHT / 2 + BORDERWIDTH );
    const fheroes2::Rect pos( dialogOffset.x - BORDERWIDTH, dialogOffset.y - BORDERWIDTH, window.width + 2 * BORDERWIDTH, window.height + 2 * BORDERWIDTH );

    const fheroes2::StandardWindow background( dialogOffset.x, dialogOffset.y, window.width, window.height, true, display );

    drawRecruitWindow( display, pos.getPosition() );
    RedrawMonsterInfo( pos, monster, available, false );

    display.render( pos );

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() && le.MousePressRight() ) {
        // Do nothing.
    }
}
