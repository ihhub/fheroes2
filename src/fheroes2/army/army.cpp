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

#include <algorithm>
#include <functional>

#include "agg.h"
#include "settings.h"
#include "payment.h"
#include "world.h"
#include "kingdom.h"
#include "maps_tiles.h"
#include "text.h"
#include "race.h"
#include "game.h"
#include "color.h"
#include "luck.h"
#include "morale.h"
#include "speed.h"
#include "castle.h"
#include "heroes.h"
#include "heroes_base.h"
#include "tools.h"
#include "army.h"

enum armysize_t
{
    ARMY_FEW     = 1,
    ARMY_SEVERAL = 5,
    ARMY_PACK    = 10,
    ARMY_LOTS    = 20,
    ARMY_HORDE   = 50,
    ARMY_THRONG  = 100,
    ARMY_SWARM   = 250,
    ARMY_ZOUNDS  = 500,
    ARMY_LEGION  = 1000
};

armysize_t ArmyGetSize(u32 count)
{
    if(ARMY_LEGION <= count)	return ARMY_LEGION;
    else
    if(ARMY_ZOUNDS <= count)	return ARMY_ZOUNDS;
    else
    if(ARMY_SWARM <= count)	return ARMY_SWARM;
    else
    if(ARMY_THRONG <= count)	return ARMY_THRONG;
    else
    if(ARMY_HORDE <= count)	return ARMY_HORDE;
    else
    if(ARMY_LOTS <= count)	return ARMY_LOTS;
    else
    if(ARMY_PACK <= count)	return ARMY_PACK;
    else
    if(ARMY_SEVERAL <= count)	return ARMY_SEVERAL;

    return ARMY_FEW;
}

std::string Army::TroopSizeString(const Troop & troop)
{
    std::string str;

    switch(ArmyGetSize(troop.GetCount()))
    {
        default:		str = _("A few\n%{monster}"); break;
        case ARMY_SEVERAL:	str = _("Several\n%{monster}"); break;
        case ARMY_PACK:		str = _("A pack of\n%{monster}"); break;
        case ARMY_LOTS:		str = _("Lots of\n%{monster}"); break;
        case ARMY_HORDE:	str = _("A horde of\n%{monster}"); break;
        case ARMY_THRONG:	str = _("A throng of\n%{monster}"); break;
        case ARMY_SWARM:	str = _("A swarm of\n%{monster}"); break;
        case ARMY_ZOUNDS:	str = _("Zounds of\n%{monster}"); break;
        case ARMY_LEGION:	str = _("A legion of\n%{monster}"); break;
    }

    StringReplace(str, "%{monster}", StringLower(troop.GetMultiName()));
    return str;
}

std::string Army::SizeString(u32 size)
{
    const char* str_size[] = { _("army|Few"), _("army|Several"), _("army|Pack"), _("army|Lots"),
		    _("army|Horde"), _("army|Throng"), _("army|Swarm"), _("army|Zounds"), _("army|Legion") };

    switch(ArmyGetSize(size))
    {
        default: break;
        case ARMY_SEVERAL:	return str_size[1];
        case ARMY_PACK:		return str_size[2];
        case ARMY_LOTS:		return str_size[3];
        case ARMY_HORDE:	return str_size[4];
        case ARMY_THRONG:	return str_size[5];
        case ARMY_SWARM:	return str_size[6];
        case ARMY_ZOUNDS:	return str_size[7];
        case ARMY_LEGION:	return str_size[8];
    }

    return str_size[0];
}

Troops::Troops()
{
}

Troops::~Troops()
{
    for(iterator it = begin(); it != end(); ++it) delete *it;
}

size_t Troops::Size(void) const
{
    return size();
}

void Troops::Assign(const Troop* it1, const Troop* it2)
{
    Clean();

    iterator it = begin();

    while(it != end() && it1 != it2)
    {
	if(it1->isValid()) (*it)->Set(*it1);
	++it; ++it1;
    }
}

void Troops::Assign(const Troops & troops)
{
    Clean();

    iterator it1 = begin();
    const_iterator it2 = troops.begin();

    while(it1 != end() && it2 != troops.end())
    {
	if((*it2)->isValid()) (*it1)->Set(**it2);
	++it2; ++it1;
    }
}

void Troops::Insert(const Troops & troops)
{
    for(const_iterator it = troops.begin(); it != troops.end(); ++it)
	push_back(new Troop(**it));
}

void Troops::PushBack(const Monster & mons, u32 count)
{
    push_back(new Troop(mons, count));
}

void Troops::PopBack(void)
{
    if(size())
    {
	delete back();
	pop_back();
    }
}

Troop* Troops::GetTroop(size_t pos)
{
    return pos < size() ? at(pos) : NULL;
}

const Troop* Troops::GetTroop(size_t pos) const
{
    return pos < size() ? at(pos) : NULL;
}

void Troops::UpgradeMonsters(const Monster & m)
{
    for(iterator it = begin(); it != end(); ++it)
	if((*it)->isValid() && **it == m) (*it)->Upgrade();
}

u32 Troops::GetCountMonsters(const Monster & m) const
{
    u32 c = 0;

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid() && **it == m) c += (*it)->GetCount();

    return c;
}

bool Troops::isValid(void) const
{
    return end() != std::find_if(begin(), end(), std::mem_fun(&Troop::isValid));
}

u32 Troops::GetCount(void) const
{
    return std::count_if(begin(), end(), std::mem_fun(&Troop::isValid));
}

bool Troops::HasMonster(const Monster & mons) const
{
    return end() != std::find_if(begin(), end(), std::bind2nd(std::mem_fun(&Troop::isMonster), mons()));
}

bool Troops::AllTroopsIsRace(int race) const
{
    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid() && (*it)->GetRace() != race) return false;

    return true;
}

bool Troops::CanJoinTroop(const Monster & mons) const
{
    const_iterator it = std::find_if(begin(), end(), std::bind2nd(std::mem_fun(&Troop::isMonster), mons()));
    if(it == end()) it = std::find_if(begin(), end(), std::not1(std::mem_fun(&Troop::isValid)));

    return it != end();
}

bool Troops::JoinTroop(const Monster & mons, u32 count)
{
    if(mons.isValid() && count)
    {
	iterator it = std::find_if(begin(), end(), std::bind2nd(std::mem_fun(&Troop::isMonster), mons()));
	if(it == end()) it = std::find_if(begin(), end(), std::not1(std::mem_fun(&Troop::isValid)));

	if(it != end())
	{
	    if((*it)->isValid())
		(*it)->SetCount((*it)->GetCount() + count);
	    else
		(*it)->Set(mons, count);

	    DEBUG(DBG_GAME, DBG_INFO, std::dec << count << " " << (*it)->GetName());
	    return true;
	}
    }

    return false;
}

bool Troops::JoinTroop(const Troop & troop)
{
    return troop.isValid() ? JoinTroop(troop(), troop.GetCount()) : false;
}

bool Troops::CanJoinTroops(const Troops & troops2) const
{
    if(this == &troops2)
	return false;

    Army troops1;
    troops1.Insert(*this);

    for(const_iterator it = troops2.begin(); it != troops2.end(); ++it)
	if((*it)->isValid() && ! troops1.JoinTroop(**it)) return false;

    return true;
}

void Troops::JoinTroops(Troops & troops2)
{
    if(this == &troops2)
	return;

    for(iterator it = troops2.begin(); it != troops2.end(); ++it)
	if((*it)->isValid())
    {
	JoinTroop(**it);
	(*it)->Reset();
    }
}

u32 Troops::GetUniqueCount(void) const
{
    std::vector<int> monsters;
    monsters.reserve(size());

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid()) monsters.push_back((*it)->GetID());

    std::sort(monsters.begin(), monsters.end());
    monsters.resize(std::distance(monsters.begin(),
		std::unique(monsters.begin(), monsters.end())));

    return monsters.size();
}


u32 Troops::GetAttack(void) const
{
    u32 res = 0;
    u32 count = 0;

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid()){ res += static_cast<Monster*>(*it)->GetAttack(); ++count; }

    return count ? res / count : 0;
}

u32 Troops::GetDefense(void) const
{
    u32 res = 0;
    u32 count = 0;

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid()){ res += static_cast<Monster*>(*it)->GetDefense(); ++count; }

    return count ? res / count : 0;
}

u32 Troops::GetHitPoints(void) const
{
    u32 res = 0;

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid()) res += (*it)->GetHitPoints();

    return res;
}

u32 Troops::GetDamageMin(void) const
{
    u32 res = 0;
    u32 count = 0;

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid()){ res += (*it)->GetDamageMin(); ++count; }

    return count ? res / count : 0;
}

u32 Troops::GetDamageMax(void) const
{
    u32 res = 0;
    u32 count = 0;

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid()){ res += (*it)->GetDamageMax(); ++count; }

    return count ? res / count : 0;
}

u32 Troops::GetStrength(void) const
{
    u32 res = 0;

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid()) res += (*it)->GetStrength();

    return res;
}

void Troops::Clean(void)
{
    std::for_each(begin(), end(), std::mem_fun(&Troop::Reset));
}

void Troops::UpgradeTroops(const Castle & castle)
{
    for(iterator it = begin(); it != end(); ++it) if((*it)->isValid())
    {
        payment_t payment = (*it)->GetUpgradeCost();
	Kingdom & kingdom = castle.GetKingdom();

	if(castle.GetRace() == (*it)->GetRace() &&
	   castle.isBuild((*it)->GetUpgrade().GetDwelling()) &&
	   kingdom.AllowPayment(payment))
	{
    	    kingdom.OddFundsResource(payment);
            (*it)->Upgrade();
	}
    }
}

Troop* Troops::GetFirstValid(void)
{
    iterator it = std::find_if(begin(), end(), std::mem_fun(&Troop::isValid));
    return it == end() ? NULL : *it;
}

Troop* Troops::GetWeakestTroop(void)
{
    iterator first, last, lowest;

    first = begin();
    last  = end();

    while(first != last) if((*first)->isValid()) break; else ++first;

    if(first == end()) return NULL;
    lowest = first;

    if(first != last)
    while(++first != last) if((*first)->isValid() && Army::WeakestTroop(*first, *lowest)) lowest = first;

    return *lowest;
}

Troop* Troops::GetSlowestTroop(void)
{
    iterator first, last, lowest;

    first = begin();
    last  = end();

    while(first != last) if((*first)->isValid()) break; else ++first;

    if(first == end()) return NULL;
    lowest = first;

    if(first != last)
    while(++first != last) if((*first)->isValid() && Army::SlowestTroop(*first, *lowest)) lowest = first;

    return *lowest;
}

Troops Troops::GetOptimized(void) const
{
    Troops result;
    result.reserve(size());

    for(const_iterator
	it1 = begin(); it1 != end(); ++it1) if((*it1)->isValid())
    {
	iterator it2 = std::find_if(result.begin(), result.end(), std::bind2nd(std::mem_fun(&Troop::isMonster), (*it1)->GetID()));

	if(it2 == result.end())
	    result.push_back(new Troop(**it1));
	else
	    (*it2)->SetCount((*it2)->GetCount() + (*it1)->GetCount());
    }

    return result;
}

void Troops::ArrangeForBattle(bool upgrade)
{
    Troops priority = GetOptimized();

    switch(priority.size())
    {
	    case 1:
	    {
		const Monster & m = *priority.back();
		const u32 count = priority.back()->GetCount();

		Clean();

		if(49 < count)
		{
		    const u32 c = count / 5;
		    at(0)->Set(m, c);
		    at(1)->Set(m, c);
		    at(2)->Set(m, c + count - (c * 5));
		    at(3)->Set(m, c);
		    at(4)->Set(m, c);

		    if(upgrade && at(2)->isAllowUpgrade())
			at(2)->Upgrade();
		}
		else
		if(20 < count)
		{
		    const u32 c = count / 3;
		    at(1)->Set(m, c);
		    at(2)->Set(m, c + count - (c * 3));
		    at(3)->Set(m, c);

		    if(upgrade && at(2)->isAllowUpgrade())
			at(2)->Upgrade();
		}
		else
	    	    at(2)->Set(m, count);
		break;
	    }
	    case 2:
	    {
		// TODO: need modify army for 2 troops
		Assign(priority);
		break;
	    }
	    case 3:
	    {
		// TODO: need modify army for 3 troops
		Assign(priority);
		break;
	    }
	    case 4:
	    {
		// TODO: need modify army for 4 troops
		Assign(priority);
		break;
	    }
	    case 5:
	    {
		// possible change orders monster
		// store
		Assign(priority);
		break;
	    }
	    default: break;
    }
}

void Troops::JoinStrongest(Troops & troops2, bool save_last)
{
    if(this == &troops2)
	return;

    Troops priority = GetOptimized();
    priority.reserve(ARMYMAXTROOPS * 2);

    Troops priority2 = troops2.GetOptimized();
    priority.Insert(priority2);

    Clean();
    troops2.Clean();

    // sort: strongest
    std::sort(priority.begin(), priority.end(), Army::StrongestTroop);

    // weakest to army2
    while(size() < priority.size())
    {
	troops2.JoinTroop(*priority.back());
	priority.PopBack();
    }

    // save half weak of strongest to army2
    if(save_last && !troops2.isValid())
    {
	Troop & last = *priority.back();
	u32 count = last.GetCount() / 2;
	troops2.JoinTroop(last, last.GetCount() - count);
	last.SetCount(count);
    }

    // strongest to army
    while(priority.size())
    {
	JoinTroop(*priority.back());
	priority.PopBack();
    }
}

void Troops::KeepOnlyWeakest(Troops & troops2, bool save_last)
{
    if(this == &troops2)
	return;

    Troops priority = GetOptimized();
    priority.reserve(ARMYMAXTROOPS * 2);

    Troops priority2 = troops2.GetOptimized();
    priority.Insert(priority2);

    Clean();
    troops2.Clean();

    // sort: strongest
    std::sort(priority.begin(), priority.end(), Army::StrongestTroop);

    // weakest to army
    while(size() < priority.size())
    {
	JoinTroop(*priority.back());
	priority.PopBack();
    }

    // save half weak of strongest to army
    if(save_last && !isValid())
    {
	Troop & last = *priority.back();
	u32 count = last.GetCount() / 2;
	JoinTroop(last, last.GetCount() - count);
	last.SetCount(count);
    }

    // strongest to army2
    while(priority.size())
    {
	troops2.JoinTroop(*priority.back());
	priority.PopBack();
    }
}

void Troops::DrawMons32LineWithScoute(s32 cx, s32 cy, u32 width, u32 first, u32 count, u32 scoute, bool shorts) const
{
    if(isValid())
    {
	if(0 == count) count = GetCount();

	const u32 chunk = width / count;
	cx += chunk / 2;

	Text text;
	text.Set(Font::SMALL);

	for(const_iterator it = begin(); it != end(); ++it)
	    if((*it)->isValid())
	{
	    if(0 == first && count)
    	    {
		const Sprite & monster = AGG::GetICN(ICN::MONS32, (*it)->GetSpriteIndex());

    	        monster.Blit(cx - monster.w() / 2, cy + 30 - monster.h());
		text.Set(Game::CountScoute((*it)->GetCount(), scoute, shorts));
		text.Blit(cx - text.w() / 2, cy + 28);

		cx += chunk;
		--count;
	    }
	    else
		--first;
	}
    }
}

void Troops::SplitTroopIntoFreeSlots(const Troop & troop, u32 slots)
{
    if(slots && slots <= (Size() - GetCount()))
    {
	u32 chunk = troop.GetCount() / slots;
	u32 limits = slots;
	std::vector<iterator> iters;

	for(iterator it = begin(); it != end(); ++it)
	    if(! (*it)->isValid() && limits)
	{
	    iters.push_back(it);
	    (*it)->Set(troop.GetMonster(), chunk);
	    --limits;
	}

	u32 last = troop.GetCount() - chunk * slots;

	for(std::vector<iterator>::iterator
	    it = iters.begin(); it != iters.end(); ++it)
	    if(last)
	{
	    (**it)->SetCount((**it)->GetCount() + 1);
	    --last;
	}
    }
}




Army::Army(HeroBase* s) : commander(s), combat_format(true), color(Color::NONE)
{
    reserve(ARMYMAXTROOPS);
    for(u32 ii = 0; ii < ARMYMAXTROOPS; ++ii) push_back(new ArmyTroop(this));
}

Army::Army(const Maps::Tiles & t) : commander(NULL), combat_format(true), color(Color::NONE)
{
    reserve(ARMYMAXTROOPS);
    for(u32 ii = 0; ii < ARMYMAXTROOPS; ++ii) push_back(new ArmyTroop(this));

    if(MP2::isCaptureObject(t.GetObject()))
	color = t.QuantityColor();

    switch(t.GetObject(false))
    {
	case MP2::OBJ_PYRAMID:
            at(0)->Set(Monster::ROYAL_MUMMY, 10);
            at(1)->Set(Monster::VAMPIRE_LORD, 10);
            at(2)->Set(Monster::ROYAL_MUMMY, 10);
            at(3)->Set(Monster::VAMPIRE_LORD, 10);
            at(4)->Set(Monster::ROYAL_MUMMY, 10);
	    break;

	case MP2::OBJ_GRAVEYARD:
	    at(0)->Set(Monster::MUTANT_ZOMBIE, 100);
	    ArrangeForBattle(false);
	    break;

	case MP2::OBJ_SHIPWRECK:
	    at(0)->Set(Monster::GHOST, t.GetQuantity2());
	    ArrangeForBattle(false);
	    break;

	case MP2::OBJ_DERELICTSHIP:
	    at(0)->Set(Monster::SKELETON, 200);
	    ArrangeForBattle(false);
	    break;

	case MP2::OBJ_ARTIFACT:
	    switch(t.QuantityVariant())
	    {
		case 6:	at(0)->Set(Monster::ROGUE, 50); break;
		case 7:	at(0)->Set(Monster::GENIE, 1); break;
		case 8:	at(0)->Set(Monster::PALADIN, 1); break;
		case 9:	at(0)->Set(Monster::CYCLOPS, 1); break;
		case 10:at(0)->Set(Monster::PHOENIX, 1); break;
		case 11:at(0)->Set(Monster::GREEN_DRAGON, 1); break;
		case 12:at(0)->Set(Monster::TITAN, 1); break;
		case 13:at(0)->Set(Monster::BONE_DRAGON, 1); break;
		default: break;
	    }
	    ArrangeForBattle(false);
	    break;

	//case MP2::OBJ_ABANDONEDMINE:
	//    at(0) = Troop(t);
	//    ArrangeForBattle(false);
	//    break;

	case MP2::OBJ_CITYDEAD:
            at(0)->Set(Monster::ZOMBIE, 20);
            at(1)->Set(Monster::VAMPIRE_LORD, 5);
            at(2)->Set(Monster::POWER_LICH, 5);
            at(3)->Set(Monster::VAMPIRE_LORD, 5);
            at(4)->Set(Monster::ZOMBIE, 20);
	    break;

	case MP2::OBJ_TROLLBRIDGE:
            at(0)->Set(Monster::TROLL, 4);
            at(1)->Set(Monster::WAR_TROLL, 4);
            at(2)->Set(Monster::TROLL, 4);
            at(3)->Set(Monster::WAR_TROLL, 4);
            at(4)->Set(Monster::TROLL, 4);
	    break;

	case MP2::OBJ_DRAGONCITY:
            at(0)->Set(Monster::GREEN_DRAGON, 3);
            at(1)->Set(Monster::RED_DRAGON, 2);
            at(2)->Set(Monster::BLACK_DRAGON, 1);
	    break;

	case MP2::OBJ_DAEMONCAVE:
            at(0)->Set(Monster::EARTH_ELEMENT, 2);
            at(1)->Set(Monster::EARTH_ELEMENT, 2);
            at(2)->Set(Monster::EARTH_ELEMENT, 2);
            at(3)->Set(Monster::EARTH_ELEMENT, 2);
	    break;

	default:
	    if(MP2::isCaptureObject(t.GetObject()))
	    {
    		CapturedObject & co = world.GetCapturedObject(t.GetIndex());
		Troop & troop = co.GetTroop();

		switch(co.GetSplit())
		{
		    case 3:
			if(3 > troop.GetCount())
			    at(0)->Set(co.GetTroop());
			else
			{
			    at(0)->Set(troop(), troop.GetCount() / 3);
			    at(4)->Set(troop(), troop.GetCount() / 3);
			    at(2)->Set(troop(), troop.GetCount() - at(4)->GetCount() - at(0)->GetCount());
			}
			break;

		    case 5:
			if(5 > troop.GetCount())
			    at(0)->Set(co.GetTroop());
			else
			{
			    at(0)->Set(troop(), troop.GetCount() / 5);
			    at(1)->Set(troop(), troop.GetCount() / 5);
			    at(3)->Set(troop(), troop.GetCount() / 5);
			    at(4)->Set(troop(), troop.GetCount() / 5);
			    at(2)->Set(troop(), troop.GetCount() - at(0)->GetCount() - at(1)->GetCount() - at(3)->GetCount() - at(4)->GetCount());
			}
			break;

		    default:
			at(0)->Set(co.GetTroop());
			break;
		}
	    }
	    else
	    {
		MapMonster* map_troop = static_cast<MapMonster*>(world.GetMapObject(t.GetObjectUID(MP2::OBJ_MONSTER)));
		Troop troop = map_troop ? map_troop->QuantityTroop() : t.QuantityTroop();

		at(0)->Set(troop);
		ArrangeForBattle(! Settings::Get().ExtWorldSaveMonsterBattle());
	    }
	    break;
    }
}

Army::~Army()
{
    for(iterator it = begin(); it != end(); ++it) delete *it;
    clear();
}

bool Army::isFullHouse(void) const
{
    return GetCount() == size();
}

void Army::SetSpreadFormat(bool f)
{
    combat_format = f;
}

bool Army::isSpreadFormat(void) const
{
    return combat_format;
}

int Army::GetColor(void) const
{
    return GetCommander() ? GetCommander()->GetColor() : color;
}

void Army::SetColor(int cl)
{
    color = cl;
}

int Army::GetRace(void) const
{
    std::vector<int> races;
    races.reserve(size());

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid()) races.push_back((*it)->GetRace());

    std::sort(races.begin(), races.end());
    races.resize(std::distance(races.begin(), std::unique(races.begin(), races.end())));

    if(races.empty())
    {
        DEBUG(DBG_GAME, DBG_WARN, "empty");
        return Race::NONE;
    }

    return 1 < races.size() ? Race::MULT : races[0];
}

int Army::GetLuck(void) const
{
    return GetCommander() ? GetCommander()->GetLuck() : GetLuckModificator(NULL);
}

int Army::GetLuckModificator(std::string *strs) const
{
    return Luck::NORMAL;
}

int Army::GetMorale(void) const
{
    return GetCommander() ? GetCommander()->GetMorale() : GetMoraleModificator(NULL);
}

// TODO:: need optimize
int Army::GetMoraleModificator(std::string *strs) const
{
    int result = Morale::NORMAL;

    // different race penalty
    u32 count = 0;
    u32 count_kngt = 0;
    u32 count_barb = 0;
    u32 count_sorc = 0;
    u32 count_wrlk = 0;
    u32 count_wzrd = 0;
    u32 count_necr = 0;
    u32 count_bomg = 0;
    bool ghost_present = false;

    for(const_iterator it = begin(); it != end(); ++it) if((*it)->isValid())
    {
        switch((*it)->GetRace())
	{
            case Race::KNGT: ++count_kngt; break;
            case Race::BARB: ++count_barb; break;
            case Race::SORC: ++count_sorc; break;
            case Race::WRLK: ++count_wrlk; break;
            case Race::WZRD: ++count_wzrd; break;
            case Race::NECR: ++count_necr; break;
            case Race::NONE: ++count_bomg; break;
            default: break;
	}
        if((*it)->GetID() == Monster::GHOST) ghost_present = true;
    }

    u32 r = Race::MULT;
    if(count_kngt){ ++count; r = Race::KNGT; }
    if(count_barb){ ++count; r = Race::BARB; }
    if(count_sorc){ ++count; r = Race::SORC; }
    if(count_wrlk){ ++count; r = Race::WRLK; }
    if(count_wzrd){ ++count; r = Race::WZRD; }
    if(count_necr){ ++count; r = Race::NECR; }
    if(count_bomg){ ++count; r = Race::NONE; }
    const u32 uniq_count = GetUniqueCount();

    switch(count)
    {
        case 2:
        case 0: break;
        case 1:
    	    if(0 == count_necr && !ghost_present)
            {
		if(1 < uniq_count)
                {
		    ++result;
            	    if(strs)
            	    {
            		std::string str = _("All %{race} troops +1");
            		StringReplace(str, "%{race}", Race::String(r));
            		strs->append(str);
            		strs->append("\n");
            	    }
		}
            }
	    else
            {
	        if(strs)
                {
            	    strs->append(_("Entire unit is undead, so morale does not apply."));
            	    strs->append("\n");
            	}
		return 0;
	    }
            break;
        case 3:
            result -= 1;
            if(strs)
            {
        	strs->append(_("Troops of 3 alignments -1"));
        	strs->append("\n");
    	    }
            break;
        case 4:
    	    result -= 2;
            if(strs)
            {
        	strs->append(_("Troops of 4 alignments -2"));
        	strs->append("\n");
    	    }
            break;
        default:
            result -= 3;
            if(strs)
            {
        	strs->append(_("Troops of 5 alignments -3"));
        	strs->append("\n");
    	    }
            break;
    }

    // undead in life group
    if((1 < uniq_count && (count_necr || ghost_present) && (count_kngt || count_barb || count_sorc || count_wrlk || count_wzrd || count_bomg)) ||
    // or artifact Arm Martyr
	(GetCommander() && GetCommander()->HasArtifact(Artifact::ARM_MARTYR)))
    {
        result -= 1;
        if(strs)
        {
    	    strs->append(_("Some undead in groups -1"));
    	    strs->append("\n");
    	}
    }

    return result;
}

u32 Army::GetAttack(void) const
{
    u32 res = 0;
    u32 count = 0;

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid()){ res += (*it)->GetAttack(); ++count; }

    return count ? res / count : 0;
}

u32 Army::GetDefense(void) const
{
    u32 res = 0;
    u32 count = 0;

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid()){ res += (*it)->GetDefense(); ++count; }

    return count ? res / count : 0;
}

void Army::Reset(bool soft)
{
    Troops::Clean();

    if(commander && commander->isHeroes())
    {
    	const Monster mons1(commander->GetRace(), DWELLING_MONSTER1);

	if(soft)
	{
    	    const Monster mons2(commander->GetRace(), DWELLING_MONSTER2);

	    switch(Rand::Get(1, 3))
	    {
		case 1:
		    JoinTroop(mons1, 3 * mons1.GetGrown());
		    break;
		case 2:
		    JoinTroop(mons2, mons2.GetGrown() + mons2.GetGrown() / 2);
		    break;
		default:
		    JoinTroop(mons1, 2 * mons1.GetGrown());
		    JoinTroop(mons2, mons2.GetGrown());
		    break;
	    }
	}
	else
	{
	    JoinTroop(mons1, 1);
	}
    }
}

void Army::SetCommander(HeroBase* c)
{
    commander = c;
}

HeroBase* Army::GetCommander(void)
{
    return (!commander || (commander->isCaptain() && !commander->isValid())) ? NULL : commander;
}

const Castle* Army::inCastle(void) const
{
    return commander ? commander->inCastle() : NULL;
}

const HeroBase* Army::GetCommander(void) const
{
    return (!commander || (commander->isCaptain() && !commander->isValid())) ? NULL : commander;
}

int Army::GetControl(void) const
{
    return commander ? commander->GetControl() : (color == Color::NONE ? CONTROL_AI : Players::GetPlayerControl(color));
}

std::string Army::String(void) const
{
    std::ostringstream os;

    os << "color(" << Color::String(commander ? commander->GetColor() : color) << "), ";

    if(GetCommander())
	os << "commander(" << GetCommander()->GetName() << "), ";

    os << ": ";

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it)->isValid())
	    os << std::dec << (*it)->GetCount() << " " << (*it)->GetName() << ", ";

    return os.str();
}

void Army::JoinStrongestFromArmy(Army & army2)
{
    bool save_last = army2.commander && army2.commander->isHeroes();
    JoinStrongest(army2, save_last);
}

void Army::KeepOnlyWeakestTroops(Army & army2)
{

    bool save_last = commander && commander->isHeroes();
    KeepOnlyWeakest(army2, save_last);
}

u32 Army::ActionToSirens(void)
{
    u32 res = 0;

    for(iterator it = begin(); it != end(); ++it)
    if((*it)->isValid())
    {
	const u32 kill = (*it)->GetCount() * 30 / 100;

	if(kill)
	{
	    (*it)->SetCount((*it)->GetCount() - kill);
	    res += kill * static_cast<Monster*>(*it)->GetHitPoints();
	}
    }

    return res;
}

bool Army::TroopsStrongerEnemyTroops(const Troops & troops1, const Troops & troops2)
{
    if(! troops2.isValid()) return true;

    const int a1 = troops1.GetAttack();
    const int d1 = troops1.GetDefense();
    float r1 = 0;

    const int a2 = troops2.GetAttack();
    const int d2 = troops2.GetDefense();
    float r2 = 0;

    if(a1 > d2)
        r1 = 1 + 0.1 * static_cast<float>(std::min(a1 - d2, 20));
    else
        r1 = 1 + 0.05 * static_cast<float>(std::min(d2 - a1, 14));

    if(a2 > d1)
        r2 = 1 + 0.1 * static_cast<float>(std::min(a2 - d1, 20));
    else
        r2 = 1 + 0.05 * static_cast<float>(std::min(d1 - a2, 14));

    const u32 s1 = troops1.GetStrength();
    const u32 s2 = troops2.GetStrength();

    const float h1 = troops1.GetHitPoints();
    const float h2 = troops2.GetHitPoints();

    DEBUG(DBG_AI, DBG_TRACE, "r1: " << r1 << ", s1: " << s1 << ", h1: " << h1 \
			<< ", r2: " << r2 << ", s2: " << s2 << ", h2: " << h2);

    r1 *= s1 / h2;
    r2 *= s2 / h1;

    return static_cast<s32>(r1) > static_cast<s32>(r2);
}

void Army::DrawMons32LineWithScoute(const Troops & troops, s32 cx, s32 cy, u32 width, u32 first, u32 count, u32 scoute)
{
    troops.DrawMons32LineWithScoute(cx, cy, width, first, count, scoute, false);
}

/* draw MONS32 sprite in line, first valid = 0, count = 0 */
void Army::DrawMons32Line(const Troops & troops, s32 cx, s32 cy, u32 width, u32 first, u32 count)
{
    troops.DrawMons32LineWithScoute(cx, cy, width, first, count, Skill::Level::EXPERT, false);
}

void Army::DrawMons32LineShort(const Troops & troops, s32 cx, s32 cy, u32 width, u32 first, u32 count)
{
    troops.DrawMons32LineWithScoute(cx, cy, width, first, count, Skill::Level::EXPERT, true);
}

JoinCount Army::GetJoinSolution(const Heroes & hero, const Maps::Tiles & tile, const Troop & troop)
{
    MapMonster* map_troop = static_cast<MapMonster*>(world.GetMapObject(tile.GetObjectUID(MP2::OBJ_MONSTER)));
    const u32  ratios = troop.isValid() ? hero.GetArmy().GetHitPoints() / troop.GetHitPoints() : 0;
    const bool check_free_stack = true; // (hero.GetArmy().GetCount() < hero.GetArmy().size() || hero.GetArmy().HasMonster(troop)); // set force, see Dialog::ArmyJoinWithCost, http://sourceforge.net/tracker/?func=detail&aid=3567985&group_id=96859&atid=616183
    const bool check_extra_condition = (!hero.HasArtifact(Artifact::HIDEOUS_MASK) && Morale::NORMAL <= hero.GetMorale());

    const bool join_skip = map_troop ? map_troop->JoinConditionSkip() : tile.MonsterJoinConditionSkip();
    const bool join_free = map_troop ? map_troop->JoinConditionFree() : tile.MonsterJoinConditionFree();
    // force join for campain and others...
    const bool join_force = map_troop ? map_troop->JoinConditionForce() : tile.MonsterJoinConditionForce();

    if(!join_skip &&
        check_free_stack && ((check_extra_condition && ratios >= 2) || join_force))
    {
        if(join_free || join_force)
            return JoinCount(JOIN_FREE, troop.GetCount());
        else
        if(hero.HasSecondarySkill(Skill::Secondary::DIPLOMACY))
        {
            // skill diplomacy
            const u32 to_join = Monster::GetCountFromHitPoints(troop,
                            troop.GetHitPoints() * hero.GetSecondaryValues(Skill::Secondary::DIPLOMACY) / 100);

            if(to_join)
        	return JoinCount(JOIN_COST, to_join);
        }
    }
    else
    if(ratios >= 5)
    {
	// ... surely flee before us
	if(! hero.isControlAI() ||
	    Rand::Get(0, 10) < 5)
	    return JoinCount(JOIN_FLEE, 0);
    }

    return JoinCount(JOIN_NONE, 0);
}

bool Army::WeakestTroop(const Troop* t1, const Troop* t2)
{
    return t1->GetDamageMax() < t2->GetDamageMax();
}

bool Army::StrongestTroop(const Troop* t1, const Troop* t2)
{
    return t1->GetDamageMin() > t2->GetDamageMin();
}

bool Army::SlowestTroop(const Troop* t1, const Troop* t2)
{
    return t1->GetSpeed() < t2->GetSpeed();
}

bool Army::FastestTroop(const Troop* t1, const Troop* t2)
{
    return t1->GetSpeed() > t2->GetSpeed();
}

void Army::SwapTroops(Troop & t1, Troop & t2)
{
    std::swap(t1, t2);
}

bool Army::SaveLastTroop(void) const
{
    return commander && commander->isHeroes() && 1 == GetCount();
}

StreamBase & operator<< (StreamBase & msg, const Army & army)
{
    msg << static_cast<u32>(army.size());

    // Army: fixed size
    for(Army::const_iterator it = army.begin(); it != army.end(); ++it)
	msg << **it;

    return msg << army.combat_format << army.color;
}

StreamBase & operator>> (StreamBase & msg, Army & army)
{
    u32 armysz;
    msg >> armysz;

    for(Army::iterator it = army.begin(); it != army.end(); ++it)
	msg >> **it;

    msg >> army.combat_format >> army.color;

    // set army
    for(Army::iterator it = army.begin(); it != army.end(); ++it)
    {
	ArmyTroop* troop = static_cast<ArmyTroop*>(*it);
	if(troop) troop->SetArmy(army);
    }

    // set later from owner (castle, heroes)
    army.commander = NULL;

    return msg;
}
