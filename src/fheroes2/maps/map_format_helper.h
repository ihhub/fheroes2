/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2026                                             *
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

#include <cstddef>
#include <cstdint>
#include <vector>

class Army;

namespace fheroes2
{
    enum class SupportedLanguage : uint8_t;
}

namespace MP2
{
    enum MapObjectType : uint16_t;
}

namespace Maps
{
    class Tile;

    namespace Map_Format
    {
        struct BaseMapFormat;
        struct MapFormat;
        struct TileInfo;
        struct TileObjectInfo;
        struct CastleMetadata;
        struct HeroMetadata;
    }

    enum class ObjectGroup : uint8_t;

    bool readMapInEditor( const Map_Format::MapFormat & map );
    bool readAllTiles( const Map_Format::MapFormat & map );

    bool readTileObject( Tile & tile, const Map_Format::TileObjectInfo & object );

    void setTerrainWithTransition( Map_Format::MapFormat & map, const int32_t startTileId, const int32_t endTileId, const int groundId );

    // Does not set or correct terrain transitions
    void setTerrainOnTile( Map_Format::MapFormat & map, const int32_t tileId, const int groundId );

    void addObjectToMap( Map_Format::MapFormat & map, const int32_t tileId, const ObjectGroup group, const uint32_t index );

    bool addStream( Map_Format::MapFormat & map, const int32_t tileId );

    // Update the existing streams around the center tile to properly connect them to the center stream.
    void updateStreamsAround( Map_Format::MapFormat & map, const int32_t centerTileId );

    // Update the existing streams to connect them to the River Delta.
    void updateStreamsToDeltaConnection( Map_Format::MapFormat & map, const int32_t tileId, const int deltaDirection );

    // Returns 'Direction::UNKNOWN' if the index does not belong to River Delta object
    int getRiverDeltaDirectionByIndex( const ObjectGroup group, const int32_t objectIndex );

    bool isRiverDeltaObject( const ObjectGroup group, const int32_t objectIndex );

    // This function updates Castles, Towns, Heroes and Capturable objects using their metadata stored in map.
    void updatePlayerRelatedObjects( const Map_Format::MapFormat & map );

    bool updateMapPlayers( Map_Format::MapFormat & map );

    uint8_t getTownColorIndex( const Map_Format::MapFormat & map, const size_t tileIndex, const uint32_t id );

    bool isJailObject( const ObjectGroup group, const uint32_t index );

    // Returns true if object can be owned, excluding Towns and Castles.
    bool isCapturableObject( const MP2::MapObjectType objectType );

    void captureObject( const Map_Format::MapFormat & map, const int32_t tileIndex, const uint32_t objectId, const MP2::MapObjectType objectType );

    uint32_t getBuildingsFromVector( const std::vector<uint32_t> & buildingsVector );

    // Should be used only for the neutral color player.
    void setDefaultCastleDefenderArmy( Map_Format::CastleMetadata & metadata );

    // Returns true if all monsters are RANDOM_MONSTER and count is 0. Should be used only for the neutral color player.
    bool isDefaultCastleDefenderArmy( const Map_Format::CastleMetadata & metadata );

    // Returns false if there are no custom units set in metadata.
    bool loadCastleArmy( Army & army, const Map_Format::CastleMetadata & metadata );
    void saveCastleArmy( const Army & army, Map_Format::CastleMetadata & metadata );
    bool loadHeroArmy( Army & army, const Map_Format::HeroMetadata & metadata );
    void saveHeroArmy( const Army & army, Map_Format::HeroMetadata & metadata );

    bool setRoadOnTile( Map_Format::MapFormat & map, const int32_t tileIndex );
    bool removeRoadFromTile( Map_Format::MapFormat & map, const int32_t tileIndex );

    void updateAllRoads( Map_Format::MapFormat & map );

    bool doesContainRoad( const Map_Format::TileInfo & tile );

    // Changing map language is the only time when a translation is going to be updated for the map.
    // Rather than implementing super complex logic for tracking all object modifications and updating the map format constantly,
    // we just update it once for the whole translation.
    // Consider translations as a snapshot of map in time for a specific language rather than a live copy of it.
    //
    // The workflow for the map maker should be the following:
    // - create a complete map for the main language (by default English)
    // - switch the language to a new one. At this time all texts are going to be copied to English translation
    //   while the existing texts are going to be preserved.
    // - modify the texts for the new language.
    void changeLanguage( Map_Format::MapFormat & map, const fheroes2::SupportedLanguage language );

    // Set the current in-game language for the map. This should be used only for lists of maps displayed for players.
    // Take a note that this function can create a "fake" entry in translations to keep a proper list of languages
    // which we display for map information.
    //
    // Return true only if the language was successfully set.
    bool setInGameLanguage( Map_Format::BaseMapFormat & map, const fheroes2::SupportedLanguage language );

    // This function sets translation language for the map. It should be used only for the game mode and not for the Editor as
    // it doesn't save the current language into the list of translations.
    bool loadTranslation( Map_Format::BaseMapFormat & map, const fheroes2::SupportedLanguage language );
    bool loadTranslation( Map_Format::MapFormat & map, const fheroes2::SupportedLanguage language );

    // This function is used exclusively by the Editor when a map maker wants to remove a translation from the list of supported languages.
    void removeTranslation( Map_Format::MapFormat & map, const fheroes2::SupportedLanguage language );
}
