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

#include <cstdint>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "army_troop.h"
#include "artifact.h"
#include "color.h"
#include "direction.h"
#include "math_base.h"
#include "mp2.h"
#include "pairs.h"
#include "resource.h"
#include "skill.h"
#include "world_regions.h"

class Heroes;
class Monster;
class Spell;
class StreamBase;

namespace fheroes2
{
    class Image;
    struct ObjectRenderingInfo;
}

namespace Interface
{
    class GameArea;
}

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

        void Init( int32_t, const MP2::mp2tile_t & );

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

        uint8_t GetObjectSpriteIndex() const
        {
            return _imageIndex;
        }

        uint32_t GetObjectUID() const
        {
            return _uid;
        }

        // Get Tile metadata field #1 (used for things like monster count or resource amount)
        uint8_t GetQuantity1() const
        {
            return quantity1;
        }

        // Get Tile metadata field #2 (used for things like animations or resource type )
        uint8_t GetQuantity2() const
        {
            return quantity2;
        }

        uint16_t GetPassable() const
        {
            return tilePassable;
        }

        int GetGround() const;

        bool isWater() const
        {
            return 30 > _terrainImageIndex;
        }

        const fheroes2::Image & GetTileSurface() const;

        bool isSameMainObject( const MP2::MapObjectType objectType ) const
        {
            return objectType == _mainObjectType;
        }

        bool hasSpriteAnimation() const
        {
            return _hasObjectAnimation;
        }

        // Checks whether it is possible to move into this tile from the specified direction under the specified conditions
        bool isPassableFrom( const int direction, const bool fromWater, const bool skipFog, const int heroColor ) const;

        // Checks whether it is possible to exit this tile in the specified direction
        bool isPassableTo( const int direction ) const
        {
            return ( direction & tilePassable ) != 0;
        }

        bool isRoad() const
        {
            return tileIsRoad || _mainObjectType == MP2::OBJ_CASTLE;
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

        static void RedrawEmptyTile( fheroes2::Image & dst, const fheroes2::Point & mp, const Interface::GameArea & area );
        void redrawTopLayerExtraObjects( fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area ) const;
        void redrawTopLayerObject( fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area, const TilesAddon & addon ) const;
        void updateFogDirectionsAround( const int32_t color ) const;
        void updateFogDirection( const int32_t color );
        uint16_t getFogDirection( const int32_t color ) const;
        uint16_t getFogDirection() const
        {
            return _fogDirection;
        }

        void drawFog( fheroes2::Image & dst, const Interface::GameArea & area ) const;
        void RedrawPassable( fheroes2::Image & dst, const int friendColors, const Interface::GameArea & area ) const;
        void redrawBottomLayerObjects( fheroes2::Image & dst, bool isPuzzleDraw, const Interface::GameArea & area, const uint8_t level ) const;

        void drawByObjectIcnType( fheroes2::Image & output, const Interface::GameArea & area, const MP2::ObjectIcnType objectIcnType ) const;

        std::vector<fheroes2::ObjectRenderingInfo> getMonsterSpritesPerTile() const;
        std::vector<fheroes2::ObjectRenderingInfo> getMonsterShadowSpritesPerTile() const;
        std::vector<fheroes2::ObjectRenderingInfo> getBoatSpritesPerTile() const;
        std::vector<fheroes2::ObjectRenderingInfo> getBoatShadowSpritesPerTile() const;
        std::vector<fheroes2::ObjectRenderingInfo> getMineGuardianSpritesPerTile() const;

        void AddonsPushLevel1( const MP2::mp2tile_t & mt );
        void AddonsPushLevel1( const MP2::mp2addon_t & ma );

        void AddonsPushLevel1( TilesAddon ta )
        {
            addons_level1.emplace_back( ta );
        }

        void AddonsPushLevel2( const MP2::mp2tile_t & mt );
        void AddonsPushLevel2( const MP2::mp2addon_t & ma );

        const Addons & getLevel1Addons() const
        {
            return addons_level1;
        }

        const Addons & getLevel2Addons() const
        {
            return addons_level2;
        }

        void AddonsSort();
        void Remove( uint32_t uniqID );
        void RemoveObjectSprite();
        void updateObjectImageIndex( const uint32_t objectUid, const MP2::ObjectIcnType objectIcnType, const int imageIndexOffset );
        void replaceObject( const uint32_t objectUid, const MP2::ObjectIcnType originalObjectIcnType, const MP2::ObjectIcnType newObjectIcnType,
                            const uint8_t originalImageIndex, const uint8_t newImageIndex );

        std::string String() const;

        bool isFog( const int colors ) const
        {
            // colors may be the union friends
            return ( _fogColors & colors ) == colors;
        }

        bool isFogAllAround( const int color ) const;
        void ClearFog( const int colors );

        void MonsterSetCount( uint32_t count );
        uint32_t MonsterCount() const;

        // Checks whether the object to be captured is guarded by its own forces
        // (castle has a hero or garrison, dwelling has creatures, etc)
        bool isCaptureObjectProtected() const;

        // Operations with tile quantities
        void QuantityUpdate( bool isFirstLoad = true );
        void QuantityReset();
        bool QuantityIsValid() const;
        void QuantitySetColor( int );
        int QuantityVariant() const;
        int QuantityExt() const;
        int QuantityColor() const;
        uint32_t QuantityGold() const;
        Spell QuantitySpell() const;
        Skill::Secondary QuantitySkill() const;
        Artifact QuantityArtifact() const;
        ResourceCount QuantityResourceCount() const;
        Funds QuantityFunds() const;
        Monster QuantityMonster() const;
        Troop QuantityTroop() const;

        void SetObjectPassable( bool );

        // Get additional metadata.
        int32_t getAdditionalMetadata() const
        {
            return additionalMetadata;
        }

        // Set Tile additional metadata field.
        void setAdditionalMetadata( const uint32_t value )
        {
            additionalMetadata = value;
        }

        void clearAdditionalMetadata()
        {
            additionalMetadata = 0;
        }

        Heroes * GetHeroes() const;
        void SetHeroes( Heroes * );

        // If tile is empty (MP2::OBJ_NONE) then verify whether it is a coast and update the tile if needed.
        void updateEmpty();

        // Set tile to coast MP2::OBJ_COAST) if it's near water or to empty (MP2::OBJ_NONE)
        void setAsEmpty();

        uint32_t getObjectIdByObjectIcnType( const MP2::ObjectIcnType objectIcnType ) const;

        std::vector<MP2::ObjectIcnType> getValidObjectIcnTypes() const;

        bool containsAnyObjectIcnType( const std::vector<MP2::ObjectIcnType> & objectIcnTypes ) const;

        bool containsSprite( const MP2::ObjectIcnType objectIcnType, const uint32_t imageIdx ) const;

        static int getColorFromBarrierSprite( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex );
        static int getColorFromTravellerTentSprite( const MP2::ObjectIcnType objectIcnType, const uint8_t icnIndex );

        static void UpdateAbandonedMineLeftSprite( MP2::ObjectIcnType & objectIcnType, uint8_t & imageIndex, const int resource );

        static void UpdateAbandonedMineRightSprite( MP2::ObjectIcnType & objectIcnType, uint8_t & imageIndex );

        static std::pair<int, int> ColorRaceFromHeroSprite( const uint32_t heroSpriteIndex );
        static std::pair<uint32_t, uint32_t> GetMonsterSpriteIndices( const Tiles & tile, const uint32_t monsterIndex );
        static void PlaceMonsterOnTile( Tiles & tile, const Monster & mons, const uint32_t count );
        static void UpdateAbandonedMineSprite( Tiles & tile );

        // Some tiles have incorrect object type. This is due to original Editor issues.
        static void fixTileObjectType( Tiles & tile );

        static int32_t getIndexOfMainTile( const Maps::Tiles & tile );

    private:
        TilesAddon * getAddonWithFlag( const uint32_t uid );

        // Set or remove a flag which belongs to UID of the object.
        void updateFlag( const int color, const uint8_t objectSpriteIndex, const uint32_t uid, const bool setOnUpperLayer );
        void RemoveJailSprite();

        void QuantitySetVariant( int );
        void QuantitySetExt( int );
        void QuantitySetSkill( int );
        void QuantitySetSpell( int );
        void QuantitySetArtifact( int );
        void QuantitySetResource( int, uint32_t );

        bool isTallObject() const;

        bool isDetachedObject() const;

        static void UpdateMonsterInfo( Tiles & );
        static void UpdateDwellingPopulation( Tiles & tile, bool isFirstLoad );
        static void UpdateMonsterPopulation( Tiles & );
        static void UpdateRNDArtifactSprite( Tiles & );
        static void UpdateRNDResourceSprite( Tiles & );

        static void updateTileById( Maps::Tiles & tile, const uint32_t uid, const uint8_t newIndex );

        friend StreamBase & operator<<( StreamBase &, const Tiles & );
        friend StreamBase & operator>>( StreamBase &, Tiles & );

        friend bool operator<( const Tiles & l, const Tiles & r )
        {
            return l.GetIndex() < r.GetIndex();
        }

        static void renderAddonObject( fheroes2::Image & output, const Interface::GameArea & area, const fheroes2::Point & offset, const TilesAddon & addon );
        void renderMainObject( fheroes2::Image & output, const Interface::GameArea & area, const fheroes2::Point & offset ) const;

        Addons addons_level1; // bottom layer
        Addons addons_level2; // top layer

        int32_t _index = 0;

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

        // An indicator that this tile is a road. Logically it shouldn't be set for addons.
        bool _isMarkedAsRoad{ false };

        MP2::MapObjectType _mainObjectType{ MP2::OBJ_NONE };
        uint16_t tilePassable = DIRECTION_ALL;
        uint8_t _fogColors = Color::ALL;
        uint16_t _fogDirection{ DIRECTION_ALL };

        uint8_t heroID = 0;

        // TODO: Combine quantity1 and quantity2 into a single 16/32-bit variable except first 2 bits of quantity1 which are used for level type of an object.
        uint8_t quantity1 = 0;
        uint8_t quantity2 = 0;

        // Additional metadata is not set from map's information but during runtime like spells or monster joining conditions.
        int32_t additionalMetadata = 0;

        bool tileIsRoad = false;

        // Heroes can only summon neutral empty boats or empty boats belonging to their kingdom.
        uint8_t _boatOwnerColor = Color::NONE;

        // This field does not persist in savegame.
        uint32_t _region = REGION_NODE_BLOCKED;
    };

    StreamBase & operator<<( StreamBase &, const TilesAddon & );
    StreamBase & operator<<( StreamBase &, const Tiles & );
    StreamBase & operator>>( StreamBase &, TilesAddon & );
    StreamBase & operator>>( StreamBase &, Tiles & );

    // In order to keep class Tiles small enough these helper functions exist.
    // If you want to add a new method to the class and this is not a genetic one you must create a function instead.

    void setSpellOnTile( Tiles & tile, const int32_t spellId );
    int32_t getSpellIdFromTile( const Tiles & tile );

    void setMonsterOnTileJoinCondition( Tiles & tile, const int32_t condition );
    bool isMonsterOnTileJoinConditionSkip( const Tiles & tile );
    bool isMonsterOnTileJoinConditionFree( const Tiles & tile );
}

#endif
