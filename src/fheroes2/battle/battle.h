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

#ifndef H2BATTLE_H
#define H2BATTLE_H

#include <vector>
#include <utility>
#include "icn.h"
#include "m82.h"
#include "gamedefs.h"
#include "army.h"

namespace Battle
{
    class Arena;
    class Cell;
    class Unit;
    class Tower;
    class Catapult;
    class Bridge;
    class Interface;

    enum { RESULT_LOSS = 0x01, RESULT_RETREAT = 0x02, RESULT_SURRENDER = 0x04, RESULT_WINS = 0x80 };

    struct Result
    {
	u32	army1;
	u32	army2;
	u32	exp1;
	u32	exp2;
	u32	killed;

	Result() : army1(0), army2(0), exp1(0), exp2(0), killed(0) {}

	bool	AttackerWins(void) const;
	bool	DefenderWins(void) const;
	u32	AttackerResult(void) const;
	u32	DefenderResult(void) const;
	u32	GetExperienceAttacker(void) const;
	u32	GetExperienceDefender(void) const;
    };

    StreamBase & operator<< (StreamBase &, const Result &);
    StreamBase & operator>> (StreamBase &, Result &);

    Result	Loader(Army &, Army &, s32);
    void	UpdateMonsterSpriteAnimation(const std::string &);
    void	UpdateMonsterAttributes(const std::string &);

    enum { AS_NONE, AS_IDLE, AS_MOVE, AS_FLY1, AS_FLY2, AS_FLY3, AS_SHOT0, AS_SHOT1, AS_SHOT2, AS_SHOT3, AS_ATTK0, AS_ATTK1, AS_ATTK2, AS_ATTK3, AS_WNCE, AS_KILL };

    struct animframe_t
    {
        int		start;
        int		count;
    };

    struct monstersprite_t
    {
        int		icn_file;
        animframe_t	frm_idle;
        animframe_t	frm_move;
        animframe_t	frm_fly1;
        animframe_t	frm_fly2;
        animframe_t	frm_fly3;
        animframe_t	frm_shot0;
        animframe_t	frm_shot1;
        animframe_t	frm_shot2;
        animframe_t	frm_shot3;
        animframe_t	frm_attk0;
        animframe_t	frm_attk1;
        animframe_t	frm_attk2;
        animframe_t	frm_attk3;
        animframe_t	frm_wnce;
        animframe_t	frm_kill;
        int		m82_attk;
        int		m82_kill;
        int		m82_move;
        int		m82_wnce;
    };

    struct TargetInfo
    {
        Unit*	defender;
	u32	damage;
	u32	killed;
	bool	resist;

	TargetInfo() : defender(NULL), damage(0), killed(0), resist(false) {}

	bool	operator==(const TargetInfo &) const;
        bool	isFinishAnimFrame(void) const;
    };

    StreamBase & operator<< (StreamBase &, const TargetInfo &);
    StreamBase & operator>> (StreamBase &, TargetInfo &);

    struct TargetsInfo : public std::vector<TargetInfo>
    {
	TargetsInfo() {}
    };

    StreamBase & operator<< (StreamBase &, const TargetsInfo &);
    StreamBase & operator>> (StreamBase &, TargetsInfo &);

    enum stats_t
    {
        TR_RESPONSED    = 0x00000001,
        TR_MOVED        = 0x00000002,
        TR_HARDSKIP     = 0x00000004,
        TR_SKIPMOVE     = 0x00000008,

        LUCK_GOOD       = 0x00000100,
        LUCK_BAD        = 0x00000200,
        MORALE_GOOD     = 0x00000400,
        MORALE_BAD      = 0x00000800,

        CAP_TOWER       = 0x00001000,
        CAP_SUMMONELEM  = 0x00002000,
        CAP_MIRROROWNER = 0x00004000,
        CAP_MIRRORIMAGE = 0x00008000,

        TR_DEFENSED     = 0x00010000,

        SP_BLOODLUST    = 0x00020000,
        SP_BLESS        = 0x00040000,
        SP_HASTE        = 0x00080000,
        SP_SHIELD       = 0x00100000,
        SP_STONESKIN    = 0x00200000,
        SP_DRAGONSLAYER = 0x00400000,
        SP_STEELSKIN    = 0x00800000,

        SP_ANTIMAGIC    = 0x01000000,

        SP_CURSE        = 0x02000000,
        SP_SLOW         = 0x04000000,
        SP_BERSERKER    = 0x08000000,
        SP_HYPNOTIZE    = 0x10000000,
        SP_BLIND        = 0x20000000,
        SP_PARALYZE     = 0x40000000,
        SP_STONE        = 0x80000000,

        IS_GOOD_MAGIC   = 0x00FE0000,
        IS_PARALYZE_MAGIC=0xC0000000,
        IS_MIND_MAGIC   = 0x78000000,
        IS_BAD_MAGIC    = 0xFE000000,
        IS_MAGIC        = 0xFFFE0000,

        IS_RED_STATUS   = IS_BAD_MAGIC,
        IS_GREEN_STATUS = SP_SHIELD | SP_STEELSKIN | SP_STONESKIN | SP_DRAGONSLAYER | SP_BLOODLUST | SP_BLESS | SP_HASTE | SP_ANTIMAGIC
    };
}

#endif
