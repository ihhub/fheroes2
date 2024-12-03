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

#include "serialize.h"
#include "zzlib.h"

namespace Maps::Map_Format
{
    // The following operators are used only inside this module, but they cannot be declared in an anonymous namespace due to the way ADL works

    OStreamBase & operator<<( OStreamBase & stream, const TileObjectInfo & object );
    IStreamBase & operator>>( IStreamBase & stream, TileObjectInfo & object );

    OStreamBase & operator<<( OStreamBase & stream, const TileInfo & tile );
    IStreamBase & operator>>( IStreamBase & stream, TileInfo & tile );

    OStreamBase & operator<<( OStreamBase & stream, const DailyEvent & eventInfo );
    IStreamBase & operator>>( IStreamBase & stream, DailyEvent & eventInfo );

    OStreamBase & operator<<( OStreamBase & stream, const StandardObjectMetadata & metadata );
    IStreamBase & operator>>( IStreamBase & stream, StandardObjectMetadata & metadata );

    OStreamBase & operator<<( OStreamBase & stream, const CastleMetadata & metadata );
    IStreamBase & operator>>( IStreamBase & stream, CastleMetadata & metadata );

    OStreamBase & operator<<( OStreamBase & stream, const HeroMetadata & metadata );
    IStreamBase & operator>>( IStreamBase & stream, HeroMetadata & metadata );

    OStreamBase & operator<<( OStreamBase & stream, const SphinxMetadata & metadata );
    IStreamBase & operator>>( IStreamBase & stream, SphinxMetadata & metadata );

    OStreamBase & operator<<( OStreamBase & stream, const SignMetadata & metadata );
    IStreamBase & operator>>( IStreamBase & stream, SignMetadata & metadata );

    OStreamBase & operator<<( OStreamBase & stream, const AdventureMapEventMetadata & metadata );
    IStreamBase & operator>>( IStreamBase & stream, AdventureMapEventMetadata & metadata );

    OStreamBase & operator<<( OStreamBase & stream, const ShrineMetadata & metadata );
    IStreamBase & operator>>( IStreamBase & stream, ShrineMetadata & metadata );
}

namespace
{
    const std::array<uint8_t, 6> magicWord{ 'h', '2', 'm', 'a', 'p', '\0' };

    // This value is set to avoid any corrupted files to be processed.
    // It is impossible to have a map with smaller than this size.
    const size_t minFileSize{ 512 };

    constexpr uint16_t minimumSupportedVersion{ 2 };

    // Change the version when there is a need to expand map format functionality.
    constexpr uint16_t currentSupportedVersion{ 6 };

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

    void convertFromV4ToV5( Maps::Map_Format::MapFormat & map )
    {
        static_assert( minimumSupportedVersion <= 4, "Remove this function." );

        if ( map.version > 4 ) {
            return;
        }

        for ( Maps::Map_Format::TileInfo & tileInfo : map.tiles ) {
            for ( Maps::Map_Format::TileObjectInfo & objInfo : tileInfo.objects ) {
                if ( objInfo.group == Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS && objInfo.index >= 128 ) {
                    // Shift the objects in the Landscape Miscellaneous group by 1 position "down" to add missed Small cliff, dirt terrain.
                    objInfo.index += 1;
                }
            }
        }
    }

    void convertFromV5ToV6( Maps::Map_Format::MapFormat & map )
    {
        static_assert( minimumSupportedVersion <= 5, "Remove this function." );

        if ( map.version > 5 ) {
            return;
        }

        for ( Maps::Map_Format::TileInfo & tileInfo : map.tiles ) {
            for ( Maps::Map_Format::TileObjectInfo & objInfo : tileInfo.objects ) {
                if ( objInfo.group == Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS && objInfo.index >= 17 ) {
                    // Shift the objects in the Adventure Miscellaneous group by 1 position "down" to add new Lean-To object.
                    objInfo.index += 1;
                }
            }
        }
    }

    bool saveToStream( OStreamBase & stream, const Maps::Map_Format::BaseMapFormat & map )
    {
        stream << currentSupportedVersion << map.isCampaign << map.difficulty << map.availablePlayerColors << map.humanPlayerColors << map.computerPlayerColors
               << map.alliances << map.playerRace << map.victoryConditionType << map.isVictoryConditionApplicableForAI << map.allowNormalVictory
               << map.victoryConditionMetadata << map.lossConditionType << map.lossConditionMetadata << map.size << map.mainLanguage << map.name << map.description;

        return !stream.fail();
    }

    bool loadFromStream( IStreamBase & stream, Maps::Map_Format::BaseMapFormat & map )
    {
        stream >> map.version;
        if ( map.version < minimumSupportedVersion || map.version > currentSupportedVersion ) {
            return false;
        }

        stream >> map.isCampaign >> map.difficulty >> map.availablePlayerColors >> map.humanPlayerColors >> map.computerPlayerColors >> map.alliances >> map.playerRace
            >> map.victoryConditionType >> map.isVictoryConditionApplicableForAI >> map.allowNormalVictory >> map.victoryConditionMetadata >> map.lossConditionType
            >> map.lossConditionMetadata >> map.size;

        if ( map.size <= 0 ) {
            // This is not a correct map size.
            return false;
        }

        stream >> map.mainLanguage >> map.name >> map.description;

        return !stream.fail();
    }

    bool saveToStream( OStreamBase & stream, const Maps::Map_Format::MapFormat & map )
    {
        // Only the base map information is not encoded.
        // The rest of data must be compressed to prevent manual corruption of the file.
        if ( !saveToStream( stream, static_cast<const Maps::Map_Format::BaseMapFormat &>( map ) ) ) {
            return false;
        }

        RWStreamBuf compressed;
        compressed.setBigendian( true );

        compressed << map.additionalInfo << map.tiles << map.dailyEvents << map.rumors << map.standardMetadata << map.castleMetadata << map.heroMetadata
                   << map.sphinxMetadata << map.signMetadata << map.adventureMapEventMetadata << map.shrineMetadata;

        const std::vector<uint8_t> temp = Compression::zipData( compressed.data(), compressed.size() );

        stream.putRaw( temp.data(), temp.size() );

        return !stream.fail();
    }

    bool loadFromStream( IStreamBase & stream, Maps::Map_Format::MapFormat & map )
    {
        // TODO: verify the correctness of metadata.
        if ( !loadFromStream( stream, static_cast<Maps::Map_Format::BaseMapFormat &>( map ) ) ) {
            map = {};
            return false;
        }

        RWStreamBuf decompressed;
        decompressed.setBigendian( true );

        {
            std::vector<uint8_t> temp = stream.getRaw( 0 );
            if ( temp.empty() ) {
                // This is a corrupted file.
                map = {};
                return false;
            }

            const std::vector<uint8_t> decompressedData = Compression::unzipData( temp.data(), temp.size() );
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

        convertFromV2ToV3( map );
        convertFromV3ToV4( map );
        convertFromV4ToV5( map );
        convertFromV5ToV6( map );

        return !stream.fail();
    }
}

namespace Maps::Map_Format
{
    OStreamBase & operator<<( OStreamBase & stream, const TileObjectInfo & object )
    {
        return stream << object.id << object.group << object.index;
    }

    IStreamBase & operator>>( IStreamBase & stream, TileObjectInfo & object )
    {
        return stream >> object.id >> object.group >> object.index;
    }

    OStreamBase & operator<<( OStreamBase & stream, const TileInfo & tile )
    {
        return stream << tile.terrainIndex << tile.terrainFlag << tile.objects;
    }

    IStreamBase & operator>>( IStreamBase & stream, TileInfo & tile )
    {
        return stream >> tile.terrainIndex >> tile.terrainFlag >> tile.objects;
    }

    OStreamBase & operator<<( OStreamBase & stream, const DailyEvent & eventInfo )
    {
        return stream << eventInfo.message << eventInfo.humanPlayerColors << eventInfo.computerPlayerColors << eventInfo.firstOccurrenceDay
                      << eventInfo.repeatPeriodInDays << eventInfo.resources;
    }

    IStreamBase & operator>>( IStreamBase & stream, DailyEvent & eventInfo )
    {
        return stream >> eventInfo.message >> eventInfo.humanPlayerColors >> eventInfo.computerPlayerColors >> eventInfo.firstOccurrenceDay
               >> eventInfo.repeatPeriodInDays >> eventInfo.resources;
    }

    OStreamBase & operator<<( OStreamBase & stream, const StandardObjectMetadata & metadata )
    {
        return stream << metadata.metadata;
    }

    IStreamBase & operator>>( IStreamBase & stream, StandardObjectMetadata & metadata )
    {
        return stream >> metadata.metadata;
    }

    OStreamBase & operator<<( OStreamBase & stream, const CastleMetadata & metadata )
    {
        return stream << metadata.customName << metadata.defenderMonsterType << metadata.defenderMonsterCount << metadata.customBuildings << metadata.builtBuildings
                      << metadata.bannedBuildings << metadata.mustHaveSpells << metadata.bannedSpells << metadata.availableToHireMonsterCount;
    }

    IStreamBase & operator>>( IStreamBase & stream, CastleMetadata & metadata )
    {
        return stream >> metadata.customName >> metadata.defenderMonsterType >> metadata.defenderMonsterCount >> metadata.customBuildings >> metadata.builtBuildings
               >> metadata.bannedBuildings >> metadata.mustHaveSpells >> metadata.bannedSpells >> metadata.availableToHireMonsterCount;
    }

    OStreamBase & operator<<( OStreamBase & stream, const HeroMetadata & metadata )
    {
        return stream << metadata.customName << metadata.customPortrait << metadata.armyMonsterType << metadata.armyMonsterCount << metadata.artifact
                      << metadata.artifactMetadata << metadata.availableSpells << metadata.isOnPatrol << metadata.patrolRadius << metadata.secondarySkill
                      << metadata.secondarySkillLevel << metadata.customLevel << metadata.customExperience << metadata.customAttack << metadata.customDefense
                      << metadata.customKnowledge << metadata.customSpellPower << metadata.magicPoints << metadata.race;
    }

    IStreamBase & operator>>( IStreamBase & stream, HeroMetadata & metadata )
    {
        return stream >> metadata.customName >> metadata.customPortrait >> metadata.armyMonsterType >> metadata.armyMonsterCount >> metadata.artifact
               >> metadata.artifactMetadata >> metadata.availableSpells >> metadata.isOnPatrol >> metadata.patrolRadius >> metadata.secondarySkill
               >> metadata.secondarySkillLevel >> metadata.customLevel >> metadata.customExperience >> metadata.customAttack >> metadata.customDefense
               >> metadata.customKnowledge >> metadata.customSpellPower >> metadata.magicPoints >> metadata.race;
    }

    OStreamBase & operator<<( OStreamBase & stream, const SphinxMetadata & metadata )
    {
        return stream << metadata.riddle << metadata.answers << metadata.artifact << metadata.artifactMetadata << metadata.resources;
    }

    IStreamBase & operator>>( IStreamBase & stream, SphinxMetadata & metadata )
    {
        return stream >> metadata.riddle >> metadata.answers >> metadata.artifact >> metadata.artifactMetadata >> metadata.resources;
    }

    OStreamBase & operator<<( OStreamBase & stream, const SignMetadata & metadata )
    {
        return stream << metadata.message;
    }

    IStreamBase & operator>>( IStreamBase & stream, SignMetadata & metadata )
    {
        return stream >> metadata.message;
    }

    OStreamBase & operator<<( OStreamBase & stream, const AdventureMapEventMetadata & metadata )
    {
        return stream << metadata.message << metadata.humanPlayerColors << metadata.computerPlayerColors << metadata.isRecurringEvent << metadata.artifact
                      << metadata.artifactMetadata << metadata.resources << metadata.attack << metadata.defense << metadata.knowledge << metadata.spellPower
                      << metadata.experience << metadata.secondarySkill << metadata.secondarySkillLevel << metadata.monsterType << metadata.monsterCount;
    }

    IStreamBase & operator>>( IStreamBase & stream, AdventureMapEventMetadata & metadata )
    {
        return stream >> metadata.message >> metadata.humanPlayerColors >> metadata.computerPlayerColors >> metadata.isRecurringEvent >> metadata.artifact
               >> metadata.artifactMetadata >> metadata.resources >> metadata.attack >> metadata.defense >> metadata.knowledge >> metadata.spellPower
               >> metadata.experience >> metadata.secondarySkill >> metadata.secondarySkillLevel >> metadata.monsterType >> metadata.monsterCount;
    }

    OStreamBase & operator<<( OStreamBase & stream, const ShrineMetadata & metadata )
    {
        return stream << metadata.allowedSpells;
    }

    IStreamBase & operator>>( IStreamBase & stream, ShrineMetadata & metadata )
    {
        return stream >> metadata.allowedSpells;
    }

    bool loadBaseMap( const std::string & path, BaseMapFormat & map )
    {
        if ( path.empty() ) {
            return false;
        }

        StreamFile fileStream;
        fileStream.setBigendian( true );

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
        fileStream.setBigendian( true );

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
        fileStream.setBigendian( true );

        if ( !fileStream.open( path, "wb" ) ) {
            return false;
        }

        for ( const uint8_t value : magicWord ) {
            fileStream.put( value );
        }

        return saveToStream( fileStream, map );
    }
}
