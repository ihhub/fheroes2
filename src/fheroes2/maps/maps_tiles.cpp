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

#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include "agg.h"
#include "world.h"
#include "race.h"
#include "settings.h"
#include "heroes.h"
#include "castle.h"
#include "mp2.h"
#include "maps.h"
#include "ground.h"
#include "game.h"
#include "game_interface.h"
#include "mounts.h"
#include "trees.h"
#include "objxloc.h"
#include "objtown.h"
#include "objwatr.h"
#include "objlava.h"
#include "objmult.h"
#include "objdsrt.h"
#include "objdirt.h"
#include "objsnow.h"
#include "objswmp.h"
#include "objgras.h"
#include "objcrck.h"
#include "monster.h"
#include "spell.h"
#include "resource.h"
#include "maps_tiles.h"

u8 monster_animation_cicle[] = { 0, 1, 2, 1, 0, 3, 4, 5, 4, 3 };

#ifdef WITH_DEBUG
Surface PassableViewSurface(int passable)
{
    const u32 w = 31;
    const u32 h = 31;
    const RGBA colr = RGBA(0xC0,0x2C,0);
    const RGBA colg = RGBA(0x90,0xC0,0);
    Surface sf(Size(w, h), true);

    if(0 == passable || Direction::CENTER == passable)
	sf.DrawBorder(colr);
    else
    if(DIRECTION_ALL == passable)
	sf.DrawBorder(colg);
    else
    {
	sf.Set(w, h, false);

        for(u32 i = 0; i < w; ++i)
        {
	    if(i < 10)
	    {
    		sf.DrawPoint(Point(i, 0), (passable & Direction::TOP_LEFT ? colg : colr));
    		sf.DrawPoint(Point(i, h - 1), (passable & Direction::BOTTOM_LEFT ? colg : colr));
	    }
	    else
	    if(i < 21)
	    {
    		sf.DrawPoint(Point(i, 0), (passable & Direction::TOP ? colg : colr));
    		sf.DrawPoint(Point(i, h - 1), (passable & Direction::BOTTOM ? colg : colr));
    	    }
	    else
	    {
		sf.DrawPoint(Point(i, 0), (passable & Direction::TOP_RIGHT ? colg : colr));
    		sf.DrawPoint(Point(i, h - 1), (passable & Direction::BOTTOM_RIGHT ? colg : colr));
	    }
	}

        for(u32 i = 0; i < h; ++i)
        {
	    if(i < 10)
	    {
    		sf.DrawPoint(Point(0, i), (passable & Direction::TOP_LEFT ? colg : colr));
    		sf.DrawPoint(Point(w - 1, i), (passable & Direction::TOP_RIGHT ? colg : colr));
	    }
	    else
	    if(i < 21)
	    {
    		sf.DrawPoint(Point(0, i), (passable & Direction::LEFT ? colg : colr));
    		sf.DrawPoint(Point(w - 1, i), (passable & Direction::RIGHT ? colg : colr));
    	    }
	    else
	    {
		sf.DrawPoint(Point(0, i), (passable & Direction::BOTTOM_LEFT ? colg : colr));
    		sf.DrawPoint(Point(w - 1, i), (passable & Direction::BOTTOM_RIGHT ? colg : colr));
	    }
	}
    }

    return sf;
}
#endif

Maps::TilesAddon::TilesAddon() : uniq(0), level(0), object(0), index(0), tmp(0)
{
}

Maps::TilesAddon::TilesAddon(int lv, u32 gid, int obj, u32 ii) : uniq(gid), level(lv), object(obj), index(ii), tmp(0)
{
}

std::string Maps::TilesAddon::String(int lvl) const
{
    std::ostringstream os;
    os << "----------------" << lvl << "--------" << std::endl <<
	  "uniq            : " << uniq << std::endl <<
	  "object          : " << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(object) <<
				", (" << ICN::GetString(MP2::GetICNObject(object)) << ")" << std::endl <<
	  "index           : " << std::dec << static_cast<int>(index) << std::endl <<
	  "level           : " << static_cast<int>(level) << ", (" << static_cast<int>(level % 4) << ")" << std::endl <<
	  "tmp             : " << static_cast<int>(tmp) << std::endl;
    return os.str();
}

Maps::TilesAddon & Maps::TilesAddon::operator= (const Maps::TilesAddon & ta)
{
    level = ta.level;
    object = ta.object;
    index = ta.index;
    uniq = ta.uniq;

    return *this;
}

bool Maps::TilesAddon::isUniq(u32 id) const
{
    return uniq == id;
}

bool Maps::TilesAddon::isICN(int icn) const
{
    return icn == MP2::GetICNObject(object);
}

bool Maps::TilesAddon::PredicateSortRules1(const Maps::TilesAddon & ta1, const Maps::TilesAddon & ta2)
{
    return ((ta1.level % 4) > (ta2.level % 4));
}

bool Maps::TilesAddon::PredicateSortRules2(const Maps::TilesAddon & ta1, const Maps::TilesAddon & ta2)
{
    return ((ta1.level % 4) < (ta2.level % 4));
}

int Maps::TilesAddon::GetLoyaltyObject(const Maps::TilesAddon & addon)
{
    switch(MP2::GetICNObject(addon.object))
    {
        case ICN::X_LOC1:
            if(addon.index == 3) return MP2::OBJ_ALCHEMYTOWER;
            else
            if(addon.index < 3) return MP2::OBJN_ALCHEMYTOWER;
            else
            if(70 == addon.index) return MP2::OBJ_ARENA;
            else
            if(3 < addon.index && addon.index < 72) return MP2::OBJN_ARENA;
            else
            if(77 == addon.index) return MP2::OBJ_BARROWMOUNDS;
            else
            if(71 < addon.index && addon.index < 78) return MP2::OBJN_BARROWMOUNDS;
            else
            if(94 == addon.index) return MP2::OBJ_EARTHALTAR;
            else
            if(77 < addon.index && addon.index < 112) return MP2::OBJN_EARTHALTAR;
            else
            if(118 == addon.index) return MP2::OBJ_AIRALTAR;
            else
            if(111 < addon.index && addon.index < 120) return MP2::OBJN_AIRALTAR;
            else
            if(127 == addon.index) return MP2::OBJ_FIREALTAR;
            else
            if(119 < addon.index && addon.index < 129) return MP2::OBJN_FIREALTAR;
            else
            if(135 == addon.index) return MP2::OBJ_WATERALTAR;
            else
            if(128 < addon.index && addon.index < 137) return MP2::OBJN_WATERALTAR;
            break;

        case ICN::X_LOC2:
            if(addon.index == 4) return MP2::OBJ_STABLES;
            else
            if(addon.index < 4) return MP2::OBJN_STABLES;
            else
            if(addon.index == 9) return MP2::OBJ_JAIL;
            else
            if(4 < addon.index && addon.index < 10) return MP2::OBJN_JAIL;
            else
            if(addon.index == 37) return MP2::OBJ_MERMAID;
            else
            if(9 < addon.index && addon.index < 47) return MP2::OBJN_MERMAID;
            else
            if(addon.index == 101) return MP2::OBJ_SIRENS;
            else
            if(46 < addon.index && addon.index < 111) return MP2::OBJN_SIRENS;
            else
            if(110 < addon.index && addon.index < 136) return MP2::OBJ_REEFS;
            break;

        case ICN::X_LOC3:
            if(addon.index == 30) return MP2::OBJ_HUTMAGI;
            else
            if(addon.index < 32) return MP2::OBJN_HUTMAGI;
            else
            if(addon.index == 50) return MP2::OBJ_EYEMAGI;
            else
            if(31 < addon.index && addon.index < 59) return MP2::OBJN_EYEMAGI;
            // fix
            break;

        default: break;
    }

    return MP2::OBJ_ZERO;
}

int Maps::TilesAddon::GetPassable(const Maps::TilesAddon & ta)
{
    int icn = MP2::GetICNObject(ta.object);

    switch(icn)
    {
        case ICN::MTNSNOW:
        case ICN::MTNSWMP:
        case ICN::MTNLAVA:
        case ICN::MTNDSRT:
        case ICN::MTNMULT:
        case ICN::MTNGRAS:      return ObjMnts1::GetPassable(icn, ta.index);

        case ICN::MTNCRCK:
        case ICN::MTNDIRT:      return ObjMnts2::GetPassable(icn, ta.index);

        case ICN::TREJNGL:
        case ICN::TREEVIL:
        case ICN::TRESNOW:
        case ICN::TREFIR:
        case ICN::TREFALL:
        case ICN::TREDECI:      return ObjTree::GetPassable(ta.index);
        case ICN::OBJNSNOW:     return ObjSnow::GetPassable(ta.index);
        case ICN::OBJNSWMP:     return ObjSwmp::GetPassable(ta.index);
        case ICN::OBJNGRAS:     return ObjGras::GetPassable(ta.index);
        case ICN::OBJNGRA2:     return ObjGra2::GetPassable(ta.index);
        case ICN::OBJNCRCK:     return ObjCrck::GetPassable(ta.index);
        case ICN::OBJNDIRT:     return ObjDirt::GetPassable(ta.index);
        case ICN::OBJNDSRT:     return ObjDsrt::GetPassable(ta.index);
        case ICN::OBJNMUL2:	return ObjMul2::GetPassable(ta.index);
        case ICN::OBJNMULT:     return ObjMult::GetPassable(ta.index);
        case ICN::OBJNLAVA:     return ObjLava::GetPassable(ta.index);
        case ICN::OBJNLAV3:     return ObjLav3::GetPassable(ta.index);
        case ICN::OBJNLAV2:     return ObjLav2::GetPassable(ta.index);
        case ICN::OBJNWAT2:	return ObjWat2::GetPassable(ta.index);
        case ICN::OBJNWATR:     return ObjWatr::GetPassable(ta.index);

        case ICN::OBJNTWBA:	return ObjTwba::GetPassable(ta.index);
        case ICN::OBJNTOWN:     return ObjTown::GetPassable(ta.index);

        case ICN::X_LOC1:       return ObjXlc1::GetPassable(ta.index);
        case ICN::X_LOC2:       return ObjXlc2::GetPassable(ta.index);
        case ICN::X_LOC3:       return ObjXlc3::GetPassable(ta.index);

        // MANUAL.ICN
        case ICN::TELEPORT1:
        case ICN::TELEPORT2:
        case ICN::TELEPORT3:
        case ICN::FOUNTAIN:     return 0;

        default: break;
    }

    return DIRECTION_ALL;
}

int Maps::TilesAddon::GetActionObject(const Maps::TilesAddon & ta)
{
    switch(MP2::GetICNObject(ta.object))
    {
        case ICN::MTNSNOW:
        case ICN::MTNSWMP:
        case ICN::MTNLAVA:
        case ICN::MTNDSRT:
        case ICN::MTNMULT:
        case ICN::MTNGRAS:      return ObjMnts1::GetActionObject(ta.index);

        case ICN::MTNCRCK:
        case ICN::MTNDIRT:      return ObjMnts2::GetActionObject(ta.index);

        case ICN::TREJNGL:
        case ICN::TREEVIL:
        case ICN::TRESNOW:
        case ICN::TREFIR:
        case ICN::TREFALL:
        case ICN::TREDECI:      return ObjTree::GetActionObject(ta.index);

        case ICN::OBJNSNOW:     return ObjSnow::GetActionObject(ta.index);
        case ICN::OBJNSWMP:     return ObjSwmp::GetActionObject(ta.index);
        case ICN::OBJNGRAS:     return ObjGras::GetActionObject(ta.index);
        case ICN::OBJNGRA2:     return ObjGra2::GetActionObject(ta.index);
        case ICN::OBJNCRCK:     return ObjCrck::GetActionObject(ta.index);
        case ICN::OBJNDIRT:     return ObjDirt::GetActionObject(ta.index);
        case ICN::OBJNDSRT:     return ObjDsrt::GetActionObject(ta.index);
        case ICN::OBJNMUL2:	return ObjMul2::GetActionObject(ta.index);
        case ICN::OBJNMULT:     return ObjMult::GetActionObject(ta.index);
        case ICN::OBJNLAVA:     return ObjLava::GetActionObject(ta.index);
        case ICN::OBJNLAV3:     return ObjLav3::GetActionObject(ta.index);
        case ICN::OBJNLAV2:     return ObjLav2::GetActionObject(ta.index);
        case ICN::OBJNWAT2:	return ObjWat2::GetActionObject(ta.index);
        case ICN::OBJNWATR:     return ObjWatr::GetActionObject(ta.index);

        case ICN::OBJNTWBA:	return ObjTwba::GetActionObject(ta.index);
        case ICN::OBJNTOWN:     return ObjTown::GetActionObject(ta.index);

        case ICN::X_LOC1:       return ObjXlc1::GetActionObject(ta.index);
        case ICN::X_LOC2:       return ObjXlc2::GetActionObject(ta.index);
        case ICN::X_LOC3:       return ObjXlc3::GetActionObject(ta.index);

        // MANUAL.ICN
        case ICN::TELEPORT1:
        case ICN::TELEPORT2:
        case ICN::TELEPORT3:	return MP2::OBJ_STONELIGHTS;
        case ICN::FOUNTAIN:	return MP2::OBJ_FOUNTAIN;

        default: break;
    }

    return MP2::OBJ_ZERO;
}

bool Maps::TilesAddon::isRoad(int direct) const
{
    switch(MP2::GetICNObject(object))
    {
	// from sprite road
	case ICN::ROAD:
	    if(0 == index ||
		4 == index ||
		5 == index ||
		13 == index ||
		26 == index) return direct & (Direction::TOP | Direction::BOTTOM);
	    else
	    if(2 == index ||
		21 == index ||
		28 == index) return direct & (Direction::LEFT | Direction::RIGHT);
	    else
	    if(17 == index ||
		29 == index) return direct & (Direction::TOP_LEFT | Direction::BOTTOM_RIGHT);
	    else
	    if(18 == index ||
		30 == index) return direct & (Direction::TOP_RIGHT | Direction::BOTTOM_LEFT);
	    else
	    if(3 == index) return direct & (Direction::TOP | Direction::BOTTOM | Direction::LEFT | Direction::RIGHT);
	    else
	    if(6 == index) return direct & (Direction::TOP | Direction::BOTTOM | Direction::RIGHT);
	    else
	    if(7 == index) return direct & (Direction::TOP | Direction::RIGHT);
	    else
	    if(9 == index) return direct & (Direction::BOTTOM | Direction::RIGHT);
	    else
	    if(12 == index) return direct & (Direction::BOTTOM | Direction::LEFT);
	    else
	    if(14 == index) return direct & (Direction::TOP | Direction::BOTTOM | Direction::LEFT);
	    else
	    if(16 == index) return direct & (Direction::TOP | Direction::LEFT);
	    else
	    if(19 == index) return direct & (Direction::TOP_LEFT | Direction::BOTTOM_RIGHT);
	    else
	    if(20 == index) return direct & (Direction::TOP_RIGHT | Direction::BOTTOM_LEFT);
	    else break;

	// castle and tower (gate)
	case ICN::OBJNTOWN:
	    if(13 == index ||
		29 == index ||
	    	45 == index ||
	    	61 == index ||
	    	77 == index ||
	    	93 == index ||
	    	109 == index ||
	    	125 == index ||
	    	141 == index ||
	    	157 == index ||
	    	173 == index ||
	    	189 == index) return direct & (Direction::TOP | Direction::BOTTOM);
	    else break;

		break;
	// castle lands (gate)
        case ICN::OBJNTWBA:
	    if(7 == index ||
		17 == index ||
		27 == index ||
		37 == index ||
		47 == index ||
		57 == index ||
		67 == index ||
		77 == index) return direct & (Direction::TOP | Direction::BOTTOM);
	    else break;

	default:
	    break;
    }

    return false;
}

bool Maps::TilesAddon::isStream(const TilesAddon & ta)
{
    return ICN::STREAM == MP2::GetICNObject(ta.object) ||
	(ICN::OBJNMUL2 == MP2::GetICNObject(ta.object) && (ta.index < 14));
}

bool Maps::TilesAddon::isRoad(const TilesAddon & ta)
{
    return ICN::ROAD == MP2::GetICNObject(ta.object);
}

bool Maps::TilesAddon::isWaterResource(const TilesAddon & ta)
{
    return (ICN::OBJNWATR == MP2::GetICNObject(ta.object) &&
                (0 == ta.index ||    // buttle
                19 == ta.index ||    // chest
                45 == ta.index ||    // flotsam
                111 == ta.index)     // surviror
            );
}

bool Maps::TilesAddon::isWhirlPool(const TilesAddon & ta)
{
    return (ICN::OBJNWATR == MP2::GetICNObject(ta.object) &&
                (ta.index >= 202 && ta.index <= 225));
}

bool Maps::TilesAddon::isStandingStone(const TilesAddon & ta)
{
    return (ICN::OBJNMULT == MP2::GetICNObject(ta.object) &&
                (ta.index == 84 || ta.index == 85));
}

bool Maps::TilesAddon::isResource(const TilesAddon & ta)
{
            // OBJNRSRC
    return ((ICN::OBJNRSRC == MP2::GetICNObject(ta.object) && (ta.index % 2)) ||
            // TREASURE
            (ICN::TREASURE == MP2::GetICNObject(ta.object)));
}

bool Maps::TilesAddon::isRandomResource(const TilesAddon & ta)
{
	    // OBJNRSRC
    return (ICN::OBJNRSRC == MP2::GetICNObject(ta.object) && 17 == ta.index);
}

bool Maps::TilesAddon::isArtifact(const TilesAddon & ta)
{
    // OBJNARTI (skip ultimate)
    return (ICN::OBJNARTI == MP2::GetICNObject(ta.object) && (ta.index > 0x10) && (ta.index % 2));
}

bool Maps::TilesAddon::isRandomArtifact(const TilesAddon & ta)
{
    // OBJNARTI
    return (ICN::OBJNARTI == MP2::GetICNObject(ta.object) && 0xA3 == ta.index);
}

bool Maps::TilesAddon::isRandomArtifact1(const TilesAddon & ta)
{
    // OBJNARTI
    return (ICN::OBJNARTI == MP2::GetICNObject(ta.object) && 0xA7 == ta.index);
}

bool Maps::TilesAddon::isRandomArtifact2(const TilesAddon & ta)
{
    // OBJNARTI
    return (ICN::OBJNARTI == MP2::GetICNObject(ta.object) && 0xA9 == ta.index);
}

bool Maps::TilesAddon::isRandomArtifact3(const TilesAddon & ta)
{
    // OBJNARTI
    return (ICN::OBJNARTI == MP2::GetICNObject(ta.object) && 0xAB == ta.index);
}

bool Maps::TilesAddon::isUltimateArtifact(const TilesAddon & ta)
{
    // OBJNARTI
    return (ICN::OBJNARTI == MP2::GetICNObject(ta.object) && 0xA4 == ta.index);
}

bool Maps::TilesAddon::isCampFire(const TilesAddon & ta)
{
	    // MTNDSRT
    return ((ICN::OBJNDSRT == MP2::GetICNObject(ta.object) && 61 == ta.index) ||
	    // OBJNMULT
            (ICN::OBJNMULT == MP2::GetICNObject(ta.object) && 131 == ta.index) ||
	    // OBJNSNOW
            (ICN::OBJNSNOW == MP2::GetICNObject(ta.object) && 4 == ta.index));
}

bool Maps::TilesAddon::isMonster(const TilesAddon & ta)
{
	    // MONS32
    return (ICN::MONS32 == MP2::GetICNObject(ta.object));
}

bool Maps::TilesAddon::isArtesianSpring(const TilesAddon & ta)
{
    return (ICN::OBJNCRCK == MP2::GetICNObject(ta.object) &&
	    (ta.index == 3 || ta.index == 4));
}

bool Maps::TilesAddon::isSkeleton(const TilesAddon & ta)
{
    return (ICN::OBJNDSRT == MP2::GetICNObject(ta.object) && ta.index == 84);
}

bool Maps::TilesAddon::isSkeletonFix(const TilesAddon & ta)
{
    return (ICN::OBJNDSRT == MP2::GetICNObject(ta.object) && ta.index == 83);
}

bool Maps::TilesAddon::isOasis(const TilesAddon & ta)
{
    return (ICN::OBJNDSRT == MP2::GetICNObject(ta.object) &&
		(ta.index == 108 || ta.index == 109));
}

bool Maps::TilesAddon::isWateringHole(const TilesAddon & ta)
{
    return (ICN::OBJNCRCK == MP2::GetICNObject(ta.object) &&
		(ta.index >= 217 && ta.index <= 220));
}

bool Maps::TilesAddon::isJail(const TilesAddon & ta)
{
    return (ICN::X_LOC2 == MP2::GetICNObject(ta.object) && 0x09 == ta.index);
}

bool Maps::TilesAddon::isEvent(const TilesAddon & ta)
{
    // OBJNMUL2
    return (ICN::OBJNMUL2 == MP2::GetICNObject(ta.object) && 0xA3 == ta.index);
}

bool Maps::TilesAddon::isMine(const TilesAddon & ta)
{
    // EXTRAOVR
    return (ICN::EXTRAOVR == MP2::GetICNObject(ta.object));
}

bool Maps::TilesAddon::isBoat(const TilesAddon & ta)
{
    // OBJNWAT2
    return (ICN::OBJNWAT2 == MP2::GetICNObject(ta.object) && 0x17 == ta.index);
}

bool Maps::TilesAddon::isMiniHero(const TilesAddon & ta)
{
    // MINIHERO
    return (ICN::MINIHERO == MP2::GetICNObject(ta.object));
}

bool Maps::TilesAddon::isCastle(const TilesAddon & ta)
{
    // OBJNTOWN
    return (ICN::OBJNTOWN == MP2::GetICNObject(ta.object));
}

bool Maps::TilesAddon::isRandomCastle(const TilesAddon & ta)
{
    // OBJNTWRD
    return (ICN::OBJNTWRD == MP2::GetICNObject(ta.object) && 32 > ta.index);
}

bool Maps::TilesAddon::isRandomMonster(const TilesAddon & ta)
{
    // MONS32
    return(ICN::MONS32 == MP2::GetICNObject(ta.object) &&
	    (0x41 < ta.index && 0x47 > ta.index));
}

bool Maps::TilesAddon::isBarrier(const TilesAddon & ta)
{
    return ICN::X_LOC3 == MP2::GetICNObject(ta.object) && 
		60 <= ta.index && 102 >= ta.index && 0 ==  (ta.index % 6);
}

int Maps::TilesAddon::ColorFromBarrierSprite(const TilesAddon & ta)
{
    // 60, 66, 72, 78, 84, 90, 96, 102
    return ICN::X_LOC3 == MP2::GetICNObject(ta.object) && 
		60 <= ta.index && 102 >= ta.index ? ((ta.index - 60) / 6) + 1 : 0;
}

int Maps::TilesAddon::ColorFromTravellerTentSprite(const TilesAddon & ta)
{
    // 110, 114, 118, 122, 126, 130, 134, 138
    return ICN::X_LOC3 == MP2::GetICNObject(ta.object) && 
		110 <= ta.index && 138 >= ta.index ? ((ta.index - 110) / 4) + 1 : 0;
}

bool Maps::TilesAddon::isAbandoneMineSprite(const TilesAddon & ta)
{
    return (ICN::OBJNGRAS == MP2::GetICNObject(ta.object) && 6 == ta.index) ||
	    (ICN::OBJNDIRT == MP2::GetICNObject(ta.object) && 8 == ta.index);
}

bool Maps::TilesAddon::isFlag32(const TilesAddon & ta)
{
    return ICN::FLAG32 == MP2::GetICNObject(ta.object);
}

bool Maps::TilesAddon::isX_LOC123(const TilesAddon & ta)
{
    return (ICN::X_LOC1 == MP2::GetICNObject(ta.object) ||
	    ICN::X_LOC2 == MP2::GetICNObject(ta.object) ||
	    ICN::X_LOC3 == MP2::GetICNObject(ta.object));
}

bool Maps::TilesAddon::isShadow(const TilesAddon & ta)
{
    const int icn = MP2::GetICNObject(ta.object);

    switch(icn)
    {
        case ICN::MTNDSRT:
        case ICN::MTNGRAS:
        case ICN::MTNLAVA:
        case ICN::MTNMULT:
        case ICN::MTNSNOW:
        case ICN::MTNSWMP: return ObjMnts1::isShadow(ta.index);

        case ICN::MTNCRCK:
        case ICN::MTNDIRT: return ObjMnts2::isShadow(ta.index);

        case ICN::TREDECI:
        case ICN::TREEVIL:
        case ICN::TREFALL:
        case ICN::TREFIR:
        case ICN::TREJNGL:
        case ICN::TRESNOW: return ObjTree::isShadow(ta.index);

	case ICN::OBJNCRCK: return ObjCrck::isShadow(ta.index);
	case ICN::OBJNDIRT: return ObjDirt::isShadow(ta.index);
	case ICN::OBJNDSRT: return ObjDsrt::isShadow(ta.index);
	case ICN::OBJNGRA2: return ObjGra2::isShadow(ta.index);
	case ICN::OBJNGRAS: return ObjGras::isShadow(ta.index);
        case ICN::OBJNMUL2: return ObjMul2::isShadow(ta.index);
        case ICN::OBJNMULT: return ObjMult::isShadow(ta.index);
	case ICN::OBJNSNOW: return ObjSnow::isShadow(ta.index);
	case ICN::OBJNSWMP: return ObjSwmp::isShadow(ta.index);
	case ICN::OBJNWAT2: return ObjWat2::isShadow(ta.index);
	case ICN::OBJNWATR: return ObjWatr::isShadow(ta.index);

	case ICN::OBJNARTI:
	case ICN::OBJNRSRC: return 0 == (ta.index % 2);

	case ICN::OBJNTWRD: return ta.index > 31;
	case ICN::OBJNTWSH: return true;

	default: break;
    }

    return false;
}

bool Maps::TilesAddon::isMounts(const TilesAddon & ta)
{
    switch(MP2::GetICNObject(ta.object))
    {
        case ICN::MTNSNOW:
        case ICN::MTNSWMP:
        case ICN::MTNLAVA:
        case ICN::MTNDSRT:
        case ICN::MTNMULT:
        case ICN::MTNGRAS:
	    return ! ObjMnts1::isShadow(ta.index);

        case ICN::MTNCRCK:
        case ICN::MTNDIRT:
	    return ! ObjMnts2::isShadow(ta.index);

	default: break;
    }

    return false;
}


bool Maps::TilesAddon::isRocs(const TilesAddon & ta)
{
    switch(MP2::GetICNObject(ta.object))
    {
	// roc objects
	case ICN::OBJNSNOW:
	    if((ta.index > 21 && ta.index < 25) || (ta.index > 25 && ta.index < 29) ||
		ta.index == 30 || ta.index == 32 || ta.index == 34 || ta.index == 35 ||
		(ta.index > 36 && ta.index < 40))
		return true;
	    break;

	case ICN::OBJNSWMP:
	    if(ta.index == 201 || ta.index == 205 || 
		(ta.index > 207 && ta.index < 211))
		return true;
	    break;

	case ICN::OBJNGRAS:
	    if((ta.index > 32 && ta.index < 36) || ta.index == 37 || ta.index == 38 ||
		ta.index == 40 || ta.index == 41 || ta.index == 43 || ta.index == 45)
		return true;
	    break;

	case ICN::OBJNDIRT:
	    if(ta.index == 92 || ta.index == 93 || ta.index == 95 || ta.index == 98 ||
		ta.index == 99 || ta.index == 101 || ta.index == 102 || ta.index == 104 || ta.index == 105)
		return true;
	    break;

	case ICN::OBJNCRCK:
	    if(ta.index == 10 || ta.index == 11 ||
	        ta.index == 18 || ta.index == 19 || ta.index == 21 || ta.index == 22 ||
		(ta.index > 23 && ta.index < 28) || (ta.index > 28 && ta.index < 33) ||
		ta.index == 34 || ta.index == 35 || ta.index == 37 || ta.index == 38 ||
		(ta.index > 39 && ta.index < 45) || ta.index == 46 || ta.index == 47 ||
		ta.index == 49 || ta.index == 50 || ta.index == 52 || ta.index == 53 || ta.index == 55)
		return true;
	    break;

	case ICN::OBJNWAT2:
	    if(ta.index == 0 || ta.index == 2)
		return true;
	    break;

	case ICN::OBJNWATR:
	    if(ta.index == 182 || ta.index == 183 ||
		(ta.index > 184 && ta.index < 188))
		return true;
	    break;

	default: break;
    }

    return false;
}
    

bool Maps::TilesAddon::isForests(const TilesAddon & ta)
{
    switch(MP2::GetICNObject(ta.object))
    {
        case ICN::TREJNGL:
        case ICN::TREEVIL:
        case ICN::TRESNOW:
        case ICN::TREFIR:
        case ICN::TREFALL:
        case ICN::TREDECI:
	    return ! ObjTree::isShadow(ta.index);

	default: break;
    }

    return false;
}

bool Maps::TilesAddon::isStump(const TilesAddon & ta)
{
    switch(MP2::GetICNObject(ta.object))
    {
	case ICN::OBJNSNOW:
	    if(ta.index == 41 || ta.index == 42)
		return true;
	    break;

	default: break;
    }

    return false;
}

bool Maps::TilesAddon::isDeadTrees(const TilesAddon & ta)
{
    switch(MP2::GetICNObject(ta.object))
    {
	case ICN::OBJNMUL2:
	    if(ta.index == 16 || ta.index == 18 || ta.index == 19)
		return true;
	    break;

	case ICN::OBJNMULT:
	    if(ta.index == 0 || ta.index == 2 || ta.index == 4)
		return true;
	    break;

	case ICN::OBJNSNOW:
	    if((ta.index > 50 && ta.index < 53) || (ta.index > 54 && ta.index < 59) ||
		(ta.index > 59 && ta.index < 63) || (ta.index > 63 && ta.index < 67) ||
		ta.index == 68 || ta.index == 69 || ta.index == 71 || ta.index == 72 ||
		ta.index == 74 || ta.index == 75 || ta.index == 77)
		return true;
	    break;

	case ICN::OBJNSWMP:
	    if(ta.index == 161 || ta.index == 162 ||
		(ta.index > 163 && ta.index < 170) ||
		(ta.index > 170 && ta.index < 175) ||
	        ta.index == 176 || ta.index == 177)
		return true;
	    break;

	default: break;
    }

    return false;
}

bool Maps::TilesAddon::isCactus(const TilesAddon & ta)
{
    switch(MP2::GetICNObject(ta.object))
    {
	case ICN::OBJNDSRT:
	    if(ta.index == 24 || ta.index == 26 || ta.index == 28 ||
		(ta.index > 29 && ta.index < 33) ||
		ta.index == 34 || ta.index == 36 || ta.index == 37 || ta.index == 39 ||
		ta.index == 40 || ta.index == 42 || ta.index == 43 ||
		ta.index == 45 || ta.index == 48 || ta.index == 49 ||
		ta.index == 51 || ta.index == 53)
		return true;
	    break;

	case ICN::OBJNCRCK:
    	    if(ta.index == 14 || ta.index == 16)
		return true;
	    break;

	default: break;
    }

    return false;
}

bool Maps::TilesAddon::isTrees(const TilesAddon & ta)
{
    switch(MP2::GetICNObject(ta.object))
    {
	// tree objects
	case ICN::OBJNDSRT:
	    if(ta.index == 3 || ta.index == 4 || ta.index == 6 ||
		ta.index == 7 || ta.index == 9 || ta.index == 10 ||
		ta.index == 12 || ta.index == 74 || ta.index == 76)
		return true;
	    break;

	case ICN::OBJNDIRT:
	    if(ta.index == 115 || ta.index == 118 || ta.index == 120 ||
		ta.index == 123 || ta.index == 125 || ta.index == 127)
		return true;
	    break;

	case ICN::OBJNGRAS:
	    if(ta.index == 80 || (ta.index > 82 && ta.index < 86) ||
		ta.index == 87 || (ta.index > 88 && ta.index < 92))
		return true;
	    break;

	default: break;
    }

    return false;
}

void Maps::TilesAddon::UpdateAbandoneMineLeftSprite(TilesAddon & ta, int resource)
{
    if(ICN::OBJNGRAS == MP2::GetICNObject(ta.object) && 6 == ta.index)
    {
	ta.object = 128; // MTNGRAS
	ta.index = 82;
    }
    else
    if(ICN::OBJNDIRT == MP2::GetICNObject(ta.object) && 8 == ta.index)
    {
	ta.object = 104; // MTNDIRT
	ta.index = 112;
    }
    else
    if(ICN::EXTRAOVR == MP2::GetICNObject(ta.object) && 5 == ta.index)
    {
	switch(resource)
	{
	    case Resource::ORE:		ta.index = 0; break;
	    case Resource::SULFUR:	ta.index = 1; break;
	    case Resource::CRYSTAL:	ta.index = 2; break;
	    case Resource::GEMS:	ta.index = 3; break;
	    case Resource::GOLD:	ta.index = 4; break;
	    default: break;
	}
    }
}

void Maps::TilesAddon::UpdateAbandoneMineRightSprite(TilesAddon & ta)
{
    if(ICN::OBJNDIRT == MP2::GetICNObject(ta.object) && ta.index == 9)
    {
	ta.object = 104;
	ta.index = 113;
    }
    else
    if(ICN::OBJNGRAS == MP2::GetICNObject(ta.object) && ta.index == 7)
    {
	ta.object = 128;
	ta.index = 83;
    }
}

void Maps::TilesAddon::UpdateFountainSprite(TilesAddon & ta)
{
    if(ICN::OBJNMUL2 == MP2::GetICNObject(ta.object) && 15 == ta.index)
    {
	ta.object = 0x14;
	ta.index = 0;
    }
}

void Maps::TilesAddon::UpdateTreasureChestSprite(TilesAddon & ta)
{
    if(ICN::OBJNRSRC == MP2::GetICNObject(ta.object) && 19 == ta.index)
    {
	ta.object = 0x15;
	ta.index = 0;
    }
}

int Maps::TilesAddon::UpdateStoneLightsSprite(TilesAddon & ta)
{
    if(ICN::OBJNMUL2 == MP2::GetICNObject(ta.object))
    switch(ta.index)
    {
	case 116: ta.object = 0x11; ta.index = 0; return 1;
	case 119: ta.object = 0x12; ta.index = 0; return 2;
	case 122: ta.object = 0x13; ta.index = 0; return 3;
	default: break;
    }
    return 0;
}

std::pair<int, int> Maps::TilesAddon::ColorRaceFromHeroSprite(const TilesAddon & ta)
{
    std::pair<int, int> res;

    if(7 > ta.index)
	res.first = Color::BLUE;
    else
    if(14 > ta.index)
	res.first = Color::GREEN;
    else
    if(21 > ta.index)
        res.first = Color::RED;
    else
    if(28 > ta.index)
        res.first = Color::YELLOW;
    else
    if(35 > ta.index)
	res.first = Color::ORANGE;
    else
	res.first = Color::PURPLE;

    //res.second = Race::NONE;

    switch(ta.index % 7)
    {
        case 0: res.second = Race::KNGT; break;
        case 1: res.second = Race::BARB; break;
        case 2: res.second = Race::SORC; break;
        case 3: res.second = Race::WRLK; break;
        case 4: res.second = Race::WZRD; break;
        case 5: res.second = Race::NECR; break;
        case 6: res.second = Race::RAND; break;
    }

    return res;
}

bool Maps::TilesAddon::ForceLevel1(const TilesAddon & ta)
{
    // broken ship: left roc
    if(ICN::OBJNWAT2 == MP2::GetICNObject(ta.object) && ta.index == 11)
	return true;

    return false;
}

bool Maps::TilesAddon::ForceLevel2(const TilesAddon & ta)
{
    return false;
}

/* Maps::Addons */
void Maps::Addons::Remove(u32 uniq)
{
    /*
    erase(std::remove_if(begin(), end(),
	    std::bind2nd(std::mem_fun_ref(&Maps::TilesAddon::isUniq), uniq)),
	    end());
    */
    remove_if(std::bind2nd(std::mem_fun_ref(&Maps::TilesAddon::isUniq), uniq));
}

u32 PackTileSpriteIndex(u32 index, u32 shape) /* index max: 0x3FFF, shape value: 0, 1, 2, 3 */
{
    return (shape << 14) | (0x3FFF & index);
}

/* Maps::Tiles */
Maps::Tiles::Tiles() : maps_index(0), pack_sprite_index(0), tile_passable(DIRECTION_ALL),
    mp2_object(0), fog_colors(Color::ALL), quantity1(0), quantity2(0), quantity3(0)
#ifdef WITH_DEBUG
    , passable_disable(0)
#endif
{
}

void Maps::Tiles::Init(s32 index, const MP2::mp2tile_t & mp2)
{
    tile_passable = DIRECTION_ALL;
    quantity1	= mp2.quantity1;
    quantity2	= mp2.quantity2;
    quantity3	= 0;
    fog_colors	= Color::ALL;

    SetTile(mp2.tileIndex, mp2.shape);
    SetIndex(index);
    SetObject(mp2.generalObject);

    addons_level1.clear();
    addons_level2.clear();

    AddonsPushLevel1(mp2);
    AddonsPushLevel2(mp2);
}

void Maps::Tiles::SetIndex(int index)
{
    maps_index	= index;
}

int Maps::Tiles::GetQuantity3(void) const
{
    return quantity3;
}

void Maps::Tiles::SetQuantity3(int mod)
{
    quantity3 = mod;
}

Heroes* Maps::Tiles::GetHeroes(void) const
{
    return MP2::OBJ_HEROES == mp2_object && GetQuantity3() ?
	world.GetHeroes(GetQuantity3() - 1) : NULL;
}

void Maps::Tiles::SetHeroes(Heroes* hero)
{
    if(hero)
    {
	hero->SetMapsObject(mp2_object);
	SetQuantity3(hero->GetID() + 1);
	SetObject(MP2::OBJ_HEROES);
    }
    else
    {
	hero = GetHeroes();

	if(hero)
	{
	    SetObject(hero->GetMapsObject());
	    hero->SetMapsObject(MP2::OBJ_ZERO);
	}
	else
	    SetObject(MP2::OBJ_ZERO);

	SetQuantity3(0);
    }
}

Point Maps::Tiles::GetCenter(void) const
{
    return Maps::GetPoint(GetIndex());
}

s32 Maps::Tiles::GetIndex(void) const
{
    return maps_index;
}

int Maps::Tiles::GetObject(bool skip_hero  /* true */) const
{
    if(!skip_hero && MP2::OBJ_HEROES == mp2_object)
    {
	const Heroes* hero = GetHeroes();
	return hero ? hero->GetMapsObject() : MP2::OBJ_ZERO;
    }

    return mp2_object;
}

void Maps::Tiles::SetObject(int object)
{
    mp2_object = object;
}

void Maps::Tiles::SetTile(u32 sprite_index, u32 shape)
{
    pack_sprite_index = PackTileSpriteIndex(sprite_index, shape);
}

u32 Maps::Tiles::TileSpriteIndex(void) const
{
    return pack_sprite_index & 0x3FFF;
}

u32  Maps::Tiles::TileSpriteShape(void) const
{
    return pack_sprite_index >> 14;
}

Surface Maps::Tiles::GetTileSurface(void) const
{
    return AGG::GetTIL(TIL::GROUND32, TileSpriteIndex(), TileSpriteShape());
}

bool isMountsRocs(const Maps::TilesAddon & ta)
{
    return Maps::TilesAddon::isMounts(ta) || Maps::TilesAddon::isRocs(ta);
}

bool isForestsTrees(const Maps::TilesAddon & ta)
{
    return Maps::TilesAddon::isForests(ta) || Maps::TilesAddon::isTrees(ta) ||
	Maps::TilesAddon::isCactus(ta);
}

bool Exclude4LongObject(const Maps::TilesAddon & ta)
{
    return Maps::TilesAddon::isStream(ta) ||
	    Maps::TilesAddon::isRoad(ta) || Maps::TilesAddon::isShadow(ta);
}

bool HaveLongObjectUniq(const Maps::Addons & level, u32 uid)
{
    for(Maps::Addons::const_iterator
	it = level.begin(); it != level.end(); ++it)
	if(!Exclude4LongObject(*it) && (*it).isUniq(uid)) return true;
    return false;
}

bool TopObjectDisable(const Maps::TilesAddon & ta)
{
    return isMountsRocs(ta) || isForestsTrees(ta);
}

bool Maps::Tiles::isLongObject(int direction)
{
    if(Maps::isValidDirection(GetIndex(), direction))
    {
	Tiles & tile = world.GetTiles(Maps::GetDirectionIndex(GetIndex(), direction));

	for(Addons::const_iterator
	    it = addons_level1.begin(); it != addons_level1.end(); ++it)
	    if(! Exclude4LongObject(*it) &&
		(HaveLongObjectUniq(tile.addons_level1, (*it).uniq) ||
		(! Maps::TilesAddon::isTrees(*it) && HaveLongObjectUniq(tile.addons_level2, (*it).uniq))))
		return true;
    }
    return false;
}

void Maps::Tiles::UpdatePassable(void)
{
    tile_passable = DIRECTION_ALL;
#ifdef WITH_DEBUG
    passable_disable = 0;
#endif

    const int obj = GetObject(false);
    bool emptyobj = MP2::OBJ_ZERO == obj || MP2::OBJ_COAST == obj || MP2::OBJ_EVENT == obj;

    if(MP2::isActionObject(obj, isWater()))
    {
	tile_passable = MP2::GetObjectDirect(obj);
#ifdef WITH_DEBUG
	if(tile_passable == 0)
	    passable_disable = 8;
#endif
	return;
    }

    // on ground
    if(MP2::OBJ_HEROES != mp2_object && !isWater())
    {
	bool mounts1 = addons_level1.end() != std::find_if(addons_level1.begin(), addons_level1.end(), isMountsRocs);
	bool mounts2 = addons_level2.end() != std::find_if(addons_level2.begin(), addons_level2.end(), isMountsRocs);
	bool trees1  = addons_level1.end() != std::find_if(addons_level1.begin(), addons_level1.end(), isForestsTrees);
	bool trees2  = addons_level2.end() != std::find_if(addons_level2.begin(), addons_level2.end(), isForestsTrees);

	// fix coast passable
	if(tile_passable &&
	    //! MP2::isActionObject(obj, false) &&
	    ! emptyobj &&
	    Maps::TileIsCoast(GetIndex(), Direction::TOP|Direction::BOTTOM|Direction::LEFT|Direction::RIGHT) &&
	    (addons_level1.size() != static_cast<size_t>(std::count_if(addons_level1.begin(), addons_level1.end(),
							std::ptr_fun(&TilesAddon::isShadow)))))
	{
	    tile_passable = 0;
#ifdef WITH_DEBUG
	    passable_disable = 1;
#endif
	}

	// fix mountain layer
	if(tile_passable &&
	    (MP2::OBJ_MOUNTS == obj || MP2::OBJ_TREES == obj) &&
	    mounts1 && (mounts2 || trees2))
	{
	    tile_passable = 0;
#ifdef WITH_DEBUG
	    passable_disable = 2;
#endif
	}

	// fix trees layer
	if(tile_passable &&
	    (MP2::OBJ_MOUNTS == obj || MP2::OBJ_TREES == obj) &&
	    trees1 && (mounts2 || trees2))
	{
	    tile_passable = 0;
#ifdef WITH_DEBUG
	    passable_disable = 3;
#endif
	}

	// town twba
	if(tile_passable &&
	    FindAddonICN1(ICN::OBJNTWBA) && (mounts2 || trees2))
	{
	    tile_passable = 0;
#ifdef WITH_DEBUG
	    passable_disable = 5;
#endif
	}

	if(Maps::isValidDirection(GetIndex(), Direction::TOP))
	{
	    Tiles & top = world.GetTiles(Maps::GetDirectionIndex(GetIndex(), Direction::TOP));
	    // fix: rocs on water
	    if(top.isWater() &&
		top.tile_passable &&
		! (Direction::TOP & top.tile_passable))
	    {
		top.tile_passable = 0;
#ifdef WITH_DEBUG
		top.passable_disable = 9;
#endif
	    }
	}
    }

    // fix bottom border: disable passable for all no action objects
    if(tile_passable &&
	! Maps::isValidDirection(GetIndex(), Direction::BOTTOM) &&
	! emptyobj &&
	! MP2::isActionObject(obj, isWater()))
    {
	tile_passable = 0;
#ifdef WITH_DEBUG
	passable_disable = 4;
#endif
    }

    // check all sprite (level 1)
    for(Addons::const_iterator
	it = addons_level1.begin(); it != addons_level1.end(); ++it)
    {
	if(tile_passable)
	{
	    tile_passable &= TilesAddon::GetPassable(*it);

#ifdef WITH_DEBUG
	    if(0 == tile_passable)
		passable_disable = 6;
#endif
	}
    }

    // fix top passable
    if(Maps::isValidDirection(GetIndex(), Direction::TOP))
    {
	Tiles & top = world.GetTiles(Maps::GetDirectionIndex(GetIndex(), Direction::TOP));

	if(isWater() == top.isWater() &&
	    top.addons_level1.end() != std::find_if(top.addons_level1.begin(), top.addons_level1.end(), TopObjectDisable) &&
	    ! MP2::isActionObject(top.GetObject(false), isWater()) &&
	      (tile_passable && ! (tile_passable & DIRECTION_TOP_ROW)) &&
	    ! (top.tile_passable & DIRECTION_TOP_ROW))
	{
	    top.tile_passable = 0;
#ifdef WITH_DEBUG
	    top.passable_disable = 7;
#endif
	}
    }

    // fix corners
    if(Maps::isValidDirection(GetIndex(), Direction::LEFT))
    {
	Tiles & left = world.GetTiles(Maps::GetDirectionIndex(GetIndex(), Direction::LEFT));

	// left corner
	if(left.tile_passable &&
    	    isLongObject(Direction::TOP) &&
	    ! ((Direction::TOP | Direction::TOP_LEFT) & tile_passable) &&
	    (Direction::TOP_RIGHT & left.tile_passable))
	{
	    left.tile_passable &= ~Direction::TOP_RIGHT;
	}
	else
	// right corner
	if(tile_passable &&
    	    left.isLongObject(Direction::TOP) &&
	    ! ((Direction::TOP | Direction::TOP_RIGHT) & left.tile_passable) &&
	    (Direction::TOP_LEFT & tile_passable))
	{
	    tile_passable &= ~Direction::TOP_LEFT;
	}
    }
}

u32 Maps::Tiles::GetObjectUID(int obj) const
{
    const Maps::TilesAddon* addon = FindObjectConst(obj);
    return addon ? addon->uniq : 0;
}

int Maps::Tiles::GetPassable(void) const
{
    return tile_passable;
}

void Maps::Tiles::AddonsPushLevel1(const MP2::mp2tile_t & mt)
{
    if(mt.objectName1 && mt.indexName1 < 0xFF)
	AddonsPushLevel1(TilesAddon(0, mt.uniqNumber1, mt.objectName1, mt.indexName1));
}

void Maps::Tiles::AddonsPushLevel1(const MP2::mp2addon_t & ma)
{
    if(ma.objectNameN1 && ma.indexNameN1 < 0xFF)
	AddonsPushLevel1(TilesAddon(ma.quantityN, ma.uniqNumberN1, ma.objectNameN1, ma.indexNameN1));
}

void Maps::Tiles::AddonsPushLevel1(const TilesAddon & ta)
{
    if(TilesAddon::ForceLevel2(ta))
	addons_level2.push_back(ta);
    else
    addons_level1.push_back(ta);
}

void Maps::Tiles::AddonsPushLevel2(const MP2::mp2tile_t & mt)
{
    if(mt.objectName2 && mt.indexName2 < 0xFF)
	AddonsPushLevel2(TilesAddon(0, mt.uniqNumber2, mt.objectName2, mt.indexName2));
}

void Maps::Tiles::AddonsPushLevel2(const MP2::mp2addon_t & ma)
{
    if(ma.objectNameN2 && ma.indexNameN2 < 0xFF)
	AddonsPushLevel2(TilesAddon(ma.quantityN, ma.uniqNumberN2, ma.objectNameN2, ma.indexNameN2));
}

void Maps::Tiles::AddonsPushLevel2(const TilesAddon & ta)
{
    if(TilesAddon::ForceLevel1(ta))
	addons_level1.push_back(ta);
    else
	addons_level2.push_back(ta);
}

void Maps::Tiles::AddonsSort(void)
{
    if(!addons_level1.empty()) addons_level1.sort(TilesAddon::PredicateSortRules1);
    if(!addons_level2.empty()) addons_level2.sort(TilesAddon::PredicateSortRules2);
}

int Maps::Tiles::GetGround(void) const
{
    const u32 index = TileSpriteIndex();

    // list grounds from GROUND32.TIL
    if(30 > index)
        return Maps::Ground::WATER;
    else
    if(92 > index)
        return Maps::Ground::GRASS;
    else
    if(146 > index)
        return Maps::Ground::SNOW;
    else
    if(208 > index)
        return Maps::Ground::SWAMP;
    else
    if(262 > index)
        return Maps::Ground::LAVA;
    else
    if(321 > index)
        return Maps::Ground::DESERT;
    else
    if(361 > index)
        return Maps::Ground::DIRT;
    else
    if(415 > index)
        return Maps::Ground::WASTELAND;

    //else if(432 > pack_sprite_index)

    return Maps::Ground::BEACH;
}

bool Maps::Tiles::isWater(void) const
{
    return 30 > TileSpriteIndex();
}

void Maps::Tiles::Remove(u32 uniq)
{
    if(!addons_level1.empty()) addons_level1.Remove(uniq);
    if(!addons_level2.empty()) addons_level2.Remove(uniq);
}

void Maps::Tiles::RedrawTile(Surface & dst) const
{
    const Interface::GameArea & area = Interface::Basic::Get().GetGameArea();
    const Point mp = Maps::GetPoint(GetIndex());

    if(area.GetRectMaps() & mp)
	area.BlitOnTile(dst, GetTileSurface(), 0, 0, mp);
}

void Maps::Tiles::RedrawBottom(Surface & dst, bool skip_objs) const
{
    const Interface::GameArea & area = Interface::Basic::Get().GetGameArea();
    const Point mp = Maps::GetPoint(GetIndex());

    if((area.GetRectMaps() & mp) &&
	!addons_level1.empty())
    {
	for(Addons::const_iterator
	    it = addons_level1.begin(); it != addons_level1.end(); ++it)
	{
	    // skip
	    if(skip_objs &&
		MP2::isRemoveObject(GetObject()) &&
		FindObjectConst(GetObject()) == &(*it)) continue;

	    const u8 & object = (*it).object;
	    const u8 & index  = (*it).index;
	    const int icn = MP2::GetICNObject(object);

	    if(ICN::UNKNOWN != icn && ICN::MINIHERO != icn && ICN::MONS32 != icn)
	    {
		const Sprite & sprite = AGG::GetICN(icn, index);
		area.BlitOnTile(dst, sprite, mp);

		// possible anime
		if(u32 anime_index = ICN::AnimationFrame(icn, index, Game::MapsAnimationFrame(), quantity2))
		{
		    const Sprite & anime_sprite = AGG::GetICN(icn, anime_index);
		    area.BlitOnTile(dst, anime_sprite, mp);
		}
	    }
	}
    }
}

void Maps::Tiles::RedrawPassable(Surface & dst) const
{
#ifdef WITH_DEBUG
    const Interface::GameArea & area = Interface::Basic::Get().GetGameArea();
    const Point mp = Maps::GetPoint(GetIndex());

    if(area.GetRectMaps() & mp)
    {
	if(0 == tile_passable ||
	   DIRECTION_ALL != tile_passable)
	{
	    Surface sf = PassableViewSurface(tile_passable);

	    if(passable_disable)
	    {
		Text text(GetString(passable_disable), Font::SMALL);
		text.Blit(13, 13, sf);
	    }

    	    area.BlitOnTile(dst, sf, 0, 0, mp);
	}
    }
#endif
}

void Maps::Tiles::RedrawObjects(Surface & dst) const
{
    switch(GetObject())
    {
        // boat
        case MP2::OBJ_BOAT:	RedrawBoat(dst); break;
        // monster
        case MP2::OBJ_MONSTER:	RedrawMonster(dst); break;
	//
	default: break;
    }
}

void Maps::Tiles::RedrawMonster(Surface & dst) const
{
    const Settings & conf = Settings::Get();
    const Point mp = Maps::GetPoint(GetIndex());
    const Interface::GameArea & area = Interface::Basic::Get().GetGameArea();
    s32 dst_index = -1;

    if(!(area.GetRectMaps() & mp)) return;

    // scan hero around
    const MapsIndexes & v = ScanAroundObject(GetIndex(), MP2::OBJ_HEROES);
    for(MapsIndexes::const_iterator
	it = v.begin(); it != v.end(); ++it)
    {
	const Tiles & tile = world.GetTiles(*it);
	dst_index = *it;

	if(MP2::OBJ_HEROES != mp2_object ||
	    // skip bottom, bottom_right, bottom_left with ground objects
	    ((DIRECTION_BOTTOM_ROW & Direction::Get(GetIndex(), *it)) && 
			    MP2::isGroundObject(tile.GetObject(false))) ||
	    // skip ground check
	    (tile.isWater() != isWater()))
	    dst_index = -1;
	else
	    break;
    }

    const u32 sprite_index = QuantityMonster().GetSpriteIndex();

    // draw attack sprite
    if(-1 != dst_index && !conf.ExtWorldOnlyFirstMonsterAttack())
    {
	bool revert = false;

	switch(Direction::Get(GetIndex(), dst_index))
	{
	    case Direction::TOP_LEFT:
	    case Direction::LEFT:
	    case Direction::BOTTOM_LEFT:
	    	revert = true;
	    	break;
	    default: break;
	}

	const Sprite & sprite_first = AGG::GetICN(ICN::MINIMON, sprite_index * 9 + (revert ? 8 : 7));
	area.BlitOnTile(dst, sprite_first, sprite_first.x() + 16, TILEWIDTH + sprite_first.y(), mp);
    }
    else
    {
	// draw first sprite
	const Sprite & sprite_first = AGG::GetICN(ICN::MINIMON, sprite_index * 9);
	area.BlitOnTile(dst, sprite_first, sprite_first.x() + 16, TILEWIDTH + sprite_first.y(), mp);

	// draw second sprite
	const Sprite & sprite_next = AGG::GetICN(ICN::MINIMON, sprite_index * 9 + 1 + 
	    monster_animation_cicle[ (Game::MapsAnimationFrame() + mp.x * mp.y) % ARRAY_COUNT(monster_animation_cicle) ]);
	area.BlitOnTile(dst, sprite_next, sprite_next.x() + 16, TILEWIDTH + sprite_next.y(), mp);
    }
}

void Maps::Tiles::RedrawBoat(Surface & dst) const
{
    const Point mp = Maps::GetPoint(GetIndex());
    const Interface::GameArea & area = Interface::Basic::Get().GetGameArea();

    if(area.GetRectMaps() & mp)
    {
	// FIXME: restore direction from Maps::Tiles
	const Sprite & sprite = AGG::GetICN(ICN::BOAT32, 18);
	area.BlitOnTile(dst, sprite, sprite.x(), TILEWIDTH + sprite.y(), mp);
    }
}

bool SkipRedrawTileBottom4Hero(const Maps::TilesAddon & ta, int passable)
{
    if(Maps::TilesAddon::isStream(ta) || Maps::TilesAddon::isRoad(ta))
	return true;
    else
    switch(MP2::GetICNObject(ta.object))
    {
	case ICN::UNKNOWN:
	case ICN::MINIHERO:
	case ICN::MONS32:
	    return true;

    	case ICN::OBJNWATR:
	    return ta.index >= 202 && ta.index <= 225; /* whirlpool */

    	case ICN::OBJNTWBA:
    	case ICN::ROAD:
    	case ICN::STREAM:
	    return true;

	case ICN::OBJNCRCK:
	    return (ta.index == 58 || ta.index == 59 || ta.index == 64 || ta.index == 65 ||
			ta.index == 188 || ta.index == 189 || (passable & DIRECTION_TOP_ROW));

	case ICN::OBJNDIRT:
	case ICN::OBJNDSRT:
	case ICN::OBJNGRA2:
	case ICN::OBJNGRAS:
	case ICN::OBJNLAVA:
	case ICN::OBJNSNOW:
	case ICN::OBJNSWMP:
	    return (passable & DIRECTION_TOP_ROW);

	default: break;
    }

    return false;
}

void Maps::Tiles::RedrawBottom4Hero(Surface & dst) const
{
    const Interface::GameArea & area = Interface::Basic::Get().GetGameArea();
    const Point mp = Maps::GetPoint(GetIndex());

    if((area.GetRectMaps() & mp) &&
	!addons_level1.empty())
    {
	for(Addons::const_iterator
	    it = addons_level1.begin(); it != addons_level1.end(); ++it)
	{
	    if(! SkipRedrawTileBottom4Hero(*it, tile_passable))
	    {
		const u8 & object = (*it).object;
		const u8 & index  = (*it).index;
		const int icn = MP2::GetICNObject(object);

		const Sprite & sprite = AGG::GetICN(icn, index);
		area.BlitOnTile(dst, sprite, mp);

		// possible anime
		if(u32 anime_index = ICN::AnimationFrame(icn, index, Game::MapsAnimationFrame(), quantity2))
		{
		    const Sprite & anime_sprite = AGG::GetICN(icn, anime_index);
		    area.BlitOnTile(dst, anime_sprite, mp);
		}
	    }
	}
    }
}

void Maps::Tiles::RedrawTop(Surface & dst, const TilesAddon* skip) const
{
    const Interface::GameArea & area = Interface::Basic::Get().GetGameArea();
    const Point mp = Maps::GetPoint(GetIndex());

    if(!(area.GetRectMaps() & mp)) return;

    // animate objects
    if(MP2::OBJ_ABANDONEDMINE == GetObject())
    {
    	const Sprite & anime_sprite = AGG::GetICN(ICN::OBJNHAUN,  Game::MapsAnimationFrame() % 15);
	area.BlitOnTile(dst, anime_sprite, mp);
    }
    else
    if(MP2::OBJ_MINES == GetObject())
    {
	const Maps::TilesAddon* addon = FindObjectConst(MP2::OBJ_MINES);
	if(addon && addon->tmp == Spell::HAUNT)
	{
    	    const Sprite & anime_sprite = AGG::GetICN(ICN::OBJNHAUN,  Game::MapsAnimationFrame() % 15);
	    area.BlitOnTile(dst, anime_sprite, mp);
	}
	else
	if(addon && addon->tmp >= Spell::SETEGUARDIAN && addon->tmp <= Spell::SETWGUARDIAN)
	{
	    area.BlitOnTile(dst,
		AGG::GetICN(ICN::MONS32, Monster(Spell(addon->tmp)).GetSpriteIndex()), TILEWIDTH, 0, mp);
	}
    }

    if(!addons_level2.empty())
    {
	for(Addons::const_iterator
	    it = addons_level2.begin(); it != addons_level2.end(); ++it)
	{
	    if(skip && skip == &(*it)) continue;

	    const u8 & object = (*it).object;
	    const u8 & index  = (*it).index;
	    const int icn = MP2::GetICNObject(object);

	    if(ICN::UNKNOWN != icn && ICN::MINIHERO != icn && ICN::MONS32 != icn)
	    {
		const Sprite & sprite = AGG::GetICN(icn, index);
		area.BlitOnTile(dst, sprite, mp);

		// possible anime
		if(u32 anime_index = ICN::AnimationFrame(icn, index, Game::MapsAnimationFrame()))
		{
		    const Sprite & anime_sprite = AGG::GetICN(icn, anime_index);
		    area.BlitOnTile(dst, anime_sprite, mp);
		}
	    }
	}
    }
}

void Maps::Tiles::RedrawTop4Hero(Surface & dst, bool skip_ground) const
{
    const Interface::GameArea & area = Interface::Basic::Get().GetGameArea();
    const Point mp = Maps::GetPoint(GetIndex());

    if((area.GetRectMaps() & mp) &&
	!addons_level2.empty())
    {
	for(Addons::const_iterator
	    it = addons_level2.begin(); it != addons_level2.end(); ++it)
	{
	    if(skip_ground && MP2::isGroundObject((*it).object)) continue;

	    const u8 & object = (*it).object;
	    const u8 & index  = (*it).index;
	    const int icn = MP2::GetICNObject(object);

	    if(ICN::HighlyObjectSprite(icn, index))
	    {
		const Sprite & sprite = AGG::GetICN(icn, index);
		area.BlitOnTile(dst, sprite, mp);

		// possible anime
		if(u32 anime_index = ICN::AnimationFrame(icn, index, Game::MapsAnimationFrame()))
		{
		    const Sprite & anime_sprite = AGG::GetICN(icn, anime_index);
		    area.BlitOnTile(dst, anime_sprite, mp);
		}
	    }
	}
    }
}

Maps::TilesAddon* Maps::Tiles::FindAddonICN1(int icn1)
{
    Addons::iterator it = std::find_if(addons_level1.begin(), addons_level1.end(),
			    std::bind2nd(std::mem_fun_ref(&Maps::TilesAddon::isICN), icn1));

    return it != addons_level1.end() ? &(*it) : NULL;
}

Maps::TilesAddon* Maps::Tiles::FindAddonICN2(int icn2)
{
    Addons::iterator it = std::find_if(addons_level2.begin(), addons_level2.end(),
			    std::bind2nd(std::mem_fun_ref(&Maps::TilesAddon::isICN), icn2));

    return it != addons_level2.end() ? &(*it) : NULL;
}

Maps::TilesAddon* Maps::Tiles::FindAddonLevel1(u32 uniq1)
{
    Addons::iterator it = std::find_if(addons_level1.begin(), addons_level1.end(),
			    std::bind2nd(std::mem_fun_ref(&Maps::TilesAddon::isUniq), uniq1));

    return it != addons_level1.end() ? &(*it) : NULL;
}

Maps::TilesAddon* Maps::Tiles::FindAddonLevel2(u32 uniq2)
{
    Addons::iterator it = std::find_if(addons_level2.begin(), addons_level2.end(),
			    std::bind2nd(std::mem_fun_ref(&Maps::TilesAddon::isUniq), uniq2));

    return it != addons_level2.end() ? &(*it) : NULL;
}


std::string Maps::Tiles::String(void) const
{
    std::ostringstream os;

    os <<
	"----------------:--------" << std::endl <<
	"maps index      : " << GetIndex() << ", " << GetString(GetCenter()) << std::endl <<
	"tile index      : " << TileSpriteIndex() << std::endl <<
	"ground          : " << Ground::String(GetGround());
    if(isRoad())
    {
	Addons::const_iterator it = std::find_if(addons_level1.begin(), addons_level1.end(),
				std::bind2nd(std::mem_fun_ref(&TilesAddon::isRoad), DIRECTION_ALL));
	os << ", (" << "road";
	if(ICN::ROAD == MP2::GetICNObject((*it).object))
	    os << ", " << "index: " << static_cast<int>((*it).index);
	os << ")";
    }
    os << std::endl <<
	"passable        : " << (tile_passable ? Direction::String(tile_passable) : "false");
#ifdef WITH_DEBUG
	if(passable_disable)
	os << ", disable(" << static_cast<int>(passable_disable) << ")";
#endif
    os <<
	std::endl <<
	"mp2 object      : " << "0x" << std::setw(2) << std::setfill('0') << static_cast<int>(GetObject()) <<
				    ", (" << MP2::StringObject(GetObject()) << ")" << std::endl <<
	"quantity 1      : " << static_cast<int>(quantity1) << std::endl <<
	"quantity 2      : " << static_cast<int>(quantity2) << std::endl <<
	"quantity 3      : " << static_cast<int>(GetQuantity3()) << std::endl;

    for(Addons::const_iterator
	it = addons_level1.begin(); it != addons_level1.end(); ++it)
	os << (*it).String(1);

    for(Addons::const_iterator
	it = addons_level2.begin(); it != addons_level2.end(); ++it)
	os << (*it).String(2);

    os <<
	"----------------I--------" << std::endl;

    // extra obj info
    switch(GetObject())
    {
	// dwelling
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREECITY:
        case MP2::OBJ_WAGONCAMP:
        case MP2::OBJ_DESERTTENT:
	case MP2::OBJ_TROLLBRIDGE:
        case MP2::OBJ_DRAGONCITY:
        case MP2::OBJ_CITYDEAD:
	//
        case MP2::OBJ_WATCHTOWER:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_TREEHOUSE:
        case MP2::OBJ_ARCHERHOUSE:
        case MP2::OBJ_GOBLINHUT:
        case MP2::OBJ_DWARFCOTT:
        case MP2::OBJ_HALFLINGHOLE:
        case MP2::OBJ_PEASANTHUT:
        case MP2::OBJ_THATCHEDHUT:
	//
	case MP2::OBJ_MONSTER:
	    os << "count           : " << MonsterCount() << std::endl;
	    break;

	case MP2::OBJ_HEROES:
	    {
		const Heroes* hero = GetHeroes();
		if(hero) os << hero->String();
	    }
	    break;

	case MP2::OBJN_CASTLE:
	case MP2::OBJ_CASTLE:
	    {
		const Castle* castle = world.GetCastle(GetCenter());
		if(castle) os << castle->String();
	    }
	    break;

	default:
	{
	    const MapsIndexes & v = Maps::GetTilesUnderProtection(GetIndex());
	    if(v.size())
	    {
		os << "protection      : ";
		for(MapsIndexes::const_iterator
		    it = v.begin(); it != v.end(); ++it)
		    os << *it << ", ";
		os << std::endl;
	    }
	    break;
	}
    }

    if(MP2::isCaptureObject(GetObject(false)))
    {
	const CapturedObject & co = world.GetCapturedObject(GetIndex());

	os <<
	"capture color   : " << Color::String(co.objcol.second) << std::endl;
	if(co.guardians.isValid())
	{
	    os <<
	    "capture guard   : " << co.guardians.GetName() << std::endl <<
	    "capture caunt   : " << co.guardians.GetCount() << std::endl;
	}
    }

    os << "----------------:--------" << std::endl;
    return os.str();
}

void Maps::Tiles::FixObject(void)
{
    if(MP2::OBJ_ZERO == mp2_object)
    {
	if(addons_level1.end() != std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isArtifact))
	    SetObject(MP2::OBJ_ARTIFACT);
	else
	if(addons_level1.end() != std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isResource))
	    SetObject(MP2::OBJ_RESOURCE);
    }
}

bool Maps::Tiles::GoodForUltimateArtifact(void) const
{
    return ! isWater() && (addons_level1.empty() ||
		addons_level1.size() == static_cast<size_t>(std::count_if(addons_level1.begin(), addons_level1.end(),
							std::ptr_fun(&TilesAddon::isShadow)))) &&
	    isPassable(NULL, Direction::CENTER, true);
}

bool TileIsGround(s32 index, int ground)
{
    return ground == world.GetTiles(index).GetGround();
}

/* accept move */
bool Maps::Tiles::isPassable(const Heroes & hero) const
{
    if(hero.isShipMaster())
    {
	if(! isWater())
	    return false;

	if(MP2::OBJ_BOAT == GetObject())
	    return false;
    }
    else
    // if(! hero->isShipMaster() &&
    if(isWater())
    {
	switch(GetObject())
	{
	    // fix shipwreck: place on water
	    case MP2::OBJ_SHIPWRECK:
		// check later
		break;

	    // for: meetings/attack hero
	    case MP2::OBJ_HEROES:
	    {
		// scan ground
		const MapsIndexes & v = Maps::GetAroundIndexes(GetIndex());
		if(v.end() == std::find_if(v.begin(), v.end(),
		    std::not1(std::bind2nd(std::ptr_fun(&TileIsGround), static_cast<int>(Ground::WATER)))))
		    return false;
	    }
	    break;

	    default:
		// ! hero->isShipMaster() && isWater()
		return false;
	}
    }

    return true;
}

bool Maps::Tiles::isPassable(const Heroes* hero, int direct, bool skipfog) const
{
    if(!skipfog && isFog(Settings::Get().CurrentColor()))
	return false;

    if(hero && ! isPassable(*hero))
	return false;

    return direct & tile_passable;
}

void Maps::Tiles::SetObjectPassable(bool pass)
{
    switch(GetObject(false))
    {
	case MP2::OBJ_TROLLBRIDGE:
	    if(pass)
		tile_passable |= Direction::TOP_LEFT;
	    else
		tile_passable &= ~Direction::TOP_LEFT;
	    break;

	default:
	    break;
    }
}

/* check road */
bool Maps::Tiles::isRoad(int direct) const
{
    return addons_level1.end() != std::find_if(addons_level1.begin(), addons_level1.end(),
				std::bind2nd(std::mem_fun_ref(&TilesAddon::isRoad), direct));
}

bool Maps::Tiles::isStream(void) const
{
    return addons_level1.end() != std::find_if(addons_level1.begin(), addons_level1.end(),
				TilesAddon::isStream);
}

Maps::TilesAddon* Maps::Tiles::FindObject(int objs)
{
    return const_cast<Maps::TilesAddon*>(FindObjectConst(objs));
}

const Maps::TilesAddon* Maps::Tiles::FindObjectConst(int objs) const
{
    Addons::const_iterator it = addons_level1.size() ? addons_level1.begin() : addons_level1.end();

    switch(objs)
    {
	case MP2::OBJ_CAMPFIRE:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isCampFire);
	    break;

        case MP2::OBJ_TREASURECHEST:
        case MP2::OBJ_ANCIENTLAMP:
        case MP2::OBJ_RESOURCE:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isResource);
	    break;

        case MP2::OBJ_RNDRESOURCE:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isRandomResource);
	    break;

        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_SHIPWRECKSURVIROR:
        case MP2::OBJ_WATERCHEST:
        case MP2::OBJ_BOTTLE:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isWaterResource);
	    break;

	case MP2::OBJ_ARTIFACT:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isArtifact);
	    break;

	case MP2::OBJ_RNDARTIFACT:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isRandomArtifact);
	    break;

	case MP2::OBJ_RNDARTIFACT1:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isRandomArtifact1);
	    break;

	case MP2::OBJ_RNDARTIFACT2:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isRandomArtifact2);
	    break;

	case MP2::OBJ_RNDARTIFACT3:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isRandomArtifact3);
	    break;

	case MP2::OBJ_RNDULTIMATEARTIFACT:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isUltimateArtifact);
	    break;

	case MP2::OBJ_MONSTER:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isMonster);
	    break;

	case MP2::OBJ_WHIRLPOOL:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isWhirlPool);
	    break;

	case MP2::OBJ_STANDINGSTONES:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isStandingStone);
	    break;

	case MP2::OBJ_ARTESIANSPRING:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isArtesianSpring);
	    break;

	case MP2::OBJ_OASIS:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isOasis);
	    break;

	case MP2::OBJ_WATERINGHOLE:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isWateringHole);
	    break;

	case MP2::OBJ_MINES:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isMine);
	    break;

	case MP2::OBJ_JAIL:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isJail);
	    break;

	case MP2::OBJ_EVENT:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isEvent);
	    break;

	case MP2::OBJ_BOAT:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isBoat);
	    break;

	case MP2::OBJ_BARRIER:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isBarrier);
	    break;

	case MP2::OBJ_HEROES:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isMiniHero);
	    break;

	case MP2::OBJ_CASTLE:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isCastle);
	    if(it == addons_level1.end())
	    {
		it = std::find_if(addons_level2.begin(), addons_level2.end(), TilesAddon::isCastle);
		return addons_level2.end() != it ? &(*it) : NULL;
	    }
	    break;

	case MP2::OBJ_RNDCASTLE:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isRandomCastle);
	    if(it == addons_level1.end())
	    {
		it = std::find_if(addons_level2.begin(), addons_level2.end(), TilesAddon::isRandomCastle);
		return addons_level2.end() != it ? &(*it) : NULL;
	    }
	    break;

	case MP2::OBJ_RNDMONSTER:
	case MP2::OBJ_RNDMONSTER1:
	case MP2::OBJ_RNDMONSTER2:
	case MP2::OBJ_RNDMONSTER3:
	case MP2::OBJ_RNDMONSTER4:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isRandomMonster);
	    break;

	case MP2::OBJ_SKELETON:
	    it = std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isSkeleton);
	    break;

	default: if(addons_level1.size() > 1) DEBUG(DBG_GAME, DBG_WARN, "FIXME for: " << MP2::StringObject(objs)); break;
    }

    return addons_level1.end() != it ? &(*it) : NULL;
}

Maps::TilesAddon* Maps::Tiles::FindFlags(void)
{
    Addons::iterator it =
	std::find_if(addons_level1.begin(), addons_level1.end(), TilesAddon::isFlag32);

    if(it == addons_level1.end())
    {
	it = std::find_if(addons_level2.begin(), addons_level2.end(), TilesAddon::isFlag32);
	return addons_level2.end() != it ? &(*it) : NULL;
    }

    return addons_level1.end() != it ? &(*it) : NULL;
}

/* ICN::FLAGS32 version */
void Maps::Tiles::CaptureFlags32(int obj, int col)
{
    u32 index = 0;

    switch(col)
    {
	case Color::BLUE:	index = 0; break;
	case Color::GREEN:	index = 1; break;
	case Color::RED:	index = 2; break;
	case Color::YELLOW:	index = 3; break;
	case Color::ORANGE:	index = 4; break;
	case Color::PURPLE:	index = 5; break;
	default:		index = 6; break;
    }

    switch(obj)
    {
	case MP2::OBJ_WINDMILL:		index += 42; CorrectFlags32(index, false); break;
	case MP2::OBJ_WATERWHEEL:	index += 14; CorrectFlags32(index, false); break;
	case MP2::OBJ_MAGICGARDEN:	index += 42; CorrectFlags32(index, false); break;

	case MP2::OBJ_MINES:		index += 14; CorrectFlags32(index, true); break;
	//case MP2::OBJ_DRAGONCITY:	index += 35; CorrectFlags32(index); break; unused
        case MP2::OBJ_LIGHTHOUSE:	index += 42; CorrectFlags32(index, false); break;

	case MP2::OBJ_ALCHEMYLAB:
	{
	    index += 21;
	    if(Maps::isValidDirection(GetIndex(), Direction::TOP))
	    {
		Maps::Tiles & tile = world.GetTiles(Maps::GetDirectionIndex(GetIndex(), Direction::TOP));
		tile.CorrectFlags32(index, true);
	    }
	}
	break;

	case MP2::OBJ_SAWMILL:
	{
	    index += 28;
	    if(Maps::isValidDirection(GetIndex(), Direction::TOP_RIGHT))
    	    {
    		Maps::Tiles & tile = world.GetTiles(Maps::GetDirectionIndex(GetIndex(), Direction::TOP_RIGHT));
    		tile.CorrectFlags32(index, true);
	    }
	}
	break;

	case MP2::OBJ_CASTLE:
	{
	    index *= 2;
	    if(Maps::isValidDirection(GetIndex(), Direction::LEFT))
	    {
		Maps::Tiles & tile = world.GetTiles(Maps::GetDirectionIndex(GetIndex(), Direction::LEFT));
    		tile.CorrectFlags32(index, true);
	    }

	    index += 1;
	    if(Maps::isValidDirection(GetIndex(), Direction::RIGHT))
	    {
		Maps::Tiles & tile = world.GetTiles(Maps::GetDirectionIndex(GetIndex(), Direction::RIGHT));
    		tile.CorrectFlags32(index, true);
	    }
	}
	break;

	default: break;
    }
}

/* correct flags, ICN::FLAGS32 vesion */
void Maps::Tiles::CorrectFlags32(u32 index, bool up)
{
    TilesAddon* taddon = FindFlags();

    // replace flag
    if(taddon)
	taddon->index = index;
    else
    if(up)
	// or new flag
	addons_level2.push_back(TilesAddon(TilesAddon::UPPER, world.GetUniq(), 0x38, index));
    else
	// or new flag
	addons_level1.push_back(TilesAddon(TilesAddon::UPPER, world.GetUniq(), 0x38, index));
}

void Maps::Tiles::FixedPreload(Tiles & tile)
{
    Addons::iterator it;

    // fix skeleton: left position
    it = std::find_if(tile.addons_level1.begin(), tile.addons_level1.end(), 
		    TilesAddon::isSkeletonFix);

    if(it != tile.addons_level1.end())
    {
	tile.SetObject(MP2::OBJN_SKELETON);
    }

    // fix price loyalty objects.
    if(Settings::Get().PriceLoyaltyVersion())
    switch(tile.GetObject())
    {
	case MP2::OBJ_UNKNW_79:
        case MP2::OBJ_UNKNW_7A:
        case MP2::OBJ_UNKNW_F9:
        case MP2::OBJ_UNKNW_FA:
	{
	    int newobj = MP2::OBJ_ZERO;
	    it = std::find_if(tile.addons_level1.begin(), tile.addons_level1.end(),
								    TilesAddon::isX_LOC123);
	    if(it != tile.addons_level1.end())
	    {
		newobj = TilesAddon::GetLoyaltyObject(*it);
	    }
	    else
	    {
		it = std::find_if(tile.addons_level2.begin(), tile.addons_level2.end(),
							TilesAddon::isX_LOC123);
		if(it != tile.addons_level2.end())
		    newobj = TilesAddon::GetLoyaltyObject(*it);
	    }

	    if(MP2::OBJ_ZERO != newobj)
		tile.SetObject(newobj);
	    else
	    {
		DEBUG(DBG_GAME, DBG_WARN, "index: " << tile.GetIndex());
	    }
	} break;

	default: break;
    }
}

/* true: if protection or has guardians */
bool Maps::Tiles::CaptureObjectIsProtection(void) const
{
    const int object = GetObject(false);

    if(MP2::isCaptureObject(object))
    {
	if(MP2::OBJ_CASTLE == object)
	{
	    Castle* castle = world.GetCastle(GetCenter());
	    if(castle)
		return castle->GetArmy().isValid();
	}
	else
	    return QuantityTroop().isValid();
    }

    return false;
}

void Maps::Tiles::RemoveObjectSprite(void)
{
    Maps::TilesAddon *addon = NULL;

    switch(GetObject())
    {
	case MP2::OBJ_WATERCHEST:
	case MP2::OBJ_BOTTLE:
	case MP2::OBJ_FLOTSAM:
	case MP2::OBJ_SHIPWRECKSURVIROR:
	case MP2::OBJ_TREASURECHEST:
	case MP2::OBJ_ANCIENTLAMP:
	case MP2::OBJ_RESOURCE:
	case MP2::OBJ_ARTIFACT:
	case MP2::OBJ_CAMPFIRE:		addon = FindObject(GetObject()); break;

	case MP2::OBJ_JAIL:		RemoveJailSprite(); tile_passable = DIRECTION_ALL; break;
	case MP2::OBJ_BARRIER:		RemoveBarrierSprite(); tile_passable = DIRECTION_ALL; break;

	default: break;
    }

    if(addon)
    {
        // remove shadow sprite from left cell
        if(Maps::isValidDirection(GetIndex(), Direction::LEFT))
    	    world.GetTiles(Maps::GetDirectionIndex(GetIndex(), Direction::LEFT)).Remove(addon->uniq);

	Remove(addon->uniq);
    }
}

void Maps::Tiles::RemoveBarrierSprite(void)
{
    Maps::TilesAddon *addon = FindObject(MP2::OBJ_BARRIER);

    if(addon)
    {
        // remove left sprite
        if(Maps::isValidDirection(GetIndex(), Direction::LEFT))
    	{
	    const s32 left = Maps::GetDirectionIndex(GetIndex(), Direction::LEFT);
	    world.GetTiles(left).Remove(addon->uniq);
	}

	Remove(addon->uniq);
    }
}

void Maps::Tiles::RemoveJailSprite(void)
{
    Maps::TilesAddon *addon = FindObject(MP2::OBJ_JAIL);

    if(addon)
    {
        // remove left sprite
        if(Maps::isValidDirection(GetIndex(), Direction::LEFT))
    	{
	    const s32 left = Maps::GetDirectionIndex(GetIndex(), Direction::LEFT);
	    world.GetTiles(left).Remove(addon->uniq);

    	    // remove left left sprite
    	    if(Maps::isValidDirection(left, Direction::LEFT))
    		world.GetTiles(Maps::GetDirectionIndex(left, Direction::LEFT)).Remove(addon->uniq);
	}

        // remove top sprite
        if(Maps::isValidDirection(GetIndex(), Direction::TOP))
    	{
	    const s32 top = Maps::GetDirectionIndex(GetIndex(), Direction::TOP);
	    world.GetTiles(top).Remove(addon->uniq);
	    world.GetTiles(top).SetObject(MP2::OBJ_ZERO);
	    world.GetTiles(top).FixObject();

    	    // remove top left sprite
    	    if(Maps::isValidDirection(top, Direction::LEFT))
    	    {
		world.GetTiles(Maps::GetDirectionIndex(top, Direction::LEFT)).Remove(addon->uniq);
		world.GetTiles(Maps::GetDirectionIndex(top, Direction::LEFT)).SetObject(MP2::OBJ_ZERO);
		world.GetTiles(Maps::GetDirectionIndex(top, Direction::LEFT)).FixObject();
	    }
	}

	Remove(addon->uniq);
    }
}

void Maps::Tiles::UpdateAbandoneMineSprite(Tiles & tile)
{
    Addons::iterator it = std::find_if(tile.addons_level1.begin(), tile.addons_level1.end(),
						    TilesAddon::isAbandoneMineSprite);
    u32 uniq = it != tile.addons_level1.end() ? (*it).uniq : 0;

    if(uniq)
    {
	const int type = tile.QuantityResourceCount().first;

	for(Addons::iterator
	    it = tile.addons_level1.begin(); it != tile.addons_level1.end(); ++it)
		TilesAddon::UpdateAbandoneMineLeftSprite(*it, type);

	if(Maps::isValidDirection(tile.GetIndex(), Direction::RIGHT))
	{
    	    Tiles & tile2 = world.GetTiles(Maps::GetDirectionIndex(tile.GetIndex(), Direction::RIGHT));
    	    TilesAddon *mines = tile2.FindAddonLevel1(uniq);

	    if(mines) TilesAddon::UpdateAbandoneMineRightSprite(*mines);
	    if(tile2.GetObject() == MP2::OBJN_ABANDONEDMINE) tile2.SetObject(MP2::OBJN_MINES);
	}
    }

    if(Maps::isValidDirection(tile.GetIndex(), Direction::LEFT))
    {
        Tiles & tile2 = world.GetTiles(Maps::GetDirectionIndex(tile.GetIndex(), Direction::LEFT));
	if(tile2.GetObject() == MP2::OBJN_ABANDONEDMINE) tile2.SetObject(MP2::OBJN_MINES);
    }

    if(Maps::isValidDirection(tile.GetIndex(), Direction::TOP))
    {
        Tiles & tile2 = world.GetTiles(Maps::GetDirectionIndex(tile.GetIndex(), Direction::TOP));
	if(tile2.GetObject() == MP2::OBJN_ABANDONEDMINE) tile2.SetObject(MP2::OBJN_MINES);

	if(Maps::isValidDirection(tile2.GetIndex(), Direction::LEFT))
	{
    	    Tiles & tile3 = world.GetTiles(Maps::GetDirectionIndex(tile2.GetIndex(), Direction::LEFT));
	    if(tile3.GetObject() == MP2::OBJN_ABANDONEDMINE) tile3.SetObject(MP2::OBJN_MINES);
	}

	if(Maps::isValidDirection(tile2.GetIndex(), Direction::RIGHT))
	{
    	    Tiles & tile3 = world.GetTiles(Maps::GetDirectionIndex(tile2.GetIndex(), Direction::RIGHT));
	    if(tile3.GetObject() == MP2::OBJN_ABANDONEDMINE) tile3.SetObject(MP2::OBJN_MINES);
	}
    }
}

void Maps::Tiles::UpdateRNDArtifactSprite(Tiles & tile)
{
    TilesAddon *addon = NULL;
    u32 index = 0;
    Artifact art;

    switch(tile.GetObject())
    {
        case MP2::OBJ_RNDARTIFACT:
            addon = tile.FindObject(MP2::OBJ_RNDARTIFACT);
	    art = Artifact::Rand(Artifact::ART_LEVEL123);
            index = art.IndexSprite();
            break;
        case MP2::OBJ_RNDARTIFACT1:
            addon = tile.FindObject(MP2::OBJ_RNDARTIFACT1);
	    art = Artifact::Rand(Artifact::ART_LEVEL1);
            index = art.IndexSprite();
            break;
        case MP2::OBJ_RNDARTIFACT2:
            addon = tile.FindObject(MP2::OBJ_RNDARTIFACT2);
	    art = Artifact::Rand(Artifact::ART_LEVEL2);
            index = art.IndexSprite();
            break;
        case MP2::OBJ_RNDARTIFACT3:
            addon = tile.FindObject(MP2::OBJ_RNDARTIFACT3);
	    art = Artifact::Rand(Artifact::ART_LEVEL3);
            index = art.IndexSprite();
            break;
        default: return;
    }

    if(!art.isValid())
    {
    	DEBUG(DBG_GAME, DBG_INFO, "unknown artifact");
    }
    else
    if(addon)
    {
        addon->index = index;
        tile.SetObject(MP2::OBJ_ARTIFACT);

        // replace shadow artifact
        if(Maps::isValidDirection(tile.GetIndex(), Direction::LEFT))
        {
            Maps::Tiles & left_tile = world.GetTiles(Maps::GetDirectionIndex(tile.GetIndex(), Direction::LEFT));
            Maps::TilesAddon *shadow = left_tile.FindAddonLevel1(addon->uniq);

            if(shadow) shadow->index = index - 1;
        }
    }
}

void Maps::Tiles::UpdateRNDResourceSprite(Tiles & tile)
{
    TilesAddon *addon = tile.FindObject(MP2::OBJ_RNDRESOURCE);

    if(addon)
    {
        addon->index = Resource::GetIndexSprite(Resource::Rand());
        tile.SetObject(MP2::OBJ_RESOURCE);

        // replace shadow artifact
        if(Maps::isValidDirection(tile.GetIndex(), Direction::LEFT))
        {
            Maps::Tiles & left_tile = world.GetTiles(Maps::GetDirectionIndex(tile.GetIndex(), Direction::LEFT));
            Maps::TilesAddon *shadow = left_tile.FindAddonLevel1(addon->uniq);

            if(shadow) shadow->index = addon->index - 1;
        }
    }
}

void Maps::Tiles::UpdateStoneLightsSprite(Tiles & tile)
{
    for(Addons::iterator
	it = tile.addons_level1.begin(); it != tile.addons_level1.end(); ++it)
	    tile.QuantitySetTeleportType(TilesAddon::UpdateStoneLightsSprite(*it));
}

void Maps::Tiles::UpdateFountainSprite(Tiles & tile)
{
    for(Addons::iterator
	it = tile.addons_level1.begin(); it != tile.addons_level1.end(); ++it)
	    TilesAddon::UpdateFountainSprite(*it);
}

void Maps::Tiles::UpdateTreasureChestSprite(Tiles & tile)
{
    for(Addons::iterator
	it = tile.addons_level1.begin(); it != tile.addons_level1.end(); ++it)
	    TilesAddon::UpdateTreasureChestSprite(*it);
}

bool Maps::Tiles::isFog(int colors) const
{
    // colors may be the union friends
    return (fog_colors & colors) == colors;
}

void Maps::Tiles::ClearFog(int colors)
{
    fog_colors &= ~colors;
}

void Maps::Tiles::RedrawFogs(Surface & dst, int color) const
{
    const Interface::GameArea & area = Interface::Basic::Get().GetGameArea();
    const Point mp = Maps::GetPoint(GetIndex());

    // get direction around foga
    int around = 0;
    const Directions directions = Direction::All();

    for(Directions::const_iterator
        it = directions.begin(); it != directions.end(); ++it)
        if(!Maps::isValidDirection(GetIndex(), *it) ||
           world.GetTiles(Maps::GetDirectionIndex(GetIndex(), *it)).isFog(color)) around |= *it;

    if(isFog(color)) around |= Direction::CENTER;
 
    // TIL::CLOF32
    if(DIRECTION_ALL == around)
    {
	const Surface & sf = AGG::GetTIL(TIL::CLOF32, GetIndex() % 4, 0);
	area.BlitOnTile(dst, sf, 0, 0, mp);
    }
    else
    {
	u32 index = 0;
	bool revert = false;

	// see ICN::CLOP32: sprite 10
	if((around & Direction::CENTER) && !(around & (Direction::TOP | Direction::BOTTOM | Direction::LEFT | Direction::RIGHT)))
	{ index = 10; revert = false; }
	else
	// see ICN::CLOP32: sprite 6, 7, 8
	if((around & (Direction::CENTER | Direction::TOP)) && !(around & (Direction::BOTTOM | Direction::LEFT | Direction::RIGHT)))
	{ index = 6; revert = false; }
	else
	if((around & (Direction::CENTER | Direction::RIGHT)) && !(around & (Direction::TOP | Direction::BOTTOM | Direction::LEFT)))
	{ index = 7; revert = false; }
	else
	if((around & (Direction::CENTER | Direction::LEFT)) && !(around & (Direction::TOP | Direction::BOTTOM | Direction::RIGHT)))
	{ index = 7; revert = true; }
	else
	if((around & (Direction::CENTER | Direction::BOTTOM)) && !(around & (Direction::TOP | Direction::LEFT | Direction::RIGHT)))
	{ index = 8; revert = false; }
	else
	// see ICN::CLOP32: sprite 9, 29
	if((around & (DIRECTION_CENTER_COL)) && !(around & (Direction::LEFT | Direction::RIGHT)))
	{ index = 9; revert = false; }
	else
	if((around & (DIRECTION_CENTER_ROW)) && !(around & (Direction::TOP | Direction::BOTTOM)))
	{ index = 29; revert = false; }
	else
	// see ICN::CLOP32: sprite 15, 22
	if(around == (DIRECTION_ALL & (~Direction::TOP_RIGHT)))
	{ index = 15; revert = false; }
	else
	if(around == (DIRECTION_ALL & (~Direction::TOP_LEFT)))
	{ index = 15; revert = true; }
	else
	if(around == (DIRECTION_ALL & (~Direction::BOTTOM_RIGHT)))
	{ index = 22; revert = false; }
	else
	if(around == (DIRECTION_ALL & (~Direction::BOTTOM_LEFT)))
	{ index = 22; revert = true; }
	else
	// see ICN::CLOP32: sprite 16, 17, 18, 23
	if(around == (DIRECTION_ALL & (~(Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT))))
	{ index = 16; revert = false; }
	else
	if(around == (DIRECTION_ALL & (~(Direction::TOP_LEFT | Direction::BOTTOM_LEFT))))
	{ index = 16; revert = true; }
	else
	if(around == (DIRECTION_ALL & (~(Direction::TOP_RIGHT | Direction::BOTTOM_LEFT))))
	{ index = 17; revert = false; }
	else
	if(around == (DIRECTION_ALL & (~(Direction::TOP_LEFT | Direction::BOTTOM_RIGHT))))
	{ index = 17; revert = true; }
	else
	if(around == (DIRECTION_ALL & (~(Direction::TOP_LEFT | Direction::TOP_RIGHT))))
	{ index = 18; revert = false; }
	else
	if(around == (DIRECTION_ALL & (~(Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT))))
	{ index = 23; revert = false; }
	else
	// see ICN::CLOP32: sprite 13, 14
	if(around == (DIRECTION_ALL & (~DIRECTION_TOP_RIGHT_CORNER)))
	{ index = 13; revert = false; }
	else
	if(around == (DIRECTION_ALL & (~DIRECTION_TOP_LEFT_CORNER)))
	{ index = 13; revert = true; }
	else
	if(around == (DIRECTION_ALL & (~DIRECTION_BOTTOM_RIGHT_CORNER)))
	{ index = 14; revert = false; }
	else
	if(around == (DIRECTION_ALL & (~DIRECTION_BOTTOM_LEFT_CORNER)))
	{ index = 14; revert = true; }
	else
	// see ICN::CLOP32: sprite 11, 12
	if((around & (Direction::CENTER | Direction::LEFT | Direction::BOTTOM_LEFT | Direction::BOTTOM)) &&
	 !(around & (Direction::TOP | Direction::TOP_RIGHT | Direction::RIGHT)))
	{ index = 11; revert = false; }
	else
	if((around & (Direction::CENTER | Direction::RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM)) &&
	 !(around & (Direction::TOP | Direction::TOP_LEFT | Direction::LEFT)))
	{ index = 11; revert = true; }
	else
	if((around & (Direction::CENTER | Direction::LEFT | Direction::TOP_LEFT | Direction::TOP)) &&
	 !(around & (Direction::BOTTOM | Direction::BOTTOM_RIGHT | Direction::RIGHT)))
	{ index = 12; revert = false; }
	else
	if((around & (Direction::CENTER | Direction::RIGHT | Direction::TOP_RIGHT | Direction::TOP)) &&
	 !(around & (Direction::BOTTOM | Direction::BOTTOM_LEFT | Direction::LEFT)))
	{ index = 12; revert = true; }
	else
	// see ICN::CLOP32: sprite 19, 20, 22
	if((around & (DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::TOP_LEFT)) &&
	 !(around & (Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT | Direction::TOP_RIGHT)))
	{ index = 19; revert = false; }
	else
	if((around & (DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::TOP_RIGHT)) &&
	 !(around & (Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT | Direction::TOP_LEFT)))
	{ index = 19; revert = true; }
	else
	if((around & (DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::BOTTOM_LEFT)) &&
	 !(around & (Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::TOP_LEFT)))
	{ index = 20; revert = false; }
	else
	if((around & (DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP | Direction::BOTTOM_RIGHT)) &&
	 !(around & (Direction::TOP_RIGHT | Direction::BOTTOM_LEFT | Direction::TOP_LEFT)))
	{ index = 20; revert = true; }
	else
	if((around & (DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::TOP)) &&
	 !(around & (Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM_LEFT | Direction::TOP_LEFT)))
	{ index = 22; revert = false; }
	else
	// see ICN::CLOP32: sprite 24, 25, 26, 30
	if((around & (DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::BOTTOM_LEFT)) &&
	 !(around & (Direction::TOP | Direction::BOTTOM_RIGHT)))
	{ index = 24; revert = false; }
	else
	if((around & (DIRECTION_CENTER_ROW | Direction::BOTTOM | Direction::BOTTOM_RIGHT)) &&
	 !(around & (Direction::TOP | Direction::BOTTOM_LEFT)))
	{ index = 24; revert = true; }
	else
	if((around & (DIRECTION_CENTER_COL | Direction::LEFT | Direction::TOP_LEFT)) &&
	 !(around & (Direction::RIGHT | Direction::BOTTOM_LEFT)))
	{ index = 25; revert = false; }
	else
	if((around & (DIRECTION_CENTER_COL | Direction::RIGHT | Direction::TOP_RIGHT)) &&
	 !(around & (Direction::LEFT | Direction::BOTTOM_RIGHT)))
	{ index = 25; revert = true; }
	else
	if((around & (DIRECTION_CENTER_COL | Direction::BOTTOM_LEFT | Direction::LEFT)) &&
	 !(around & (Direction::RIGHT | Direction::TOP_LEFT)))
	{ index = 26; revert = false; }
	else
	if((around & (DIRECTION_CENTER_COL | Direction::BOTTOM_RIGHT | Direction::RIGHT)) &&
	 !(around & (Direction::LEFT | Direction::TOP_RIGHT)))
	{ index = 26; revert = true; }
	else
	if((around & (DIRECTION_CENTER_ROW | Direction::TOP_LEFT | Direction::TOP)) &&
	 !(around & (Direction::BOTTOM | Direction::TOP_RIGHT)))
	{ index = 30; revert = false; }
	else
	if((around & (DIRECTION_CENTER_ROW | Direction::TOP_RIGHT | Direction::TOP)) &&
	 !(around & (Direction::BOTTOM | Direction::TOP_LEFT)))
	{ index = 30; revert = true; }
	else
	// see ICN::CLOP32: sprite 27, 28
	if((around & (Direction::CENTER | Direction::BOTTOM | Direction::LEFT)) &&
	 !(around & (Direction::TOP | Direction::TOP_RIGHT | Direction::RIGHT | Direction::BOTTOM_LEFT)))
	{ index = 27; revert = false; }
	else
	if((around & (Direction::CENTER | Direction::BOTTOM | Direction::RIGHT)) &&
	 !(around & (Direction::TOP | Direction::TOP_LEFT | Direction::LEFT | Direction::BOTTOM_RIGHT)))
	{ index = 27; revert = true; }
	else
	if((around & (Direction::CENTER | Direction::LEFT | Direction::TOP)) &&
	 !(around & (Direction::TOP_LEFT | Direction::RIGHT | Direction::BOTTOM | Direction::BOTTOM_RIGHT)))
	{ index = 28; revert = false; }
	else
	if((around & (Direction::CENTER | Direction::RIGHT | Direction::TOP)) &&
	 !(around & (Direction::TOP_RIGHT | Direction::LEFT | Direction::BOTTOM | Direction::BOTTOM_LEFT)))
	{ index = 28; revert = true; }
	else
	// see ICN::CLOP32: sprite 31, 32, 33
	if((around & (DIRECTION_CENTER_ROW | Direction::TOP)) &&
	 !(around & (Direction::BOTTOM | Direction::TOP_LEFT | Direction::TOP_RIGHT)))
	{ index = 31; revert = false; }
	else
	if((around & (DIRECTION_CENTER_COL | Direction::RIGHT)) &&
	 !(around & (Direction::LEFT | Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT)))
	{ index = 32; revert = false; }
	else
	if((around & (DIRECTION_CENTER_COL | Direction::LEFT)) &&
	 !(around & (Direction::RIGHT | Direction::TOP_LEFT | Direction::BOTTOM_LEFT)))
	{ index = 32; revert = true; }
	else
	if((around & (DIRECTION_CENTER_ROW | Direction::BOTTOM)) &&
	 !(around & (Direction::TOP | Direction::BOTTOM_LEFT | Direction::BOTTOM_RIGHT)))
	{ index = 33; revert = false; }
	else
	// see ICN::CLOP32: sprite 0, 1, 2, 3, 4, 5
	if((around & (DIRECTION_CENTER_ROW | DIRECTION_BOTTOM_ROW)) &&
	 !(around & (Direction::TOP)))
	{ index = (GetIndex() % 2) ? 0 : 1; revert = false; }
	else
	if((around & (DIRECTION_CENTER_ROW | DIRECTION_TOP_ROW)) &&
	 !(around & (Direction::BOTTOM)))
	{ index = (GetIndex() % 2) ? 4 : 5; revert = false; }
	else
	if((around & (DIRECTION_CENTER_COL | DIRECTION_LEFT_COL)) &&
	 !(around & (Direction::RIGHT)))
	{ index = (GetIndex() % 2) ? 2 : 3; revert = false; }
	else
	if((around & (DIRECTION_CENTER_COL | DIRECTION_RIGHT_COL)) &&
	 !(around & (Direction::LEFT)))
	{ index = (GetIndex() % 2) ? 2 : 3; revert = true; }
	// unknown
	else
	{
	    const Surface & sf = AGG::GetTIL(TIL::CLOF32, GetIndex() % 4, 0);
	    area.BlitOnTile(dst, sf, 0, 0, mp);
	    return;
	}

	const Sprite & sprite = AGG::GetICN(ICN::CLOP32, index, revert);
	area.BlitOnTile(dst, sprite, (revert ? sprite.x() + TILEWIDTH - sprite.w() : sprite.x()), sprite.y(), mp);
    }
}

StreamBase & Maps::operator<< (StreamBase & msg, const TilesAddon & ta)
{
    return msg << ta.level << ta.uniq << ta.object << ta.index << ta.tmp;
}

StreamBase & Maps::operator>> (StreamBase & msg, TilesAddon & ta)
{
    return msg >> ta.level >> ta.uniq >> ta.object >> ta.index >> ta.tmp;
}

StreamBase & Maps::operator<< (StreamBase & msg, const Tiles & tile)
{
    return msg <<
	tile.maps_index <<
	tile.pack_sprite_index <<
	tile.tile_passable <<
	tile.mp2_object <<
	tile.fog_colors <<
	tile.quantity1 <<
	tile.quantity2 <<
	tile.quantity3 <<
	tile.addons_level1 <<
	tile.addons_level2;
}

StreamBase & Maps::operator>> (StreamBase & msg, Tiles & tile)
{
    return msg >>
	tile.maps_index >>
	tile.pack_sprite_index >>
	tile.tile_passable >>
	tile.mp2_object >>
	tile.fog_colors >>
	tile.quantity1 >>
	tile.quantity2 >>
	tile.quantity3 >>
	tile.addons_level1 >>
	tile.addons_level2;
}
