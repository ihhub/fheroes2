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
#include <functional>
#include "gamedefs.h"
#include "direction.h"
#include "skill.h"
#include "artifact.h"
#include "army_troop.h"
#include "resource.h"

class Sprite;
class Heroes;
class Spell;
class Monster;

namespace  MP2
{
    struct mp2tile_t; struct mp2addon_t;
}

namespace Maps
{
    struct TilesAddon
    {
	enum level_t { GROUND = 0, DOWN = 1, SHADOW = 2, UPPER = 3 };

	TilesAddon();
	TilesAddon(int lv, u32 gid, int obj, u32 ii);

	TilesAddon & operator= (const TilesAddon & ta);

	bool isUniq(u32) const;
	bool isRoad(int) const;
	bool isICN(int) const;

	std::string String(int level) const;

	static bool isStream(const TilesAddon &);
	static bool isRoad(const TilesAddon &);

	static bool isResource(const TilesAddon &);
	static bool isWaterResource(const TilesAddon &);
	static bool isWhirlPool(const TilesAddon &);
	static bool isStandingStone(const TilesAddon &);
	static bool isArtifact(const TilesAddon &);
	static bool isCampFire(const TilesAddon &);
	static bool isMonster(const TilesAddon &);
	static bool isArtesianSpring(const TilesAddon &);
	static bool isOasis(const TilesAddon &);
	static bool isWateringHole(const TilesAddon &);
	static bool isJail(const TilesAddon &);
	static bool isMine(const TilesAddon &);
	static bool isShadow(const TilesAddon &);
	static bool isEvent(const TilesAddon &);
	static bool isBoat(const TilesAddon &);
	static bool isMiniHero(const TilesAddon &);
	static bool isRandomResource(const TilesAddon &);
	static bool isRandomArtifact(const TilesAddon &);
	static bool isRandomArtifact1(const TilesAddon &);
	static bool isRandomArtifact2(const TilesAddon &);
	static bool isRandomArtifact3(const TilesAddon &);
	static bool isUltimateArtifact(const TilesAddon &);
	static bool isCastle(const TilesAddon &);
	static bool isRandomCastle(const TilesAddon &);
	static bool isRandomMonster(const TilesAddon &);
	static bool isSkeleton(const TilesAddon &);
	static bool isSkeletonFix(const TilesAddon &);
	static bool isFlag32(const TilesAddon &);
	static bool isX_LOC123(const TilesAddon &);
	static bool isAbandoneMineSprite(const TilesAddon &);
	static bool isMounts(const TilesAddon &);
	static bool isRocs(const TilesAddon &);
	static bool isForests(const TilesAddon &);
	static bool isTrees(const TilesAddon &);
	static bool isDeadTrees(const TilesAddon &);
	static bool isCactus(const TilesAddon &);
	static bool isStump(const TilesAddon &);
	static int  GetPassable(const TilesAddon &);
	static int  GetActionObject(const TilesAddon &);
	static int  GetLoyaltyObject(const TilesAddon &);

	static bool isBarrier(const TilesAddon &);
	static int  ColorFromBarrierSprite(const TilesAddon &);
	static int  ColorFromTravellerTentSprite(const TilesAddon &);

	static std::pair<int, int>
		    ColorRaceFromHeroSprite(const TilesAddon &);

	static bool PredicateSortRules1(const TilesAddon &, const TilesAddon &);
	static bool PredicateSortRules2(const TilesAddon &, const TilesAddon &);

	static void UpdateFountainSprite(TilesAddon &);
	static void UpdateTreasureChestSprite(TilesAddon &);
	static int  UpdateStoneLightsSprite(TilesAddon &);
	static void UpdateAbandoneMineLeftSprite(TilesAddon &, int resource);
	static void UpdateAbandoneMineRightSprite(TilesAddon &);

	static bool ForceLevel1(const TilesAddon &);
	static bool ForceLevel2(const TilesAddon &);

        u32	uniq;
        u8	level;
        u8	object;
        u8	index;
	u8	tmp;
    };

    struct Addons : public std::list<TilesAddon>
    {
	void Remove(u32 uniq);
    };

    class Tiles
    {
    public:
	Tiles();

	void Init(s32, const MP2::mp2tile_t &);

	s32	GetIndex(void) const;
	Point	GetCenter(void) const;
	int	GetObject(bool skip_hero = true) const;
	u32	GetObjectUID(int obj) const;
	int	GetQuantity1(void) const{ return quantity1; }
	int	GetQuantity2(void) const{ return quantity2; }
	int	GetPassable(void) const;
	int	GetGround(void) const;
	bool	isWater(void) const;

	u32	TileSpriteIndex(void) const;
	u32	TileSpriteShape(void) const;

	Surface GetTileSurface(void) const;

	bool	isPassable(const Heroes &) const;
	bool	isPassable(const Heroes*, int direct, bool skipfog) const;
	bool	isRoad(int = DIRECTION_ALL) const;
	bool	isObject(int obj) const { return obj == mp2_object; };
	bool	isStream(void) const;
	bool	GoodForUltimateArtifact(void) const;

	TilesAddon* 		FindAddonICN1(int icn1);
	TilesAddon* 		FindAddonICN2(int icn2);

	TilesAddon* 		FindAddonLevel1(u32 uniq1);
	TilesAddon* 		FindAddonLevel2(u32 uniq2);

	TilesAddon* 		FindObject(int);
	const TilesAddon*	FindObjectConst(int) const;

	void	SetTile(u32 sprite_index, u32 shape /* 0: none, 1 : vert, 2: horz, 3: both */);
	void 	SetObject(int object);
	void	SetIndex(int);

	void	FixObject(void);

	void	UpdatePassable(void);
	void	CaptureFlags32(int obj, int col);

	void	RedrawTile(Surface &) const;
	void	RedrawBottom(Surface &, bool skip_objs = false) const;
	void 	RedrawBottom4Hero(Surface &) const;
	void 	RedrawTop(Surface &, const TilesAddon* skip = NULL) const;
	void 	RedrawTop4Hero(Surface &, bool skip_ground) const;
	void 	RedrawObjects(Surface &) const;
	void 	RedrawFogs(Surface &, int) const;
	void 	RedrawPassable(Surface &) const;

	void	AddonsPushLevel1(const MP2::mp2tile_t &);
	void	AddonsPushLevel1(const MP2::mp2addon_t &);
	void	AddonsPushLevel1(const TilesAddon &);
	void	AddonsPushLevel2(const MP2::mp2tile_t &);
	void	AddonsPushLevel2(const MP2::mp2addon_t &);
	void	AddonsPushLevel2(const TilesAddon &);

	void	AddonsSort(void);
	void	Remove(u32 uniq);
	void	RemoveObjectSprite(void);

	std::string String(void) const;
	
	bool	isFog(int color) const;
	void	ClearFog(int color);

	/* monster operation */
	bool MonsterJoinConditionSkip(void) const;
	bool MonsterJoinConditionMoney(void) const;
	bool MonsterJoinConditionFree(void) const;
	bool MonsterJoinConditionForce(void) const;
	int  MonsterJoinCondition(void) const;
	void MonsterSetJoinCondition(int);
	void MonsterSetFixedCount(void);
	bool MonsterFixedCount(void) const;
	void MonsterSetCount(u32 count);
	u32  MonsterCount(void) const;

	bool CaptureObjectIsProtection(void) const;

	/* object quantity operation */
	void		QuantityUpdate(void);
	void		QuantityReset(void);
	bool		QuantityIsValid(void) const;
	void		QuantitySetColor(int);
	int		QuantityTeleportType(void) const;
	int   		QuantityVariant(void) const;
	int  		QuantityExt(void) const;
	int  		QuantityColor(void) const;
	u32   		QuantityGold(void) const;
	Spell		QuantitySpell(void) const;
	Skill::Secondary QuantitySkill(void) const;
	Artifact	QuantityArtifact(void) const;
	ResourceCount	QuantityResourceCount(void) const;
	Funds         	QuantityFunds(void) const;
	Monster		QuantityMonster(void) const;
	Troop		QuantityTroop(void) const;

	void 		SetObjectPassable(bool);

	Heroes* 	GetHeroes(void) const;
	void    	SetHeroes(Heroes*);

	static void 	PlaceMonsterOnTile(Tiles &, const Monster &, u32);
	static void 	UpdateAbandoneMineSprite(Tiles &);
	static void 	FixedPreload(Tiles &);

    private:
	TilesAddon*	FindFlags(void);
	void		CorrectFlags32(u32 index, bool);
	void		RemoveJailSprite(void);
	void		RemoveBarrierSprite(void);
	bool		isLongObject(int direction);

	void 		RedrawBoat(Surface &) const;
	void 		RedrawMonster(Surface &) const;

	void 		QuantitySetVariant(int);
	void 		QuantitySetExt(int);
	void 		QuantitySetSkill(int);
	void 		QuantitySetSpell(int);
	void 		QuantitySetArtifact(int);
	void 		QuantitySetResource(int, u32);
	void 		QuantitySetTeleportType(int);

	int		GetQuantity3(void) const;
	void		SetQuantity3(int);

	static void 	UpdateMonsterInfo(Tiles &);
	static void 	UpdateDwellingPopulation(Tiles &);
	static void 	UpdateMonsterPopulation(Tiles &);
	static void 	UpdateRNDArtifactSprite(Tiles &);
	static void 	UpdateRNDResourceSprite(Tiles &);
	static void 	UpdateStoneLightsSprite(Tiles &);
	static void 	UpdateFountainSprite(Tiles &);
	static void 	UpdateTreasureChestSprite(Tiles &);

    private:
	friend StreamBase & operator<< (StreamBase &, const Tiles &);
	friend StreamBase & operator>> (StreamBase &, Tiles &);
#ifdef WITH_XML
	friend TiXmlElement & operator>> (TiXmlElement &, Tiles &);
#endif

        Addons addons_level1;
        Addons addons_level2; // 16

	u32	maps_index;
	u16	pack_sprite_index;

	u16	tile_passable;
        u8      mp2_object;
        u8	fog_colors;

        u8      quantity1;
        u8      quantity2;
        u8      quantity3;

#ifdef WITH_DEBUG
	u8	passable_disable;
#endif
    };

    StreamBase & operator<< (StreamBase &, const TilesAddon &);
    StreamBase & operator<< (StreamBase &, const Tiles &);
    StreamBase & operator>> (StreamBase &, TilesAddon &);
    StreamBase & operator>> (StreamBase &, Tiles &);
}

#endif
