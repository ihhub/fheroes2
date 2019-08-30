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
#include <functional>
#include "agg.h"
#include "text.h"
#include "game.h"
#include "cursor.h"
#include "dialog.h"
#include "heroes_base.h"
#include "skill.h"
#include "dialog_selectitems.h"
#include "settings.h"
#include "spell_book.h"

#define SPELL_PER_PAGE		6
#define SPELL_PER_PAGE_SMALL	2

struct SpellFiltered : std::binary_function<Spell, int, bool>
{
    bool operator() (Spell s, int f) const
    {
	return ((SpellBook::ADVN & f) && s.isCombat()) || ((SpellBook::CMBT & f) && !s.isCombat());
    }
};

void SpellBookRedrawLists(const SpellStorage &, Rects &, size_t, const Point &, u32, int only, const HeroBase & hero);
void SpellBookRedrawSpells(const SpellStorage &, Rects &, size_t, s32, s32, const HeroBase & hero);
void SpellBookRedrawMP(const Point &, u32);

bool SpellBookSortingSpell(const Spell & spell1, const Spell & spell2)
{
    return ((spell1.isCombat() != spell2.isCombat() && spell1.isCombat()) ||
	    (std::string(spell1.GetName()) < std::string(spell2.GetName())));
}

Spell SpellBook::Open(const HeroBase & hero, int filt, bool canselect) const
{
    if(!hero.HaveSpellBook())
    {
	Dialog::Message("", _("No spell to cast."), Font::BIG, Dialog::OK);
	return Spell(Spell::NONE);
    }

    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    bool small = Settings::Get().QVGA();

    const int oldcursor = cursor.Themes();

    const Sprite & r_list = AGG::GetICN(ICN::BOOK, 0);
    const Sprite & l_list = AGG::GetICN(ICN::BOOK, 0, true);

    int filter = filt;
    SpellStorage spells2 = SetFilter(filter, &hero);

    if(canselect && spells2.empty())
    {
	Dialog::Message("", _("No spell to cast."), Font::BIG, Dialog::OK);
	return Spell::NONE;
    }

    // sorting results
    std::sort(spells2.begin(), spells2.end(), SpellBookSortingSpell);

    size_t current_index = 0;

    cursor.Hide();
    cursor.SetThemes(Cursor::POINTER);

    const Sprite & bookmark_info = AGG::GetICN(ICN::BOOK, 6);
    const Sprite & bookmark_advn = AGG::GetICN(ICN::BOOK, 3);
    const Sprite & bookmark_cmbt = AGG::GetICN(ICN::BOOK, 4);
    const Sprite & bookmark_clos = AGG::GetICN(ICN::BOOK, 5);

    const Rect pos((display.w() - (r_list.w() + l_list.w())) / 2, (display.h() - r_list.h()) / 2, r_list.w() + l_list.w(), r_list.h() + 70);
    SpriteBack back(pos);

    const Rect prev_list(pos.x + (small ? 15 : 30), pos.y + (small ? 4 : 8), (small ? 15 : 30), (small ? 12 : 25));
    const Rect next_list(pos.x + (small ? 205 : 410), pos.y + (small ? 4: 8), (small ? 15 : 30), (small ? 12 :25));

    const Rect info_rt(pos.x + (small ? 64 : 125), pos.y + (small ? 137: 275), bookmark_info.w(), bookmark_info.h());
    const Rect advn_rt(pos.x + (small ? 135: 270), pos.y + (small ? 135: 270), bookmark_advn.w(), bookmark_advn.h());
    const Rect cmbt_rt(pos.x + (small ? 152: 304), pos.y + (small ? 138: 278), bookmark_cmbt.w(), bookmark_cmbt.h());
    const Rect clos_rt(pos.x + (small ? 210: 420), pos.y + (small ? 142: 284), bookmark_clos.w(), bookmark_clos.h());

    Spell curspell(Spell::NONE);

    Rects coords;
    coords.reserve(small ? SPELL_PER_PAGE_SMALL * 2 : SPELL_PER_PAGE * 2);

    SpellBookRedrawLists(spells2, coords, current_index, pos, hero.GetSpellPoints(), filt, hero);
    bool redraw = false;

    cursor.Show();
    display.Flip();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while(le.HandleEvents())
    {
	if(le.MouseClickLeft(prev_list) && current_index)
	{
	    current_index -= small ? SPELL_PER_PAGE_SMALL * 2 : SPELL_PER_PAGE * 2;
	    redraw = true;
	}
	else
	if(le.MouseClickLeft(next_list) && spells2.size() > (current_index + (small ? SPELL_PER_PAGE_SMALL * 2 : SPELL_PER_PAGE * 2)))
	{
	    current_index += small ? SPELL_PER_PAGE_SMALL * 2 : SPELL_PER_PAGE * 2;
	    redraw = true;
	}
	else
	if((le.MouseClickLeft(info_rt)) ||
	   (le.MousePressRight(info_rt)))
	{
	    std::string str = _("Your hero has %{point} spell points remaining");
	    StringReplace(str, "%{point}", hero.GetSpellPoints());
	    cursor.Hide();
	    Dialog::Message("", str, Font::BIG, Dialog::OK);
	    cursor.Show();
	    display.Flip();
	}
	else
	if(le.MouseClickLeft(advn_rt) && filter != ADVN && filt != CMBT)
	{
	    filter = ADVN;
	    current_index = 0;
	    spells2 = SetFilter(filter, &hero);
	    redraw = true;
	}
	else
	if(le.MouseClickLeft(cmbt_rt) && filter != CMBT && filt != ADVN)
	{
	    filter = CMBT;
	    current_index = 0;
	    spells2 = SetFilter(filter, &hero);
	    redraw = true;
	}
	else
	if(le.MouseClickLeft(clos_rt) ||
		Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) break;
	else
	if(le.MouseClickLeft(pos))
	{
	    const s32 index = coords.GetIndex(le.GetMouseCursor());

	    if(0 <= index)
	    {
		SpellStorage::const_iterator spell = spells2.begin() + (index + current_index);

		if(spell < spells2.end())
		{
		    if(canselect)
		    {
			std::string str;
			if(hero.CanCastSpell(*spell, &str))
			{
			    curspell = *spell;
			    break;
			}
			else
			{
			    cursor.Hide();
			    StringReplace(str, "%{mana}", (*spell).SpellPoint(&hero));
			    StringReplace(str, "%{point}", hero.GetSpellPoints());
			    Dialog::Message("", str, Font::BIG, Dialog::OK);
			    cursor.Show();
			    display.Flip();
			}
		    }
		    else
		    {
			cursor.Hide();
			Dialog::SpellInfo(*spell, true);
			cursor.Show();
			display.Flip();
		    }
		}
	    }
	}

	if(le.MousePressRight(pos))
	{
	    const s32 index = coords.GetIndex(le.GetMouseCursor());

	    if(0 <= index)
	    {
		SpellStorage::const_iterator spell = spells2.begin() + (index + current_index);
		if(spell < spells2.end())
		{
		    cursor.Hide();
		    Dialog::SpellInfo(*spell, false);
		    cursor.Show();
		    display.Flip();
		}
	    }
	}

	if(redraw)
	{
	    cursor.Hide();
	    SpellBookRedrawLists(spells2, coords, current_index, pos, hero.GetSpellPoints(), filt, hero);
	    cursor.Show();
	    display.Flip();
	    redraw = false;
	}
    }

    cursor.Hide();
    back.Restore();
    cursor.SetThemes(oldcursor);
    cursor.Show();
    display.Flip();

    return curspell;
}

void SpellBook::Edit(const HeroBase & hero)
{
    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();

    const int oldcursor = cursor.Themes();

    const Sprite & r_list = AGG::GetICN(ICN::BOOK, 0);
    const Sprite & l_list = AGG::GetICN(ICN::BOOK, 0, true);

    size_t current_index = 0;
    SpellStorage spells2 = SetFilter(SpellBook::ALL, &hero);

    cursor.Hide();
    cursor.SetThemes(Cursor::POINTER);

    const Sprite & bookmark_clos = AGG::GetICN(ICN::BOOK, 5);

    const Rect pos((display.w() - (r_list.w() + l_list.w())) / 2, (display.h() - r_list.h()) / 2, r_list.w() + l_list.w(), r_list.h() + 70);
    SpriteBack back(pos);

    const Rect prev_list(pos.x + 30, pos.y + 8, 30, 25);
    const Rect next_list(pos.x + 410, pos.y + 8, 30, 25);
    const Rect clos_rt(pos.x + 420, pos.y + 284, bookmark_clos.w(), bookmark_clos.h());

    Rects coords;
    coords.reserve(SPELL_PER_PAGE * 2);

    SpellBookRedrawLists(spells2, coords, current_index, pos, hero.GetSpellPoints(), SpellBook::ALL, hero);
    bool redraw = false;

    cursor.Show();
    display.Flip();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while(le.HandleEvents())
    {
	if(le.MouseClickLeft(prev_list) && current_index)
	{
	    current_index -= SPELL_PER_PAGE * 2;
	    redraw = true;
	}
	else
	if(le.MouseClickLeft(next_list) && size() > (current_index + SPELL_PER_PAGE * 2))
	{
	    current_index += SPELL_PER_PAGE * 2;
	    redraw = true;
	}
	else
	if(le.MouseClickLeft(clos_rt) ||
		Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) break;
	else
	if(le.MouseClickLeft(pos))
	{
	    const s32 index = coords.GetIndex(le.GetMouseCursor());

	    if(0 <= index)
	    {
		SpellStorage::const_iterator spell = spells2.begin() + (index + current_index);

		if(spell < spells2.end())
		{
		    Dialog::SpellInfo(*spell, true);
		    redraw = true;
		}
	    }
	    else
	    {
		Spell spell = Dialog::SelectSpell();
		spells2.Append(spell);
		Append(spell);
		redraw = true;
	    }
	}

	if(le.MousePressRight(pos))
	{
	    const s32 index = coords.GetIndex(le.GetMouseCursor());

	    if(0 <= index)
	    {
		SpellStorage::const_iterator spell = spells2.begin() + (index + current_index);

		if(spell < spells2.end())
		{
		    Dialog::SpellInfo(*spell, false);
		    redraw = true;
		}
	    }
	}

	if(redraw)
	{
	    cursor.Hide();
	    SpellBookRedrawLists(spells2, coords, current_index, pos, hero.GetSpellPoints(), SpellBook::ALL, hero);
	    cursor.Show();
	    display.Flip();
	    redraw = false;
	}
    }

    cursor.Hide();
    back.Restore();
    cursor.SetThemes(oldcursor);
    cursor.Show();
    display.Flip();
}

SpellStorage SpellBook::SetFilter(int filter, const HeroBase* hero) const
{
    SpellStorage res(*this);

    // added heroes spell scrolls
    if(hero) res.Append(hero->GetBagArtifacts());

    if(filter != SpellBook::ALL)
    {
	res.resize(std::distance(res.begin(),
		    std::remove_if(res.begin(), res.end(), std::bind2nd(SpellFiltered(), filter))));
    }

    // check on water: disable portal spells
    if(hero && hero->Modes(Heroes::SHIPMASTER))
    {
	SpellStorage::iterator itend = res.end();
	itend = std::remove(res.begin(), itend, Spell(Spell::TOWNGATE));
	itend = std::remove(res.begin(), itend, Spell(Spell::TOWNPORTAL));
	if(res.end() != itend)
	    res.resize(std::distance(res.begin(), itend));
    }

    return res;
}

void SpellBookRedrawMP(const Point & dst, u32 mp)
{
    bool small = Settings::Get().QVGA();

    Point tp(dst.x + (small ? 5 : 11), dst.y + (small ? 1 : 9));
    if(0 == mp)
    {
	Text text("0", Font::SMALL);
	text.Blit(tp.x - text.w() / 2, tp.y);
    }
    else
    for(u32 i = 100; i >= 1; i /= 10) if(mp >= i)
    {
	Text text(GetString((mp % (i * 10)) / i), Font::SMALL);
	text.Blit(tp.x - text.w() / 2, tp.y);
	tp.y += (small ? -2 : 0) + text.h();
    }
}

void SpellBookRedrawLists(const SpellStorage & spells, Rects & coords, const size_t cur, const Point & pt, u32 sp, int only, const HeroBase & hero)
{
    bool small = Settings::Get().QVGA();

    const Sprite & r_list = AGG::GetICN(ICN::BOOK, 0);
    const Sprite & l_list = AGG::GetICN(ICN::BOOK, 0, true);
    const Sprite & bookmark_info = AGG::GetICN(ICN::BOOK, 6);
    const Sprite & bookmark_advn = AGG::GetICN(ICN::BOOK, 3);
    const Sprite & bookmark_cmbt = AGG::GetICN(ICN::BOOK, 4);
    const Sprite & bookmark_clos = AGG::GetICN(ICN::BOOK, 5);

    const Rect info_rt(pt.x + (small ? 64 : 125), pt.y + (small ? 137: 275), bookmark_info.w(), bookmark_info.h());
    const Rect advn_rt(pt.x + (small ? 135: 270), pt.y + (small ? 135: 270), bookmark_advn.w(), bookmark_advn.h());
    const Rect cmbt_rt(pt.x + (small ? 152: 304), pt.y + (small ? 138: 278), bookmark_cmbt.w(), bookmark_cmbt.h());
    const Rect clos_rt(pt.x + (small ? 210: 420), pt.y + (small ? 142: 284), bookmark_clos.w(), bookmark_clos.h());

    l_list.Blit(pt.x, pt.y);
    r_list.Blit(pt.x + l_list.w(), pt.y);
    bookmark_info.Blit(info_rt);
    if(SpellBook::CMBT != only)
	bookmark_advn.Blit(advn_rt);
    if(SpellBook::ADVN != only)
	bookmark_cmbt.Blit(cmbt_rt);
    bookmark_clos.Blit(clos_rt);

    if(coords.size()) coords.clear();

    SpellBookRedrawMP(info_rt, sp);
    SpellBookRedrawSpells(spells, coords, cur, pt.x, pt.y, hero);
    SpellBookRedrawSpells(spells, coords, cur + (small ? SPELL_PER_PAGE_SMALL : SPELL_PER_PAGE), pt.x + (small ? 110 : 220), pt.y, hero);
}

void SpellBookRedrawSpells(const SpellStorage & spells, Rects & coords, const size_t cur, s32 px, s32 py, const HeroBase & hero)
{
    bool small = Settings::Get().QVGA();

    s32 ox = 0;
    s32 oy = 0;

    for(u32 ii = 0; ii < (small ? SPELL_PER_PAGE_SMALL : SPELL_PER_PAGE); ++ii) if(spells.size() > cur + ii)
    {
	if(small)
	{
	    if(0 == (ii % SPELL_PER_PAGE_SMALL))
	    {
		oy = 25;
		ox = 60;
	    }
	}
	else
	{
	    if(0 == (ii % (SPELL_PER_PAGE / 2)))
	    {
		oy = 50;
		ox += 80;
	    }
	}

	const Spell & spell = spells[ii + cur];
	const Sprite & icon = AGG::GetICN(ICN::SPELLS, spell.IndexSprite());
	const Rect rect(px + ox - icon.w() / 2, py + oy - icon.h() / 2, icon.w(), icon.h() + 10);

    	icon.Blit(rect.x, rect.y);

	// multiple icons for mass spells
	if(!small)
	switch(spell())
	{
	    case Spell::MASSBLESS:
            case Spell::MASSCURE:
            case Spell::MASSHASTE:
            case Spell::MASSSLOW:
            case Spell::MASSCURSE:
            case Spell::MASSDISPEL:
            case Spell::MASSSHIELD:
    		icon.Blit(rect.x - 10, rect.y + 8);
    		icon.Blit(rect.x + 10, rect.y + 8);
		break;

	    default: break;
	}

	TextBox box(std::string(spell.GetName()) + " [" + GetString(spell.SpellPoint(&hero)) + "]", Font::SMALL, (small ? 94 : 80));
	box.Blit(px + ox - (small ? 47 : 40), py + oy + (small ? 22 : 25));

    	oy += small ? 65 : 80;

	coords.push_back(rect);
    }
}
