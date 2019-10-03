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

#ifndef H2BATTLE_TROOP_H
#define H2BATTLE_TROOP_H

#include <vector>
#include <utility>
#include "bitmodes.h"
#include "sprite.h"
#include "battle.h"
#include "battle_army.h"
#include "battle_arena.h"

class Sprite;
class Spell;
class HeroBase;

namespace Battle
{
    struct ModeDuration : public std::pair<u32, u32>
    {
	ModeDuration();
	ModeDuration(u32, u32);

	bool isMode(u32) const;
	bool isZeroDuration() const;
	void DecreaseDuration();
    };

    struct ModesAffected : public std::vector<ModeDuration>
    {
	ModesAffected();

	u32  GetMode(u32) const;
	void AddMode(u32, u32);
	void RemoveMode(u32);
	void DecreaseDuration();

	u32  FindZeroDuration() const;
    };

    StreamBase & operator<< (StreamBase &, const ModesAffected &);
    StreamBase & operator>> (StreamBase &, ModesAffected &);

    enum { CONTOUR_MAIN = 0, CONTOUR_BLACK = 0x01, CONTOUR_REFLECT = 0x02 };

    // battle troop stats
    class Unit : public ArmyTroop, public BitModes, public Control
    {
    public:
	Unit(const Troop &, s32 pos, bool reflect);
	~Unit();

	bool		isModes(u32) const;
	bool		isBattle() const;
	std::string	GetShotString() const;
	std::string	GetSpeedString() const;
	u32		GetHitPointsLeft() const;
	u32		GetAffectedDuration(u32) const;
	u32		GetSpeed() const;
	Surface		GetContour(int) const;

	void	InitContours();
	void	SetMirror(Unit*);
	void	SetRandomMorale();
	void	SetRandomLuck();
	void	NewTurn();

	bool	isValid() const;
	bool	isArchers() const;
	bool	isFly() const;
	bool	isTwiceAttack() const;

	bool	AllowResponse() const;
	bool	isHandFighting() const;
	bool	isReflect() const;
	bool	isHaveDamage() const;
	bool 	isMagicResist(const Spell &, u32) const;
	bool	isMagicAttack() const;
	bool	OutOfWalls() const;


	std::string String(bool more = false) const;

	u32	GetUID() const;
	bool	isUID(u32) const;

	s32		GetHeadIndex() const;
	s32		GetTailIndex() const;
	const Position & GetPosition() const;
	void		SetPosition(s32);
	void		SetPosition(const Position &);
	void		SetReflection(bool);

	u32	GetAttack() const;
	u32	GetDefense() const;
	int	GetArmyColor() const;
	int	GetColor() const;
	u32	GetSpeed(bool skip_standing_check) const;
	int	GetControl() const;
	u32	GetDamage(const Unit &) const;
	s32	GetScoreQuality(const Unit &) const;
	u32	GetDead() const;
	u32	GetHitPoints() const;
	u32	GetShots() const;
	u32	ApplyDamage(Unit &, u32);
	u32	ApplyDamage(u32);
	u32	GetDamageMin(const Unit &) const;
	u32	GetDamageMax(const Unit &) const;
	u32     CalculateDamageUnit(const Unit &, float) const;
	bool	ApplySpell(const Spell &, const HeroBase* hero, TargetInfo &);
	bool	AllowApplySpell(const Spell &, const HeroBase* hero, std::string* msg = NULL) const;
	void	PostAttackAction(Unit &);
	void	ResetBlind();
	void	SpellModesAction(const Spell &, u32, const HeroBase*);
	void	SpellApplyDamage(const Spell &, u32, const HeroBase*, TargetInfo &);
	void	SpellRestoreAction(const Spell &, u32, const HeroBase*);
	u32	Resurrect(u32, bool, bool);

	const monstersprite_t &
		GetMonsterSprite() const;

	const animframe_t &
		GetFrameState() const;
	const animframe_t &
		GetFrameState(int) const;
	void	IncreaseAnimFrame(bool loop = false);
	bool    isStartAnimFrame() const;
	bool    isFinishAnimFrame() const;
	void	SetFrameStep(int);
	void	SetFrame(int);
	int	GetFrame() const;
	int     GetFrameOffset() const;
	int     GetFrameStart() const;
	int     GetFrameCount() const;

	int	GetStartMissileOffset(int) const;

	int	M82Attk() const;
	int	M82Kill() const;
	int	M82Move() const;
	int	M82Wnce() const;
	int	M82Expl() const;

	int	ICNFile() const;
	int	ICNMiss() const;

	Point	GetBackPoint() const;
	Rect	GetRectPosition() const;

	u32	HowManyCanKill(const Unit &) const;
	u32	HowManyWillKilled(u32) const;

	void	SetResponse();
	void	ResetAnimFrame(int);
	void	UpdateDirection();
	bool	UpdateDirection(const Rect &);
	void	PostKilledAction();

	u32	GetMagicResist(const Spell &, u32) const;
	int	GetSpellMagic(bool force = false) const;
	u32	GetObstaclesPenalty(const Unit &) const;

	const HeroBase* GetCommander() const;

	static bool isHandFighting(const Unit &, const Unit &);

    private:
	friend StreamBase & operator<< (StreamBase &, const Unit &);
	friend StreamBase & operator>> (StreamBase &, Unit &);

	u32		uid;
	u32		hp;
	u32		count0;
	u32		dead;
	u32		shots;
	u32		disruptingray;
	bool		reflect;

	u32		animstate;
	s32		animframe;
	s32		animstep;

	Position	position;
	ModesAffected	affected;
	Unit*		mirror;
	Surface		contours[4];

	bool		blindanswer;
    };

    StreamBase & operator<< (StreamBase &, const Unit &);
    StreamBase & operator>> (StreamBase &, Unit &);
}

#endif
