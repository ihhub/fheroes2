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
#include "payment.h"
#include "world.h"
#include "button.h"
#include "kingdom.h"
#include "monster.h"
#include "game.h"
#include "dialog.h"

void RedrawCurrentInfo(const Point & pos, u32 available, u32 result,
	    const payment_t & paymentMonster, const payment_t & paymentCosts, const Funds & funds, const std::string & label)
{
    Text text;

    std::string str = _("Available: %{count}");
    StringReplace(str, "%{count}", available);
    text.Set(str, Font::SMALL);
    text.Blit(pos.x + 70 - text.w() / 2, pos.y + 130);
    text.Set(GetString(result), Font::BIG);
    text.Blit(pos.x + 167 - text.w() / 2, pos.y + 160);
    const std::string sgold = GetString(paymentCosts.gold) + " " + "(" + GetString(funds.gold - paymentCosts.gold) + ")";
    int rsext = paymentMonster.GetValidItems() & ~Resource::GOLD;

    if(rsext)
    {
	text.Set(sgold, Font::SMALL);
	text.Blit(pos.x + 133 - text.w() / 2, pos.y + 228);

	text.Set(GetString(paymentCosts.Get(rsext)) + " " + "(" + GetString(funds.Get(rsext) - paymentCosts.Get(rsext)) + ")", Font::SMALL);
	text.Blit(pos.x + 195 - text.w() / 2, pos.y + 228);
    }
    else
    {
	text.Set(sgold, Font::SMALL);
	text.Blit(pos.x + 160 - text.w() / 2, pos.y + 228);
    }

    text.Set(label, Font::SMALL);
    text.Blit(pos.x + 165 - text.w() / 2, pos.y + 180);
}

void RedrawResourceInfo(const Surface & sres, const Point & pos, s32 value,
	s32 px1, s32 py1, s32 px2, s32 py2)
{
    Display & display = Display::Get();
    Point dst_pt;

    dst_pt.x = pos.x + px1;
    dst_pt.y = pos.y + py1;
    sres.Blit(dst_pt, display);

    Text text(GetString(value), Font::SMALL);
    dst_pt.x = pos.x + px2 - text.w() / 2;
    dst_pt.y = pos.y + py2;
    text.Blit(dst_pt);
}

void RedrawStaticInfo(const Rect & pos, const Monster & monster, bool label)
{
    Text text;
    Point dst_pt;
    std::string str;

    const Sprite & box = AGG::GetICN(ICN::RECRBKG, 0);
    box.Blit(pos.x, pos.y);

    payment_t paymentMonster = monster.GetCost();
    bool extres = 2 == paymentMonster.GetValidItemsCount();

    // smear hardcore text "Cost per troop:"
    const Sprite & smear = AGG::GetICN(ICN::TOWNNAME, 0);
    dst_pt.x = pos.x + 144;
    dst_pt.y = pos.y + 55;
    smear.Blit(Rect(8, 1, 120, 12), dst_pt);

    text.Set(_("Cost per troop:"), Font::SMALL);
    dst_pt.x = pos.x + 206 - text.w() / 2;
    dst_pt.y = pos.y + 55;
    text.Blit(dst_pt);

    // text recruit monster
    str = _("Recruit %{name}");
    StringReplace(str, "%{name}", monster.GetMultiName());
    text.Set(str, Font::BIG);
    dst_pt.x = pos.x + (pos.w - text.w()) / 2;
    dst_pt.y = pos.y + 25;
    text.Blit(dst_pt);

    // sprite monster
    const Sprite & smon = AGG::GetICN(monster.ICNMonh(), 0);
    dst_pt.x = pos.x + 70 - smon.w() / 2;
    dst_pt.y = pos.y + 130 - smon.h();
    smon.Blit(dst_pt);

    // change label
    if(label)
    {
	text.Set("( change )", Font::YELLOW_SMALL);
	text.Blit(pos.x + 68 - text.w() / 2, pos.y + 80);
    }

    // info resource
    // gold
    const Sprite & sgold = AGG::GetICN(ICN::RESOURCE, 6);
    dst_pt.x = pos.x + (extres ? 150 : 175);
    dst_pt.y = pos.y + 75;
    sgold.Blit(dst_pt);

    dst_pt.x = pos.x + (extres ? 105 : 130);
    dst_pt.y = pos.y + 200;
    sgold.Blit(dst_pt);

    text.Set(GetString(paymentMonster.gold), Font::SMALL);
    dst_pt.x = pos.x + (extres ? 183 : 205) - text.w() / 2;
    dst_pt.y = pos.y + 103;
    text.Blit(dst_pt);

    // crystal
    if(paymentMonster.crystal)
    {
        const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 4);
	RedrawResourceInfo(sres, pos, paymentMonster.crystal,
				225, 75, 240, 103);
	dst_pt.x = pos.x + 180;
	dst_pt.y = pos.y + 200;
	sres.Blit(dst_pt);
    }
    else
    // mercury
    if(paymentMonster.mercury)
    {
        const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 1);
	RedrawResourceInfo(sres, pos, paymentMonster.mercury,
				225, 72, 240, 103);
	dst_pt.x = pos.x + 180;
	dst_pt.y = pos.y + 197;
	sres.Blit(dst_pt);
    }
    else
    // wood
    if(paymentMonster.wood)
    {
        const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 0);
	RedrawResourceInfo(sres, pos, paymentMonster.wood,
				225, 72, 240, 103);
	dst_pt.x = pos.x + 180;
	dst_pt.y = pos.y + 197;
	sres.Blit(dst_pt);
    }
    else
    // ore
    if(paymentMonster.ore)
    {
        const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 2);
	RedrawResourceInfo(sres, pos, paymentMonster.ore,
				225, 72, 240, 103);
	dst_pt.x = pos.x + 180;
	dst_pt.y = pos.y + 197;
	sres.Blit(dst_pt);
    }
    else
    // sulfur
    if(paymentMonster.sulfur)
    {
        const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 3);
	RedrawResourceInfo(sres, pos, paymentMonster.sulfur,
				225, 75, 240, 103);
	dst_pt.x = pos.x + 180;
	dst_pt.y = pos.y + 200;
	sres.Blit(dst_pt);
    }
    else
    // gems
    if(paymentMonster.gems)
    {
        const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 5);
	RedrawResourceInfo(sres, pos, paymentMonster.gems,
				225, 75, 240, 103);
	dst_pt.x = pos.x + 180;
	dst_pt.y = pos.y + 200;
	sres.Blit(dst_pt);
    }

    // text number buy
    text.Set(_("Number to buy:"));
    dst_pt.x = pos.x + 30;
    dst_pt.y = pos.y + 163;
    text.Blit(dst_pt);

}

const char* SwitchMaxMinButtons(Button & btnMax, Button & btnMin, bool max)
{
    if(btnMax.isEnable() || btnMin.isEnable())
    {
	if(max)
	{
	    btnMax.SetDisable(true);
	    btnMin.SetDisable(false);
	}
	else
	{
	    btnMin.SetDisable(true);
	    btnMax.SetDisable(false);
	}

	return max ? "max" : "min";
    }

    return "";
}

u32 CalculateMax(const Monster & monster, const Kingdom & kingdom, u32 available)
{
    u32 max = 0;
    while(kingdom.AllowPayment(monster.GetCost() * (max + 1)) && (max + 1) <= available) ++max;

    return max;
}

Troop Dialog::RecruitMonster(const Monster & monster0, u32 available, bool ext)
{
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    const int oldcursor = cursor.Themes();
    cursor.Hide();
    cursor.SetThemes(Cursor::POINTER);

    // calculate max count
    Monster monster = monster0;
    payment_t paymentMonster = monster.GetCost();
    const Kingdom & kingdom = world.GetKingdom(Settings::Get().CurrentColor());

    u32 max = CalculateMax(monster, kingdom, available);
    u32 result = max;

    payment_t paymentCosts(paymentMonster * result);

    const Sprite & box = AGG::GetICN(ICN::RECRBKG, 0);

    SpriteBack back(Rect((display.w() - box.w()) / 2, (display.h() - box.h()) / 2 - (Settings::Get().QVGA() ?  15 : 65), box.w(), box.h()));
    const Rect & pos = back.GetArea();

    const Rect rtChange(pos.x + 25, pos.y + 35, 85, 95);
    RedrawStaticInfo(pos, monster, ext && monster0.GetDowngrade() != monster0);

    // buttons
    Point dst_pt;

    dst_pt.x = pos.x + 34;
    dst_pt.y = pos.y + 249;
    Button buttonOk(dst_pt.x, dst_pt.y, ICN::RECRUIT, 8, 9);

    dst_pt.x = pos.x + 187;
    dst_pt.y = pos.y + 249;
    Button buttonCancel(dst_pt.x, dst_pt.y, ICN::RECRUIT, 6, 7);

    dst_pt.x = pos.x + 230;
    dst_pt.y = pos.y + 155;
    Button buttonMax(dst_pt.x, dst_pt.y, ICN::RECRUIT, 4, 5);
    Button buttonMin(dst_pt.x, dst_pt.y, ICN::BTNMIN, 0, 1);

    dst_pt.x = pos.x + 208;
    dst_pt.y = pos.y + 156;
    Button buttonUp(dst_pt.x, dst_pt.y, ICN::RECRUIT, 0, 1);

    dst_pt.x = pos.x + 208;
    dst_pt.y = pos.y + 171;
    Button buttonDn(dst_pt.x, dst_pt.y, ICN::RECRUIT, 2, 3);

    const Rect rtWheel(pos.x + 130, pos.y +155, 100, 30);

    if(0 == result)
    {
	buttonOk.Press();
	buttonOk.SetDisable(true);
	buttonMax.Press();
	buttonMin.Press();
	buttonMax.SetDisable(true);
	buttonMin.SetDisable(true);
	buttonMax.Draw();
    }

    const Funds & funds = kingdom.GetFunds();
    std::string maxmin = SwitchMaxMinButtons(buttonMax, buttonMin, true);
    RedrawCurrentInfo(pos, available, result, paymentMonster, paymentCosts, funds, maxmin);

    buttonOk.Draw();
    buttonCancel.Draw();
    if(buttonMax.isEnable()) buttonMax.Draw();
    if(buttonMin.isEnable()) buttonMin.Draw();
    buttonUp.Draw();
    buttonDn.Draw();

    cursor.Show();
    display.Flip();

    bool redraw = false;

    // str loop
    while(le.HandleEvents())
    {
	if(buttonOk.isEnable())
	    le.MousePressLeft(buttonOk) ? buttonOk.PressDraw() : buttonOk.ReleaseDraw();
	le.MousePressLeft(buttonCancel) ? buttonCancel.PressDraw() : buttonCancel.ReleaseDraw();
	le.MousePressLeft(buttonUp) ? buttonUp.PressDraw() : buttonUp.ReleaseDraw();
	le.MousePressLeft(buttonDn) ? buttonDn.PressDraw() : buttonDn.ReleaseDraw();

	if(buttonMax.isEnable())
	    le.MousePressLeft(buttonMax) ? buttonMax.PressDraw() : buttonMax.ReleaseDraw();
	if(buttonMin.isEnable())
	    le.MousePressLeft(buttonMin) ? buttonMin.PressDraw() : buttonMin.ReleaseDraw();

	if(ext && le.MouseClickLeft(rtChange))
	{
	    if(monster != monster.GetDowngrade())
	    {
		monster = monster.GetDowngrade();
		max = CalculateMax(monster, kingdom, available);
		result = max;
		paymentMonster = monster.GetCost();
		paymentCosts = paymentMonster * result;
		redraw = true;
	    }
	    else
	    if(monster != monster0)
	    {
		monster = monster0;
		max = CalculateMax(monster, kingdom, available);
		result = max;
		paymentMonster = monster.GetCost();
		paymentCosts = paymentMonster * result;
		redraw = true;
	    }

	    if(result == max)
	    {
		maxmin = SwitchMaxMinButtons(buttonMax, buttonMin, true);
	    }
	}

	if(PressIntKey(0, max, result))
	{
	    paymentCosts = paymentMonster * result;
	    redraw = true;
	    maxmin.clear();

	    if(result == max)
	    {
		maxmin = SwitchMaxMinButtons(buttonMax, buttonMin, true);
	    }
	    else
	    if(result == 1)
	    {
		maxmin = SwitchMaxMinButtons(buttonMax, buttonMin, false);
	    }
	}

	if((le.MouseWheelUp(rtWheel) || le.MouseClickLeft(buttonUp)) && result < max)
	{
	    ++result;
	    paymentCosts += paymentMonster;
	    redraw = true;
	    maxmin.clear();

	    if(result == max)
	    {
		maxmin = SwitchMaxMinButtons(buttonMax, buttonMin, true);
	    }
	    else
	    if(result == 1)
	    {
		maxmin = SwitchMaxMinButtons(buttonMax, buttonMin, false);
	    }
	}
	else
	if((le.MouseWheelDn(rtWheel) || le.MouseClickLeft(buttonDn)) && result)
	{
	    --result;
	    paymentCosts -= paymentMonster;
	    redraw = true;
	    maxmin.clear();

	    if(result == max)
	    {
		maxmin = SwitchMaxMinButtons(buttonMax, buttonMin, true);
	    }
	    else
	    if(result == 1)
	    {
		maxmin = SwitchMaxMinButtons(buttonMax, buttonMin, false);
	    }
	}
	else
	if(buttonMax.isEnable() && le.MouseClickLeft(buttonMax) && result != max)
	{
	    maxmin = SwitchMaxMinButtons(buttonMax, buttonMin, true);
	    result = max;
	    paymentCosts = paymentMonster * max;
	    redraw = true;
	}
	else
	if(buttonMin.isEnable() && le.MouseClickLeft(buttonMin) && result != 1)
	{
	    maxmin = SwitchMaxMinButtons(buttonMax, buttonMin, false);
	    result = 1;
	    paymentCosts = paymentMonster;
	    redraw = true;
	}

	if(redraw)
	{
	    cursor.Hide();
	    RedrawStaticInfo(pos, monster, ext && monster0.GetDowngrade() != monster0);
	    RedrawCurrentInfo(pos, available, result, paymentMonster, paymentCosts, funds, maxmin);

	    if(0 == result)
	    {
		buttonOk.Press();
		buttonOk.SetDisable(true);
		buttonOk.Draw();
	    }
	    else
	    {
		buttonOk.Release();
		buttonOk.SetDisable(false);
		buttonOk.Draw();
	    }

	    if(buttonMax.isEnable()) buttonMax.Draw();
	    if(buttonMin.isEnable()) buttonMin.Draw();
	    cursor.Show();
	    display.Flip();
	    redraw = false;
	}

	if(le.MouseClickLeft(buttonOk) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_READY)) break;

	if(le.MouseClickLeft(buttonCancel) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)){ result = 0; break; }
    }

    cursor.Hide();

    back.Restore();
    cursor.SetThemes(oldcursor);

    cursor.Show();
    display.Flip();

    return Troop(monster, result);
}

void Dialog::DwellingInfo(const Monster & monster, u32 available)
{
    Display & display = Display::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    const int oldcursor = cursor.Themes();
    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    const payment_t paymentMonster = monster.GetCost();
    const Sprite & box = AGG::GetICN(ICN::RECR2BKG, 0);

    SpriteBack back(Rect((display.w() - box.w()) / 2, (display.h() - box.h()) / 2, box.w(), box.h()));
    const Rect & pos = back.GetArea();

    box.Blit(pos.x, pos.y);

    LocalEvent & le = LocalEvent::Get();

    Point dst_pt;
    Text text;
    std::string str;

    // text recruit monster
    str = _("Recruit %{name}");
    StringReplace(str, "%{name}", monster.GetMultiName());
    text.Set(str, Font::BIG);
    text.Blit(pos.x + (pos.w - text.w()) / 2, pos.y + 25);

    // sprite monster
    const Sprite & smon = AGG::GetICN(monster.ICNMonh(), 0);
    dst_pt.x = pos.x + 70 - smon.w() / 2;
    dst_pt.y = pos.y + 120 - smon.h();
    smon.Blit(dst_pt);

    bool extres = 2 == paymentMonster.GetValidItemsCount();

    // info resource
    // gold
    const Sprite & sgold = AGG::GetICN(ICN::RESOURCE, 6);
    dst_pt.x = pos.x + (extres ? 150 : 175);
    dst_pt.y = pos.y + 75;
    sgold.Blit(dst_pt);

    text.Set(GetString(paymentMonster.gold), Font::SMALL);
    dst_pt.x = pos.x + (extres ? 183 : 205) - text.w() / 2;
    dst_pt.y = pos.y + 103;
    text.Blit(dst_pt);
    // crystal
    if(paymentMonster.crystal)
    {
	const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 4);
	RedrawResourceInfo(sres, pos, paymentMonster.crystal,
				225, 75, 240, 103);
    }
    else
    // mercury
    if(paymentMonster.mercury)
    {
	const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 1);
	RedrawResourceInfo(sres, pos, paymentMonster.mercury,
				225, 72, 240, 103);
    }
    else
    // wood
    if(paymentMonster.wood)
    {
	const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 0);
	RedrawResourceInfo(sres, pos, paymentMonster.wood,
				225, 72, 240, 103);
    }
    else
    // ore
    if(paymentMonster.ore)
    {
	const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 2);
	RedrawResourceInfo(sres, pos, paymentMonster.ore,
				225, 72, 240, 103);
    }
    else
    // sulfur
    if(paymentMonster.sulfur)
    {
	const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 3);
	RedrawResourceInfo(sres, pos, paymentMonster.sulfur,
				225, 75, 240, 103);
    }
    else
    // gems
    if(paymentMonster.gems)
    {
	const Sprite & sres = AGG::GetICN(ICN::RESOURCE, 5);
	RedrawResourceInfo(sres, pos, paymentMonster.gems,
				225, 75, 240, 103);
    }

    // text available
    str = _("Available: %{count}");
    StringReplace(str, "%{count}", available);
    text.Set(str);
    text.Blit(pos.x + 70 - text.w() / 2, pos.y + 130);

    cursor.Show();
    display.Flip();

    //
    while(le.HandleEvents() && le.MousePressRight());

    cursor.Hide();

    back.Restore();
    cursor.SetThemes(oldcursor);

    cursor.Show();
    display.Flip();
}
