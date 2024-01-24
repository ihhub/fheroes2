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

#include "map_format_info.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>

#include "serialize.h"

namespace
{
    const std::array<uint8_t, 6> magicWord{ 'h', '2', 'm', 'a', 'p', '\0' };

    // This value is set to avoid any corrupted files to be processed.
    const size_t minFileSize{ 128 };
}

namespace Maps::Map_Format
{
    StreamBase & operator<<( StreamBase & msg, const ObjectInfo & object )
    {
        using GroupUnderlyingType = std::underlying_type_t<decltype( object.group )>;

        return msg << object.id << static_cast<GroupUnderlyingType>( object.group ) << object.index;
    }

    StreamBase & operator>>( StreamBase & msg, ObjectInfo & object )
    {
        msg >> object.id;

        using GroupUnderlyingType = std::underlying_type_t<decltype( object.group )>;
        GroupUnderlyingType group;
        msg >> group;

        object.group = static_cast<ObjectGroup>( group );

        return msg >> object.index;
    }

    StreamBase & operator<<( StreamBase & msg, const TileInfo & tile )
    {
        return msg << tile.terrainIndex << tile.terrainFlag << tile.objects;
    }

    StreamBase & operator>>( StreamBase & msg, TileInfo & tile )
    {
        return msg >> tile.terrainIndex >> tile.terrainFlag >> tile.objects;
    }

    StreamBase & operator<<( StreamBase & msg, const StandardObjectMetadata & metadata )
    {
        return msg << metadata.metadata;
    }

    StreamBase & operator>>( StreamBase & msg, StandardObjectMetadata & metadata )
    {
        return msg >> metadata.metadata;
    }

    StreamBase & operator<<( StreamBase & msg, const CastleMetadata & metadata )
    {
        return msg << metadata.customName << metadata.defenderMonsterType << metadata.defenderMonsterCount << metadata.isCaptainAvailable << metadata.builtBuildings
                   << metadata.bannedBuildings << metadata.mustHaveSpells << metadata.bannedSpells << metadata.availableToHireMonsterCount;
    }

    StreamBase & operator>>( StreamBase & msg, CastleMetadata & metadata )
    {
        return msg >> metadata.customName >> metadata.defenderMonsterType >> metadata.defenderMonsterCount >> metadata.isCaptainAvailable >> metadata.builtBuildings
               >> metadata.bannedBuildings >> metadata.mustHaveSpells >> metadata.bannedSpells >> metadata.availableToHireMonsterCount;
    }

    StreamBase & operator<<( StreamBase & msg, const HeroMetadata & metadata )
    {
        return msg << metadata.customName << metadata.customPortrait << metadata.armyMonsterType << metadata.armyMonsterCount << metadata.artifact
                   << metadata.artifactMetadata << metadata.availableSpells << metadata.isOnPatrol << metadata.patrolRadius << metadata.secondarySkill
                   << metadata.secondarySkillLevel << metadata.customLevel << metadata.customExperience << metadata.customAttack << metadata.customDefence
                   << metadata.customKnowledge << metadata.customSpellPower << metadata.magicPoints;
    }

    StreamBase & operator>>( StreamBase & msg, HeroMetadata & metadata )
    {
        return msg >> metadata.customName >> metadata.customPortrait >> metadata.armyMonsterType >> metadata.armyMonsterCount >> metadata.artifact
               >> metadata.artifactMetadata >> metadata.availableSpells >> metadata.isOnPatrol >> metadata.patrolRadius >> metadata.secondarySkill
               >> metadata.secondarySkillLevel >> metadata.customLevel >> metadata.customExperience >> metadata.customAttack >> metadata.customDefence
               >> metadata.customKnowledge >> metadata.customSpellPower >> metadata.magicPoints;
    }

    StreamBase & operator<<( StreamBase & msg, const BaseMapFormat & map )
    {
        return msg << map.version << map.isCampaign << map.difficulty << map.availablePlayerColors << map.humanPlayerColors << map.computerPlayerColors << map.alliances
                   << map.victoryConditionType << map.isVictoryConditionApplicableForAI << map.allowNormalVictory << map.victoryConditionMetadata << map.lossCondition
                   << map.lossConditionMetadata << map.size << map.name << map.description;
    }

    StreamBase & operator>>( StreamBase & msg, BaseMapFormat & map )
    {
        return msg >> map.version >> map.isCampaign >> map.difficulty >> map.availablePlayerColors >> map.humanPlayerColors >> map.computerPlayerColors >> map.alliances
               >> map.victoryConditionType >> map.isVictoryConditionApplicableForAI >> map.allowNormalVictory >> map.victoryConditionMetadata >> map.lossCondition
               >> map.lossConditionMetadata >> map.size >> map.name >> map.description;
    }

    StreamBase & operator<<( StreamBase & msg, const MapFormat & map )
    {
        return msg << static_cast<const BaseMapFormat &>( map ) << map.additionalInfo << map.tiles << map.standardMetadata << map.castleMetadata << map.heroMetadata;
    }

    StreamBase & operator>>( StreamBase & msg, MapFormat & map )
    {
        // TODO: verify the correctness of metadata.
        return msg >> static_cast<BaseMapFormat &>( map ) >> map.additionalInfo >> map.tiles >> map.standardMetadata >> map.castleMetadata >> map.heroMetadata;
    }

    bool loadBaseMap( const std::string & path, BaseMapFormat & map )
    {
        if ( path.empty() ) {
            return false;
        }

        StreamFile fileStream;
        fileStream.setbigendian( true );

        if ( !fileStream.open( path, "rb" ) ) {
            return false;
        }

        const size_t fileSize = fileStream.size();
        if ( fileSize < minFileSize ) {
            return false;
        }

        for ( const uint8_t value : magicWord ) {
            if ( fileStream.get() != value ) {
                return false;
            }
        }

        fileStream >> map;

        return true;
    }

    bool loadMap( const std::string & path, MapFormat & map )
    {
        if ( path.empty() ) {
            return false;
        }

        StreamFile fileStream;
        fileStream.setbigendian( true );

        if ( !fileStream.open( path, "rb" ) ) {
            return false;
        }

        const size_t fileSize = fileStream.size();
        if ( fileSize < minFileSize ) {
            return false;
        }

        for ( const uint8_t value : magicWord ) {
            if ( fileStream.get() != value ) {
                return false;
            }
        }

        fileStream >> map;

        return true;
    }

    bool saveMap( const std::string & path, const MapFormat & map )
    {
        if ( path.empty() ) {
            return false;
        }

        StreamFile fileStream;
        fileStream.setbigendian( true );

        if ( !fileStream.open( path, "wb" ) ) {
            return false;
        }

        for ( const uint8_t value : magicWord ) {
            fileStream.put( value );
        }

        fileStream << map;

        return true;
    }
}
