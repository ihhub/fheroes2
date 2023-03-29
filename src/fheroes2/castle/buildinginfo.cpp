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

#include "buildinginfo.h"

#include <algorithm>
#include <cassert>

#include "agg_image.h"
#include "army_troop.h"
#include "audio_manager.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "localevent.h"
#include "m82.h"
#include "monster.h"
#include "pal.h"
#include "profit.h"
#include "race.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "statusbar.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"

namespace
{
    fheroes2::Point GetFlagOffset( int race )
    {
        switch ( race ) {
        case Race::KNGT:
            return { 36, 10 };
        case Race::BARB:
            return { 35, 9 };
        case Race::SORC:
            return { 36, 10 };
        case Race::WRLK:
            return { 34, 10 };
        case Race::WZRD:
            return { 35, 9 };
        case Race::NECR:
            return { 35, 10 };
        default:
            // Did you add a new race?
            assert( 0 );
            return {};
        }
    }
}

struct buildstats_t
{
    uint32_t id2;
    uint8_t race;
    cost_t cost;
};

buildstats_t _builds[] = {
    // id                             gold wood mercury ore sulfur crystal gems
    { BUILD_THIEVESGUILD, Race::ALL, { 750, 5, 0, 0, 0, 0, 0 } },
    { BUILD_TAVERN, Race::ALL, { 500, 5, 0, 0, 0, 0, 0 } },
    { BUILD_SHIPYARD, Race::ALL, { 2000, 20, 0, 0, 0, 0, 0 } },
    { BUILD_WELL, Race::ALL, { 500, 0, 0, 0, 0, 0, 0 } },
    { BUILD_STATUE, Race::ALL, { 1250, 0, 0, 5, 0, 0, 0 } },
    { BUILD_LEFTTURRET, Race::ALL, { 1500, 0, 0, 5, 0, 0, 0 } },
    { BUILD_RIGHTTURRET, Race::ALL, { 1500, 0, 0, 5, 0, 0, 0 } },
    { BUILD_MARKETPLACE, Race::ALL, { 500, 5, 0, 0, 0, 0, 0 } },
    { BUILD_MOAT, Race::ALL, { 750, 0, 0, 0, 0, 0, 0 } },
    { BUILD_CASTLE, Race::ALL, { 5000, 20, 0, 20, 0, 0, 0 } },
    { BUILD_CAPTAIN, Race::ALL, { 500, 0, 0, 0, 0, 0, 0 } },
    { BUILD_MAGEGUILD1, Race::ALL, { 2000, 5, 0, 5, 0, 0, 0 } },
    { BUILD_MAGEGUILD2, Race::ALL, { 1000, 5, 4, 5, 4, 4, 4 } },
    { BUILD_MAGEGUILD3, Race::ALL, { 1000, 5, 6, 5, 6, 6, 6 } },
    { BUILD_MAGEGUILD4, Race::ALL, { 1000, 5, 8, 5, 8, 8, 8 } },
    { BUILD_MAGEGUILD5, Race::ALL, { 1000, 5, 10, 5, 10, 10, 10 } },

    { BUILD_WEL2, Race::KNGT, { 1000, 0, 0, 0, 0, 0, 0 } },
    { BUILD_WEL2, Race::BARB, { 1000, 0, 0, 0, 0, 0, 0 } },
    { BUILD_WEL2, Race::SORC, { 1000, 0, 0, 0, 0, 0, 0 } },
    { BUILD_WEL2, Race::WRLK, { 1000, 0, 0, 0, 0, 0, 0 } },
    { BUILD_WEL2, Race::WZRD, { 1000, 0, 0, 0, 0, 0, 0 } },
    { BUILD_WEL2, Race::NECR, { 1000, 0, 0, 0, 0, 0, 0 } },

    { BUILD_SPEC, Race::KNGT, { 1500, 5, 0, 15, 0, 0, 0 } },
    { BUILD_SPEC, Race::BARB, { 2000, 10, 0, 10, 0, 0, 0 } },
    { BUILD_SPEC, Race::SORC, { 1500, 0, 0, 0, 0, 10, 0 } },
    { BUILD_SPEC, Race::WRLK, { 3000, 5, 0, 10, 0, 0, 0 } },
    { BUILD_SPEC, Race::WZRD, { 1500, 5, 5, 5, 5, 5, 5 } },
    { BUILD_SPEC, Race::NECR, { 1000, 0, 10, 0, 10, 0, 0 } },

    { BUILD_SHRINE, Race::NECR, { 4000, 10, 0, 0, 0, 10, 0 } },

    { DWELLING_MONSTER1, Race::KNGT, { 200, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER2, Race::KNGT, { 1000, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_UPGRADE2, Race::KNGT, { 1500, 5, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER3, Race::KNGT, { 1000, 0, 0, 5, 0, 0, 0 } },
    { DWELLING_UPGRADE3, Race::KNGT, { 1500, 0, 0, 5, 0, 0, 0 } },
    { DWELLING_MONSTER4, Race::KNGT, { 2000, 10, 0, 10, 0, 0, 0 } },
    { DWELLING_UPGRADE4, Race::KNGT, { 2000, 5, 0, 5, 0, 0, 0 } },
    { DWELLING_MONSTER5, Race::KNGT, { 3000, 20, 0, 0, 0, 0, 0 } },
    { DWELLING_UPGRADE5, Race::KNGT, { 3000, 10, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER6, Race::KNGT, { 5000, 20, 0, 0, 0, 20, 0 } },
    { DWELLING_UPGRADE6, Race::KNGT, { 5000, 10, 0, 0, 0, 10, 0 } },

    { DWELLING_MONSTER1, Race::BARB, { 300, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER2, Race::BARB, { 800, 5, 0, 0, 0, 0, 0 } },
    { DWELLING_UPGRADE2, Race::BARB, { 1200, 5, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER3, Race::BARB, { 1000, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER4, Race::BARB, { 2000, 10, 0, 10, 0, 0, 0 } },
    { DWELLING_UPGRADE4, Race::BARB, { 3000, 5, 0, 5, 0, 0, 0 } },
    { DWELLING_MONSTER5, Race::BARB, { 4000, 0, 0, 20, 0, 0, 0 } },
    { DWELLING_UPGRADE5, Race::BARB, { 2000, 0, 0, 10, 0, 0, 0 } },
    { DWELLING_MONSTER6, Race::BARB, { 6000, 0, 0, 20, 0, 20, 0 } },

    { DWELLING_MONSTER1, Race::SORC, { 500, 5, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER2, Race::SORC, { 1000, 5, 0, 0, 0, 0, 0 } },
    { DWELLING_UPGRADE2, Race::SORC, { 1500, 5, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER3, Race::SORC, { 1500, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_UPGRADE3, Race::SORC, { 1500, 5, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER4, Race::SORC, { 2500, 0, 0, 10, 0, 0, 0 } },
    { DWELLING_UPGRADE4, Race::SORC, { 1500, 0, 5, 0, 0, 0, 0 } },
    { DWELLING_MONSTER5, Race::SORC, { 3000, 10, 0, 0, 0, 0, 10 } },
    { DWELLING_MONSTER6, Race::SORC, { 10000, 0, 20, 30, 0, 0, 0 } },

    { DWELLING_MONSTER1, Race::WRLK, { 500, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER2, Race::WRLK, { 1000, 0, 0, 10, 0, 0, 0 } },
    { DWELLING_MONSTER3, Race::WRLK, { 2000, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER4, Race::WRLK, { 3000, 0, 0, 0, 0, 0, 10 } },
    { DWELLING_UPGRADE4, Race::WRLK, { 2000, 0, 0, 0, 0, 0, 5 } },
    { DWELLING_MONSTER5, Race::WRLK, { 4000, 0, 0, 0, 10, 0, 0 } },
    { DWELLING_MONSTER6, Race::WRLK, { 15000, 0, 0, 30, 20, 0, 0 } },
    { DWELLING_UPGRADE6, Race::WRLK, { 5000, 0, 0, 5, 10, 0, 0 } },
    { DWELLING_UPGRADE7, Race::WRLK, { 5000, 0, 0, 5, 10, 0, 0 } },

    { DWELLING_MONSTER1, Race::WZRD, { 400, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER2, Race::WZRD, { 800, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER3, Race::WZRD, { 1500, 5, 0, 5, 0, 0, 0 } },
    { DWELLING_UPGRADE3, Race::WZRD, { 1500, 0, 5, 0, 0, 0, 0 } },
    { DWELLING_MONSTER4, Race::WZRD, { 3000, 5, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER5, Race::WZRD, { 3500, 5, 5, 5, 5, 5, 5 } },
    { DWELLING_UPGRADE5, Race::WZRD, { 4000, 5, 0, 5, 0, 0, 0 } },
    { DWELLING_MONSTER6, Race::WZRD, { 12500, 5, 0, 5, 0, 0, 20 } },
    { DWELLING_UPGRADE6, Race::WZRD, { 12500, 5, 0, 5, 0, 0, 20 } },

    { DWELLING_MONSTER1, Race::NECR, { 400, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER2, Race::NECR, { 1000, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_UPGRADE2, Race::NECR, { 1000, 0, 0, 0, 0, 0, 0 } },
    { DWELLING_MONSTER3, Race::NECR, { 1500, 0, 0, 10, 0, 0, 0 } },
    { DWELLING_UPGRADE3, Race::NECR, { 1500, 0, 0, 5, 0, 0, 0 } },
    { DWELLING_MONSTER4, Race::NECR, { 3000, 10, 0, 0, 0, 0, 0 } },
    { DWELLING_UPGRADE4, Race::NECR, { 4000, 5, 0, 0, 0, 10, 10 } },
    { DWELLING_MONSTER5, Race::NECR, { 4000, 10, 0, 0, 10, 0, 0 } },
    { DWELLING_UPGRADE5, Race::NECR, { 3000, 0, 0, 5, 0, 5, 0 } },
    { DWELLING_MONSTER6, Race::NECR, { 10000, 10, 5, 10, 5, 5, 5 } },

    // end
    { BUILD_NOTHING, Race::NONE, { 0, 0, 0, 0, 0, 0, 0 } },
};

payment_t BuildingInfo::GetCost( uint32_t build, int race )
{
    payment_t payment;
    const buildstats_t * ptr = &_builds[0];

    while ( BUILD_NOTHING != ptr->id2 && !( ptr->id2 == build && ( !race || ( race & ptr->race ) ) ) )
        ++ptr;

    if ( BUILD_NOTHING != ptr->id2 ) {
        payment.gold = ptr->cost.gold;
        payment.wood = ptr->cost.wood;
        payment.mercury = ptr->cost.mercury;
        payment.ore = ptr->cost.ore;
        payment.sulfur = ptr->cost.sulfur;
        payment.crystal = ptr->cost.crystal;
        payment.gems = ptr->cost.gems;
    }

    return payment;
}

int GetIndexBuildingSprite( uint32_t build )
{
    switch ( build ) {
    case DWELLING_MONSTER1:
        return 19;
    case DWELLING_MONSTER2:
        return 20;
    case DWELLING_MONSTER3:
        return 21;
    case DWELLING_MONSTER4:
        return 22;
    case DWELLING_MONSTER5:
        return 23;
    case DWELLING_MONSTER6:
        return 24;
    case DWELLING_UPGRADE2:
        return 25;
    case DWELLING_UPGRADE3:
        return 26;
    case DWELLING_UPGRADE4:
        return 27;
    case DWELLING_UPGRADE5:
        return 28;
    case DWELLING_UPGRADE6:
        return 29;
    case DWELLING_UPGRADE7:
        return 30;
    case BUILD_MAGEGUILD1:
    case BUILD_MAGEGUILD2:
    case BUILD_MAGEGUILD3:
    case BUILD_MAGEGUILD4:
    case BUILD_MAGEGUILD5:
        return 0;
    case BUILD_THIEVESGUILD:
        return 1;
    case BUILD_SHRINE:
    case BUILD_TAVERN:
        return 2;
    case BUILD_SHIPYARD:
        return 3;
    case BUILD_WELL:
        return 4;
    case BUILD_CASTLE:
        return 6;
    case BUILD_STATUE:
        return 7;
    case BUILD_LEFTTURRET:
        return 8;
    case BUILD_RIGHTTURRET:
        return 9;
    case BUILD_MARKETPLACE:
        return 10;
    case BUILD_WEL2:
        return 11;
    case BUILD_MOAT:
        return 12;
    case BUILD_SPEC:
        return 13;
    case BUILD_CAPTAIN:
        return 15;
    default:
        break;
    }

    return 0;
}

BuildingInfo::BuildingInfo( const Castle & c, const building_t b )
    : castle( c )
    , building( b )
    , area( 0, 0, 135, 70 )
    , bcond( ALLOW_BUILD )
{
    if ( IsDwelling() )
        building = castle.GetActualDwelling( b );

    building = castle.isBuild( b ) ? castle.GetUpgradeBuilding( b ) : b;

    if ( BUILD_TAVERN == building && Race::NECR == castle.GetRace() )
        building = ( Settings::Get().isCurrentMapPriceOfLoyalty() ) ? BUILD_SHRINE : BUILD_NOTHING;

    bcond = castle.CheckBuyBuilding( building );

    // generate description
    if ( BUILD_DISABLE == bcond )
        description = GetConditionDescription();
    else if ( IsDwelling() ) {
        description = _( "The %{building} produces %{monster}." );
        StringReplace( description, "%{building}", Castle::GetStringBuilding( building, castle.GetRace() ) );
        StringReplaceWithLowercase( description, "%{monster}", Monster( castle.GetRace(), building ).GetMultiName() );
    }
    else
        description = Castle::GetDescriptionBuilding( building, castle.GetRace() );

    switch ( building ) {
    case BUILD_WELL:
        StringReplace( description, "%{count}", Castle::GetGrownWell() );
        break;

    case BUILD_WEL2:
        StringReplace( description, "%{count}", Castle::GetGrownWel2() );
        break;

    case BUILD_CASTLE:
    case BUILD_STATUE:
    case BUILD_SPEC: {
        const payment_t profit = ProfitConditions::FromBuilding( building, castle.GetRace() );
        StringReplace( description, "%{count}", profit.gold );
        break;
    }

    default:
        break;
    }

    // fix area for captain
    if ( b == BUILD_CAPTAIN ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::Get4Captain( castle.GetRace() ), ( building & BUILD_CAPTAIN ? 1 : 0 ) );
        area.width = sprite.width();
        area.height = sprite.height();
    }
}

void BuildingInfo::SetPos( int32_t x, int32_t y )
{
    area.x = x;
    area.y = y;
}

bool BuildingInfo::IsDwelling() const
{
    switch ( building ) {
    case DWELLING_MONSTER1:
    case DWELLING_MONSTER2:
    case DWELLING_MONSTER3:
    case DWELLING_MONSTER4:
    case DWELLING_MONSTER5:
    case DWELLING_MONSTER6:
    case DWELLING_UPGRADE2:
    case DWELLING_UPGRADE3:
    case DWELLING_UPGRADE4:
    case DWELLING_UPGRADE5:
    case DWELLING_UPGRADE6:
    case DWELLING_UPGRADE7:
        return true;
    default:
        break;
    }
    return false;
}

void BuildingInfo::RedrawCaptain() const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    if ( bcond == ALREADY_BUILT ) {
        const fheroes2::Sprite & captainSprite = fheroes2::AGG::GetICN( ICN::Get4Captain( castle.GetRace() ), 1 );
        const fheroes2::Sprite & flag = fheroes2::AGG::GetICN( ICN::GetFlagIcnId( castle.GetColor() ), 0 );

        fheroes2::Blit( captainSprite, display, area.x, area.y );
        const fheroes2::Point flagOffset = GetFlagOffset( castle.GetRace() );
        fheroes2::Blit( flag, display, area.x + flagOffset.x, area.y + flagOffset.y );
    }
    else {
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::Get4Captain( castle.GetRace() ), 0 ), display, area.x, area.y );
    }

    // indicator
    if ( bcond == ALREADY_BUILT ) {
        const fheroes2::Sprite & spriteAllow = fheroes2::AGG::GetICN( ICN::TOWNWIND, 11 );
        fheroes2::Blit( spriteAllow, display, area.x + 83 - 4 - spriteAllow.width(), area.y + 79 - 2 - spriteAllow.height() );
    }
    else if ( bcond != ALLOW_BUILD ) {
        if ( LACK_RESOURCES == bcond ) {
            const fheroes2::Sprite & spriteMoney = fheroes2::AGG::GetICN( ICN::TOWNWIND, 13 );
            fheroes2::Blit( spriteMoney, display, area.x + 83 - 4 + 1 - spriteMoney.width(), area.y + 79 - 3 - spriteMoney.height() );
        }
        else {
            const fheroes2::Sprite & spriteDeny = fheroes2::AGG::GetICN( ICN::TOWNWIND, 12 );
            fheroes2::Blit( spriteDeny, display, area.x + 83 - 4 + 1 - spriteDeny.width(), area.y + 79 - 2 - spriteDeny.height() );
        }
    }
}

void BuildingInfo::Redraw() const
{
    if ( BUILD_CAPTAIN == building ) {
        RedrawCaptain();
    }
    else {
        fheroes2::Display & display = fheroes2::Display::instance();
        const int index = GetIndexBuildingSprite( building );

        const fheroes2::Sprite & buildingFrame = fheroes2::AGG::GetICN( ICN::BLDGXTRA, 0 );
        fheroes2::Blit( buildingFrame, display, area.x, area.y );
        if ( BUILD_DISABLE == bcond ) {
            const fheroes2::Point offset( 6, 59 );
            fheroes2::Sprite grayedOut = fheroes2::Crop( buildingFrame, offset.x, offset.y, 125, 12 );
            fheroes2::ApplyPalette( grayedOut, PAL::GetPalette( PAL::PaletteType::GRAY ) );
            fheroes2::ApplyPalette( grayedOut, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
            fheroes2::Blit( grayedOut, display, area.x + offset.x, area.y + offset.y );
        }

        // build image
        if ( BUILD_NOTHING == building ) {
            const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
            fheroes2::Blit( fheroes2::AGG::GetICN( isEvilInterface ? ICN::CASLXTRA_EVIL : ICN::CASLXTRA, 0 ), display, area.x, area.y );
            return;
        }

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::Get4Building( castle.GetRace() ), index ), display, area.x + 1, area.y + 1 );

        // indicator
        if ( bcond == ALREADY_BUILT ) {
            const fheroes2::Sprite & spriteAllow = fheroes2::AGG::GetICN( ICN::TOWNWIND, 11 );
            fheroes2::Blit( spriteAllow, display, area.x + buildingFrame.width() - 5 - spriteAllow.width(), area.y + 58 - 2 - spriteAllow.height() );
        }
        else if ( bcond == BUILD_DISABLE ) {
            const fheroes2::Sprite & spriteDeny = fheroes2::AGG::GetICN( ICN::TOWNWIND, 12 );
            fheroes2::Sprite disabledSprite( spriteDeny );
            fheroes2::ApplyPalette( disabledSprite, PAL::GetPalette( PAL::PaletteType::GRAY ) );
            fheroes2::ApplyPalette( disabledSprite, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
            fheroes2::Blit( disabledSprite, display, area.x + buildingFrame.width() - 5 + 1 - spriteDeny.width(), area.y + 58 - 2 - spriteDeny.height() );
        }
        else if ( bcond != ALLOW_BUILD ) {
            if ( LACK_RESOURCES == bcond ) {
                const fheroes2::Sprite & spriteMoney = fheroes2::AGG::GetICN( ICN::TOWNWIND, 13 );
                fheroes2::Blit( spriteMoney, display, area.x + buildingFrame.width() - 5 + 1 - spriteMoney.width(), area.y + 58 - 3 - spriteMoney.height() );
            }
            else {
                const fheroes2::Sprite & spriteDeny = fheroes2::AGG::GetICN( ICN::TOWNWIND, 12 );
                fheroes2::Blit( spriteDeny, display, area.x + buildingFrame.width() - 5 + 1 - spriteDeny.width(), area.y + 58 - 2 - spriteDeny.height() );
            }
        }

        // status bar
        if ( bcond != BUILD_DISABLE && bcond != ALREADY_BUILT ) {
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CASLXTRA, bcond == ALLOW_BUILD ? 1 : 2 ), display, area.x, area.y + 58 );
        }

        // name
        const Text text( Castle::GetStringBuilding( building, castle.GetRace() ), Font::SMALL );
        text.Blit( area.x + 68 - text.w() / 2, area.y + 59 );
    }
}

const char * BuildingInfo::GetName() const
{
    return Castle::GetStringBuilding( building, castle.GetRace() );
}

bool BuildingInfo::QueueEventProcessing( fheroes2::ButtonBase & exitButton ) const
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( area ) ) {
        if ( bcond == LACK_RESOURCES || bcond == ALLOW_BUILD ) {
            fheroes2::ButtonRestorer exitRestorer( exitButton );
            return DialogBuyBuilding( true );
        }
    }
    else if ( le.MousePressRight( area ) ) {
        DialogBuyBuilding( false );
    }
    return false;
}

bool BuildingInfo::DialogBuyBuilding( bool buttons ) const
{
    if ( building == BUILD_NOTHING ) {
        return false;
    }

    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( buttons, Cursor::POINTER );

    std::string box1str = description;

    if ( ALLOW_BUILD != bcond ) {
        const std::string & ext = GetConditionDescription();
        if ( !ext.empty() && ext != description ) {
            box1str.append( "\n \n" );
            box1str.append( ext );
        }
    }

    TextBox box1( box1str, Font::BIG, BOXAREA_WIDTH );

    // prepare requirement build string
    std::string str;
    const uint32_t requirement = castle.GetBuildingRequirement( building );
    const std::string sep = "\n";

    for ( uint32_t itr = 0x00000001; itr; itr <<= 1 )
        if ( requirement & itr ) {
            str.append( Castle::GetStringBuilding( itr, castle.GetRace() ) );
            str.append( sep );
        }

    // replace end sep
    if ( !str.empty() )
        str.replace( str.size() - sep.size(), sep.size(), "" );

    const bool isRequired = !str.empty();
    Text requires_text( _( "Requires:" ), Font::BIG );
    TextBox box2( str, Font::BIG, BOXAREA_WIDTH );

    Resource::BoxSprite rbs( PaymentConditions::BuyBuilding( castle.GetRace(), building ), BOXAREA_WIDTH );

    const fheroes2::Sprite & window_icons = fheroes2::AGG::GetICN( ICN::BLDGXTRA, 0 );
    const int space = 10;
    Dialog::FrameBox box( space + window_icons.height() + space + box1.h() + space + ( isRequired ? requires_text.h() + box2.h() + space : 0 ) + rbs.GetArea().height,
                          buttons );
    const fheroes2::Rect & box_rt = box.GetArea();
    LocalEvent & le = LocalEvent::Get();

    fheroes2::Point dst_pt;

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int buttonOkayIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_OKAY_BUTTON : ICN::UNIFORM_GOOD_OKAY_BUTTON;

    dst_pt.x = box_rt.x;
    dst_pt.y = box_rt.y + box_rt.height - fheroes2::AGG::GetICN( buttonOkayIcnID, 0 ).height();
    fheroes2::Button button1( dst_pt.x, dst_pt.y, buttonOkayIcnID, 0, 1 );

    const int buttonCancelIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_CANCEL_BUTTON : ICN::UNIFORM_GOOD_CANCEL_BUTTON;

    dst_pt.x = box_rt.x + box_rt.width - fheroes2::AGG::GetICN( buttonCancelIcnID, 0 ).width();
    dst_pt.y = box_rt.y + box_rt.height - fheroes2::AGG::GetICN( buttonCancelIcnID, 0 ).height();
    fheroes2::Button button2( dst_pt.x, dst_pt.y, buttonCancelIcnID, 0, 1 );

    dst_pt.x = box_rt.x + ( box_rt.width - window_icons.width() ) / 2;
    dst_pt.y = box_rt.y + space;
    fheroes2::Blit( window_icons, display, dst_pt.x, dst_pt.y );

    const fheroes2::Sprite & building_icons = fheroes2::AGG::GetICN( ICN::Get4Building( castle.GetRace() ), GetIndexBuildingSprite( building ) );
    dst_pt.x = box_rt.x + ( box_rt.width - building_icons.width() ) / 2;
    dst_pt.y += 1;
    fheroes2::Blit( building_icons, display, dst_pt.x, dst_pt.y );

    Text text( GetName(), Font::SMALL );
    dst_pt.x = box_rt.x + ( box_rt.width - text.w() ) / 2;
    dst_pt.y += 58;
    text.Blit( dst_pt.x, dst_pt.y );

    dst_pt.x = box_rt.x;
    dst_pt.y = box_rt.y + space + window_icons.height() + space;
    box1.Blit( dst_pt.x, dst_pt.y );

    dst_pt.y += box1.h() + space;
    if ( isRequired ) {
        dst_pt.x = box_rt.x + ( box_rt.width - requires_text.w() ) / 2;
        requires_text.Blit( dst_pt.x, dst_pt.y );

        dst_pt.x = box_rt.x;
        dst_pt.y += requires_text.h();
        box2.Blit( dst_pt.x, dst_pt.y );

        dst_pt.y += box2.h() + space;
    }

    rbs.SetPos( dst_pt.x, dst_pt.y );
    rbs.Redraw();

    if ( buttons ) {
        if ( ALLOW_BUILD != castle.CheckBuyBuilding( building ) )
            button1.disable();

        button1.draw();
        button2.draw();
    }

    display.render();

    // message loop
    while ( le.HandleEvents() ) {
        if ( !buttons && !le.MousePressRight() )
            break;

        le.MousePressLeft( button1.area() ) ? button1.drawOnPress() : button1.drawOnRelease();
        le.MousePressLeft( button2.area() ) ? button2.drawOnPress() : button2.drawOnRelease();

        if ( button1.isEnabled() && ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( button1.area() ) ) )
            return true;

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( button2.area() ) )
            break;
    }

    return false;
}

const char * GetBuildConditionDescription( int bcond )
{
    switch ( bcond ) {
    case NOT_TODAY:
        return _( "Cannot build. You have already built here today." );

    case NEED_CASTLE:
        return _( "For this action it is necessary to build a castle first." );

    default:
        break;
    }

    return nullptr;
}

std::string BuildingInfo::GetConditionDescription() const
{
    std::string res;

    switch ( bcond ) {
    case NOT_TODAY:
    case NEED_CASTLE:
        return GetBuildConditionDescription( bcond );

    case BUILD_DISABLE:
        if ( building == BUILD_SHIPYARD ) {
            res = _( "Cannot build %{name} because castle is too far from water." );
            StringReplace( res, "%{name}", Castle::GetStringBuilding( BUILD_SHIPYARD, castle.GetRace() ) );
        }
        else {
            res = _( "disable build." );
        }
        break;

    case LACK_RESOURCES:
        res = _( "Cannot afford %{name}." );
        StringReplace( res, "%{name}", GetName() );
        break;

    case ALREADY_BUILT:
        res = _( "%{name} is already built." );
        StringReplace( res, "%{name}", GetName() );
        break;

    case REQUIRES_BUILD:
        res = _( "Cannot build %{name}." );
        StringReplace( res, "%{name}", GetName() );
        break;

    case ALLOW_BUILD:
        res = _( "Build %{name}." );
        StringReplace( res, "%{name}", GetName() );
        break;

    default:
        break;
    }

    return res;
}

void BuildingInfo::SetStatusMessage( StatusBar & bar ) const
{
    if ( building == BUILD_NOTHING ) {
        return;
    }

    switch ( bcond ) {
    case NOT_TODAY:
    case ALREADY_BUILT:
    case NEED_CASTLE:
    case BUILD_DISABLE:
    case LACK_RESOURCES:
    case REQUIRES_BUILD:
    case ALLOW_BUILD:
        bar.ShowMessage( GetConditionDescription() );
        break;

    default:
        break;
    }
}

DwellingItem::DwellingItem( const Castle & castle, uint32_t dw )
{
    type = castle.GetActualDwelling( dw );
    mons = Monster( castle.GetRace(), type );
}

DwellingsBar::DwellingsBar( Castle & cstl, const fheroes2::Size & sz )
    : castle( cstl )
    , backsf( sz.width, sz.height )
{
    backsf.reset();

    for ( uint32_t dw = DWELLING_MONSTER1; dw <= DWELLING_MONSTER6; dw <<= 1 )
        content.emplace_back( castle, dw );

    SetContent( content );

    fheroes2::DrawBorder( backsf, fheroes2::GetColorId( 0xd0, 0xc0, 0x48 ) );
    setSingleItemSize( sz );
}

void DwellingsBar::RedrawBackground( const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    fheroes2::Blit( backsf, dstsf, pos.x, pos.y );
}

void DwellingsBar::RedrawItem( DwellingItem & dwl, const fheroes2::Rect & pos, fheroes2::Image & dstsf )
{
    const fheroes2::Sprite & mons32 = fheroes2::AGG::GetICN( ICN::MONS32, dwl.mons.GetSpriteIndex() );
    fheroes2::Blit( mons32, dstsf, pos.x + ( pos.width - mons32.width() ) / 2, pos.y + ( pos.height - 3 - mons32.height() ) );

    if ( castle.isBuild( dwl.type ) ) {
        // count
        Text text( std::to_string( castle.getMonstersInDwelling( dwl.type ) ), Font::SMALL );
        text.Blit( pos.x + pos.width - text.w() - 3, pos.y + pos.height - text.h() - 1 );

        uint32_t grown = dwl.mons.GetGrown();
        if ( castle.isBuild( BUILD_WELL ) )
            grown += Castle::GetGrownWell();
        if ( castle.isBuild( BUILD_WEL2 ) && DWELLING_MONSTER1 == dwl.type )
            grown += Castle::GetGrownWel2();

        // grown
        text.Set( "+" + std::to_string( grown ), Font::YELLOW_SMALL );
        text.Blit( pos.x + pos.width - text.w() - 3, pos.y + 2 );
    }
    else
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CSLMARKER, 0 ), dstsf, pos.x + pos.width - 10, pos.y + 4 );
}

bool DwellingsBar::ActionBarLeftMouseSingleClick( DwellingItem & dwl )
{
    if ( castle.isBuild( dwl.type ) ) {
        castle.RecruitMonster( Dialog::RecruitMonster( dwl.mons, castle.getMonstersInDwelling( dwl.type ), true, -60 ) );
    }
    else if ( !castle.isBuild( BUILD_CASTLE ) )
        Dialog::Message( "", GetBuildConditionDescription( NEED_CASTLE ), Font::BIG, Dialog::OK );
    else {
        BuildingInfo dwelling( castle, static_cast<building_t>( dwl.type ) );

        if ( dwelling.DialogBuyBuilding( true ) ) {
            AudioManager::PlaySound( M82::BUILDTWN );
            castle.BuyBuilding( dwl.type );
        }
    }

    return true;
}

bool DwellingsBar::ActionBarRightMouseHold( DwellingItem & dwl )
{
    Dialog::DwellingInfo( dwl.mons, castle.getMonstersInDwelling( dwl.type ) );

    return true;
}
