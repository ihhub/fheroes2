/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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
    bool SkipRedrawTileBottom4Hero( const uint8_t tileset, const uint8_t icnIndex, const int passable );
}

namespace Maps
{
    struct TilesAddon
    {
        enum level_t : uint8_t
        {
            GROUND = 0,
            DOWN = 1,
            SHADOW = 2,
            UPPER = 3
        };

        TilesAddon();
        TilesAddon( const uint8_t lv, const uint32_t uid, const uint8_t obj, const uint32_t index_ );
        TilesAddon( const TilesAddon & ta );

        ~TilesAddon() = default;

        TilesAddon & operator=( const TilesAddon & ta ) = delete;

        bool isUniq( const uint32_t id ) const
        {
            return uniq == id;
        }

        bool isRoad() const;
        bool hasSpriteAnimation() const;

        std::string String( int level ) const;

        static bool isShadow( const TilesAddon & );

        static bool isResource( const TilesAddon & );
        static bool isArtifact( const TilesAddon & );
        static bool isFlag32( const TilesAddon & );

        static bool PredicateSortRules1( const TilesAddon &, const TilesAddon & );

        uint32_t uniq;
        uint8_t level;
        uint8_t object;
        uint8_t index;
    };

    struct Addons : public std::list<TilesAddon>
    {
        void Remove( u32 uniq );
    };

    class Tiles
    {
    public:
        Tiles();

        void Init( s32, const MP2::mp2tile_t & );

        int32_t GetIndex() const
        {
            return _index;
        }

        fheroes2::Point GetCenter( void ) const;
        MP2::MapObjectType GetObject( bool ignoreObjectUnderHero = true ) const;
        uint8_t GetObjectTileset() const;

        uint8_t GetObjectSpriteIndex() const;

        u32 GetObjectUID() const;

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

        // Get third field containing Tile metadata (adventure spell ID)
        uint8_t GetQuantity3() const
        {
            return quantity3;
        }

        int GetPassable() const;
        int GetGround() const;
        bool isWater() const;

        u32 TileSpriteIndex( void ) const;
        u32 TileSpriteShape( void ) const;

        const fheroes2::Image & GetTileSurface( void ) const;

        bool isObject( const MP2::MapObjectType objectType ) const;
        bool hasSpriteAnimation() const;
        bool validateWaterRules( bool fromWater ) const;
        bool isPassable( int direct, bool fromWater, bool skipfog, const int heroColor ) const;
        bool isRoad() const;
        bool isStream( void ) const;
        bool isShadow() const;
        bool GoodForUltimateArtifact() const;

        TilesAddon * FindAddonLevel1( u32 uniq1 );
        TilesAddon * FindAddonLevel2( u32 uniq2 );

        void SetTile( u32 sprite_index, u32 shape /* 0: none, 1 : vert, 2: horz, 3: both */ );
        void SetObject( const MP2::MapObjectType objectType );

        void SetIndex( const uint32_t index )
        {
            _index = index;
        }

        void setBoat( int direction );
        int getBoatDirection() const;
        void resetObjectSprite();

        void FixObject( void );

        uint32_t GetRegion() const;
        void UpdateRegion( uint32_t newRegionID );

        // Set initial passability based on information read from mp2 and addon structures.
        void setInitialPassability();

        // Update passability based on neigbhours around.
        void updatePassability();

        int getOriginalPassability() const;

        bool isClearGround() const;

        bool doesObjectExist( const uint32_t uid ) const;

        // ICN::FLAGS32 version
        void CaptureFlags32( const MP2::MapObjectType objectType, int col );

        // Removes all ICN::FLAGS32 objects from this tile.
        void removeFlags();

        void RedrawTile( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const;
        static void RedrawEmptyTile( fheroes2::Image & dst, const fheroes2::Point & mp, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area );
        void RedrawBottom( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, bool isPuzzleDraw, const Interface::GameArea & area ) const;
        void RedrawBottom4Hero( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const;
        void RedrawTop( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const bool isPuzzleDraw, const Interface::GameArea & area ) const;
        void RedrawTopFromBottom( fheroes2::Image & dst, const Interface::GameArea & area ) const;
        void RedrawTop4Hero( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, bool skip_ground, const Interface::GameArea & area ) const;
        void RedrawObjects( fheroes2::Image & dst, bool isPuzzleDraw, const Interface::GameArea & area ) const;
        void RedrawBoat( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const;
        void RedrawBoatShadow( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const;
        void RedrawMonster( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const;
        int GetFogDirections( int color ) const;
        void RedrawFogs( fheroes2::Image & dst, int color, const Interface::GameArea & area ) const;
        void RedrawAddon( fheroes2::Image & dst, const Addons & addon, const fheroes2::Rect & visibleTileROI, bool isPuzzleDraw, const Interface::GameArea & area ) const;
        void RedrawPassable( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const;

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

        void AddonsSort( void );
        void Remove( u32 uniqID );
        void RemoveObjectSprite( void );
        void UpdateObjectSprite( uint32_t uniqID, uint8_t rawTileset, uint8_t newTileset, int indexChange );
        void ReplaceObjectSprite( uint32_t uniqID, uint8_t rawTileset, uint8_t newTileset, uint8_t indexToReplace, uint8_t newIndex );

        std::string String( void ) const;

        bool isFog( const int colors ) const
        {
            // colors may be the union friends
            return ( fog_colors & colors ) == colors;
        }

        bool isFogAllAround( const int color ) const;
        void ClearFog( int color );

        /* monster operation */
        bool MonsterJoinConditionSkip( void ) const;
        bool MonsterJoinConditionFree( void ) const;
        int MonsterJoinCondition( void ) const;
        void MonsterSetJoinCondition( int );
        void MonsterSetFixedCount( void );
        bool MonsterFixedCount( void ) const;
        void MonsterSetCount( u32 count );
        u32 MonsterCount( void ) const;

        bool CaptureObjectIsProtection( void ) const;

        /* object quantity operation */
        void QuantityUpdate( bool isFirstLoad = true );
        void QuantityReset( void );
        bool QuantityIsValid( void ) const;
        void QuantitySetColor( int );
        int QuantityVariant( void ) const;
        int QuantityExt( void ) const;
        int QuantityColor( void ) const;
        u32 QuantityGold( void ) const;
        Spell QuantitySpell( void ) const;
        Skill::Secondary QuantitySkill( void ) const;
        Artifact QuantityArtifact( void ) const;
        ResourceCount QuantityResourceCount( void ) const;
        Funds QuantityFunds( void ) const;
        Monster QuantityMonster( void ) const;
        Troop QuantityTroop( void ) const;

        void SetObjectPassable( bool );

        // Set Tile metadata field (used for things like adventure spell ID)
        void SetQuantity3( const uint8_t value )
        {
            quantity3 = value;
        }

        Heroes * GetHeroes( void ) const;
        void SetHeroes( Heroes * );

        // If tile is empty (MP2::OBJ_ZERO) then verify whether it is a coast and update the tile if needed.
        void updateEmpty();

        // Set tile to coast MP2::OBJ_COAST) if it's near water or to empty (MP2::OBJ_ZERO)
        void setAsEmpty();

        uint32_t getObjectIdByICNType( const int icnId ) const;

        std::vector<uint8_t> getValidTileSets() const;

        bool containsTileSet( const std::vector<uint8_t> & tileSets ) const;

        static int ColorFromBarrierSprite( const uint8_t tileset, const uint8_t icnIndex );
        static int ColorFromTravellerTentSprite( const uint8_t tileset, const uint8_t icnIndex );
        static MP2::MapObjectType GetLoyaltyObject( const uint8_t tileset, const uint8_t icnIndex );
        static bool isShadowSprite( const uint8_t tileset, const uint8_t icnIndex );
        static bool isShadowSprite( const int tileset, const uint8_t icnIndex );
        static void UpdateAbandoneMineLeftSprite( uint8_t & tileset, uint8_t & index, const int resource );
        static void UpdateAbandoneMineRightSprite( uint8_t & tileset, uint8_t & index );
        static std::pair<int, int> ColorRaceFromHeroSprite( const uint32_t heroSpriteIndex );
        static std::pair<uint32_t, uint32_t> GetMonsterSpriteIndices( const Tiles & tile, const uint32_t monsterIndex );
        static void PlaceMonsterOnTile( Tiles & tile, const Monster & mons, const uint32_t count );
        static void UpdateAbandoneMineSprite( Tiles & tile );

        // Some tiles have incorrect object type. This is due to original Editor issues.
        static void fixTileObjectType( Tiles & tile );

        static int32_t getIndexOfMainTile( const Maps::Tiles & tile );

    private:
        TilesAddon * FindFlags( void );

        // correct flags, ICN::FLAGS32 vesion
        void CorrectFlags32( const int col, const u32 index, const bool up );
        void RemoveJailSprite( void );

        void QuantitySetVariant( int );
        void QuantitySetExt( int );
        void QuantitySetSkill( int );
        void QuantitySetSpell( int );
        void QuantitySetArtifact( int );
        void QuantitySetResource( int, u32 );

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

        Addons addons_level1;
        Addons addons_level2; // 16

        int32_t _index = 0;
        uint16_t pack_sprite_index = 0;

        uint32_t uniq = 0;
        uint8_t objectTileset = 0;
        uint8_t objectIndex = 255;
        uint8_t mp2_object = 0;
        uint16_t tilePassable = DIRECTION_ALL;
        uint8_t fog_colors = Color::ALL;

        uint8_t heroID = 0;
        uint8_t quantity1 = 0;
        uint8_t quantity2 = 0;
        uint8_t quantity3 = 0;

        bool tileIsRoad = false;

        // This field does not persist in savegame.
        uint32_t _region = 0;

        uint8_t _level = 0;
    };

    StreamBase & operator<<( StreamBase &, const TilesAddon & );
    StreamBase & operator<<( StreamBase &, const Tiles & );
    StreamBase & operator>>( StreamBase &, TilesAddon & );
    StreamBase & operator>>( StreamBase &, Tiles & );
}

#endif
