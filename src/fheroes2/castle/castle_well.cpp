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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "agg_image.h"
#include "army.h"
#include "army_troop.h"
#include "battle_cell.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "localevent.h"
#include "math_base.h"
#include "monster.h"
#include "monster_anim.h"
#include "payment.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "speed.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    const int32_t bottomBarOffsetY = 461;

    uint32_t howManyRecruitMonster( const Castle & castle, Troops & tempCastleArmy, Troops & tempHeroArmy, const uint32_t dw, const Funds & add, Funds & res )
    {
        const Monster monsters( castle.GetRace(), castle.GetActualDwelling( dw ) );
        if ( !tempCastleArmy.CanJoinTroop( monsters ) && !tempHeroArmy.CanJoinTroop( monsters ) )
            return 0;

        uint32_t count = castle.getMonstersInDwelling( dw );
        payment_t payment;

        const Kingdom & kingdom = castle.GetKingdom();

        while ( count ) {
            payment = monsters.GetCost() * count;
            res = payment;
            payment += add;
            if ( kingdom.AllowPayment( payment ) )
                break;
            --count;
        }

        if ( count > 0 ) {
            if ( tempCastleArmy.CanJoinTroop( monsters ) )
                tempCastleArmy.JoinTroop( monsters, count, false );
            else if ( tempHeroArmy.CanJoinTroop( monsters ) )
                tempHeroArmy.JoinTroop( monsters, count, false );
        }

        return count;
    }

    building_t getPressedBuildingHotkey()
    {
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_1 ) ) {
            return DWELLING_MONSTER1;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_2 ) ) {
            return DWELLING_MONSTER2;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_3 ) ) {
            return DWELLING_MONSTER3;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_4 ) ) {
            return DWELLING_MONSTER4;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_5 ) ) {
            return DWELLING_MONSTER5;
        }
        if ( HotKeyPressEvent( Game::HotKeyEvent::TOWN_DWELLING_LEVEL_6 ) ) {
            return DWELLING_MONSTER6;
        }

        return BUILD_NOTHING;
    }
}

void Castle::recruitCastleMax( const Troops & currentCastleArmy, const std::vector<uint32_t> & allCastleDwellings )
{
    std::vector<Troop> totalRecruitmentResult;
    Funds currentMonsterCost;
    Funds totalMonstersCost;
    std::string monstersRecruitedText;

    Troops tempCastleArmy( currentCastleArmy );
    Troops tempGuestArmy;

    const Heroes * hero = GetHero();

    if ( hero ) {
        tempGuestArmy.Insert( hero->GetArmy().getTroops() );
    }

    for ( const uint32_t dwellingType : allCastleDwellings ) {
        const uint32_t recruitableNumber = howManyRecruitMonster( *this, tempCastleArmy, tempGuestArmy, dwellingType, totalMonstersCost, currentMonsterCost );

        if ( recruitableNumber == 0 ) {
            continue;
        }
        const Monster recruitableMonster( race, GetActualDwelling( dwellingType ) );

        totalRecruitmentResult.emplace_back( recruitableMonster, recruitableNumber );
        totalMonstersCost += currentMonsterCost;

        monstersRecruitedText += recruitableMonster.GetPluralName( recruitableNumber );
        monstersRecruitedText += " : ";
        monstersRecruitedText += std::to_string( recruitableNumber );
        monstersRecruitedText += '\n';
    }

    const fheroes2::FontType normalWhite = fheroes2::FontType::normalWhite();

    if ( monstersRecruitedText.empty() ) {
        bool isCreaturePresent = false;
        bool canAffordOneCreature = false;

        for ( uint32_t currentDwelling = DWELLING_MONSTER1; currentDwelling <= DWELLING_MONSTER6; currentDwelling <<= 1 ) {
            if ( getMonstersInDwelling( currentDwelling ) > 0 ) {
                const Monster monsters( race, currentDwelling );
                const payment_t payment = monsters.GetCost();

                if ( GetKingdom().AllowPayment( payment ) ) {
                    canAffordOneCreature = true;
                }
                isCreaturePresent = true;
                break;
            }
        }
        if ( isCreaturePresent ) {
            if ( !canAffordOneCreature ) {
                fheroes2::showMessage( fheroes2::Text( "", {} ), fheroes2::Text( _( "Not enough resources to recruit creatures." ), normalWhite ), Dialog::OK );
            }
            else {
                fheroes2::showMessage( fheroes2::Text( "", {} ), fheroes2::Text( _( "You are unable to recruit at this time, your ranks are full." ), normalWhite ),
                                       Dialog::OK );
            }
        }
        else {
            fheroes2::showMessage( fheroes2::Text( "", {} ), fheroes2::Text( _( "No creatures available for purchase." ), normalWhite ), Dialog::OK );
        }
    }
    else if ( fheroes2::showResourceMessage( fheroes2::Text( _( "Recruit Creatures" ), fheroes2::FontType::normalYellow() ),
                                             fheroes2::Text( monstersRecruitedText, normalWhite ), Dialog::YES | Dialog::NO, totalMonstersCost )
              == Dialog::YES ) {
        for ( const Troop & troop : totalRecruitmentResult ) {
            RecruitMonster( troop, false );
        }
    }
}

void Castle::OpenWell()
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::ImageRestorer restorer( display, ( display.width() - fheroes2::Display::DEFAULT_WIDTH ) / 2,
                                            ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2, fheroes2::Display::DEFAULT_WIDTH,
                                            fheroes2::Display::DEFAULT_HEIGHT );

    const fheroes2::Point cur_pt( restorer.x(), restorer.y() );
    fheroes2::Point dst_pt( cur_pt.x, cur_pt.y );

    // button exit
    dst_pt.x = cur_pt.x + 578;
    dst_pt.y = cur_pt.y + bottomBarOffsetY;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::WELLXTRA, 0, 1 );

    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y + bottomBarOffsetY;
    fheroes2::Button buttonMax( dst_pt.x, dst_pt.y, ICN::BUYMAX, 0, 1 );

    const fheroes2::Rect rectMonster1( cur_pt.x + 20, cur_pt.y + 18, 288, 124 );
    const fheroes2::Rect rectMonster2( cur_pt.x + 20, cur_pt.y + 168, 288, 124 );
    const fheroes2::Rect rectMonster3( cur_pt.x + 20, cur_pt.y + 318, 288, 124 );
    const fheroes2::Rect rectMonster4( cur_pt.x + 334, cur_pt.y + 18, 288, 124 );
    const fheroes2::Rect rectMonster5( cur_pt.x + 334, cur_pt.y + 168, 288, 124 );
    const fheroes2::Rect rectMonster6( cur_pt.x + 334, cur_pt.y + 318, 288, 124 );

    buttonExit.draw();

    std::vector<fheroes2::RandomMonsterAnimation> monsterAnimInfo;
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER1 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER2 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER3 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER4 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER5 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER6 ) ) );

    WellRedrawInfoArea( cur_pt, monsterAnimInfo );

    buttonMax.draw();

    std::vector<uint32_t> allDwellings;
    allDwellings.reserve( 6 );
    allDwellings.push_back( DWELLING_MONSTER6 );
    allDwellings.push_back( DWELLING_MONSTER5 );
    allDwellings.push_back( DWELLING_MONSTER4 );
    allDwellings.push_back( DWELLING_MONSTER3 );
    allDwellings.push_back( DWELLING_MONSTER2 );
    allDwellings.push_back( DWELLING_MONSTER1 );

    display.render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents( Game::isDelayNeeded( { Game::CASTLE_UNIT_DELAY } ) ) ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        le.MousePressLeft( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();
        const building_t pressedHotkeyBuildingID = getPressedBuildingHotkey();

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
            break;
        }
        if ( le.MouseClickLeft( buttonMax.area() ) || HotKeyPressEvent( Game::HotKeyEvent::TOWN_WELL_BUY_ALL ) ) {
            const Troops & currentArmy = GetArmy();
            recruitCastleMax( currentArmy, allDwellings );
        }
        else if ( ( building & DWELLING_MONSTER1 ) && ( le.MouseClickLeft( rectMonster1 ) || pressedHotkeyBuildingID == DWELLING_MONSTER1 ) )
            RecruitMonster( Dialog::RecruitMonster( { race, GetActualDwelling( DWELLING_MONSTER1 ) }, dwelling[0], true, 2 ) );
        else if ( ( building & DWELLING_MONSTER2 ) && ( le.MouseClickLeft( rectMonster2 ) || pressedHotkeyBuildingID == DWELLING_MONSTER2 ) )
            RecruitMonster( Dialog::RecruitMonster( { race, GetActualDwelling( DWELLING_MONSTER2 ) }, dwelling[1], true, 2 ) );
        else if ( ( building & DWELLING_MONSTER3 ) && ( le.MouseClickLeft( rectMonster3 ) || pressedHotkeyBuildingID == DWELLING_MONSTER3 ) )
            RecruitMonster( Dialog::RecruitMonster( { race, GetActualDwelling( DWELLING_MONSTER3 ) }, dwelling[2], true, 2 ) );
        else if ( ( building & DWELLING_MONSTER4 ) && ( le.MouseClickLeft( rectMonster4 ) || pressedHotkeyBuildingID == DWELLING_MONSTER4 ) )
            RecruitMonster( Dialog::RecruitMonster( { race, GetActualDwelling( DWELLING_MONSTER4 ) }, dwelling[3], true, 2 ) );
        else if ( ( building & DWELLING_MONSTER5 ) && ( le.MouseClickLeft( rectMonster5 ) || pressedHotkeyBuildingID == DWELLING_MONSTER5 ) )
            RecruitMonster( Dialog::RecruitMonster( { race, GetActualDwelling( DWELLING_MONSTER5 ) }, dwelling[4], true, 2 ) );
        else if ( ( building & DWELLING_MONSTER6 ) && ( le.MouseClickLeft( rectMonster6 ) || pressedHotkeyBuildingID == DWELLING_MONSTER6 ) )
            RecruitMonster( Dialog::RecruitMonster( { race, GetActualDwelling( DWELLING_MONSTER6 ) }, dwelling[5], true, 2 ) );
        else if ( ( building & DWELLING_MONSTER1 ) && le.MousePressRight( rectMonster1 ) )
            Dialog::DwellingInfo( { race, GetActualDwelling( DWELLING_MONSTER1 ) }, dwelling[0] );
        else if ( ( building & DWELLING_MONSTER2 ) && le.MousePressRight( rectMonster2 ) )
            Dialog::DwellingInfo( { race, GetActualDwelling( DWELLING_MONSTER2 ) }, dwelling[1] );
        else if ( ( building & DWELLING_MONSTER3 ) && le.MousePressRight( rectMonster3 ) )
            Dialog::DwellingInfo( { race, GetActualDwelling( DWELLING_MONSTER3 ) }, dwelling[2] );
        else if ( ( building & DWELLING_MONSTER4 ) && le.MousePressRight( rectMonster4 ) )
            Dialog::DwellingInfo( { race, GetActualDwelling( DWELLING_MONSTER4 ) }, dwelling[3] );
        else if ( ( building & DWELLING_MONSTER5 ) && le.MousePressRight( rectMonster5 ) )
            Dialog::DwellingInfo( { race, GetActualDwelling( DWELLING_MONSTER5 ) }, dwelling[4] );
        else if ( ( building & DWELLING_MONSTER6 ) && le.MousePressRight( rectMonster6 ) )
            Dialog::DwellingInfo( { race, GetActualDwelling( DWELLING_MONSTER6 ) }, dwelling[5] );

        if ( Game::validateAnimationDelay( Game::CASTLE_UNIT_DELAY ) ) {
            WellRedrawInfoArea( cur_pt, monsterAnimInfo );

            for ( size_t i = 0; i < monsterAnimInfo.size(); ++i )
                monsterAnimInfo[i].increment();

            buttonMax.draw();
            display.render();
        }

        if ( le.MousePressRight( buttonExit.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Exit" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Exit this menu." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }

        if ( le.MousePressRight( buttonMax.area() ) ) {
            fheroes2::showMessage( fheroes2::Text( _( "Max" ), fheroes2::FontType::normalYellow() ),
                                   fheroes2::Text( _( "Hire all creatures in the town." ), fheroes2::FontType::normalWhite() ), Dialog::ZERO );
        }
    }
}

void Castle::WellRedrawInfoArea( const fheroes2::Point & cur_pt, const std::vector<fheroes2::RandomMonsterAnimation> & monsterAnimInfo ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    fheroes2::Blit( fheroes2::AGG::GetICN( isEvilInterface ? ICN::WELLBKG_EVIL : ICN::WELLBKG, 0 ), display, cur_pt.x, cur_pt.y );

    fheroes2::Point pt;

    const fheroes2::FontType statsFontType = fheroes2::FontType::smallWhite();

    const fheroes2::Sprite & button = fheroes2::AGG::GetICN( ICN::BUYMAX, 0 );
    const fheroes2::Rect src_rt( 0, bottomBarOffsetY, button.width(), 19 );

    // The original ICN::WELLBKG image has incorrect bottom message bar with no yellow outline. Also the original graphics did not have MAX button.
    const int32_t allowedBottomBarWidth = 578 - button.width();
    const fheroes2::Sprite & bottomBar = fheroes2::AGG::GetICN( ICN::SMALLBAR, 0 );

    fheroes2::Blit( bottomBar, 0, 0, display, cur_pt.x + button.width(), cur_pt.y + bottomBarOffsetY, allowedBottomBarWidth / 2, bottomBar.height() );
    fheroes2::Blit( bottomBar, bottomBar.width() - ( allowedBottomBarWidth - allowedBottomBarWidth / 2 ) - 1, 0, display,
                    cur_pt.x + button.width() + allowedBottomBarWidth / 2, cur_pt.y + bottomBarOffsetY, allowedBottomBarWidth - allowedBottomBarWidth / 2,
                    bottomBar.height() );

    fheroes2::Text text( _( "Town Population Information and Statistics" ), fheroes2::FontType() );
    fheroes2::Point dst_pt( cur_pt.x + 315 - text.width() / 2, cur_pt.y + 464 );
    text.draw( dst_pt.x, dst_pt.y, display );

    uint32_t dw = DWELLING_MONSTER1;
    size_t monsterId = 0u;

    while ( dw <= DWELLING_MONSTER6 ) {
        bool present = false;
        uint32_t dw_orig = DWELLING_MONSTER1;
        uint32_t icnindex = 0;
        uint32_t available = 0;

        switch ( dw ) {
        case DWELLING_MONSTER1:
            pt.x = cur_pt.x;
            pt.y = cur_pt.y + 1;
            present = ( DWELLING_MONSTER1 & building ) != 0;
            icnindex = 19;
            available = dwelling[0];
            break;
        case DWELLING_MONSTER2:
            pt.x = cur_pt.x;
            pt.y = cur_pt.y + 151;
            present = ( DWELLING_MONSTER2 & building ) != 0;
            dw_orig = GetActualDwelling( DWELLING_MONSTER2 );
            icnindex = DWELLING_UPGRADE2 & building ? 25 : 20;
            available = dwelling[1];
            break;
        case DWELLING_MONSTER3:
            pt.x = cur_pt.x;
            pt.y = cur_pt.y + 301;
            present = ( DWELLING_MONSTER3 & building ) != 0;
            dw_orig = GetActualDwelling( DWELLING_MONSTER3 );
            icnindex = DWELLING_UPGRADE3 & building ? 26 : 21;
            available = dwelling[2];
            break;
        case DWELLING_MONSTER4:
            pt.x = cur_pt.x + 314;
            pt.y = cur_pt.y + 1;
            present = ( DWELLING_MONSTER4 & building ) != 0;
            dw_orig = GetActualDwelling( DWELLING_MONSTER4 );
            icnindex = DWELLING_UPGRADE4 & building ? 27 : 22;
            available = dwelling[3];
            break;
        case DWELLING_MONSTER5:
            pt.x = cur_pt.x + 314;
            pt.y = cur_pt.y + 151;
            present = ( DWELLING_MONSTER5 & building ) != 0;
            dw_orig = GetActualDwelling( DWELLING_MONSTER5 );
            icnindex = DWELLING_UPGRADE5 & building ? 28 : 23;
            available = dwelling[4];
            break;
        case DWELLING_MONSTER6:
            pt.x = cur_pt.x + 314;
            pt.y = cur_pt.y + 301;
            present = ( DWELLING_MONSTER6 & building ) != 0;
            dw_orig = GetActualDwelling( DWELLING_MONSTER6 );
            icnindex = DWELLING_UPGRADE7 & building ? 30 : ( DWELLING_UPGRADE6 & building ? 29 : 24 );
            available = dwelling[5];
            break;
        default:
            break;
        }

        const Monster monster( race, dw_orig );

        // sprite
        dst_pt.x = pt.x + 21;
        dst_pt.y = pt.y + 35;
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::Get4Building( race ), icnindex ), display, dst_pt.x, dst_pt.y );

        // monster dwelling name
        text.set( GetStringBuilding( dw_orig, race ), statsFontType );
        dst_pt.x = pt.x + 86 - text.width() / 2;
        dst_pt.y = pt.y + 104;
        text.draw( dst_pt.x, dst_pt.y, display );

        // creature name
        text.set( monster.GetMultiName(), statsFontType );
        dst_pt.x = pt.x + 122 - text.width() / 2;
        dst_pt.y = pt.y + 19;
        text.draw( dst_pt.x, dst_pt.y, display );

        // attack
        std::string str;
        str = _( "Attack" );
        str += ": ";
        str += std::to_string( monster.GetAttack() );

        text.set( str, statsFontType );

        const int32_t statsOffsetX = 269;
        const int32_t statsInitialOffsetY = 22;
        int32_t statsOffsetY = statsInitialOffsetY;

        dst_pt.x = pt.x + statsOffsetX - text.width() / 2;
        dst_pt.y = pt.y + statsOffsetY;
        text.draw( dst_pt.x, dst_pt.y, display );
        statsOffsetY += text.height( text.width() );

        // defense
        str = _( "Defense" );
        str += ": ";
        str += std::to_string( monster.GetDefense() );

        text.set( str, statsFontType );
        dst_pt.x = pt.x + statsOffsetX - text.width() / 2;
        dst_pt.y = pt.y + statsOffsetY;
        text.draw( dst_pt.x, dst_pt.y, display );
        statsOffsetY += text.height( text.width() );

        // damage
        str = _( "Damg" );
        str += ": ";

        const uint32_t monsterMinDamage = monster.GetDamageMin();
        const uint32_t monsterMaxDamage = monster.GetDamageMax();

        str += std::to_string( monsterMinDamage );

        if ( monsterMinDamage != monsterMaxDamage ) {
            str += '-';
            str += std::to_string( monsterMaxDamage );
        }

        text.set( str, statsFontType );
        dst_pt.x = pt.x + statsOffsetX - text.width() / 2;
        dst_pt.y = pt.y + statsOffsetY;
        text.draw( dst_pt.x, dst_pt.y, display );
        statsOffsetY += text.height( text.width() );

        // hp
        str = _( "HP" );
        str += ": ";
        str += std::to_string( monster.GetHitPoints() );

        text.set( str, statsFontType );
        dst_pt.x = pt.x + statsOffsetX - text.width() / 2;
        dst_pt.y = pt.y + statsOffsetY;
        text.draw( dst_pt.x, dst_pt.y, display );
        statsOffsetY += 2 * ( text.height( text.width() ) ); // skip a line

        // speed
        str = _( "Speed" );
        str += ':';

        text.set( str, statsFontType );
        dst_pt.x = pt.x + statsOffsetX - text.width() / 2;
        dst_pt.y = pt.y + statsOffsetY;
        text.draw( dst_pt.x, dst_pt.y, display );
        statsOffsetY += text.height( text.width() );

        text.set( Speed::String( monster.GetSpeed() ), statsFontType );
        dst_pt.x = pt.x + statsOffsetX - text.width() / 2;
        dst_pt.y = pt.y + statsOffsetY;
        text.draw( dst_pt.x, dst_pt.y, display );
        statsOffsetY += 2 * ( text.height( text.width() ) ); // skip a line

        // growth and number available
        if ( present ) {
            uint32_t monsterGrown = monster.GetGrown();
            monsterGrown += building & BUILD_WELL ? GetGrownWell() : 0;

            if ( DWELLING_MONSTER1 & dw ) {
                monsterGrown += building & BUILD_WEL2 ? GetGrownWel2() : 0;
            }

            text.set( _( "Growth" ), statsFontType );
            dst_pt.x = pt.x + statsOffsetX - text.width() / 2;
            dst_pt.y = pt.y + statsOffsetY;
            text.draw( dst_pt.x, dst_pt.y, display );
            statsOffsetY += text.height( text.width() );

            str = "+ ";
            str += std::to_string( monsterGrown );
            str += " / ";
            str += _( "week" );

            text.set( str, statsFontType );
            dst_pt.x = pt.x + statsOffsetX - text.width() / 2;
            dst_pt.y = pt.y + statsOffsetY;
            text.draw( dst_pt.x, dst_pt.y, display );

            str = _( "Available" );
            str += ':';

            text.set( str, statsFontType );
            dst_pt.x = pt.x + 44;
            dst_pt.y = pt.y + 122;
            text.draw( dst_pt.x, dst_pt.y, display );

            text.set( std::to_string( available ), fheroes2::FontType::normalYellow() );
            dst_pt.x = pt.x + 129 - text.width() / 2;
            dst_pt.y = pt.y + 120;
            text.draw( dst_pt.x, dst_pt.y, display );
        }

        // monster
        const bool flipMonsterSprite = ( dw >= DWELLING_MONSTER4 );

        const fheroes2::Sprite & smonster = fheroes2::AGG::GetICN( monsterAnimInfo[monsterId].icnFile(), monsterAnimInfo[monsterId].frameId() );
        if ( flipMonsterSprite )
            dst_pt.x = pt.x + 193 - ( smonster.x() + smonster.width() ) + ( monster.isWide() ? CELLW / 2 : 0 ) + monsterAnimInfo[monsterId].offset();
        else
            dst_pt.x = pt.x + 193 + smonster.x() - ( monster.isWide() ? CELLW / 2 : 0 ) - monsterAnimInfo[monsterId].offset();

        dst_pt.y = pt.y + 124 + smonster.y();

        fheroes2::Point inPos( 0, 0 );
        fheroes2::Point outPos( dst_pt.x, dst_pt.y );
        fheroes2::Size inSize( smonster.width(), smonster.height() );

        if ( fheroes2::FitToRoi( smonster, inPos, display, outPos, inSize,
                                 { cur_pt.x, cur_pt.y, fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT } ) ) {
            fheroes2::Blit( smonster, inPos, display, outPos, inSize, flipMonsterSprite );
        }

        dw <<= 1;
        ++monsterId;
    }
}
