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
    // It is impossible to have a map with smaller than this size.
    const size_t minFileSize{ 512 };

    constexpr uint16_t minimumSupportedVersion{ 2 };

    // Change the version when there is a need to expand map format functionality.
    constexpr uint16_t currentSupportedVersion{ 4 };

    void convertFromV2ToV3( Maps::Map_Format::MapFormat & map )
    {
        static_assert( minimumSupportedVersion <= 2, "Remove this function." );

        if ( map.version > 2 ) {
            return;
        }

        for ( Maps::Map_Format::TileInfo & tileInfo : map.tiles ) {
            for ( Maps::Map_Format::TileObjectInfo & objInfo : tileInfo.objects ) {
                if ( objInfo.group == Maps::ObjectGroup::ADVENTURE_DWELLINGS ) {
                    switch ( objInfo.index ) {
                    case 17: // Graveyard, grass terrain, ugly version
                        objInfo.group = Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS;
                        objInfo.index = 62;
                        break;
                    case 18: // Graveyard, snow terrain, ugly version
                        objInfo.group = Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS;
                        objInfo.index = 63;
                        break;
                    case 19: // Graveyard, desert terrain(?), ugly version
                        objInfo.group = Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS;
                        objInfo.index = 64;
                        break;
                    case 20: // Graveyard, generic terrain
                        objInfo.group = Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS;
                        objInfo.index = 0;
                        break;
                    case 21: // Graveyard, snow terrain
                        objInfo.group = Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS;
                        objInfo.index = 1;
                        break;
                    default: // Shift the rest of the objects in the Dwellings group by 5 positions "up"
                        if ( objInfo.index > 21 ) {
                            objInfo.index -= 5;
                        }
                        break;
                    }

                    continue;
                }

                if ( objInfo.group == Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS ) {
                    // Shift the objects in the Miscellaneous group by 2 positions "down", since non-ugly Graveyard versions were added to the beginning of this group
                    objInfo.index += 2;
                }
            }
        }
    }

    void convertFromV3ToV4( Maps::Map_Format::MapFormat & map )
    {
        static_assert( minimumSupportedVersion <= 3, "Remove this function." );

        if ( map.version > 3 ) {
            return;
        }

        for ( Maps::Map_Format::TileInfo & tileInfo : map.tiles ) {
            for ( Maps::Map_Format::TileObjectInfo & objInfo : tileInfo.objects ) {
                if ( objInfo.group == Maps::ObjectGroup::ADVENTURE_DWELLINGS && objInfo.index >= 18 ) {
                    // Shift the objects in the Dwellings group by 1 position "down" to add a new Cave object.
                    objInfo.index += 1;
                }
            }
        }
    }
}

namespace Maps::Map_Format
{
    StreamBase & operator<<( StreamBase & msg, const TileObjectInfo & object )
    {
        using GroupUnderlyingType = std::underlying_type_t<decltype( object.group )>;

        return msg << object.id << static_cast<GroupUnderlyingType>( object.group ) << object.index;
    }

    StreamBase & operator>>( StreamBase & msg, TileObjectInfo & object )
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

    StreamBase & operator<<( StreamBase & msg, const DailyEvent & eventInfo )
    {
        return msg << eventInfo.message << eventInfo.humanPlayerColors << eventInfo.computerPlayerColors << eventInfo.firstOccurrenceDay << eventInfo.repeatPeriodInDays
                   << eventInfo.resources;
    }

    StreamBase & operator>>( StreamBase & msg, DailyEvent & eventInfo )
    {
        return msg >> eventInfo.message >> eventInfo.humanPlayerColors >> eventInfo.computerPlayerColors >> eventInfo.firstOccurrenceDay >> eventInfo.repeatPeriodInDays
               >> eventInfo.resources;
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
        return msg << metadata.customName << metadata.defenderMonsterType << metadata.defenderMonsterCount << metadata.customBuildings << metadata.builtBuildings
                   << metadata.bannedBuildings << metadata.mustHaveSpells << metadata.bannedSpells << metadata.availableToHireMonsterCount;
    }

    StreamBase & operator>>( StreamBase & msg, CastleMetadata & metadata )
    {
        return msg >> metadata.customName >> metadata.defenderMonsterType >> metadata.defenderMonsterCount >> metadata.customBuildings >> metadata.builtBuildings
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
        return msg << metadata.riddle << metadata.answers << metadata.artifact << metadata.artifactMetadata << metadata.resources;
    }

    StreamBase & operator>>( StreamBase & msg, SphinxMetadata & metadata )
    {
        return msg >> metadata.riddle >> metadata.answers >> metadata.artifact >> metadata.artifactMetadata >> metadata.resources;
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
                   << metadata.artifactMetadata << metadata.resources << metadata.attack << metadata.defense << metadata.knowledge << metadata.spellPower
                   << metadata.experience << metadata.secondarySkill << metadata.secondarySkillLevel << metadata.monsterType << metadata.monsterCount;
    }

    StreamBase & operator>>( StreamBase & msg, AdventureMapEventMetadata & metadata )
    {
        return msg >> metadata.message >> metadata.humanPlayerColors >> metadata.computerPlayerColors >> metadata.isRecurringEvent >> metadata.artifact
               >> metadata.artifactMetadata >> metadata.resources >> metadata.attack >> metadata.defense >> metadata.knowledge >> metadata.spellPower
               >> metadata.experience >> metadata.secondarySkill >> metadata.secondarySkillLevel >> metadata.monsterType >> metadata.monsterCount;
    }

    StreamBase & operator<<( StreamBase & msg, const ShrineMetadata & metadata )
    {
        return msg << metadata.allowedSpells;
    }

    StreamBase & operator>>( StreamBase & msg, ShrineMetadata & metadata )
    {
        return msg >> metadata.allowedSpells;
    }

    bool saveToStream( StreamBase & msg, const BaseMapFormat & map )
    {
        using LanguageUnderlyingType = std::underlying_type_t<decltype( map.language )>;

        msg << currentSupportedVersion << map.isCampaign << map.difficulty << map.availablePlayerColors << map.humanPlayerColors << map.computerPlayerColors
            << map.alliances << map.playerRace << map.victoryConditionType << map.isVictoryConditionApplicableForAI << map.allowNormalVictory
            << map.victoryConditionMetadata << map.lossConditionType << map.lossConditionMetadata << map.size << static_cast<LanguageUnderlyingType>( map.language )
            << map.name << map.description;

        return !msg.fail();
    }

    bool loadFromStream( StreamBase & msg, BaseMapFormat & map )
    {
        msg >> map.version;
        if ( map.version < minimumSupportedVersion || map.version > currentSupportedVersion ) {
            return false;
        }

        msg >> map.isCampaign >> map.difficulty >> map.availablePlayerColors >> map.humanPlayerColors >> map.computerPlayerColors >> map.alliances >> map.playerRace
            >> map.victoryConditionType >> map.isVictoryConditionApplicableForAI >> map.allowNormalVictory >> map.victoryConditionMetadata >> map.lossConditionType
            >> map.lossConditionMetadata >> map.size;

        if ( map.size <= 0 ) {
            // This is not a correct map size.
            return false;
        }

        using LanguageUnderlyingType = std::underlying_type_t<decltype( map.language )>;
        static_assert( std::is_same_v<LanguageUnderlyingType, uint8_t>, "Type of language has been changed, check the logic below" );
        LanguageUnderlyingType language;

        msg >> language;
        map.language = static_cast<fheroes2::SupportedLanguage>( language );

        msg >> map.name >> map.description;

        return !msg.fail();
    }

    bool saveToStream( StreamBase & msg, const MapFormat & map )
    {
        // Only the base map information is not encoded.
        // The rest of data must be compressed to prevent manual corruption of the file.
        if ( !saveToStream( msg, static_cast<const BaseMapFormat &>( map ) ) ) {
            return false;
        }

        StreamBuf compressed;
        compressed.setbigendian( true );

        compressed << map.additionalInfo << map.tiles << map.dailyEvents << map.rumors << map.standardMetadata << map.castleMetadata << map.heroMetadata
                   << map.sphinxMetadata << map.signMetadata << map.adventureMapEventMetadata << map.shrineMetadata;

        const std::vector<uint8_t> temp = Compression::compressData( compressed.data(), compressed.size() );

        msg.putRaw( temp.data(), temp.size() );

        return !msg.fail();
    }

    bool loadFromStream( StreamBase & msg, MapFormat & map )
    {
        // TODO: verify the correctness of metadata.
        if ( !loadFromStream( msg, static_cast<BaseMapFormat &>( map ) ) ) {
            map = {};
            return false;
        }

        StreamBuf decompressed;
        decompressed.setbigendian( true );

        {
            std::vector<uint8_t> temp = msg.getRaw();
            if ( temp.empty() ) {
                // This is a corrupted file.
                map = {};
                return false;
            }

            const std::vector<uint8_t> decompressedData = Compression::decompressData( temp.data(), temp.size() );
            if ( decompressedData.empty() ) {
                // This is a corrupted file.
                map = {};
                return false;
            }

            // Let's try to free up some memory
            temp = std::vector<uint8_t>{};

            decompressed.putRaw( decompressedData.data(), decompressedData.size() );
        }

        decompressed >> map.additionalInfo >> map.tiles;

        if ( map.tiles.size() != static_cast<size_t>( map.size ) * map.size ) {
            map = {};
            return false;
        }

        decompressed >> map.dailyEvents >> map.rumors >> map.standardMetadata >> map.castleMetadata >> map.heroMetadata >> map.sphinxMetadata >> map.signMetadata
            >> map.adventureMapEventMetadata >> map.shrineMetadata;

        static_assert( minimumSupportedVersion <= 2, "Remove the following function call." );
        convertFromV2ToV3( map );
        convertFromV3ToV4( map );

        return !msg.fail();
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

        return loadFromStream( fileStream, map );
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

        return loadFromStream( fileStream, map );
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

        return saveToStream( fileStream, map );
    }
}
