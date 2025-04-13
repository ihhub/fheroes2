/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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

#include "ui_map_object.h"

#if defined( WITH_DEBUG )
#include <map>
#include <set>
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "agg_image.h"
#include "ground.h"
#include "map_object_info.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"

namespace
{
    const int32_t tileSize{ 32 };

    void renderObjectPart( fheroes2::Sprite & image, const Maps::ObjectPartInfo & objectPart, const fheroes2::Point & minOffset )
    {
        const int icnId = MP2::getIcnIdFromObjectIcnType( objectPart.icnType );

        const fheroes2::Sprite & imagePart = fheroes2::AGG::GetICN( icnId, objectPart.icnIndex );
        Blit( imagePart, 0, 0, image, ( objectPart.tileOffset.x - minOffset.x ) * tileSize + imagePart.x(),
              ( objectPart.tileOffset.y - minOffset.y ) * tileSize + imagePart.y(), imagePart.width(), imagePart.height() );

        if ( objectPart.animationFrames > 0 ) {
            const fheroes2::Sprite & animationFrame = fheroes2::AGG::GetICN( icnId, objectPart.icnIndex + 1 );
            Blit( animationFrame, 0, 0, image, ( objectPart.tileOffset.x - minOffset.x ) * tileSize + animationFrame.x(),
                  ( objectPart.tileOffset.y - minOffset.y ) * tileSize + animationFrame.y(), animationFrame.width(), animationFrame.height() );
        }
    }

    void renderObject( fheroes2::Sprite & image, const Maps::ObjectInfo & object, const fheroes2::Point & minOffset )
    {
        for ( const auto & objectPart : object.groundLevelParts ) {
            renderObjectPart( image, objectPart, minOffset );
        }

        for ( const auto & objectPart : object.topLevelParts ) {
            renderObjectPart( image, objectPart, minOffset );
        }
    }

    void getMinMaxOffsets( const Maps::ObjectInfo & object, fheroes2::Point & minOffset, fheroes2::Point & maxOffset )
    {
        for ( const auto & objectPart : object.groundLevelParts ) {
            minOffset.x = std::min( minOffset.x, objectPart.tileOffset.x );
            minOffset.y = std::min( minOffset.y, objectPart.tileOffset.y );

            maxOffset.x = std::max( maxOffset.x, objectPart.tileOffset.x );
            maxOffset.y = std::max( maxOffset.y, objectPart.tileOffset.y );
        }

        for ( const auto & objectPart : object.topLevelParts ) {
            minOffset.x = std::min( minOffset.x, objectPart.tileOffset.x );
            minOffset.y = std::min( minOffset.y, objectPart.tileOffset.y );

            maxOffset.x = std::max( maxOffset.x, objectPart.tileOffset.x );
            maxOffset.y = std::max( maxOffset.y, objectPart.tileOffset.y );
        }
    }
}

namespace fheroes2
{
    Sprite generateMapObjectImage( const Maps::ObjectInfo & object )
    {
        if ( object.groundLevelParts.empty() ) {
            // Why are you passing an empty object? Check your logic!
            assert( 0 );
            return {};
        }

        // The first object part must always have no offset.
        assert( object.groundLevelParts.front().tileOffset == Point() );

        if ( object.groundLevelParts.size() == 1 && object.topLevelParts.empty() ) {
            const Maps::ObjectPartInfo & objectPart = object.groundLevelParts.front();
            Sprite image;

            if ( objectPart.animationFrames > 0 ) {
                image.resize( tileSize, tileSize );
                image.reset();

                renderObjectPart( image, objectPart, { 0, 0 } );
            }
            else {
                image = AGG::GetICN( MP2::getIcnIdFromObjectIcnType( objectPart.icnType ), objectPart.icnIndex );
            }

            // If it is a one tile image make sure that the offset is in the middle of the image.
            image.setPosition( -image.width() / 2, -image.height() / 2 );
            return image;
        }

#if defined( WITH_DEBUG )
        // Verify that all offsets are unique.
        std::map<MP2::ObjectIcnType, std::set<Point>> uniqueOffsets;
        for ( const auto & objectPart : object.groundLevelParts ) {
            if ( const auto [dummy, inserted] = uniqueOffsets[objectPart.icnType].emplace( objectPart.tileOffset ); !inserted ) {
                // The object hasn't formed properly!
                assert( 0 );
            }
        }

        for ( const auto & objectPart : object.topLevelParts ) {
            if ( const auto [dummy, inserted] = uniqueOffsets[objectPart.icnType].emplace( objectPart.tileOffset ); !inserted ) {
                // The object hasn't formed properly!
                assert( 0 );
            }
        }
#endif

        // Calculate the required size of the object in tiles.
        Point minOffset;
        Point maxOffset;
        getMinMaxOffsets( object, minOffset, maxOffset );

        // We can use an approximate size of the object based on tiles it needs
        // but make sure that the offset corresponds to { 0, 0 } tile offset's center.
        // In other words, the main tile should be the center of the image.
        const int32_t width{ ( maxOffset.x - minOffset.x + 1 ) * tileSize };
        const int32_t height{ ( maxOffset.y - minOffset.y + 1 ) * tileSize };
        const Point imageOffset{ ( minOffset.x * tileSize ) - tileSize / 2, ( minOffset.y * tileSize ) - tileSize / 2 };

        Sprite image{ width, height, imageOffset.x, imageOffset.y };
        // Since we don't generate a pixel precise image make it transparent at first.
        image.reset();

        renderObject( image, object, minOffset );

        return image;
    }

    Sprite generateTownObjectImage( const int townType, const int color, const int groundId )
    {
        if ( townType < 0 || color < 0 ) {
            assert( 0 );
            return {};
        }

        // Towns cannot be placed on water but we have to display them as cursor.
        // So we are showing town basement as Grass for Water terrain.
        const int32_t basementOffset = getTownBasementId( ( groundId == Maps::Ground::WATER ) ? Maps::Ground::GRASS : groundId );

        // NOTICE: This calculations should be consistent with objects position in KINGDOM_TOWNS and LANDSCAPE_FLAGS vectors.
        assert( Maps::getObjectsByGroup( Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS ).size() > static_cast<size_t>( basementOffset ) );
        assert( Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_TOWNS ).size() > static_cast<size_t>( townType ) );
        assert( Maps::getObjectsByGroup( Maps::ObjectGroup::LANDSCAPE_FLAGS ).size() > static_cast<size_t>( color ) * 2 + 1 );

        const auto & basementObject = Maps::getObjectsByGroup( Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS )[basementOffset];
        const auto & combinedTownObject = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_TOWNS )[townType];
        const auto & leftFlagObject = Maps::getObjectsByGroup( Maps::ObjectGroup::LANDSCAPE_FLAGS )[static_cast<size_t>( color ) * 2];
        const auto & rightFlagObject = Maps::getObjectsByGroup( Maps::ObjectGroup::LANDSCAPE_FLAGS )[static_cast<size_t>( color ) * 2 + 1];

        Maps::ObjectInfo townObject{ combinedTownObject.objectType };
        Maps::ObjectInfo shadowObject{ MP2::OBJ_NONE };
        for ( const auto & partInfo : combinedTownObject.groundLevelParts ) {
            if ( partInfo.layerType == Maps::SHADOW_LAYER ) {
                shadowObject.groundLevelParts.push_back( partInfo );
            }
            else {
                townObject.groundLevelParts.push_back( partInfo );
            }
        }

        townObject.topLevelParts = combinedTownObject.topLevelParts;

        fheroes2::Point maxOffset;
        fheroes2::Point basementMinOffset;
        fheroes2::Point townMinOffset;
        fheroes2::Point shadowMinOffset;
        const fheroes2::Point leftFlagMinOffset{ 1, 0 };
        const fheroes2::Point rightFlagMinOffset{ -1, 0 };

        getMinMaxOffsets( basementObject, basementMinOffset, maxOffset );
        getMinMaxOffsets( townObject, townMinOffset, maxOffset );
        getMinMaxOffsets( shadowObject, shadowMinOffset, maxOffset );

        assert( leftFlagObject.groundLevelParts.size() == 1 && leftFlagObject.topLevelParts.empty() );
        assert( rightFlagObject.groundLevelParts.size() == 1 && rightFlagObject.topLevelParts.empty() );

        fheroes2::Point minOffset = basementMinOffset;
        minOffset.x = std::min( minOffset.x, townMinOffset.x );
        minOffset.x = std::min( minOffset.x, shadowMinOffset.x );
        minOffset.y = std::min( minOffset.y, townMinOffset.y );
        minOffset.y = std::min( minOffset.y, shadowMinOffset.y );

        const int32_t width{ ( maxOffset.x - minOffset.x + 1 ) * tileSize };
        const int32_t height{ ( maxOffset.y - minOffset.y + 1 ) * tileSize };
        const Point imageOffset{ ( minOffset.x * tileSize ) - tileSize / 2, ( minOffset.y * tileSize ) - tileSize / 2 };

        fheroes2::Sprite outputImage( width, height, imageOffset.x, imageOffset.y );
        outputImage.reset();

        renderObject( outputImage, shadowObject, minOffset );
        renderObject( outputImage, basementObject, minOffset );
        renderObject( outputImage, townObject, minOffset );
        renderObject( outputImage, leftFlagObject, minOffset + leftFlagMinOffset );
        renderObject( outputImage, rightFlagObject, minOffset + rightFlagMinOffset );

        return outputImage;
    }

    int32_t getTownBasementId( const int groundType )
    {
        // NOTICE: 'basementOffset' should be consistent with basement objects position in LANDSCAPE_TOWN_BASEMENTS vector.
        switch ( groundType ) {
        case Maps::Ground::WATER:
            // Logically Water is not allowed but let's do this.
            assert( 0 );
            return 0;
        case Maps::Ground::GRASS:
            return 0;
        case Maps::Ground::SNOW:
            return 1;
        case Maps::Ground::SWAMP:
            return 2;
        case Maps::Ground::LAVA:
            return 3;
        case Maps::Ground::DESERT:
            return 4;
        case Maps::Ground::DIRT:
            return 5;
        case Maps::Ground::WASTELAND:
            return 6;
        case Maps::Ground::BEACH:
            return 7;
        default:
            // Have you added a new ground? Add the logic above!
            assert( 0 );
            break;
        }

        return 0;
    }
}
