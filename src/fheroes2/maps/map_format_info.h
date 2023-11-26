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

#include <string>
#include <vector>

#include "map_object_info.h"

class StreamBase;

namespace Maps::Map_Format
{
    struct ObjectInfo
    {
        uint32_t id{ 0 };

        ObjectGroup group{ ObjectGroup::Landscape_Mountains };

        uint32_t index{ 0 };
    };

    struct TileInfo
    {
        uint16_t terrainIndex{ 0 };
        uint8_t terrainFlag{ 0 };

        std::vector<ObjectInfo> objects;
    };

    struct MapFormat
    {
        // TODO: change it only once the Editor is released to public and there is a need to expand map format functionality.
        uint16_t version{ 1 };

        int32_t size{ 0 };

        std::string name;
        std::string description;

        std::vector<TileInfo> tiles;
    };

    StreamBase & operator<<( StreamBase & msg, const ObjectInfo & object );
    StreamBase & operator>>( StreamBase & msg, ObjectInfo & object );

    StreamBase & operator<<( StreamBase & msg, const TileInfo & tile );
    StreamBase & operator>>( StreamBase & msg, TileInfo & tile );

    StreamBase & operator<<( StreamBase & msg, const MapFormat & map );
    StreamBase & operator>>( StreamBase & msg, MapFormat & map );
}
