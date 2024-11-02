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

#include "buildinginfo.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include "agg_image.h"
#include "army_troop.h"
#include "audio_manager.h"
#include "castle.h"
#include "castle_building_info.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "localevent.h"
#include "m82.h"
#include "maps_fileinfo.h"
#include "monster.h"
#include "pal.h"
#include "payment.h"
#include "profit.h"
#include "race.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "statusbar.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_text.h"

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

    const char * GetBuildConditionDescription( const BuildingStatus status )
    {
        switch ( status ) {
        case BuildingStatus::NOT_TODAY:
            return _( "Cannot build. You have already built here today." );

        case BuildingStatus::NEED_CASTLE:
            return _( "For this action it is necessary to build a castle first." );

        default:
            break;
        }

        return nullptr;
    }
}

struct BuildingStats
{
    uint32_t id2;
    uint8_t race;
    Cost cost;
};

const BuildingStats buildingStats[] = {
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

Funds BuildingInfo::GetCost( uint32_t build, int race )
{
    Funds payment;
    const BuildingStats * ptr = &buildingStats[0];

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

BuildingInfo::BuildingInfo( const Castle & c, const BuildingType b )
    : castle( c )
    , _buildingType( b )
    , area( 0, 0, 135, 70 )
    , _status( BuildingStatus::ALLOW_BUILD )
{
    if ( isDwelling( _buildingType ) )
        _buildingType = castle.GetActualDwelling( b );

    _buildingType = castle.isBuild( b ) ? castle.GetUpgradeBuilding( b ) : b;

    if ( BUILD_TAVERN == _buildingType && Race::NECR == castle.GetRace() ) {
        const GameVersion version = Settings::Get().getCurrentMapInfo().version;
        _buildingType = ( version == GameVersion::PRICE_OF_LOYALTY || version == GameVersion::RESURRECTION ) ? BUILD_SHRINE : BUILD_NOTHING;
    }

    _status = castle.CheckBuyBuilding( _buildingType );

    // generate description
    if ( _status == BuildingStatus::BUILD_DISABLE || _status == BuildingStatus::SHIPYARD_NOT_ALLOWED )
        description = GetConditionDescription();
    else {
        description = getBuildingDescription( castle.GetRace(), _buildingType );
    }

    // fix area for captain
    if ( b == BUILD_CAPTAIN ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::getCaptainIcnId( castle.GetRace() ), ( _buildingType & BUILD_CAPTAIN ? 1 : 0 ) );
        area.width = sprite.width();
        area.height = sprite.height();
    }
}

void BuildingInfo::SetPos( int32_t x, int32_t y )
{
    area.x = x;
    area.y = y;
}

bool BuildingInfo::isDwelling( const uint32_t building )
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

std::string BuildingInfo::getBuildingDescription( const int race, const uint32_t buildingId )
{
    std::string description;

    if ( isDwelling( buildingId ) ) {
        description = _( "The %{building} produces %{monster}." );
        StringReplace( description, "%{building}", Castle::GetStringBuilding( buildingId, race ) );
        if ( race == Race::RAND ) {
            StringReplace( description, "%{monster}", Monster::getRandomRaceMonstersName( buildingId ) );
        }
        else {
            StringReplaceWithLowercase( description, "%{monster}", Monster( race, buildingId ).GetMultiName() );
        }
    }
    else {
        description = fheroes2::getBuildingDescription( race, static_cast<BuildingType>( buildingId ) );

        switch ( buildingId ) {
        case BUILD_WELL:
            StringReplace( description, "%{count}", Castle::GetGrownWell() );
            break;
        case BUILD_WEL2:
            StringReplace( description, "%{count}", Castle::GetGrownWel2() );
            break;
        case BUILD_CASTLE:
        case BUILD_STATUE:
        case BUILD_SPEC: {
            const Funds profit = ProfitConditions::FromBuilding( buildingId, race );
            StringReplace( description, "%{count}", profit.gold );
            break;
        }
        default:
            break;
        }
    }

    return description;
}

void BuildingInfo::RedrawCaptain() const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    if ( _status == BuildingStatus::ALREADY_BUILT ) {
        const fheroes2::Sprite & captainSprite = fheroes2::AGG::GetICN( ICN::getCaptainIcnId( castle.GetRace() ), 1 );
        const fheroes2::Sprite & flag = fheroes2::AGG::GetICN( ICN::getFlagIcnId( castle.GetColor() ), 0 );

        fheroes2::Blit( captainSprite, display, area.x, area.y );
        const fheroes2::Point flagOffset = GetFlagOffset( castle.GetRace() );
        fheroes2::Blit( flag, display, area.x + flagOffset.x, area.y + flagOffset.y );
    }
    else {
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::getCaptainIcnId( castle.GetRace() ), 0 ), display, area.x, area.y );
    }

    // indicator
    if ( _status == BuildingStatus::ALREADY_BUILT ) {
        const fheroes2::Sprite & spriteAllow = fheroes2::AGG::GetICN( ICN::TOWNWIND, 11 );
        fheroes2::Blit( spriteAllow, display, area.x + 83 - 4 - spriteAllow.width(), area.y + 79 - 2 - spriteAllow.height() );
    }
    else if ( _status != BuildingStatus::ALLOW_BUILD ) {
        if ( BuildingStatus::LACK_RESOURCES == _status ) {
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
    if ( BUILD_CAPTAIN == _buildingType ) {
        RedrawCaptain();
        return;
    }

    fheroes2::Display & display = fheroes2::Display::instance();
    const int index = fheroes2::getIndexBuildingSprite( static_cast<BuildingType>( _buildingType ) );

    const fheroes2::Sprite & buildingFrame = fheroes2::AGG::GetICN( ICN::BLDGXTRA, 0 );
    fheroes2::Blit( buildingFrame, display, area.x, area.y );
    if ( _status == BuildingStatus::BUILD_DISABLE || _status == BuildingStatus::SHIPYARD_NOT_ALLOWED ) {
        const fheroes2::Point offset( 6, 59 );
        fheroes2::Sprite grayedOut = fheroes2::Crop( buildingFrame, offset.x, offset.y, 125, 12 );
        fheroes2::ApplyPalette( grayedOut, PAL::GetPalette( PAL::PaletteType::GRAY ) );
        fheroes2::ApplyPalette( grayedOut, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
        fheroes2::Copy( grayedOut, 0, 0, display, area.x + offset.x, area.y + offset.y, 125, 12 );
    }

    // build image
    if ( BUILD_NOTHING == _buildingType ) {
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        const fheroes2::Sprite & buildBackground = fheroes2::AGG::GetICN( isEvilInterface ? ICN::CASLXTRA_EVIL : ICN::CASLXTRA, 0 );
        fheroes2::Copy( buildBackground, 0, 0, display, area.x, area.y, buildBackground.width(), buildBackground.height() );
        return;
    }

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::getBuildingIcnId( castle.GetRace() ), index ), display, area.x + 1, area.y + 1 );

    // indicator
    if ( _status == BuildingStatus::ALREADY_BUILT ) {
        const fheroes2::Sprite & spriteAllow = fheroes2::AGG::GetICN( ICN::TOWNWIND, 11 );
        fheroes2::Blit( spriteAllow, display, area.x + buildingFrame.width() - 5 - spriteAllow.width(), area.y + 58 - 2 - spriteAllow.height() );
    }
    else if ( _status == BuildingStatus::BUILD_DISABLE || _status == BuildingStatus::SHIPYARD_NOT_ALLOWED ) {
        const fheroes2::Sprite & spriteDeny = fheroes2::AGG::GetICN( ICN::TOWNWIND, 12 );
        fheroes2::Sprite disabledSprite( spriteDeny );
        fheroes2::ApplyPalette( disabledSprite, PAL::GetPalette( PAL::PaletteType::GRAY ) );
        fheroes2::ApplyPalette( disabledSprite, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
        fheroes2::Blit( disabledSprite, display, area.x + buildingFrame.width() - 5 + 1 - spriteDeny.width(), area.y + 58 - 2 - spriteDeny.height() );
    }
    else if ( _status != BuildingStatus::ALLOW_BUILD ) {
        if ( BuildingStatus::LACK_RESOURCES == _status ) {
            const fheroes2::Sprite & spriteMoney = fheroes2::AGG::GetICN( ICN::TOWNWIND, 13 );
            fheroes2::Blit( spriteMoney, display, area.x + buildingFrame.width() - 5 + 1 - spriteMoney.width(), area.y + 58 - 3 - spriteMoney.height() );
        }
        else {
            const fheroes2::Sprite & spriteDeny = fheroes2::AGG::GetICN( ICN::TOWNWIND, 12 );
            fheroes2::Blit( spriteDeny, display, area.x + buildingFrame.width() - 5 + 1 - spriteDeny.width(), area.y + 58 - 2 - spriteDeny.height() );
        }

        const fheroes2::Sprite & textBackground = fheroes2::AGG::GetICN( ICN::CASLXTRA, 2 );
        fheroes2::Copy( textBackground, 0, 0, display, area.x, area.y + 58, textBackground.width(), textBackground.height() );
    }
    else {
        const fheroes2::Sprite & textBackground = fheroes2::AGG::GetICN( ICN::CASLXTRA, 1 );
        fheroes2::Copy( textBackground, 0, 0, display, area.x, area.y + 58, textBackground.width(), textBackground.height() );
    }

    const fheroes2::Text buildingName( Castle::GetStringBuilding( _buildingType, castle.GetRace() ), fheroes2::FontType::smallWhite() );
    buildingName.draw( area.x + 68 - buildingName.width() / 2, area.y + 61, display );
}

const char * BuildingInfo::GetName() const
{
    return Castle::GetStringBuilding( _buildingType, castle.GetRace() );
}

bool BuildingInfo::QueueEventProcessing( fheroes2::ButtonBase & exitButton ) const
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.MouseClickLeft( area ) ) {
        if ( _status == BuildingStatus::LACK_RESOURCES || _status == BuildingStatus::ALLOW_BUILD ) {
            const fheroes2::ButtonRestorer exitRestorer( exitButton );
            return DialogBuyBuilding( true );
        }
    }
    else if ( le.isMouseRightButtonPressedInArea( area ) ) {
        DialogBuyBuilding( false );
    }
    return false;
}

bool BuildingInfo::DialogBuyBuilding( bool buttons ) const
{
    if ( _buildingType == BUILD_NOTHING ) {
        return false;
    }

    const CursorRestorer cursorRestorer( buttons, Cursor::POINTER );

    std::string extendedDescription = description;

    if ( BuildingStatus::ALLOW_BUILD != _status ) {
        const std::string & ext = GetConditionDescription();
        if ( !ext.empty() && ext != description ) {
            extendedDescription.append( "\n\n" );
            extendedDescription.append( ext );
        }
    }

    const fheroes2::Text descriptionText( std::move( extendedDescription ), fheroes2::FontType::normalWhite() );

    // prepare requirement build string
    std::string requirement;

    if ( _status != BuildingStatus::BUILD_DISABLE ) {
        requirement = fheroes2::getBuildingRequirementString( castle.GetRace(), static_cast<BuildingType>( _buildingType ) );
    }

    const bool requirementsPresent = !requirement.empty();

    const fheroes2::Text requirementTitle( _( "Requires:" ), fheroes2::FontType::normalWhite() );
    const fheroes2::Text requirementText( std::move( requirement ), fheroes2::FontType::normalWhite() );

    const int elementOffset = 10;

    int32_t requirementHeight = 0;
    if ( requirementsPresent ) {
        requirementHeight = requirementTitle.height() + requirementText.height( fheroes2::boxAreaWidthPx ) + elementOffset;
    }

    Resource::BoxSprite rbs( PaymentConditions::BuyBuilding( castle.GetRace(), _buildingType ), fheroes2::boxAreaWidthPx );

    const fheroes2::Sprite & buildingFrame = fheroes2::AGG::GetICN( ICN::BLDGXTRA, 0 );

    const int32_t totalDialogHeight = elementOffset + buildingFrame.height() + elementOffset + descriptionText.height( fheroes2::boxAreaWidthPx ) + elementOffset
                                      + requirementHeight + rbs.GetArea().height;

    const Dialog::FrameBox dialogFrame( totalDialogHeight, buttons );
    const fheroes2::Rect & dialogRoi = dialogFrame.GetArea();

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int buttonOkayIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_OKAY_BUTTON : ICN::UNIFORM_GOOD_OKAY_BUTTON;

    fheroes2::Point pos{ dialogRoi.x, dialogRoi.y + dialogRoi.height - fheroes2::AGG::GetICN( buttonOkayIcnID, 0 ).height() };
    fheroes2::Button buttonOkay( pos.x, pos.y, buttonOkayIcnID, 0, 1 );

    const int buttonCancelIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_CANCEL_BUTTON : ICN::UNIFORM_GOOD_CANCEL_BUTTON;

    pos.x = dialogRoi.x + dialogRoi.width - fheroes2::AGG::GetICN( buttonCancelIcnID, 0 ).width();
    pos.y = dialogRoi.y + dialogRoi.height - fheroes2::AGG::GetICN( buttonCancelIcnID, 0 ).height();
    fheroes2::Button buttonCancel( pos.x, pos.y, buttonCancelIcnID, 0, 1 );

    pos.x = dialogRoi.x + ( dialogRoi.width - buildingFrame.width() ) / 2;
    pos.y = dialogRoi.y + elementOffset;

    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Blit( buildingFrame, display, pos.x, pos.y );

    const fheroes2::Sprite & buildingImage
        = fheroes2::AGG::GetICN( ICN::getBuildingIcnId( castle.GetRace() ), fheroes2::getIndexBuildingSprite( static_cast<BuildingType>( _buildingType ) ) );
    pos.x = dialogRoi.x + ( dialogRoi.width - buildingImage.width() ) / 2;
    pos.y += 1;
    fheroes2::Blit( buildingImage, display, pos.x, pos.y );

    const fheroes2::Text buildingName( GetName(), fheroes2::FontType::smallWhite() );
    pos.x = dialogRoi.x + ( dialogRoi.width - buildingName.width() ) / 2;
    pos.y += 58;
    buildingName.draw( pos.x, pos.y + 2, display );

    pos.x = dialogRoi.x;
    pos.y = dialogRoi.y + elementOffset + buildingFrame.height() + elementOffset;
    descriptionText.draw( pos.x, pos.y + 2, fheroes2::boxAreaWidthPx, display );

    pos.y += descriptionText.height( fheroes2::boxAreaWidthPx ) + elementOffset;
    if ( requirementsPresent ) {
        pos.x = dialogRoi.x + ( dialogRoi.width - requirementTitle.width() ) / 2;
        requirementTitle.draw( pos.x, pos.y + 2, display );

        pos.x = dialogRoi.x;
        pos.y += requirementTitle.height();
        requirementText.draw( pos.x, pos.y + 2, fheroes2::boxAreaWidthPx, display );

        pos.y += requirementText.height( fheroes2::boxAreaWidthPx ) + elementOffset;
    }

    rbs.SetPos( pos.x, pos.y );
    rbs.Redraw();

    if ( buttons ) {
        if ( BuildingStatus::ALLOW_BUILD != castle.CheckBuyBuilding( _buildingType ) ) {
            buttonOkay.disable();
        }

        buttonOkay.draw();
        buttonCancel.draw();
    }
    else {
        buttonOkay.disable();
        buttonOkay.hide();

        buttonCancel.disable();
        buttonCancel.hide();
    }

    display.render();

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        if ( !buttons && !le.isMouseRightButtonPressed() ) {
            break;
        }

        le.isMouseLeftButtonPressedInArea( buttonOkay.area() ) ? buttonOkay.drawOnPress() : buttonOkay.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        if ( buttonOkay.isEnabled() && ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOkay.area() ) ) ) {
            return true;
        }

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancel.area() ) ) {
            break;
        }

        if ( buttonOkay.isVisible() && le.isMouseRightButtonPressedInArea( buttonOkay.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Okay" ), GetConditionDescription(), Dialog::ZERO );
        }
        else if ( buttonCancel.isVisible() && le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
        }
    }

    return false;
}

std::string BuildingInfo::GetConditionDescription() const
{
    switch ( _status ) {
    case BuildingStatus::NOT_TODAY:
    case BuildingStatus::NEED_CASTLE:
        return GetBuildConditionDescription( _status );

    case BuildingStatus::SHIPYARD_NOT_ALLOWED: {
        assert( _buildingType == BUILD_SHIPYARD );
        std::string res = _( "Cannot build %{name}. The castle is too far away from an ocean." );
        StringReplace( res, "%{name}", Castle::GetStringBuilding( BUILD_SHIPYARD, castle.GetRace() ) );
        return res;
    }

    case BuildingStatus::BUILD_DISABLE:
        return _( "This building has been disabled." );

    case BuildingStatus::LACK_RESOURCES: {
        std::string res = _( "Cannot afford the %{name}." );
        StringReplace( res, "%{name}", GetName() );
        return res;
    }

    case BuildingStatus::ALREADY_BUILT: {
        std::string res = _( "The %{name} is already built." );
        StringReplace( res, "%{name}", GetName() );
        return res;
    }

    case BuildingStatus::REQUIRES_BUILD: {
        std::string res = _( "Cannot build the %{name}." );
        StringReplace( res, "%{name}", GetName() );
        return res;
    }

    case BuildingStatus::ALLOW_BUILD: {
        std::string res = _( "Build %{name}." );
        StringReplace( res, "%{name}", GetName() );
        return res;
    }

    default:
        break;
    }

    return {};
}

void BuildingInfo::SetStatusMessage( StatusBar & bar ) const
{
    if ( _buildingType == BUILD_NOTHING ) {
        return;
    }

    switch ( _status ) {
    case BuildingStatus::NOT_TODAY:
    case BuildingStatus::ALREADY_BUILT:
    case BuildingStatus::NEED_CASTLE:
    case BuildingStatus::BUILD_DISABLE:
    case BuildingStatus::SHIPYARD_NOT_ALLOWED:
    case BuildingStatus::LACK_RESOURCES:
    case BuildingStatus::REQUIRES_BUILD:
    case BuildingStatus::ALLOW_BUILD:
        bar.ShowMessage( GetConditionDescription() );
        break;

    default:
        break;
    }
}

DwellingsBar::DwellingsBar( Castle & cstl, const fheroes2::Size & sz )
    : castle( cstl )
    , backsf( sz.width, sz.height )
{
    backsf.reset();

    content.reserve( CountBits( DWELLING_MONSTERS ) );

    for ( uint32_t dw = DWELLING_MONSTER1; dw <= DWELLING_MONSTER6; dw <<= 1 ) {
        content.emplace_back( dw );
    }

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
    const uint32_t dwType = castle.GetActualDwelling( dwl.dwType );
    const Monster mons{ castle.GetRace(), dwType };

    const fheroes2::Sprite & mons32 = fheroes2::AGG::GetICN( ICN::MONS32, mons.GetSpriteIndex() );
    fheroes2::Blit( mons32, dstsf, pos.x + ( pos.width - mons32.width() ) / 2, pos.y + ( pos.height - 3 - mons32.height() ) );

    if ( !castle.isBuild( dwType ) ) {
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CSLMARKER, 0 ), dstsf, pos.x + pos.width - 10, pos.y + 4 );

        return;
    }

    // Units available for hire.
    fheroes2::Text text( std::to_string( castle.getMonstersInDwelling( dwType ) ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + pos.width - text.width() - 3, pos.y + pos.height - text.height() + 1, dstsf );

    uint32_t grown = mons.GetGrown();
    if ( castle.isBuild( BUILD_WELL ) ) {
        grown += Castle::GetGrownWell();
    }
    if ( castle.isBuild( BUILD_WEL2 ) && DWELLING_MONSTER1 == dwType ) {
        grown += Castle::GetGrownWel2();
    }

    // Dwelling's growth.
    text.set( "+" + std::to_string( grown ), fheroes2::FontType::smallYellow() );
    text.draw( pos.x + pos.width - text.width() - 3, pos.y + 4, dstsf );
}

bool DwellingsBar::ActionBarLeftMouseSingleClick( DwellingItem & dwl )
{
    const uint32_t dwType = castle.GetActualDwelling( dwl.dwType );

    if ( castle.isBuild( dwType ) ) {
        castle.RecruitMonster( Dialog::RecruitMonster( { castle.GetRace(), dwType }, castle.getMonstersInDwelling( dwType ), true, -60 ) );
    }
    else if ( !castle.isBuild( BUILD_CASTLE ) )
        fheroes2::showStandardTextMessage( "", GetBuildConditionDescription( BuildingStatus::NEED_CASTLE ), Dialog::OK );
    else {
        const BuildingInfo _dwelling( castle, static_cast<BuildingType>( dwType ) );

        if ( _dwelling.DialogBuyBuilding( true ) ) {
            AudioManager::PlaySound( M82::BUILDTWN );
            castle.BuyBuilding( dwType );
        }
    }

    return true;
}

bool DwellingsBar::ActionBarRightMouseHold( DwellingItem & dwl )
{
    const uint32_t dwType = castle.GetActualDwelling( dwl.dwType );

    Dialog::DwellingInfo( { castle.GetRace(), dwType }, castle.getMonstersInDwelling( dwType ) );

    return true;
}
