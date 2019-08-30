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
#ifndef H2CURSOR_H
#define H2CURSOR_H

#include "gamedefs.h"

class Cursor : public SpriteMove
{
public:

    enum
    {
	NONE               = 0x0000,
	// ADVMCO.ICN
	POINTER            = 0x1000,
	WAIT               = 0x1001,
        HEROES             = 0x1002,
	CASTLE             = 0x1003,
        MOVE               = 0x1004,
	FIGHT              = 0x1005,
        BOAT               = 0x1006,
	ANCHOR             = 0x1007,
        CHANGE             = 0x1008,
	ACTION             = 0x1009,
        MOVE2              = 0x100A,
	FIGHT2             = 0x100B,
        BOAT2              = 0x100C,
	ANCHOR2            = 0x100D,
        CHANGE2            = 0x100E,
        ACTION2            = 0x100F,
	MOVE3              = 0x1010,
        FIGHT3             = 0x1011,
	BOAT3              = 0x1012,
        ANCHOR3            = 0x1013,
	CHANGE3            = 0x1014,
        ACTION3            = 0x1015,
	MOVE4              = 0x1016,
        FIGHT4             = 0x1017,
	BOAT4              = 0x1018,
        ANCHOR4            = 0x1019,
	CHANGE4            = 0x101A,
        ACTION4            = 0x101B,
	REDBOAT            = 0x101C,
        REDBOAT2           = 0x101D,
        REDBOAT3           = 0x101E,
        REDBOAT4           = 0x101F,
        SCROLL_TOP         = 0x1020,
        SCROLL_TOPRIGHT    = 0x1021,
        SCROLL_RIGHT       = 0x1022,
        SCROLL_BOTTOMRIGHT = 0x1023,
        SCROLL_BOTTOM      = 0x1024,
        SCROLL_BOTTOMLEFT  = 0x1025,
        SCROLL_LEFT        = 0x1026,
        SCROLL_TOPLEFT     = 0x1027,
	POINTER2           = 0x1028,
	// CMSECO.ICN
        WAR_NONE           = 0x2000,
	WAR_MOVE           = 0x2001,
        WAR_FLY            = 0x2002,
        WAR_ARROW          = 0x2003,
        WAR_HERO           = 0x2004,
        WAR_INFO           = 0x2005,
        WAR_POINTER        = 0x2006,
        SWORD_TOPRIGHT     = 0x2007,
        SWORD_RIGHT        = 0x2008,
        SWORD_BOTTOMRIGHT  = 0x2009,
        SWORD_BOTTOMLEFT   = 0x200A,
        SWORD_LEFT         = 0x200B,
        SWORD_TOPLEFT      = 0x200C,
        SWORD_TOP          = 0x200D,
        SWORD_BOTTOM       = 0x200E,
        WAR_BROKENARROW    = 0x200F,
        // SPELCO.ICN
        SP_NONE               = WAR_NONE,
        SP_SLOW               = 0x3001,
        SP_UNKNOWN            = 0x3002,
        SP_CURSE              = 0x3003,
        SP_LIGHTNINGBOLT      = 0x3004,
        SP_CHAINLIGHTNING     = 0x3005,
        SP_CURE               = 0x3006,
        SP_BLESS              = 0x3007,
        SP_FIREBALL           = 0x3008,
        SP_FIREBLAST          = 0x3009,
        SP_TELEPORT           = 0x300A,
        SP_ELEMENTALSTORM     = 0x300B,
        SP_RESURRECTTRUE      = 0x300C,
        SP_RESURRECT          = 0x300D,
        SP_HASTE              = 0x300E,
        SP_SHIELD             = 0x300F,
        SP_ARMAGEDDON         = 0x3010,
        SP_ANTIMAGIC          = 0x3011,
        SP_DISPEL             = 0x3012,
        SP_BERSERKER          = 0x3013,
        SP_PARALYZE           = 0x3014,
        SP_BLIND              = 0x3015,
        SP_HOLYWORD           = 0x3016,
        SP_HOLYSHOUT          = 0x3017,
        SP_METEORSHOWER       = 0x3018,
        SP_ANIMATEDEAD        = 0x3019,
        SP_MIRRORIMAGE        = 0x301A,
        SP_BLOODLUST          = 0x301B,
        SP_DEATHRIPPLE        = 0x301C,
        SP_DEATHWAVE          = 0x301D,
        SP_STEELSKIN          = 0x301E,
        SP_STONESKIN          = 0x301F,
        SP_DRAGONSLAYER       = 0x3020,
        SP_EARTHQUAKE         = 0x3021,
        SP_DISRUPTINGRAY      = 0x3022,
        SP_COLDRING           = 0x3023,
        SP_COLDRAY            = 0x3024,
        SP_HYPNOTIZE          = 0x3025,
        SP_ARROW              = 0x3026
    };

	static Cursor &	Get(void);

	static void	Redraw(s32, s32);
	static int	DistanceThemes(int, u32);
	static int	WithoutDistanceThemes(int);

	int		Themes(void);
	bool		SetThemes(int, bool force = false);
	void		Show(void);

private:
	Cursor();
	void		SetOffset(int);
	void		Move(s32, s32);

	int		theme;
        s32		offset_x;
        s32		offset_y;
};

#endif
