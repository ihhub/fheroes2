/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "artifact.h"
#include "artifact_ultimate.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "castle.h"
#include "color.h"
#include "game_over.h"
#include "heroes.h"
#include "kingdom.h"
#include "logging.h"
#include "map_format_helper.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "maps_objects.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "mp2.h"
#include "mp2_helper.h"
#include "players.h"
#include "race.h"
#include "rand.h"
#include "resource.h"
#include "serialize.h"
#include "settings.h"
#include "world.h" // IWYU pragma: associated
#include "world_object_uid.h"

namespace
{
    const int32_t ultimateArtifactOffset = 9;

    Artifact getUltimateArtifact()
    {
        if ( Settings::Get().isCampaignGameType() ) {
            const Campaign::ScenarioVictoryCondition victoryCondition = Campaign::getCurrentScenarioVictoryCondition();
            if ( victoryCondition == Campaign::ScenarioVictoryCondition::OBTAIN_ULTIMATE_CROWN ) {
                return Artifact::ULTIMATE_CROWN;
            }
            else if ( victoryCondition == Campaign::ScenarioVictoryCondition::OBTAIN_SPHERE_NEGATION ) {
                return Artifact::SPHERE_NEGATION;
            }
        }

        return Artifact::Rand( Artifact::ART_ULTIMATE );
    }

    void updateCastleNames( const AllCastles & castles )
    {
        // Find castles with no names.
        std::vector<Castle *> castleWithNoName;
        std::set<std::string, std::less<>> castleNames;

        for ( Castle * castle : castles ) {
            if ( castle == nullptr ) {
                // How do we have an empty pointer in this container?
                assert( 0 );
                continue;
            }

            const std::string & name = castle->GetName();

            if ( name.empty() ) {
                castleWithNoName.emplace_back( castle );
            }
            else {
                castleNames.emplace( name );
            }
        }

        if ( castleWithNoName.empty() ) {
            return;
        }

        for ( Castle * castle : castleWithNoName ) {
            castle->setName( castleNames );
            castleNames.emplace( castle->GetName() );
        }
    }

    void updateArtifactStats()
    {
        // Clear artifact flags to correctly generate random artifacts.
        fheroes2::ResetArtifactStats();

        const Maps::FileInfo & mapInfo = Settings::Get().getCurrentMapInfo();

        // do not let the player get a random artifact that allows him to win the game
        if ( ( mapInfo.ConditionWins() & GameOver::WINS_ARTIFACT ) == GameOver::WINS_ARTIFACT && !mapInfo.WinsFindUltimateArtifact() ) {
            fheroes2::ExcludeArtifactFromRandom( mapInfo.WinsFindArtifactID() );
        }
    }
}

bool World::LoadMapMP2( const std::string & filename, const bool isOriginalMp2File )
{
    Reset();
    Defaults();

    StreamFile fs;
    if ( !fs.open( filename, "rb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Map file not found " << filename )
        return false;
    }

    // Read magic number.
    if ( fs.getBE32() != 0x5C000000 ) {
        // It is not a MP2 or MX2 file.
        return false;
    }

    const size_t totalFileSize = fs.size();
    if ( totalFileSize < MP2::MP2_MAP_INFO_SIZE ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Map file " << filename << " is corrupted" )
        return false;
    }

    // Go to the end of the file and read last 4 bytes which are used as a UID counter for all objects on the map.
    fs.seek( totalFileSize - 4 );

    // In theory, this counter can be smaller than the some object UIDs if the map is corrupted or modified manually.
    Maps::setLastObjectUID( fs.getLE32() );

    // Go to the end of the map info section to read two 32-bit values representing width and height of the map.
    fs.seek( MP2::MP2_MAP_INFO_SIZE - 2 * 4 );

    const uint32_t mapWidth = fs.getLE32();
    switch ( mapWidth ) {
    case Maps::SMALL:
    case Maps::MEDIUM:
    case Maps::LARGE:
    case Maps::XLARGE:
        width = static_cast<int32_t>( mapWidth );
        break;
    default:
        width = 0;
        break;
    }

    const uint32_t mapHeight = fs.getLE32();
    switch ( mapHeight ) {
    case Maps::SMALL:
    case Maps::MEDIUM:
    case Maps::LARGE:
    case Maps::XLARGE:
        height = static_cast<int32_t>( mapHeight );
        break;
    default:
        height = 0;
        break;
    }

    if ( width == 0 || height == 0 || width != height ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid MP2 file format: dimensions of the map [" << width << " x " << height << "] are incorrect." )
        return false;
    }

    // This is to make sure that we are at the end of the map info section.
    assert( fs.tell() == MP2::MP2_MAP_INFO_SIZE );

    const int32_t worldSize = width * height;

    if ( totalFileSize < MP2::MP2_MAP_INFO_SIZE + static_cast<size_t>( worldSize ) * MP2::MP2_TILE_STRUCTURE_SIZE + MP2::MP2_ADDON_COUNT_SIZE ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Map file " << filename << " is corrupted" )
        return false;
    }

    // Skip MP2 tile structures for now and read addons.
    fs.skip( static_cast<size_t>( worldSize ) * MP2::MP2_TILE_STRUCTURE_SIZE );

    // It is a valid case that a map has no add-ons.
    const size_t addonCount = fs.getLE32();
    std::vector<MP2::MP2AddonInfo> vec_mp2addons( addonCount );

    if ( totalFileSize < MP2::MP2_MAP_INFO_SIZE + static_cast<size_t>( worldSize ) * MP2::MP2_TILE_STRUCTURE_SIZE + addonCount * MP2::MP2_ADDON_STRUCTURE_SIZE
                             + MP2::MP2_ADDON_COUNT_SIZE ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Map file " << filename << " is corrupted" )
        return false;
    }

    for ( auto & mp2addon : vec_mp2addons ) {
        MP2::loadAddon( fs, mp2addon );
    }

    // If this assertion blows up it means that we are not reading the data properly from the file.
    assert( fs.tell()
            == MP2::MP2_MAP_INFO_SIZE + static_cast<size_t>( worldSize ) * MP2::MP2_TILE_STRUCTURE_SIZE + addonCount * MP2::MP2_ADDON_STRUCTURE_SIZE
                   + MP2::MP2_ADDON_COUNT_SIZE );
    const size_t afterAddonInfoPos = fs.tell();

    // Come back to the end of map info section and read information about MP2 tiles.
    fs.seek( MP2::MP2_MAP_INFO_SIZE );

    vec_tiles.resize( worldSize );

    const bool checkPoLObjects = !Settings::Get().isPriceOfLoyaltySupported() && isOriginalMp2File;

    MapsIndexes vec_object; // index maps for OBJ_CASTLE, OBJ_HERO, OBJ_SIGN, OBJ_BOTTLE, OBJ_EVENT
    vec_object.reserve( 128 );

    for ( int32_t i = 0; i < worldSize; ++i ) {
        Maps::Tile & tile = vec_tiles[i];

        MP2::MP2TileInfo mp2tile;
        MP2::loadTile( fs, mp2tile );
        // There are some tiles which have object type as 65 and 193 which are Thatched Hut. This is exactly the same object as Peasant Hut.
        // Since the original number of object types is limited and in order not to confuse players we will convert this type into Peasant Hut.
        if ( mp2tile.mapObjectType == 65 ) {
            mp2tile.mapObjectType = MP2::OBJ_NON_ACTION_PEASANT_HUT;
        }
        else if ( mp2tile.mapObjectType == 193 ) {
            mp2tile.mapObjectType = MP2::OBJ_PEASANT_HUT;
        }

        if ( checkPoLObjects ) {
            switch ( mp2tile.mapObjectType ) {
            case MP2::OBJ_BARRIER:
            case MP2::OBJ_EXPANSION_DWELLING:
            case MP2::OBJ_EXPANSION_OBJECT:
            case MP2::OBJ_JAIL:
            case MP2::OBJ_TRAVELLER_TENT:
                ERROR_LOG( "Failed to load The Price of Loyalty map '" << filename << "' which is not supported by this version of the game." )
                // You are trying to load a PoL map named as a MP2 file.
                return false;
            default:
                break;
            }
        }

        tile.Init( i, mp2tile );

        // Read extra information if it's present.
        size_t addonIndex = mp2tile.nextAddonIndex;
        while ( addonIndex > 0 ) {
            if ( vec_mp2addons.size() <= addonIndex ) {
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid MP2 format: incorrect addon index " << addonIndex )
                break;
            }
            tile.pushGroundObjectPart( vec_mp2addons[addonIndex] );
            tile.pushTopObjectPart( vec_mp2addons[addonIndex] );
            addonIndex = vec_mp2addons[addonIndex].nextAddonIndex;
        }

        tile.sortObjectParts();

        if ( MP2::doesObjectNeedExtendedMetadata( tile.getMainObjectType() ) ) {
            vec_object.push_back( i );
        }
    }

    // If this assertion blows up it means that we are not reading the data properly from the file.
    assert( fs.tell() == MP2::MP2_MAP_INFO_SIZE + static_cast<size_t>( worldSize ) * MP2::MP2_TILE_STRUCTURE_SIZE );

    // Go back to the section after the add-on structure information and read the rest of data.
    fs.seek( afterAddonInfoPos );

    if ( totalFileSize < afterAddonInfoPos + static_cast<size_t>( MP2::MP2_CASTLE_COUNT * MP2::MP2_CASTLE_POSITION_SIZE ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Map file " << filename << " is corrupted" )
        return false;
    }

    // Read castle / town coordinates. The number of castles is fixed per map.
    for ( int32_t i = 0; i < MP2::MP2_CASTLE_COUNT; ++i ) {
        const int32_t posX = fs.get();
        const int32_t posY = fs.get();
        const int32_t castleType = fs.get();

        if ( 0xFF == posX && 0xFF == posY ) {
            // This is an empty block so skip it.
            continue;
        }

        // Types from 0x0 to 0x7F are to towns, from 0x80 to 0xFF are for castles.
        switch ( castleType ) {
        case 0x00:
        case 0x80:
            vec_castles.AddCastle( std::make_unique<Castle>( posX, posY, Race::KNGT ) );
            break;
        case 0x01:
        case 0x81:
            vec_castles.AddCastle( std::make_unique<Castle>( posX, posY, Race::BARB ) );
            break;
        case 0x02:
        case 0x82:
            vec_castles.AddCastle( std::make_unique<Castle>( posX, posY, Race::SORC ) );
            break;
        case 0x03:
        case 0x83:
            vec_castles.AddCastle( std::make_unique<Castle>( posX, posY, Race::WRLK ) );
            break;
        case 0x04:
        case 0x84:
            vec_castles.AddCastle( std::make_unique<Castle>( posX, posY, Race::WZRD ) );
            break;
        case 0x05:
        case 0x85:
            vec_castles.AddCastle( std::make_unique<Castle>( posX, posY, Race::NECR ) );
            break;
        case 0x06:
        case 0x86:
            vec_castles.AddCastle( std::make_unique<Castle>( posX, posY, Race::NONE ) );
            break;
        default:
            DEBUG_LOG( DBG_GAME, DBG_WARN,
                       "Invalid MP2 format: castle at position [" << posX << "; " << posY << "], index " << posX + posY * width << " has invalid castle type "
                                                                  << castleType )
            break;
        }

        // Add the castle to the list of objects which can be captured.
        map_captureobj.Set( Maps::GetIndexFromAbsPoint( posX, posY ), MP2::OBJ_CASTLE, Color::NONE );
    }

    // If this assertion blows up it means that we are not reading the data properly from the file.
    assert( fs.tell() == afterAddonInfoPos + static_cast<size_t>( MP2::MP2_CASTLE_COUNT * MP2::MP2_CASTLE_POSITION_SIZE ) );

    if ( totalFileSize
         < afterAddonInfoPos
               + static_cast<size_t>( MP2::MP2_CASTLE_COUNT * MP2::MP2_CASTLE_POSITION_SIZE + MP2::MP2_CAPTURE_OBJECT_COUNT * MP2::MP2_CAPTURE_OBJECT_POSITION_SIZE ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Map file " << filename << " is corrupted" )
        return false;
    }

    // Read information about objects which can be captured on the map.
    for ( int32_t i = 0; i < MP2::MP2_CAPTURE_OBJECT_COUNT; ++i ) {
        const int32_t posX = fs.get();
        const int32_t posY = fs.get();
        const int32_t objectType = fs.get();

        if ( 0xFF == posX && 0xFF == posY ) {
            // This is an empty block so skip it.
            continue;
        }

        switch ( objectType ) {
        case 0x00:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( posX, posY ), MP2::OBJ_SAWMILL, Color::NONE );
            break;
        case 0x01:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( posX, posY ), MP2::OBJ_ALCHEMIST_LAB, Color::NONE );
            break;
        case 0x02: // Ore mine.
        case 0x03: // Sulfur mine.
        case 0x04: // Crystal mine.
        case 0x05: // Gems mine.
        case 0x06: // Gold mine.
            // TODO: should we verify the mine type by something?
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( posX, posY ), MP2::OBJ_MINE, Color::NONE );
            break;
        case 0x64:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( posX, posY ), MP2::OBJ_LIGHTHOUSE, Color::NONE );
            break;
        case 0x65:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( posX, posY ), MP2::OBJ_DRAGON_CITY, Color::NONE );
            break;
        case 0x67:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( posX, posY ), MP2::OBJ_ABANDONED_MINE, Color::NONE );
            break;
        default:
            DEBUG_LOG( DBG_GAME, DBG_WARN,
                       "Invalid MP2 format: unknown capture object type " << objectType << " at position [" << posX << "; " << posY << "], index "
                                                                          << posX + posY * width )
            break;
        }
    }

    // If this assertion blows up it means that we are not reading the data properly from the file.
    assert(
        fs.tell()
        == afterAddonInfoPos
               + static_cast<size_t>( MP2::MP2_CASTLE_COUNT * MP2::MP2_CASTLE_POSITION_SIZE + MP2::MP2_CAPTURE_OBJECT_COUNT * MP2::MP2_CAPTURE_OBJECT_POSITION_SIZE ) );

    // TODO: find a way to use this value properly.
    const uint8_t obeliskCount = fs.get();
    (void)obeliskCount;

    // Get the amount of last information blocks to be read.
    // It looks like this is a versioning system since only the last 2 entries matter.
    uint32_t infoBlockCount = 0;
    while ( true ) {
        const uint32_t l = fs.get();
        const uint32_t h = fs.get();

        if ( fs.tell() == fs.size() ) {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Map file " << filename << " is corrupted" )
            return false;
        }

        if ( 0 == h && 0 == l ) {
            break;
        }

        infoBlockCount = 256 * h + l - 1;
    }

    // castle or heroes or (events, rumors, etc)
    for ( uint32_t i = 0; i < infoBlockCount; ++i ) {
        int32_t objectTileId = -1;

        const size_t blockSize = fs.getLE16();
        if ( blockSize == 0 ) {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid MP2 file format: received an empty block size " )
            continue;
        }

        const std::vector<uint8_t> pblock = fs.getRaw( blockSize );

        for ( const int32_t tileId : vec_object ) {
            const Maps::Tile & tile = vec_tiles[tileId];
            if ( ( tile.getMainObjectPart().layerType & 0x3 ) != Maps::OBJECT_LAYER ) {
                continue;
            }

            if ( tile.metadata()[0] == i + 1 ) {
                objectTileId = tileId;
                break;
            }
        }

        if ( 0 <= objectTileId ) {
            const Maps::Tile & tile = vec_tiles[objectTileId];
            const MP2::MapObjectType objectType = tile.getMainObjectType();

            switch ( objectType ) {
            case MP2::OBJ_CASTLE:
                if ( MP2::MP2_CASTLE_STRUCTURE_SIZE != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid MP2 file format: received invalid castle structure size equal to " << pblock.size() )
                }
                else {
                    Castle * castle = getCastleEntrance( Maps::GetPoint( objectTileId ) );
                    if ( castle ) {
                        castle->LoadFromMP2( pblock );
                        map_captureobj.SetColor( tile.GetIndex(), castle->GetColor() );
                    }
                    else {
                        DEBUG_LOG( DBG_GAME, DBG_WARN,
                                   "load castle: "
                                       << "not found, index: " << objectTileId )
                    }
                }
                break;
            case MP2::OBJ_RANDOM_TOWN:
            case MP2::OBJ_RANDOM_CASTLE:
                if ( MP2::MP2_CASTLE_STRUCTURE_SIZE != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid MP2 file format: received invalid castle structure size equal to " << pblock.size() )
                }
                else {
                    // Random castle's entrance tile is marked as OBJ_RNDCASTLE or OBJ_RNDTOWN instead of OBJ_CASTLE.
                    Castle * castle = getCastle( Maps::GetPoint( objectTileId ) );
                    if ( castle ) {
                        castle->LoadFromMP2( pblock );
                        Maps::UpdateCastleSprite( castle->GetCenter(), castle->GetRace(), castle->isCastle(), true );
                        Maps::ReplaceRandomCastleObjectId( castle->GetCenter() );
                        map_captureobj.SetColor( tile.GetIndex(), castle->GetColor() );
                    }
                    else {
                        DEBUG_LOG( DBG_GAME, DBG_WARN,
                                   "load castle: "
                                       << "not found, index: " << objectTileId )
                    }
                }
                break;
            case MP2::OBJ_JAIL:
                if ( MP2::MP2_HEROES_STRUCTURE_SIZE != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN,
                               "Invalid MP2 file format: expected minimum size of Jail structure is " << MP2::MP2_HEROES_STRUCTURE_SIZE << " while loaded size is "
                                                                                                      << pblock.size() )
                }
                else {
                    // Byte 60 contains race type information.
                    const uint8_t raceId = pblock[60];
                    int raceType = Race::KNGT;
                    switch ( raceId ) {
                    case 0:
                        raceType = Race::KNGT;
                        break;
                    case 1:
                        raceType = Race::BARB;
                        break;
                    case 2:
                        raceType = Race::SORC;
                        break;
                    case 3:
                        raceType = Race::WRLK;
                        break;
                    case 4:
                        raceType = Race::WZRD;
                        break;
                    case 5:
                        raceType = Race::NECR;
                        break;
                    default:
                        DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid MP2 file format: unknown race ID for Jail hero " << static_cast<int>( raceId ) )
                        break;
                    }

                    Heroes * hero = nullptr;

                    // Byte 17 determines whether the hero has a custom portrait, and byte 18 contains the custom portrait ID. If the hero has a custom portrait, then we
                    // should directly use the hero corresponding to this portrait, if possible.
                    // MP2 format stores hero IDs start from 0, while fheroes2 engine starts from 1.
                    if ( pblock[17] && pblock[18] + 1 <= Heroes::JARKONAS ) {
                        hero = vec_heroes.Get( pblock[18] + 1 );
                    }

                    if ( !hero || !hero->isAvailableForHire() ) {
                        hero = GetHeroForHire( raceType );
                    }

                    if ( hero ) {
                        hero->LoadFromMP2( objectTileId, Color::NONE, raceType, true, pblock );
                    }
                    else {
                        DEBUG_LOG( DBG_GAME, DBG_WARN, "MP2 file format: no free heroes are available from race " << Race::String( raceType ) )
                    }
                }
                break;
            case MP2::OBJ_HERO:
                if ( MP2::MP2_HEROES_STRUCTURE_SIZE != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN,
                               "read heroes: "
                                   << "incorrect size block: " << pblock.size() )
                }
                else {
                    std::pair<int, int> colorRace = Maps::getColorRaceFromHeroSprite( tile.getMainObjectPart().icnIndex );
                    const Kingdom & kingdom = GetKingdom( colorRace.first );

                    if ( colorRace.second == Race::RAND && colorRace.first != Color::NONE ) {
                        colorRace.second = kingdom.GetRace();
                    }

                    // Check if the kingdom has exceeded the limit on hired heroes
                    if ( kingdom.AllowRecruitHero( false ) ) {
                        Heroes * hero = nullptr;

                        // Byte 17 determines whether the hero has a custom portrait, and byte 18 contains the custom portrait ID. If the hero has a custom portrait, then
                        // we should directly use the hero corresponding to this portrait, if possible.
                        // MP2 format stores hero IDs start from 0, while fheroes2 engine starts from 1.
                        if ( pblock[17] && pblock[18] + 1 <= Heroes::JARKONAS ) {
                            hero = vec_heroes.Get( pblock[18] + 1 );
                        }

                        if ( !hero || !hero->isAvailableForHire() ) {
                            hero = GetHeroForHire( colorRace.second );
                        }

                        if ( hero ) {
                            hero->LoadFromMP2( objectTileId, colorRace.first, colorRace.second, false, pblock );
                        }
                        else {
                            DEBUG_LOG( DBG_GAME, DBG_WARN, "MP2 file format: no free heroes are available from race " << Race::String( colorRace.second ) )
                        }
                    }
                    else {
                        DEBUG_LOG( DBG_GAME, DBG_WARN, "load heroes maximum" )
                    }
                }
                break;
            case MP2::OBJ_SIGN:
            case MP2::OBJ_BOTTLE:
                if ( MP2::MP2_SIGN_STRUCTURE_MIN_SIZE <= pblock.size() && 0x01 == pblock[0] ) {
                    auto obj = std::make_unique<MapSign>();
                    obj->LoadFromMP2( objectTileId, pblock );

                    map_objects.add( std::move( obj ) );
                }
                break;
            case MP2::OBJ_EVENT:
                if ( MP2::MP2_EVENT_STRUCTURE_MIN_SIZE <= pblock.size() && 0x01 == pblock[0] ) {
                    auto obj = std::make_unique<MapEvent>();
                    obj->LoadFromMP2( objectTileId, pblock );

                    map_objects.add( std::move( obj ) );
                }
                break;
            case MP2::OBJ_SPHINX:
                if ( MP2::MP2_RIDDLE_STRUCTURE_MIN_SIZE <= pblock.size() && 0x00 == pblock[0] ) {
                    auto obj = std::make_unique<MapSphinx>();
                    obj->LoadFromMP2( objectTileId, pblock );

                    obj->validate();

                    map_objects.add( std::move( obj ) );
                }
                break;
            default:
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid MP2 file format: unhandled object type " << MP2::StringObject( objectType ) )
                break;
            }
        }
        // other events
        else if ( 0x00 == pblock[0] ) {
            // Daily event.
            if ( MP2::MP2_EVENT_STRUCTURE_MIN_SIZE <= pblock.size() && pblock[42] == 1 ) {
                vec_eventsday.emplace_back();
                vec_eventsday.back().LoadFromMP2( pblock );
            }
            else if ( MP2::MP2_RUMOR_STRUCTURE_MIN_SIZE <= pblock.size() ) {
                // Structure containing information about a rumor.
                //
                // - uint8_t (1 byte)
                //     Always equal to 0.
                //
                // - unused 7 bytes
                //    Unknown / unused. TODO: find out what these bytes used for.
                //
                // - string
                //    Null terminated string of the rumor.
                std::string rumor( reinterpret_cast<const char *>( pblock.data() ) + 8 );

                if ( !rumor.empty() ) {
                    _customRumors.emplace_back( std::move( rumor ) );
                    DEBUG_LOG( DBG_GAME, DBG_INFO, "MP2 format: add rumor " << _customRumors.back() )
                }
            }
            else {
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid MP2 format: unknown event type object of size of " << pblock.size() )
            }
        }
        else {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid MP2 format: unknown information object of size of " << pblock.size() )
        }
    }

    // If this assertion blows up it means that we are not reading the data properly from the file.
    assert( fs.tell() + 4 == fs.size() );

    updateCastleNames( vec_castles );

    updateArtifactStats();

    if ( !ProcessNewMP2Map( filename, checkPoLObjects ) ) {
        return false;
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "Loading of MP2 map is completed." )
    return true;
}

bool World::loadResurrectionMap( const std::string & filename )
{
    Reset();
    Defaults();

    Maps::Map_Format::MapFormat map;
    if ( !Maps::Map_Format::loadMap( filename, map ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Map file '" << filename << "' is corrupted or missing." )
        return false;
    }

    width = map.size;
    height = map.size;

    vec_tiles.resize( static_cast<size_t>( width ) * height );

    if ( !Maps::readAllTiles( map ) ) {
        return false;
    }

    if ( !Maps::updateMapPlayers( map ) ) {
        return false;
    }

    if ( map.availablePlayerColors == 0 ) {
        // No players inside the map.
        return false;
    }

    // Read and populate objects.
    const auto & townObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_TOWNS );
    const auto & heroObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_HEROES );
    const auto & miscellaneousObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS );
    const auto & waterObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_WATER );
    const auto & artifactObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_ARTIFACTS );

#if defined( WITH_DEBUG )
    std::set<uint32_t> standardMetadataUIDs;
    std::set<uint32_t> castleMetadataUIDs;
    std::set<uint32_t> heroMetadataUIDs;
    std::set<uint32_t> sphinxMetadataUIDs;
    std::set<uint32_t> signMetadataUIDs;
    std::set<uint32_t> adventureMapEventMetadataUIDs;
#endif

    std::set<size_t> hiredHeroTileId;

    for ( size_t tileId = 0; tileId < map.tiles.size(); ++tileId ) {
        const auto & tile = map.tiles[tileId];

        for ( const auto & object : tile.objects ) {
            if ( object.group == Maps::ObjectGroup::KINGDOM_TOWNS ) {
#if defined( WITH_DEBUG )
                castleMetadataUIDs.emplace( object.id );
#endif
                assert( map.castleMetadata.find( object.id ) != map.castleMetadata.end() );
                auto & castleInfo = map.castleMetadata[object.id];

                const int color = Color::IndexToColor( Maps::getTownColorIndex( map, tileId, object.id ) );

                int race = Race::IndexToRace( static_cast<int>( townObjects[object.index].metadata[0] ) );
                const bool isRandom = ( race == Race::RAND );

                if ( isRandom ) {
                    assert( townObjects[object.index].objectType == MP2::OBJ_RANDOM_CASTLE || townObjects[object.index].objectType == MP2::OBJ_RANDOM_TOWN );

                    if ( ( color & Color::ALL ) == 0 ) {
                        // This is a neutral town.
                        race = Race::Rand();
                    }
                    else {
                        const Kingdom & kingdom = GetKingdom( color );
                        assert( kingdom.GetColor() == color );

                        race = static_cast<uint8_t>( kingdom.GetRace() );
                    }
                }

                assert( ( std::find( castleInfo.builtBuildings.begin(), castleInfo.builtBuildings.end(), BUILD_CASTLE ) != castleInfo.builtBuildings.end() )
                        == ( townObjects[object.index].metadata[1] != 0 ) );
                assert( ( std::find( castleInfo.builtBuildings.begin(), castleInfo.builtBuildings.end(), BUILD_TENT ) != castleInfo.builtBuildings.end() )
                        == ( townObjects[object.index].metadata[1] == 0 ) );

                fheroes2::Point castleCenter;
                bool isCastle;

                {
                    auto castle = std::make_unique<Castle>( static_cast<int32_t>( tileId ) % width, static_cast<int32_t>( tileId ) / width, race );
                    castle->SetColor( color );
                    castle->loadFromResurrectionMap( castleInfo );

                    castleCenter = castle->GetCenter();
                    isCastle = castle->isCastle();

                    vec_castles.AddCastle( std::move( castle ) );
                }

                if ( isRandom ) {
                    Maps::UpdateCastleSprite( castleCenter, race, isCastle, true );
                    Maps::ReplaceRandomCastleObjectId( castleCenter );
                }

                map_captureobj.Set( static_cast<int32_t>( tileId ), MP2::OBJ_CASTLE, color );
            }
            else if ( object.group == Maps::ObjectGroup::KINGDOM_HEROES ) {
#if defined( WITH_DEBUG )
                heroMetadataUIDs.emplace( object.id );
#endif

                assert( map.heroMetadata.find( object.id ) != map.heroMetadata.end() );

                const auto & metadata = heroObjects[object.index].metadata;
                auto & heroInfo = map.heroMetadata[object.id];

                const int color = Color::IndexToColor( static_cast<int>( metadata[0] ) );

                // Check the race correctness.
                assert( heroInfo.race == Race::IndexToRace( metadata[1] ) );

                // Heroes can not be neutral.
                assert( color != Color::NONE );

                const Kingdom & kingdom = GetKingdom( color );

                // Set race for random hero according to the kingdom's race.
                if ( heroInfo.race == Race::RAND ) {
                    heroInfo.race = static_cast<uint8_t>( kingdom.GetRace() );
                }

                // Check if the kingdom has exceeded the limit on hired heroes
                if ( kingdom.AllowRecruitHero( false ) ) {
                    Heroes * hero = GetHeroForHire( heroInfo.race );
                    if ( hero != nullptr ) {
                        hero->SetCenter( { static_cast<int32_t>( tileId ) % width, static_cast<int32_t>( tileId ) / width } );

                        hero->SetColor( color );

                        hero->applyHeroMetadata( heroInfo, false, false );

                        hiredHeroTileId.emplace( tileId );
                    }
                    else {
                        VERBOSE_LOG( "A hero at position " << tileId << " with UID " << object.id << " failed to be hired." )
                    }
                }
                else {
                    VERBOSE_LOG( "A hero at position " << tileId << " with UID " << object.id << " cannot be hired." )
                }
            }
            else if ( object.group == Maps::ObjectGroup::MONSTERS ) {
#if defined( WITH_DEBUG )
                standardMetadataUIDs.emplace( object.id );
#endif
                assert( map.standardMetadata.find( object.id ) != map.standardMetadata.end() );
                auto & objectInfo = map.standardMetadata[object.id];

                std::array<uint32_t, 3> & tileData = vec_tiles[static_cast<int32_t>( tileId )].metadata();

                for ( size_t idx = 0; idx < objectInfo.metadata.size(); ++idx ) {
                    tileData[idx] = static_cast<uint32_t>( objectInfo.metadata[idx] );
                }
            }
            else if ( object.group == Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS ) {
                assert( object.index < miscellaneousObjects.size() );

                const auto objectType = miscellaneousObjects[object.index].objectType;
                switch ( objectType ) {
                case MP2::OBJ_EVENT: {
#if defined( WITH_DEBUG )
                    adventureMapEventMetadataUIDs.emplace( object.id );
#endif
                    assert( map.adventureMapEventMetadata.find( object.id ) != map.adventureMapEventMetadata.end() );
                    auto & eventInfo = map.adventureMapEventMetadata[object.id];

                    eventInfo.humanPlayerColors = eventInfo.humanPlayerColors & map.humanPlayerColors;
                    eventInfo.computerPlayerColors = eventInfo.computerPlayerColors & map.computerPlayerColors;

                    const int humanColors = Players::HumanColors() & eventInfo.humanPlayerColors;
                    const int computerColors = ( ~Players::HumanColors() ) & eventInfo.computerPlayerColors;

                    if ( humanColors == 0 && computerColors == 0 ) {
                        // This event is not being executed for anyone. Skip it.
                        break;
                    }

                    // TODO: change MapEvent to support map format functionality.
                    auto eventObject = std::make_unique<MapEvent>();
                    eventObject->resources = eventInfo.resources;
                    eventObject->artifact = eventInfo.artifact;
                    if ( eventInfo.artifact == Artifact::SPELL_SCROLL ) {
                        eventObject->artifact.SetSpell( eventInfo.artifactMetadata );
                    }

                    eventObject->computer = ( computerColors != 0 );
                    eventObject->colors = humanColors | computerColors;
                    eventObject->message = std::move( eventInfo.message );
                    eventObject->isSingleTimeEvent = !eventInfo.isRecurringEvent;

                    eventObject->setUIDAndIndex( static_cast<int32_t>( tileId ) );

                    map_objects.add( std::move( eventObject ) );

                    break;
                }
                case MP2::OBJ_JAIL: {
#if defined( WITH_DEBUG )
                    heroMetadataUIDs.emplace( object.id );
#endif

                    assert( map.heroMetadata.find( object.id ) != map.heroMetadata.end() );

                    auto & heroInfo = map.heroMetadata[object.id];

                    const int color = Color::NONE;

                    if ( heroInfo.race == Race::RAND ) {
                        heroInfo.race = static_cast<uint8_t>( Race::Rand() );
                    }

                    Heroes * hero = GetHeroForHire( heroInfo.race );
                    if ( hero != nullptr ) {
                        hero->SetCenter( { static_cast<int32_t>( tileId ) % width, static_cast<int32_t>( tileId ) / width } );

                        hero->SetColor( color );

                        hero->applyHeroMetadata( heroInfo, true, false );
                    }
                    else {
                        VERBOSE_LOG( "A hero at position " << tileId << " with UID " << object.id << " failed to be hired." )
                    }
                    break;
                }
                case MP2::OBJ_SIGN: {
#if defined( WITH_DEBUG )
                    signMetadataUIDs.emplace( object.id );
#endif
                    assert( map.signMetadata.find( object.id ) != map.signMetadata.end() );
                    auto & signInfo = map.signMetadata[object.id];

                    auto signObject = std::make_unique<MapSign>();
                    signObject->message = std::move( signInfo.message );
                    signObject->setUIDAndIndex( static_cast<int32_t>( tileId ) );
                    if ( signObject->message.empty() ) {
                        signObject->setDefaultMessage();
                    }

                    map_objects.add( std::move( signObject ) );

                    break;
                }
                case MP2::OBJ_SPHINX: {
#if defined( WITH_DEBUG )
                    sphinxMetadataUIDs.emplace( object.id );
#endif

                    assert( map.sphinxMetadata.find( object.id ) != map.sphinxMetadata.end() );
                    auto & sphinxInfo = map.sphinxMetadata[object.id];

                    auto sphinxObject = std::make_unique<MapSphinx>();
                    sphinxObject->riddle = std::move( sphinxInfo.riddle );

                    for ( auto & answer : sphinxInfo.answers ) {
                        if ( !answer.empty() ) {
                            sphinxObject->answers.push_back( std::move( answer ) );
                        }
                        else {
                            // How is it even possible?
                            assert( 0 );
                        }
                    }

                    sphinxObject->resources = sphinxInfo.resources;
                    sphinxObject->artifact = sphinxInfo.artifact;
                    if ( sphinxInfo.artifact == Artifact::SPELL_SCROLL ) {
                        sphinxObject->artifact.SetSpell( sphinxInfo.artifactMetadata );
                    }

                    sphinxObject->validate();

                    sphinxObject->setUIDAndIndex( static_cast<int32_t>( tileId ) );

                    // The original game assumes answers only to be up to 4 characters.
                    // However, it seems logically incorrect.
                    sphinxObject->isTruncatedAnswer = false;

                    map_objects.add( std::move( sphinxObject ) );

                    break;
                }
                default:
                    // Other objects do not have metadata as of now.
                    break;
                }
            }
            else if ( object.group == Maps::ObjectGroup::ADVENTURE_WATER ) {
                assert( object.index < waterObjects.size() );

                if ( waterObjects[object.index].objectType == MP2::OBJ_BOTTLE ) {
#if defined( WITH_DEBUG )
                    signMetadataUIDs.emplace( object.id );
#endif
                    assert( map.signMetadata.find( object.id ) != map.signMetadata.end() );
                    auto & signInfo = map.signMetadata[object.id];

                    auto signObject = std::make_unique<MapSign>();
                    signObject->message = std::move( signInfo.message );
                    signObject->setUIDAndIndex( static_cast<int32_t>( tileId ) );
                    if ( signObject->message.empty() ) {
                        signObject->setDefaultMessage();
                    }

                    map_objects.add( std::move( signObject ) );
                }
            }
            else if ( object.group == Maps::ObjectGroup::ADVENTURE_ARTIFACTS ) {
#if defined( WITH_DEBUG )
                standardMetadataUIDs.emplace( object.id );
#endif

                assert( map.standardMetadata.find( object.id ) != map.standardMetadata.end() );
                auto & objectInfo = map.standardMetadata[object.id];

                std::array<uint32_t, 3> & tileData = vec_tiles[static_cast<int32_t>( tileId )].metadata();

                for ( size_t idx = 0; idx < objectInfo.metadata.size(); ++idx ) {
                    tileData[idx] = static_cast<uint32_t>( objectInfo.metadata[idx] );
                }

                assert( object.index < artifactObjects.size() );
                if ( artifactObjects[object.index].objectType == MP2::OBJ_ARTIFACT && artifactObjects[object.index].metadata[0] == Artifact::SPELL_SCROLL ) {
                    // This is a hack we need to do since in the original game spells start from 0.
                    // TODO: fix this hack.
                    assert( tileData[0] > 0 );
                    tileData[0] = tileData[0] - 1U;
                }
            }
        }
    }

#if defined( WITH_DEBUG )
    assert( standardMetadataUIDs.size() == map.standardMetadata.size() );
    assert( castleMetadataUIDs.size() == map.castleMetadata.size() );
    assert( heroMetadataUIDs.size() == map.heroMetadata.size() );
    assert( sphinxMetadataUIDs.size() == map.sphinxMetadata.size() );
    assert( signMetadataUIDs.size() == map.signMetadata.size() );
    assert( adventureMapEventMetadataUIDs.size() == map.adventureMapEventMetadata.size() );

    for ( const uint32_t uid : standardMetadataUIDs ) {
        assert( map.standardMetadata.find( uid ) != map.standardMetadata.end() );
    }

    for ( const uint32_t uid : castleMetadataUIDs ) {
        assert( map.castleMetadata.find( uid ) != map.castleMetadata.end() );
    }

    for ( const uint32_t uid : heroMetadataUIDs ) {
        assert( map.heroMetadata.find( uid ) != map.heroMetadata.end() );
    }

    for ( const uint32_t uid : sphinxMetadataUIDs ) {
        assert( map.sphinxMetadata.find( uid ) != map.sphinxMetadata.end() );
    }

    for ( const uint32_t uid : signMetadataUIDs ) {
        assert( map.signMetadata.find( uid ) != map.signMetadata.end() );
    }

    for ( const uint32_t uid : adventureMapEventMetadataUIDs ) {
        assert( map.adventureMapEventMetadata.find( uid ) != map.adventureMapEventMetadata.end() );
    }
#endif

    // Load daily events.
    for ( auto & event : map.dailyEvents ) {
        if ( event.firstOccurrenceDay == 0 ) {
            // This event will never be executed. Skip it.
            continue;
        }

        event.humanPlayerColors = event.humanPlayerColors & map.humanPlayerColors;
        event.computerPlayerColors = event.computerPlayerColors & map.computerPlayerColors;

        const int humanColors = Players::HumanColors() & event.humanPlayerColors;
        const int computerColors = ( ~Players::HumanColors() ) & event.computerPlayerColors;

        if ( humanColors == 0 && computerColors == 0 ) {
            // This event is not being executed for anyone. Skip it.
            continue;
        }

        // TODO: modify EventDate structure to have more flexibility.
        auto & newEvent = vec_eventsday.emplace_back();

        newEvent.message = std::move( event.message );
        newEvent.colors = ( humanColors | computerColors );
        newEvent.isApplicableForAIPlayers = ( computerColors != 0 );

        newEvent.firstOccurrenceDay = event.firstOccurrenceDay;
        newEvent.repeatPeriodInDays = event.repeatPeriodInDays;
        newEvent.resource = event.resources;
    }

    // Load rumors.
    for ( auto & rumor : map.rumors ) {
        if ( !rumor.empty() ) {
            _customRumors.emplace_back( std::move( rumor ) );
        }
    }

    // Verify that a capture or loss object exists.
    if ( map.lossConditionType == Maps::FileInfo::LOSS_HERO ) {
        auto iter = hiredHeroTileId.find( map.lossConditionMetadata[0] );
        if ( iter == hiredHeroTileId.end() ) {
            VERBOSE_LOG( "A hero at tile " << map.lossConditionMetadata[0] << " does not exist." )
            return false;
        }
    }
    else if ( map.lossConditionType == Maps::FileInfo::LOSS_TOWN ) {
        const Castle * castle
            = vec_castles.Get( { static_cast<int32_t>( map.lossConditionMetadata[0] % map.size ), static_cast<int32_t>( map.lossConditionMetadata[0] / map.size ) } );
        if ( castle == nullptr ) {
            VERBOSE_LOG( "A castle at tile " << map.lossConditionMetadata[0] << " does not exist." )
            return false;
        }
    }

    if ( map.victoryConditionType == Maps::FileInfo::VICTORY_KILL_HERO ) {
        auto iter = hiredHeroTileId.find( map.victoryConditionMetadata[0] );
        if ( iter == hiredHeroTileId.end() ) {
            VERBOSE_LOG( "A hero at tile " << map.victoryConditionMetadata[0] << " does not exist." )
            return false;
        }
    }
    else if ( map.victoryConditionType == Maps::FileInfo::VICTORY_CAPTURE_TOWN ) {
        const Castle * castle = vec_castles.Get(
            { static_cast<int32_t>( map.victoryConditionMetadata[0] % map.size ), static_cast<int32_t>( map.victoryConditionMetadata[0] / map.size ) } );
        if ( castle == nullptr ) {
            VERBOSE_LOG( "A castle at tile " << map.victoryConditionMetadata[0] << " does not exist." )
            return false;
        }
    }

    updateCastleNames( vec_castles );

    updateArtifactStats();

    if ( !ProcessNewMP2Map( filename, false ) ) {
        return false;
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "Loading of FH2M map is completed." )

    return true;
}

bool World::ProcessNewMP2Map( const std::string & filename, const bool checkPoLObjects )
{
    for ( Maps::Tile & tile : vec_tiles ) {
        Maps::Tile::fixMP2MapTileObjectType( tile );

        if ( !updateTileMetadata( tile, tile.getMainObjectType(), checkPoLObjects ) ) {
            ERROR_LOG( "Failed to load The Price of Loyalty map '" << filename << "' which is not supported by this version of the game." )
            // You are trying to load a PoL map named as a MP2 file.
            return false;
        }
    }

    // add heroes to kingdoms
    vec_kingdoms.AddHeroes( vec_heroes );

    // add castles to kingdoms
    vec_kingdoms.AddCastles( vec_castles );

    setHeroIdsForMapConditions();

    // Search for a tile with a predefined Ultimate Artifact
    const auto ultArtTileIter
        = std::find_if( vec_tiles.begin(), vec_tiles.end(), []( const Maps::Tile & tile ) { return tile.getMainObjectType() == MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT; } );
    int32_t ultimateArtifactTileId = -1;
    int32_t ultimateArtifactRadius = 0;
    if ( ultArtTileIter != vec_tiles.end() ) {
        ultimateArtifactTileId = ultArtTileIter->GetIndex();
        ultimateArtifactRadius = static_cast<int32_t>( ultArtTileIter->metadata()[0] );

        // Remove the predefined Ultimate Artifact object
        ultArtTileIter->removeObjectPartsByUID( ultArtTileIter->getMainObjectPart()._uid );
    }

    setUltimateArtifact( ultimateArtifactTileId, ultimateArtifactRadius );

    PostLoad( true, false );

    vec_kingdoms.ApplyPlayWithStartingHero();

    addDebugHero();

    return true;
}

bool World::updateTileMetadata( Maps::Tile & tile, const MP2::MapObjectType objectType, const bool checkPoLObjects )
{
    switch ( objectType ) {
    case MP2::OBJ_ARTIFACT:
        updateObjectInfoTile( tile, true );
        if ( checkPoLObjects ) {
            const Artifact art = Maps::getArtifactFromTile( tile );
            if ( fheroes2::isPriceOfLoyaltyArtifact( art.GetID() ) ) {
                return false;
            }
        }

        break;
    case MP2::OBJ_ABANDONED_MINE:
    case MP2::OBJ_ALCHEMIST_LAB:
    case MP2::OBJ_ARCHER_HOUSE:
    case MP2::OBJ_BARRIER:
    case MP2::OBJ_BOAT:
    case MP2::OBJ_CAMPFIRE:
    case MP2::OBJ_CAVE:
    case MP2::OBJ_CITY_OF_DEAD:
    case MP2::OBJ_DAEMON_CAVE:
    case MP2::OBJ_DERELICT_SHIP:
    case MP2::OBJ_DESERT_TENT:
    case MP2::OBJ_DRAGON_CITY:
    case MP2::OBJ_DWARF_COTTAGE:
    case MP2::OBJ_EVENT:
    case MP2::OBJ_EXCAVATION:
    case MP2::OBJ_FLOTSAM:
    case MP2::OBJ_FOUNTAIN:
    case MP2::OBJ_GENIE_LAMP:
    case MP2::OBJ_GOBLIN_HUT:
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_HALFLING_HOLE:
    case MP2::OBJ_LEAN_TO:
    case MP2::OBJ_MAGIC_GARDEN:
    case MP2::OBJ_MINE:
    case MP2::OBJ_MONSTER:
    case MP2::OBJ_PEASANT_HUT:
    case MP2::OBJ_PYRAMID:
    case MP2::OBJ_RANDOM_ARTIFACT:
    case MP2::OBJ_RANDOM_ARTIFACT_MAJOR:
    case MP2::OBJ_RANDOM_ARTIFACT_MINOR:
    case MP2::OBJ_RANDOM_ARTIFACT_TREASURE:
    case MP2::OBJ_RANDOM_MONSTER:
    case MP2::OBJ_RANDOM_MONSTER_MEDIUM:
    case MP2::OBJ_RANDOM_MONSTER_STRONG:
    case MP2::OBJ_RANDOM_MONSTER_VERY_STRONG:
    case MP2::OBJ_RANDOM_MONSTER_WEAK:
    case MP2::OBJ_RANDOM_RESOURCE:
    case MP2::OBJ_RESOURCE:
    case MP2::OBJ_RUINS:
    case MP2::OBJ_SAWMILL:
    case MP2::OBJ_SEA_CHEST:
    case MP2::OBJ_SHRINE_FIRST_CIRCLE:
    case MP2::OBJ_SHRINE_SECOND_CIRCLE:
    case MP2::OBJ_SHRINE_THIRD_CIRCLE:
    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_SHIPWRECK_SURVIVOR:
    case MP2::OBJ_SKELETON:
    case MP2::OBJ_STONE_LITHS:
    case MP2::OBJ_TRAVELLER_TENT:
    case MP2::OBJ_TREASURE_CHEST:
    case MP2::OBJ_TREE_CITY:
    case MP2::OBJ_TREE_HOUSE:
    case MP2::OBJ_TREE_OF_KNOWLEDGE:
    case MP2::OBJ_TROLL_BRIDGE:
    case MP2::OBJ_WAGON:
    case MP2::OBJ_WAGON_CAMP:
    case MP2::OBJ_WATCH_TOWER:
    case MP2::OBJ_WATER_WHEEL:
    case MP2::OBJ_WINDMILL:
    case MP2::OBJ_WITCHS_HUT:
        updateObjectInfoTile( tile, true );
        break;

    case MP2::OBJ_AIR_ALTAR:
    case MP2::OBJ_BARROW_MOUNDS:
    case MP2::OBJ_EARTH_ALTAR:
    case MP2::OBJ_FIRE_ALTAR:
    case MP2::OBJ_WATER_ALTAR:
        // We need to clear metadata because it is being stored as a part of an MP2 object.
        resetObjectMetadata( tile );
        updateObjectInfoTile( tile, true );
        break;

    case MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT:
        // We need information from an Ultimate artifact for later use. We will reset metadata later.
        break;

    case MP2::OBJ_HERO: {
        // remove map editor sprite
        if ( tile.getMainObjectPart().icnType == MP2::OBJ_ICN_TYPE_MINIHERO ) {
            tile.removeObjectPartsByUID( tile.getMainObjectPart()._uid );
        }

        Heroes * chosenHero = GetHeroes( Maps::GetPoint( tile.GetIndex() ) );
        assert( chosenHero != nullptr );

        tile.setHero( chosenHero );

        if ( checkPoLObjects ) {
            Heroes * hero = tile.getHero();
            assert( hero );

            if ( hero->isPoLPortrait() ) {
                return false;
            }

            const BagArtifacts & artifacts = hero->GetBagArtifacts();
            for ( const Artifact & artifact : artifacts ) {
                if ( fheroes2::isPriceOfLoyaltyArtifact( artifact.GetID() ) ) {
                    return false;
                }
            }
        }

        const MP2::MapObjectType updatedObjectType = tile.getMainObjectType( false );
        if ( updatedObjectType != objectType ) {
            return updateTileMetadata( tile, updatedObjectType, checkPoLObjects );
        }

        break;
    }

    default:
        // Remove any metadata from other objects.
        resetObjectMetadata( tile );
        break;
    }

    return true;
}

void World::setUltimateArtifact( const int32_t tileId, const int32_t radius )
{
    assert( radius >= 0 );

    const auto checkTileForSuitabilityForUltArt = [this]( const int32_t idx ) {
        const int32_t x = idx % width;
        if ( x < ultimateArtifactOffset || x >= width - ultimateArtifactOffset ) {
            return false;
        }

        const int32_t y = idx / width;
        if ( y < ultimateArtifactOffset || y >= height - ultimateArtifactOffset ) {
            return false;
        }

        return getTile( idx ).GoodForUltimateArtifact();
    };

    if ( tileId < 0 ) {
        // No tile was set for an Ultimate Artifact.
        std::vector<int32_t> pool;
        pool.reserve( vec_tiles.size() / 2 );

        for ( const Maps::Tile & tile : vec_tiles ) {
            const int32_t idx = tile.GetIndex();

            if ( checkTileForSuitabilityForUltArt( idx ) ) {
                pool.push_back( idx );
            }
        }

        if ( !pool.empty() ) {
            const int32_t pos = Rand::Get( pool );

            ultimate_artifact.Set( pos, getUltimateArtifact() );

            DEBUG_LOG( DBG_GAME, DBG_INFO, "Ultimate Artifact index: " << pos )
        }
        else {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "no suitable tile to place the Ultimate Artifact was found" )
        }

        return;
    }

    assert( tileId < world.w() * world.h() );

    // Use the predefined Ultimate Artifact tile index as a fallback
    int32_t pos = tileId;

    if ( radius > 0 ) {
        MapsIndexes pool = Maps::getAroundIndexes( tileId, radius );

        // Maps::getAroundIndexes() results does not include the central index, so we have to append it manually
        assert( std::find( pool.begin(), pool.end(), tileId ) == pool.end() );
        pool.push_back( tileId );

        pool.erase( std::remove_if( pool.begin(), pool.end(),
                                    [&checkTileForSuitabilityForUltArt]( const int32_t idx ) { return !checkTileForSuitabilityForUltArt( idx ); } ),
                    pool.end() );

        if ( !pool.empty() ) {
            pos = Rand::Get( pool );
        }
    }

    ultimate_artifact.Set( pos, getUltimateArtifact() );

    DEBUG_LOG( DBG_GAME, DBG_INFO, "Predefined Ultimate Artifact tile index: " << tileId << ", radius: " << radius << ", final tile index: " << pos )
}

void World::addDebugHero()
{
    if ( !IS_DEVEL() ) {
        return;
    }

    // If we are in developer mode, then add the DEBUG_HERO
    const int color = Color::GetFirst( Players::HumanColors() );
    assert( color != Color::NONE );

    Kingdom & kingdom = GetKingdom( color );
    if ( kingdom.GetCastles().empty() ) {
        // No castles so no debug hero.
        return;
    }

    const Castle * castle = kingdom.GetCastles().front();
    const fheroes2::Point & cp = castle->GetCenter();
    Heroes * hero = vec_heroes.Get( Heroes::DEBUG_HERO );

    if ( hero && !getTile( cp.x, cp.y + 1 ).getHero() ) {
        hero->Recruit( castle->GetColor(), { cp.x, cp.y + 1 } );
    }
}

void World::setHeroIdsForMapConditions()
{
    const Maps::FileInfo & mapInfo = Settings::Get().getCurrentMapInfo();

    // update wins, loss conditions
    if ( GameOver::WINS_HERO & mapInfo.ConditionWins() ) {
        const fheroes2::Point & pos = mapInfo.WinsMapsPositionObject();

        const Heroes * hero = GetHeroes( pos );
        if ( hero == nullptr ) {
            heroIdAsWinCondition = Heroes::UNKNOWN;
            ERROR_LOG( "A winning condition hero at location ['" << pos.x << ", " << pos.y << "'] is not found." )
        }
        else {
            heroIdAsWinCondition = hero->GetID();
        }
    }

    if ( GameOver::LOSS_HERO & mapInfo.ConditionLoss() ) {
        const fheroes2::Point & pos = mapInfo.LossMapsPositionObject();

        Heroes * hero = GetHeroes( pos );
        if ( hero == nullptr ) {
            heroIdAsLossCondition = Heroes::UNKNOWN;
            ERROR_LOG( "A loosing condition hero at location ['" << pos.x << ", " << pos.y << "'] is not found." )
        }
        else {
            heroIdAsLossCondition = hero->GetID();
            hero->SetModes( Heroes::NOTDISMISS | Heroes::CUSTOM );
        }
    }
}
