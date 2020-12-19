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

#include <functional>
#include <list>

#include "army_troop.h"
#include "artifact.h"
#include "direction.h"
#include "gamedefs.h"
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

namespace Maps
{
    struct TilesAddon
    {
        enum level_t
        {
            GROUND = 0,
            DOWN = 1,
            SHADOW = 2,
            UPPER = 3
        };

        TilesAddon();
        TilesAddon( int lv, u32 gid, int obj, u32 ii );
        TilesAddon( const TilesAddon & ta );
        TilesAddon & operator=( const TilesAddon & ta );

        bool isUniq( u32 ) const;
        bool isRoad() const;
        bool hasRoadFlag() const;
        bool isICN( int ) const;
        bool hasSpriteAnimation() const;

        std::string String( int level ) const;

        static bool isShadow( const TilesAddon & );
        static bool isRoadObject( const TilesAddon & );

        static bool isResource( const TilesAddon & );
        static bool isArtifact( const TilesAddon & );
        static bool isFlag32( const TilesAddon & );

        static bool isMounts( const TilesAddon & );
        static bool isRocs( const TilesAddon & );
        static bool isForests( const TilesAddon & );
        static bool isTrees( const TilesAddon & );
        static bool isDeadTrees( const TilesAddon & );
        static bool isCactus( const TilesAddon & );
        static bool isStump( const TilesAddon & );
        static int GetActionObject( const TilesAddon & );

        static bool PredicateSortRules1( const TilesAddon &, const TilesAddon & );
        static bool PredicateSortRules2( const TilesAddon &, const TilesAddon & );

        static bool ForceLevel1( const TilesAddon & );

        u32 uniq;
        u8 level;
        u8 object;
        u8 index;
        u8 tmp;
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

        s32 GetIndex( void ) const;
        Point GetCenter( void ) const;
        int GetObject( bool ignoreObjectUnderHero = true ) const;
        uint8_t GetObjectTileset() const;

        uint8_t GetObjectSpriteIndex() const;
        void SetObjectSpriteIndex( const uint8_t index );

        u32 GetObjectUID() const;
        int GetQuantity1() const;
        int GetQuantity2() const;
        int GetQuantity3() const;
        int GetPassable() const;
        int GetGround() const;
        bool isWater() const;

        u32 TileSpriteIndex( void ) const;
        u32 TileSpriteShape( void ) const;

        const fheroes2::Image & GetTileSurface( void ) const;

        bool isObject( int obj ) const;
        bool hasSpriteAnimation() const;
        bool validateWaterRules( bool fromWater ) const;
        bool isPassable( int direct, bool fromWater, bool skipfog ) const;
        bool isRoad() const;
        bool isStream( void ) const;
        bool isShadow( void ) const;
        bool GoodForUltimateArtifact( void ) const;

        TilesAddon * FindAddonICN( int icn1, int level = -1, int index = -1 );

        TilesAddon * FindAddonLevel1( u32 uniq1 );
        TilesAddon * FindAddonLevel2( u32 uniq2 );

        void SetTile( u32 sprite_index, u32 shape /* 0: none, 1 : vert, 2: horz, 3: both */ );
        void SetObject( int object );
        void SetIndex( int );
        void setBoat( int direction );
        int getBoatDirection() const;
        void resetObjectSprite();

        void FixObject( void );

        uint32_t GetRegion() const;
        void UpdateRegion( uint32_t newRegionID );
        void UpdatePassable( void );
        void CaptureFlags32( int obj, int col );

        void RedrawTile( fheroes2::Image & ) const;
        static void RedrawEmptyTile( fheroes2::Image & dst, const Point & mp );
        void RedrawBottom( fheroes2::Image & dst, bool isPuzzleDraw = false ) const;
        void RedrawBottom4Hero( fheroes2::Image & ) const;
        void RedrawTop( fheroes2::Image & dst ) const;
        void RedrawTop4Hero( fheroes2::Image &, bool skip_ground ) const;
        void RedrawObjects( fheroes2::Image & dst, bool isPuzzleDraw = false ) const;
        void RedrawMonstersAndBoat( fheroes2::Image & dst, bool withShadow = true ) const;
        int GetFogDirections( int color ) const;
        void RedrawFogs( fheroes2::Image &, int ) const;
        void RedrawAddon( fheroes2::Image & dst, const Addons & addon, bool isPuzzleDraw = false ) const;
        void RedrawPassable( fheroes2::Image & ) const;

        void AddonsPushLevel1( const MP2::mp2tile_t & );
        void AddonsPushLevel1( const MP2::mp2addon_t & );
        void AddonsPushLevel1( const TilesAddon & );
        void AddonsPushLevel2( const MP2::mp2tile_t & );
        void AddonsPushLevel2( const MP2::mp2addon_t & );
        void AddonsPushLevel2( const TilesAddon & );

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

        void ClearFog( int color );

        /* monster operation */
        bool MonsterJoinConditionSkip( void ) const;
        bool MonsterJoinConditionMoney( void ) const;
        bool MonsterJoinConditionFree( void ) const;
        bool MonsterJoinConditionForce( void ) const;
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
        void SetQuantity3( int value );

        Heroes * GetHeroes( void ) const;
        void SetHeroes( Heroes * );

        static int ColorFromBarrierSprite( uint8_t tileset, uint8_t icnIndex );
        static int ColorFromTravellerTentSprite( uint8_t tileset, uint8_t icnIndex );
        static int GetLoyaltyObject( uint8_t tileset, uint8_t icnIndex );
        static bool isShadowSprite( uint8_t tileset, uint8_t icnIndex );
        static void UpdateAbandoneMineLeftSprite( uint8_t & tileset, uint8_t & index, int resource );
        static void UpdateAbandoneMineRightSprite( uint8_t & tileset, uint8_t & index );
        static int GetPassable( uint32_t tileset, uint32_t index );
        static std::pair<int, int> ColorRaceFromHeroSprite( uint32_t heroSpriteIndex );
        static std::pair<int, int> GetMonsterSpriteIndices( const Tiles & tile, uint32_t monsterIndex );
        static void PlaceMonsterOnTile( Tiles &, const Monster &, u32 );
        static void UpdateAbandoneMineSprite( Tiles & );
        static void FixedPreload( Tiles & );

    private:
        TilesAddon * FindFlags( void );
        void CorrectFlags32( u32 index, bool );
        void RemoveJailSprite( void );
        bool isLongObject( int direction );

        void RedrawBoat( fheroes2::Image & dst, bool withShadow ) const;
        void RedrawMonster( fheroes2::Image & dst ) const;

        void QuantitySetVariant( int );
        void QuantitySetExt( int );
        void QuantitySetSkill( int );
        void QuantitySetSpell( int );
        void QuantitySetArtifact( int );
        void QuantitySetResource( int, u32 );

        static void UpdateMonsterInfo( Tiles & );
        static void UpdateDwellingPopulation( Tiles & tile, bool isFirstLoad );
        static void UpdateMonsterPopulation( Tiles & );
        static void UpdateRNDArtifactSprite( Tiles & );
        static void UpdateRNDResourceSprite( Tiles & );

        friend StreamBase & operator<<( StreamBase &, const Tiles & );
        friend StreamBase & operator>>( StreamBase &, Tiles & );
#ifdef WITH_XML
        friend TiXmlElement & operator>>( TiXmlElement &, Tiles & );
#endif

        Addons addons_level1;
        Addons addons_level2; // 16

        uint32_t maps_index = 0;
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

        // This field does not persist in savegame
        uint32_t _region = 0;

#ifdef WITH_DEBUG
        uint8_t impassableTileRule = 0;
#endif
    };

    StreamBase & operator<<( StreamBase &, const TilesAddon & );
    StreamBase & operator<<( StreamBase &, const Tiles & );
    StreamBase & operator>>( StreamBase &, TilesAddon & );
    StreamBase & operator>>( StreamBase &, Tiles & );
}

#endif
