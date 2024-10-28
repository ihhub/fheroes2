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

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"

namespace Maps
{
    // An object usually contains of multiple parts / tiles. Each part has its own features like object layer type or image index.
    // An object always contains a main object part.
    // All object's parts shares images from the same ICN source (MP2::ObjectIcnType).
    struct ObjectPartInfo
    {
        ObjectPartInfo( const MP2::ObjectIcnType icn, const uint32_t index, const fheroes2::Point & offset, const MP2::MapObjectType type )
            : tileOffset( offset )
            , icnIndex( index )
            , objectType( type )
            , icnType( icn )
        {
            // Do nothing.
        }

        // A tile offset from the main object tile.
        fheroes2::Point tileOffset;

        // Image index from an ICN resource to render this object part.
        uint32_t icnIndex{ 0 };

        // Object type associated with this object part. Not every object part has a type. For example, shadows don't have types.
        MP2::MapObjectType objectType{ MP2::OBJ_NONE };

        // ICN type associated to this object part. Some objects like towns can have parts coming from different ICN resources.
        MP2::ObjectIcnType icnType{ MP2::OBJ_ICN_TYPE_UNKNOWN };

        // The number of following by index images is used to animate this object part.
        // In most cases this value is 0 as the majority of object parts do not have animations.
        uint8_t animationFrames{ 0 };
    };

    struct LayeredObjectPartInfo : public ObjectPartInfo
    {
        LayeredObjectPartInfo( const MP2::ObjectIcnType icn, const uint32_t index, const fheroes2::Point & offset, const MP2::MapObjectType type,
                               const ObjectLayerType layer )
            : ObjectPartInfo( icn, index, offset, type )
            , layerType( layer )
        {
            // Do nothing.
        }

        // A layer where this object part sits on.
        // The layer is used for passability calculations as well as an order of rendering objects.
        ObjectLayerType layerType{ OBJECT_LAYER };
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
        std::vector<LayeredObjectPartInfo> groundLevelParts;

        // Top level parts. Can be empty and should never contain objects with Y value more or equal 0.
        // It does not matter what layer is set here as it is going to be ignored. For consistency put OBJECT_LAYER to detect any bad logic.
        std::vector<ObjectPartInfo> topLevelParts;

        // Some action objects require additional information to be stored.
        // Information stored in this member should be interpret based on a group.
        std::array<uint32_t, 2> metadata{ 0 };

        // Object type. Some objects don't have it like cracks.
        MP2::MapObjectType objectType{ MP2::OBJ_NONE };

        bool empty() const
        {
            return groundLevelParts.empty();
        }
    };

    // Some objects use the same ICN resources and belong to the same object type
    // but the majority of them have different object types while using the same ICN
    // or use multiple ICN resource among the same object type.
    // Plus the fheroes2 Editor requires object type and index in order to save it,
    // therefore, such enumeration exist.
    //
    // These groups do not correlate with the original Editor.
    //
    // !!! IMPORTANT !!!
    // Do NOT change the order of the items as they are used for the map format.
    enum class ObjectGroup : uint8_t
    {
        // These groups are not being used by the Editor directly but they are still a part of a tile.
        NONE,

        ROADS,
        STREAMS,

        // Landscape objects.
        LANDSCAPE_MOUNTAINS,
        LANDSCAPE_ROCKS,
        LANDSCAPE_TREES,
        LANDSCAPE_WATER,
        LANDSCAPE_MISCELLANEOUS,

        // Extra landscape objects placed together with other objects.
        LANDSCAPE_TOWN_BASEMENTS,
        LANDSCAPE_FLAGS,

        // Adventure objects.
        ADVENTURE_ARTIFACTS,
        ADVENTURE_DWELLINGS,
        ADVENTURE_MINES,
        ADVENTURE_POWER_UPS,
        ADVENTURE_TREASURES,
        ADVENTURE_WATER,
        ADVENTURE_MISCELLANEOUS,

        // Kingdom objects.
        KINGDOM_HEROES,
        KINGDOM_TOWNS,

        // Monsters.
        MONSTERS,

        // Extra map objects that are not placed in editor (currently).
        MAP_EXTRAS,

        // IMPORTANT!!!
        // Put all new entries just above this entry.
        GROUP_COUNT
    };

    const std::vector<ObjectInfo> & getObjectsByGroup( const ObjectGroup group );

    const ObjectInfo & getObjectInfo( const ObjectGroup group, const int32_t objectIndex );

    // The function can return nullptr if an object does not exist.
    // A valid pointer could also point to LayeredObjectPartInfo object.
    const ObjectPartInfo * getObjectPartByIcn( const MP2::ObjectIcnType icnType, const uint32_t icnIndex );

    MP2::MapObjectType getObjectTypeByIcn( const MP2::ObjectIcnType icnType, const uint32_t icnIndex );

    // The function returns tile offsets only for ground level objects located on OBJECT_LAYER and BACKGROUND_LAYER layers.
    // Objects on other layers do not affect passabilities of tiles so they do not 'occupy' these tiles.
    std::vector<fheroes2::Point> getGroundLevelOccupiedTileOffset( const ObjectInfo & info );

    // The function returns tile offsets only for ground level objects located on OBJECT_LAYER, BACKGROUND_LAYER and TERRAIN_LAYER layers.
    // SHADOW_LAYER is excluded as it does not "use" any tile.
    std::vector<fheroes2::Point> getGroundLevelUsedTileOffset( const ObjectInfo & info );
}
