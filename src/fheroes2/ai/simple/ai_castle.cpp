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

#include "world.h"
#include "kingdom.h"
#include "heroes.h"
#include "castle.h"
#include "race.h"
#include "game.h"
#include "ai_simple.h"

void AICastleDefense(Castle & c)
{
    if(c.isCastle())
    {
        if(!c.isBuild(BUILD_LEFTTURRET))
		c.BuyBuilding(BUILD_LEFTTURRET);

        if(!c.isBuild(BUILD_RIGHTTURRET))
		c.BuyBuilding(BUILD_RIGHTTURRET);

        if(!c.isBuild(BUILD_MOAT))
    		c.BuyBuilding(BUILD_MOAT);

        if(!c.isBuild(BUILD_CAPTAIN) && NULL == c.GetHeroes().Guest())
		c.BuyBuilding(BUILD_CAPTAIN);

        if(!c.isBuild(BUILD_TAVERN) && Race::KNGT == c.GetRace())
		c.BuyBuilding(BUILD_TAVERN);

        if(!c.isBuild(BUILD_SPEC))
		c.BuyBuilding(BUILD_SPEC);
    }
    c.RecruitAllMonster();
}

void AICastleDevelopment(Castle & c)
{
    const Kingdom & kingdom = c.GetKingdom();

    if(c.isCastle())
    {
	// build for capital or large golds
	if(c.isCapital() || kingdom.GetFunds().Get(Resource::GOLD) > 8000)
	{
	    if(!c.isBuild(BUILD_STATUE))
		c.BuyBuilding(BUILD_STATUE);

	    if(!c.isBuild(BUILD_SPEC) && Race::WRLK == c.GetRace())
		c.BuyBuilding(BUILD_SPEC);

	    if(!c.isBuild(BUILD_TAVERN) && (Race::KNGT == c.GetRace() || Race::SORC == c.GetRace()))
		c.BuyBuilding(BUILD_TAVERN);

	    if(!c.isBuild(BUILD_MAGEGUILD1) && ((Race::SORC | Race::WZRD | Race::WRLK | Race::NECR) & c.GetRace()))
		c.BuyBuilding(BUILD_MAGEGUILD1);

	    if(!c.isBuild(BUILD_WELL))
		c.BuyBuilding(BUILD_WELL);


	    if(!c.isBuild(DWELLING_MONSTER1))
		c.BuyBuilding(DWELLING_MONSTER1);

	    if(!c.isBuild(DWELLING_MONSTER2))
		c.BuyBuilding(DWELLING_MONSTER2);

	    if(!c.isBuild(DWELLING_MONSTER3))
		c.BuyBuilding(DWELLING_MONSTER3);

	    if(!c.isBuild(DWELLING_MONSTER4))
		c.BuyBuilding(DWELLING_MONSTER4);


	    if(!c.isBuild(BUILD_THIEVESGUILD) && ((Race::NECR) & c.GetRace()))
		c.BuyBuilding(BUILD_THIEVESGUILD);

	    if(!c.isBuild(BUILD_MARKETPLACE))
		c.BuyBuilding(BUILD_MARKETPLACE);

	    if(!c.isBuild(BUILD_MAGEGUILD1))
		c.BuyBuilding(BUILD_MAGEGUILD1);

	    if(!c.isBuild(BUILD_MAGEGUILD2) && ((Race::SORC | Race::WZRD | Race::WRLK | Race::NECR) & c.GetRace()))
		c.BuyBuilding(BUILD_MAGEGUILD2);

	    if(!c.isBuild(DWELLING_UPGRADE2))
		c.BuyBuilding(DWELLING_UPGRADE2);

	    if(!c.isBuild(DWELLING_UPGRADE3))
		c.BuyBuilding(DWELLING_UPGRADE3);

	    if(!c.isBuild(DWELLING_UPGRADE4))
		c.BuyBuilding(DWELLING_UPGRADE4);

	    if(!c.isBuild(BUILD_LEFTTURRET))
		c.BuyBuilding(BUILD_LEFTTURRET);

	    if(!c.isBuild(BUILD_RIGHTTURRET))
		c.BuyBuilding(BUILD_RIGHTTURRET);

	    if(!c.isBuild(BUILD_MOAT))
		c.BuyBuilding(BUILD_MOAT);

	    if(!c.isBuild(BUILD_CAPTAIN))
		c.BuyBuilding(BUILD_CAPTAIN);

	    if(!c.isBuild(BUILD_TAVERN))
		c.BuyBuilding(BUILD_TAVERN);

	    if(!c.isBuild(BUILD_MAGEGUILD2))
		c.BuyBuilding(BUILD_MAGEGUILD2);

	    if(!c.isBuild(DWELLING_MONSTER5))
		c.BuyBuilding(DWELLING_MONSTER5);

	    if(!c.isBuild(DWELLING_MONSTER6))
		c.BuyBuilding(DWELLING_MONSTER6);

	    if(!c.isBuild(BUILD_MAGEGUILD3))
		c.BuyBuilding(BUILD_MAGEGUILD3);

	    if(!c.isBuild(DWELLING_UPGRADE5))
		c.BuyBuilding(DWELLING_UPGRADE5);

	    if(!c.isBuild(DWELLING_UPGRADE6))
		c.BuyBuilding(DWELLING_UPGRADE6);
	}
    }
    else
    {
	// build castle only monday or tuesday or for capital
	if(c.isCapital() || 3 > world.GetDay()) c.BuyBuilding(BUILD_CASTLE);
    }

    // last day and buy monster
    if(world.LastDay()) c.RecruitAllMonster();
}

void AICastleTurn(Castle* castle)
{
    if(castle) AI::CastleTurn(*castle);
}

void AI::CastleTurn(Castle & castle)
{
    // skip neutral town
    if(castle.GetColor() != Color::NONE)
    {
	s32 range = Game::GetViewDistance(castle.isCastle() ? Game::VIEW_CASTLE : Game::VIEW_TOWN);
	const Heroes* enemy = NULL;

	// find enemy hero
	for(s32 y = -range; y <= range; ++y)
    	    for(s32 x = -range; x <= range; ++x)
	{
    	    if(!y && !x) continue;
	    const Point & center = castle.GetCenter();

    	    if(Maps::isValidAbsPoint(center.x + x, center.y + y))
    	    {
        	const Maps::Tiles & tile = world.GetTiles(Maps::GetIndexFromAbsPoint(center.x + x, center.y + y));

        	if(MP2::OBJ_HEROES == tile.GetObject())
		    enemy = tile.GetHeroes();

		if(enemy && castle.GetColor() == enemy->GetColor())
		    enemy = NULL;

		if(enemy) break;
	    }
	}

	enemy ? AICastleDefense(castle) : AICastleDevelopment(castle);

	Kingdom & kingdom = castle.GetKingdom();
	Heroes* hero = castle.GetHeroes().Guest();
	bool can_recruit = castle.isCastle() && !hero && kingdom.GetHeroes().size() < Kingdom::GetMaxHeroes();

	// part II
	if(enemy &&
	    castle.GetArmy().isValid() &&
	    Army::TroopsStrongerEnemyTroops(castle.GetArmy(), enemy->GetArmy()))
	{
    	    if(can_recruit)
    	    {
        	Recruits & rec = kingdom.GetRecruits();

        	if(rec.GetHero1())
		    hero = castle.RecruitHero(rec.GetHero1());
        	else
        	if(rec.GetHero2())
		    hero = castle.RecruitHero(rec.GetHero2());
    	    }

    	    if(hero)
		hero->SetModes(AI::HEROES_HUNTER);
	}

	// part III
	AIKingdom & ai = AIKingdoms::Get(castle.GetColor());
	if(ai.capital != &castle &&
	    castle.GetArmy().isValid() && ! hero &&
	    2 < castle.GetArmy().GetCount() &&
	    150 < castle.GetArmy().GetHitPoints() &&
	    can_recruit)
	{
    	    Recruits & rec = kingdom.GetRecruits();

    	    if(rec.GetHero1())
		hero = castle.RecruitHero(rec.GetHero1());
    	    else
    	    if(rec.GetHero2())
		hero = castle.RecruitHero(rec.GetHero2());

    	    if(hero)
		hero->SetModes(AI::HEROES_HUNTER|AI::HEROES_SCOUTER);
	}
    }
}

void AI::CastlePreBattle(Castle & castle)
{
    Heroes* hero = castle.GetHeroes().GuardFirst();
    if(hero && castle.GetArmy().isValid())
	hero->GetArmy().JoinStrongestFromArmy(castle.GetArmy());
}

void AI::CastleAfterBattle(Castle &, bool attacker_wins)
{
}
