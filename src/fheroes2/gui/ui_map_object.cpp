/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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
#include <set>
#endif

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

#include "agg_image.h"
#include "map_object_info.h"
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
        std::set<Point> uniqueOffsets;
        for ( const auto & objectPart : object.groundLevelParts ) {
            const auto [dummy, inserted] = uniqueOffsets.emplace( objectPart.tileOffset );
            if ( !inserted ) {
                // The object hasn't formed properly!
                assert( 0 );
            }
        }

        for ( const auto & objectPart : object.topLevelParts ) {
            const auto [dummy, inserted] = uniqueOffsets.emplace( objectPart.tileOffset );
            if ( !inserted ) {
                // The object hasn't formed properly!
                assert( 0 );
            }
        }
#endif

        // Calculate the required size of the object in tiles.
        Point minOffset;
        Point maxOffset;
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

        // We can use an approximate size of the object based on tiles it needs
        // but make sure that the offset corresponds to { 0, 0 } tile offset's center.
        // In other words, the main tile should be the center of the image.
        const int32_t width{ ( maxOffset.x - minOffset.x + 1 ) * tileSize };
        const int32_t height{ ( maxOffset.y - minOffset.y + 1 ) * tileSize };
        const Point imageOffset{ ( minOffset.x * tileSize ) - tileSize / 2, ( minOffset.y * tileSize ) - tileSize / 2 };

        Sprite image{ width, height, imageOffset.x, imageOffset.y };
        // Since we don't generate a pixel precise image make it transparent at first.
        image.reset();

        for ( const auto & objectPart : object.groundLevelParts ) {
            renderObjectPart( image, objectPart, minOffset );
        }

        for ( const auto & objectPart : object.topLevelParts ) {
            renderObjectPart( image, objectPart, minOffset );
        }

        return image;
    }
}
