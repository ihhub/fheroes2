/***************************************************************************
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "agg.h"
#include "text.h"
#include "settings.h"
#include "cursor.h"
#include "button.h"
#include "world.h"
#include "game.h"
#include "dialog.h"

s32 GetIndexClickRects(const Rects & rects)
{
    LocalEvent & le = LocalEvent::Get();
    const s32 index = rects.GetIndex(le.GetMouseCursor());
    return 0 <= index && le.MouseClickLeft() ? index : -1;
}

struct SelectRecipientsColors
{
    const Colors	colors;
    int			recipients;
    Rects		positions;

    SelectRecipientsColors(const Point & pos) :
	colors(Settings::Get().GetPlayers().GetColors() & ~Settings::Get().GetPlayers().current_color), recipients(0)
    {
	positions.reserve(colors.size());

	for(Colors::const_iterator
    	    it = colors.begin(); it != colors.end(); ++it)
	{
    	    const u32 current = std::distance(colors.begin(), it);
	    const Sprite & sprite = AGG::GetICN(ICN::CELLWIN, 43);

    	    positions.push_back(Rect(pos.x + Game::GetStep4Player(current, sprite.w() + 15, colors.size()),
									    pos.y, sprite.w(), sprite.h()));
	}
    }

    s32 GetIndexClick(void) const
    {
	return GetIndexClickRects(positions);
    }

    void Redraw(void) const
    {
	for(Colors::const_iterator
    	    it = colors.begin(); it != colors.end(); ++it)
	{
	    const Rect & pos = positions[std::distance(colors.begin(), it)];

	    AGG::GetICN(ICN::CELLWIN, 43 + Color::GetIndex(*it)).Blit(pos);
	    if(recipients & *it)
		AGG::GetICN(ICN::CELLWIN, 2).Blit(pos.x + 2, pos.y + 2);
	}
    }

    bool QueueEventProcessing(void)
    {
	const s32 index = GetIndexClick();

    	if(index >= 0)
	{
	    const int & cols = colors[index];

	    if(recipients & cols)
		recipients &= ~cols;
	    else
		recipients |= cols;

	    return true;
	}

	return false;
    }
};

struct ResourceBar
{
    Funds &	resource;
    Point	pos;
    Rects	positions;

    ResourceBar(Funds & funds, s32 posx, s32 posy) : resource(funds), pos(posx, posy)
    {
	positions.reserve(7);
	const Sprite & sprite = AGG::GetICN(ICN::TRADPOST, 7);

	positions.push_back(Rect(posx, posy, sprite.w(), sprite.h()));
	positions.push_back(Rect(posx + 40, posy, sprite.w(), sprite.h()));
	positions.push_back(Rect(posx + 80, posy, sprite.w(), sprite.h()));
	positions.push_back(Rect(posx + 120, posy, sprite.w(), sprite.h()));
	positions.push_back(Rect(posx + 160, posy, sprite.w(), sprite.h()));
	positions.push_back(Rect(posx + 200, posy, sprite.w(), sprite.h()));
	positions.push_back(Rect(posx + 240, posy, sprite.w(), sprite.h()));
    }

    static void RedrawResource(int type, s32 count, s32 posx, s32 posy)
    {
	std::ostringstream os;

	os << count;
	Text text(os.str(), Font::SMALL);
	const Sprite & sprite = AGG::GetICN(ICN::TRADPOST, 7 + Resource::GetIndexSprite2(type));
	sprite.Blit(posx, posy);
	text.Blit(posx + (sprite.w() - text.w()) / 2, posy + sprite.h() - 12);
    }

    void Redraw(const Funds* res = NULL) const
    {
	if(! res) res = &resource;

	for(Rects::const_iterator
	    it = positions.begin(); it != positions.end(); ++it)
	{
	    int rs = Resource::FromIndexSprite2(std::distance(positions.begin(), it));
	    RedrawResource(rs, res->Get(rs), (*it).x, (*it).y);
	}
    }

    s32 GetIndexClick(void) const
    {
	return GetIndexClickRects(positions);
    }

    bool QueueEventProcessing(Funds & funds, u32 mul)
    {
	const s32 index = GetIndexClick();

	if(index >= 0)
	{
	    int rs = Resource::FromIndexSprite2(index);
	    u32 step = rs == Resource::GOLD ? 100 : 1;

	    u32 cur = resource.Get(rs);
	    u32 sel = cur;
	    u32 max = mul > 1 ? (funds.Get(rs) + resource.Get(rs)) / mul : funds.Get(rs) + resource.Get(rs);

	    if(0 == mul)
	    {
		Dialog::Message("", "First select recipients!", Font::BIG, Dialog::OK);
	    }
	    else
	    if(0 == max)
	    {
		std::string msg = _("You cannot select %{resource}!");
		StringReplace(msg, "%{resource}", Resource::String(rs));
		Dialog::Message("", msg, Font::BIG, Dialog::OK);
	    }
	    else
	    {
		std::string msg = _("Select count %{resource}:");
		StringReplace(msg, "%{resource}", Resource::String(rs));

		if(Dialog::SelectCount(msg, 0, max, sel, step) && cur != sel)
		{
		    s32* from = funds.GetPtr(rs);
		    s32* to = resource.GetPtr(rs);

		    if(from && to)
		    {
			s32 count = sel - cur;

			*from -= mul > 1 ? count * mul : count;
			*to += count;

			return true;
		    }
		}
	    }
	}

	return false;
    }
};

void Dialog::MakeGiftResource(void)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();
    const Settings & conf = Settings::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder(Size(320, 224));
    const Rect & box = frameborder.GetArea();

    Kingdom & myKingdom = world.GetKingdom(conf.CurrentColor());

    Funds funds1(myKingdom.GetFunds());
    Funds funds2;
    Text text;

    text.Set("Select Recipients");
    text.Blit(box.x + (box.w - text.w()) / 2, box.y + 5);

    SelectRecipientsColors selector(Point(box.x + 65, box.y + 28));
    selector.Redraw();

    text.Set("Your Funds");
    text.Blit(box.x + (box.w - text.w()) / 2, box.y + 55);

    ResourceBar info1(funds1, box.x + 25, box.y + 80);
    info1.Redraw();

    text.Set("Planned Gift");
    text.Blit(box.x + (box.w - text.w()) / 2, box.y + 125);

    ResourceBar info2(funds2, box.x + 25, box.y + 150);
    info2.Redraw();


    ButtonGroups btnGroups(box, Dialog::OK|Dialog::CANCEL);
    btnGroups.DisableButton1(true);
    btnGroups.Draw();


    cursor.Show();
    display.Flip();

    u32 count = Color::Count(selector.recipients);

    // message loop
    int result = Dialog::ZERO;
    while(result == Dialog::ZERO && le.HandleEvents())
    {
	if(selector.QueueEventProcessing())
	{
	    u32 new_count = Color::Count(selector.recipients);
	    cursor.Hide();
	    btnGroups.DisableButton1(0 == new_count || 0 == funds2.GetValidItemsCount());
	    if(count != new_count)
	    {
		funds1 = myKingdom.GetFunds();
		funds2.Reset();
		info1.Redraw();
		info2.Redraw();
		count = new_count;
	    }
	    btnGroups.Draw();
	    selector.Redraw();
	    cursor.Show();
	    display.Flip();
	}
	else
	if(info2.QueueEventProcessing(funds1, count))
	{
	    cursor.Hide();
	    btnGroups.DisableButton1(0 == Color::Count(selector.recipients) || 0 == funds2.GetValidItemsCount());
	    info1.Redraw();
	    info2.Redraw();
	    btnGroups.Draw();
	    cursor.Show();
	    display.Flip();
	}

        result = btnGroups.QueueEventProcessing();
    }

    if(Dialog::OK == result)
    {
	EventDate event;

	event.resource = funds2;
	event.computer = true;
	event.first = world.CountDay() + 1;
	event.subsequent = 0;
	event.colors = selector.recipients;
	event.message = "Gift from %{name}";
	const Player* player = Settings::Get().GetPlayers().GetCurrent();
	if(player)
	    StringReplace(event.message, "%{name}", player->GetName());

	world.AddEventDate(event);

	if(1 < count) funds2 *= count;
	myKingdom.OddFundsResource(funds2);
    }
}
