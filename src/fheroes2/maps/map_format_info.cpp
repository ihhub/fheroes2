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

#include <array>
#include <cstddef>
#include <type_traits>

#include "serialize.h"
#include "zzlib.h"

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
                   << metadata.secondarySkillLevel << metadata.customLevel << metadata.customExperience << metadata.customAttack << metadata.customDefense
                   << metadata.customKnowledge << metadata.customSpellPower << metadata.magicPoints << metadata.race;
    }

    StreamBase & operator>>( StreamBase & msg, HeroMetadata & metadata )
    {
        return msg >> metadata.customName >> metadata.customPortrait >> metadata.armyMonsterType >> metadata.armyMonsterCount >> metadata.artifact
               >> metadata.artifactMetadata >> metadata.availableSpells >> metadata.isOnPatrol >> metadata.patrolRadius >> metadata.secondarySkill
               >> metadata.secondarySkillLevel >> metadata.customLevel >> metadata.customExperience >> metadata.customAttack >> metadata.customDefense
               >> metadata.customKnowledge >> metadata.customSpellPower >> metadata.magicPoints >> metadata.race;
    }

    StreamBase & operator<<( StreamBase & msg, const SphinxMetadata & metadata )
    {
        return msg << metadata.question << metadata.answers << metadata.artifact << metadata.resources;
    }

    StreamBase & operator>>( StreamBase & msg, SphinxMetadata & metadata )
    {
        return msg >> metadata.question >> metadata.answers >> metadata.artifact >> metadata.resources;
    }

    StreamBase & operator<<( StreamBase & msg, const SignMetadata & metadata )
    {
        return msg << metadata.message;
    }

    StreamBase & operator>>( StreamBase & msg, SignMetadata & metadata )
    {
        return msg >> metadata.message;
    }

    StreamBase & operator<<( StreamBase & msg, const AdventureMapEventMetadata & metadata )
    {
        return msg << metadata.message << metadata.humanPlayerColors << metadata.computerPlayerColors << metadata.isRecurringEvent << metadata.artifact
                   << metadata.resources;
    }

    StreamBase & operator>>( StreamBase & msg, AdventureMapEventMetadata & metadata )
    {
        return msg >> metadata.message >> metadata.humanPlayerColors >> metadata.computerPlayerColors >> metadata.isRecurringEvent >> metadata.artifact
               >> metadata.resources;
    }

    StreamBase & operator<<( StreamBase & msg, const BaseMapFormat & map )
    {
        return msg << map.version << map.isCampaign << map.difficulty << map.availablePlayerColors << map.humanPlayerColors << map.computerPlayerColors << map.alliances
                   << map.playerRace << map.victoryConditionType << map.isVictoryConditionApplicableForAI << map.allowNormalVictory << map.victoryConditionMetadata
                   << map.lossCondition << map.lossConditionMetadata << map.size << map.name << map.description;
    }

    StreamBase & operator>>( StreamBase & msg, BaseMapFormat & map )
    {
        return msg >> map.version >> map.isCampaign >> map.difficulty >> map.availablePlayerColors >> map.humanPlayerColors >> map.computerPlayerColors >> map.alliances
               >> map.playerRace >> map.victoryConditionType >> map.isVictoryConditionApplicableForAI >> map.allowNormalVictory >> map.victoryConditionMetadata
               >> map.lossCondition >> map.lossConditionMetadata >> map.size >> map.name >> map.description;
    }

    StreamBase & operator<<( StreamBase & msg, const MapFormat & map )
    {
        // Only the base map information is not encoded.
        // The rest of data must be compressed to prevent manual corruption of the file.
        msg << static_cast<const BaseMapFormat &>( map );

        StreamBuf compressed;
        compressed.setbigendian( true );

        compressed << map.additionalInfo << map.tiles << map.standardMetadata << map.castleMetadata << map.heroMetadata << map.sphinxMetadata << map.signMetadata
                   << map.adventureMapEventMetadata;

        const std::vector<uint8_t> temp = Compression::compressData( compressed.data(), compressed.size() );

        msg.putRaw( temp.data(), temp.size() );

        return msg;
    }

    StreamBase & operator>>( StreamBase & msg, MapFormat & map )
    {
        // TODO: verify the correctness of metadata.
        msg >> static_cast<BaseMapFormat &>( map );

        StreamBuf decompressed;
        decompressed.setbigendian( true );

        {
            std::vector<uint8_t> temp = msg.getRaw();
            if ( temp.empty() ) {
                // This is a corrupted file.
                map = {};
                return msg;
            }

            const std::vector<uint8_t> decompressedData = Compression::decompressData( temp.data(), temp.size() );
            if ( decompressedData.empty() ) {
                // This is a corrupted file.
                map = {};
                return msg;
            }

            // Let's try to free up some memory
            temp = std::vector<uint8_t>{};

            decompressed.putRaw( decompressedData.data(), decompressedData.size() );
        }

        decompressed >> map.additionalInfo >> map.tiles >> map.standardMetadata >> map.castleMetadata >> map.heroMetadata >> map.sphinxMetadata >> map.signMetadata
            >> map.adventureMapEventMetadata;

        return msg;
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
