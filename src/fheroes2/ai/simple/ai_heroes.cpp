/********************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>               *
 *   All rights reserved.                                                       *
 *                                                                              *
 *   Part of the Free Heroes2 Engine:                                           *
 *   http://sourceforge.net/projects/fheroes2                                   *
 *                                                                              *
 *   Redistribution and use in source and binary forms, with or without         *
 *   modification, are permitted provided that the following conditions         *
 *   are met:                                                                   *
 *   - Redistributions may not be sold, nor may they be used in a               *
 *     commercial product or activity.                                          *
 *   - Redistributions of source code and/or in binary form must reproduce      *
 *     the above copyright notice, this list of conditions and the              *
 *     following disclaimer in the documentation and/or other materials         *
 *     provided with the distribution.                                          *
 *                                                                              *
 * THIS SOFTWARE IS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,   *
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS    *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT     *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,        *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;  *
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,     *
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE         *
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,            *
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           *
 *******************************************************************************/

#include <functional>
#include <algorithm>

#include "settings.h"
#include "kingdom.h"
#include "castle.h"
#include "army.h"
#include "battle.h"
#include "luck.h"
#include "morale.h"
#include "race.h"
#include "difficulty.h"
#include "world.h"
#include "payment.h"
#include "heroes.h"
#include "cursor.h"
#include "game_interface.h"
#include "interface_gamearea.h"
#include "maps_tiles.h"
#include "ai_simple.h"

#define HERO_MAX_SHEDULED_TASK 7

AIHeroes & AIHeroes::Get(void)
{
    static AIHeroes ai_heroes;
    return ai_heroes;
}

AIHero & AIHeroes::Get(const Heroes & ht)
{
    return AIHeroes::Get().at(ht.GetID());
}

void AIHeroes::Reset(void)
{
    AIHeroes & ai = AIHeroes::Get();
    std::for_each(ai.begin(), ai.end(), std::mem_fun_ref(&AIHero::Reset));
}

void AIHero::Reset(void)
{
    primary_target = -1;
    sheduled_visit.clear();
    fix_loop = 0;
}

bool AI::HeroesSkipFog(void)
{
    return false;
}

void AI::HeroesActionComplete(Heroes &, s32)
{
}

std::string AI::HeroesString(const Heroes & hero)
{
    std::ostringstream os;

    AIHero & ai_hero = AIHeroes::Get(hero);
    Queue & task = ai_hero.sheduled_visit;

    os << "flags           : " << 
	(hero.Modes(AI::HEROES_SCOUTER) ? "SCOUTER," : "") <<
        (hero.Modes(AI::HEROES_HUNTER) ? "HUNTER," : "") <<
        (hero.Modes(AI::HEROES_WAITING) ? "WAITING," : "") <<
	(hero.Modes(AI::HEROES_STUPID) ? "STUPID" : "") << std::endl;

    os << "ai primary target: " << ai_hero.primary_target << std::endl <<
          "ai sheduled visit: ";
    for(Queue::const_iterator
	it = task.begin(); it != task.end(); ++it)
	os << *it << "(" << MP2::StringObject(world.GetTiles(*it).GetObject()) << "), ";
    os << std::endl;

    return os.str();
}

void AI::HeroesPostLoad(Heroes & hero)
{
    hero.SetModes(AI::HEROES_HUNTER);
}

void AI::HeroesLevelUp(Heroes & hero)
{
    if(4 < hero.GetLevel() && !hero.Modes(AI::HEROES_HUNTER))
	hero.SetModes(AI::HEROES_HUNTER);

    if(9 < hero.GetLevel() && hero.Modes(AI::HEROES_SCOUTER))
	hero.ResetModes(AI::HEROES_SCOUTER);
}

void AI::HeroesPreBattle(HeroBase & hero)
{
    Castle* castle = world.GetCastle(hero.GetCenter());
    if(castle && hero.GetType() != HeroBase::CAPTAIN)
	hero.GetArmy().JoinTroops(castle->GetArmy());
}

void AI::HeroesAfterBattle(HeroBase & hero)
{
}

void AI::HeroesClearTask(const Heroes & hero)
{
    AIHeroes::Get(hero).ClearTasks();
}

bool AIHeroesValidObject2(const Heroes* hero, s32 index)
{
    const Heroes & hero2 = *hero;
    return AI::HeroesValidObject(hero2, index);
}

// get priority object for AI independent of distance (1 day)
bool AIHeroesPriorityObject(const Heroes & hero, s32 index)
{
    Maps::Tiles & tile = world.GetTiles(index);

    if(MP2::OBJ_CASTLE == tile.GetObject())
    {
	const Castle* castle = world.GetCastle(Maps::GetPoint(index));
	if(castle)
	{
	    if(hero.GetColor() == castle->GetColor())
	    {
		// maybe need join army
		return hero.Modes(AI::HEROES_HUNTER) &&
		    castle->GetArmy().isValid() &&
		    ! hero.isVisited(world.GetTiles(castle->GetIndex()));
	    }
	    else
	    if(! hero.isFriends(castle->GetColor()))
		return AI::HeroesValidObject(hero, index);
	}
    }
    else
    if(MP2::OBJ_HEROES == tile.GetObject())
    {
	// kill enemy hero
	const Heroes* hero2 = tile.GetHeroes();
	return hero2 &&
		! hero.isFriends(hero2->GetColor()) &&
		AI::HeroesValidObject(hero, index);
    }

    switch(tile.GetObject())
    {
	case MP2::OBJ_MONSTER:
	case MP2::OBJ_SAWMILL:
	case MP2::OBJ_MINES:
	case MP2::OBJ_ALCHEMYLAB:

	case MP2::OBJ_ARTIFACT:
	case MP2::OBJ_RESOURCE:
	case MP2::OBJ_CAMPFIRE:
	case MP2::OBJ_TREASURECHEST:

	return AI::HeroesValidObject(hero, index);

	default: break;
    }

    return false;
}

s32  FindUncharteredTerritory(Heroes & hero, u32 scoute)
{
    Maps::Indexes v = Maps::GetAroundIndexes(hero.GetIndex(), scoute, true);
    Maps::Indexes res;

    v.resize(std::distance(v.begin(),
	std::remove_if(v.begin(), v.end(), std::ptr_fun(&Maps::TileIsUnderProtection))));


#if defined(ANDROID)
    const MapsIndexes::const_reverse_iterator crend = v.rend();

    for(MapsIndexes::const_reverse_iterator
	it = v.rbegin(); it != crend && res.size() < 4; ++it)
#else
    for(MapsIndexes::const_reverse_iterator
	it = v.rbegin(); it != v.rend() && res.size() < 4; ++it)
#endif
    {
	// find fogs
	if(world.GetTiles(*it).isFog(hero.GetColor()) &&
    	    world.GetTiles(*it).isPassable(&hero, Direction::CENTER, true) &&
	    hero.GetPath().Calculate(*it))
	    res.push_back(*it);
    }

    const s32 result = res.size() ? *Rand::Get(res) : -1;

    if(0 <= result)
    {
	DEBUG(DBG_AI, DBG_INFO, Color::String(hero.GetColor()) <<
                ", hero: " << hero.GetName() << ", added task: " << result);
    }

    return result;
}

s32  GetRandomHeroesPosition(Heroes & hero, u32 scoute)
{
    Maps::Indexes v = Maps::GetAroundIndexes(hero.GetIndex(), scoute, true);
    Maps::Indexes res;

    v.resize(std::distance(v.begin(),
	std::remove_if(v.begin(), v.end(), std::ptr_fun(&Maps::TileIsUnderProtection))));

#if defined(ANDROID)
    const MapsIndexes::const_reverse_iterator crend = v.rend();

    for(MapsIndexes::const_reverse_iterator
	it = v.rbegin(); it != crend && res.size() < 4; ++it)
#else
    for(MapsIndexes::const_reverse_iterator
	it = v.rbegin(); it != v.rend() && res.size() < 4; ++it)
#endif
    {
        if(world.GetTiles(*it).isPassable(&hero, Direction::CENTER, true) &&
	    hero.GetPath().Calculate(*it))
	    res.push_back(*it);
    }

    const s32 result = res.size() ? *Rand::Get(res) : -1;

    if(0 <= result)
    {
	DEBUG(DBG_AI, DBG_INFO, Color::String(hero.GetColor()) <<
                ", hero: " << hero.GetName() << ", added task: " << result);
    }

    return result;
}

void AIHeroesAddedRescueTask(Heroes & hero)
{
    AIHero & ai_hero = AIHeroes::Get(hero);
    Queue & task = ai_hero.sheduled_visit;

    DEBUG(DBG_AI, DBG_TRACE, hero.GetName());

    u32 scoute = hero.GetScoute();

    switch(Settings::Get().GameDifficulty())
    {
        case Difficulty::NORMAL:    scoute += 2; break;
        case Difficulty::HARD:      scoute += 3; break;
        case Difficulty::EXPERT:    scoute += 4; break;
        case Difficulty::IMPOSSIBLE:scoute += 6; break;
        default: break;
    }

    // find unchartered territory
    s32 index = FindUncharteredTerritory(hero, scoute);
    const Maps::Tiles & tile = world.GetTiles(hero.GetIndex());

    if(index < 0)
    {
	// check teleports
	if(MP2::OBJ_STONELIGHTS == tile.GetObject(false) ||
	    MP2::OBJ_WHIRLPOOL == tile.GetObject(false))
	{
	    AI::HeroesAction(hero, hero.GetIndex());
	}
	else
	{
	    // random
	    index = GetRandomHeroesPosition(hero, scoute);
	}
    }

    if(0 <= index) task.push_back(index);
}

void AIHeroesAddedTask(Heroes & hero)
{
    AIHero & ai_hero = AIHeroes::Get(hero);
    AIKingdom & ai_kingdom = AIKingdoms::Get(hero.GetColor());

    Queue & task = ai_hero.sheduled_visit;
    IndexObjectMap & ai_objects = ai_kingdom.scans;

    // load minimal distance tasks
    std::vector<IndexDistance> objs;
    objs.reserve(ai_objects.size());

    for(std::map<s32, int>::const_iterator
	it = ai_objects.begin(); it != ai_objects.end(); ++it)
    {
	const Maps::Tiles & tile = world.GetTiles((*it).first);

	if(hero.isShipMaster())
	{
	    if(MP2::OBJ_COAST != tile.GetObject() &&
		! tile.isWater()) continue;

	    // check previous positions
	    if(MP2::OBJ_COAST == (*it).second &&
		hero.isVisited(world.GetTiles((*it).first))) continue;
	}
	else
	{
	    if(tile.isWater() && MP2::OBJ_BOAT != tile.GetObject()) continue;
	}

	objs.push_back(IndexDistance((*it).first,
			    Maps::GetApproximateDistance(hero.GetIndex(), (*it).first)));
    }

    DEBUG(DBG_AI, DBG_INFO, Color::String(hero.GetColor()) <<
		    ", hero: " << hero.GetName() << ", task prepare: " << objs.size());

    std::sort(objs.begin(), objs.end(), IndexDistance::Shortest);

    for(std::vector<IndexDistance>::const_iterator
	it = objs.begin(); it != objs.end(); ++it)
    {
	if(task.size() >= HERO_MAX_SHEDULED_TASK) break;
	const bool validobj = AI::HeroesValidObject(hero, (*it).first);

	if(validobj &&
	    hero.GetPath().Calculate((*it).first))
	{
	    DEBUG(DBG_AI, DBG_INFO, Color::String(hero.GetColor()) <<
		    ", hero: " << hero.GetName() << ", added tasks: " <<
		    MP2::StringObject(ai_objects[(*it).first]) << ", index: " << (*it).first <<
		    ", distance: " << (*it).second);

	    task.push_back((*it).first);
	    ai_objects.erase((*it).first);
	}
	else
	{
	    DEBUG(DBG_AI, DBG_TRACE, Color::String(hero.GetColor()) <<
		    ", hero: " << hero.GetName() << (!validobj ? ", invalid: " : ", impossible: ") <<
		    MP2::StringObject(ai_objects[(*it).first]) << ", index: " << (*it).first <<
		    ", distance: " << (*it).second);
	}
    }

    if(task.empty())
	AIHeroesAddedRescueTask(hero);
}

void AI::HeroesActionNewPosition(Heroes & hero)
{
    AIHero & ai_hero = AIHeroes::Get(hero);
    //AIKingdom & ai_kingdom = AIKingdoms::Get(hero.GetColor());
    Queue & task = ai_hero.sheduled_visit;

    const u8 objs[] = { MP2::OBJ_ARTIFACT, MP2::OBJ_RESOURCE, MP2::OBJ_CAMPFIRE, MP2::OBJ_TREASURECHEST, 0 };
    Maps::Indexes pickups = Maps::ScanAroundObjects(hero.GetIndex(), objs);

    if(pickups.size() && hero.GetPath().isValid() &&
	pickups.end() == std::find(pickups.begin(), pickups.end(), hero.GetPath().GetDestinationIndex()))
	hero.GetPath().Reset();

    for(MapsIndexes::const_iterator
	it = pickups.begin(); it != pickups.end(); ++it)
	if(*it != hero.GetPath().GetDestinationIndex())
	    task.push_front(*it);
}

bool AI::HeroesGetTask(Heroes & hero)
{
    std::vector<s32> results;
    results.reserve(5);

    const Settings & conf = Settings::Get();
    AIHero & ai_hero = AIHeroes::Get(hero);
    AIKingdom & ai_kingdom = AIKingdoms::Get(hero.GetColor());

    Queue & task = ai_hero.sheduled_visit;
    IndexObjectMap & ai_objects = ai_kingdom.scans;

    const u8 objs1[] = { MP2::OBJ_ARTIFACT, MP2::OBJ_RESOURCE, MP2::OBJ_CAMPFIRE, MP2::OBJ_TREASURECHEST, 0 };
    const u8 objs2[] = { MP2::OBJ_SAWMILL, MP2::OBJ_MINES, MP2::OBJ_ALCHEMYLAB, 0 };
    const u8 objs3[] = { MP2::OBJ_CASTLE, MP2::OBJ_HEROES, MP2::OBJ_MONSTER, 0 };

    // rescan path
    hero.RescanPath();

    Castle* castle = hero.inCastle();
    // if hero in castle
    if(castle)
    {
	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", in castle");

	castle->RecruitAllMonster();
	hero.GetArmy().UpgradeTroops(*castle);

	// recruit army
	if(hero.Modes(AI::HEROES_HUNTER))
		hero.GetArmy().JoinStrongestFromArmy(castle->GetArmy());
	else
	if(hero.Modes(AI::HEROES_SCOUTER))
		hero.GetArmy().KeepOnlyWeakestTroops(castle->GetArmy());

	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", " << hero.GetArmy().String());
    }

    // patrol task
    if(hero.Modes(Heroes::PATROL))
    {
	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", is patrol mode");

	// goto patrol center
	if(hero.GetCenterPatrol() != hero.GetCenter() &&
	   hero.GetPath().Calculate(Maps::GetIndexFromAbsPoint(hero.GetCenterPatrol())))
		return true;

	// scan enemy hero
	if(hero.GetSquarePatrol())
	{
	    const Maps::Indexes & results = Maps::ScanAroundObject(Maps::GetIndexFromAbsPoint(hero.GetCenterPatrol()),
									hero.GetSquarePatrol(), MP2::OBJ_HEROES);
	    for(MapsIndexes::const_iterator
		it = results.begin(); it != results.end(); ++it)
	    {
		const Heroes* enemy = world.GetTiles(*it).GetHeroes();
		if(enemy && ! enemy->isFriends(hero.GetColor()))
		{
		    if(hero.GetPath().Calculate(enemy->GetIndex()))
		    {
			DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", find enemy");
			return true;
		    }
		}
	    }
	}

	// can pickup objects
	if(conf.ExtHeroPatrolAllowPickup())
	{
	    const Maps::Indexes & results = Maps::ScanAroundObjects(hero.GetIndex(),
								    hero.GetSquarePatrol(), objs1);
	    for(MapsIndexes::const_iterator
		it = results.begin(); it != results.end(); ++it)
    		if(AI::HeroesValidObject(hero, *it) &&
		    hero.GetPath().Calculate(*it))
	    {
		ai_objects.erase(*it);

		DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ": find object: " <<
			MP2::StringObject(world.GetTiles(*it).GetObject()) << "(" << *it << ")");
		return true;
	    }
	}

	// random move
	/*
	// disable move: https://sourceforge.net/tracker/?func=detail&aid=3157397&group_id=96859&atid=616180
	{
	    Maps::ScanAroundObject(hero.GetIndex(), hero.GetSquarePatrol(), MP2::OBJ_ZERO);
	    if(results.size())
	    {
		std::random_shuffle(results.begin(), results.end());
		std::vector<s32>::const_iterator it = results.begin();
		for(; it != results.end(); ++it)
		    if(world.GetTiles(*it).isPassable(&hero, Direction::CENTER, true) &&
			hero.GetPath().Calculate(*it))
		{
		    DEBUG(DBG_AI, Color::String(hero.GetColor()) <<
			", Patrol " << hero.GetName() << ": move: " << *it);
		    return;
		}
	    }
	}
	*/

	hero.SetModes(AI::HEROES_STUPID);
	return false;
    }

    if(ai_hero.fix_loop > 3)
    {
	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ": loop");
	hero.SetModes(hero.Modes(AI::HEROES_WAITING) ? AI::HEROES_STUPID : AI::HEROES_WAITING);
	return false;
    }

    // primary target
    if(Maps::isValidAbsIndex(ai_hero.primary_target))
    {
	if(hero.GetIndex() == ai_hero.primary_target)
	{
	    ai_hero.primary_target = -1;
	    hero.GetPath().Reset();
	    DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", reset path");
	}
	else
	{
	    DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", primary target: " <<
		    ai_hero.primary_target << ", " << MP2::StringObject(world.GetTiles(ai_hero.primary_target).GetObject()));

	    const Castle* castle = NULL;

	    if(NULL != (castle = world.GetCastle(Maps::GetPoint(ai_hero.primary_target))) &&
		NULL != castle->GetHeroes().Guest() && castle->isFriends(hero.GetColor()))
	    {
		hero.SetModes(AI::HEROES_WAITING);
		DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", castle busy..");
	    }

	    // make path
	    if(ai_hero.primary_target != hero.GetPath().GetDestinationIndex() &&
		!hero.GetPath().Calculate(ai_hero.primary_target))
	    {
		DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", path unknown, primary target reset");
		ai_hero.primary_target = -1;
	    }
	}

	if(hero.GetPath().isValid()) return true;
    }

    // scan heroes and castle
    const Maps::Indexes & enemies = Maps::ScanAroundObjects(hero.GetIndex(), hero.GetScoute(), objs3);

    for(MapsIndexes::const_iterator
	it = enemies.begin(); it != enemies.end(); ++it)
	if(AIHeroesPriorityObject(hero, *it) &&
		hero.GetPath().Calculate(*it))
    {
	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", set primary target: " <<
	MP2::StringObject(world.GetTiles(*it).GetObject()) << "(" << *it << ")");

	ai_hero.primary_target = *it;
	return true;
    }

    // check destination
    if(hero.GetPath().isValid())
    {
	if(! AI::HeroesValidObject(hero, hero.GetPath().GetDestinationIndex()))
	    hero.GetPath().Reset();
	else
	if(hero.GetPath().size() < 5)
	{
	    DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", continue short");
	    ai_hero.fix_loop++;
	    return true;
	}
    }

    // scan 2x2 pickup objects
    Maps::Indexes pickups = Maps::ScanAroundObjects(hero.GetIndex(), 2, objs1);
    // scan 3x3 capture objects
    const Maps::Indexes & captures = Maps::ScanAroundObjects(hero.GetIndex(), 3, objs2);
    if(captures.size()) pickups.insert(pickups.end(), captures.begin(), captures.end());

    if(pickups.size())
    {
	hero.GetPath().Reset();

	for(MapsIndexes::const_iterator
	    it = pickups.begin(); it != pickups.end(); ++it)
    	    if(AI::HeroesValidObject(hero, *it))
	{
	    task.push_front(*it);

	    DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", find object: " <<
		MP2::StringObject(world.GetTiles(*it).GetObject()) << "(" << *it << ")");
	}
    }

    if(hero.GetPath().isValid())
    {
	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", continue");
        ai_hero.fix_loop++;
	return true;
    }

    if(task.empty())
    {
	// get task from kingdom
	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", empty task");
	AIHeroesAddedTask(hero);
    }
    else
    // remove invalid task
	task.remove_if(std::not1(std::bind1st(std::ptr_fun(&AIHeroesValidObject2), &hero)));

    // random shuffle
    if(1 < task.size() && Rand::Get(1))
    {
	Queue::iterator it1, it2;
	it2 = it1 = task.begin();
	++it2;

    	std::swap(*it1, *it2);
    }

    // find passable index
    while(task.size())
    {
	const s32 & index = task.front();

	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", look for: " << MP2::StringObject(world.GetTiles(index).GetObject()) << "(" << index << ")");
	if(hero.GetPath().Calculate(index)) break;

	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << " say: unable get object: " << index << ", remove task...");
	task.pop_front();
    }

    // success
    if(task.size())
    {
	const s32 & index = task.front();
	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << " go to: " << index);

	ai_objects.erase(index);
	task.pop_front();

	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", route: " << hero.GetPath().String());
	return true;
    }
    else
    if(hero.Modes(AI::HEROES_WAITING))
    {
	hero.GetPath().Reset();
	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << " say: unknown task., help my please..");

	hero.ResetModes(AI::HEROES_WAITING);
	hero.SetModes(AI::HEROES_STUPID);
    }
    else
    {
	DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << " say: waiting...");
	hero.SetModes(AI::HEROES_WAITING);
    }

    return false;
}

void AIHeroesTurn(Heroes* hero)
{
    if(hero) AI::HeroesTurn(*hero);
}

void AI::HeroesTurn(Heroes & hero)
{
    DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", start: " <<
	    (hero.Modes(Heroes::SHIPMASTER) ? "SHIPMASTER," : "") <<
            (hero.Modes(AI::HEROES_SCOUTER) ? "SCOUTER," : "") <<
            (hero.Modes(AI::HEROES_HUNTER) ? "HUNTER," : "") <<
            (hero.Modes(Heroes::PATROL) ? "PATROL," : "") <<
            (hero.Modes(AI::HEROES_WAITING) ? "WAITING," : "") <<
            (hero.Modes(AI::HEROES_STUPID) ? "STUPID" : ""));

    Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();

    while(hero.MayStillMove() &&
	    !hero.Modes(AI::HEROES_WAITING|AI::HEROES_STUPID))
    {
        // turn indicator
        status.RedrawTurnProgress(3);

        // get task for heroes
        AI::HeroesGetTask(hero);

        // turn indicator
        status.RedrawTurnProgress(5);

        // heroes AI turn
        AI::HeroesMove(hero);

	// turn indicator
        status.RedrawTurnProgress(7);
    }

    DEBUG(DBG_AI, DBG_TRACE, hero.GetName() << ", end");
}

bool AIHeroesScheduledVisit(const Kingdom & kingdom, s32 index)
{
    for(KingdomHeroes::const_iterator
	it = kingdom.GetHeroes().begin(); it != kingdom.GetHeroes().end(); ++it)
    {
	AIHero & ai_hero = AIHeroes::Get(**it);
	Queue & task = ai_hero.sheduled_visit;
	if(task.isPresent(index)) return true;
    }
    return false;
}

bool IsPriorityAndNotVisitAndNotPresent(const std::pair<s32, int> & indexObj, const Heroes* hero)
{
    AIHero & ai_hero = AIHeroes::Get(*hero);
    Queue & task = ai_hero.sheduled_visit;

    return AIHeroesPriorityObject(*hero, indexObj.first) &&
	    ! AIHeroesScheduledVisit(hero->GetKingdom(), indexObj.first) &&
	    ! task.isPresent(indexObj.first);
}

void AIHeroesEnd(Heroes* hero)
{
    if(hero)
    {
	AIHero & ai_hero = AIHeroes::Get(*hero);
	AIKingdom & ai_kingdom = AIKingdoms::Get(hero->GetColor());
	Queue & task = ai_hero.sheduled_visit;
	IndexObjectMap & ai_objects = ai_kingdom.scans;

	if(hero->Modes(AI::HEROES_WAITING|AI::HEROES_STUPID))
	{
	    ai_hero.Reset();
	    hero->ResetModes(AI::HEROES_WAITING|AI::HEROES_STUPID);
	}

	IndexObjectMap::iterator it;

	while(true)
	{
	    for(it = ai_objects.begin(); it != ai_objects.end(); ++it)
		if(IsPriorityAndNotVisitAndNotPresent(*it, hero)) break;

	    if(ai_objects.end() == it) break;

	    DEBUG(DBG_AI, DBG_TRACE, hero->GetName() << ", added priority object: " <<
		MP2::StringObject((*it).second) << ", index: " << (*it).first);
	    task.push_front((*it).first);
	    ai_objects.erase((*it).first);
	}
    }
}


void AIHeroesSetHunterWithTarget(Heroes* hero, s32 dst)
{
    if(hero)
    {
	AIHero & ai_hero = AIHeroes::Get(*hero);

	hero->SetModes(AI::HEROES_HUNTER);

	if(0 > ai_hero.primary_target)
	{
	    ai_hero.primary_target = dst;
	}
    }
}

void AIHeroesCaptureNearestTown(Heroes* hero)
{
    if(hero)
    {
	AIHero & ai_hero = AIHeroes::Get(*hero);

	if(0 > ai_hero.primary_target)
	{
	    const Maps::Indexes & castles = Maps::GetObjectPositions(hero->GetIndex(), MP2::OBJ_CASTLE, true);

	    for(MapsIndexes::const_iterator
		it = castles.begin(); it != castles.end(); ++it)
	    {
		const Castle* castle = world.GetCastle(Maps::GetPoint(*it));

		if(castle)
		    DEBUG(DBG_AI, DBG_TRACE, hero->GetName() << ", to castle: " << castle->GetName());

		if(castle &&
		    Army::TroopsStrongerEnemyTroops(hero->GetArmy(), castle->GetArmy()))
		{
		    ai_hero.primary_target = *it;

		    DEBUG(DBG_AI, DBG_INFO, Color::String(hero->GetColor()) <<
			    ", Hero " << hero->GetName() << " set primary target: " << *it);
		    break;
		}
	    }
	}
    }
}
