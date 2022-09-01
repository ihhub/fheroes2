/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#include <list>

#include "army_troop.h"
#include "artifact.h"
#include "color.h"
#include "direction.h"
#include "mp2.h"
#include "resource.h"
#include "skill.h"
#include "world_regions.h"

class Heroes;
class Spell;
class Monster;

namespace MP2
{
    struct mp2tile_t;
    struct mp2addon_t;
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
        TilesAddon();
        TilesAddon( const uint8_t lv, const uint32_t uid, const uint8_t obj, const uint32_t index_ );

        TilesAddon( const TilesAddon & ) = default;

        ~TilesAddon() = default;

        TilesAddon & operator=( const TilesAddon & ) = delete;

        bool isUniq( const uint32_t id ) const
        {
            return uniq == id;
        }

        bool isRoad() const;

        bool hasSpriteAnimation() const
        {
            return object & 1;
        }

        std::string String( int level ) const;

        static bool isShadow( const TilesAddon & ta );

        static bool isResource( const TilesAddon & ta );
        static bool isArtifact( const TilesAddon & ta );

        static bool PredicateSortRules1( const TilesAddon & ta1, const TilesAddon & ta2 );

        uint32_t uniq;
        uint8_t level;
        uint8_t object;
        uint8_t index;
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

        uint8_t GetObjectTileset() const
        {
            return objectTileset;
        }

        uint8_t GetObjectSpriteIndex() const
        {
            return objectIndex;
        }

        uint32_t GetObjectUID() const
        {
            return uniq;
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
            return 30 > TileSpriteIndex();
        }

        uint32_t TileSpriteIndex() const
        {
            return pack_sprite_index & 0x3FFF;
        }

        uint32_t TileSpriteShape() const
        {
            return pack_sprite_index >> 14;
        }

        const fheroes2::Image & GetTileSurface() const;

        bool isObject( const MP2::MapObjectType objectType ) const
        {
            return objectType == mp2_object;
        }

        bool hasSpriteAnimation() const
        {
            return objectTileset & 1;
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
            return tileIsRoad || mp2_object == MP2::OBJ_CASTLE;
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

        void setBoat( int direction );
        int getBoatDirection() const;

        void resetObjectSprite()
        {
            objectTileset = 0;
            objectIndex = 255;
        }

        void FixObject();

        uint32_t GetRegion() const
        {
            return _region;
        }

        void UpdateRegion( uint32_t newRegionID );

        // Set initial passability based on information read from mp2 and addon structures.
        void setInitialPassability();

        // Update passability based on neigbhours around.
        void updatePassability();

        int getOriginalPassability() const;

        bool isClearGround() const;

        bool doesObjectExist( const uint32_t uid ) const;

        void setOwnershipFlag( const MP2::MapObjectType objectType, const int color );

        void correctOldSaveOwnershipFlag();
        void correctDiggingHoles();

        void removeOwnershipFlag( const MP2::MapObjectType objectType );

        static void RedrawEmptyTile( fheroes2::Image & dst, const fheroes2::Point & mp, const Interface::GameArea & area );
        void redrawTopLayerExtraObjects( fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area ) const;
        void redrawTopLayerObject( fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area, const TilesAddon & addon ) const;
        int GetFogDirections( int color ) const;

        void drawFog( fheroes2::Image & dst, int color, const Interface::GameArea & area ) const;
        void RedrawPassable( fheroes2::Image & dst, const Interface::GameArea & area ) const;
        void redrawBottomLayerObjects( fheroes2::Image & dst, bool isPuzzleDraw, const Interface::GameArea & area, const uint8_t level ) const;

        void drawByIcnId( fheroes2::Image & output, const Interface::GameArea & area, const int32_t icnId ) const;

        std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> getMonsterSpritesPerTile() const;
        std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> getMonsterShadowSpritesPerTile() const;
        std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> getBoatSpritesPerTile() const;
        std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> getBoatShadowSpritesPerTile() const;
        std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> getMineGuardianSpritesPerTile() const;

        void AddonsPushLevel1( const MP2::mp2tile_t & );
        void AddonsPushLevel1( const MP2::mp2addon_t & );
        void AddonsPushLevel1( const TilesAddon & );
        void AddonsPushLevel2( const MP2::mp2tile_t & );
        void AddonsPushLevel2( const MP2::mp2addon_t & );

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
        void UpdateObjectSprite( uint32_t uniqID, uint8_t rawTileset, uint8_t newTileset, int indexChange );
        void ReplaceObjectSprite( uint32_t uniqID, uint8_t rawTileset, uint8_t newTileset, uint8_t indexToReplace, uint8_t newIndex );

        std::string String() const;

        bool isFog( const int colors ) const
        {
            // colors may be the union friends
            return ( fog_colors & colors ) == colors;
        }

        bool isFogAllAround( const int color ) const;

        void ClearFog( int colors )
        {
            fog_colors &= ~colors;
        }

        /* monster operation */
        void MonsterSetCount( uint32_t count );
        uint32_t MonsterCount() const;

        // Checks whether the object to be captured is guarded by its own forces
        // (castle has a hero or garrison, dwelling has creatures, etc)
        bool isCaptureObjectProtected() const;

        /* object quantity operation */
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

        // If tile is empty (MP2::OBJ_ZERO) then verify whether it is a coast and update the tile if needed.
        void updateEmpty();

        // Set tile to coast MP2::OBJ_COAST) if it's near water or to empty (MP2::OBJ_ZERO)
        void setAsEmpty();

        uint32_t getObjectIdByICNType( const int icnId ) const;

        std::vector<uint8_t> getValidTileSets() const;

        bool containsTileSet( const std::vector<uint8_t> & tileSets ) const;

        bool containsSprite( uint8_t tileSetId, const uint32_t objectIdx ) const;

        static int ColorFromBarrierSprite( const uint8_t tileset, const uint8_t icnIndex );
        static int ColorFromTravellerTentSprite( const uint8_t tileset, const uint8_t icnIndex );
        static MP2::MapObjectType GetLoyaltyObject( const uint8_t tileset, const uint8_t icnIndex );
        static bool isShadowSprite( const uint8_t tileset, const uint8_t icnIndex );
        static bool isShadowSprite( const int tileset, const uint8_t icnIndex );
        static void UpdateAbandonedMineLeftSprite( uint8_t & tileset, uint8_t & index, const int resource );
        static void UpdateAbandonedMineRightSprite( uint8_t & tileset, uint8_t & index );
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

        void SetTerrain( uint32_t sprite_index, uint32_t shape /* 0: none, 1 : vert, 2: horz, 3: both */ );

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

        static bool removeOldFlag( Addons & addons, const uint8_t startIndex, int & ownerColor );

        Addons addons_level1; // bottom layer
        Addons addons_level2; // top layer

        int32_t _index = 0;
        uint16_t pack_sprite_index = 0;

        uint32_t uniq = 0;
        uint8_t objectTileset = 0;
        uint8_t objectIndex = 255;
        MP2::MapObjectType mp2_object = MP2::OBJ_ZERO;
        uint16_t tilePassable = DIRECTION_ALL;
        uint8_t fog_colors = Color::ALL;

        uint8_t heroID = 0;

        // TODO: Combine quantity1 and quantity2 into a single 16/32-bit variable except first 2 bits of quantity1 which are used for level type of an object.
        uint8_t quantity1 = 0;
        uint8_t quantity2 = 0;

        // Additional metadata is not set from map's information but during runtime like spells or monster joining conditions.
        int32_t additionalMetadata = 0;

        bool tileIsRoad = false;

        // This field does not persist in savegame.
        uint32_t _region = REGION_NODE_BLOCKED;

        uint8_t _level = 0;
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
