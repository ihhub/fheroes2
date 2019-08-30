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
#ifndef H2DIALOG_H
#define H2DIALOG_H

#include <string>
#include <list>
#include <vector>
#include "gamedefs.h"

#define	SHADOWWIDTH	16
#define BOXAREA_WIDTH   245

class Castle;
class Heroes;
class Surface;
class Artifact;
class Spell;
class Monster;
class Funds;
class Troop;
struct CapturedObject;

namespace Skill
{
    class Secondary;
}

namespace Maps
{
    class FileInfo;
    class Tiles;
}

namespace Dialog
{
    enum
    {
	ZERO	= 0x0000,
	YES     = 0x0001,
        OK      = 0x0002,
        NO      = 0x0004,
        CANCEL  = 0x0008,
        DISMISS = 0x0010,
        UPGRADE = 0x0020,
        MAX     = 0x0040,
	PREV	= 0x0080,
	NEXT	= 0x0100,

	WORLD	= 0x0200,
	PUZZLE	= 0x0400,
	INFO	= 0x0800,
	DIG	= 0x1000,

	UPGRADE_DISABLE = MAX,

	READONLY= 0x2000,
	BUTTONS = (YES|OK|NO|CANCEL)
    };

    int		AdventureOptions(bool enabledig);
    int		FileOptions(void);
    int		SystemOptions(void);
    std::string SelectFileLoad(void);
    std::string SelectFileSave(void);
    // show info cell maps
    void	QuickInfo(const Maps::Tiles &);
    void	QuickInfo(const Castle &);
    void	QuickInfo(const Heroes &);
    int		Message(const std::string &, const std::string &, int ft, int buttons = 0 /* buttons: OK : CANCEL : OK|CANCEL : YES|NO */);
    void	ExtSettings(bool);
    int   	LevelUpSelectSkill(const std::string &, const std::string &, const Skill::Secondary &, const Skill::Secondary &, Heroes &);
    bool	SelectGoldOrExp(const std::string &, const std::string &, u32 gold, u32 expr, const Heroes &);
    void	SpellInfo(const Spell &, bool ok_button = true);
    void	SpellInfo(const std::string &, const std::string &, const Spell &, bool ok_button = true);
    void	SecondarySkillInfo(const Skill::Secondary &, const bool ok_button = true);
    void	SecondarySkillInfo(const std::string &, const std::string &, const Skill::Secondary &, const bool ok_button = true);
    void	PrimarySkillInfo(const std::string &, const std::string &, int);
    int		SpriteInfo(const std::string &, const std::string &, const Surface &, int buttons = Dialog::OK);
    int		ArtifactInfo(const std::string &, const std::string &, const Artifact &, int buttons = Dialog::OK);
    int		ResourceInfo(const std::string &, const std::string &, const Funds &, int buttons = Dialog::OK);
    int		SelectSkillFromArena(void);
    bool	SelectCount(const std::string &, u32 min, u32 max, u32 & res, int step = 1);
    bool	InputString(const std::string &, std::string &);
    Troop	RecruitMonster(const Monster &, u32 available, bool);
    void	DwellingInfo(const Monster &, u32 available);
    bool	SetGuardian(Heroes &, Troop &, CapturedObject &, bool readonly);
    int		ArmyInfo(const Troop & troop, int flags);
    int		ArmyJoinFree(const Troop &, Heroes &);
    int		ArmyJoinWithCost(const Troop &, u32 join, u32 gold, Heroes &);
    int		ArmySplitTroop(int free_slots, u32 max, u32 &, bool);
    void	Marketplace(bool fromTradingPost = false);
    void	MakeGiftResource(void);
    int		BuyBoat(bool enable);
    void	PuzzleMaps(void);
    void	ThievesGuild(bool oracle);
    void	GameInfo(void);

    class FrameBox
    {
    public:
	FrameBox(int height, bool buttons = false);
	~FrameBox();

	const Rect &	GetArea(void){ return area; };

    protected:
	SpriteBack	background;
	Rect		area;
    };

    class FrameBorder
    {
    public:
	FrameBorder(int v = BORDERWIDTH);
	FrameBorder(const Size &);
	FrameBorder(const Size &, const Surface &);
	FrameBorder(s32, s32, u32, u32);
	~FrameBorder();

	void	SetBorder(int);
	int	BorderWidth(void) const;
	int	BorderHeight(void) const;
	bool	isValid(void) const;
	void	SetPosition(s32, s32, u32 = 0, u32 = 0);

	const Rect & GetRect(void) const;
	const Rect & GetArea(void) const;
	const Rect & GetTop(void) const;

	static void RenderRegular(const Rect &);
	static void RenderOther(const Surface &, const Rect &);

    protected:
	SpriteBack	background;
	Rect		rect;
	Rect		area;
	Rect		top;
	int		border;
    };
}

#endif
