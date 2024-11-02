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

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "army.h"
#include "army_troop.h"
#include "battle_cell.h"
#include "castle.h" // IWYU pragma: associated
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

    const std::array<uint32_t, Castle::maxNumOfDwellings> castleDwellings
        = { DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6 };

    uint32_t howManyRecruitMonster( const Castle & castle, Troops & tempCastleArmy, Troops & tempHeroArmy, const uint32_t dw, const Funds & add, Funds & res )
    {
        const Monster monsters( castle.GetRace(), castle.GetActualDwelling( dw ) );
        if ( !tempCastleArmy.CanJoinTroop( monsters ) && !tempHeroArmy.CanJoinTroop( monsters ) ) {
            return 0;
        }

        uint32_t count = castle.getMonstersInDwelling( dw );
        Funds payment;

        const Kingdom & kingdom = castle.GetKingdom();

        while ( count ) {
            payment = monsters.GetCost() * count;
            res = payment;
            payment += add;
            if ( kingdom.AllowPayment( payment ) ) {
                break;
            }
            --count;
        }

        if ( count > 0 ) {
            if ( tempCastleArmy.CanJoinTroop( monsters ) ) {
                tempCastleArmy.JoinTroop( monsters, count, false );
            }
            else if ( tempHeroArmy.CanJoinTroop( monsters ) ) {
                tempHeroArmy.JoinTroop( monsters, count, false );
            }
        }

        return count;
    }

    BuildingType getPressedBuildingHotkey()
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

bool Castle::_recruitCastleMax( const Troops & currentCastleArmy )
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

    // In this loop we should go in reverse order - from strongest monsters to weakest in order to purchase most of the best monsters.
    for ( size_t id = 0; id < maxNumOfDwellings; ++id ) {
        const uint32_t dwellingType = castleDwellings[maxNumOfDwellings - id - 1];
        const uint32_t recruitableNumber = howManyRecruitMonster( *this, tempCastleArmy, tempGuestArmy, dwellingType, totalMonstersCost, currentMonsterCost );

        if ( recruitableNumber == 0 ) {
            continue;
        }
        const Monster recruitableMonster( _race, GetActualDwelling( dwellingType ) );

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

        for ( const uint32_t dwellingType : castleDwellings ) {
            if ( getMonstersInDwelling( dwellingType ) > 0 ) {
                const Monster monsters( _race, dwellingType );
                const Funds payment = monsters.GetCost();

                if ( GetKingdom().AllowPayment( payment ) ) {
                    canAffordOneCreature = true;
                }
                isCreaturePresent = true;
                break;
            }
        }
        if ( isCreaturePresent ) {
            if ( !canAffordOneCreature ) {
                fheroes2::showStandardTextMessage( {}, _( "Not enough resources to recruit creatures." ), Dialog::OK );
            }
            else {
                fheroes2::showStandardTextMessage( {}, _( "You are unable to recruit at this time, your ranks are full." ), Dialog::OK );
            }
        }
        else {
            fheroes2::showStandardTextMessage( {}, _( "No creatures available for purchase." ), Dialog::OK );
        }
    }
    else if ( fheroes2::showResourceMessage( fheroes2::Text( _( "Recruit Creatures" ), fheroes2::FontType::normalYellow() ),
                                             fheroes2::Text( std::move( monstersRecruitedText ), normalWhite ), Dialog::YES | Dialog::NO, totalMonstersCost )
              == Dialog::YES ) {
        for ( const Troop & troop : totalRecruitmentResult ) {
            RecruitMonster( troop, false );
        }

        return true;
    }

    return false;
}

void Castle::_openWell()
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::ImageRestorer restorer( display, ( display.width() - fheroes2::Display::DEFAULT_WIDTH ) / 2,
                                            ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2, fheroes2::Display::DEFAULT_WIDTH,
                                            fheroes2::Display::DEFAULT_HEIGHT );

    const fheroes2::Rect roi = restorer.rect();

    const int32_t buttonOffsetY = roi.y + bottomBarOffsetY;

    // MAX button.
    fheroes2::Button buttonMax( roi.x, buttonOffsetY, ICN::BUYMAX, 0, 1 );

    // EXIT button.
    fheroes2::Button buttonExit( roi.x + roi.width - fheroes2::AGG::GetICN( ICN::BUTTON_GUILDWELL_EXIT, 0 ).width(), buttonOffsetY, ICN::BUTTON_GUILDWELL_EXIT, 0, 1 );

    const std::array<fheroes2::Rect, maxNumOfDwellings> rectMonster
        = { fheroes2::Rect( roi.x + 20, roi.y + 18, 288, 124 ),   fheroes2::Rect( roi.x + 20, roi.y + 168, 288, 124 ),
            fheroes2::Rect( roi.x + 20, roi.y + 318, 288, 124 ),  fheroes2::Rect( roi.x + 334, roi.y + 18, 288, 124 ),
            fheroes2::Rect( roi.x + 334, roi.y + 168, 288, 124 ), fheroes2::Rect( roi.x + 334, roi.y + 318, 288, 124 ) };

    const std::array<Monster, maxNumOfDwellings> castleMonster
        = { Monster( _race, GetActualDwelling( castleDwellings[0] ) ), Monster( _race, GetActualDwelling( castleDwellings[1] ) ),
            Monster( _race, GetActualDwelling( castleDwellings[2] ) ), Monster( _race, GetActualDwelling( castleDwellings[3] ) ),
            Monster( _race, GetActualDwelling( castleDwellings[4] ) ), Monster( _race, GetActualDwelling( castleDwellings[5] ) ) };

    std::array<fheroes2::RandomMonsterAnimation, maxNumOfDwellings> monsterAnimInfo
        = { fheroes2::RandomMonsterAnimation( castleMonster[0] ), fheroes2::RandomMonsterAnimation( castleMonster[1] ),
            fheroes2::RandomMonsterAnimation( castleMonster[2] ), fheroes2::RandomMonsterAnimation( castleMonster[3] ),
            fheroes2::RandomMonsterAnimation( castleMonster[4] ), fheroes2::RandomMonsterAnimation( castleMonster[5] ) };

    fheroes2::Image background;
    background._disableTransformLayer();
    background.resize( roi.width, roi.height );
    _wellRedrawBackground( background );

    LocalEvent & le = LocalEvent::Get();

    Game::passAnimationDelay( Game::CASTLE_UNIT_DELAY );

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::CASTLE_UNIT_DELAY } ) ) ) {
        le.isMouseLeftButtonPressedInArea( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        le.isMouseLeftButtonPressedInArea( buttonMax.area() ) ? buttonMax.drawOnPress() : buttonMax.drawOnRelease();
        const BuildingType pressedHotkeyBuildingID = getPressedBuildingHotkey();

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
            break;
        }
        if ( le.MouseClickLeft( buttonMax.area() ) || HotKeyPressEvent( Game::HotKeyEvent::TOWN_WELL_BUY_ALL ) ) {
            const Troops & currentArmy = GetArmy();
            if ( _recruitCastleMax( currentArmy ) ) {
                // Update available monster count on background.

                for ( const uint32_t dwellingType : castleDwellings ) {
                    _wellRedrawAvailableMonsters( dwellingType, true, background );
                }
            }
        }

        for ( size_t dwellingId = 0; dwellingId < maxNumOfDwellings; ++dwellingId ) {
            if ( _constructedBuildings & castleDwellings[dwellingId] ) {
                if ( le.MouseClickLeft( rectMonster[dwellingId] ) || pressedHotkeyBuildingID == castleDwellings[dwellingId] ) {
                    if ( RecruitMonster( Dialog::RecruitMonster( castleMonster[dwellingId], _dwelling[dwellingId], true, 2 ) ) ) {
                        // Update available monster count on background.

                        _wellRedrawAvailableMonsters( castleDwellings[dwellingId], true, background );
                    }

                    break;
                }

                if ( le.isMouseRightButtonPressedInArea( rectMonster[dwellingId] ) ) {
                    Dialog::DwellingInfo( castleMonster[dwellingId], _dwelling[dwellingId] );

                    break;
                }
            }
        }

        if ( Game::validateAnimationDelay( Game::CASTLE_UNIT_DELAY ) ) {
            fheroes2::Copy( background, 0, 0, display, roi.x, roi.y, roi.width, roi.height );

            _wellRedrawMonsterAnimation( roi, monsterAnimInfo );

            buttonMax.draw();
            buttonExit.draw();
            display.render( roi );
        }

        if ( le.isMouseRightButtonPressedInArea( buttonExit.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Exit" ), _( "Exit this menu." ), Dialog::ZERO );
        }

        if ( le.isMouseRightButtonPressedInArea( buttonMax.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Max" ), _( "Hire all creatures in the town." ), Dialog::ZERO );
        }
    }
}

void Castle::_wellRedrawAvailableMonsters( const uint32_t dwellingType, const bool restoreBackground, fheroes2::Image & background ) const
{
    if ( !( _constructedBuildings & dwellingType ) ) {
        // This building has not been built.
        return;
    }

    uint32_t population = 0;
    fheroes2::Point offset{ 20, 121 };

    switch ( dwellingType ) {
    case DWELLING_MONSTER1:
        population = _dwelling[0];
        break;
    case DWELLING_MONSTER2:
        population = _dwelling[1];
        offset.y += 150;
        break;
    case DWELLING_MONSTER3:
        population = _dwelling[2];
        offset.y += 300;
        break;
    case DWELLING_MONSTER4:
        offset.x += 314;
        population = _dwelling[3];
        break;
    case DWELLING_MONSTER5:
        population = _dwelling[4];
        offset.x += 314;
        offset.y += 150;
        break;
    case DWELLING_MONSTER6:
        population = _dwelling[5];
        offset.x += 314;
        offset.y += 300;
        break;
    default:
        // Have you added a new dwelling?
        assert( 0 );
        break;
    }

    if ( restoreBackground ) {
        // Restore background under "Available: <number>" text.
        const fheroes2::Sprite & wellBackground = fheroes2::AGG::GetICN( Settings::Get().isEvilInterfaceEnabled() ? ICN::WELLBKG_EVIL : ICN::WELLBKG, 0 );

        fheroes2::Copy( wellBackground, offset.x, offset.y, background, offset.x, offset.y, 137, fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) );
    }

    std::string textString = _( "Available" );
    textString += ": ";

    fheroes2::Text text( std::move( textString ), fheroes2::FontType::smallWhite() );
    text.draw( offset.x + 24, offset.y + 2, background );

    text.set( std::to_string( population ), fheroes2::FontType::normalYellow() );
    text.draw( offset.x + 109 - text.width() / 2, offset.y, background );
}

void Castle::_wellRedrawBackground( fheroes2::Image & background ) const
{
    const fheroes2::Sprite & wellBackground = fheroes2::AGG::GetICN( Settings::Get().isEvilInterfaceEnabled() ? ICN::WELLBKG_EVIL : ICN::WELLBKG, 0 );
    const int32_t backgroundWidth = wellBackground.width();

    fheroes2::Copy( wellBackground, 0, 0, background, 0, 0, backgroundWidth, bottomBarOffsetY );

    // TODO: Make Well bottom bar and buttons for Evil interface.

    // The original ICN::WELLBKG image has incorrect bottom message bar with no yellow outline. Also the original graphics did not have MAX button.
    const fheroes2::Sprite & bottomBar = fheroes2::AGG::GetICN( ICN::SMALLBAR, 0 );
    const int32_t barHeight = bottomBar.height();
    const int32_t exitWidth = fheroes2::AGG::GetICN( ICN::BUTTON_GUILDWELL_EXIT, 0 ).width();
    const int32_t buttonMaxWidth = fheroes2::AGG::GetICN( ICN::BUYMAX, 0 ).width();
    // ICN::SMALLBAR image's first column contains all black pixels. This should not be drawn.
    fheroes2::Copy( bottomBar, 1, 0, background, buttonMaxWidth, bottomBarOffsetY, backgroundWidth / 2 - buttonMaxWidth, barHeight );
    fheroes2::Copy( bottomBar, bottomBar.width() - backgroundWidth / 2 + exitWidth - 1, 0, background, backgroundWidth / 2, bottomBarOffsetY,
                    backgroundWidth / 2 - exitWidth + 1, barHeight );

    // The original assets Well background has a transparent line to the right of EXIT button and it is not covered by any other image. Fill it with the black color.
    fheroes2::Fill( background, background.width() - 1, bottomBarOffsetY, 1, bottomBar.height(), static_cast<uint8_t>( 0 ) );

    fheroes2::Text text( _( "Town Population Information and Statistics" ), fheroes2::FontType::normalWhite() );
    text.draw( 315 - text.width() / 2, bottomBarOffsetY + 3, background );

    const fheroes2::FontType statsFontType = fheroes2::FontType::smallWhite();

    for ( const uint32_t dwellingType : castleDwellings ) {
        // By default the 'icnIndex' and 'offset' values are set for DWELLING_MONSTER1.
        uint32_t icnIndex = 19;
        fheroes2::Point offset{ 0, 1 };

        switch ( dwellingType ) {
        case DWELLING_MONSTER1:
            break;
        case DWELLING_MONSTER2:
            offset.y = 151;
            icnIndex = DWELLING_UPGRADE2 & _constructedBuildings ? 25 : 20;
            break;
        case DWELLING_MONSTER3:
            offset.y = 301;
            icnIndex = DWELLING_UPGRADE3 & _constructedBuildings ? 26 : 21;
            break;
        case DWELLING_MONSTER4:
            offset.x = 314;
            icnIndex = DWELLING_UPGRADE4 & _constructedBuildings ? 27 : 22;
            break;
        case DWELLING_MONSTER5:
            offset.x = 314;
            offset.y = 151;
            icnIndex = DWELLING_UPGRADE5 & _constructedBuildings ? 28 : 23;
            break;
        case DWELLING_MONSTER6:
            offset.x = 314;
            offset.y = 301;
            icnIndex = DWELLING_UPGRADE7 & _constructedBuildings ? 30 : ( DWELLING_UPGRADE6 & _constructedBuildings ? 29 : 24 );
            break;
        default:
            // Have you added a new dwelling?
            assert( 0 );
            break;
        }

        const uint32_t actualDwellindType = GetActualDwelling( dwellingType );
        const Monster monster( _race, actualDwellindType );

        // Dwelling building image.
        fheroes2::Point renderPoint( offset.x + 21, offset.y + 35 );
        const fheroes2::Sprite & dwellingImage = fheroes2::AGG::GetICN( ICN::getBuildingIcnId( _race ), icnIndex );
        fheroes2::Copy( dwellingImage, 0, 0, background, renderPoint.x, renderPoint.y, dwellingImage.width(), dwellingImage.height() );

        // Dwelling name.
        text.set( GetStringBuilding( actualDwellindType, _race ), statsFontType );
        renderPoint.x = offset.x + 86 - text.width() / 2;
        renderPoint.y = offset.y + 104;
        text.draw( renderPoint.x, renderPoint.y, background );

        // Creature name.
        text.set( monster.GetMultiName(), statsFontType );
        renderPoint.x = offset.x + 122 - text.width() / 2;
        renderPoint.y = offset.y + 19;
        text.draw( renderPoint.x, renderPoint.y, background );

        // attack
        std::string textString;
        textString = _( "Attack" );
        textString += ": ";
        textString += std::to_string( monster.GetAttack() );

        text.set( textString, statsFontType );

        // Set initial X and Y offset.
        renderPoint.x = offset.x + 269;
        renderPoint.y = offset.y + 22;

        text.draw( renderPoint.x - text.width() / 2, renderPoint.y, background );
        renderPoint.y += text.height( text.width() );

        // defense
        textString = _( "Defense" );
        textString += ": ";
        textString += std::to_string( monster.GetDefense() );

        text.set( textString, statsFontType );
        text.draw( renderPoint.x - text.width() / 2, renderPoint.y, background );
        renderPoint.y += text.height( text.width() );

        // damage
        textString = _( "Damage" );
        textString += ": ";

        const uint32_t monsterMinDamage = monster.GetDamageMin();
        const uint32_t monsterMaxDamage = monster.GetDamageMax();

        textString += std::to_string( monsterMinDamage );

        if ( monsterMinDamage != monsterMaxDamage ) {
            textString += '-';
            textString += std::to_string( monsterMaxDamage );
        }

        text.set( textString, statsFontType );
        text.draw( renderPoint.x - text.width() / 2, renderPoint.y, background );
        renderPoint.y += text.height( text.width() );

        // hp
        textString = _( "HP" );
        textString += ": ";
        textString += std::to_string( monster.GetHitPoints() );

        text.set( textString, statsFontType );
        text.draw( renderPoint.x - text.width() / 2, renderPoint.y, background );
        renderPoint.y += 2 * ( text.height( text.width() ) ); // skip a line

        // speed
        textString = _( "Speed" );
        textString += ':';

        text.set( textString, statsFontType );
        text.draw( renderPoint.x - text.width() / 2, renderPoint.y, background );
        renderPoint.y += text.height( text.width() );

        text.set( Speed::String( static_cast<int>( monster.GetSpeed() ) ), statsFontType );
        text.draw( renderPoint.x - text.width() / 2, renderPoint.y, background );
        renderPoint.y += 2 * ( text.height( text.width() ) ); // skip a line

        // If dwelling is built show growth and available population.
        if ( _constructedBuildings & dwellingType ) {
            uint32_t monsterGrown = monster.GetGrown();
            monsterGrown += _constructedBuildings & BUILD_WELL ? GetGrownWell() : 0;

            if ( DWELLING_MONSTER1 & dwellingType ) {
                monsterGrown += _constructedBuildings & BUILD_WEL2 ? GetGrownWel2() : 0;
            }

            text.set( _( "Growth" ), statsFontType );
            text.draw( renderPoint.x - text.width() / 2, renderPoint.y, background );
            renderPoint.y += text.height( text.width() );

            textString = "+ ";
            textString += std::to_string( monsterGrown );
            textString += " / ";
            textString += _( "week" );

            text.set( textString, statsFontType );
            text.draw( renderPoint.x - text.width() / 2, renderPoint.y, background );

            _wellRedrawAvailableMonsters( dwellingType, false, background );
        }
    }
}

void Castle::_wellRedrawMonsterAnimation( const fheroes2::Rect & roi, std::array<fheroes2::RandomMonsterAnimation, maxNumOfDwellings> & monsterAnimInfo ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();

    for ( size_t monsterId = 0; monsterId < maxNumOfDwellings; ++monsterId ) {
        fheroes2::Point outPos( roi.x, roi.y + 1 );

        switch ( monsterId ) {
        case 0:
            break;
        case 1:
            outPos.y += 150;
            break;
        case 2:
            outPos.y += 300;
            break;
        case 3:
            outPos.x += 314;
            break;
        case 4:
            outPos.x += 314;
            outPos.y += 150;
            break;
        case 5:
            outPos.x += 314;
            outPos.y += 300;
            break;
        default:
            // Have you added a new dwelling?
            assert( 0 );
            break;
        }

        const Monster monster( _race, GetActualDwelling( castleDwellings[monsterId] ) );

        // monster
        const bool flipMonsterSprite = ( monsterId > 2 );

        const fheroes2::Sprite & smonster = fheroes2::AGG::GetICN( monsterAnimInfo[monsterId].icnFile(), monsterAnimInfo[monsterId].frameId() );
        if ( flipMonsterSprite ) {
            outPos.x += 193 - ( smonster.x() + smonster.width() ) + ( monster.isWide() ? Battle::Cell::widthPx / 2 : 0 ) + monsterAnimInfo[monsterId].offset();
        }
        else {
            outPos.x += 193 + smonster.x() - ( monster.isWide() ? Battle::Cell::widthPx / 2 : 0 ) - monsterAnimInfo[monsterId].offset();
        }

        outPos.y += 124 + smonster.y();

        fheroes2::Point inPos( 0, 0 );
        fheroes2::Size inSize( smonster.width(), smonster.height() );

        if ( fheroes2::FitToRoi( smonster, inPos, display, outPos, inSize, roi ) ) {
            fheroes2::Blit( smonster, inPos, display, outPos, inSize, flipMonsterSprite );
        }

        monsterAnimInfo[monsterId].increment();
    }
}
