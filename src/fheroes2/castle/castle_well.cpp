/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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

#include <string>

#include "agg_image.h"
#include "army_troop.h"
#include "battle_cell.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "icn.h"
#include "kingdom.h"
#include "monster_anim.h"
#include "resource.h"
#include "speed.h"
#include "text.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    uint32_t HowManyRecruitMonster( const Castle & castle, Troops & tempArmy, const uint32_t dw, const Funds & add, Funds & res )
    {
        const Monster ms( castle.GetRace(), castle.GetActualDwelling( dw ) );
        if ( !tempArmy.CanJoinTroop( ms ) )
            return 0;

        uint32_t count = castle.getMonstersInDwelling( dw );
        payment_t payment;

        const Kingdom & kingdom = castle.GetKingdom();

        while ( count ) {
            payment = ms.GetCost() * count;
            res = payment;
            payment += add;
            if ( kingdom.AllowPayment( payment ) )
                break;
            --count;
        }

        if ( count > 0 ) {
            tempArmy.JoinTroop( ms, count );
        }

        return count;
    }

    building_t getPressedBuildingHotkey()
    {
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_1 ) ) {
            return DWELLING_MONSTER1;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_2 ) ) {
            return DWELLING_MONSTER2;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_3 ) ) {
            return DWELLING_MONSTER3;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_4 ) ) {
            return DWELLING_MONSTER4;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_5 ) ) {
            return DWELLING_MONSTER5;
        }
        if ( HotKeyPressEvent( Game::EVENT_TOWN_DWELLING_LEVEL_6 ) ) {
            return DWELLING_MONSTER6;
        }

        return BUILD_NOTHING;
    }
}

void Castle::OpenWell( void )
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
    dst_pt.y = cur_pt.y + 461;
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, ICN::WELLXTRA, 0, 1 );

    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y + 461;
    fheroes2::Button buttonMax( dst_pt.x, dst_pt.y, ICN::BUYMAX, 0, 1 );

    const fheroes2::Rect rectMonster1( cur_pt.x + 20, cur_pt.y + 18, 288, 124 );
    const fheroes2::Rect rectMonster2( cur_pt.x + 20, cur_pt.y + 168, 288, 124 );
    const fheroes2::Rect rectMonster3( cur_pt.x + 20, cur_pt.y + 318, 288, 124 );
    const fheroes2::Rect rectMonster4( cur_pt.x + 334, cur_pt.y + 18, 288, 124 );
    const fheroes2::Rect rectMonster5( cur_pt.x + 334, cur_pt.y + 168, 288, 124 );
    const fheroes2::Rect rectMonster6( cur_pt.x + 334, cur_pt.y + 318, 288, 124 );

    buttonExit.draw();

    std::vector<fheroes2::RandomMonsterAnimation> monsterAnimInfo;
    monsterAnimInfo.emplace_back( Monster( race, DWELLING_MONSTER1 ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER2 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER3 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER4 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER5 ) ) );
    monsterAnimInfo.emplace_back( Monster( race, GetActualDwelling( DWELLING_MONSTER6 ) ) );

    WellRedrawInfoArea( cur_pt, monsterAnimInfo );

    buttonMax.draw();

    std::vector<u32> alldwellings;
    alldwellings.reserve( 6 );
    alldwellings.push_back( DWELLING_MONSTER6 );
    alldwellings.push_back( DWELLING_MONSTER5 );
    alldwellings.push_back( DWELLING_MONSTER4 );
    alldwellings.push_back( DWELLING_MONSTER3 );
    alldwellings.push_back( DWELLING_MONSTER2 );
    alldwellings.push_back( DWELLING_MONSTER1 );

    display.render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        le.MousePressLeft( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();
        const building_t pressedHotkeyBuildingID = getPressedBuildingHotkey();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow ) {
            break;
        }
        if ( le.MouseClickLeft( buttonMax.area() ) || HotKeyPressEvent( Game::EVENT_WELL_BUY_ALL_CREATURES ) ) {
            std::vector<Troop> results;
            Funds cur;
            Funds total;
            std::string str;

            const Troops & currentArmy = GetArmy();
            Troops tempArmy( currentArmy );

            for ( const uint32_t dwellingType : alldwellings ) {
                const uint32_t canRecruit = HowManyRecruitMonster( *this, tempArmy, dwellingType, total, cur );
                if ( canRecruit != 0 ) {
                    const Monster ms( race, GetActualDwelling( dwellingType ) );
                    results.emplace_back( ms, canRecruit );
                    total += cur;
                    str.append( ms.GetPluralName( canRecruit ) );
                    str.append( " - " );
                    str.append( std::to_string( canRecruit ) );
                    str += '\n';
                }
            }

            if ( str.empty() ) {
                bool isCreaturePresent = false;
                for ( int i = 0; i < CASTLEMAXMONSTER; ++i ) {
                    if ( dwelling[i] > 0 ) {
                        isCreaturePresent = true;
                        break;
                    }
                }
                if ( isCreaturePresent ) {
                    Dialog::Message( "", _( "Not enough resources to buy creatures." ), Font::BIG, Dialog::OK );
                }
                else {
                    Dialog::Message( "", _( "No creatures available for purchase." ), Font::BIG, Dialog::OK );
                }
            }
            else if ( fheroes2::showResourceMessage( fheroes2::Text( _( "Buy Creatures" ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } ),
                                                     fheroes2::Text( str, { fheroes2::FontSize::NORMAL, fheroes2::FontColor::WHITE } ), Dialog::YES | Dialog::NO, total )
                      == Dialog::YES ) {
                for ( const Troop & troop : results ) {
                    RecruitMonster( troop, false );
                }
            }
        }
        else if ( ( building & DWELLING_MONSTER1 ) && ( le.MouseClickLeft( rectMonster1 ) || pressedHotkeyBuildingID == DWELLING_MONSTER1 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, DWELLING_MONSTER1 ), dwelling[0], true, 0 ) );
        else if ( ( building & DWELLING_MONSTER2 ) && ( le.MouseClickLeft( rectMonster2 ) || pressedHotkeyBuildingID == DWELLING_MONSTER2 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, GetActualDwelling( DWELLING_MONSTER2 ) ), dwelling[1], true, 0 ) );
        else if ( ( building & DWELLING_MONSTER3 ) && ( le.MouseClickLeft( rectMonster3 ) || pressedHotkeyBuildingID == DWELLING_MONSTER3 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, GetActualDwelling( DWELLING_MONSTER3 ) ), dwelling[2], true, 0 ) );
        else if ( ( building & DWELLING_MONSTER4 ) && ( le.MouseClickLeft( rectMonster4 ) || pressedHotkeyBuildingID == DWELLING_MONSTER4 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, GetActualDwelling( DWELLING_MONSTER4 ) ), dwelling[3], true, 0 ) );
        else if ( ( building & DWELLING_MONSTER5 ) && ( le.MouseClickLeft( rectMonster5 ) || pressedHotkeyBuildingID == DWELLING_MONSTER5 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, GetActualDwelling( DWELLING_MONSTER5 ) ), dwelling[4], true, 0 ) );
        else if ( ( building & DWELLING_MONSTER6 ) && ( le.MouseClickLeft( rectMonster6 ) || pressedHotkeyBuildingID == DWELLING_MONSTER6 ) )
            RecruitMonster( Dialog::RecruitMonster( Monster( race, GetActualDwelling( DWELLING_MONSTER6 ) ), dwelling[5], true, 0 ) );
        else if ( ( building & DWELLING_MONSTER1 ) && le.MousePressRight( rectMonster1 ) )
            Dialog::DwellingInfo( Monster( race, DWELLING_MONSTER1 ), dwelling[0] );
        else if ( ( building & DWELLING_MONSTER2 ) && le.MousePressRight( rectMonster2 ) )
            Dialog::DwellingInfo( Monster( race, DWELLING_MONSTER2 ), dwelling[1] );
        else if ( ( building & DWELLING_MONSTER3 ) && le.MousePressRight( rectMonster3 ) )
            Dialog::DwellingInfo( Monster( race, DWELLING_MONSTER3 ), dwelling[2] );
        else if ( ( building & DWELLING_MONSTER4 ) && le.MousePressRight( rectMonster4 ) )
            Dialog::DwellingInfo( Monster( race, DWELLING_MONSTER4 ), dwelling[3] );
        else if ( ( building & DWELLING_MONSTER5 ) && le.MousePressRight( rectMonster5 ) )
            Dialog::DwellingInfo( Monster( race, DWELLING_MONSTER5 ), dwelling[4] );
        else if ( ( building & DWELLING_MONSTER6 ) && le.MousePressRight( rectMonster6 ) )
            Dialog::DwellingInfo( Monster( race, DWELLING_MONSTER6 ), dwelling[5] );

        if ( Game::validateAnimationDelay( Game::CASTLE_UNIT_DELAY ) ) {
            WellRedrawInfoArea( cur_pt, monsterAnimInfo );

            for ( size_t i = 0; i < monsterAnimInfo.size(); ++i )
                monsterAnimInfo[i].increment();

            buttonMax.draw();
            display.render();
        }
    }
}

void Castle::WellRedrawInfoArea( const fheroes2::Point & cur_pt, const std::vector<fheroes2::RandomMonsterAnimation> & monsterAnimInfo ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::WELLBKG, 0 ), display, cur_pt.x, cur_pt.y );

    fheroes2::Text text;
    fheroes2::Point dst_pt;
    fheroes2::Point pt;

    const fheroes2::FontType statsFontType{ fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE };

    const fheroes2::Sprite & button = fheroes2::AGG::GetICN( ICN::BUYMAX, 0 );
    const fheroes2::Rect src_rt( 0, 461, button.width(), 19 );
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::WELLBKG, 0 ), src_rt.x, src_rt.y, display, cur_pt.x + button.width() + 1, cur_pt.y + 461, src_rt.width, src_rt.height );
    fheroes2::Fill( display, cur_pt.x + button.width(), cur_pt.y + 461, 1, src_rt.height, 0 );

    text.set( _( "Town Population Information and Statistics" ), fheroes2::FontType() );
    dst_pt.x = cur_pt.x + 315 - text.width() / 2;
    dst_pt.y = cur_pt.y + 464;
    text.draw( dst_pt.x, dst_pt.y, display );

    u32 dw = DWELLING_MONSTER1;
    size_t monsterId = 0u;

    while ( dw <= DWELLING_MONSTER6 ) {
        bool present = false;
        u32 dw_orig = DWELLING_MONSTER1;
        u32 icnindex = 0;
        u32 available = 0;

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

            text.set( std::to_string( available ), { fheroes2::FontSize::NORMAL, fheroes2::FontColor::YELLOW } );
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
                                 fheroes2::Rect( cur_pt.x, cur_pt.y, fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) ) ) {
            fheroes2::Blit( smonster, inPos, display, outPos, inSize, flipMonsterSprite );
        }

        dw <<= 1;
        ++monsterId;
    }
}
