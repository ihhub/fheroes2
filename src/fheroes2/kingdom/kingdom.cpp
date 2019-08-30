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

#include "settings.h"
#include "castle.h"
#include "heroes.h"
#include "difficulty.h"
#include "color.h"
#include "game.h"
#include "profit.h"
#include "world.h"
#include "visit.h"
#include "race.h"
#include "battle.h"
#include "kingdom.h"
#include "players.h"
#include "game_static.h"
#include "game_interface.h"
#include "ai.h"

bool HeroesStrongestArmy(const Heroes* h1, const Heroes* h2)
{
    return h1 && h2 && Army::TroopsStrongerEnemyTroops(h2->GetArmy(), h1->GetArmy());
}

Kingdom::Kingdom() : color(Color::NONE), lost_town_days(0), visited_tents_colors(0)
{
    heroes_cond_loss.reserve(4);
}

void Kingdom::Init(int clr)
{
    clear();
    color = clr;

    if(Color::ALL & color)
    {
	heroes.reserve(GetMaxHeroes());
	castles.reserve(15);

	UpdateStartingResource();
    }
    else
    {
	DEBUG(DBG_GAME, DBG_INFO, "Kingdom: unknown player: " << Color::String(color) << "(" << static_cast<int>(color) << ")");
    }
}

void Kingdom::clear(void)
{
    modes	= 0;

    color	= Color::NONE;
    visited_tents_colors = 0;
    lost_town_days = Game::GetLostTownDays() + 1;

    heroes.clear();
    castles.clear();
    visit_object.clear();

    recruits.Reset();

    heroes_cond_loss.clear();
}

int Kingdom::GetControl(void) const
{
    return Players::GetPlayerControl(color);
}

int Kingdom::GetColor(void) const
{
    return color;
}

int Kingdom::GetRace(void) const
{
    return Players::GetPlayerRace(GetColor());
}

void Kingdom::UpdateStartingResource(void)
{
    resource = GameStatic::GetKingdomStartingResource(isControlAI() ? 5 : Settings::Get().GameDifficulty());
}

bool Kingdom::isLoss(void) const
{
    return castles.empty() && heroes.empty();
}

bool Kingdom::isPlay(void) const
{
    return Players::GetPlayerInGame(color);
}

void Kingdom::LossPostActions(void)
{
    if(isPlay())
    {
	Players::SetPlayerInGame(color, false);

	if(heroes.size())
	{
	    std::for_each(heroes.begin(), heroes.end(),
		std::bind2nd(std::mem_fun(&Heroes::SetFreeman), static_cast<int>(Battle::RESULT_LOSS)));
	    heroes.clear();
	}
	if(castles.size())
	{
	    castles.ChangeColors(GetColor(), Color::NONE);
	    castles.clear();
	}
	world.ResetCapturedObjects(GetColor());
    }
}

void Kingdom::ActionBeforeTurn(void)
{
    // rescan heroes path
    std::for_each(heroes.begin(), heroes.end(), std::mem_fun(&Heroes::RescanPath));
}

void Kingdom::ActionNewDay(void)
{
    if(isLoss() || 0 == lost_town_days)
    {
	LossPostActions();
	return;
    }

    // modes
    ResetModes(IDENTIFYHERO);

    // check lost town
    if(castles.empty()) --lost_town_days;

    // skip incomes for first day
    if(1 < world.CountDay())
    {
        // castle New Day
	std::for_each(castles.begin(), castles.end(), std::mem_fun(&Castle::ActionNewDay));

	// heroes New Day
	std::for_each(heroes.begin(), heroes.end(), std::mem_fun(&Heroes::ActionNewDay));

	// income
	AddFundsResource(GetIncome());
    }

    // check event day AI
    EventsDate events = world.GetEventsDate(GetColor());
    for(EventsDate::const_iterator
	it = events.begin(); it != events.end(); ++it)
	AddFundsResource((*it).resource);

    // remove day visit object
    visit_object.remove_if(Visit::isDayLife);
}

void Kingdom::ActionNewWeek(void)
{
    ResetModes(DISABLEHIRES);

    // skip first day
    if(1 < world.CountDay())
    {
	// castle New Week
	std::for_each(castles.begin(), castles.end(), std::mem_fun(&Castle::ActionNewWeek));

	// heroes New Week
	std::for_each(heroes.begin(), heroes.end(), std::mem_fun(&Heroes::ActionNewWeek));

	// debug an gift
	if(IS_DEVEL() && isControlHuman())
	{
	    Funds gift(20, 20, 10, 10, 10, 10, 5000);
	    DEBUG(DBG_GAME, DBG_INFO, "debug gift: " << gift.String());
	    resource += gift;
	}
    }

    // remove week visit object
    visit_object.remove_if(Visit::isWeekLife);

    UpdateRecruits();
}

void Kingdom::ActionNewMonth(void)
{
    // skip first day
    if(1 < world.CountDay())
    {
	// castle New Month
	std::for_each(castles.begin(), castles.end(), std::mem_fun(&Castle::ActionNewMonth));

	// heroes New Month
	std::for_each(heroes.begin(), heroes.end(), std::mem_fun(&Heroes::ActionNewMonth));
    }

    // remove week visit object
    visit_object.remove_if(Visit::isMonthLife);
}

void Kingdom::AddHeroes(Heroes* hero)
{
    if(hero)
    {
	if(heroes.end() == std::find(heroes.begin(), heroes.end(), hero))
	    heroes.push_back(hero);

	Player* player = Settings::Get().GetPlayers().GetCurrent();
	if(player && player->isColor(GetColor()))
	    Interface::Basic::Get().GetIconsPanel().ResetIcons(ICON_HEROES);

	AI::HeroesAdd(*hero);
    }
}

void Kingdom::AddHeroStartCondLoss(Heroes* hero)
{
    // see: Settings::ExtWorldStartHeroLossCond4Humans
    heroes_cond_loss.push_back(hero);
}

const Heroes* Kingdom::GetFirstHeroStartCondLoss(void) const
{
    for(KingdomHeroes::const_iterator
        it = heroes_cond_loss.begin(); it != heroes_cond_loss.end(); ++it)
	if((*it)->isFreeman() || (*it)->GetColor() != GetColor())
	    return *it;
    return NULL;
}

std::string Kingdom::GetNamesHeroStartCondLoss(void) const
{
    std::string result;
    for(KingdomHeroes::const_iterator
        it = heroes_cond_loss.begin(); it != heroes_cond_loss.end(); ++it)
    {
	result.append((*it)->GetName());
	if(it + 1 != heroes_cond_loss.end()) result.append(", ");
    }
    return result;
}

void Kingdom::RemoveHeroes(const Heroes* hero)
{
    if(hero)
    {
	if(heroes.size())
	    heroes.erase(std::find(heroes.begin(), heroes.end(), hero));

	AI::HeroesRemove(*hero);
    }

    if(isLoss()) LossPostActions();
}

void Kingdom::AddCastle(const Castle* castle)
{
    if(castle)
    {
	if(castles.end() == std::find(castles.begin(), castles.end(), castle))
	    castles.push_back(const_cast<Castle*>(castle));

	Player* player = Settings::Get().GetPlayers().GetCurrent();
	if(player && player->isColor(GetColor()))
	    Interface::Basic::Get().GetIconsPanel().ResetIcons(ICON_CASTLES);

	AI::CastleAdd(*castle);
    }

    lost_town_days = Game::GetLostTownDays() + 1;
}

void Kingdom::RemoveCastle(const Castle* castle)
{
    if(castle)
    {
	if(castles.size())
	    castles.erase(std::find(castles.begin(), castles.end(), castle));

	AI::CastleRemove(*castle);
    }

    if(isLoss()) LossPostActions();
}

u32 Kingdom::GetCountCastle(void) const
{
    return std::count_if(castles.begin(), castles.end(), Castle::PredicateIsCastle);
}

u32 Kingdom::GetCountTown(void) const
{
    return std::count_if(castles.begin(), castles.end(), Castle::PredicateIsTown);
}

u32 Kingdom::GetCountMarketplace(void) const
{
    return std::count_if(castles.begin(), castles.end(), Castle::PredicateIsBuildMarketplace);
}

u32 Kingdom::GetCountNecromancyShrineBuild(void) const
{
    return std::count_if(castles.begin(), castles.end(), std::mem_fun(&Castle::isNecromancyShrineBuild));
}

u32 Kingdom::GetCountBuilding(u32 build) const
{
    return std::count_if(castles.begin(), castles.end(), std::bind2nd(std::mem_fun(&Castle::isBuild), build));
}

bool Kingdom::AllowPayment(const Funds & funds) const
{
    return
	(resource.wood >= funds.wood || 0 == funds.wood) &&
	(resource.mercury >= funds.mercury || 0 == funds.mercury) &&
	(resource.ore >= funds.ore || 0 == funds.ore) &&
	(resource.sulfur >= funds.sulfur || 0 == funds.sulfur) &&
	(resource.crystal >= funds.crystal || 0 == funds.crystal) &&
	(resource.gems >= funds.gems || 0 == funds.gems) &&
	(resource.gold >= funds.gold || 0 == funds.gold);
}

/* is visited cell */
bool Kingdom::isVisited(const Maps::Tiles & tile) const
{
    return isVisited(tile.GetIndex(), tile.GetObject());
}

bool Kingdom::isVisited(s32 index, int object) const
{
    std::list<IndexObject>::const_iterator it = std::find_if(visit_object.begin(), visit_object.end(), std::bind2nd(std::mem_fun_ref(&IndexObject::isIndex), index));
    return visit_object.end() != it && (*it).isObject(object);
}

/* return true if object visited */
bool Kingdom::isVisited(int object) const
{
    return visit_object.end() != std::find_if(visit_object.begin(), visit_object.end(), std::bind2nd(std::mem_fun_ref(&IndexObject::isObject), object));
}

u32 Kingdom::CountVisitedObjects(int object) const
{
    return std::count_if(visit_object.begin(), visit_object.end(), std::bind2nd(std::mem_fun_ref(&IndexObject::isObject), object));
}

/* set visited cell */
void Kingdom::SetVisited(s32 index, int object)
{
    if(!isVisited(index, object) && object != MP2::OBJ_ZERO) visit_object.push_front(IndexObject(index, object));
}

bool Kingdom::HeroesMayStillMove(void) const
{
    return heroes.end() != std::find_if(heroes.begin(), heroes.end(), std::mem_fun(&Heroes::MayStillMove));
}

u32 Kingdom::GetCountCapital(void) const
{
    return std::count_if(castles.begin(), castles.end(), Castle::PredicateIsCapital);
}

void Kingdom::AddFundsResource(const Funds & funds)
{
    resource = resource + funds;
}

void Kingdom::OddFundsResource(const Funds & funds)
{
    resource = resource - funds;
}

u32 Kingdom::GetLostTownDays(void) const
{
    return lost_town_days;
}

Recruits & Kingdom::GetRecruits(void)
{
    // update hero1
    if(Heroes::UNKNOWN == recruits.GetID1() || (recruits.GetHero1() && !recruits.GetHero1()->isFreeman()))
	recruits.SetHero1(world.GetFreemanHeroes(GetRace()));

    // update hero2
    if(Heroes::UNKNOWN == recruits.GetID2() || (recruits.GetHero2() && !recruits.GetHero2()->isFreeman()))
	recruits.SetHero2(world.GetFreemanHeroes());

    if(recruits.GetID1() == recruits.GetID2()) world.UpdateRecruits(recruits);

    return recruits;
}

void Kingdom::UpdateRecruits(void)
{
    recruits.SetHero1(world.GetFreemanHeroes(GetRace()));
    recruits.SetHero2(world.GetFreemanHeroes());

    if(recruits.GetID1() == recruits.GetID2()) world.UpdateRecruits(recruits);
}

const Puzzle & Kingdom::PuzzleMaps(void) const
{
    return puzzle_maps;
}

Puzzle & Kingdom::PuzzleMaps(void)
{
    return puzzle_maps;
}

void Kingdom::SetVisitTravelersTent(int col)
{
    visited_tents_colors |= col;
}

bool Kingdom::IsVisitTravelersTent(int col) const
{
    return visited_tents_colors & col;
}

bool Kingdom::AllowRecruitHero(bool check_payment, int level) const
{
    return (heroes.size() < GetMaxHeroes()) && (!check_payment || AllowPayment(PaymentConditions::RecruitHero(level)));
}

void Kingdom::ApplyPlayWithStartingHero(void)
{
    if(isPlay() && castles.size())
    {
	// get first castle
	Castle* first = castles.GetFirstCastle();
	if(NULL == first) first = castles.front();

	// check manual set hero (castle position + point(0, 1))?
	const Point & cp = (first)->GetCenter();
	Heroes* hero = world.GetTiles(cp.x, cp.y + 1).GetHeroes();

    	// and move manual set hero to castle
	if(hero && hero->GetColor() == GetColor())
	{
		bool patrol = hero->Modes(Heroes::PATROL);
    		hero->SetFreeman(0);
    		hero->Recruit(*first);

		if(patrol)
		{
		    hero->SetModes(Heroes::PATROL);
		    hero->SetCenterPatrol(cp);
		}
	}
	else
	if(Settings::Get().GameStartWithHeroes())
	{
    	    Heroes* hero = world.GetFreemanHeroes(first->GetRace());
	    if(hero && AllowRecruitHero(false, 0)) hero->Recruit(*first);
	}
    }
}

u32 Kingdom::GetMaxHeroes(void)
{
    return GameStatic::GetKingdomMaxHeroes();
}

void Kingdom::HeroesActionNewPosition(void)
{
    // Heroes::ActionNewPosition: can remove elements from heroes vector.
    KingdomHeroes heroes2(heroes);
    std::for_each(heroes2.begin(), heroes2.end(), std::mem_fun(&Heroes::ActionNewPosition));
}

Funds Kingdom::GetIncome(int type /* INCOME_ALL */) const
{
    Funds resource;

    if(INCOME_CAPTURED & type)
    {
	// captured object
	const int resources[] = { Resource::WOOD, Resource::ORE, Resource::MERCURY, Resource::SULFUR,
                                    Resource::CRYSTAL, Resource::GEMS, Resource::GOLD, Resource::UNKNOWN };

	for(u32 index = 0; resources[index] != Resource::UNKNOWN; ++index)
    	    resource += ProfitConditions::FromMine(resources[index]) *
                            	    world.CountCapturedMines(resources[index], GetColor());
    }

    if(INCOME_CASTLES & type)
    {
	// castles
	for(KingdomCastles::const_iterator
	    it = castles.begin(); it != castles.end(); ++it)
	{
    	    const Castle & castle = **it;

    	    // castle or town profit
    	    resource += ProfitConditions::FromBuilding((castle.isCastle() ? BUILD_CASTLE : BUILD_TENT), 0);

    	    // statue
    	    if(castle.isBuild(BUILD_STATUE))
                resource += ProfitConditions::FromBuilding(BUILD_STATUE, 0);

    	    // dungeon for warlock
	    if(castle.isBuild(BUILD_SPEC) && Race::WRLK == castle.GetRace())
                resource += ProfitConditions::FromBuilding(BUILD_SPEC, Race::WRLK);
	}
    }

    if(INCOME_ARTIFACTS & type)
    {
	// find artifacts                                                                                            
	const int artifacts[] = { Artifact::GOLDEN_GOOSE, Artifact::ENDLESS_SACK_GOLD, Artifact::ENDLESS_BAG_GOLD,
                Artifact::ENDLESS_PURSE_GOLD, Artifact::ENDLESS_POUCH_SULFUR, Artifact::ENDLESS_VIAL_MERCURY,
                Artifact::ENDLESS_POUCH_GEMS, Artifact::ENDLESS_CORD_WOOD, Artifact::ENDLESS_CART_ORE,
                Artifact::ENDLESS_POUCH_CRYSTAL, Artifact::UNKNOWN };

	for(u32 index = 0; artifacts[index] != Artifact::UNKNOWN; ++index)
	    for(KingdomHeroes::const_iterator
		ith = heroes.begin(); ith != heroes.end(); ++ith)
    		resource += ProfitConditions::FromArtifact(artifacts[index]) * 
		    (**ith).GetBagArtifacts().Count(Artifact(artifacts[index]));

	// TAX_LIEN
	for(KingdomHeroes::const_iterator
	    ith = heroes.begin(); ith != heroes.end(); ++ith)
    	    resource -= ProfitConditions::FromArtifact(Artifact::TAX_LIEN) * 
		    (**ith).GetBagArtifacts().Count(Artifact(Artifact::TAX_LIEN));
    }

    if(INCOME_HEROSKILLS & type)
    {
	// estates skill bonus
	for(KingdomHeroes::const_iterator
	    ith = heroes.begin(); ith != heroes.end(); ++ith)
	    resource.gold += (**ith).GetSecondaryValues(Skill::Secondary::ESTATES);
    }

    return resource;
}

const Heroes* Kingdom::GetBestHero(void) const
{
    return heroes.size() ? *std::max_element(heroes.begin(), heroes.end(), HeroesStrongestArmy) : NULL;
}

u32 Kingdom::GetArmiesStrength(void) const
{
    u32 res = 0;

    for(KingdomHeroes::const_iterator
	ith = heroes.begin(); ith != heroes.end(); ++ith)
	res += (**ith).GetArmy().GetStrength();

    for(KingdomCastles::const_iterator
	itc = castles.begin(); itc != castles.end(); ++itc)
	res += (**itc).GetArmy().GetStrength();

    return res;
}

Kingdoms::Kingdoms()
{
}

void Kingdoms::Init(void)
{
    const Colors colors(Settings::Get().GetPlayers().GetColors());

    clear();

    for(Colors::const_iterator
	it = colors.begin(); it != colors.end(); ++it)
	GetKingdom(*it).Init(*it);
}

u32 Kingdoms::size(void) const
{
    return KINGDOMMAX + 1;
}

void Kingdoms::clear(void)
{
    for(u32 ii = 0; ii < size(); ++ii)
	kingdoms[ii].clear();
}

void Kingdoms::ApplyPlayWithStartingHero(void)
{
    for(u32 ii = 0; ii < size(); ++ii)
	if(kingdoms[ii].isPlay()) kingdoms[ii].ApplyPlayWithStartingHero();
}

const Kingdom & Kingdoms::GetKingdom(int color) const
{
    switch(color)
    {
        case Color::BLUE:       return kingdoms[0];
        case Color::GREEN:      return kingdoms[1];
        case Color::RED:        return kingdoms[2];
        case Color::YELLOW:     return kingdoms[3];
        case Color::ORANGE:     return kingdoms[4];
        case Color::PURPLE:     return kingdoms[5];
	default: break;
    }

    return kingdoms[6];
}

Kingdom & Kingdoms::GetKingdom(int color)
{
    switch(color)
    {
        case Color::BLUE:       return kingdoms[0];
        case Color::GREEN:      return kingdoms[1];
        case Color::RED:        return kingdoms[2];
        case Color::YELLOW:     return kingdoms[3];
        case Color::ORANGE:     return kingdoms[4];
        case Color::PURPLE:     return kingdoms[5];
	default: break;
    }

    return kingdoms[6];
}

void Kingdom::SetLastLostHero(Heroes & hero)
{
    lost_hero.first = hero.GetID();
    lost_hero.second = world.CountDay();
}

void Kingdom::ResetLastLostHero(void)
{
    lost_hero.first = Heroes::UNKNOWN;
    lost_hero.second = 0;
}

Heroes* Kingdom::GetLastLostHero(void) const
{
    return Heroes::UNKNOWN != lost_hero.first && world.CountDay() - lost_hero.second < DAYOFWEEK ?
		world.GetHeroes(lost_hero.first) : NULL;
}

void Kingdoms::NewDay(void)
{
    for(u32 ii = 0; ii < size(); ++ii)
	if(kingdoms[ii].isPlay()) kingdoms[ii].ActionNewDay();
}

void Kingdoms::NewWeek(void)
{
    for(u32 ii = 0; ii < size(); ++ii)
	if(kingdoms[ii].isPlay()) kingdoms[ii].ActionNewWeek();
}

void Kingdoms::NewMonth(void)
{
    for(u32 ii = 0; ii < size(); ++ii)
	if(kingdoms[ii].isPlay()) kingdoms[ii].ActionNewMonth();
}

int Kingdoms::GetNotLossColors(void) const
{
    int result = 0;
    for(u32 ii = 0; ii < size(); ++ii)
	if(kingdoms[ii].GetColor() &&
	    ! kingdoms[ii].isLoss()) result |= kingdoms[ii].GetColor();
    return result;
}

int Kingdoms::GetLossColors(void) const
{
    int result = 0;
    for(u32 ii = 0; ii < size(); ++ii)
	if(kingdoms[ii].GetColor() &&
	    kingdoms[ii].isLoss()) result |= kingdoms[ii].GetColor();
    return result;
}

int Kingdoms::FindWins(int cond) const
{
    for(u32 ii = 0; ii < size(); ++ii)
	if(kingdoms[ii].GetColor() &&
	    world.KingdomIsWins(kingdoms[ii], cond))
		return kingdoms[ii].GetColor();
    return 0;
}

void Kingdoms::AddHeroes(const AllHeroes & heroes)
{
    for(AllHeroes::const_iterator
        it = heroes.begin(); it != heroes.end(); ++it)
	// skip gray color
        if((*it)->GetColor()) GetKingdom((*it)->GetColor()).AddHeroes(*it);
}

void Kingdoms::AddCondLossHeroes(const AllHeroes & heroes)
{
    for(AllHeroes::const_iterator
        it = heroes.begin(); it != heroes.end(); ++it)
	// skip gray color
        if((*it)->GetColor())
    {
	Kingdom & kingdom = GetKingdom((*it)->GetColor());

	if(kingdom.isControlHuman())
	{
	    (*it)->SetModes(Heroes::NOTDISMISS | Heroes::NOTDEFAULTS);
	    kingdom.AddHeroStartCondLoss(*it);
	}
    }
}

void Kingdoms::AddCastles(const AllCastles & castles)
{
    for(AllCastles::const_iterator
	it = castles.begin(); it != castles.end(); ++it)
	// skip gray color
        if((*it)->GetColor()) GetKingdom((*it)->GetColor()).AddCastle(*it);
}

void Kingdoms::AddTributeEvents(CapturedObjects & captureobj, u32 day, int obj)
{
    for(u32 ii = 0; ii < size(); ++ii)
	if(kingdoms[ii].isPlay())
    {
	const int color = kingdoms[ii].GetColor();
	const Funds & funds = captureobj.TributeCapturedObject(color, obj);

	kingdoms[ii].AddFundsResource(funds);

	// for show dialogs
        if(funds.GetValidItemsCount() && kingdoms[ii].isControlHuman())
        {
            EventDate event;

            event.computer = true;
            event.first = day;
            event.colors = color;
	    event.resource = funds;
	    event.message = MP2::StringObject(obj);

	    world.AddEventDate(event);
	}
    }
}

StreamBase & operator<< (StreamBase & msg, const Kingdom & kingdom)
{
    return msg <<
	kingdom.modes <<
	kingdom.color <<
	kingdom.resource <<
	kingdom.lost_town_days <<
	kingdom.castles <<
	kingdom.heroes <<
	kingdom.recruits <<
	kingdom.lost_hero <<
	kingdom.visit_object <<
	kingdom.puzzle_maps <<
	kingdom.visited_tents_colors <<
	kingdom.heroes_cond_loss;
}

StreamBase & operator>> (StreamBase & msg, Kingdom & kingdom)
{
    return msg >>
	kingdom.modes >>
	kingdom.color >>
	kingdom.resource >>
	kingdom.lost_town_days >>
	kingdom.castles >>
	kingdom.heroes >>
	kingdom.recruits >>
	kingdom.lost_hero >>
	kingdom.visit_object >>
	kingdom.puzzle_maps >>
	kingdom.visited_tents_colors >>
	kingdom.heroes_cond_loss;
}

StreamBase & operator<< (StreamBase & msg, const Kingdoms & obj)
{
    msg << static_cast<u32>(obj.size());
    for(u32 ii = 0; ii < obj.size(); ++ii)
	msg << obj.kingdoms[ii];

    return msg;
}

StreamBase & operator>> (StreamBase & msg, Kingdoms & obj)
{
    u32 kingdomscount;
    msg >> kingdomscount; // FIXME: check kingdomscount

    for(u32 ii = 0; ii < kingdomscount; ++ii)
	msg >> obj.kingdoms[ii];

    return msg;
}

StreamBase & operator>> (StreamBase & sb, LastLoseHero & st)
{
    return sb >> st.first >> st.second;
}
