/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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
#ifndef H2TILES_H
#define H2TILES_H

#include <array>
#include <cstdint>
#include <list>
#include <string>
#include <vector>

#include "color.h"
#include "direction.h"
#include "ground.h"
#include "math_base.h"
#include "mp2.h"
#include "world_regions.h"

class Heroes;
class StreamBase;

namespace Maps
{
    enum ObjectLayerType : uint8_t
    {
        OBJECT_LAYER = 0, // main and action objects like mines, forest, mountains, castles and etc.
        BACKGROUND_LAYER = 1, // background objects like lakes or bushes.
        SHADOW_LAYER = 2, // shadows and some special objects like castle's entrance road.
        TERRAIN_LAYER = 3 // roads, water flaws and cracks. Essentially everything what is a part of terrain.
    };

    struct TilesAddon
    {
        TilesAddon() = default;

        TilesAddon( const uint8_t layerType, const uint32_t uid, const MP2::ObjectIcnType objectIcnType, const uint8_t imageIndex, const bool hasObjectAnimation,
                    const bool isMarkedAsRoad );

        TilesAddon( const TilesAddon & ) = default;

        ~TilesAddon() = default;

        TilesAddon & operator=( const TilesAddon & ) = delete;

        bool operator==( const TilesAddon & addon ) const
        {
            return ( _uid == addon._uid ) && ( _layerType == addon._layerType ) && ( _objectIcnType == addon._objectIcnType ) && ( _imageIndex == addon._imageIndex )
                   && ( _hasObjectAnimation == addon._hasObjectAnimation ) && ( _isMarkedAsRoad == addon._isMarkedAsRoad );
        }

        bool isUniq( const uint32_t id ) const
        {
            return _uid == id;
        }

        bool isRoad() const;

        bool hasSpriteAnimation() const
        {
            return _hasObjectAnimation;
        }

        std::string String( int level ) const;

        static bool isShadow( const TilesAddon & ta );

        static bool isResource( const TilesAddon & ta );
        static bool isArtifact( const TilesAddon & ta );

        static bool PredicateSortRules1( const TilesAddon & ta1, const TilesAddon & ta2 );

        // Unique identifier of an object. UID can be shared among multiple object parts if an object is bigger than 1 tile.
        uint32_t _uid{ 0 };

        // Layer type shows how the object is rendered on Adventure Map. See ObjectLayerType enumeration.
        uint8_t _layerType{ OBJECT_LAYER };

        // The type of object which correlates to ICN id. See MP2::getIcnIdFromObjectIcnType() function for more details.
        MP2::ObjectIcnType _objectIcnType{ MP2::OBJ_ICN_TYPE_UNKNOWN };

        // Image index to define which part of the object is. This index corresponds to an index in ICN objects storing multiple sprites (images).
        uint8_t _imageIndex{ 255 };

        // An indicator where the object has extra animation frames on Adventure Map.
        bool _hasObjectAnimation{ false };

        // An indicator that this tile is a road. Logically it shouldn't be set for addons.
        bool _isMarkedAsRoad{ false };
    };

    using Addons = std::list<TilesAddon>;

    class Tiles
    {
    public:
        Tiles() = default;

        bool operator==( const Tiles & tile ) const
        {
            return ( _addonBottomLayer == tile._addonBottomLayer ) && ( _addonTopLayer == tile._addonTopLayer ) && ( _index == tile._index )
                   && ( _terrainImageIndex == tile._terrainImageIndex ) && ( _terrainFlags == tile._terrainFlags ) && ( _uid == tile._uid )
                   && ( _layerType == tile._layerType ) && ( _objectIcnType == tile._objectIcnType ) && ( _imageIndex == tile._imageIndex )
                   && ( _hasObjectAnimation == tile._hasObjectAnimation ) && ( _isMarkedAsRoad == tile._isMarkedAsRoad )
                   && ( _mainObjectType == tile._mainObjectType ) && ( _metadata == tile._metadata ) && ( _tilePassabilityDirections == tile._tilePassabilityDirections )
                   && ( _isTileMarkedAsRoad == tile._isTileMarkedAsRoad ) && ( _occupantHeroId == tile._occupantHeroId );
        }

        bool operator!=( const Tiles & tile ) const
        {
            return !operator==( tile );
        }

        void Init( int32_t index, const MP2::mp2tile_t & mp2 );

        int32_t GetIndex() const
        {
            return _index;
        }

        fheroes2::Point GetCenter() const;

        MP2::MapObjectType GetObject( bool ignoreObjectUnderHero = true ) const;

        MP2::ObjectIcnType getObjectIcnType() const
        {
            return _objectIcnType;
        }

        void setObjectIcnType( const MP2::ObjectIcnType type )
        {
            _objectIcnType = type;
        }

        uint8_t GetObjectSpriteIndex() const
        {
            return _imageIndex;
        }

        void setObjectSpriteIndex( const uint8_t index )
        {
            _imageIndex = index;
        }

        uint32_t GetObjectUID() const
        {
            return _uid;
        }

        void setObjectUID( const uint32_t uid )
        {
            _uid = uid;
        }

        uint8_t getLayerType() const
        {
            return _layerType;
        }

        uint16_t GetPassable() const
        {
            return _tilePassabilityDirections;
        }

        void resetPassability()
        {
            _tilePassabilityDirections = DIRECTION_ALL;
        }

        int GetGround() const
        {
            return Ground::getGroundByImageIndex( _terrainImageIndex );
        }

        bool isWater() const
        {
            return Ground::getGroundByImageIndex( _terrainImageIndex ) == Ground::WATER;
        }

        bool isSameMainObject( const MP2::MapObjectType objectType ) const
        {
            return objectType == _mainObjectType;
        }

        bool hasSpriteAnimation() const
        {
            return _hasObjectAnimation;
        }

        // Checks whether it is possible to move into this tile from the specified direction
        bool isPassableFrom( const int direction ) const
        {
            return ( direction & _tilePassabilityDirections ) != 0;
        }

        // Checks whether it is possible to move into this tile from the specified direction under the specified conditions
        bool isPassableFrom( const int direction, const bool fromWater, const bool skipFog, const int heroColor ) const;

        // Checks whether it is possible to exit this tile in the specified direction
        bool isPassableTo( const int direction ) const
        {
            return ( direction & _tilePassabilityDirections ) != 0;
        }

        bool isRoad() const
        {
            return _isTileMarkedAsRoad || _mainObjectType == MP2::OBJ_CASTLE;
        }

        bool isStream() const;
        bool isShadow() const;
        bool GoodForUltimateArtifact() const;

        TilesAddon * FindAddonLevel1( uint32_t uniq1 );
        TilesAddon * FindAddonLevel2( uint32_t uniq2 );

        void SetObject( const MP2::MapObjectType objectType );

        void SetIndex( const uint32_t index )
        {
            _index = index;
        }

        void resetBoatOwnerColor()
        {
            _boatOwnerColor = Color::NONE;
        }

        int getBoatOwnerColor() const
        {
            return _boatOwnerColor;
        }

        void setBoat( const int direction, const int color );
        int getBoatDirection() const;

        void resetObjectSprite()
        {
            _objectIcnType = MP2::OBJ_ICN_TYPE_UNKNOWN;
            _imageIndex = 255;
        }

        void FixObject();

        uint32_t GetRegion() const
        {
            return _region;
        }

        void UpdateRegion( uint32_t newRegionID );

        // Set initial passability based on information read from mp2 and addon structures.
        void setInitialPassability();

        // Update passability based on neighbours around.
        void updatePassability();

        int getOriginalPassability() const;

        bool isClearGround() const;

        bool doesObjectExist( const uint32_t uid ) const;

        void setOwnershipFlag( const MP2::MapObjectType objectType, const int color );

        void removeOwnershipFlag( const MP2::MapObjectType objectType );

        // Determine the fog direction in the area between min and max positions for given player(s) color code and store it in corresponding tile data.
        static void updateFogDirectionsInArea( const fheroes2::Point & minPos, const fheroes2::Point & maxPos, const int32_t color );

        // Return fog direction of tile. A tile without fog returns "Direction::UNKNOWN".
        uint16_t getFogDirection() const
        {
            return _fogDirection;
        }

        void pushBottomLayerAddon( const MP2::mp2tile_t & mt );
        void pushBottomLayerAddon( const MP2::mp2addon_t & ma );

        void pushBottomLayerAddon( TilesAddon ta )
        {
            _addonBottomLayer.emplace_back( ta );
        }

        void pushTopLayerAddon( const MP2::mp2tile_t & mt );
        void pushTopLayerAddon( const MP2::mp2addon_t & ma );

        const Addons & getBottomLayerAddons() const
        {
            return _addonBottomLayer;
        }

        Addons & getBottomLayerAddons()
        {
            return _addonBottomLayer;
        }

        const Addons & getTopLayerAddons() const
        {
            return _addonTopLayer;
        }

        void AddonsSort();
        void Remove( uint32_t uniqID );

        void updateObjectImageIndex( const uint32_t objectUid, const MP2::ObjectIcnType objectIcnType, const int imageIndexOffset );
        void replaceObject( const uint32_t objectUid, const MP2::ObjectIcnType originalObjectIcnType, const MP2::ObjectIcnType newObjectIcnType,
                            const uint8_t originalImageIndex, const uint8_t newImageIndex );

        std::string String() const;

        bool isFog( const int colors ) const
        {
            // colors may be the union friends
            return ( _fogColors & colors ) == colors;
        }

        void ClearFog( const int colors );

        void SetObjectPassable( bool pass );

        const std::array<uint32_t, 3> & metadata() const
        {
            return _metadata;
        }

        std::array<uint32_t, 3> & metadata()
        {
            return _metadata;
        }

        uint8_t getTerrainFlags() const
        {
            return _terrainFlags;
        }

        uint16_t getTerrainImageIndex() const
        {
            return _terrainImageIndex;
        }

        void setTerrain( const uint16_t terrainImageIndex, const bool horizontalFlip, const bool verticalFlip )
        {
            _terrainImageIndex = terrainImageIndex;
            _terrainFlags = ( verticalFlip ? 1 : 0 ) + ( horizontalFlip ? 2 : 0 );
        }

        Heroes * getHero() const;
        void setHero( Heroes * hero );

        // If tile is empty (MP2::OBJ_NONE) then verify whether it is a coast and update the tile if needed.
        void updateEmpty();

        // Set tile to coast MP2::OBJ_COAST) if it's near water or to empty (MP2::OBJ_NONE)
        void setAsEmpty();

        uint32_t getObjectIdByObjectIcnType( const MP2::ObjectIcnType objectIcnType ) const;

        std::vector<MP2::ObjectIcnType> getValidObjectIcnTypes() const;

        bool containsAnyObjectIcnType( const std::vector<MP2::ObjectIcnType> & objectIcnTypes ) const;

        bool containsSprite( const MP2::ObjectIcnType objectIcnType, const uint32_t imageIdx ) const;

        // Some tiles have incorrect object type. This is due to original Editor issues.
        static void fixTileObjectType( Tiles & tile );

        static int32_t getIndexOfMainTile( const Maps::Tiles & tile );

        void swap( TilesAddon & addon ) noexcept;

        static void updateTileById( Maps::Tiles & tile, const uint32_t uid, const uint8_t newIndex );

        // The old code was using weird quantity based values which were very hard to understand.
        // Since we must have backwards compatibility we need to do the conversion.
        void quantityIntoMetadata( const uint8_t quantityValue1, const uint8_t quantityValue2, const uint32_t additionalMetadata );

        // The old code stored an unknown artifact ID as 103. This prevented from adding new artifacts without breaking compatibility every time we do such.
        // This method serves to fix incorrect artifact IDs.
        void fixOldArtifactIDs();

    private:
        TilesAddon * getAddonWithFlag( const uint32_t uid );

        // Set or remove a flag which belongs to UID of the object.
        void updateFlag( const int color, const uint8_t objectSpriteIndex, const uint32_t uid, const bool setOnUpperLayer );

        bool isTallObject() const;

        bool isDetachedObject() const;

        void _setFogDirection( const uint16_t fogDirection )
        {
            _fogDirection = fogDirection;
        }

        friend StreamBase & operator<<( StreamBase &, const Tiles & );
        friend StreamBase & operator>>( StreamBase &, Tiles & );

        static uint8_t convertOldMainObjectType( const uint8_t mainObjectType );

        // The following members are used in the Editor and in the game.

        Addons _addonBottomLayer;

        Addons _addonTopLayer;

        int32_t _index{ 0 };

        uint16_t _terrainImageIndex{ 0 };

        uint8_t _terrainFlags{ 0 };

        // Unique identifier of an object. UID can be shared among multiple object parts if an object is bigger than 1 tile.
        uint32_t _uid{ 0 };

        // Layer type shows how the object is rendered on Adventure Map. See ObjectLayerType enumeration.
        uint8_t _layerType{ OBJECT_LAYER };

        // The type of object which correlates to ICN id. See MP2::getIcnIdFromObjectIcnType() function for more details.
        MP2::ObjectIcnType _objectIcnType{ MP2::OBJ_ICN_TYPE_UNKNOWN };

        // Image index to define which part of the object is. This index corresponds to an index in ICN objects storing multiple sprites (images).
        uint8_t _imageIndex{ 255 };

        // An indicator where the object has extra animation frames on Adventure Map.
        bool _hasObjectAnimation{ false };

        // An indicator that this tile is a road. Logically it shouldn't be set for add-ons.
        bool _isMarkedAsRoad{ false };

        MP2::MapObjectType _mainObjectType{ MP2::OBJ_NONE };

        std::array<uint32_t, 3> _metadata{ 0 };

        uint16_t _tilePassabilityDirections{ DIRECTION_ALL };

        bool _isTileMarkedAsRoad{ false };

        // This member holds Hero ID that is incremented by 1.
        // TODO: once hero IDs are going to be updated change the logic for this member as well.
        uint8_t _occupantHeroId{ 0 };

        // The following members are only used in the game.

        uint8_t _fogColors{ Color::ALL };

        // Fog direction to render fog in Game Area.
        uint16_t _fogDirection{ DIRECTION_ALL };

        // Heroes can only summon neutral empty boats or empty boats belonging to their kingdom.
        uint8_t _boatOwnerColor{ Color::NONE };

        // This field does not persist in savegame.
        uint32_t _region{ REGION_NODE_BLOCKED };
    };

    StreamBase & operator<<( StreamBase & msg, const TilesAddon & ta );
    StreamBase & operator<<( StreamBase & msg, const Tiles & tile );
    StreamBase & operator>>( StreamBase & msg, TilesAddon & ta );
    StreamBase & operator>>( StreamBase & msg, Tiles & tile );
}

#endif
