/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include "army.h"
#include "color.h"
#include "cursor.h"
#include "artifact.h"
#include "settings.h"
#include "heroes_base.h"
#include "skill.h"
#include "agg.h"
#include "text.h"
#include "dialog.h"
#include "world.h"
#include "kingdom.h"
#include "game.h"
#include "ai.h"
#include "battle_arena.h"
#include "battle_army.h"

namespace Battle
{
    void PickupArtifactsAction(HeroBase &, HeroBase &, bool);
    void EagleEyeSkillAction(HeroBase &, const SpellStorage &, bool);
    void NecromancySkillAction(HeroBase &, u32, bool);
}

Battle::Result Battle::Loader(Army & army1, Army & army2, s32 mapsindex)
{
    // pre battle army1
    if(army1.GetCommander())
    {
	if(army1.GetCommander()->isCaptain())
	    army1.GetCommander()->ActionPreBattle();
	else
	if(army1.isControlAI())
    	    AI::HeroesPreBattle(*army1.GetCommander());
        else
	    army1.GetCommander()->ActionPreBattle();
    }

    // pre battle army2
    if(army2.GetCommander())
    {
	if(army2.GetCommander()->isCaptain())
	    army2.GetCommander()->ActionPreBattle();
	else
	if(army2.isControlAI())
    	    AI::HeroesPreBattle(*army2.GetCommander());
        else
	    army2.GetCommander()->ActionPreBattle();
    }

    AGG::ResetMixer();
    bool local = army1.isControlHuman() || army2.isControlHuman();

#ifdef WITH_DEBUG
    if(IS_DEBUG(DBG_BATTLE, DBG_TRACE)) local = true;
#endif

    Arena arena(army1, army2, mapsindex, local);

    DEBUG(DBG_BATTLE, DBG_INFO, "army1 " << army1.String());
    DEBUG(DBG_BATTLE, DBG_INFO, "army2 " << army2.String());

    while(arena.BattleValid())
	arena.Turns();

    const Result & result = arena.GetResult();
    AGG::ResetMixer();

    HeroBase* hero_wins = (result.army1 & RESULT_WINS ? army1.GetCommander() : (result.army2 & RESULT_WINS ? army2.GetCommander() : NULL));
    HeroBase* hero_loss = (result.army1 & RESULT_LOSS ? army1.GetCommander() : (result.army2 & RESULT_LOSS ? army2.GetCommander() : NULL));
    const u32 loss_result =  result.army1 & RESULT_LOSS ? result.army1 : result.army2;

    if(local)
    {
	// fade arena
	arena.FadeArena();

	// dialog summary
	arena.DialogBattleSummary(result);
    }

    // save count troop
    arena.GetForce1().SyncArmyCount();
    arena.GetForce2().SyncArmyCount();

    // after battle army1
    if(army1.GetCommander())
    {
	if(army1.isControlAI())
    	    AI::HeroesAfterBattle(*army1.GetCommander());
        else
	    army1.GetCommander()->ActionAfterBattle();
    }

    // after battle army2
    if(army2.GetCommander())
    {
	if(army2.isControlAI())
    	    AI::HeroesAfterBattle(*army2.GetCommander());
        else
	    army2.GetCommander()->ActionAfterBattle();
    }

    // pickup artifact
    if(hero_wins && hero_loss &&
	!((RESULT_RETREAT | RESULT_SURRENDER) & loss_result) &&
	hero_wins->isHeroes() &&
	hero_loss->isHeroes())
	PickupArtifactsAction(*hero_wins, *hero_loss, hero_wins->isControlHuman());

    // eagle eye capability
    if(hero_wins && hero_loss &&
	hero_wins->GetLevelSkill(Skill::Secondary::EAGLEEYE) &&
	hero_loss->isHeroes())
	    EagleEyeSkillAction(*hero_wins, arena.GetUsageSpells(), hero_wins->isControlHuman());

    // necromancy capability
    if(hero_wins &&
	hero_wins->GetLevelSkill(Skill::Secondary::NECROMANCY))
	    NecromancySkillAction(*hero_wins, result.killed, hero_wins->isControlHuman());

    DEBUG(DBG_BATTLE, DBG_INFO, "army1 " << army1.String());
    DEBUG(DBG_BATTLE, DBG_INFO, "army2 " << army1.String());

    // update army
    if(army1.GetCommander() && army1.GetCommander()->isHeroes())
    {
	// hard reset army
	if(!army1.isValid() || (result.army1 & RESULT_RETREAT)) army1.Reset(false);
    }

    // update army
    if(army2.GetCommander() && army2.GetCommander()->isHeroes())
    {
	// hard reset army
        if(!army2.isValid() || (result.army2 & RESULT_RETREAT)) army2.Reset(false);
    }

    DEBUG(DBG_BATTLE, DBG_INFO, "army1: " << (result.army1 & RESULT_WINS ? "wins" : "loss") << ", army2: " << (result.army2 & RESULT_WINS ? "wins" : "loss"));

    return result;
}

void Battle::PickupArtifactsAction(HeroBase & hero1, HeroBase & hero2, bool local)
{
    BagArtifacts & bag1 = hero1.GetBagArtifacts();
    BagArtifacts & bag2 = hero2.GetBagArtifacts();

    for(u32 ii = 0; ii < bag2.size(); ++ii)
    {
	Artifact & art = bag2[ii];

	if(art.isUltimate())
	{
	    art = Artifact::UNKNOWN;
	}
	else
	if(art() != Artifact::UNKNOWN && art() != Artifact::MAGIC_BOOK)
        {
            BagArtifacts::iterator it = std::find(bag1.begin(), bag1.end(), Artifact((Artifact::UNKNOWN)));
            if(bag1.end() != it)
            {
        	*it = art;
        	if(local)
		{
		    Game::PlayPickupSound();
		    Dialog::ArtifactInfo(_("You have captured an enemy artifact!"), "", art);
		}
    	    }
    	    art = Artifact::UNKNOWN;
	}
    }
}

void Battle::EagleEyeSkillAction(HeroBase & hero, const SpellStorage & spells, bool local)
{
    if(spells.empty() ||
	!hero.HaveSpellBook()) return;

    SpellStorage new_spells;
    new_spells.reserve(10);

    const Skill::Secondary eagleeye(Skill::Secondary::EAGLEEYE, hero.GetLevelSkill(Skill::Secondary::EAGLEEYE));

    // filter spells
    for(SpellStorage::const_iterator
	it = spells.begin(); it != spells.end(); ++it)
    {
	const Spell & sp = *it;
    	if(!hero.HaveSpell(sp))
	{
	    switch(eagleeye.Level())
	    {
		case Skill::Level::BASIC:
		    // 20%
		    if(3 > sp.Level() && eagleeye.GetValues() >= Rand::Get(1, 100)) new_spells.push_back(sp);
		    break;
		case Skill::Level::ADVANCED:
		    // 30%
		    if(4 > sp.Level() && eagleeye.GetValues() >= Rand::Get(1, 100)) new_spells.push_back(sp);
		    break;
		case Skill::Level::EXPERT:
		    // 40%
		    if(5 > sp.Level() && eagleeye.GetValues() >= Rand::Get(1, 100)) new_spells.push_back(sp);
		    break;
		default: break;
    	    }
	}
    }

    // add new spell
    for(SpellStorage::const_iterator
	it = new_spells.begin(); it != new_spells.end(); ++it)
    {
	const Spell & sp = *it;
	if(local)
	{
	    std::string msg = _("Through eagle-eyed observation, %{name} is able to learn the magic spell %{spell}.");
	    StringReplace(msg, "%{name}", hero.GetName());
	    StringReplace(msg, "%{spell}", sp.GetName());
	    Game::PlayPickupSound();
	    Dialog::SpellInfo("", msg, sp);
	}
    }

    hero.AppendSpellsToBook(new_spells, true);
}

void Battle::NecromancySkillAction(HeroBase & hero, u32 killed, bool local)
{
    Army & army = hero.GetArmy();

    if(0 == killed ||
	(army.isFullHouse() && !army.HasMonster(Monster::SKELETON))) return;

    // check necromancy shrine build
    u32 percent = 10 * world.GetKingdom(army.GetColor()).GetCountNecromancyShrineBuild();

    // check artifact
    u32 acount = hero.HasArtifact(Artifact::SPADE_NECROMANCY);
    if(acount) percent += acount * 10;

    // fix over 60%
    if(percent > 60) percent = 60;

    percent += hero.GetSecondaryValues(Skill::Secondary::NECROMANCY);

    // hard fix overflow
    if(percent > 90) percent = 90;

    const Monster mons(Monster::SKELETON);
    const u32 count = Monster::GetCountFromHitPoints(Monster::SKELETON, mons.GetHitPoints() * killed * percent / 100);
    army.JoinTroop(mons, count);

    if(local)
    {
	std::string msg = _("Practicing the dark arts of necromancy, you are able to raise %{count} of the enemy's dead to return under your service as %{monster}");
	StringReplace(msg, "%{count}", count);
	StringReplace(msg, "%{monster}", mons.GetPluralName(count));
	Surface sf1(Size(40, 45), false);
	const Sprite & sf2 = AGG::GetICN(ICN::MONS32, mons.GetSpriteIndex());
	sf2.Blit((sf1.w() - sf2.w()) / 2, 0, sf1);
	Text text(GetString(count), Font::SMALL);
	text.Blit((sf1.w() - text.w()) / 2, sf2.h() + 3, sf1);
	Game::PlayPickupSound();

	Dialog::SpriteInfo("", msg, sf1);
    }

    DEBUG(DBG_BATTLE, DBG_TRACE, "raise: " << count << mons.GetMultiName());
}

u32 Battle::Result::AttackerResult(void) const
{
    if(RESULT_SURRENDER & army1) return RESULT_SURRENDER;
    else
    if(RESULT_RETREAT & army1) return RESULT_RETREAT;
    else
    if(RESULT_LOSS & army1) return RESULT_LOSS;
    else
    if(RESULT_WINS & army1) return RESULT_WINS;

    return 0;
}

u32 Battle::Result::DefenderResult(void) const
{
    if(RESULT_SURRENDER & army2) return RESULT_SURRENDER;
    else
    if(RESULT_RETREAT & army2) return RESULT_RETREAT;
    else
    if(RESULT_LOSS & army2) return RESULT_LOSS;
    else
    if(RESULT_WINS & army2) return RESULT_WINS;

    return 0;
}

u32 Battle::Result::GetExperienceAttacker(void) const
{
    return exp1;
}

u32 Battle::Result::GetExperienceDefender(void) const
{
    return exp2;
}

bool Battle::Result::AttackerWins(void) const
{
    return army1 & RESULT_WINS;
}

bool Battle::Result::DefenderWins(void) const
{
    return army2 & RESULT_WINS;
}

StreamBase & Battle::operator<< (StreamBase & msg, const Result & res)
{
    return msg << res.army1 << res.army2 << res.exp1 << res.exp2 << res.killed;
}

StreamBase & Battle::operator>> (StreamBase & msg, Result & res)
{
    return msg >> res.army1 >> res.army2 >> res.exp1 >> res.exp2 >> res.killed;
}
