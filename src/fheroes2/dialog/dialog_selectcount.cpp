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

#include "agg.h"
#include "text.h"
#include "settings.h"
#include "cursor.h"
#include "button.h"
#include "pocketpc.h"
#include "game.h"
#include "dialog.h"

class SelectValue : public Rect
{
public:
    SelectValue(u32 min, u32 max, u32 cur, u32 st) : vmin(min), vmax(max), vcur(cur), step(st)
    {
	if(vmin >= vmax) vmin = 0;
	if(vcur > vmax || vcur < vmin) vcur = vmin;

	btnUp.SetSprite(ICN::TOWNWIND, 5, 6);
	btnDn.SetSprite(ICN::TOWNWIND, 7, 8);

	pos.w = 90;
	pos.h = 30;
    }

    u32 Min(void)
    {
	return vmin;
    }

    u32 Max(void)
    {
	return vmax;
    }

    void SetCur(u32 v)
    {
	vcur = v;
    }

    void SetPos(const Point & pt)
    {
	pos = pt;

	btnUp.SetPos(pt.x + 70, pt.y);
	btnDn.SetPos(pt.x + 70, pt.y + 16);
    }

    u32 operator() (void) const
    {
	return vcur;
    }

    void Redraw(void)
    {
	const Sprite & sprite_edit = AGG::GetICN(ICN::TOWNWIND, 4);
	sprite_edit.Blit(pos.x, pos.y + 4);

	Text text(GetString(vcur), Font::BIG);
	text.Blit(pos.x + (sprite_edit.w() - text.w()) / 2, pos.y + 5);

	btnUp.Draw();
	btnDn.Draw();
    }

    bool QueueEventProcessing(void)
    {
	LocalEvent & le = LocalEvent::Get();

	le.MousePressLeft(btnUp) ? btnUp.PressDraw() : btnUp.ReleaseDraw();
	le.MousePressLeft(btnDn) ? btnDn.PressDraw() : btnDn.ReleaseDraw();

	if((le.MouseWheelUp(pos) ||
            le.MouseClickLeft(btnUp)) && vcur < vmax)
	{
	    vcur += vcur + step <= vmax ? step : vmax - vcur;
    	    return true;
	}
	else
	// down
	if((le.MouseWheelDn(pos) ||
            le.MouseClickLeft(btnDn)) && vmin < vcur)
	{
	    vcur -= vmin + vcur >= step ? step : vcur;
    	    return true;
	}

	return false;
    }

protected:
    u32		vmin;
    u32		vmax;
    u32		vcur;
    u32		step;

    Rect	pos;

    Button	btnUp;
    Button	btnDn;
};

bool Dialog::SelectCount(const std::string &header, u32 min, u32 max, u32 & cur, int step)
{
    Display & display = Display::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    Text text(header, Font::BIG);
    const int spacer = Settings::Get().QVGA() ? 5 : 10;

    FrameBox box(text.h() + spacer + 30, true);
    SelectValue sel(min, max, cur, step);

    const Rect & pos = box.GetArea();

    text.Blit(pos.x + (pos.w - text.w()) / 2, pos.y);

    sel.SetPos(Point(pos.x + 80, pos.y + 30));
    sel.Redraw();

    ButtonGroups btnGroups(box.GetArea(), Dialog::OK | Dialog::CANCEL);
    btnGroups.Draw();

    text.Set("MAX", Font::SMALL);
    const Rect rectMax(pos.x + 173, pos.y + 38, text.w(), text.h());
    text.Blit(rectMax.x, rectMax.y);

    LocalEvent & le = LocalEvent::Get();

    bool redraw_count = false;
    cursor.Show();
    display.Flip();

    // message loop
    int result = Dialog::ZERO;
    while(result == Dialog::ZERO && le.HandleEvents())
    {
	if(PressIntKey(min, max, cur))
	{
	    sel.SetCur(cur);
	    redraw_count = true;
	}

        // max
        if(le.MouseClickLeft(rectMax))
        {
	    sel.SetCur(max);
    	    redraw_count = true;
        }
	if(sel.QueueEventProcessing())
    	    redraw_count = true;

	if(redraw_count)
	{
	    cursor.Hide();
	    sel.Redraw();
	    cursor.Show();
	    display.Flip();

	    redraw_count = false;
	}

        result = btnGroups.QueueEventProcessing();
    }

    cur = result == Dialog::OK ? sel() : 0;

    return result == Dialog::OK;
}

bool Dialog::InputString(const std::string & header, std::string & res)
{
    const int system = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;

    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    int oldcursor = cursor.Themes();
    cursor.SetThemes(cursor.POINTER);

    //const bool pda = Settings::Get().PocketPC();
    if(res.size()) res.clear();
    res.reserve(48);
    size_t charInsertPos = 0;

    TextBox textbox(header, Font::BIG, BOXAREA_WIDTH);
    Point dst_pt;
    const Sprite & sprite = AGG::GetICN((Settings::Get().ExtGameEvilInterface() ? ICN::BUYBUILD : ICN::BUYBUILE), 3);

    FrameBox box(10 + textbox.h() + 10 + sprite.h(), OK|CANCEL);
    const Rect & box_rt = box.GetArea();

    // text
    dst_pt.x = box_rt.x + (box_rt.w - textbox.w()) / 2;
    dst_pt.y = box_rt.y + 10;
    textbox.Blit(dst_pt);

    dst_pt.y = box_rt.y + 10 + textbox.h() + 10;
    dst_pt.x = box_rt.x + (box_rt.w - sprite.w()) / 2;
    sprite.Blit(dst_pt, display);
    const Rect text_rt(dst_pt.x, dst_pt.y, sprite.w(), sprite.h());

    Text text("_", Font::BIG);
    sprite.Blit(text_rt, display);
    text.Blit(dst_pt.x + (sprite.w() - text.w()) / 2, dst_pt.y - 1);

    dst_pt.x = box_rt.x;
    dst_pt.y = box_rt.y + box_rt.h - AGG::GetICN(system, 1).h();
    Button buttonOk(dst_pt.x, dst_pt.y, system, 1, 2);

    dst_pt.x = box_rt.x + box_rt.w - AGG::GetICN(system, 3).w();
    dst_pt.y = box_rt.y + box_rt.h - AGG::GetICN(system, 3).h();
    Button buttonCancel(dst_pt.x, dst_pt.y, system, 3, 4);

    buttonOk.SetDisable(res.empty());
    buttonOk.Draw();
    buttonCancel.Draw();

    cursor.Show();
    display.Flip();

    LocalEvent & le = LocalEvent::Get();
    bool redraw = true;

    // message loop
    while(le.HandleEvents())
    {
	buttonOk.isEnable() && le.MousePressLeft(buttonOk) ? buttonOk.PressDraw() : buttonOk.ReleaseDraw();
        le.MousePressLeft(buttonCancel) ? buttonCancel.PressDraw() : buttonCancel.ReleaseDraw();

	if(Settings::Get().PocketPC() && le.MousePressLeft(text_rt))
	{
	    PocketPC::KeyboardDialog(res);
	    redraw = true;
	}

        if(Game::HotKeyPressEvent(Game::EVENT_DEFAULT_READY) || (buttonOk.isEnable() && le.MouseClickLeft(buttonOk))) break;
	else
	if(Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT) || le.MouseClickLeft(buttonCancel)){ res.clear(); break; }
	else
	if(le.KeyPress())
	{
	    charInsertPos = InsertKeySym(res, charInsertPos, le.KeyValue(), le.KeyMod());
	    redraw = true;
	}

	if(redraw)
	{
	    buttonOk.SetDisable(res.empty());
	    buttonOk.Draw();

	    text.Set(InsertString(res, charInsertPos, "_"));

	    if(text.w() < sprite.w() - 24)
	    {
		cursor.Hide();
		sprite.Blit(text_rt, display);
		text.Blit(text_rt.x + (text_rt.w - text.w()) / 2, text_rt.y - 1);
		cursor.Show();
		display.Flip();
	    }
	    redraw = false;
	}
    }

    cursor.SetThemes(oldcursor);
    cursor.Hide();

    return res.size();
}

int Dialog::ArmySplitTroop(int free_slots, u32 max, u32 & cur, bool savelast)
{
    Display & display = Display::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    const u32 min = 1;
    const int spacer = Settings::Get().QVGA() ? 5 : 10;

    FrameBox box(free_slots > 2 ? 90 + spacer : 45, true);
    SelectValue sel(min, max, cur, 1);
    Text text;

    const Rect & pos = box.GetArea();
    const int center = pos.x + pos.w / 2;

    text.Set(_("Move how many troops?"), Font::BIG);
    text.Blit(center - text.w() / 2, pos.y);

    sel.SetPos(Point(pos.x + 70, pos.y + 30));
    sel.Redraw();

    SpriteMove ssp;
    Surface sp3, sp4, sp5;

    std::vector<Rect> vrts(3);

    Rect & rt3 = vrts[0];
    Rect & rt4 = vrts[1];
    Rect & rt5 = vrts[2];

    switch(free_slots)
    {
	case 0:
	    break;

	case 3:	
	    sp3 = AGG::GetICN(ICN::REQUESTS, 22);
	    rt3 = Rect(center - sp3.w() / 2, pos.y + 95, sp3.w(), sp3.h());
	    break;

	case 4:
	    sp3 = AGG::GetICN(ICN::REQUESTS, 22);
	    sp4 = AGG::GetICN(ICN::REQUESTS, 23);
	    rt3 = Rect(center - 5 - sp3.w(), pos.y + 95, sp3.w(), sp3.h());
	    rt4 = Rect(center + 5, pos.y + 95, sp4.w(), sp4.h());
	    break;

	case 5:
	    sp3 = AGG::GetICN(ICN::REQUESTS, 22);
	    sp4 = AGG::GetICN(ICN::REQUESTS, 23);
	    sp5 = AGG::GetICN(ICN::REQUESTS, 24);
	    rt3 = Rect(center - sp3.w() / 2 - 10 - sp3.w(), pos.y + 95, sp3.w(), sp3.h());
	    rt4 = Rect(center - sp4.w() / 2, pos.y + 95, sp4.w(), sp4.h());
	    rt5 = Rect(center + sp5.w() / 2 + 10, pos.y + 95, sp5.w(), sp5.h());
	    break;
    }

    if(sp3.isValid())
    {
	text.Set(_("Fast separation into slots:"), Font::BIG);
	text.Blit(center - text.w() / 2, pos.y + 65);

	sp3.Blit(rt3, display);
	if(sp4.isValid()) sp4.Blit(rt4, display);
	if(sp5.isValid()) sp5.Blit(rt5, display);

	ssp.Set(sp3.w(), sp3.h(), true);
	ssp.DrawBorder(RGBA(0xC0, 0x2C, 0));
    }

    ButtonGroups btnGroups(box.GetArea(), Dialog::OK | Dialog::CANCEL);
    btnGroups.Draw();

    if(savelast)
	text.Set(std::string("MAX") + " " + "(" + GetString(max) + ")", Font::SMALL);
    else
	text.Set(std::string("MAX") + " " + "(" + GetString(max) + ")" + " " + "-" + " " + "1", Font::SMALL);
    const Rect rectMax(pos.x + 163, pos.y + 30, text.w(), text.h());
    text.Blit(rectMax.x, rectMax.y);

    text.Set(std::string("MIN") + " " + "(" + GetString(min) + ")", Font::SMALL);
    const Rect rectMin(pos.x + 163, pos.y + 45, text.w(), text.h());
    text.Blit(rectMin.x, rectMin.y);

    LocalEvent & le = LocalEvent::Get();

    bool redraw_count = false;
    cursor.Show();
    display.Flip();

    // message loop
    int bres = Dialog::ZERO;
    while(bres == Dialog::ZERO && le.HandleEvents())
    {
	if(PressIntKey(min, max, cur))
	{
	    sel.SetCur(cur);
	    redraw_count = true;
	}
	else
        if(le.MouseClickLeft(rectMax))
        {
	    sel.SetCur(savelast ? max : max - 1);
    	    redraw_count = true;
        }
	else
        if(le.MouseClickLeft(rectMin))
        {
	    sel.SetCur(min);
    	    redraw_count = true;
        }
	else
	if(sel.QueueEventProcessing())
    	    redraw_count = true;

	if(ssp.isValid())
	for(std::vector<Rect>::const_iterator
	    it = vrts.begin(); it != vrts.end(); ++it)
	{
	    if(le.MouseClickLeft(*it))
	    {
		cursor.Hide();
		ssp.Move(*it);
		cursor.Show();
		display.Flip();
	    }
	}

	if(redraw_count)
	{
	    cursor.Hide();
	    if(ssp.isValid()) ssp.Hide();
	    sel.Redraw();
	    cursor.Show();
	    display.Flip();

	    redraw_count = false;
	}

        bres = btnGroups.QueueEventProcessing();
    }

    int result = 0;

    if(bres == Dialog::OK)
    {
	cur = sel();

	if(ssp.isVisible())
	{
	    const Rect & rt = ssp.GetArea();

	    if(rt == rt3)
		result = 3;
	    else
	    if(rt == rt4)
		result = 4;
	    else
	    if(rt == rt5)
		result = 5;
	}
	else
	    result = 2;
    }

    return result;
}
