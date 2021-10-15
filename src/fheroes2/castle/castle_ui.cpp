/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "castle_ui.h"
#include "agg_image.h"
#include "castle.h"
#include "icn.h"
#include "logging.h"
#include "race.h"
#include "settings.h"
#include "ui_text.h"

#include <array>
#include <cassert>

namespace fheroes2
{
    void drawCastleIcon( const Castle & castle, Image & output, const Point & offset )
    {
        uint32_t icnIndex = 1;

        switch ( castle.GetRace() ) {
        case Race::KNGT:
            icnIndex = castle.isCastle() ? 9 : 15;
            break;
        case Race::BARB:
            icnIndex = castle.isCastle() ? 10 : 16;
            break;
        case Race::SORC:
            icnIndex = castle.isCastle() ? 11 : 17;
            break;
        case Race::WRLK:
            icnIndex = castle.isCastle() ? 12 : 18;
            break;
        case Race::WZRD:
            icnIndex = castle.isCastle() ? 13 : 19;
            break;
        case Race::NECR:
            icnIndex = castle.isCastle() ? 14 : 20;
            break;
        default:
            assert( 0 );
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "unknown race" );
        }

        const Sprite & castleImage = fheroes2::AGG::GetICN( Settings::Get().ExtGameEvilInterface() ? ICN::LOCATORE : ICN::LOCATORS, icnIndex );
        fheroes2::Blit( castleImage, output, offset.x, offset.y );

        // Draw castle's marker.
        switch ( Castle::GetAllBuildingStatus( castle ) ) {
        case NOT_TODAY:
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CSLMARKER, 0 ), output, offset.x + 40, offset.y );
            break;
        case REQUIRES_BUILD:
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CSLMARKER, 1 ), output, offset.x + 40, offset.y );
            break;
        default:
            break;
        }
    }

    Rect drawResourcePanel( const Funds & kingdomTreasures, Image & output, const Point & offset )
    {
        const fheroes2::Rect roi( offset.x + 552, offset.y + 262, 82, 192 );
        fheroes2::Fill( output, roi.x, roi.y, roi.width, roi.height, 0 );

        // Maximum width is 39 pixels (except gold), maximum height is 32 pixels
        const int32_t maxWidth = 39;
        const int32_t maxHeight = 32;
        const int32_t leftColumnOffset = roi.x + 1;
        const int32_t rightColumnOffset = roi.x + 1 + maxWidth + 2;

        const fheroes2::FontType fontType( fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE );
        const int32_t fontHeight = fheroes2::Text( std::string(), fontType ).height();

        const std::array<int32_t, 4> offsetY = { 0, maxHeight + fontHeight + 2, ( maxHeight + fontHeight ) * 2 - 1, ( maxHeight + fontHeight ) * 3 + 1 };

        const fheroes2::Sprite & woodImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 0 );
        const fheroes2::Sprite & mercuryImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 1 );
        const fheroes2::Sprite & oreImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 2 );
        const fheroes2::Sprite & sulfurImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 3 );
        const fheroes2::Sprite & crystalImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 4 );
        const fheroes2::Sprite & gemsImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 5 );
        const fheroes2::Sprite & goldImage = fheroes2::AGG::GetICN( ICN::RESOURCE, 6 );

        fheroes2::Blit( woodImage, output, leftColumnOffset + ( maxWidth - woodImage.width() ) / 2, roi.y + offsetY[0] + maxHeight - woodImage.height() );
        fheroes2::Blit( sulfurImage, output, rightColumnOffset + ( maxWidth - sulfurImage.width() ) / 2, roi.y + offsetY[0] + maxHeight - sulfurImage.height() );

        fheroes2::Blit( crystalImage, output, leftColumnOffset + ( maxWidth - crystalImage.width() ) / 2, roi.y + offsetY[1] + maxHeight - crystalImage.height() );
        fheroes2::Blit( mercuryImage, output, rightColumnOffset + ( maxWidth - mercuryImage.width() ) / 2, roi.y + offsetY[1] + maxHeight - mercuryImage.height() );

        fheroes2::Blit( oreImage, output, leftColumnOffset + ( maxWidth - oreImage.width() ) / 2, roi.y + offsetY[2] + maxHeight - oreImage.height() );
        fheroes2::Blit( gemsImage, output, rightColumnOffset + ( maxWidth - gemsImage.width() ) / 2, roi.y + offsetY[2] + maxHeight - gemsImage.height() );

        fheroes2::Blit( goldImage, output, roi.x + ( roi.width - goldImage.width() ) / 2, roi.y + offsetY[3] );

        fheroes2::Text text;
        text.set( std::to_string( kingdomTreasures.wood ), fontType );
        text.draw( leftColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[0] + maxHeight + 1, output );

        text.set( std::to_string( kingdomTreasures.sulfur ), fontType );
        text.draw( rightColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[0] + maxHeight + 1, output );

        text.set( std::to_string( kingdomTreasures.crystal ), fontType );
        text.draw( leftColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[1] + maxHeight + 1, output );

        text.set( std::to_string( kingdomTreasures.mercury ), fontType );
        text.draw( rightColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[1] + maxHeight + 1, output );

        text.set( std::to_string( kingdomTreasures.ore ), fontType );
        text.draw( leftColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[2] + maxHeight + 1, output );

        text.set( std::to_string( kingdomTreasures.gems ), fontType );
        text.draw( rightColumnOffset + ( maxWidth - text.width() ) / 2, roi.y + offsetY[2] + maxHeight + 1, output );

        text.set( std::to_string( kingdomTreasures.gold ), fontType );
        text.draw( roi.x + ( roi.width - text.width() ) / 2, roi.y + offsetY[3] + goldImage.height() + 1, output );

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::TREASURY, 1 ), output, roi.x + 1, roi.y + 166 );

        return roi;
    }

    void drawCastleName( const Castle & castle, Image & output, const Point & offset )
    {
        const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::TOWNNAME, 0 );
        fheroes2::Blit( background, fheroes2::Display::instance(), offset.x + 320 - background.width() / 2, offset.y + 248 );

        const fheroes2::Text text( castle.GetName(), { fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE } );
        text.draw( offset.x + 320 - text.width() / 2, offset.y + 250, output );
    }

    Sprite getHeroExchangeImage()
    {
        const Sprite & sprite = fheroes2::AGG::GetICN( ICN::ADVMCO, 8 );

        Sprite result( sprite.width() + 4, sprite.height() + 4 );
        result.fill( 0 );

        DrawBorder( result, fheroes2::GetColorId( 0xe0, 0xb4, 0 ) );
        Blit( sprite, result, 2, 2 );

        return result;
    }
}
