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

#include <string>
#include <algorithm>
#include "agg.h"
#include "button.h"
#include "cursor.h"
#include "settings.h"
#include "text.h"
#include "world.h"
#include "kingdom.h"
#include "game.h"
#include "dialog.h"
#include "castle.h"
#include "pocketpc.h"

// dialog_thievesguild.cpp
struct ValueColors : std::pair<int, int>
{
    ValueColors();
    ValueColors(int v, int c);

    bool IsValue(int v) const;
    bool IsColor(int c) const;

    static bool SortValueGreat(const ValueColors & v1, const ValueColors & v2);
};

void GetTownsInfo(std::vector<ValueColors> &, const Colors &);
void GetCastlesInfo(std::vector<ValueColors> &, const Colors &);
void GetHeroesInfo(std::vector<ValueColors> &, const Colors &);
void GetGoldsInfo(std::vector<ValueColors> &, const Colors &);
void GetWoodOreInfo(std::vector<ValueColors> &, const Colors &);
void GetGemsCrSlfMerInfo(std::vector<ValueColors> &, const Colors &);
void GetObelisksInfo(std::vector<ValueColors> &, const Colors &);
void GetArmyInfo(std::vector<ValueColors> &, const Colors &);
void GetIncomesInfo(std::vector<ValueColors> &, const Colors &);
void GetBestHeroArmyInfo(std::vector<ValueColors> &, const Colors &);

// from dialog_thievesguild.cpp
void DrawFlags(const std::vector<ValueColors> &, const Point &, u32 width, u32 count);
void DrawHeroIcons(const std::vector<ValueColors> &, const Point &, u32 width);

void PocketPC::ThievesGuild(bool oracle)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder(Size(320, 224));
    const Rect & dst_rt = frameborder.GetArea();

    const Point & cur_pt = dst_rt;
    Point dst_pt(cur_pt);

    const u32 count = oracle ? 0xFF : world.GetKingdom(Settings::Get().CurrentColor()).GetCountBuilding(BUILD_THIEVESGUILD);

    std::vector<ValueColors> v;
    v.reserve(KINGDOMMAX);
    const Colors colors(Game::GetActualKingdomColors());
    u32 textx = 115;
    u32 startx = 120;
    u32 maxw = 200;
    Text text;
    text.Set(Font::SMALL);

    // head 1
    u32 ii = 0;
    for(ii = 0; ii < colors.size(); ++ii)
    {
	switch(ii+1)
	{
	    case 1: text.Set(_("1st")); break;
	    case 2: text.Set(_("2nd")); break;
	    case 3: text.Set(_("3rd")); break;
	    case 4: text.Set(_("4th")); break;
	    case 5: text.Set(_("5th")); break;
	    case 6: text.Set(_("6th")); break;
	    default: break;
	}

	dst_pt.x = cur_pt.x + startx + maxw / (colors.size() * 2) + ii * maxw / colors.size() - text.w() / 2;
	dst_pt.y = cur_pt.y + 25;
	text.Blit(dst_pt);
    }

    // button exit
    const Rect rectExit(dst_rt.x + dst_rt.w - 26, dst_rt.y + 7, 25, 25);
    AGG::GetICN(ICN::TOWNWIND, 12).Blit(rectExit.x, rectExit.y);

    text.Set(_("Number of Towns:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 35;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetTownsInfo(v, colors);
    DrawFlags(v, dst_pt, maxw, colors.size());

    text.Set(_("Number of Castles:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 47;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetCastlesInfo(v, colors);
    DrawFlags(v, dst_pt, maxw, colors.size());

    text.Set(_("Number of Heroes:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 59;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetHeroesInfo(v, colors);
    DrawFlags(v, dst_pt, maxw, colors.size());

    text.Set(_("Gold in Treasury:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 71;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetGoldsInfo(v, colors);
    if(1 < count) DrawFlags(v, dst_pt, maxw, colors.size());

    text.Set(_("Wood & Ore:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 83;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetWoodOreInfo(v, colors);
    if(1 < count) DrawFlags(v, dst_pt, maxw, colors.size());

    text.Set(_("Gems, Cr, Slf & Mer:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 95;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetGemsCrSlfMerInfo(v, colors);
    if(1 < count) DrawFlags(v, dst_pt, maxw, colors.size());

    text.Set(_("Obelisks Found:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 107;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetObelisksInfo(v, colors);
    if(2 < count) DrawFlags(v, dst_pt, maxw, colors.size());

    text.Set(_("Total Army Strength:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 119;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetArmyInfo(v, colors);
    if(3 < count) DrawFlags(v, dst_pt, maxw, colors.size());

    text.Set(_("Income:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 131;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetIncomesInfo(v, colors);
    if(4 < count) DrawFlags(v, dst_pt, maxw, colors.size());

    textx = 75;
    startx = 80;
    maxw = 240;

    // head 2
    ii = 0;
    for(Colors::const_iterator
	color = colors.begin(); color != colors.end(); ++color)
    {
	text.Set(Color::String(*color), Font::SMALL);
	dst_pt.x = cur_pt.x + startx + maxw / (colors.size() * 2) + ii * maxw / colors.size() - text.w() / 2;
	dst_pt.y = cur_pt.y + 145;
	text.Blit(dst_pt);
	++ii;
    }

    text.Set(_("Best Hero:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 160;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetBestHeroArmyInfo(v, colors);
    DrawHeroIcons(v, dst_pt, maxw);

    text.Set(_("Best Hero Stats:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 200;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    //GetBestHeroStatsInfo(v);
    //if(1 < count) DrawHeroIcons(v, dst_pt, maxw);
/*
    text.Set(_("Personality:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 388;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    //GetPersonalityInfo(v);
    //if(2 < count) DrawHeroIcons(v, dst_pt, maxw);

    text.Set(_("Best Monster:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 429;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    //GetBestMonsterInfo(v);
    //if(3 < count) DrawHeroIcons(v, dst_pt, maxw);

    //buttonExit.Draw();
*/
    cursor.Show();
    display.Flip();

    // message loop
    while(le.HandleEvents())
    {
        if(le.MouseClickLeft(rectExit) || HotKeyCloseWindow) break;
    }
}
