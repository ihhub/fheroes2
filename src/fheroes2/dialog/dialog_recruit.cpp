/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
#include "payment.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "world.h"

namespace
{
    void drawRecruitWindow( fheroes2::Image & output, const fheroes2::Point & offset, const int icnId )
    {
        const fheroes2::Sprite & window = fheroes2::AGG::GetICN( icnId, 0 );
        fheroes2::Blit( window, output, offset.x, offset.y );

        const fheroes2::Rect backgroundArea( 144, 55, 120, 12 );

        // Mask 'hardcoded' title to allow translation text appear.
        const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( ICN::BLDGXTRA, 0 );
        fheroes2::Copy( backgroundImage, 6, 59, output, offset.x + backgroundArea.x, offset.y + backgroundArea.y, backgroundArea.width / 2, backgroundArea.height );
        fheroes2::Copy( backgroundImage, 6 + 65, 59, output, offset.x + backgroundArea.x + backgroundArea.width / 2, offset.y + backgroundArea.y,
                        backgroundArea.width / 2, backgroundArea.height );

        const fheroes2::Text text( _( "Cost per troop:" ), fheroes2::FontType::smallWhite() );
        text.draw( offset.x + backgroundArea.x + ( backgroundArea.width - text.width() ) / 2, offset.y + backgroundArea.y + 2, output );
    }
}

void RedrawCurrentInfo( const fheroes2::Point & pos, uint32_t result, const payment_t & paymentMonster, const payment_t & paymentCosts, const Funds & funds,
                        const std::string & label )
{
    Text text;

    text.Set( std::to_string( result ), Font::BIG );
    text.Blit( pos.x + 167 - text.w() / 2, pos.y + 161 );
    const std::string sgold = std::to_string( paymentCosts.gold ) + " " + "(" + std::to_string( funds.gold - paymentCosts.gold ) + ")";
    int rsext = paymentMonster.GetValidItems() & ~Resource::GOLD;

    if ( rsext ) {
        text.Set( sgold, Font::SMALL );
        text.Blit( pos.x + 133 - text.w() / 2, pos.y + 228 );

        text.Set( std::to_string( paymentCosts.Get( rsext ) ) + " " + "(" + std::to_string( funds.Get( rsext ) - paymentCosts.Get( rsext ) ) + ")", Font::SMALL );
        text.Blit( pos.x + 195 - text.w() / 2, pos.y + 228 );
    }
    else {
        text.Set( sgold, Font::SMALL );
        text.Blit( pos.x + 160 - text.w() / 2, pos.y + 228 );
    }

    text.Set( label, Font::SMALL );
    text.Blit( pos.x + 167 - text.w() / 2, pos.y + 180 );
}

void RedrawResourceInfo( const fheroes2::Image & sres, const fheroes2::Point & pos, int32_t value, int32_t px1, int32_t py1, int32_t px2, int32_t py2 )
{
    fheroes2::Point dst_pt( pos.x + px1, pos.y + py1 );
    fheroes2::Blit( sres, fheroes2::Display::instance(), dst_pt.x, dst_pt.y );

    const Text text( std::to_string( value ), Font::SMALL );
    dst_pt.x = pos.x + px2 - text.w() / 2;
    dst_pt.y = pos.y + py2;
    text.Blit( dst_pt.x, dst_pt.y );
}

void RedrawMonsterInfo( const fheroes2::Rect & pos, const Monster & monster, uint32_t available, bool showTotalSum )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const payment_t paymentMonster = monster.GetCost();
    const bool extres = 2 == paymentMonster.GetValidItemsCount();

    // text recruit monster
    std::string str = _( "Recruit %{name}" );
    StringReplace( str, "%{name}", monster.GetMultiName() );
    Text text( str, Font::YELLOW_BIG );
    fheroes2::Point dst_pt( pos.x + ( pos.width - text.w() ) / 2, pos.y + 25 );
    text.Blit( dst_pt.x, dst_pt.y );

    // sprite monster
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

    // info resource
    // gold
    const fheroes2::Sprite & sgold = fheroes2::AGG::GetICN( ICN::RESOURCE, 6 );
    RedrawResourceInfo( sgold, pos.getPosition(), paymentMonster.gold, extres ? 150 : 175, 75, extres ? 183 : 205, 103 );
    if ( showTotalSum ) {
        dst_pt.x = pos.x + ( extres ? 105 : 130 );
        dst_pt.y = pos.y + 200;
        fheroes2::Blit( sgold, display, dst_pt.x, dst_pt.y );
    }

    if ( paymentMonster.crystal ) {
        const fheroes2::Sprite & sres = fheroes2::AGG::GetICN( ICN::RESOURCE, 4 );
        RedrawResourceInfo( sres, pos.getPosition(), paymentMonster.crystal, 222, 69, 240, 103 );
        if ( showTotalSum ) {
            dst_pt.x = pos.x + 177;
            dst_pt.y = pos.y + 194;
            fheroes2::Blit( sres, display, dst_pt.x, dst_pt.y );
        }
    }
    else if ( paymentMonster.mercury ) {
        const fheroes2::Sprite & sres = fheroes2::AGG::GetICN( ICN::RESOURCE, 1 );
        RedrawResourceInfo( sres, pos.getPosition(), paymentMonster.mercury, 225, 72, 240, 103 );
        if ( showTotalSum ) {
            dst_pt.x = pos.x + 180;
            dst_pt.y = pos.y + 197;
            fheroes2::Blit( sres, display, dst_pt.x, dst_pt.y );
        }
    }
    else if ( paymentMonster.wood ) {
        const fheroes2::Sprite & sres = fheroes2::AGG::GetICN( ICN::RESOURCE, 0 );
        RedrawResourceInfo( sres, pos.getPosition(), paymentMonster.wood, 225, 72, 240, 103 );
        if ( showTotalSum ) {
            dst_pt.x = pos.x + 180;
            dst_pt.y = pos.y + 197;
            fheroes2::Blit( sres, display, dst_pt.x, dst_pt.y );
        }
    }
    else if ( paymentMonster.ore ) {
        const fheroes2::Sprite & sres = fheroes2::AGG::GetICN( ICN::RESOURCE, 2 );
        RedrawResourceInfo( sres, pos.getPosition(), paymentMonster.ore, 225, 72, 240, 103 );
        if ( showTotalSum ) {
            dst_pt.x = pos.x + 180;
            dst_pt.y = pos.y + 197;
            fheroes2::Blit( sres, display, dst_pt.x, dst_pt.y );
        }
    }
    else if ( paymentMonster.sulfur ) {
        const fheroes2::Sprite & sres = fheroes2::AGG::GetICN( ICN::RESOURCE, 3 );
        RedrawResourceInfo( sres, pos.getPosition(), paymentMonster.sulfur, 225, 75, 240, 103 );
        if ( showTotalSum ) {
            dst_pt.x = pos.x + 180;
            dst_pt.y = pos.y + 200;
            fheroes2::Blit( sres, display, dst_pt.x, dst_pt.y );
        }
    }
    else if ( paymentMonster.gems ) {
        const fheroes2::Sprite & sres = fheroes2::AGG::GetICN( ICN::RESOURCE, 5 );
        RedrawResourceInfo( sres, pos.getPosition(), paymentMonster.gems, 225, 75, 240, 103 );
        if ( showTotalSum ) {
            dst_pt.x = pos.x + 180;
            dst_pt.y = pos.y + 200;
            fheroes2::Blit( sres, display, dst_pt.x, dst_pt.y );
        }
    }

    str = _( "Available: %{count}" );
    StringReplace( str, "%{count}", available );
    text.Set( str, Font::SMALL );
    text.Blit( pos.x + 80 - text.w() / 2, pos.y + 135 );
}

void RedrawStaticInfo( const fheroes2::Rect & pos, const Monster & monster, uint32_t available, const int windowIcnId )
{
    drawRecruitWindow( fheroes2::Display::instance(), { pos.x, pos.y }, windowIcnId );

    RedrawMonsterInfo( pos, monster, available, true );

    // text number buy
    Text text;
    text.Set( _( "Number to buy:" ), Font::SMALL );
    text.Blit( pos.x + 29, pos.y + 163 );
}

const char * SwitchMaxMinButtons( fheroes2::ButtonBase & btnMax, fheroes2::ButtonBase & btnMin, bool max )
{
    if ( btnMax.isEnabled() || btnMin.isEnabled() ) {
        if ( max ) {
            btnMax.disable();
            btnMin.enable();
        }
        else {
            btnMin.disable();
            btnMax.enable();
        }

        return max ? _( "Max" ) : _( "Min" );
    }

    return "";
}

uint32_t CalculateMax( const Monster & monster, const Kingdom & kingdom, uint32_t available )
{
    uint32_t max = 0;
    while ( kingdom.AllowPayment( monster.GetCost() * ( max + 1 ) ) && ( max + 1 ) <= available )
        ++max;

    return max;
}

Troop Dialog::RecruitMonster( const Monster & monster0, uint32_t available, const bool allowDowngradedMonster, const int32_t windowOffsetY )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    // calculate max count
    Monster monster = monster0;
    payment_t paymentMonster = monster.GetCost();
    const Kingdom & kingdom = world.GetKingdom( Settings::Get().CurrentColor() );

    uint32_t max = CalculateMax( monster, kingdom, available );
    uint32_t result = max;

    payment_t paymentCosts( paymentMonster * result );

    const int windowIcnId = ICN::RECRBKG;

    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( windowIcnId, 0 );
    const fheroes2::Sprite & boxShadow = fheroes2::AGG::GetICN( windowIcnId, 1 );

    const fheroes2::Point dialogOffset( ( display.width() - box.width() ) / 2, ( display.height() - box.height() ) / 2 + windowOffsetY );
    const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

    fheroes2::ImageRestorer back( display, shadowOffset.x, shadowOffset.y, box.width() + BORDERWIDTH, box.height() + BORDERWIDTH );
    const fheroes2::Rect pos( dialogOffset.x, dialogOffset.y, box.width(), box.height() );

    fheroes2::Blit( boxShadow, display, pos.x - BORDERWIDTH, pos.y + BORDERWIDTH );

    drawRecruitWindow( display, { pos.x, pos.y }, windowIcnId );

    RedrawStaticInfo( pos, monster, available, windowIcnId );

    // buttons
    fheroes2::Point dst_pt;

    dst_pt.x = pos.x + 34;
    dst_pt.y = pos.y + 249;
    fheroes2::Button buttonOk( dst_pt.x, dst_pt.y, ICN::BUTTON_SMALL_OKAY_GOOD, 0, 1 );

    dst_pt.x = pos.x + 187;
    dst_pt.y = pos.y + 249;
    fheroes2::Button buttonCancel( dst_pt.x, dst_pt.y, ICN::BUTTON_SMALL_CANCEL_GOOD, 0, 1 );

    dst_pt.x = pos.x + 229;
    dst_pt.y = pos.y + 156;
    fheroes2::ButtonSprite buttonMax( dst_pt.x, dst_pt.y, fheroes2::AGG::GetICN( ICN::BUTTON_SMALL_MAX_GOOD, 0 ), fheroes2::AGG::GetICN( ICN::BUTTON_SMALL_MAX_GOOD, 1 ),
                                      fheroes2::AGG::GetICN( ICN::MAX_DISABLED_BUTTON, 0 ) );
    fheroes2::Button buttonMin( dst_pt.x, dst_pt.y, ICN::BUTTON_SMALL_MIN_GOOD, 0, 1 );

    dst_pt.x = pos.x + 205;
    dst_pt.y = pos.y + 154;
    fheroes2::Button buttonUp( dst_pt.x, dst_pt.y, ICN::RECRUIT, 0, 1 );

    dst_pt.x = pos.x + 205;
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

        monsterSwitchLeft.setPosition( pos.x + 24, pos.y + 80 );
        monsterSwitchRight.setPosition( pos.x + 121, pos.y + 80 );
    }
    else {
        monsterSwitchLeft.hide();
        monsterSwitchRight.hide();

        monsterSwitchLeft.disable();
        monsterSwitchRight.disable();
    }

    const fheroes2::Rect monsterArea( pos.x + 40, pos.y + 35, 75, 95 );

    if ( 0 == result ) {
        buttonOk.disable();
        buttonMax.disable();
        buttonMin.disable();
        buttonMax.draw();
    }

    const Funds & funds = kingdom.GetFunds();
    std::string maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, true );
    RedrawCurrentInfo( pos.getPosition(), result, paymentMonster, paymentCosts, funds, maxmin );

    buttonOk.draw();
    buttonCancel.draw();
    if ( buttonMax.isEnabled() ) {
        buttonMax.draw();
    }
    if ( buttonMin.isEnabled() ) {
        buttonMin.draw();
    }
    buttonUp.draw();
    buttonDn.draw();
    monsterSwitchLeft.draw();
    monsterSwitchRight.draw();

    display.render( back.rect() );

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
        le.MousePressLeft( buttonUp.area() ) ? buttonUp.drawOnPress() : buttonUp.drawOnRelease();
        le.MousePressLeft( buttonDn.area() ) ? buttonDn.drawOnPress() : buttonDn.drawOnRelease();

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
            max = CalculateMax( monster, kingdom, available );
            result = max;
            paymentMonster = monster.GetCost();
            paymentCosts = paymentMonster * result;
            redraw = true;
            maxmin = SwitchMaxMinButtons( buttonMax, buttonMin, true );
        }

        bool skipHotKeyCheck = false;

        if ( le.MousePressRight( monsterArea ) ) {
            ArmyInfo( Troop( monster, available ), READONLY );

            // Perform a full rendering to properly restore the parts of the screen outside of this dialog
            display.render();
        }
        else if ( le.MouseClickLeft( monsterArea ) ) {
            ArmyInfo( Troop( monster, available ), READONLY | BUTTONS );

            skipHotKeyCheck = true;

            // Perform a full rendering to properly restore the parts of the screen outside of this dialog
            display.render();
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
            RedrawStaticInfo( pos, monster, available, windowIcnId );
            RedrawCurrentInfo( pos.getPosition(), result, paymentMonster, paymentCosts, funds, maxmin );

            if ( 0 == result ) {
                buttonOk.disable();
                buttonOk.draw();
            }
            else {
                buttonOk.enable();
                buttonOk.draw();
            }

            buttonCancel.draw();

            if ( buttonMax.isEnabled() || max == 0 )
                buttonMax.draw();
            if ( buttonMin.isEnabled() )
                buttonMin.draw();

            monsterSwitchLeft.draw();
            monsterSwitchRight.draw();

            display.render( back.rect() );
        }

        if ( buttonOk.isEnabled() && ( le.MouseClickLeft( buttonOk.area() ) || ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) && !skipHotKeyCheck ) ) ) {
            break;
        }

        if ( le.MouseClickLeft( buttonCancel.area() ) || ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) && !skipHotKeyCheck ) ) {
            result = 0;
            break;
        }
    }

    back.restore();
    display.render( back.rect() );

    return Troop( monster, result );
}

void Dialog::DwellingInfo( const Monster & monster, uint32_t available )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( false, Cursor::POINTER );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int icnId = isEvilInterface ? ICN::RECR2BKG_EVIL : ICN::RECR2BKG;

    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( icnId, 0 );
    const fheroes2::Sprite & boxShadow = fheroes2::AGG::GetICN( icnId, 1 );

    const fheroes2::Point dialogOffset( ( display.width() - box.width() ) / 2, display.height() / 2 - display.DEFAULT_HEIGHT / 2 + BORDERWIDTH );
    const fheroes2::Point shadowOffset( dialogOffset.x - BORDERWIDTH, dialogOffset.y );

    fheroes2::ImageRestorer back( display, shadowOffset.x, shadowOffset.y, box.width() + BORDERWIDTH, box.height() + BORDERWIDTH );
    const fheroes2::Rect pos( dialogOffset.x, dialogOffset.y, box.width(), box.height() );

    fheroes2::Blit( boxShadow, display, pos.x - BORDERWIDTH, pos.y + BORDERWIDTH );

    drawRecruitWindow( display, { pos.x, pos.y }, icnId );

    LocalEvent & le = LocalEvent::Get();

    RedrawMonsterInfo( pos, monster, available, false );

    display.render();

    while ( le.HandleEvents() && le.MousePressRight() )
        ;

    back.restore();
    display.render();
}
