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

#ifndef H2HEROES_H
#define H2HEROES_H

#include <string>
#include <vector>
#include <list>
#include "route.h"
#include "pairs.h"
#include "visit.h"
#include "heroes_base.h"
#include "army.h"
#include "gamedefs.h"

class Recruits;
class Surface;
class MageGuild;
namespace Battle { class Only; }

class Heroes : public HeroBase, public ColorBase
{
public:
    enum
    {
	// knight
	LORDKILBURN, SIRGALLANTH, ECTOR, GVENNETH, TYRO, AMBROSE, RUBY, MAXIMUS, DIMITRY,
	// barbarian
	THUNDAX, FINEOUS, JOJOSH, CRAGHACK, JEZEBEL, JACLYN, ERGON, TSABU, ATLAS,
	// sorceress
	ASTRA, NATASHA, TROYAN, VATAWNA, REBECCA, GEM, ARIEL, CARLAWN, LUNA,
	// warlock
	ARIE, ALAMAR, VESPER, CRODO, BAROK, KASTORE, AGAR, FALAGAR, WRATHMONT,
	// wizard
	MYRA, FLINT, DAWN, HALON, MYRINI, WILFREY, SARAKIN, KALINDRA, MANDIGAL,
	// necromancer
	ZOM, DARLANA, ZAM, RANLOO, CHARITY, RIALDO, ROXANA, SANDRO, CELIA,
	// from campain
	ROLAND, CORLAGON, ELIZA, ARCHIBALD, HALTON, BAX,
	// from extended
	SOLMYR, DAINWIN, MOG, UNCLEIVAN, JOSEPH, GALLAVANT, ELDERIAN, CEALLACH, DRAKONIA, MARTINE, JARKONAS,
	// debugger
	SANDYSANDY, UNKNOWN
    };

    static Surface	GetPortrait(int heroid, int type);
    static const char*	GetName(int heroid);

    enum flags_t
    {
	SHIPMASTER	= 0x00000001,
	//UNUSED	= 0x00000002,
	SPELLCASTED	= 0x00000004,
	ENABLEMOVE	= 0x00000008,
	//UNUSED	= 0x00000010,
	//UNUSED	= 0x00000020,
	//UNUSED	= 0x00000040,
	JAIL		= 0x00000080,
	ACTION		= 0x00000100,
	SAVEPOINTS	= 0x00000200,
	SLEEPER		= 0x00000400,
	GUARDIAN	= 0x00000800,
	NOTDEFAULTS	= 0x00001000,
	NOTDISMISS	= 0x00002000,
	VISIONS		= 0x00004000,
	PATROL		= 0x00008000,
	CUSTOMARMY	= 0x00010000,
	CUSTOMSKILLS	= 0x00020000
    };

    Heroes();
    Heroes(int heroid, int rc);

    bool		isValid() const;
    bool		isFreeman() const;
    void		SetFreeman(int reason);

    const Castle*	inCastle() const;
    Castle*		inCastle();

    void		LoadFromMP2(s32 map_index, int cl, int rc, StreamBuf);
    void		PostLoad();

    int			GetRace() const;
    const std::string &	GetName() const;
    int			GetColor() const;
    int			GetType() const;
    int			GetControl() const;

    int			GetKillerColor() const;
    void		SetKillerColor(int);

    const Army &	GetArmy() const;
    Army &		GetArmy();

    int			GetID() const;

    int			GetAttack() const;
    int			GetDefense() const;
    int			GetPower() const;
    int			GetKnowledge() const;

    int			GetAttack(std::string*) const;
    int			GetDefense(std::string*) const;
    int			GetPower(std::string*) const;
    int			GetKnowledge(std::string*) const;

    void		IncreasePrimarySkill(int skill);

    int			GetMorale() const;
    int			GetLuck() const;
    int			GetMoraleWithModificators(std::string *str = NULL) const;
    int			GetLuckWithModificators(std::string *str = NULL) const;
    int			GetLevel() const;

    int			GetMapsObject() const;
    void		SetMapsObject(int);

    const Point &	GetCenterPatrol() const;
    void		SetCenterPatrol(const Point &);
    int			GetSquarePatrol() const;

    u32			GetMaxSpellPoints() const;
    u32			GetMaxMovePoints() const;

    u32			GetMovePoints() const;
    void		IncreaseMovePoints(u32);
    bool		MayStillMove() const;
    void		ResetMovePoints();
    void		MovePointsScaleFixed();
    void		RecalculateMovePoints();

    bool		HasSecondarySkill(int) const;
    bool		HasMaxSecondarySkill() const;
    int			GetLevelSkill(int) const;
    u32			GetSecondaryValues(int) const;
    void		LearnSkill(const Skill::Secondary &);
    Skill::SecSkills &	GetSecondarySkills();

    bool		PickupArtifact(const Artifact &);
    bool		HasUltimateArtifact() const;
    u32			GetCountArtifacts() const;
    bool		IsFullBagArtifacts() const;

    int			GetMobilityIndexSprite() const;
    int			GetManaIndexSprite() const;

    int			OpenDialog(bool readonly = false, bool fade = false);
    void		MeetingDialog(Heroes &);

    bool		Recruit(int col, const Point & pt);
    bool		Recruit(const Castle & castle);

    void		ActionNewDay();
    void		ActionNewWeek();
    void		ActionNewMonth();
    void		ActionAfterBattle();
    void		ActionPreBattle();

    bool		BuySpellBook(const Castle*, int shrine = 0);

    const Route::Path &	GetPath() const;
    Route::Path &	GetPath();
    int			GetRangeRouteDays(s32) const;
    void		ShowPath(bool);
    void		RescanPath();
    void		RescanPathPassable();

    int			GetDirection() const;

    void		SetVisited(s32, Visit::type_t = Visit::LOCAL);
    void		SetVisitedWideTile(s32, int object, Visit::type_t = Visit::LOCAL);
    bool		isVisited(int object, Visit::type_t = Visit::LOCAL) const;
    bool		isVisited(const Maps::Tiles &, Visit::type_t = Visit::LOCAL) const;

    bool		Move(bool fast = false);
    void		Move2Dest(const s32 &, bool skip_action = false);
    bool		isEnableMove() const;
    bool		CanMove() const;
    void		SetMove(bool);
    bool		isAction() const;
    void		ResetAction();
    void		Action(s32);
    void		ActionNewPosition();
    bool		ApplyPenaltyMovement();
    bool		ActionSpellCast(const Spell &);

    void		Redraw(Surface &, bool) const;
    void		Redraw(Surface &, s32, s32, bool) const;
    void		PortraitRedraw(s32, s32, int type, Surface &) const;
    int			GetSpriteIndex() const;
    void		FadeOut() const;
    void		FadeIn() const;
    void		Scoute() const;
    int			GetScoute() const;
    int			CanScouteTile(s32) const;
    u32			GetVisionsDistance() const;

    bool		isShipMaster() const;
    void		SetShipMaster(bool);

    u32			GetExperience() const;
    void		IncreaseExperience(u32);

    bool		AllowBattle(bool attacker) const;

    std::string		String() const;
    Surface             GetPortrait(int type) const;

    static int		GetLevelFromExperience(u32);
    static u32		GetExperienceFromLevel(int);

    static void		ScholarAction(Heroes &, Heroes &);

private:
    friend StreamBase & operator<< (StreamBase &, const Heroes &);
    friend StreamBase & operator>> (StreamBase &, Heroes &);
#ifdef WITH_XML
    friend TiXmlElement & operator>> (TiXmlElement &, Heroes &);
#endif
    friend class Recruits;
    friend class Battle::Only;

    void		LevelUp(bool skipsecondary, bool autoselect = false);
    int			LevelUpPrimarySkill();
    void		LevelUpSecondarySkill(int, bool autoselect = false);
    void		AngleStep(int);
    bool		MoveStep(bool fast = false);
    static void		MoveStep(Heroes &, s32 from, s32 to, bool newpos);

    std::string		name;
    ColorBase		killer_color;
    u32			experience;
    s32			move_point_scale;

    Skill::SecSkills	secondary_skills;

    Army        	army;

    int			hid; 		/* hero id */
    int			portrait;	/* hero id */
    int			race;
    int			save_maps_object;

    Route::Path		path;

    int			direction;
    int			sprite_index;

    Point		patrol_center;
    int			patrol_square;

    std::list<IndexObject>
			visit_object;
};

struct VecHeroes : public std::vector<Heroes*>
{
    Heroes* Get(int /* hero id */) const;
    Heroes* Get(const Point &) const;
};

struct AllHeroes : public VecHeroes
{
    AllHeroes();
    ~AllHeroes();

    void	Init();
    void	clear();

    void	Scoute(int) const;

    Heroes*	GetGuest(const Castle &) const;
    Heroes*	GetGuard(const Castle &) const;
    Heroes*	GetFreeman(int race) const;
    Heroes*	FromJail(s32) const;

    bool HaveTwoFreemans() const;
};

StreamBase & operator<< (StreamBase &, const VecHeroes &);
StreamBase & operator>> (StreamBase &, VecHeroes &);

StreamBase & operator<< (StreamBase &, const Heroes &);
StreamBase & operator>> (StreamBase &, Heroes &);

StreamBase & operator<< (StreamBase &, const AllHeroes &);
StreamBase & operator>> (StreamBase &, AllHeroes &);

#endif
