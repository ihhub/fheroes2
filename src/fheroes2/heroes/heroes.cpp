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

#include <cmath>
#include <algorithm>
#include <functional>
#include "artifact.h"
#include "world.h"
#include "castle.h"
#include "settings.h"
#include "mp2.h"
#include "agg.h"
#include "speed.h"
#include "luck.h"
#include "race.h"
#include "direction.h"
#include "ground.h"
#include "morale.h"
#include "monster.h"
#include "payment.h"
#include "profit.h"
#include "cursor.h"
#include "kingdom.h"
#include "visit.h"
#include "battle.h"
#include "heroes.h"
#include "game.h"
#include "game_interface.h"
#include "game_static.h"
#include "ai.h"

const char* Heroes::GetName(int id)
{
    const char* names[] = {
	// knight
	_("Lord Kilburn"), _("Sir Gallanth"), _("Ector"), _("Gwenneth"), _("Tyro"), _("Ambrose"), _("Ruby"), _("Maximus"), _("Dimitry"),
	// barbarian
	_("Thundax"), _("Fineous"), _("Jojosh"), _("Crag Hack"), _("Jezebel"), _("Jaclyn"), _("Ergon"), _("Tsabu"), _("Atlas"),
	// sorceress
	_("Astra"), _("Natasha"), _("Troyan"), _("Vatawna"), _("Rebecca"), _("Gem"), _("Ariel"), _("Carlawn"), _("Luna"),
	// warlock
	_("Arie"), _("Alamar"), _("Vesper"), _("Crodo"), _("Barok"), _("Kastore"), _("Agar"), _("Falagar"), _("Wrathmont"),
	// wizard
	_("Myra"), _("Flint"), _("Dawn"), _("Halon"), _("Myrini"), _("Wilfrey"), _("Sarakin"), _("Kalindra"), _("Mandigal"),
	// necromant
	_("Zom"), _("Darlana"), _("Zam"), _("Ranloo"), _("Charity"), _("Rialdo"), _("Roxana"), _("Sandro"), _("Celia"),
	// campains
	_("Roland"), _("Lord Corlagon"), _("Sister Eliza"), _("Archibald"), _("Lord Halton"), _("Brother Bax"),
	// loyalty version
	_("Solmyr"), _("Dainwin"), _("Mog"), _("Uncle Ivan"), _("Joseph"), _("Gallavant"), _("Elderian"), _("Ceallach"), _("Drakonia"), _("Martine"), _("Jarkonas"),
	// debug
	"SandySandy", "Unknown" };

    return names[id];
}

int ObjectVisitedModifiersResult(int type, const u8* objs, u32 size, const Heroes & hero, std::string* strs)
{
    int result = 0;

    for(u32 ii = 0; ii < size; ++ii)
    {
    	if(hero.isVisited(objs[ii]))
    	{
    	    result += GameStatic::ObjectVisitedModifiers(objs[ii]);

    	    if(strs)
    	    {
		strs->append(MP2::StringObject(objs[ii]));
		StringAppendModifiers(*strs, GameStatic::ObjectVisitedModifiers(objs[ii]));
		strs->append("\n");
	    }
    	}
    }

    return result;
}

Heroes::Heroes() : move_point_scale(-1), army(this), hid(UNKNOWN), portrait(UNKNOWN), race(UNKNOWN), save_maps_object(0), path(*this),
    direction(Direction::RIGHT), sprite_index(18), patrol_square(0)
{
}

Heroes::Heroes(int heroid, int rc) : HeroBase(HeroBase::HEROES, rc), ColorBase(Color::NONE), experience(0),
    move_point_scale(-1), secondary_skills(rc), army(this), hid(heroid), portrait(heroid), race(rc),
    save_maps_object(MP2::OBJ_ZERO), path(*this), direction(Direction::RIGHT), sprite_index(18), patrol_square(0)
{
    name = _(Heroes::GetName(heroid));

    // set default army
    army.Reset(true);

    // extra hero
    switch(hid)
    {
        case ROLAND:
            attack    = 0;
            defense   = 1;
            power     = 4;
            knowledge = 5;

	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::WISDOM, Skill::Level::ADVANCED));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::LEADERSHIP, Skill::Level::EXPERT));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::ARCHERY, Skill::Level::BASIC));
    	    break;

        case CORLAGON:
            attack    = 5;
            defense   = 3;
            power     = 1;
            knowledge = 1;

	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::NECROMANCY, Skill::Level::EXPERT));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::BALLISTICS, Skill::Level::BASIC));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::WISDOM, Skill::Level::BASIC));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::LEADERSHIP, Skill::Level::BASIC));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::PATHFINDING, Skill::Level::BASIC));
    	    break;

        case ELIZA:
            attack    = 0;
            defense   = 1;
            power     = 2;
            knowledge = 6;

	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::NAVIGATION, Skill::Level::ADVANCED));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::WISDOM, Skill::Level::EXPERT));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::ARCHERY, Skill::Level::BASIC));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::LUCK, Skill::Level::BASIC));
    	    break;

    	case ARCHIBALD:
            attack    = 1;
            defense   = 1;
            power     = 4;
            knowledge = 4;

	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::SCOUTING, Skill::Level::EXPERT));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::LEADERSHIP, Skill::Level::EXPERT));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::WISDOM, Skill::Level::ADVANCED));
    	    break;

    	case HALTON:
            attack    = 3;
            defense   = 3;
            power     = 3;
            knowledge = 2;

	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::BALLISTICS, Skill::Level::BASIC));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::LEADERSHIP, Skill::Level::ADVANCED));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::DIPLOMACY, Skill::Level::BASIC));
    	    break;

    	case BAX:
            attack    = 1;
            defense   = 1;
            power     = 4;
            knowledge = 3;

	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::WISDOM, Skill::Level::EXPERT));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::NECROMANCY, Skill::Level::BASIC));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::NAVIGATION, Skill::Level::BASIC));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::PATHFINDING, Skill::Level::BASIC));
    	    break;

	case SOLMYR:
	case DRAKONIA:
	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::WISDOM, Skill::Level::ADVANCED));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::LEADERSHIP, Skill::Level::BASIC));
	    break;

	case DAINWIN:
	case ELDERIAN:
	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::WISDOM, Skill::Level::ADVANCED));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::SCOUTING, Skill::Level::BASIC));
	    break;

	case MOG:
	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::WISDOM, Skill::Level::BASIC));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::NECROMANCY, Skill::Level::ADVANCED));
	    break;

	case UNCLEIVAN:
	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::PATHFINDING, Skill::Level::ADVANCED));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::LEADERSHIP, Skill::Level::BASIC));
	    break;

	case JOSEPH:
	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::LEADERSHIP, Skill::Level::BASIC));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::SCOUTING, Skill::Level::BASIC));
	    break;

	case GALLAVANT:
	    break;

	case CEALLACH:
	    break;

	case MARTINE:
	    break;

	case JARKONAS:
	    break;

    	case SANDYSANDY:
	    army.Clean();
	    army.JoinTroop(Monster::BLACK_DRAGON, 2);
	    army.JoinTroop(Monster::RED_DRAGON, 3);

	    secondary_skills = Skill::SecSkills();
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::PATHFINDING, Skill::Level::ADVANCED));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::LOGISTICS, Skill::Level::ADVANCED));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::SCOUTING, Skill::Level::BASIC));
	    secondary_skills.AddSkill(Skill::Secondary(Skill::Secondary::MYSTICISM, Skill::Level::BASIC));

	    PickupArtifact(Artifact::STEALTH_SHIELD);
	    PickupArtifact(Artifact::DRAGON_SWORD);
	    PickupArtifact(Artifact::NOMAD_BOOTS_MOBILITY);
	    PickupArtifact(Artifact::TRAVELER_BOOTS_MOBILITY);
	    PickupArtifact(Artifact::TRUE_COMPASS_MOBILITY);

	    experience = 777;
	    magic_point = 120;

	    // all spell in magic book
	    for(u32 spell = Spell::FIREBALL; spell < Spell::STONE; ++spell) AppendSpellToBook(Spell(spell), true);
	    break;

	default: break;
    }

    if(! magic_point)
	SetSpellPoints(GetMaxSpellPoints());
    move_point = GetMaxMovePoints();
}

void Heroes::LoadFromMP2(s32 map_index, int cl, int rc, StreamBuf st)
{
    // reset modes
    modes = 0;

    SetIndex(map_index);
    SetColor(cl);

    // unknown
    st.skip(1);

    // custom troops
    if(st.get())
    {
        Troop troops[5];

        // set monster id
        for(u32 ii = 0; ii < ARRAY_COUNT(troops); ++ii)
            troops[ii].SetMonster(st.get() + 1);

        // set count
        for(u32 ii = 0; ii < ARRAY_COUNT(troops); ++ii)
            troops[ii].SetCount(st.getLE16());

        army.Assign(troops, ARRAY_COUNT_END(troops));
    }
    else
	st.skip(15);

    // custom portrate
    bool custom_portrait = st.get();

    if(custom_portrait)
    {
	SetModes(NOTDEFAULTS);

	// index sprite portrait
	portrait = st.get();

	if(UNKNOWN <= portrait)
	{
	    DEBUG(DBG_GAME, DBG_WARN, "custom portrait incorrect: " << portrait);
	    portrait = hid;
	}

	// fixed race for custom portrait (after level up)
	if(race != rc)
	    race = rc;
    }
    else
	st.skip(1);

    // 3 artifacts
    PickupArtifact(Artifact(st.get()));
    PickupArtifact(Artifact(st.get()));
    PickupArtifact(Artifact(st.get()));

    // unknown byte
    st.skip(1);

    // experience
    experience = st.getLE32();

    bool custom_secskill = st.get();

    // custom skill
    if(custom_secskill)
    {
	SetModes(NOTDEFAULTS);
	SetModes(CUSTOMSKILLS);
	std::vector<Skill::Secondary> secs(8);

	for(std::vector<Skill::Secondary>::iterator
	    it = secs.begin(); it != secs.end(); ++it)
	    (*it).SetSkill(st.get() + 1);

	for(std::vector<Skill::Secondary>::iterator
	    it = secs.begin(); it != secs.end(); ++it)
	    (*it).SetLevel(st.get());

        secondary_skills = Skill::SecSkills();

	for(std::vector<Skill::Secondary>::const_iterator
	    it = secs.begin(); it != secs.end(); ++it)
	    if((*it).isValid()) secondary_skills.AddSkill(*it);
    }
    else
	st.skip(16);

    // unknown
    st.skip(1);

    // custom name
    if(st.get())
    {
	SetModes(NOTDEFAULTS);
        name = Game::GetEncodeString(st.toString(13));
    }
    else
	st.skip(13);

    // patrol
    if(st.get())
    {
	SetModes(PATROL);
	patrol_center = GetCenter();
    }

    // count square
    patrol_square = st.get();

    PostLoad();
}

void Heroes::PostLoad(void)
{
    killer_color.SetColor(Color::NONE);
    
    // save general object
    save_maps_object = MP2::OBJ_ZERO;

    // fix zero army
    if(!army.isValid())
	army.Reset(true);
    else
	SetModes(CUSTOMARMY);

    // level up
    int level = GetLevel();
    while(1 < level--)
    {
	SetModes(NOTDEFAULTS);
	LevelUp(Modes(CUSTOMSKILLS), true);
    }

    if((race & (Race::SORC | Race::WRLK | Race::WZRD | Race::NECR)) &&
	! HaveSpellBook())
    {
	Spell spell = Skill::Primary::GetInitialSpell(race);
        if(spell.isValid())
        {
            SpellBookActivate();
            AppendSpellToBook(spell, true);
        }
    }

    // other param
    SetSpellPoints(GetMaxSpellPoints());
    move_point = GetMaxMovePoints();

    if(isControlAI())
    {
	AI::HeroesPostLoad(*this);
    }

    DEBUG(DBG_GAME , DBG_INFO, name << ", color: " << Color::String(GetColor()) << ", race: " << Race::String(race));
}

int Heroes::GetID(void) const
{
    return hid;
}

int Heroes::GetRace(void) const
{
    return race;
}

const std::string & Heroes::GetName(void) const
{
    return name;
}

int Heroes::GetColor(void) const
{
    return ColorBase::GetColor();
}

int Heroes::GetType(void) const
{
    return HeroBase::HEROES;
}

const Army & Heroes::GetArmy(void) const
{
    return army;
}

Army & Heroes::GetArmy(void)
{
    return army;
}

int Heroes::GetMobilityIndexSprite(void) const
{
    // valid range (0 - 25)
    int index = !CanMove() ? 0 : move_point / 100;
    return 25 >= index ? index : 25;
}

int Heroes::GetManaIndexSprite(void) const
{
    // valid range (0 - 25)
    int r = GetSpellPoints() / 5;
    return 25 >= r ? r : 25;
}

int Heroes::GetAttack(void) const
{
    return GetAttack(NULL);
}

int Heroes::GetAttack(std::string* strs) const
{
    int result = attack + GetAttackModificator(strs);
    return result < 0 ? 0 : (result > 255 ? 255 : result);
}

int Heroes::GetDefense(void) const
{
    return GetDefense(NULL);
}

int Heroes::GetDefense(std::string* strs) const
{
    int result = defense + GetDefenseModificator(strs);
    return result < 0 ? 0 : (result > 255 ? 255 : result);
}

int Heroes::GetPower(void) const
{
    return GetPower(NULL);
}

int Heroes::GetPower(std::string* strs) const
{
    int result = power + GetPowerModificator(strs);
    return result < 0 ? 0 : (result > 255 ? 255 : result);
}

int Heroes::GetKnowledge(void) const
{
    return GetKnowledge(NULL);
}

int Heroes::GetKnowledge(std::string* strs) const
{
    int result = knowledge + GetKnowledgeModificator(strs);
    return result < 0 ? 0 : (result > 255 ? 255 : result);
}

void Heroes::IncreasePrimarySkill(int skill)
{
    switch(skill)
    {
	case Skill::Primary::ATTACK:	++attack; break;
	case Skill::Primary::DEFENSE:	++defense; break;
	case Skill::Primary::POWER:	++power; break;
	case Skill::Primary::KNOWLEDGE:	++knowledge; break;
	default: break;
    }
}

u32 Heroes::GetExperience(void) const
{
    return experience;
}

void Heroes::IncreaseMovePoints(u32 point)
{
    move_point += point;
}

u32 Heroes::GetMovePoints(void) const
{
    return move_point;
}

u32 Heroes::GetMaxSpellPoints(void) const
{
    return 10 * GetKnowledge();
}

u32 Heroes::GetMaxMovePoints(void) const
{
    int point = 0;
    int acount = 0;

    // start point
    if(isShipMaster())
    {
	point = 1500;

	// skill navigation
	point += point * GetSecondaryValues(Skill::Secondary::NAVIGATION) / 100;

	// artifact bonus
        acount = HasArtifact(Artifact::SAILORS_ASTROLABE_MOBILITY);
        if(acount) point += acount * 1000;

        // visited object
        if(isVisited(MP2::OBJ_LIGHTHOUSE)) point += 500;
    }
    else
    {
	Troop* troop = const_cast<Army &>(army).GetSlowestTroop();

	if(troop)
    	switch(troop->GetSpeed())
	{
	    default: break;
	    case Speed::CRAWLING:
	    case Speed::VERYSLOW:	point = 1000; break;
	    case Speed::SLOW:		point = 1100; break;
	    case Speed::AVERAGE:	point = 1200; break;
	    case Speed::FAST:		point = 1300; break;
	    case Speed::VERYFAST:	point = 1400; break;
	    case Speed::ULTRAFAST:
	    case Speed::BLAZING:
	    case Speed::INSTANT:	point = 1500; break;
	}

	// skill logistics
	point += point * GetSecondaryValues(Skill::Secondary::LOGISTICS) / 100;

	// artifact bonus
	acount = HasArtifact(Artifact::NOMAD_BOOTS_MOBILITY);
	if(acount) point += acount * 600;

	acount = HasArtifact(Artifact::TRAVELER_BOOTS_MOBILITY);
	if(acount) point += acount * 300;

        // visited object
        if(isVisited(MP2::OBJ_STABLES)) point += 500;
    }

    acount = HasArtifact(Artifact::TRUE_COMPASS_MOBILITY);
    if(acount) point += acount * 500;

    return point;
}

int Heroes::GetMorale(void) const
{
    return GetMoraleWithModificators(NULL);
}

int Heroes::GetMoraleWithModificators(std::string *strs) const
{
    int result = Morale::NORMAL;

    // bonus artifact
    result += GetMoraleModificator(strs);

    if(army.AllTroopsIsRace(Race::NECR)) return Morale::NORMAL;

    // bonus leadership
    result += Skill::GetLeadershipModifiers(GetLevelSkill(Skill::Secondary::LEADERSHIP), strs);

    // object visited
    const u8 objs [] = { MP2::OBJ_BUOY, MP2::OBJ_OASIS, MP2::OBJ_WATERINGHOLE, MP2::OBJ_TEMPLE, MP2::OBJ_GRAVEYARD, MP2::OBJ_DERELICTSHIP, MP2::OBJ_SHIPWRECK };
    result += ObjectVisitedModifiersResult(MDF_MORALE, objs, ARRAY_COUNT(objs), *this, strs);

    // result
    if(result < Morale::AWFUL)	return Morale::TREASON;
    else
    if(result < Morale::POOR)	return Morale::AWFUL;
    else
    if(result < Morale::NORMAL)	return Morale::POOR;
    else
    if(result < Morale::GOOD)	return Morale::NORMAL;
    else
    if(result < Morale::GREAT)	return Morale::GOOD;
    else
    if(result < Morale::BLOOD)	return Morale::GREAT;

    return Morale::BLOOD;
}

int Heroes::GetLuck(void) const
{
    return GetLuckWithModificators(NULL);
}

int Heroes::GetLuckWithModificators(std::string *strs) const
{
    int result = Luck::NORMAL;

    // bonus artifact
    result += GetLuckModificator(strs);

    // bonus luck
    result += Skill::GetLuckModifiers(GetLevelSkill(Skill::Secondary::LUCK), strs);

    // object visited
    const u8 objs [] = { MP2::OBJ_MERMAID, MP2::OBJ_FAERIERING, MP2::OBJ_FOUNTAIN, MP2::OBJ_IDOL, MP2::OBJ_PYRAMID };
    result += ObjectVisitedModifiersResult(MDF_LUCK, objs, ARRAY_COUNT(objs), *this, strs);

    if(result < Luck::AWFUL)	return Luck::CURSED;
    else
    if(result < Luck::BAD)	return Luck::AWFUL;
    else
    if(result < Luck::NORMAL)	return Luck::BAD;
    else
    if(result < Luck::GOOD)	return Luck::NORMAL;
    else
    if(result < Luck::GREAT)	return Luck::GOOD;
    else
    if(result < Luck::IRISH)	return Luck::GREAT;

    return Luck::IRISH;
}

/* recrut hero */
bool Heroes::Recruit(int cl, const Point & pt)
{
    if(GetColor() != Color::NONE)
    {
	DEBUG(DBG_GAME, DBG_WARN, "not freeman");
	return false;
    }

    Kingdom & kingdom = world.GetKingdom(cl);

    if(kingdom.AllowRecruitHero(false, 0))
    {
	Maps::Tiles & tiles = world.GetTiles(pt.x, pt.y);
	SetColor(cl);
	killer_color.SetColor(Color::NONE);
	SetCenter(pt);
	if(!Modes(SAVEPOINTS)) move_point = GetMaxMovePoints();
	MovePointsScaleFixed();

	if(!army.isValid()) army.Reset(false);

	tiles.SetHeroes(this);
	kingdom.AddHeroes(this);

	return true;
    }

    return false;
}

bool Heroes::Recruit(const Castle & castle)
{
    if(Recruit(castle.GetColor(), castle.GetCenter()))
    {
	if(castle.GetLevelMageGuild())
	{
	    // magic point
	    if(!Modes(SAVEPOINTS)) SetSpellPoints(GetMaxSpellPoints());
	    // learn spell
	    castle.MageGuildEducateHero(*this);
	}
	return true;
    }

    return false;
}

void Heroes::ActionNewDay(void)
{
    // recovery move points
    move_point = GetMaxMovePoints();
    MovePointsScaleFixed();

    // stables visited?
    if(isVisited(MP2::OBJ_STABLES)) move_point += 400;

    // recovery spell points
    //if(HaveSpellBook())
    {
	u32 curr = GetSpellPoints();
	u32 maxp = GetMaxSpellPoints();
	const Castle* castle = inCastle();

	// possible visit arteian spring 2 * max
	if(curr < maxp)
	{
	    // in castle?
	    if(castle && castle->GetLevelMageGuild())
	    {
		//restore from mage guild
		if(Settings::Get().ExtCastleGuildRestorePointsTurn())
		    curr += maxp * GameStatic::GetMageGuildRestoreSpellPointsPercentDay(castle->GetLevelMageGuild()) / 100;
		else
		    curr = maxp;
	    }

	    // everyday
	    curr += GameStatic::GetHeroesRestoreSpellPointsPerDay();

	    // power ring action
	    int acount = HasArtifact(Artifact::POWER_RING);
	    if(acount)
		curr += acount * Artifact(Artifact::POWER_RING).ExtraValue();

	    // secondary skill
	    curr += GetSecondaryValues(Skill::Secondary::MYSTICISM);

	    SetSpellPoints(curr > maxp ? maxp : curr);
	}
    }

    // remove day visit object
    visit_object.remove_if(Visit::isDayLife);


    // new day, new capacities
    ResetModes(SAVEPOINTS);
}

void Heroes::ActionNewWeek(void)
{
    // remove week visit object
    visit_object.remove_if(Visit::isWeekLife);

    // fix artesian spring effect
    if(GetSpellPoints() > GetMaxSpellPoints()) SetSpellPoints(GetMaxSpellPoints());
}

void Heroes::ActionNewMonth(void)
{
    // remove month visit object
    visit_object.remove_if(Visit::isMonthLife);
}

void Heroes::ActionAfterBattle(void)
{
    // remove month visit object
    visit_object.remove_if(Visit::isBattleLife);
    //
    SetModes(ACTION);
}

void Heroes::RescanPathPassable(void)
{
    if(path.isValid()) path.RescanPassable();
}

void Heroes::RescanPath(void)
{
    if(path.isValid())
    {
	const Maps::Tiles & tile = world.GetTiles(path.GetDestinationIndex());

	if(!isShipMaster() && tile.isWater() &&
	    !MP2::isNeedStayFront(tile.GetObject()))
	    path.PopBack();
    }

    if(path.isValid())
    {
	if(isControlAI())
	{
	    if(path.hasObstacle()) path.Reset();
	}
	else
	{
	    path.RescanObstacle();
	}
    }
}

/* if hero in castle */
const Castle* Heroes::inCastle(void) const
{
    const Castle* castle = Color::NONE != GetColor() ? world.GetCastle(GetCenter()) : NULL;
    return castle && castle->GetHeroes() == this ? castle : NULL;
}

Castle* Heroes::inCastle(void)
{
    Castle* castle = Color::NONE != GetColor() ? world.GetCastle(GetCenter()) : NULL;
    return castle && castle->GetHeroes() == this ? castle : NULL;
}

/* is visited cell */
bool Heroes::isVisited(const Maps::Tiles & tile, Visit::type_t type) const
{
    const s32 & index = tile.GetIndex();
    int object = tile.GetObject(false);

    if(Visit::GLOBAL == type) return GetKingdom().isVisited(index, object);

    return visit_object.end() != std::find(visit_object.begin(), visit_object.end(), IndexObject(index, object));
}

/* return true if object visited */
bool Heroes::isVisited(int object, Visit::type_t type) const
{
    if(Visit::GLOBAL == type) return GetKingdom().isVisited(object);

    return visit_object.end() != std::find_if(visit_object.begin(), visit_object.end(),
				std::bind2nd(std::mem_fun_ref(&IndexObject::isObject), object));
}

/* set visited cell */
void Heroes::SetVisited(s32 index, Visit::type_t type)
{
    const Maps::Tiles & tile = world.GetTiles(index);
    int object = tile.GetObject(false);

    if(Visit::GLOBAL == type)
	GetKingdom().SetVisited(index, object);
    else
    if(! isVisited(tile) && MP2::OBJ_ZERO != object)
	visit_object.push_front(IndexObject(index, object));
}

void Heroes::SetVisitedWideTile(s32 index, int object, Visit::type_t type)
{
    const Maps::Tiles & tile = world.GetTiles(index);
    const Maps::TilesAddon* addon = tile.FindObjectConst(object);
    int wide = 0;

    switch(object)
    {
	case MP2::OBJ_SKELETON:
	case MP2::OBJ_OASIS:
	case MP2::OBJ_STANDINGSTONES:
	case MP2::OBJ_ARTESIANSPRING:	wide = 2; break;
	case MP2::OBJ_WATERINGHOLE:	wide = 4; break;
	default: break;
    }

    if(addon && wide)
    {
	for(s32 ii = tile.GetIndex() - (wide - 1); ii <= tile.GetIndex() + (wide - 1); ++ii)
	    if(Maps::isValidAbsIndex(ii) &&
		world.GetTiles(ii).FindAddonLevel1(addon->uniq)) SetVisited(ii, type);
    }
}

int Heroes::GetSpriteIndex(void) const
{
    return sprite_index;
}

bool Heroes::isAction(void) const
{
    return Modes(ACTION);
}

void Heroes::ResetAction(void)
{
    ResetModes(ACTION);
}

u32 Heroes::GetCountArtifacts(void) const
{
    return bag_artifacts.CountArtifacts();
}

bool Heroes::HasUltimateArtifact(void) const
{
    return bag_artifacts.ContainUltimateArtifact();
}

bool Heroes::IsFullBagArtifacts(void) const
{
    return bag_artifacts.isFull();
}

bool Heroes::PickupArtifact(const Artifact & art)
{
    if(!art.isValid()) return false;

    //const Settings & conf = Settings::Get();

    if(! bag_artifacts.PushArtifact(art))
    {
	if(isControlHuman())
	{
	    art() == Artifact::MAGIC_BOOK ?
	    Dialog::Message("", _("You must purchase a spell book to use the mage guild, but you currently have no room for a spell book. Try giving one of your artifacts to another hero."), Font::BIG, Dialog::OK) :
	    Dialog::Message(art.GetName(), _("You have no room to carry another artifact!"), Font::BIG, Dialog::OK);
	}
	return false;
    }

    // check: anduran garb
    if(bag_artifacts.MakeBattleGarb())
    {
	if(isControlHuman())
	    Dialog::ArtifactInfo("", _("The three Anduran artifacts magically combine into one."), Artifact::BATTLE_GARB);
    }

    return true;
}

/* return level hero */
int Heroes::GetLevel(void) const
{
    return GetLevelFromExperience(experience);
}

const Route::Path & Heroes::GetPath(void) const
{
    return path;
}

Route::Path & Heroes::GetPath(void)
{
    return path;
}

void Heroes::ShowPath(bool f)
{
    f ? path.Show() : path.Hide();
}

void Heroes::IncreaseExperience(u32 exp)
{
    int level_old = GetLevelFromExperience(experience);
    int level_new = GetLevelFromExperience(experience + exp);

    for(int ii = 0; ii < level_new - level_old; ++ii) LevelUp(false);

    experience += exp;
}

/* calc level from exp */
int Heroes::GetLevelFromExperience(u32 exp)
{
    for(int lvl = 1; lvl < 255; ++ lvl) if(exp < GetExperienceFromLevel(lvl)) return lvl;

    return 0;
}

/* calc exp from level */
u32 Heroes::GetExperienceFromLevel(int lvl)
{
    switch(lvl)
    {
	case 0:		return 0;
	case 1:		return 1000;
	case 2:		return 2000;
	case 3:		return 3200;
	case 4:		return 4500;
	case 5:		return 6000;
	case 6:		return 7700;
	case 7:		return 9000;
	case 8: 	return 11000;
	case 9:		return 13200;
	case 10:	return 15500;
	case 11:	return 18500;
	case 12:	return 22100;
	case 13:	return 26400;
	case 14:	return 31600;
	case 15:	return 37800;
	case 16:	return 45300;
	case 17:	return 54200;
	case 18:	return 65000;
	case 19:	return 78000;
	case 20:	return 93600;
	case 21:	return 112300;
	case 22:	return 134700;
	case 23:	return 161600;
	case 24:	return 193900;
	case 25:	return 232700;
	case 26:	return 279300;
	case 27:	return 335200;
	case 28:	return 402300;
	case 29:	return 482800;
	case 30:	return 579400;
	case 31:	return 695300;
	case 32:	return 834400;
	case 33:	return 1001300;
	case 34:	return 1201600;
	case 35:	return 1442000;
	case 36:	return 1730500;
	case 37:	return 2076700;
	case 38:	return 2492100;
	case 39:	return 2990600;

	default:        break;
    }

    const u32 l1 = GetExperienceFromLevel(lvl - 1);
    return (l1 + static_cast<u32>(round((l1 - GetExperienceFromLevel(lvl - 2)) * 1.2 / 100) * 100));
}

/* buy book */
bool Heroes::BuySpellBook(const Castle* castle, int shrine)
{
    if(HaveSpellBook() || Color::NONE == GetColor()) return false;

    const payment_t payment = PaymentConditions::BuySpellBook(shrine);
    Kingdom & kingdom = GetKingdom();

    std::string header = _("To cast spells, you must first buy a spell book for %{gold} gold.");
    StringReplace(header, "%{gold}", payment.gold);

    if( ! kingdom.AllowPayment(payment))
    {
	if(isControlHuman())
	{
	    header.append(" ");
	    header.append(_("Unfortunately, you seem to be a little short of cash at the moment."));
	    Dialog::Message("", header, Font::BIG, Dialog::OK);
	}
	return false;
    }

    if(isControlHuman())
    {
	const Sprite & border = AGG::GetICN(ICN::RESOURCE, 7);
	Surface sprite = border.GetSurface();

	AGG::GetICN(ICN::ARTIFACT, Artifact(Artifact::MAGIC_BOOK).IndexSprite64()).Blit(5, 5, sprite);

	header.append(" ");
	header.append(_("Do you wish to buy one?"));

	if(Dialog::NO == Dialog::SpriteInfo(GetName(), header, sprite, Dialog::YES | Dialog::NO)) return false;
    }

    if(SpellBookActivate())
    {
	kingdom.OddFundsResource(payment);

	// add all spell to book
	if(castle) castle->MageGuildEducateHero(*this);

	return true;
    }

    return false;
}

/* return true is move enable */
bool Heroes::isEnableMove(void) const
{
    return Modes(ENABLEMOVE) && path.isValid() && path.GetFrontPenalty() <= move_point;
}

bool Heroes::CanMove(void) const
{
    return move_point >= Maps::Ground::GetPenalty(GetIndex(), Direction::CENTER, GetLevelSkill(Skill::Secondary::PATHFINDING));
}

/* set enable move */
void Heroes::SetMove(bool f)
{
    if(f)
    {
	SetModes(ENABLEMOVE);
    }
    else
    {
	ResetModes(ENABLEMOVE);

	// reset sprite position
	switch(direction)
        {
            case Direction::TOP:            sprite_index = 0; break;
            case Direction::BOTTOM:         sprite_index = 36; break;
            case Direction::TOP_RIGHT:
            case Direction::TOP_LEFT:       sprite_index = 9; break;
            case Direction::BOTTOM_RIGHT:
            case Direction::BOTTOM_LEFT:    sprite_index = 27; break;
            case Direction::RIGHT:
            case Direction::LEFT:           sprite_index = 18; break;
            default: break;
	}
    }
}

bool Heroes::isShipMaster(void) const
{
    return Modes(SHIPMASTER);
}

void Heroes::SetShipMaster(bool f)
{
    f ? SetModes(SHIPMASTER) : ResetModes(SHIPMASTER);
}

Skill::SecSkills & Heroes::GetSecondarySkills(void)
{
    return secondary_skills;
}

bool Heroes::HasSecondarySkill(int skill) const
{
    return Skill::Level::NONE != secondary_skills.GetLevel(skill);
}

u32 Heroes::GetSecondaryValues(int skill) const
{
    return secondary_skills.GetValues(skill);
}

bool Heroes::HasMaxSecondarySkill(void) const
{
    return HEROESMAXSKILL <= secondary_skills.Count();
}

int Heroes::GetLevelSkill(int skill) const
{
    return secondary_skills.GetLevel(skill);
}

void Heroes::LearnSkill(const Skill::Secondary & skill)
{
    if(skill.isValid())
	secondary_skills.AddSkill(skill);
}

void Heroes::Scoute(void) const
{
    Maps::ClearFog(GetIndex(), GetScoute(), GetColor());
}

int Heroes::GetScoute(void) const
{
    int acount = HasArtifact(Artifact::TELESCOPE);

    return (acount ? acount * Game::GetViewDistance(Game::VIEW_TELESCOPE) : 0) +
	Game::GetViewDistance(Game::VIEW_HEROES) + GetSecondaryValues(Skill::Secondary::SCOUTING);
}

u32 Heroes::GetVisionsDistance(void) const
{
    int dist = Spell(Spell::VISIONS).ExtraValue();
    int acount = HasArtifact(Artifact::CRYSTAL_BALL);

    if(acount)
        dist = acount * (Settings::Get().UseAltResource() ? dist * 2 + 2 : 8);

    return dist;
}

int Heroes::GetDirection(void) const
{
    return direction;
}

/* return route range in days */
int Heroes::GetRangeRouteDays(s32 dst) const
{
    const u32 max = GetMaxMovePoints();
    const u32 limit = max * 5 / 100; // limit ~5 day

    // approximate distance, this restriction calculation
    if((4 * max / 100) < Maps::GetApproximateDistance(GetIndex(), dst))
    {
	DEBUG(DBG_GAME, DBG_INFO, "distance limit");
	return 0;
    }

    Route::Path test(*this);
    // approximate limit, this restriction path finding algorithm
    if(test.Calculate(dst, limit))
    {
	u32 total = test.GetTotalPenalty();
	if(move_point >= total) return 1;

	total -= move_point;
	if(max >= total) return 2;

	total -= move_point;
	if(max >= total) return 3;

	return 4;
    }
    else
    DEBUG(DBG_GAME, DBG_INFO, "iteration limit: " << limit);

    return 0;
}

/* up level */
void Heroes::LevelUp(bool skipsecondary, bool autoselect)
{
    int primary = LevelUpPrimarySkill();
    if(! skipsecondary)
	LevelUpSecondarySkill(primary, (autoselect || isControlAI()));
    if(isControlAI()) AI::HeroesLevelUp(*this);
}

int Heroes::LevelUpPrimarySkill(void)
{
    int skill = Skill::Primary::LevelUp(race, GetLevel());

    DEBUG(DBG_GAME, DBG_INFO, "for " << GetName() << ", up " << Skill::Primary::String(skill));
    return skill;
}

void Heroes::LevelUpSecondarySkill(int primary, bool autoselect)
{
    Skill::Secondary sec1;
    Skill::Secondary sec2;

    secondary_skills.FindSkillsForLevelUp(race, sec1, sec2);
    DEBUG(DBG_GAME, DBG_INFO, GetName() << " select " << Skill::Secondary::String(sec1.Skill()) <<
						    " or " << Skill::Secondary::String(sec2.Skill()));
    Skill::Secondary* selected = NULL;

    if(autoselect)
    {
	if(Skill::Secondary::UNKNOWN == sec1.Skill() || Skill::Secondary::UNKNOWN == sec2.Skill())
	{
	    if(Skill::Secondary::UNKNOWN != sec1.Skill())
		selected = &sec1;
	    else
	    if(Skill::Secondary::UNKNOWN != sec2.Skill())
		selected = &sec2;
	}
	else
	if(Skill::Secondary::UNKNOWN != sec1.Skill() && Skill::Secondary::UNKNOWN != sec2.Skill())
    	    selected = (Rand::Get(0, 1) ? &sec1 : &sec2);
    }
    else
    {
	AGG::PlaySound(M82::NWHEROLV);
	int result = Dialog::LevelUpSelectSkill(name, Skill::Primary::String(primary), sec1, sec2, *this);

	if(Skill::Secondary::UNKNOWN != result)
	    selected = result == sec2.Skill() ? & sec2 : &sec1;
    }

    // level up sec. skill
    if(selected)
    {
	DEBUG(DBG_GAME, DBG_INFO, GetName() << ", selected: " << Skill::Secondary::String(selected->Skill()));
	std::vector<Skill::Secondary>::iterator it;

	Skill::Secondary* secs = secondary_skills.FindSkill(selected->Skill());

	if(secs)
	    secs->NextLevel();
	else
	    secondary_skills.AddSkill(Skill::Secondary(selected->Skill(), Skill::Level::BASIC));

	// post action
	switch(selected->Skill())
	{
	    case Skill::Secondary::SCOUTING:
		Scoute();
		break;

	    default: break;
        }
    }
}

/* apply penalty */
bool Heroes::ApplyPenaltyMovement(void)
{
    u32 penalty = path.isValid() ?
	    path.GetFrontPenalty() :
	    Maps::Ground::GetPenalty(GetIndex(), Direction::CENTER, GetLevelSkill(Skill::Secondary::PATHFINDING));

    if(move_point >= penalty) move_point -= penalty;
    else return false;

    return true;
}

void Heroes::ResetMovePoints(void)
{
    move_point = 0;
}

bool Heroes::MayStillMove(void) const
{
    if(Modes(SLEEPER|GUARDIAN) || isFreeman()) return false;
    return path.isValid() ? (move_point >= path.GetFrontPenalty()) : CanMove();
}

bool Heroes::isValid(void) const
{
    return hid != UNKNOWN;
}

bool Heroes::isFreeman(void) const
{
    return isValid() && Color::NONE == GetColor() && !Modes(JAIL);
}

void Heroes::SetFreeman(int reason)
{
    if(! isFreeman())
    {
        bool savepoints = false;
	Kingdom & kingdom = GetKingdom();

	if((Battle::RESULT_RETREAT | Battle::RESULT_SURRENDER) & reason)
	{
	    if(Settings::Get().ExtHeroRememberPointsForRetreating()) savepoints = true;
	    kingdom.SetLastLostHero(*this);
	}

	if(!army.isValid() || (Battle::RESULT_RETREAT & reason)) army.Reset(false);
	else
	if((Battle::RESULT_LOSS & reason) && !(Battle::RESULT_SURRENDER & reason)) army.Reset(true);

	if(GetColor() != Color::NONE) kingdom.RemoveHeroes(this);

	SetColor(Color::NONE);
	world.GetTiles(GetIndex()).SetHeroes(NULL);
	modes = 0;
	SetIndex(-1);
	move_point_scale = -1;
	path.Reset();
	SetMove(false);
	SetModes(ACTION);
	if(savepoints) SetModes(SAVEPOINTS);
    }
}

void Heroes::SetKillerColor(int col)
{
    killer_color.SetColor(col);
}

int Heroes::GetKillerColor(void) const
{
    return killer_color.GetColor();
}

int Heroes::GetControl(void) const
{
    return GetKingdom().GetControl();
}

int Heroes::GetMapsObject(void) const
{
    return save_maps_object;
}

void Heroes::SetMapsObject(int obj)
{
    save_maps_object = obj != MP2::OBJ_HEROES ? obj : MP2::OBJ_ZERO;
}

bool Heroes::AllowBattle(bool attacker) const
{
    if(!attacker)
    switch(world.GetTiles(GetIndex()).GetObject(false))
    {
	case MP2::OBJ_TEMPLE: return false;
	default: break;
    }

    return true;
}

void Heroes::ActionPreBattle(void)
{
}

void RedrawGameAreaAndHeroAttackMonster(Heroes & hero, s32 dst)
{
    // redraw gamearea for monster action sprite
    if(hero.isControlHuman())
    {
	Interface::Basic & I = Interface::Basic::Get();
        Cursor::Get().Hide();
	I.GetGameArea().SetCenter(hero.GetCenter());
	I.RedrawFocus();
	I.Redraw();
        Cursor::Get().Show();
	// force flip, for monster attack show sprite
	Display::Get().Flip();
    }
    hero.Action(dst);
}

void Heroes::ActionNewPosition(void)
{
    const Settings & conf = Settings::Get();
    // check around monster
    MapsIndexes targets = Maps::GetTilesUnderProtection(GetIndex());

    if(targets.size())
    {
	bool skip_battle = false;
	SetMove(false);
	GetPath().Hide();

	// first target
        MapsIndexes::iterator it = std::find(targets.begin(), targets.end(),
                                                        GetPath().GetDestinedIndex());
        if(it != targets.end())
	{
	    RedrawGameAreaAndHeroAttackMonster(*this, *it);
	    targets.erase(it);
	    if(conf.ExtWorldOnlyFirstMonsterAttack()) skip_battle = true;
	}

	// other around targets
	for(MapsIndexes::const_iterator
	    it = targets.begin(); it != targets.end() && !isFreeman() && !skip_battle; ++it)
	{
	    RedrawGameAreaAndHeroAttackMonster(*this, *it);
	    if(conf.ExtWorldOnlyFirstMonsterAttack()) skip_battle = true;
	}
    }

    if(! isFreeman() &&
	GetMapsObject() == MP2::OBJ_EVENT)
    {
	const MapEvent* event = world.GetMapEvent(GetCenter());

	if(event && event->isAllow(GetColor()))
	{
	    Action(GetIndex());
	    SetMove(false);
	}
    }

    if(isControlAI())
	AI::HeroesActionNewPosition(*this);

    ResetModes(VISIONS);
}

void Heroes::SetCenterPatrol(const Point & pt)
{
    patrol_center = pt;
}

const Point & Heroes::GetCenterPatrol(void) const
{
    return patrol_center;
}

int Heroes::GetSquarePatrol(void) const
{
    return patrol_square;
}

int Heroes::CanScouteTile(s32 dst) const
{
    int scouting = GetSecondaryValues(Skill::Secondary::SCOUTING);
    bool army_info = false;

    switch(world.GetTiles(dst).GetObject())
    {
	case MP2::OBJ_MONSTER:
	case MP2::OBJ_CASTLE:
	case MP2::OBJ_HEROES:
	    army_info = true; break;

	default: break;
    }

    if(army_info)
    {
	// depends from distance
	if(Maps::GetApproximateDistance(GetIndex(), dst) <= GetVisionsDistance())
	{
	    // check crystal ball
	    return HasArtifact(Artifact::CRYSTAL_BALL) ?
			    Skill::Level::EXPERT : scouting;
	}
	else
	{
	    // check spell identify hero
	    if(GetKingdom().Modes(Kingdom::IDENTIFYHERO) &&
		MP2::OBJ_HEROES == world.GetTiles(dst).GetObject())
		return Skill::Level::EXPERT;
	}
    }
    else
    {
	if(Settings::Get().ExtWorldScouteExtended())
	{
	    //const Maps::Tiles & tile = world.GetTiles(dst);

	    u32 dist = GetSecondaryValues(Skill::Secondary::SCOUTING) ? GetScoute() : 0;
	    if(Modes(VISIONS) && dist < GetVisionsDistance()) dist = GetVisionsDistance();

	    if(dist > Maps::GetApproximateDistance(GetIndex(), dst))
		return scouting;
	}
    }

    return 0;
}

void Heroes::MovePointsScaleFixed(void)
{
    move_point_scale = move_point * 1000 / GetMaxMovePoints();
}

void Heroes::RecalculateMovePoints(void)
{
    if(0 <= move_point_scale) move_point = GetMaxMovePoints() * move_point_scale / 1000;
}

void Heroes::Move2Dest(const s32 & dst_index, bool skip_action /* false */)
{
    if(dst_index != GetIndex())
    {
	world.GetTiles(GetIndex()).SetHeroes(NULL);
	SetIndex(dst_index);
	Scoute();
	ApplyPenaltyMovement();
	world.GetTiles(dst_index).SetHeroes(this);

	if(!skip_action)
	    ActionNewPosition();
    }
}

Surface Heroes::GetPortrait(int id, int type)
{
    if(Heroes::UNKNOWN != id)
    switch(type)
    {
	case PORT_BIG:		return AGG::GetICN(ICN::PORTxxxx(id), 0);
        case PORT_MEDIUM:	return Heroes::SANDYSANDY > id ? AGG::GetICN(ICN::PORTMEDI, id + 1) : AGG::GetICN(ICN::PORTMEDI, BAX + 1);
        case PORT_SMALL:	return Heroes::SANDYSANDY > id ? AGG::GetICN(ICN::MINIPORT, id) : AGG::GetICN(ICN::MINIPORT, BAX);
	default: break;
    }

    return Surface();
}

Surface Heroes::GetPortrait(int type) const
{
    return Heroes::GetPortrait(portrait, type);
}

void Heroes::PortraitRedraw(s32 px, s32 py, int type, Surface & dstsf) const
{
    Surface port = GetPortrait(portrait, type);
    Point mp;

    if(port.isValid())
    {
	if(PORT_BIG == type)
	{
	    port.Blit(px, py, dstsf);
	    mp.y = 2;
	    mp.x = port.w() - 12;
	}
	else
	if(PORT_MEDIUM == type)
	{
	    port.Blit(px, py, dstsf);
	    mp.x = port.w() - 10;
	}
	else
	if(PORT_SMALL == type)
	{
    	    const Sprite & mobility = AGG::GetICN(ICN::MOBILITY, GetMobilityIndexSprite());
    	    const Sprite & mana = AGG::GetICN(ICN::MANA, GetManaIndexSprite());

    	    const int iconsw = Interface::IconsBar::GetItemWidth();
    	    const int iconsh = Interface::IconsBar::GetItemHeight();
    	    const int barw = 7;

    	    dstsf.FillRect(Rect(px, py, iconsw, iconsh), ColorBlack);
	    const RGBA blue = RGBA(15, 30, 120);

    	    // mobility
    	    dstsf.FillRect(Rect(px, py, barw, iconsh), blue);
    	    mobility.Blit(px, py + mobility.y(), dstsf);

    	    // portrait
    	    port.Blit(px + barw + 1, py, dstsf);

    	    // mana
    	    dstsf.FillRect(Rect(px + barw + port.w() + 2, py, barw, iconsh), blue);
    	    mana.Blit(px + barw + port.w() + 2, py + mana.y(), dstsf);

	    mp.x = 35;
	}
    }

    // heroes marker
    if(Modes(Heroes::SHIPMASTER))
    {
        const Sprite & sprite = AGG::GetICN(ICN::BOAT12, 0);
	const Rect pos(px + mp.x, py + mp.y - 1, sprite.w(), sprite.h());
	dstsf.FillRect(pos, ColorBlack);
	sprite.Blit(pos.x, pos.y, dstsf);
        mp.y = sprite.h();
    }
    else
    if(Modes(Heroes::GUARDIAN))
    {
	const Sprite & sprite = AGG::GetICN(ICN::MISC6, 11);
	const Rect pos(px + mp.x + 3, py + mp.y, sprite.w(), sprite.h());
	dstsf.FillRect(pos, ColorBlack);
	sprite.Blit(pos.x, pos.y, dstsf);
        mp.y = sprite.h();
    }

    if(Modes(Heroes::SLEEPER))
    {
        const Sprite & sprite = AGG::GetICN(ICN::MISC4, 14);
	const Rect pos(px + mp.x + 3, py + mp.y - 1, sprite.w() - 4, sprite.h() - 4);
	dstsf.FillRect(pos, ColorBlack);
	sprite.Blit(pos.x - 2, pos.y - 2);
    }
}

std::string Heroes::String(void) const
{
    std::ostringstream os;

    os <<
	"name            : " << name << std::endl <<
	"race            : " << Race::String(race) << std::endl <<
	"color           : " << Color::String(GetColor()) << std::endl <<
	"experience      : " << experience << std::endl <<
	"level           : " << GetLevel() << std::endl <<
	"magic point     : " << GetSpellPoints() << std::endl <<
	"position x      : " << GetCenter().x << std::endl <<
	"position y      : " << GetCenter().y << std::endl <<
	"move point      : " << move_point << std::endl <<
	"max magic point : " << GetMaxSpellPoints() << std::endl <<
	"max move point  : " << GetMaxMovePoints() << std::endl <<
	"direction       : " << Direction::String(direction) << std::endl <<
	"index sprite    : " << sprite_index << std::endl <<
	"in castle       : " << (inCastle() ? "true" : "false") << std::endl <<
	"save object     : " << MP2::StringObject(world.GetTiles(GetIndex()).GetObject(false)) << std::endl <<
	"flags           : " << (Modes(SHIPMASTER) ? "SHIPMASTER," : "") <<
                                         (Modes(PATROL) ? "PATROL" : "") << std::endl;

    if(Modes(PATROL))
    {
	os << "patrol square   : " << patrol_square << std::endl;
    }

    if(! visit_object.empty())
    {
	os << "visit objects   : ";
	for(std::list<IndexObject>::const_iterator
	    it = visit_object.begin(); it != visit_object.end(); ++it)
		os << MP2::StringObject((*it).second) << "(" << (*it).first << "), ";
	os << std::endl;
    }

    if(isControlAI())
    {
	os <<
	    "skills          : " << secondary_skills.String() << std::endl <<
	    "artifacts       : " << bag_artifacts.String() << std::endl <<
	    "spell book      : " << (HaveSpellBook() ? spell_book.String() : "disabled") << std::endl <<
	    "army dump       : " << army.String() << std::endl;

	os << AI::HeroesString(*this);
    }

    return os.str();
}

struct InCastleAndGuardian : public std::binary_function <const Castle*, Heroes*, bool>
{
    bool operator() (const Castle* castle, Heroes* hero) const
    {
        const Point & cpt = castle->GetCenter();
        const Point & hpt = hero->GetCenter();
        return cpt.x == hpt.x && cpt.y == hpt.y + 1 && hero->Modes(Heroes::GUARDIAN);
    }
};

struct InCastleNotGuardian : public std::binary_function <const Castle*, Heroes*, bool>
{
    bool operator() (const Castle* castle, Heroes* hero) const
    {
        return castle->GetCenter() == hero->GetCenter() && !hero->Modes(Heroes::GUARDIAN);
    }
};

struct InJailMode : public std::binary_function <s32, Heroes*, bool>
{
    bool operator() (s32 index, Heroes* hero) const
    {
	return hero->Modes(Heroes::JAIL) && index == hero->GetIndex();
    }
};

AllHeroes::AllHeroes()
{
    reserve(HEROESMAXCOUNT + 2);
}

AllHeroes::~AllHeroes()
{
    AllHeroes::clear();
}

void AllHeroes::Init(void)
{
    if(size())
	AllHeroes::clear();

    const bool loyalty = Settings::Get().PriceLoyaltyVersion();

    // knight: LORDKILBURN, SIRGALLANTH, ECTOR, GVENNETH, TYRO, AMBROSE, RUBY, MAXIMUS, DIMITRY
    for(u32 hid = Heroes::LORDKILBURN; hid <= Heroes::DIMITRY; ++hid)
        push_back(new Heroes(hid, Race::KNGT));

    // barbarian: THUNDAX, FINEOUS, JOJOSH, CRAGHACK, JEZEBEL, JACLYN, ERGON, TSABU, ATLAS
    for(u32 hid = Heroes::THUNDAX; hid <= Heroes::ATLAS; ++hid)
        push_back(new Heroes(hid, Race::BARB));

    // sorceress: ASTRA, NATASHA, TROYAN, VATAWNA, REBECCA, GEM, ARIEL, CARLAWN, LUNA
    for(u32 hid = Heroes::ASTRA; hid <= Heroes::LUNA; ++hid)
        push_back(new Heroes(hid, Race::SORC));

    // warlock: ARIE, ALAMAR, VESPER, CRODO, BAROK, KASTORE, AGAR, FALAGAR, WRATHMONT
    for(u32 hid = Heroes::ARIE; hid <= Heroes::WRATHMONT; ++hid)
        push_back(new Heroes(hid, Race::WRLK));

    // wizard: MYRA, FLINT, DAWN, HALON, MYRINI, WILFREY, SARAKIN, KALINDRA, MANDIGAL
    for(u32 hid = Heroes::MYRA; hid <= Heroes::MANDIGAL; ++hid)
        push_back(new Heroes(hid, Race::WZRD));

    // necromancer: ZOM, DARLANA, ZAM, RANLOO, CHARITY, RIALDO, ROXANA, SANDRO, CELIA
    for(u32 hid = Heroes::ZOM; hid <= Heroes::CELIA; ++hid)
        push_back(new Heroes(hid, Race::NECR));

    // from campain
    push_back(new Heroes(Heroes::ROLAND, Race::WZRD));
    push_back(new Heroes(Heroes::CORLAGON, Race::KNGT));
    push_back(new Heroes(Heroes::ELIZA, Race::SORC));
    push_back(new Heroes(Heroes::ARCHIBALD, Race::WRLK));
    push_back(new Heroes(Heroes::HALTON, Race::KNGT));
    push_back(new Heroes(Heroes::BAX, Race::NECR));

    // loyalty version
    push_back(new Heroes(loyalty ? Heroes::SOLMYR : Heroes::UNKNOWN, Race::WZRD));
    push_back(new Heroes(loyalty ? Heroes::DAINWIN : Heroes::UNKNOWN, Race::WRLK));
    push_back(new Heroes(loyalty ? Heroes::MOG : Heroes::UNKNOWN, Race::NECR));
    push_back(new Heroes(loyalty ? Heroes::UNCLEIVAN : Heroes::UNKNOWN, Race::BARB));
    push_back(new Heroes(loyalty ? Heroes::JOSEPH : Heroes::UNKNOWN, Race::KNGT));
    push_back(new Heroes(loyalty ? Heroes::GALLAVANT : Heroes::UNKNOWN, Race::KNGT));
    push_back(new Heroes(loyalty ? Heroes::ELDERIAN : Heroes::UNKNOWN, Race::WRLK));
    push_back(new Heroes(loyalty ? Heroes::CEALLACH : Heroes::UNKNOWN, Race::KNGT));
    push_back(new Heroes(loyalty ? Heroes::DRAKONIA : Heroes::UNKNOWN, Race::WZRD));
    push_back(new Heroes(loyalty ? Heroes::MARTINE : Heroes::UNKNOWN, Race::SORC));
    push_back(new Heroes(loyalty ? Heroes::JARKONAS : Heroes::UNKNOWN, Race::BARB));

    // devel
    push_back(new Heroes(IS_DEVEL() ? Heroes::SANDYSANDY : Heroes::UNKNOWN, Race::WRLK));
    push_back(new Heroes(Heroes::UNKNOWN, Race::KNGT));
}

void AllHeroes::clear(void)
{
    for(iterator
	it = begin(); it != end(); ++it) delete *it;
    std::vector<Heroes*>::clear();
}

Heroes* VecHeroes::Get(int hid) const
{
    const std::vector<Heroes *> & vec = * this;
    return 0 <= hid && hid < Heroes::UNKNOWN ? vec[hid] : NULL;
}

Heroes* VecHeroes::Get(const Point & center) const
{
    const_iterator it = begin();
    for(; it != end(); ++it) if((*it)->isPosition(center)) break;
    return end() != it ? *it : NULL;
}

Heroes* AllHeroes::GetGuest(const Castle & castle) const
{
    const_iterator it = std::find_if(begin(), end(),
	    std::bind1st(InCastleNotGuardian(), &castle));
    return end() != it ? *it : NULL;
}

Heroes* AllHeroes::GetGuard(const Castle & castle) const
{
    const_iterator it = Settings::Get().ExtCastleAllowGuardians() ?
        std::find_if(begin(), end(), std::bind1st(InCastleAndGuardian(), &castle)) : end();
    return end() != it ? *it : NULL;
}

Heroes* AllHeroes::GetFreeman(int race) const
{
    const Settings & conf = Settings::Get();

    int min = Heroes::UNKNOWN;
    int max = Heroes::UNKNOWN;

    switch(race)
    {
	case Race::KNGT:
	    min = Heroes::LORDKILBURN;
	    max = Heroes::DIMITRY;
	    break;

	case Race::BARB:
	    min = Heroes::THUNDAX;
	    max = Heroes::ATLAS;
	    break;

	case Race::SORC:
	    min = Heroes::ASTRA;
	    max = Heroes::LUNA;
	    break;

	case Race::WRLK:
	    min = Heroes::ARIE;
	    max = Heroes::WRATHMONT;
	    break;

	case Race::WZRD:
	    min = Heroes::MYRA;
	    max = Heroes::MANDIGAL;
	    break;

	case Race::NECR:
	    min = Heroes::ZOM;
	    max = Heroes::CELIA;
	    break;

	default:
	    min = Heroes::LORDKILBURN;
	    max = conf.ExtCastleAllowRecruitSpecialHeroes() ? (conf.PriceLoyaltyVersion() ? Heroes::JARKONAS : Heroes::BAX) : Heroes::CELIA;
	    break;
    }

    std::vector<int> freeman_heroes;
    freeman_heroes.reserve(HEROESMAXCOUNT);

    // find freeman in race (skip: manual changes)
    for(int ii = min; ii <= max; ++ii)
	if(at(ii)->isFreeman() && !at(ii)->Modes(Heroes::NOTDEFAULTS)) freeman_heroes.push_back(ii);

    // not found, find any race
    if(Race::NONE != race && freeman_heroes.empty())
    {
	min = Heroes::LORDKILBURN;
	max = conf.ExtCastleAllowRecruitSpecialHeroes() ? (conf.PriceLoyaltyVersion() ? Heroes::JARKONAS : Heroes::BAX) : Heroes::CELIA;

	for(int ii = min; ii <= max; ++ii)
	    if(at(ii)->isFreeman()) freeman_heroes.push_back(ii);
    }

    // not found, all heroes busy
    if(freeman_heroes.empty())
    {
	DEBUG(DBG_GAME, DBG_WARN, "freeman not found, all heroes busy.");
	return NULL;
    }

    return at(*Rand::Get(freeman_heroes));
}

void AllHeroes::Scoute(int colors) const
{
    for(const_iterator it = begin(); it != end(); ++it)
	if(colors & (*it)->GetColor()) (*it)->Scoute();
}

Heroes* AllHeroes::FromJail(s32 index) const
{
    const_iterator it = std::find_if(begin(), end(),
	    std::bind1st(InJailMode(), index));
    return end() != it ? *it : NULL;
}

bool AllHeroes::HaveTwoFreemans(void) const
{
    return 2 <= std::count_if(begin(), end(),
			    std::mem_fun(&Heroes::isFreeman));
}

StreamBase & operator<< (StreamBase & msg, const VecHeroes & heroes)
{
    msg << static_cast<u32>(heroes.size());

    for(AllHeroes::const_iterator
        it = heroes.begin(); it != heroes.end(); ++it)
        msg << (*it ? (*it)->GetID() : Heroes::UNKNOWN);

    return msg;
}

StreamBase & operator>> (StreamBase & msg, VecHeroes & heroes)
{
    u32 size;
    msg >> size;

    heroes.resize(size, NULL);

    for(AllHeroes::iterator
        it = heroes.begin(); it != heroes.end(); ++it)
    {
	u32 hid;
    	msg >> hid;
	*it = (hid != Heroes::UNKNOWN ? world.GetHeroes(hid) : NULL);
    }

    return msg;
}

StreamBase & operator<< (StreamBase & msg, const Heroes & hero)
{
    const HeroBase & base = hero;
    const ColorBase & col = hero;

    return msg <<
	base <<
	// heroes
	hero.name <<
	col <<
	hero.killer_color <<
	hero.experience <<
	hero.move_point_scale <<
	hero.secondary_skills <<
	hero.army <<
	hero.hid <<
	hero.portrait <<
	hero.race <<
	hero.save_maps_object <<
	hero.path <<
	hero.direction <<
	hero.sprite_index <<
	hero.patrol_center <<
	hero.patrol_square <<
	hero.visit_object;
}

enum deprecated_t
{
    AIWAITING     = 0x00000002,
    HUNTER        = 0x00000010,
    SCOUTER       = 0x00000020,
    STUPID        = 0x00000040
};

StreamBase & operator>> (StreamBase & msg, Heroes & hero)
{
    HeroBase & base = hero;
    ColorBase & col = hero;

    msg >> base >>
	hero.name >>
	col >>
	hero.killer_color >>
	hero.experience >>
	hero.move_point_scale >>
	hero.secondary_skills >>
	hero.army >>
	hero.hid >>
	hero.portrait >>
	hero.race >>
	hero.save_maps_object >>
	hero.path >>
	hero.direction >>
	hero.sprite_index >>
	hero.patrol_center >>
	hero.patrol_square >>
	hero.visit_object;

    hero.army.SetCommander(&hero);
    return msg;
}

StreamBase & operator<< (StreamBase & msg, const AllHeroes & heroes)
{
    msg << static_cast<u32>(heroes.size());

    for(AllHeroes::const_iterator
        it = heroes.begin(); it != heroes.end(); ++it)
        msg << **it;

    return msg;
}

StreamBase & operator>> (StreamBase & msg, AllHeroes & heroes)
{
    u32 size;
    msg >> size;

    heroes.clear();
    heroes.resize(size, NULL);

    for(AllHeroes::iterator
	it = heroes.begin(); it != heroes.end(); ++it)
    {
	*it = new Heroes();
        msg >> **it;
    }

    return msg;
}
