/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include "ui_castle.h"

#include <array>
#include <cassert>
#include <string>
#include <vector>

#include "agg_image.h"
#include "castle.h"
#include "color.h"
#include "icn.h"
#include "image.h"
#include "logging.h"
#include "race.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "ui_text.h"

namespace
{
    const int32_t originalCastleFlagStartColorId = 152;
    const int32_t originalCastleFlagColorLength = 16 + 7;

    std::vector<uint8_t> getModifiedPalette( const uint8_t inputStartId, const uint8_t inputLength, const uint8_t outputStartId, const uint8_t outputLength )
    {
        assert( inputLength > 0 && outputLength > 0 );

        std::vector<uint8_t> output( 256 );
        for ( int32_t i = 0; i < 256; ++i ) {
            output[i] = static_cast<uint8_t>( i );
        }

        for ( uint8_t i = 0; i < inputLength; ++i ) {
            output[i + inputStartId] = outputStartId + i * outputLength / inputLength;
        }

        return output;
    }

    std::vector<uint8_t> getModifiedPaletteByPlayerColor( const uint8_t inputStartId, const uint8_t inputLength, const int playerColor )
    {
        switch ( playerColor ) {
        case Color::BLUE:
            return getModifiedPalette( inputStartId, inputLength, 63, 16 + 6 );
        case Color::GREEN:
            return getModifiedPalette( inputStartId, inputLength, 85, 16 + 7 );
        case Color::RED:
            return getModifiedPalette( inputStartId, inputLength, 175, 16 + 7 );
        case Color::YELLOW:
            return getModifiedPalette( inputStartId, inputLength, 108, 16 + 7 );
        case Color::ORANGE:
            return getModifiedPalette( inputStartId, inputLength, 199, 16 );
        case Color::PURPLE:
            return getModifiedPalette( inputStartId, inputLength, 132, 16 + 5 );
        default:
            // Did you add a new color? Please add the logic above.
            assert( 0 );
            break;
        }

        return getModifiedPalette( inputStartId, inputLength, inputStartId, inputLength );
    }

    std::vector<fheroes2::Rect> getColorEffectiveAreas( const int32_t icnId, const uint32_t icnIndex )
    {
        switch ( icnId ) {
        case ICN::TWNZCSTL: {
            if ( icnIndex == 0 ) {
                return { fheroes2::Rect( 76, 7, 3, 3 ), fheroes2::Rect( 96, 7, 3, 3 ), fheroes2::Rect( 178, 11, 3, 3 ) };
            }

            const fheroes2::Sprite & image = fheroes2::AGG::GetICN( icnId, icnIndex );
            return { fheroes2::Rect( 0, 0, image.width(), image.height() ) };
        }
        case ICN::TWNKCSTL: {
            if ( icnIndex == 0 ) {
                return { fheroes2::Rect( 127, 36, 3, 3 ), fheroes2::Rect( 287, 6, 3, 3 ) };
            }

            const fheroes2::Sprite & image = fheroes2::AGG::GetICN( icnId, icnIndex );
            return { fheroes2::Rect( 0, 0, image.width(), image.height() ) };
        }
        case ICN::TWNKDW_4:
        case ICN::TWNKUP_4:
            if ( icnIndex == 0 ) {
                return { fheroes2::Rect( 61, 3, 1, 1 ) };
            }

            return { fheroes2::Rect( 59, 0, 6, 5 ) };
        case ICN::TWNKLTUR: {
            if ( icnIndex == 0 ) {
                return { fheroes2::Rect( 5, 6, 3, 3 ) };
            }

            const fheroes2::Sprite & image = fheroes2::AGG::GetICN( icnId, icnIndex );
            return { fheroes2::Rect( 0, 0, image.width(), image.height() ) };
        }
        case ICN::TWNKRTUR: {
            if ( icnIndex == 0 ) {
                return { fheroes2::Rect( 55, 6, 3, 3 ) };
            }

            const fheroes2::Sprite & image = fheroes2::AGG::GetICN( icnId, icnIndex );
            return { fheroes2::Rect( 0, 0, image.width(), image.height() ) };
        }
        default:
            // You are calling this function for unsupported image ID. Verify your logic!
            assert( 0 );
            break;
        }

        return {};
    }

    fheroes2::Sprite getModifiedByColorImage( const int32_t icnId, const uint32_t icnIndex, const int32_t colorId )
    {
        const std::vector<fheroes2::Rect> regions = getColorEffectiveAreas( icnId, icnIndex );
        const std::vector<uint8_t> palette = getModifiedPaletteByPlayerColor( originalCastleFlagStartColorId, originalCastleFlagColorLength, colorId );

        fheroes2::Sprite temp = fheroes2::AGG::GetICN( icnId, icnIndex );

        for ( const fheroes2::Rect & roi : regions ) {
            fheroes2::ApplyPalette( temp, roi.x, roi.y, temp, roi.x, roi.y, roi.width, roi.height, palette );
        }

        return temp;
    }

    bool isImagePlayerColorDependent( const int32_t icnId )
    {
        switch ( icnId ) {
        case ICN::TWNZCSTL:
        case ICN::TWNKCSTL:
        case ICN::TWNKDW_4:
        case ICN::TWNKUP_4:
        case ICN::TWNKLTUR:
        case ICN::TWNKRTUR:
            return true;
        default:
            break;
        }

        return false;
    }
}

namespace fheroes2
{
    uint32_t getCastleIcnIndex( const int race, const bool isCastle )
    {
        switch ( race ) {
        case Race::KNGT:
            return isCastle ? 9 : 15;
        case Race::BARB:
            return isCastle ? 10 : 16;
        case Race::SORC:
            return isCastle ? 11 : 17;
        case Race::WRLK:
            return isCastle ? 12 : 18;
        case Race::WZRD:
            return isCastle ? 13 : 19;
        case Race::NECR:
            return isCastle ? 14 : 20;
        case Race::RAND:
            // It is used in Editor to select random castle as a victory/loss special condition.
            return isCastle ? 25 : 26;
        default:
            assert( 0 );
            DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown race" )
        }

        return 1;
    }

    uint32_t getCastleLeftFlagIcnIndex( const int color )
    {
        switch ( color ) {
        case Color::BLUE:
            return 0;
        case Color::GREEN:
            return 2;
        case Color::RED:
            return 4;
            break;
        case Color::YELLOW:
            return 6;
        case Color::ORANGE:
            return 8;
        case Color::PURPLE:
            return 10;
        case Color::NONE:
            return 12;
        default:
            // Have you added a new player color? Update the logic above.
            assert( 0 );
            break;
        }

        return 0;
    }

    void drawCastleIcon( const Castle & castle, Image & output, const Point & offset )
    {
        const uint32_t icnIndex = getCastleIcnIndex( castle.GetRace(), castle.isCastle() );

        const Sprite & castleImage = AGG::GetICN( Settings::Get().isEvilInterfaceEnabled() ? ICN::LOCATORE : ICN::LOCATORS, icnIndex );
        Copy( castleImage, 0, 0, output, offset.x, offset.y, castleImage.width(), castleImage.height() );

        // Draw castle's marker.
        switch ( Castle::GetAllBuildingStatus( castle ) ) {
        case BuildingStatus::NOT_TODAY:
            Blit( AGG::GetICN( ICN::CSLMARKER, 0 ), output, offset.x + 40, offset.y );
            break;
        case BuildingStatus::REQUIRES_BUILD:
            Blit( AGG::GetICN( ICN::CSLMARKER, 1 ), output, offset.x + 40, offset.y );
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

        fheroes2::Text text( std::to_string( kingdomTreasures.wood ), fontType );
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

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::BUTTON_EXIT_TOWN, 0 ), output, roi.x + 1, roi.y + 166 );

        return roi;
    }

    void drawCastleName( const Castle & castle, Image & output, const Point & offset )
    {
        const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::TOWNNAME, 0 );
        fheroes2::Blit( background, fheroes2::Display::instance(), offset.x + 320 - background.width() / 2, offset.y + 248 );

        const fheroes2::Text text( castle.GetName(), fheroes2::FontType::smallWhite() );
        text.draw( offset.x + 320 - text.width() / 2, offset.y + 250, output );
    }

    void drawCastleDialogBuilding( const int32_t icnId, const uint32_t icnIndex, const Castle & castle, const Point & offset, const Rect & renderArea,
                                   const uint8_t alpha )
    {
        const fheroes2::Sprite & image
            = isImagePlayerColorDependent( icnId ) ? getModifiedByColorImage( icnId, icnIndex, castle.GetColor() ) : AGG::GetICN( icnId, icnIndex );

        const fheroes2::Rect imageRoi{ offset.x + image.x(), offset.y + image.y(), image.width(), image.height() };
        const fheroes2::Rect overlappedRoi = renderArea ^ imageRoi;

        fheroes2::AlphaBlit( image, overlappedRoi.x - imageRoi.x, overlappedRoi.y - imageRoi.y, fheroes2::Display::instance(), overlappedRoi.x, overlappedRoi.y,
                             overlappedRoi.width, overlappedRoi.height, alpha );
    }
}
