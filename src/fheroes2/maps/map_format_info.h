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

#include <array>
#include <cstdint>
#include <map>
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

    // This structure should be used for any object that require simple data to be saved into map.
    struct StandardObjectMetadata
    {
        std::array<uint32_t, 3> metadata{ 0 };
    };

    struct CastleMetadata
    {
        // Color, type and whether it is castle or town must come from ObjectInfo to make sure
        // that these values correspond to graphical representation of the castle.

        // If the name is empty a random name is going to be set by the engine.
        std::string customName;

        // Defending monsters that are set in the castle.
        std::array<int32_t, 5> defenderMonsterType{ 0 };
        std::array<int32_t, 5> defenderMonsterCount{ 0 };
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

        // These are matadata maps in relation to object UID.
        std::map<uint32_t, StandardObjectMetadata> standardMetadata;

        std::map<uint32_t, CastleMetadata> castleMetadata;
    };

    bool loadBaseMap( const std::string & path, BaseMapFormat & map );
    bool loadMap( const std::string & path, MapFormat & map );

    bool saveMap( const std::string & path, const MapFormat & map );

    StreamBase & operator<<( StreamBase & msg, const ObjectInfo & object );
    StreamBase & operator>>( StreamBase & msg, ObjectInfo & object );
    StreamBase & operator<<( StreamBase & msg, const TileInfo & tile );
    StreamBase & operator>>( StreamBase & msg, TileInfo & tile );
    StreamBase & operator<<( StreamBase & msg, const StandardObjectMetadata & metadata );
    StreamBase & operator>>( StreamBase & msg, StandardObjectMetadata & metadata );
    StreamBase & operator<<( StreamBase & msg, const CastleMetadata & metadata );
    StreamBase & operator>>( StreamBase & msg, CastleMetadata & metadata );
    StreamBase & operator<<( StreamBase & msg, const BaseMapFormat & map );
    StreamBase & operator>>( StreamBase & msg, BaseMapFormat & map );
    StreamBase & operator<<( StreamBase & msg, const MapFormat & map );
    StreamBase & operator>>( StreamBase & msg, MapFormat & map );
}
