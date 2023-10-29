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

#pragma once

#include <cstdint>
#include <vector>

#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"

namespace Maps
{
    // An object usually contains of multiple parts / tiles. Each part has its own features like object layer type or image index.
    // An object always contains a main object part (addon).
    // All object's parts shares images from the same ICN source (MP2::ObjectIcnType).
    struct ObjectPartInfo
    {
        ObjectPartInfo( const MP2::ObjectIcnType icn, const uint32_t index, const fheroes2::Point offset, const MP2::MapObjectType type, const ObjectLayerType layer )
            : icnType( icn )
            , icnIndex( index )
            , tileOffset( offset )
            , objectType( type )
            , layerType( layer )
        {
            // Do nothing.
        }

        // ICN type associated to this object part. Some objects like towns can have parts coming from different ICN resources.
        MP2::ObjectIcnType icnType{ MP2::OBJ_ICN_TYPE_UNKNOWN };

        // Image index from an ICN resource to render this object part.
        uint32_t icnIndex{ 0 };

        // A tile offset from the main object tile.
        fheroes2::Point tileOffset;

        // Object type associated with this object part. Not every object part has a type. For example, shadows don't have types.
        MP2::MapObjectType objectType{ MP2::OBJ_NONE };

        // A layer where this object part / addon sits on.
        // The layer is used for passability calculations as well as an order of rendering objects.
        ObjectLayerType layerType{ OBJECT_LAYER };

        // TODO: add information about animation.
    };

    struct ObjectInfo
    {
        explicit ObjectInfo( const MP2::MapObjectType type )
            : objectType( type )
        {
            // Do nothing.
        }

        // A collection of object parts.
        // IMPORTANT!!!
        // - must not be empty
        // - the main object part must come first
        // IMPORTANT!!!
        std::vector<ObjectPartInfo> groundLevelParts;

        // Top level parts. Can be empty and should never contain objects with Y value more or equal 0.
        // It does not matter what layer is set here as it is going to be ignored. For consistency put OBJECT_LAYER to detect any bad logic.
        std::vector<ObjectPartInfo> topLevelParts;

        // Object type. Some objects don't have it like cracks.
        MP2::MapObjectType objectType{ MP2::OBJ_NONE };
    };

    // Some objects use the same ICN resources and belong to the same object type
    // but the majority of them have different object types while using the same ICN
    // or use multiple ICN resource among the same object type.
    // Plus the fheroes2 Editor requires object type and index in order to save it,
    // therefore, such enumeration exist.
    enum class ObjectGroup : int32_t
    {
        Artifact,
        Resource,
        Water_Object,

        // IMPORTANT!!!
        // Put all new entries just above this entry.
        Group_Count
    };

    const std::vector<ObjectInfo> & getObjectsByGroup( const ObjectGroup group );
}
