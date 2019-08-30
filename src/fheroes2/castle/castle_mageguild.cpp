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

#include <algorithm>
#include <vector>
#include <string>
#include "agg.h"
#include "button.h"
#include "cursor.h"
#include "castle.h"
#include "dialog.h"
#include "game.h"
#include "race.h"
#include "settings.h"
#include "mageguild.h"
#include "text.h"

RowSpells::RowSpells(const Point & pos, const Castle & castle, int lvl)
{
    const MageGuild & guild = castle.GetMageGuild();
    bool hide = castle.GetLevelMageGuild() < lvl;
    const Sprite & roll_show = AGG::GetICN(ICN::TOWNWIND, 0);
    const Sprite & roll_hide = AGG::GetICN(ICN::TOWNWIND, 1);
    const Sprite & roll = (hide ? roll_hide : roll_show);

    u32 count = 0;

    switch(lvl)
    {
	case 1:
	case 2: count = 3; break;
	case 3:
	case 4: count = 2; break;
	case 5: count = 1; break;
	default: break;
    }

    for(u32 ii = 0; ii < count; ++ii)
	coords.push_back(Rect(pos.x + coords.size() * (Settings::Get().QVGA() ? 72 : 110) - roll.w() / 2, pos.y, roll.w(), roll.h()));

    if(castle.HaveLibraryCapability())
    {
	if(! hide && castle.isLibraryBuild())
	    coords.push_back(Rect(pos.x + coords.size() * (Settings::Get().QVGA() ? 72 : 110) - roll_show.w() / 2, pos.y, roll_show.w(), roll_show.h()));
	else
	    coords.push_back(Rect(pos.x + coords.size() * (Settings::Get().QVGA() ? 72 : 110) - roll_hide.w() / 2, pos.y, roll_hide.w(), roll_hide.h()));
    }

    spells.reserve(6);
    spells = guild.GetSpells(castle.GetLevelMageGuild(), castle.isLibraryBuild(), lvl);
    spells.resize(coords.size(), Spell::NONE);
}

void RowSpells::Redraw(void)
{
    const Sprite & roll_show = AGG::GetICN(ICN::TOWNWIND, 0);
    const Sprite & roll_hide = AGG::GetICN(ICN::TOWNWIND, 1);

    for(Rects::iterator
	it = coords.begin(); it != coords.end(); ++it)
    {
	const Rect & dst = (*it);
	const Spell & spell = spells[std::distance(coords.begin(), it)];

	// roll hide
	if(dst.w < roll_show.w() || spell == Spell::NONE)
	{
	    roll_hide.Blit(dst);
	}
	// roll show
	else
	{
	    roll_show.Blit(dst);

	    const Sprite & icon = AGG::GetICN(ICN::SPELLS, spell.IndexSprite());

	    if(Settings::Get().QVGA())
	    {
		icon.Blit(dst.x + 2 + (dst.w - icon.w()) / 2, dst.y + 20 - icon.h() / 2);
	    }
	    else
	    {
		icon.Blit(dst.x + 5 + (dst.w - icon.w()) / 2, dst.y + 40 - icon.h() / 2);

		TextBox text(std::string(spell.GetName()) + " [" + GetString(spell.SpellPoint(NULL)) + "]", Font::SMALL, 78);
		text.Blit(dst.x + 18, dst.y + 62);
	    }
	}
    }
}

bool RowSpells::QueueEventProcessing(void)
{
    LocalEvent & le = LocalEvent::Get();
    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();

    const s32 index = coords.GetIndex(le.GetMouseCursor());

    if(0 <= index &&
       (le.MouseClickLeft() || le.MousePressRight()))
    {
	const Spell & spell = spells[index];

	if(spell != Spell::NONE)
	{
    	    cursor.Hide();
    	    Dialog::SpellInfo(spell, !le.MousePressRight());
    	    cursor.Show();
    	    display.Flip();
	}
    }

    return 0 <= index;
}

void Castle::OpenMageGuild(void)
{
    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    Dialog::FrameBorder frameborder(Size(640, 480));
    const Point & cur_pt = frameborder.GetArea();
    Text text;

    // bar
    AGG::GetICN(ICN::WELLXTRA, 2).Blit(cur_pt.x, cur_pt.y + 461);

    // text bar
    text.Set(_("The above spells have been added to your book."), Font::BIG);
    text.Blit(cur_pt.x + 280 - text.w() / 2, cur_pt.y + 461);

    const int level = GetLevelMageGuild();
    // sprite
    int icn = ICN::UNKNOWN;
    switch(race)
    {
        case Race::KNGT: icn = ICN::MAGEGLDK; break;
        case Race::BARB: icn = ICN::MAGEGLDB; break;
        case Race::SORC: icn = ICN::MAGEGLDS; break;
        case Race::WRLK: icn = ICN::MAGEGLDW; break;
        case Race::WZRD: icn = ICN::MAGEGLDZ; break;
        case Race::NECR: icn = ICN::MAGEGLDN; break;
	default: break;
    }
    const Sprite & sprite = AGG::GetICN(icn, level - 1);
    sprite.Blit(cur_pt.x + 90 - sprite.w() / 2, cur_pt.y + 290 - sprite.h());

    RowSpells spells5(Point(cur_pt.x + 250, cur_pt.y +  5),  *this, 5);
    RowSpells spells4(Point(cur_pt.x + 250, cur_pt.y +  95), *this, 4);
    RowSpells spells3(Point(cur_pt.x + 250, cur_pt.y + 185), *this, 3);
    RowSpells spells2(Point(cur_pt.x + 250, cur_pt.y + 275), *this, 2);
    RowSpells spells1(Point(cur_pt.x + 250, cur_pt.y + 365), *this, 1);

    spells1.Redraw();
    spells2.Redraw();
    spells3.Redraw();
    spells4.Redraw();
    spells5.Redraw();

    // button exit
    Button buttonExit(cur_pt.x + 578, cur_pt.y + 461, ICN::WELLXTRA, 0, 1);
    buttonExit.Draw();

    cursor.Show();
    display.Flip();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while(le.HandleEvents())
    {
        le.MousePressLeft(buttonExit) ? buttonExit.PressDraw() : buttonExit.ReleaseDraw();

        if(le.MouseClickLeft(buttonExit) || HotKeyCloseWindow) break;

        if(spells1.QueueEventProcessing() ||
    	    spells2.QueueEventProcessing() ||
    	    spells3.QueueEventProcessing() ||
    	    spells4.QueueEventProcessing() ||
    	    spells5.QueueEventProcessing()){}
    }
}
