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

#pragma once

#include "math_base.h"
#include "mp2.h"

#include <vector>

namespace fheroes2
{
    // This structure represents a part of a map object.
    struct MapObjectPartInfo
    {
        MapObjectPartInfo( const uint32_t index_, const uint32_t animationFrameCount_, const fheroes2::Point & offset_, const MP2::MapObjectType type_,
                           const uint8_t level_ )
            : index( index_ )
            , animationFrameCount( animationFrameCount_ )
            , offset( offset_ )
            , type( type_ )
            , level( level_ )
        {}

        // ICN index.
        uint32_t index;

        // If the object has some animation then this number is not equal to 0.
        uint32_t animationFrameCount;

        // Offset in tiles from the main tile of the object.
        fheroes2::Point offset;

        MP2::MapObjectType type;

        uint8_t level;
    };

    struct MapObjectInfo
    {
        std::vector<MapObjectPartInfo> parts;
    };

    MP2::MapObjectType getObjectType( const int icnId, const uint32_t index );

    const std::vector<MapObjectInfo> & getObjects( const int icnId );
}
