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
#include <string>
#include <vector>

#include "map_object_info.h"

class StreamBase;

namespace Maps::Map_Format
{
    struct ObjectInfo
    {
        uint32_t id{ 0 };

        ObjectGroup group{ ObjectGroup::LANDSCAPE_MOUNTAINS };

        uint32_t index{ 0 };
    };

    struct TileInfo
    {
        uint16_t terrainIndex{ 0 };
        uint8_t terrainFlag{ 0 };

        std::vector<ObjectInfo> objects;
    };

    struct BaseMapFormat
    {
        // TODO: change it only once the Editor is released to public and there is a need to expand map format functionality.
        uint16_t version{ 1 };
        bool isCampaign{ false };

        uint8_t difficulty{ 0 };

        uint8_t availablePlayerColors{ 0 };
        uint8_t humanPlayerColors{ 0 };
        uint8_t computerPlayerColors{ 0 };
        std::vector<uint8_t> alliances;

        uint8_t victoryConditionType{ 0 };
        bool isVictoryConditionApplicableForAI{ false };
        bool allowNormalVictory{ false };
        std::vector<uint32_t> victoryConditionMetadata;

        uint8_t lossCondition{ 0 };
        std::vector<uint32_t> lossConditionMetadata;

        int32_t size{ 0 };

        std::string name;
        std::string description;
    };

    struct MapFormat : public BaseMapFormat
    {
        // This is used only for campaign maps.
        std::vector<uint32_t> additionalInfo;

        std::vector<TileInfo> tiles;
    };

    bool loadBaseMap( const std::string & path, BaseMapFormat & map );
    bool loadMap( const std::string & path, MapFormat & map );

    bool saveMap( const std::string & path, const MapFormat & map );

    StreamBase & operator<<( StreamBase & msg, const ObjectInfo & object );
    StreamBase & operator>>( StreamBase & msg, ObjectInfo & object );
    StreamBase & operator<<( StreamBase & msg, const TileInfo & tile );
    StreamBase & operator>>( StreamBase & msg, TileInfo & tile );
    StreamBase & operator<<( StreamBase & msg, const BaseMapFormat & map );
    StreamBase & operator>>( StreamBase & msg, BaseMapFormat & map );
    StreamBase & operator<<( StreamBase & msg, const MapFormat & map );
    StreamBase & operator>>( StreamBase & msg, MapFormat & map );
}
