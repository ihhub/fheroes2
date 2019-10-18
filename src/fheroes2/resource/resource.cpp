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

#include "mp2.h"
#include "world.h"
#include "agg.h"
#include "text.h"
#include "settings.h"
#include "pairs.h"
#include "resource.h"

Funds::Funds()
    : wood(0), mercury(0), ore(0), sulfur(0), crystal(0), gems(0), gold(0)
{
}

Funds::Funds(s32 _ore, s32 _wood, s32 _mercury, s32 _sulfur, s32 _crystal, s32 _gems, s32 _gold)
    : wood(_wood), mercury(_mercury), ore(_ore), sulfur(_sulfur), crystal(_crystal), gems(_gems), gold(_gold)
{
}

Funds::Funds(int rs, u32 count) : wood(0), mercury(0), ore(0), sulfur(0), crystal(0), gems(0), gold(0)
{
    switch(rs)
    {
	case Resource::ORE:	ore = count;	break;
	case Resource::WOOD:	wood = count;	break;
	case Resource::MERCURY:	mercury = count;break;
	case Resource::SULFUR:	sulfur = count;	break;
	case Resource::GEMS:	gems = count;	break;
	case Resource::CRYSTAL:	crystal = count;break;
	case Resource::GOLD:	gold = count;	break;

	default:	DEBUG(DBG_GAME, DBG_WARN, "unknown resource"); break;
    }
}

Funds::Funds(const cost_t & cost)
    : wood(cost.wood), mercury(cost.mercury), ore(cost.ore), sulfur(cost.sulfur), crystal(cost.crystal), gems(cost.gems), gold(cost.gold)
{
}

Funds::Funds(const ResourceCount & rs)
    : wood(0), mercury(0), ore(0), sulfur(0), crystal(0), gems(0), gold(0)
{
    s32* ptr = GetPtr(rs.first);
    if(ptr) *ptr = rs.second;
}

int Resource::Rand(bool with_gold)
{
    switch(Rand::Get(1, (with_gold ? 7 : 6)))
    {
        case 1: return Resource::WOOD;
        case 2: return Resource::MERCURY;
        case 3: return Resource::ORE;
        case 4: return Resource::SULFUR;
        case 5: return Resource::CRYSTAL;
        case 6: return Resource::GEMS;
	case 7: return Resource::GOLD;
        default: break;
    }

    return Resource::UNKNOWN;
}

s32* Funds::GetPtr(int rs)
{
    switch(rs)
    {
        case Resource::ORE:     return &ore;
        case Resource::WOOD:    return &wood;
        case Resource::MERCURY: return &mercury;
        case Resource::SULFUR:  return &sulfur;
        case Resource::GEMS:    return &gems;
        case Resource::CRYSTAL: return &crystal;
        case Resource::GOLD:    return &gold;
        default: break;
    }
    return NULL;
}

s32 Funds::Get(int rs) const
{
    switch(rs)
    {
        case Resource::ORE:     return ore;
        case Resource::WOOD:    return wood;
        case Resource::MERCURY: return mercury;
        case Resource::SULFUR:  return sulfur;
        case Resource::GEMS:    return gems;
        case Resource::CRYSTAL: return crystal;
        case Resource::GOLD:    return gold;
        default: break;
    }
    return 0;
}

Funds & Funds::operator= (const cost_t & cost)
{
    wood = cost.wood;
    mercury = cost.mercury;
    ore = cost.ore;
    sulfur = cost.sulfur;
    crystal = cost.crystal;
    gems = cost.gems;
    gold = cost.gold;

    return *this;
}                                                                                                                        


// operator Funds +
Funds Funds::operator+ (const Funds & pm) const
{
    Funds res;

    res.wood = wood + pm.wood;
    res.mercury = mercury + pm.mercury;
    res.ore = ore + pm.ore;
    res.sulfur = sulfur + pm.sulfur;
    res.crystal = crystal + pm.crystal;
    res.gems = gems + pm.gems;
    res.gold = gold + pm.gold;

    return res;
}

Funds & Funds::operator+= (const Funds & pm)
{
    wood += pm.wood;
    mercury += pm.mercury;
    ore += pm.ore;
    sulfur += pm.sulfur;
    crystal += pm.crystal;
    gems += pm.gems;
    gold += pm.gold;

    return *this;
}

// operator Funds -
Funds Funds::operator- (const Funds & pm) const
{
    Funds res;

    res.wood = wood - pm.wood;
    res.mercury = mercury - pm.mercury;
    res.ore = ore - pm.ore;
    res.sulfur = sulfur - pm.sulfur;
    res.crystal = crystal - pm.crystal;
    res.gems = gems - pm.gems;
    res.gold = gold - pm.gold;

    return res;
}

Funds & Funds::operator-= (const Funds & pm)
{
    wood -= pm.wood;
    mercury -= pm.mercury;
    ore -= pm.ore;
    sulfur -= pm.sulfur;
    crystal -= pm.crystal;
    gems -= pm.gems;
    gold -= pm.gold;

    return *this;
}

// operator Funds *
Funds Funds::operator* (u32 mul) const
{
    Funds res;

    res.wood = wood * mul;
    res.mercury = mercury * mul;
    res.ore = ore * mul;
    res.sulfur = sulfur * mul;
    res.crystal = crystal * mul;
    res.gems = gems * mul;
    res.gold = gold * mul;

    return res;
}

Funds & Funds::operator*= (u32 mul)
{
    wood *= mul;
    mercury *= mul;
    ore *= mul;
    sulfur *= mul;
    crystal *= mul;
    gems *= mul;
    gold *= mul;

    return *this;
}

// operator Funds <
bool Funds::operator< (const Funds & pm) const
{
    return      wood < pm.wood &&
                mercury < pm.mercury &&
                ore < pm.ore &&
                sulfur < pm.sulfur &&
                crystal < pm.crystal &&
                gems < pm.gems &&
                gold < pm.gold;
}

// operator Funds <=
bool Funds::operator<= (const Funds & pm) const
{
    return      wood <= pm.wood &&
                mercury <= pm.mercury &&
                ore <= pm.ore &&
                sulfur <= pm.sulfur &&
                crystal <= pm.crystal &&
                gems <= pm.gems &&
                gold <= pm.gold;
}

// operator Funds >
bool Funds::operator> (const Funds & pm) const
{
    return      wood > pm.wood &&
                mercury > pm.mercury &&
                ore > pm.ore &&
                sulfur > pm.sulfur &&
                crystal > pm.crystal &&
                gems > pm.gems &&
                gold > pm.gold;
}

// operator Funds >=
bool Funds::operator>= (const Funds & pm) const
{
    return      wood >= pm.wood &&
                mercury >= pm.mercury &&
                ore >= pm.ore &&
                sulfur >= pm.sulfur &&
                crystal >= pm.crystal &&
                gems >= pm.gems &&
                gold >= pm.gold;
}

std::string Funds::String(void) const
{
    std::ostringstream os;
    os << "ore: " << ore <<
	", wood: " << wood <<
	", mercury: " << mercury <<
	", sulfur: " << sulfur <<
	", crystal: " << crystal <<
	", gems: " << gems <<
	", gold: " << gold;
    return os.str();
}

/* name resource */
const char* Resource::String(int resource)
{
    const char* res[] = { "Unknown", _("Wood"), _("Mercury"), _("Ore"), _("Sulfur"), _("Crystal"), _("Gems"), _("Gold") };

    switch(resource){
        case Resource::WOOD:	return res[1];
        case Resource::MERCURY:	return res[2];
        case Resource::ORE:	return res[3];
        case Resource::SULFUR:	return res[4];
        case Resource::CRYSTAL: return res[5];
        case Resource::GEMS:	return res[6];
	case Resource::GOLD:	return res[7];
	default: break;
    }
    
    return res[0];
}

/* return index sprite objnrsrc.icn */
u32 Resource::GetIndexSprite(int resource)
{
    switch(resource){
        case Resource::WOOD:	return  1;
        case Resource::MERCURY:	return  3;
        case Resource::ORE:	return  5;
        case Resource::SULFUR:	return  7;
        case Resource::CRYSTAL: return  9;
        case Resource::GEMS:	return  11;
	case Resource::GOLD:	return  13;
        default: 
	    DEBUG(DBG_GAME, DBG_WARN, "unknown resource");
    }

    return 0;
}

int Resource::FromIndexSprite(u32 index)
{
    switch(index)
    {
        case 1:     return WOOD;
        case 3:     return MERCURY;
	case 5:     return ORE;
        case 7:     return SULFUR;
        case 9:     return CRYSTAL;
        case 11:    return GEMS;
        case 13:    return GOLD;

	default: break;
    }

    return UNKNOWN;
}

/* return index sprite resource.icn */
u32 Resource::GetIndexSprite2(int resource)
{
    switch(resource)
    {
        case Resource::WOOD:	return  0;
        case Resource::MERCURY:	return  1;
        case Resource::ORE:	return  2;
        case Resource::SULFUR:	return  3;
        case Resource::CRYSTAL: return  4;
        case Resource::GEMS:	return  5;
	case Resource::GOLD:	return  6;
        default: 
	    DEBUG(DBG_GAME, DBG_WARN, "unknown resource");
    }

    return 0;
}

int Resource::FromIndexSprite2(u32 index)
{
    switch(index)
    {
        case 0:     return WOOD;
        case 1:     return MERCURY;
	case 2:     return ORE;
        case 3:     return SULFUR;
        case 4:     return CRYSTAL;
        case 5:     return GEMS;
        case 6:     return GOLD;

	default: break;
    }

    return UNKNOWN;
}

int Funds::GetValidItems(void) const
{
    int rs = 0;

    if(wood)	rs |= Resource::WOOD;
    if(ore)	rs |= Resource::ORE;
    if(mercury)	rs |= Resource::MERCURY;
    if(sulfur)	rs |= Resource::SULFUR;
    if(crystal)	rs |= Resource::CRYSTAL;
    if(gems)	rs |= Resource::GEMS;
    if(gold)	rs |= Resource::GOLD;

    return rs;
}

u32 Funds::GetValidItemsCount(void) const
{
    u32 result = 0;

    if(wood) ++result;
    if(ore) ++result;
    if(mercury) ++result;
    if(sulfur) ++result;
    if(crystal) ++result;
    if(gems) ++result;
    if(gold) ++result;

    return result;
}

void Funds::Reset(void)
{
    wood = 0;
    ore = 0;
    mercury = 0;
    sulfur = 0;
    crystal = 0;
    gems = 0;
    gold = 0;
}

Resource::BoxSprite::BoxSprite(const Funds & f, u32 w) : Rect(0, 0, w, 0), rs(f)
{
    const u32 count = rs.GetValidItemsCount();
    h = 4 > count ? 45 : (7 > count ? 90 : 135);
}

const Rect & Resource::BoxSprite::GetArea(void) const
{
    return *this;
}

void Resource::BoxSprite::SetPos(s32 px, s32 py)
{
    x = px;
    y = py;
}

void RedrawResourceSprite(const Surface & sf, const Point & pos,
		    u32 count, u32 width, u32 offset, s32 value)
{
    Display & display = Display::Get();
    Point dst_pt;
    Text text;

    dst_pt.x = pos.x + width / 2 + count * width;
    dst_pt.y = pos.y + offset;

    sf.Blit(dst_pt.x - sf.w() / 2, dst_pt.y - sf.h(), display);

    text.Set(GetString(value), Font::SMALL);
    text.Blit(dst_pt.x - text.w() / 2, dst_pt.y + 2);
}

void Resource::BoxSprite::Redraw(void) const
{
    const u32 valid_resource = rs.GetValidItemsCount();
    if(0 == valid_resource) return;

    u32 width = 2 < valid_resource ? w / 3 : w / valid_resource;

    u32 count = 0;
    u32 offset = 35;

    if(rs.wood)
    {
	const Sprite & sprite = AGG::GetICN(ICN::RESOURCE, 0);
	RedrawResourceSprite(sprite, Point(x, y), count, width, offset, rs.wood);
	++count;
    }

    if(rs.ore)
    {
	const Sprite & sprite = AGG::GetICN(ICN::RESOURCE, 2);
	RedrawResourceSprite(sprite, Point(x, y), count, width, offset, rs.ore);
	++count;
    }

    if(rs.mercury)
    {
	const Sprite & sprite = AGG::GetICN(ICN::RESOURCE, 1);
	RedrawResourceSprite(sprite, Point(x, y), count, width, offset, rs.mercury);
	++count;
    }

    if(2 < count){ count = 0; offset += 45; }

    if(rs.sulfur)
    {
	const Sprite & sprite = AGG::GetICN(ICN::RESOURCE, 3);
	RedrawResourceSprite(sprite, Point(x, y), count, width, offset, rs.sulfur);
	++count;
    }

    if(2 < count){ count = 0; offset += 45; }
    if(rs.crystal)
    {
	const Sprite & sprite = AGG::GetICN(ICN::RESOURCE, 4);
	RedrawResourceSprite(sprite, Point(x, y), count, width, offset, rs.crystal);
	++count;
    }

    if(2 < count){ count = 0; offset += 45; }
    if(rs.gems)
    {
	const Sprite & sprite = AGG::GetICN(ICN::RESOURCE, 5);
	RedrawResourceSprite(sprite, Point(x, y), count, width, offset, rs.gems);
	++count;
    }

    if(2 < count){ count = 0; offset += 45; }
    if(rs.gold)
    {
	const Sprite & sprite = AGG::GetICN(ICN::RESOURCE, 6);
	if(! count) width = w;
	RedrawResourceSprite(sprite, Point(x, y), count, width, offset, rs.gold);
    }
}

StreamBase & operator<< (StreamBase & msg, const Funds & res)
{
    return msg << res.wood <<
	res.mercury << res.ore << res.sulfur <<
	res.crystal << res.gems << res.gold;
}

StreamBase & operator>> (StreamBase & msg, Funds & res)
{
    return msg >> res.wood >>
	res.mercury >> res.ore >> res.sulfur >>
	res.crystal >> res.gems >> res.gold;
}

StreamBase & operator<< (StreamBase & msg, const cost_t & res)
{
    return msg << res.wood <<
	res.mercury << res.ore << res.sulfur <<
	res.crystal << res.gems << res.gold;
}

StreamBase & operator>> (StreamBase & msg, cost_t & res)
{
    return msg >> res.wood >>
	res.mercury >> res.ore >> res.sulfur >>
	res.crystal >> res.gems >> res.gold;
}
