/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cassert>
#include <cstdint>

#include "math_base.h"
#include "screen.h"

namespace fheroes2
{
    class Image;
}

class Cursor
{
public:
    enum CursorType : int
    {
        NONE = 0x0000,
        // ADVMCO.ICN
        POINTER = 0x1000,
        WAIT = 0x1001,
        HEROES = 0x1002,
        CASTLE = 0x1003,
        // 0x1004 to 0x101F are not in use anymore
        SCROLL_TOP = 0x1020,
        SCROLL_TOPRIGHT = 0x1021,
        SCROLL_RIGHT = 0x1022,
        SCROLL_BOTTOMRIGHT = 0x1023,
        SCROLL_BOTTOM = 0x1024,
        SCROLL_BOTTOMLEFT = 0x1025,
        SCROLL_LEFT = 0x1026,
        SCROLL_TOPLEFT = 0x1027,
        POINTER_VIDEO = 0x1028, // this cursor is used only for video playback
        // CMSECO.ICN
        WAR_NONE = 0x2000,
        WAR_MOVE = 0x2001,
        WAR_FLY = 0x2002,
        WAR_ARROW = 0x2003,
        WAR_HERO = 0x2004,
        WAR_INFO = 0x2005,
        WAR_POINTER = 0x2006,
        SWORD_TOPRIGHT = 0x2007,
        SWORD_RIGHT = 0x2008,
        SWORD_BOTTOMRIGHT = 0x2009,
        SWORD_BOTTOMLEFT = 0x200A,
        SWORD_LEFT = 0x200B,
        SWORD_TOPLEFT = 0x200C,
        SWORD_TOP = 0x200D,
        SWORD_BOTTOM = 0x200E,
        WAR_BROKENARROW = 0x200F,
        // SPELCO.ICN
        SP_NONE = WAR_NONE,
        SP_SLOW = 0x3001,
        SP_UNKNOWN = 0x3002,
        SP_CURSE = 0x3003,
        SP_LIGHTNINGBOLT = 0x3004,
        SP_CHAINLIGHTNING = 0x3005,
        SP_CURE = 0x3006,
        SP_BLESS = 0x3007,
        SP_FIREBALL = 0x3008,
        SP_FIREBLAST = 0x3009,
        SP_TELEPORT = 0x300A,
        SP_ELEMENTALSTORM = 0x300B,
        SP_RESURRECTTRUE = 0x300C,
        SP_RESURRECT = 0x300D,
        SP_HASTE = 0x300E,
        SP_SHIELD = 0x300F,
        SP_ARMAGEDDON = 0x3010,
        SP_ANTIMAGIC = 0x3011,
        SP_DISPEL = 0x3012,
        SP_BERSERKER = 0x3013,
        SP_PARALYZE = 0x3014,
        SP_BLIND = 0x3015,
        SP_HOLYWORD = 0x3016,
        SP_HOLYSHOUT = 0x3017,
        SP_METEORSHOWER = 0x3018,
        SP_ANIMATEDEAD = 0x3019,
        SP_MIRRORIMAGE = 0x301A,
        SP_BLOODLUST = 0x301B,
        SP_DEATHRIPPLE = 0x301C,
        SP_DEATHWAVE = 0x301D,
        SP_STEELSKIN = 0x301E,
        SP_STONESKIN = 0x301F,
        SP_DRAGONSLAYER = 0x3020,
        SP_EARTHQUAKE = 0x3021,
        SP_DISRUPTINGRAY = 0x3022,
        SP_COLDRING = 0x3023,
        SP_COLDRAY = 0x3024,
        SP_HYPNOTIZE = 0x3025,
        SP_ARROW = 0x3026
    };

    enum MapHeroCursor : int
    {
        CURSOR_HERO_MOVE = 0x4000,
        CURSOR_HERO_MOVE_2,
        CURSOR_HERO_MOVE_3,
        CURSOR_HERO_MOVE_4,
        CURSOR_HERO_MOVE_5,
        CURSOR_HERO_MOVE_6,
        CURSOR_HERO_MOVE_7,
        CURSOR_HERO_MOVE_8,
        CURSOR_HERO_FIGHT,
        CURSOR_HERO_FIGHT_2,
        CURSOR_HERO_FIGHT_3,
        CURSOR_HERO_FIGHT_4,
        CURSOR_HERO_FIGHT_5,
        CURSOR_HERO_FIGHT_6,
        CURSOR_HERO_FIGHT_7,
        CURSOR_HERO_FIGHT_8,
        CURSOR_HERO_BOAT,
        CURSOR_HERO_BOAT_2,
        CURSOR_HERO_BOAT_3,
        CURSOR_HERO_BOAT_4,
        CURSOR_HERO_BOAT_5,
        CURSOR_HERO_BOAT_6,
        CURSOR_HERO_BOAT_7,
        CURSOR_HERO_BOAT_8,
        CURSOR_HERO_ANCHOR,
        CURSOR_HERO_ANCHOR_2,
        CURSOR_HERO_ANCHOR_3,
        CURSOR_HERO_ANCHOR_4,
        CURSOR_HERO_ANCHOR_5,
        CURSOR_HERO_ANCHOR_6,
        CURSOR_HERO_ANCHOR_7,
        CURSOR_HERO_ANCHOR_8,
        CURSOR_HERO_MEET,
        CURSOR_HERO_MEET_2,
        CURSOR_HERO_MEET_3,
        CURSOR_HERO_MEET_4,
        CURSOR_HERO_MEET_5,
        CURSOR_HERO_MEET_6,
        CURSOR_HERO_MEET_7,
        CURSOR_HERO_MEET_8,
        CURSOR_HERO_ACTION,
        CURSOR_HERO_ACTION_2,
        CURSOR_HERO_ACTION_3,
        CURSOR_HERO_ACTION_4,
        CURSOR_HERO_ACTION_5,
        CURSOR_HERO_ACTION_6,
        CURSOR_HERO_ACTION_7,
        CURSOR_HERO_ACTION_8,
        CURSOR_HERO_BOAT_ACTION,
        CURSOR_HERO_BOAT_ACTION_2,
        CURSOR_HERO_BOAT_ACTION_3,
        CURSOR_HERO_BOAT_ACTION_4,
        CURSOR_HERO_BOAT_ACTION_5,
        CURSOR_HERO_BOAT_ACTION_6,
        CURSOR_HERO_BOAT_ACTION_7,
        CURSOR_HERO_BOAT_ACTION_8
    };

    Cursor( const Cursor & ) = delete;

    Cursor & operator=( const Cursor & ) = delete;

    static Cursor & Get();

    // Returns a non-empty area if it should be updated while rendering.
    static fheroes2::Rect updateCursorPosition( const int32_t x, const int32_t y );

    static int DistanceThemes( const int theme, uint32_t distance );
    static int WithoutDistanceThemes( const int theme );
    static void Refresh();

    int Themes() const
    {
        assert( _theme <= CURSOR_HERO_BOAT_ACTION_8 );
        return _theme;
    }

    void SetThemes( const int theme, const bool force = false );

    void setCustomImage( const fheroes2::Image & image, const fheroes2::Point & offset );

    // Only for software emulation.
    void setVideoPlaybackCursor();

    // Do not call this method directly anywhere except Settings.
    void setMonochromeCursor( const bool enable )
    {
        _monochromeCursorThemes = enable;
    }

private:
    Cursor() = default;
    ~Cursor() = default;

    void SetOffset( int name, const fheroes2::Point & defaultOffset );
    void Move( int32_t x, int32_t y ) const;

    int _theme{ Cursor::CursorType::NONE };

    fheroes2::Point _offset;

    bool _monochromeCursorThemes{ false };
};

class CursorRestorer
{
public:
    CursorRestorer() = default;
    // Use only to hide a cursor. Previous cursor visibility and theme will be restored when this object is destroyed.
    explicit CursorRestorer( const bool visible );
    // Use to set cursor visibility and theme of the cursor. Previous cursor visibility and theme will be restored when this object is destroyed.
    CursorRestorer( const bool visible, const int theme );
    CursorRestorer( const CursorRestorer & ) = delete;

    ~CursorRestorer();

    CursorRestorer & operator=( const CursorRestorer & ) = delete;

private:
    const int _theme{ Cursor::Get().Themes() };
    const bool _visible{ fheroes2::cursor().isVisible() };
};

#endif
