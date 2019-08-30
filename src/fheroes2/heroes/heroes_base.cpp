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
#include <utility>
#include <sstream>
#include "artifact.h"
#include "game.h"
#include "race.h"
#include "color.h"
#include "army.h"
#include "world.h"
#include "kingdom.h"
#include "castle.h"
#include "settings.h"
#include "heroes_base.h"

int ArtifactsModifiersResult(int type, const u8* arts, u32 size, const HeroBase & base, std::string* strs)
{
    int result = 0;

    for(u32 ii = 0; ii < size; ++ii)
    {
	    const Artifact art(arts[ii]);

    	    if(art.isValid())
    	    {
		int acount = base.HasArtifact(art);
		if(acount)
		{
		    s32 mod = art.ExtraValue();

		    switch(art())
		    {
			case Artifact::SWORD_BREAKER:		if(type == MDF_ATTACK) mod = 1; break;
			// power
			case Artifact::BROACH_SHIELDING:	if(type == MDF_POWER) mod = -2; break;
			// morale/luck
			case Artifact::BATTLE_GARB:		if(type == MDF_MORALE || type == MDF_LUCK) mod = 10; break;
			case Artifact::MASTHEAD:		if(type == MDF_MORALE || type == MDF_LUCK) mod = base.Modes(Heroes::SHIPMASTER) ? art.ExtraValue() : 0; break;
			// morale
			case Artifact::FIZBIN_MISFORTUNE:	if(type == MDF_MORALE) mod = -art.ExtraValue(); break;
			default: break;
		    }

    		    result += mod * acount;

    		    if(strs && mod)
    		    {
        		strs->append(art.GetName());
			StringAppendModifiers(*strs, mod);
        		strs->append("\n");
		    }
		}
	    }
    }

    return result;
}

int ArtifactsModifiersAttack(const HeroBase & base, std::string* strs)
{
    const u8 arts[] = {
            Artifact::SPIKED_HELM, Artifact::THUNDER_MACE, Artifact::GIANT_FLAIL,
            Artifact::SWORD_BREAKER, Artifact::SPIKED_SHIELD, Artifact::POWER_AXE,
            Artifact::LEGENDARY_SCEPTER, Artifact::DRAGON_SWORD, Artifact::ULTIMATE_CROWN,
            Artifact::BATTLE_GARB, Artifact::SWORD_ANDURAN, Artifact::HOLY_HAMMER,
            Artifact::ULTIMATE_SHIELD, Artifact::ULTIMATE_SWORD };

    return ArtifactsModifiersResult(MDF_ATTACK, arts, ARRAY_COUNT(arts), base, strs);
}

int ArtifactsModifiersDefense(const HeroBase & base, std::string* strs)
{
    const u8 arts[] = {
	    Artifact::SPIKED_HELM, Artifact::ARMORED_GAUNTLETS, Artifact::DEFENDER_HELM,
	    Artifact::SPIKED_SHIELD, Artifact::STEALTH_SHIELD, Artifact::LEGENDARY_SCEPTER,
	    Artifact::DIVINE_BREASTPLATE, Artifact::ULTIMATE_CROWN,
	    Artifact::SWORD_BREAKER, Artifact::BREASTPLATE_ANDURAN, Artifact::BATTLE_GARB,
	    Artifact::ULTIMATE_SHIELD, Artifact::ULTIMATE_CLOAK };

    return ArtifactsModifiersResult(MDF_DEFENSE, arts, ARRAY_COUNT(arts), base, strs);
}

int ArtifactsModifiersPower(const HeroBase & base, std::string* strs)
{
    const u8 arts[] = {
	    Artifact::WHITE_PEARL, Artifact::BLACK_PEARL, Artifact::CASTER_BRACELET,
	    Artifact::MAGE_RING, Artifact::LEGENDARY_SCEPTER, Artifact::WITCHES_BROACH,
	    Artifact::ARM_MARTYR, Artifact::ULTIMATE_CROWN, Artifact::ARCANE_NECKLACE,
	    Artifact::BATTLE_GARB, Artifact::STAFF_WIZARDRY, Artifact::HELMET_ANDURAN,
	    Artifact::ULTIMATE_STAFF, Artifact::ULTIMATE_WAND, Artifact::BROACH_SHIELDING };

    return ArtifactsModifiersResult(MDF_POWER, arts, ARRAY_COUNT(arts), base, strs);
}

int ArtifactsModifiersKnowledge(const HeroBase & base, std::string* strs)
{
    const u8 arts[] = {
	    Artifact::WHITE_PEARL, Artifact::BLACK_PEARL, Artifact::MINOR_SCROLL,
	    Artifact::MAJOR_SCROLL, Artifact::SUPERIOR_SCROLL, Artifact::FOREMOST_SCROLL,
	    Artifact::LEGENDARY_SCEPTER, Artifact::ULTIMATE_CROWN,
	    Artifact::ULTIMATE_STAFF, Artifact::ULTIMATE_BOOK };

    return ArtifactsModifiersResult(MDF_KNOWLEDGE, arts, ARRAY_COUNT(arts), base, strs);
}

int ArtifactsModifiersMorale(const HeroBase & base, std::string* strs)
{
    const u8 arts[] = {
	    Artifact::MEDAL_VALOR, Artifact::MEDAL_COURAGE, Artifact::MEDAL_HONOR,
	    Artifact::MEDAL_DISTINCTION, Artifact::BATTLE_GARB, Artifact::MASTHEAD,
	    Artifact::FIZBIN_MISFORTUNE };

    return ArtifactsModifiersResult(MDF_MORALE, arts, ARRAY_COUNT(arts), base, strs);
}

int ArtifactsModifiersLuck(const HeroBase & base, std::string* strs)
{
    const u8 arts[] = {
	    Artifact::RABBIT_FOOT, Artifact::GOLDEN_HORSESHOE, Artifact::GAMBLER_LUCKY_COIN,
	    Artifact::FOUR_LEAF_CLOVER, Artifact::BATTLE_GARB, Artifact::MASTHEAD };

    return ArtifactsModifiersResult(MDF_LUCK, arts, ARRAY_COUNT(arts), base, strs);
}

HeroBase::HeroBase(int type, int race)
    : magic_point(0), move_point(0), spell_book()
{
    bag_artifacts.assign(HEROESMAXARTIFACT, Artifact::UNKNOWN);
    LoadDefaults(type, race);
}

void HeroBase::LoadDefaults(int type, int race)
{
    if(Race::ALL & race)
    {
	// fixed default primary skills
	Skill::Primary::LoadDefaults(type, race);

	// fixed default spell
	switch(type)
	{
	    case HeroBase::CAPTAIN:
	    {
		// force add spell book
		SpellBookActivate();

		Spell spell = Skill::Primary::GetInitialSpell(race);
		if(spell.isValid())
		    AppendSpellToBook(spell, true);
	    }
    	    break;

	    case HeroBase::HEROES:
	    {
		Spell spell = Skill::Primary::GetInitialSpell(race);
		if(spell.isValid())
		{
		    SpellBookActivate();
		    AppendSpellToBook(spell, true);
		}
	    }
    	    break;

	    default: break;
	}
    }
}

HeroBase::HeroBase() : magic_point(0), move_point(0), spell_book()
{
}

bool HeroBase::isCaptain(void) const
{
    return GetType() == CAPTAIN;
}

bool HeroBase::isHeroes(void) const
{
    return GetType() == HEROES;
}

u32 HeroBase::GetSpellPoints(void) const
{
    return magic_point;
}

void HeroBase::SetSpellPoints(u32 points)
{
    magic_point = points;
}

bool HeroBase::HaveSpellPoints(const Spell & spell) const
{
    return magic_point >= spell.SpellPoint(this);
}

void HeroBase::EditSpellBook(void)
{
    spell_book.Edit(*this);
}

Spell HeroBase::OpenSpellBook(int filter, bool canselect) const
{
    return spell_book.Open(*this, filter, canselect);
}

bool HeroBase::HaveSpellBook(void) const
{
    return HasArtifact(Artifact::MAGIC_BOOK);
}

bool HeroBase::HaveSpell(const Spell & spell, bool skip_bag) const
{
    return HaveSpellBook() &&
	(spell_book.isPresentSpell(spell) || (!skip_bag && bag_artifacts.ContainSpell(spell)));
}

void HeroBase::AppendSpellToBook(const Spell & spell, bool without_wisdom)
{
    if(without_wisdom || CanLearnSpell(spell))
	spell_book.Append(spell);
}

void HeroBase::AppendSpellsToBook(const SpellStorage & spells, bool without_wisdom)
{
    for(SpellStorage::const_iterator
	it = spells.begin(); it != spells.end(); ++it)
	AppendSpellToBook(*it, without_wisdom);
}

bool HeroBase::SpellBookActivate(void)
{
    return ! HaveSpellBook() &&
	    bag_artifacts.PushArtifact(Artifact::MAGIC_BOOK);
}

const BagArtifacts & HeroBase::GetBagArtifacts(void) const
{
    return bag_artifacts;
}

BagArtifacts & HeroBase::GetBagArtifacts(void)
{
    return bag_artifacts;
}

u32 HeroBase::HasArtifact(const Artifact & art) const
{
    bool unique = true;

    switch(art.Type())
    {
	case 1:	unique = Settings::Get().ExtWorldUseUniqueArtifactsML(); break; /* morale/luck arts. */
	case 2:	unique = Settings::Get().ExtWorldUseUniqueArtifactsRS(); break; /* resource affecting arts. */
	case 3:	unique = Settings::Get().ExtWorldUseUniqueArtifactsPS(); break; /* primary/mp/sp arts. */
	case 4:	unique = Settings::Get().ExtWorldUseUniqueArtifactsSS(); break; /* sec. skills arts. */
	default: break;
    }

    return ! unique ? bag_artifacts.Count(art) :
        (bag_artifacts.isPresentArtifact(art) ? 1 : 0);
}

int HeroBase::GetAttackModificator(std::string* strs) const
{
    int result = ArtifactsModifiersAttack(*this, strs);

    // check castle modificator
    const Castle* castle = inCastle();

    if(castle)
	result += castle->GetAttackModificator(strs);

    return result;
}

int HeroBase::GetDefenseModificator(std::string* strs) const
{
    int result = ArtifactsModifiersDefense(*this, strs);

    // check castle modificator
    const Castle* castle = inCastle();

    if(castle)
	result += castle->GetDefenseModificator(strs);

    return result;
}

int HeroBase::GetPowerModificator(std::string* strs) const
{
    int result = ArtifactsModifiersPower(*this, strs);

    // check castle modificator
    const Castle* castle = inCastle();

    if(castle)
	result += castle->GetPowerModificator(strs);

    return result;
}

int HeroBase::GetKnowledgeModificator(std::string* strs) const
{
    int result = ArtifactsModifiersKnowledge(*this, strs);

    // check castle modificator
    const Castle* castle = inCastle();

    if(castle)
	result += castle->GetKnowledgeModificator(strs);

    return result;
}

int HeroBase::GetMoraleModificator(std::string* strs) const
{
    int result = ArtifactsModifiersMorale(*this, strs);

    // check castle modificator
    const Castle* castle = inCastle();

    if(castle)
	result += castle->GetMoraleModificator(strs);

    // army modificator
    if(GetArmy().AllTroopsIsRace(Race::NECR))
    {
	if(strs) strs->clear();
	result = 0;
    }

    result += GetArmy().GetMoraleModificator(strs);

    return result;
}

int HeroBase::GetLuckModificator(std::string* strs) const
{
    int result = ArtifactsModifiersLuck(*this, strs);

    // check castle modificator
    const Castle* castle = inCastle();

    if(castle)
	result += castle->GetLuckModificator(strs);

    // army modificator
    result += GetArmy().GetLuckModificator(strs);

    return result;
}

bool HeroBase::CanCastSpell(const Spell & spell, std::string* res) const
{
    const Kingdom & kingdom = world.GetKingdom(GetColor());

    if(res)
    {
	std::ostringstream os;

	if(HaveSpellBook())
	{
	    if(HaveSpell(spell))
	    {
		if(HaveSpellPoints(spell))
		{
		    if(spell.MovePoint() <= move_point)
		    {
			if(kingdom.AllowPayment(spell.GetCost()))
			    return true;
			else
	    		    os << "resource" << " " << "failed";
		    }
		    else
	    		os << "move points" << " " << "failed";
		}
		else
	    	    os << _("That spell costs %{mana} mana. You only have %{point} mana, so you can't cast the spell.");
	    }
	    else
	    	os << "spell" << " " << "not found";
	}
	else
	    os << "spell book" << " " << "not found";
	*res = os.str();
	return false;
    }

    return HaveSpellBook() && HaveSpell(spell) && HaveSpellPoints(spell) && kingdom.AllowPayment(spell.GetCost());
}

void HeroBase::SpellCasted(const Spell & spell)
{
    // resource cost
    Kingdom & kingdom = world.GetKingdom(GetColor());
    const payment_t & cost = spell.GetCost();
    if(cost.GetValidItemsCount()) kingdom.OddFundsResource(cost);

    // spell point cost
    magic_point -= (spell.SpellPoint(this) < magic_point ? spell.SpellPoint(this) : magic_point);

    // move point cost
    if(spell.MovePoint())
	move_point -= (spell.MovePoint() < move_point ? spell.MovePoint() : move_point);
}

bool HeroBase::CanTranscribeScroll(const Artifact & art) const
{
    Spell spell = art.GetSpell();

    if(spell.isValid() && CanCastSpell(spell))
    {
	int learning = GetLevelSkill(Skill::Secondary::LEARNING);

	return ((3 <  spell.Level() && Skill::Level::EXPERT == learning) ||
	    (3 == spell.Level() && Skill::Level::ADVANCED <= learning) ||
	    (3 > spell.Level() && Skill::Level::BASIC <= learning));
    }

    return false;
}

bool HeroBase::CanTeachSpell(const Spell & spell) const
{
    int learning = GetLevelSkill(Skill::Secondary::LEARNING);

    return ((4 == spell.Level() && Skill::Level::EXPERT == learning) ||
	    (3 == spell.Level() && Skill::Level::ADVANCED <= learning) ||
	    (3 > spell.Level() && Skill::Level::BASIC <= learning));
}

bool HeroBase::CanLearnSpell(const Spell & spell) const
{
    int wisdom = GetLevelSkill(Skill::Secondary::WISDOM);

    return ((4 < spell.Level() && Skill::Level::EXPERT == wisdom) ||
            (4 == spell.Level() && Skill::Level::ADVANCED <= wisdom) ||
            (3 == spell.Level() && Skill::Level::BASIC <= wisdom) || 3 > spell.Level());
}

void HeroBase::TranscribeScroll(const Artifact & art)
{
    Spell spell = art.GetSpell();

    if(spell.isValid())
    {
	// add spell
	spell_book.Append(spell);

	// remove art
	bag_artifacts.RemoveScroll(art);

	// reduce mp and resource
	SpellCasted(spell);
    }
}

/* pack hero base */
StreamBase & operator<< (StreamBase & msg, const HeroBase & hero)
{
    return
	msg << 
	static_cast<const Skill::Primary &>(hero) <<
	static_cast<const MapPosition &>(hero) <<
	// modes
	hero.modes <<
	// hero base
	hero.magic_point << hero.move_point <<
	hero.spell_book << hero.bag_artifacts;
}

/* unpack hero base */
StreamBase & operator>> (StreamBase & msg, HeroBase & hero)
{
    msg >>
	static_cast<Skill::Primary &>(hero) >>
	static_cast<MapPosition &>(hero) >>
	// modes
	hero.modes >>
	hero.magic_point >> hero.move_point >>
        hero.spell_book >> hero.bag_artifacts;

    if(FORMAT_VERSION_3269 > Game::GetLoadVersion())
    {
	if(hero.bag_artifacts.size() < HEROESMAXARTIFACT)
	    hero.bag_artifacts.resize(HEROESMAXARTIFACT, Artifact::UNKNOWN);
    }

    return msg;
}
