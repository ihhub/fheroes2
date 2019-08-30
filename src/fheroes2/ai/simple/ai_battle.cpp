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

#include <algorithm>
#include <functional>
#include "settings.h"
#include "heroes.h"
#include "castle.h"
#include "speed.h"
#include "battle_arena.h"
#include "battle_cell.h"
#include "battle_troop.h"
#include "battle_interface.h"
#include "battle_command.h"
#include "ai_simple.h"

namespace Battle
{
    bool	AIApplySpell(const Spell &, const Unit*, const HeroBase &, Actions &);
    s32		AIShortDistance(s32, const Indexes &);
    s32		AIAttackPosition(Arena &, const Unit &, const Indexes &);
    s32		AIMaxQualityPosition(const Indexes &);
    const Unit* AIGetEnemyAbroadMaxQuality(s32, int color);
    const Unit* AIGetEnemyAbroadMaxQuality(const Unit &);
    s32		AIAreaSpellDst(const HeroBase &);

    bool MaxDstCount(const std::pair<s32, u32> & p1, const std::pair<s32, u32> & p2) { return p1.second < p2.second; }
}

s32 Battle::AIAreaSpellDst(const HeroBase & hero)
{
    std::map<s32, u32> dstcount;

    Arena* arena = GetArena();
    Units enemies(arena->GetForce(hero.GetColor(), true), true);

    for(Units::const_iterator
	it1 = enemies.begin(); it1 != enemies.end(); ++it1)
    {
	const Indexes around = Board::GetAroundIndexes(**it1);

	for(Indexes::const_iterator
	    it2 = around.begin(); it2 != around.end(); ++it2)
	    dstcount[*it2] += 1;
    }

    // find max
    std::map<s32, u32>::const_iterator max = std::max_element(dstcount.begin(), dstcount.end(), MaxDstCount);

    return max != dstcount.end() ? (*max).first : -1;
}

s32 Battle::AIMaxQualityPosition(const Indexes & positions)
{
    s32 res = -1;

    for(Indexes::const_iterator
	it = positions.begin(); it != positions.end(); ++it)
    if(Board::isValidIndex(*it))
    {
	if(res < 0)
	    res = *it;
	else
        if(Board::GetCell(res)->GetQuality() < Board::GetCell(*it)->GetQuality())
	    res = *it;
    }

    return res;
}

const Battle::Unit* Battle::AIGetEnemyAbroadMaxQuality(s32 position, int color)
{
    const Unit* res = NULL;
    s32 quality = 0;

    const Indexes around = Board::GetAroundIndexes(position);

    for(Indexes::const_iterator
	it = around.begin(); it != around.end(); ++it)
    {
        const Cell* cell = Board::GetCell(*it);
        const Unit* enemy = cell ? cell->GetUnit() : NULL;

        if(enemy && enemy->GetColor() != color &&
			quality < cell->GetQuality())
        {
            res = enemy;
            quality = cell->GetQuality();
        }
    }

    return res;
}

const Battle::Unit* Battle::AIGetEnemyAbroadMaxQuality(const Unit & b)
{
    const Unit* res1 = AIGetEnemyAbroadMaxQuality(b.GetHeadIndex(), b.GetColor());

    if(b.isWide())
    {
        const Unit* res2 = AIGetEnemyAbroadMaxQuality(b.GetTailIndex(), b.GetColor());

        if(!res1) return res2;
        else
        if(!res2) return res1;
        else
        {
            const s32 & quality1 = res1->GetPosition().GetHead()->GetQuality();
            const s32 & quality2 = res2->GetPosition().GetHead()->GetQuality();

            return quality1 > quality2 ? res1 : res2;
        }
    }

    return res1;
}

s32 Battle::AIShortDistance(s32 from, const Indexes & indexes)
{
    u32 len = MAXU16;
    s32 res = -1;

    for(Indexes::const_iterator
        it = indexes.begin(); it != indexes.end(); ++it)
    {
        const u32 length = Board::GetDistance(from, *it);

        if(len > length)
        {
            len = length;
            res = *it;
        }
    }

    DEBUG(DBG_BATTLE, DBG_TRACE, res);

    return res;
}

s32 Battle::AIAttackPosition(Arena & arena, const Unit & b, const Indexes & positions)
{
    s32 res = -1;

    if(b.isMultiCellAttack())
    {
        res = AIMaxQualityPosition(positions);
    }
    else
    if(b.isDoubleCellAttack())
    {
        Indexes results;
        results.reserve(12);

	const Units enemies(arena.GetForce(b.GetColor(), true), true);

	if(1 < enemies.size())
	{
	    for(Units::const_iterator
		it1 = enemies.begin(); it1 != enemies.end(); ++it1)
	    {
		const Indexes around = Board::GetAroundIndexes(**it1);

		for(Indexes::const_iterator
		    it2 = around.begin(); it2 != around.end(); ++it2)
		{
		    const Unit* unit = Board::GetCell(*it2)->GetUnit();
		    if(unit && enemies.end() != std::find(enemies.begin(), enemies.end(), unit))
			results.push_back(*it2);
		}
	    }

    	    if(results.size())
    	    {
        	// find passable results
        	Indexes passable = Arena::GetBoard()->GetPassableQualityPositions(b);
        	Indexes::iterator it2 = results.begin();

        	for(Indexes::const_iterator
		    it = results.begin(); it != results.end(); ++it)
            	    if(passable.end() != std::find(passable.begin(), passable.end(), *it))
                	*it2++ = *it;

        	if(it2 != results.end())
            	    results.resize(std::distance(results.begin(), it2));

        	// get max quality
        	if(results.size())
            	    res = AIMaxQualityPosition(results);
    	    }
	}
    }

    return 0 > res ? AIShortDistance(b.GetHeadIndex(), positions) : res;
}


using namespace Battle;

void AI::BattleTurn(Arena & arena, const Unit & b, Actions & a)
{
    Board* board = Arena::GetBoard();

    // reset quality param for board
    board->Reset();

    // set quality for enemy troop
    board->SetEnemyQuality(b);

    const Unit* enemy = NULL;
    bool attack = false;

    if(b.isArchers() && !b.isHandFighting())
    {
	enemy = arena.GetEnemyMaxQuality(b.GetColor());
	if(BattleMagicTurn(arena, b, a, enemy)) return; /* repeat turn: correct spell ability */
	attack = true;
    }
    else
    if(b.isHandFighting())
    {
	enemy = AIGetEnemyAbroadMaxQuality(b);
	if(BattleMagicTurn(arena, b, a, enemy)) return; /* repeat turn: correct spell ability */
	attack = true;
    }
    else
    {
	s32 move = -1;

	if(b.Modes(SP_BERSERKER))
	{
	    const Indexes positions = board->GetNearestTroopIndexes(b.GetHeadIndex(), NULL);
	    if(positions.size()) move = *Rand::Get(positions);
	}
	else
	{
	    if(BattleMagicTurn(arena, b, a, NULL)) return; /* repeat turn: correct spell ability */

	    // set quality position from enemy
	    board->SetPositionQuality(b);

	    // get passable quality positions
	    const Indexes positions = board->GetPassableQualityPositions(b);
	    attack = true;

	    if(positions.size())
		move = AIAttackPosition(arena, b, positions);
	}

	if(Board::isValidIndex(move))
	{
	    if(b.isFly())
	    {
		enemy = AIGetEnemyAbroadMaxQuality(move, b.GetColor());
		if(BattleMagicTurn(arena, b, a, enemy)) return; /* repeat turn: correct spell ability */
	    	a.push_back(Battle::Command(MSG_BATTLE_MOVE, b.GetUID(), move));
		attack = true;
	    }
	    else
	    {
		Position dst = Position::GetCorrect(b, move);
		Indexes path = arena.GetPath(b, dst);

		if(path.empty())
		{
		    const u32 direction = b.GetPosition().GetHead()->GetPos().x > dst.GetHead()->GetPos().x ?
						RIGHT : LEFT;
		    // find near position
		    while(path.empty() &&
			Board::isValidDirection(dst.GetHead()->GetIndex(), direction))
		    {
			const s32 pos = Board::GetIndexDirection(dst.GetHead()->GetIndex(), direction);
			if(b.GetHeadIndex() == pos) break;

			dst.Set(pos, b.isWide(), direction == RIGHT);
			path = arena.GetPath(b, dst);
		    }
		}

		if(path.size())
		{
		    if(b.isWide())
		    {
			const s32 head = dst.GetHead()->GetIndex();
			const s32 tail = dst.GetTail()->GetIndex();

			if(path.back() == head || path.back() == tail)
			{
			    enemy = AIGetEnemyAbroadMaxQuality(head, b.GetColor());

			    if(!enemy)
				enemy = AIGetEnemyAbroadMaxQuality(tail, b.GetColor());
			}
		    }

		    if(! enemy)
			enemy = AIGetEnemyAbroadMaxQuality(path.back(), b.GetColor());

	    	    a.push_back(Battle::Command(MSG_BATTLE_MOVE, b.GetUID(), path.back()));

		    // archers move and short attack only
		    attack = b.isArchers() ? false : true;
		}
	    }
	}
	else
	    enemy = AIGetEnemyAbroadMaxQuality(b);
    }

    if(enemy)
    {
	if(attack) a.push_back(Battle::Command(MSG_BATTLE_ATTACK, b.GetUID(), enemy->GetUID(), enemy->GetHeadIndex(), 0));
    }
    else
    {
	DEBUG(DBG_BATTLE, DBG_TRACE, "enemy: " << "is NULL" << ", board: " << board->AllUnitsInfo());
    }

    // end action
    a.push_back(Battle::Command(MSG_BATTLE_END_TURN, b.GetUID()));
}


bool AI::BattleMagicTurn(Arena & arena, const Unit & b, Actions & a, const Unit* enemy)
{
    const HeroBase* hero = b.GetCommander();

    if(b.Modes(SP_BERSERKER) || !hero || hero->Modes(Heroes::SPELLCASTED) || !hero->HaveSpellBook() ||
	arena.isDisableCastSpell(Spell(), NULL) || a.HaveCommand(Battle::MSG_BATTLE_CAST))
	return false;

    const Force & my_army = arena.GetForce(b.GetArmyColor(), false);
    const Force & enemy_army = arena.GetForce(b.GetArmyColor(), true);

    Units friends(my_army, true);
    Units enemies(enemy_army, true);

    // sort strongest
    friends.SortStrongest();

    // troop bad spell - clean
    {
	// sort strongest
	Units::iterator it = std::find_if(friends.begin(), friends.end(),
					    std::bind2nd(std::mem_fun(&Unit::Modes), IS_BAD_MAGIC));
	if(it != friends.end())
	{
	    if(AIApplySpell(Spell::DISPEL, *it, *hero, a)) return true;
	    if(AIApplySpell(Spell::CURE, *it, *hero, a)) return true;
	}
    }

    // area damage spell
    {
	const u8 areasp[] = { Spell::METEORSHOWER, Spell::FIREBLAST, Spell::CHAINLIGHTNING, Spell::FIREBALL, Spell::COLDRING };
	s32 dst = AIAreaSpellDst(*hero);

	if(Board::isValidIndex(dst))
	for(u32 ii = 0; ii < ARRAY_COUNT(areasp); ++ii)
	{
	    if(hero->CanCastSpell(areasp[ii]))
	    {
		a.push_back(Battle::Command(MSG_BATTLE_CAST, areasp[ii], dst));
		return true;
	    }
	}
    }

    // if handfighting
    if(enemy)
    {
	// kill dragons
	if(enemy->isDragons() &&
	    !b.Modes(SP_DRAGONSLAYER) && AIApplySpell(Spell::DRAGONSLAYER, &b, *hero, a)) return true;

	// curse
	if(!enemy->Modes(SP_CURSE) && AIApplySpell(Spell::CURSE, enemy, *hero, a)) return true;
	// enemy good spell - clean
	if(enemy->Modes(IS_GOOD_MAGIC) && AIApplySpell(Spell::DISPEL, enemy, *hero, a)) return true;

	// up defense
	if(!b.Modes(SP_STEELSKIN) && !b.Modes(SP_STONESKIN) && AIApplySpell(Spell::STEELSKIN, &b, *hero, a)) return true;
	if(!b.Modes(SP_STONESKIN) && !b.Modes(SP_STEELSKIN) && AIApplySpell(Spell::STONESKIN, &b, *hero, a)) return true;
    }

    // my army blessing
    if(b.isArchers())
    {
	if(!b.Modes(SP_BLESS) && AIApplySpell(Spell::BLESS, &b, *hero, a)) return true;
	if(!b.Modes(SP_BLOODLUST) && AIApplySpell(Spell::BLOODLUST, &b, *hero, a)) return true;
    }

    // up speed
    if(hero->HaveSpell(Spell::HASTE) && !enemy)
    {
	// sort strongest
	Units::iterator it = std::find_if(friends.begin(), friends.end(),
					    std::not1(std::bind2nd(std::mem_fun(&Unit::Modes), SP_HASTE)));
	if(it != friends.end() &&
		AIApplySpell(Spell::HASTE, *it, *hero, a)) return true;
    }

    // shield spell conditions
    {
	Units::iterator it = std::find_if(enemies.begin(), enemies.end(),
					    std::mem_fun(&Unit::isArchers));

	const Castle* castle = Arena::GetCastle();

	// find enemy archers
	if(it != enemies.end() ||
	// or archers tower
	    (castle && castle->GetColor() != b.GetColor() && castle->isCastle()))
	{
	    // find strongest archers
	    for(it = friends.begin(); it != friends.end(); ++it)
		if((*it)->isArchers() && ! (*it)->Modes(SP_SHIELD)) break;

	    // or other strongest friends
	    if(it == friends.end())
		it = std::find_if(friends.begin(), friends.end(),
				std::not1(std::bind2nd(std::mem_fun(&Unit::Modes), SP_SHIELD)));

	    if(it != friends.end() &&
		    AIApplySpell(Spell::SHIELD, *it, *hero, a)) return true;
	}
    }


    // enemy army spell
    {
	// find mirror image or summon elem
	Units::iterator it = std::find_if(enemies.begin(), enemies.end(),
					    std::bind2nd(std::mem_fun(&Unit::Modes), CAP_MIRRORIMAGE | CAP_SUMMONELEM));

	if(it != enemies.end())
	{
	    if(AIApplySpell(Spell::ARROW, *it, *hero, a)) return true;
	    if(AIApplySpell(Spell::LIGHTNINGBOLT, *it, *hero, a)) return true;
	}

	// find good magic
	it = std::find_if(enemies.begin(), enemies.end(),
					std::bind2nd(std::mem_fun(&Unit::Modes), IS_GOOD_MAGIC));

	if(it != enemies.end())
	{
	    // slow
	    if((*it)->Modes(SP_HASTE) && AIApplySpell(Spell::SLOW, *it, *hero, a)) return true;
	    // curse
	    if((*it)->Modes(SP_CURSE) && AIApplySpell(Spell::CURSE, *it, *hero, a)) return true;
	    //
	    if(AIApplySpell(Spell::DISPEL, *it, *hero, a)) return true;
	}

	// check undead
	if(std::count_if(friends.begin(), friends.end(), std::mem_fun(&Unit::isUndead)) <
		    std::count_if(enemies.begin(), enemies.end(), std::mem_fun(&Unit::isUndead)))
	{
	    if(AIApplySpell(Spell::HOLYSHOUT, NULL, *hero, a)) return true;
	    if(AIApplySpell(Spell::HOLYWORD, NULL, *hero, a)) return true;
	}

	// check alife
	if(std::count_if(friends.begin(), friends.end(), std::mem_fun(&Unit::isAlive)) <
		    std::count_if(enemies.begin(), enemies.end(), std::mem_fun(&Unit::isAlive)))
	{
	    if(AIApplySpell(Spell::DEATHRIPPLE, NULL, *hero, a)) return true;
	    if(AIApplySpell(Spell::DEATHWAVE, NULL, *hero, a)) return true;
	}

	Unit* stats = *Rand::Get(enemies);

	if(AIApplySpell(Spell::LIGHTNINGBOLT, stats, *hero, a)) return true;
	if(AIApplySpell(Spell::ARROW, stats, *hero, a)) return true;
    }

/*
    FIX: Damage Spell:
*/

    if(AIApplySpell(Spell::ARMAGEDDON, NULL, *hero, a)) return true;
    if(AIApplySpell(Spell::ELEMENTALSTORM, NULL, *hero, a)) return true;

    return false;
}

bool Battle::AIApplySpell(const Spell & spell, const Unit* b, const HeroBase & hero, Actions & a)
{
    u32 mass = Spell::NONE;

    switch(spell())
    {
        case Spell::CURE:	mass = Spell::MASSCURE; break;
        case Spell::HASTE:	mass = Spell::MASSHASTE; break;
        case Spell::SLOW:	mass = Spell::MASSSLOW; break;
        case Spell::BLESS:	mass = Spell::MASSBLESS; break;
        case Spell::CURSE:	mass = Spell::MASSCURSE; break;
        case Spell::DISPEL:	mass = Spell::MASSDISPEL; break;
        case Spell::SHIELD:	mass = Spell::MASSSHIELD; break;

	default: break;
    }

    if(mass != Spell::NONE &&
	AIApplySpell(mass, b, hero, a)) return true;

    if(hero.CanCastSpell(spell) && (!b || b->AllowApplySpell(spell, &hero)))
    {
	a.push_back(Battle::Command(MSG_BATTLE_CAST, spell(), (b ? b->GetHeadIndex() : -1)));
	return true;
    }

    return false;
}
