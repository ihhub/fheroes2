/*********[6~******************************************************************
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
#include <vector>
#include "agg.h"
#include "text.h"
#include "button.h"
#include "world.h"
#include "cursor.h"
#include "settings.h"
#include "resource.h"
#include "kingdom.h"
#include "splitter.h"
#include "game.h"
#include "dialog.h"
#include "marketplace.h"

void RedrawFromResource(const Point &, const Funds &);
void RedrawToResource(const Point &, bool showcost, bool tradingPost, int from_resource = 0);
std::string GetStringTradeCosts(int rs_from, int rs_to, bool tradingPost);
u32 GetTradeCosts(int rs_from, int rs_to, bool tradingPost);

class TradeWindowGUI
{
public:
    TradeWindowGUI(const Rect & rt) :
	pos_rt(rt),
	tradpost(Settings::Get().ExtGameEvilInterface() ? ICN::TRADPOSE : ICN::TRADPOST)
    {
	back.Save(Rect(rt.x - 5, rt.y + 15, rt.w + 10, 160));

        buttonGift.SetPos(pos_rt.x + (pos_rt.w - AGG::GetICN(tradpost, 17).w()) / 2, pos_rt.y + 120);
        buttonTrade.SetPos(pos_rt.x + (pos_rt.w - AGG::GetICN(tradpost, 17).w()) / 2, pos_rt.y + 150);
	buttonLeft.SetPos(pos_rt.x + 11, pos_rt.y + 129);
	buttonRight.SetPos(pos_rt.x + 220, pos_rt.y + 129);

	buttonGift.SetSprite(ICN::BTNGIFT, 0, 1);
	buttonTrade.SetSprite(tradpost, 15, 16);
	buttonLeft.SetSprite(tradpost, 3, 4);
	buttonRight.SetSprite(tradpost, 5, 6);

        splitter.SetSprite(AGG::GetICN(tradpost, 2));
        splitter.SetArea(Rect(pos_rt.x + (pos_rt.w - AGG::GetICN(tradpost, 1).w()) / 2 + 21, pos_rt.y + 131, 189, 11));
        splitter.HideCursor();

	TextBox(_("Please inspect our fine wares. If you feel like offering a trade, click on the items you wish to trade with and for."), Font::BIG, Rect(pos_rt.x, pos_rt.y + 30, pos_rt.w, 100));

        textSell.SetFont(Font::SMALL);
        textBuy.SetFont(Font::SMALL);
    };

    void RedrawInfoBuySell(u32 count_sell, u32 count_buy, u32 max_sell, u32 orig_buy);
    void ShowTradeArea(int resourceFrom, int resourceTo, u32 max_buy, u32 max_sell, u32 count_buy, u32 count_sell, bool fromTradingPost);

    Rect   buttonMax;
    Rect   buttonMin;
    Button buttonTrade;
    Button buttonLeft;
    Button buttonRight;
    Button buttonGift;
    Splitter splitter;

private:
    Rect pos_rt;
    SpriteBack back;
    int tradpost;

    TextSprite textSell;
    TextSprite textBuy;
};

void TradeWindowGUI::ShowTradeArea(int resourceFrom, int resourceTo, u32 max_buy, u32 max_sell, u32 count_buy, u32 count_sell, bool fromTradingPost)
{
    Cursor &cursor = Cursor::Get();
    Display &display = Display::Get();
    bool disable = world.GetKingdom(Settings::Get().CurrentColor()).GetFunds().Get(resourceFrom) <= 0;

    if(disable || resourceFrom == resourceTo || (Resource::GOLD != resourceTo && 0 == max_buy))
    {
        cursor.Hide();
	splitter.HideCursor();
        back.Restore();
        Rect dst_rt(pos_rt.x, pos_rt.y + 30, pos_rt.w, 100);
        TextBox(_("You have received quite a bargain. I expect to make no profit on the deal. Can I interest you in any of my other wares?"), Font::BIG, dst_rt);
        buttonGift.SetDisable(false);
	buttonTrade.SetDisable(true);
        buttonLeft.SetDisable(true);
        buttonRight.SetDisable(true);
        buttonGift.Draw();
	buttonMax = Rect();
	buttonMin = Rect();
        cursor.Show();
        display.Flip();
    }
    else
    {
        cursor.Hide();
        back.Restore();

        Point dst_pt;
        const Sprite & bar = AGG::GetICN(tradpost, 1);
        dst_pt.x = pos_rt.x + (pos_rt.w - bar.w()) / 2 - 2;
        dst_pt.y = pos_rt.y + 128;
        bar.Blit(dst_pt);
        splitter.SetRange(0, (Resource::GOLD == resourceTo ? max_sell : max_buy));
        u32 exchange_rate = GetTradeCosts(resourceFrom, resourceTo, fromTradingPost);
	std::string message;
        if(Resource::GOLD == resourceTo)
        {
            message = _("I can offer you %{count} for 1 unit of %{resfrom}.");
            StringReplace(message, "%{count}", exchange_rate);
            StringReplace(message, "%{resfrom}", Resource::String(resourceFrom));
        }
        else
        {
            message = _("I can offer you 1 unit of %{resto} for %{count} units of %{resfrom}.");
            StringReplace(message, "%{resto}", Resource::String(resourceTo));
            StringReplace(message, "%{resfrom}", Resource::String(resourceFrom));
            StringReplace(message, "%{count}", exchange_rate);
        }
        TextBox(message, Font::BIG, Rect(pos_rt.x, pos_rt.y + 30, pos_rt.w, 100));
        const Sprite & sprite_from = AGG::GetICN(ICN::RESOURCE, Resource::GetIndexSprite2(resourceFrom));
        dst_pt.x = pos_rt.x + (pos_rt.w - sprite_from.w()) / 2 - 70;
        dst_pt.y = pos_rt.y + 115 - sprite_from.h();
        sprite_from.Blit(dst_pt);
        const Sprite & sprite_to = AGG::GetICN(ICN::RESOURCE, Resource::GetIndexSprite2(resourceTo));
        dst_pt.x = pos_rt.x + (pos_rt.w - sprite_to.w()) / 2 + 70;
        dst_pt.y = pos_rt.y + 115 - sprite_to.h();
        sprite_to.Blit(dst_pt);
        const Sprite & sprite_fromto = AGG::GetICN(tradpost, 0);
        dst_pt.x = pos_rt.x + (pos_rt.w - sprite_fromto.w()) / 2;
        dst_pt.y = pos_rt.y + 90;
        sprite_fromto.Blit(dst_pt);
        Text text("max", Font::YELLOW_SMALL);
        dst_pt.x = pos_rt.x + (pos_rt.w - text.w()) / 2 - 5;
        dst_pt.y = pos_rt.y + 80;
	buttonMax = Rect(dst_pt.x, dst_pt.y, text.w(), text.h());
        text.Blit(dst_pt);
	text.Set("min", Font::YELLOW_SMALL);
        dst_pt.x = pos_rt.x + (pos_rt.w - text.w()) / 2 - 5;
        dst_pt.y = pos_rt.y + 103;
	buttonMin = Rect(dst_pt.x, dst_pt.y, text.w(), text.h());
        text.Blit(dst_pt);
	text.Set(_("Qty to trade"), Font::SMALL);
        dst_pt.x = pos_rt.x + (pos_rt.w - text.w()) / 2;
        dst_pt.y = pos_rt.y + 115;
        text.Blit(dst_pt);

        buttonGift.SetDisable(true);
        buttonTrade.SetDisable(false);
        buttonLeft.SetDisable(false);
        buttonRight.SetDisable(false);

        buttonTrade.Draw();
        buttonLeft.Draw();
        buttonRight.Draw();

        RedrawInfoBuySell(count_sell, count_buy, max_sell, world.GetKingdom(Settings::Get().CurrentColor()).GetFunds().Get(resourceTo));
	splitter.ShowCursor();
        cursor.Show();
        display.Flip();
    }
}

void TradeWindowGUI::RedrawInfoBuySell(u32 count_sell, u32 count_buy, u32 max_sell, u32 orig_buy)
{
    Point dst_pt;

    splitter.HideCursor();

    textSell.Hide();
    textSell.SetText(std::string("-") + GetString(count_sell) + " " + "(" + GetString(max_sell - count_sell) + ")");
    dst_pt.x = pos_rt.x + pos_rt.w / 2 - 70 - textSell.w() / 2;
    dst_pt.y = pos_rt.y + 116;
    textSell.SetPos(dst_pt);
    textSell.Show();

    textBuy.Hide();
    textBuy.SetText(std::string("+") + GetString(count_buy) + " " + "(" + GetString(orig_buy + count_buy) + ")");
    dst_pt.x = pos_rt.x + pos_rt.w / 2 + 70 - textBuy.w() / 2;
    dst_pt.y = pos_rt.y + 116;
    textBuy.SetPos(dst_pt);
    textBuy.Show();

    splitter.ShowCursor();
}

void Dialog::Marketplace(bool fromTradingPost)
{
    Display & display = Display::Get();
    const int tradpost = Settings::Get().ExtGameEvilInterface() ? ICN::TRADPOSE : ICN::TRADPOST;
    const std::string & header = _("Marketplace");

    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBox box(260, true);

    const Rect & pos_rt = box.GetArea();
    Point dst_pt(pos_rt.x, pos_rt.y);
    //Rect dst_rt(pos_rt);
    Text text;

    // header
    text.Set(header, Font::BIG);
    dst_pt.x = pos_rt.x + (pos_rt.w - text.w()) / 2;
    dst_pt.y = pos_rt.y;
    text.Blit(dst_pt);

    TradeWindowGUI gui(pos_rt);

    Kingdom & kingdom = world.GetKingdom(Settings::Get().CurrentColor());

    const std::string & header_from = _("Your Resources");

    Funds fundsFrom = kingdom.GetFunds();
    int resourceFrom = 0;
    const Point pt1(pos_rt.x, pos_rt.y + 190);
    Rects rectsFrom;
    rectsFrom.reserve(7);
    rectsFrom.push_back(Rect(pt1.x, pt1.y, 34, 34));		// wood
    rectsFrom.push_back(Rect(pt1.x + 37, pt1.y, 34, 34));	// mercury
    rectsFrom.push_back(Rect(pt1.x + 74, pt1.y, 34, 34));	// ore
    rectsFrom.push_back(Rect(pt1.x, pt1.y + 37, 34, 34));	// sulfur
    rectsFrom.push_back(Rect(pt1.x + 37, pt1.y + 37, 34, 34));	// crystal
    rectsFrom.push_back(Rect(pt1.x + 74, pt1.y + 37, 34, 34));	// gems
    rectsFrom.push_back(Rect(pt1.x + 37, pt1.y + 74, 34, 34));	// gold

    SpriteMove cursorFrom(AGG::GetICN(tradpost, 14));
    text.Set(header_from, Font::SMALL);
    dst_pt.x = pt1.x + (108 - text.w()) / 2;
    dst_pt.y = pt1.y - 15;
    text.Blit(dst_pt);
    RedrawFromResource(pt1, fundsFrom);

    const std::string & header_to = _("Available Trades");

    Funds fundsTo;
    int resourceTo = 0;
    const Point pt2(138 + pos_rt.x, pos_rt.y + 190);
    Rects rectsTo;
    rectsTo.reserve(7);
    rectsTo.push_back(Rect(pt2.x, pt2.y, 34, 34));		// wood
    rectsTo.push_back(Rect(pt2.x + 37, pt2.y, 34, 34));		// mercury
    rectsTo.push_back(Rect(pt2.x + 74, pt2.y, 34, 34));		// ore
    rectsTo.push_back(Rect(pt2.x, pt2.y + 37, 34, 34));		// sulfur
    rectsTo.push_back(Rect(pt2.x + 37, pt2.y + 37, 34, 34));	// crystal
    rectsTo.push_back(Rect(pt2.x + 74, pt2.y + 37, 34, 34));	// gems
    rectsTo.push_back(Rect(pt2.x + 37, pt2.y + 74, 34, 34));	// gold

    SpriteMove cursorTo(AGG::GetICN(tradpost, 14));
    text.Set(header_to, Font::SMALL);
    dst_pt.x = pt2.x + (108 - text.w()) / 2;
    dst_pt.y = pt2.y - 15;
    text.Blit(dst_pt);
    RedrawToResource(pt2, false, fromTradingPost);

    u32 count_sell = 0;
    u32 count_buy = 0;

    u32 max_sell = 0;
    u32 max_buy = 0;

    Rect & buttonMax = gui.buttonMax;
    Rect & buttonMin = gui.buttonMin;
    Button & buttonGift = gui.buttonGift;
    Button & buttonTrade = gui.buttonTrade;
    Button & buttonLeft = gui.buttonLeft;
    Button & buttonRight = gui.buttonRight;
    Splitter & splitter = gui.splitter;

    // button exit
    const Sprite & sprite_exit = AGG::GetICN(tradpost, 17);
    dst_pt.x = pos_rt.x + (pos_rt.w - sprite_exit.w()) / 2;
    dst_pt.y = pos_rt.y + pos_rt.h - sprite_exit.h();
    Button buttonExit(dst_pt.x, dst_pt.y, tradpost, 17, 18);

    buttonGift.Draw();
    buttonExit.Draw();
    cursor.Show();
    display.Flip();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while(le.HandleEvents())
    {
        if(buttonGift.isEnable()) le.MousePressLeft(buttonGift) ? buttonGift.PressDraw() : buttonGift.ReleaseDraw();
        if(buttonTrade.isEnable()) le.MousePressLeft(buttonTrade) ? buttonTrade.PressDraw() : buttonTrade.ReleaseDraw();
        if(buttonLeft.isEnable()) le.MousePressLeft(buttonLeft) ? buttonLeft.PressDraw() : buttonLeft.ReleaseDraw();
        if(buttonRight.isEnable()) le.MousePressLeft(buttonRight) ? buttonRight.PressDraw() : buttonRight.ReleaseDraw();

        le.MousePressLeft(buttonExit) ? buttonExit.PressDraw() : buttonExit.ReleaseDraw();

        if(le.MouseClickLeft(buttonExit) || HotKeyCloseWindow) break;

	if(buttonGift.isEnable() &&
	    le.MouseClickLeft(buttonGift))
	{
            cursor.Hide();
	    Dialog::MakeGiftResource();
	    fundsFrom = kingdom.GetFunds();
	    RedrawFromResource(pt1, fundsFrom);
            cursor.Show();
            display.Flip();
	}

        // click from
        for(u32 ii = 0; ii < rectsFrom.size(); ++ii)
        {
            const Rect & rect_from = rectsFrom[ii];

            if(le.MouseClickLeft(rect_from))
            {
		resourceFrom = Resource::FromIndexSprite2(ii);
                max_sell = fundsFrom.Get(resourceFrom);

                if(GetTradeCosts(resourceFrom, resourceTo, fromTradingPost))
                {
                    max_buy = Resource::GOLD == resourceTo ?
                        max_sell * GetTradeCosts(resourceFrom, resourceTo, fromTradingPost) :
                        max_sell / GetTradeCosts(resourceFrom, resourceTo, fromTradingPost);
                }

                count_sell = 0;
                count_buy = 0;

                cursor.Hide();
                cursorFrom.Move(rect_from.x - 2, rect_from.y - 2);

                if(resourceTo) cursorTo.Hide();
                RedrawToResource(pt2, true, fromTradingPost, resourceFrom);
                if(resourceTo) cursorTo.Show();
                if(resourceTo) gui.ShowTradeArea(resourceFrom, resourceTo, max_buy, max_sell, count_buy, count_sell, fromTradingPost);

                cursor.Show();
                display.Flip();
            }
    	    else
	    if(le.MousePressRight(rect_from))
        	Dialog::ResourceInfo("", "income:", kingdom.GetIncome(INCOME_ALL), 0);
        }

        // click to
        for(u32 ii = 0; ii < rectsTo.size(); ++ii)
        {
            const Rect & rect_to = rectsTo[ii];

            if(le.MouseClickLeft(rect_to))
            {
		resourceTo = Resource::FromIndexSprite2(ii);

                if(GetTradeCosts(resourceFrom, resourceTo, fromTradingPost))
                {
                    max_buy = Resource::GOLD == resourceTo ?
                        max_sell * GetTradeCosts(resourceFrom, resourceTo, fromTradingPost) :
                        max_sell / GetTradeCosts(resourceFrom, resourceTo, fromTradingPost);
                }

                count_sell = 0;
                count_buy = 0;

                cursor.Hide();
                cursorTo.Move(rect_to.x - 2, rect_to.y - 2);

                if(resourceFrom)
                {
                    cursorTo.Hide();
                    RedrawToResource(pt2, true, fromTradingPost, resourceFrom);
                    cursorTo.Show();
                    gui.ShowTradeArea(resourceFrom, resourceTo, max_buy, max_sell, count_buy, count_sell, fromTradingPost);
                }
                cursor.Show();
                display.Flip();
            }
        }

        // move splitter
        if(buttonLeft.isEnable() && buttonRight.isEnable() && max_buy && le.MousePressLeft(splitter.GetRect()))
        {
    	    s32 seek = (le.GetMouseCursor().x - splitter.GetRect().x) * 100 / splitter.GetStep();

            if(seek < splitter.Min()) seek = splitter.Min();
            else
            if(seek > splitter.Max()) seek = splitter.Max();

            count_buy = seek * (Resource::GOLD == resourceTo ? GetTradeCosts(resourceFrom, resourceTo, fromTradingPost) : 1);
            count_sell = seek * (Resource::GOLD == resourceTo ? 1: GetTradeCosts(resourceFrom, resourceTo, fromTradingPost));

            cursor.Hide();
            splitter.MoveIndex(seek);
            gui.RedrawInfoBuySell(count_sell, count_buy, max_sell, fundsFrom.Get(resourceTo));
            cursor.Show();
            display.Flip();
        }
        else
	// click max
	if(buttonMax.w && max_buy && le.MouseClickLeft(buttonMax))
	{
	    const u32 & max = splitter.Max();

            count_buy  = max * (Resource::GOLD == resourceTo ? GetTradeCosts(resourceFrom, resourceTo, fromTradingPost) : 1);
            count_sell = max * (Resource::GOLD == resourceTo ? 1: GetTradeCosts(resourceFrom, resourceTo, fromTradingPost));

            cursor.Hide();
            splitter.MoveIndex(max);
            gui.RedrawInfoBuySell(count_sell, count_buy, max_sell, fundsFrom.Get(resourceTo));
            cursor.Show();
            display.Flip();
	}
	// click min
	if(buttonMin.w && max_buy && le.MouseClickLeft(buttonMin))
	{
	    const u32 min = 1;

            count_buy  = min * (Resource::GOLD == resourceTo ? GetTradeCosts(resourceFrom, resourceTo, fromTradingPost) : 1);
            count_sell = min * (Resource::GOLD == resourceTo ? 1: GetTradeCosts(resourceFrom, resourceTo, fromTradingPost));

            cursor.Hide();
            splitter.MoveIndex(min);
            gui.RedrawInfoBuySell(count_sell, count_buy, max_sell, fundsFrom.Get(resourceTo));
            cursor.Show();
            display.Flip();
	}

        // trade
        if(buttonTrade.isEnable() && le.MouseClickLeft(buttonTrade) && count_sell && count_buy)
        {
            kingdom.OddFundsResource(Funds(resourceFrom, count_sell));
            kingdom.AddFundsResource(Funds(resourceTo, count_buy));

            resourceTo = resourceFrom = Resource::UNKNOWN;
            gui.ShowTradeArea(resourceFrom, resourceTo, 0, 0, 0, 0, fromTradingPost);

            fundsFrom = kingdom.GetFunds();
            cursorTo.Hide();
            cursorFrom.Hide();
            RedrawFromResource(pt1, fundsFrom);
            RedrawToResource(pt2, false, fromTradingPost, resourceFrom);
            display.Flip();
        }

        // decrease trade resource
        if(count_buy &&
           ((buttonLeft.isEnable() && le.MouseClickLeft(gui.buttonLeft)) ||
            le.MouseWheelDn(splitter.GetRect())))
        {
            count_buy -= Resource::GOLD == resourceTo ? GetTradeCosts(resourceFrom, resourceTo, fromTradingPost) : 1;

            count_sell -= Resource::GOLD == resourceTo ? 1: GetTradeCosts(resourceFrom, resourceTo, fromTradingPost);

            cursor.Hide();
            splitter.Backward();
            gui.RedrawInfoBuySell(count_sell, count_buy, max_sell, fundsFrom.Get(resourceTo));
            cursor.Show();
            display.Flip();
        }

        // increase trade resource
        if( count_buy < max_buy &&
            ((buttonRight.isEnable() && le.MouseClickLeft(buttonRight)) ||
             le.MouseWheelUp(splitter.GetRect())))
        {
            count_buy += Resource::GOLD == resourceTo ? GetTradeCosts(resourceFrom, resourceTo, fromTradingPost) : 1;

            count_sell += Resource::GOLD == resourceTo ? 1: GetTradeCosts(resourceFrom, resourceTo, fromTradingPost);

            cursor.Hide();
            splitter.Forward();
            gui.RedrawInfoBuySell(count_sell, count_buy, max_sell, fundsFrom.Get(resourceTo));
            cursor.Show();
            display.Flip();
        }
    }
}

void RedrawResourceSprite(const Surface & sf, s32 px, s32 py, s32 value)
{
    Display & display = Display::Get();
    Text text;
    Point dst_pt(px, py);

    sf.Blit(dst_pt, display);
    text.Set(GetString(value), Font::SMALL);
    dst_pt.x += (34 - text.w()) / 2;
    dst_pt.y += 21;
    text.Blit(dst_pt);
}

void RedrawFromResource(const Point & pt, const Funds & rs)
{
    const int tradpost = Settings::Get().ExtGameEvilInterface() ? ICN::TRADPOSE : ICN::TRADPOST;

    // wood
    RedrawResourceSprite(AGG::GetICN(tradpost, 7), pt.x, pt.y, rs.wood);
    // mercury
    RedrawResourceSprite(AGG::GetICN(tradpost, 8), pt.x + 37, pt.y, rs.mercury);
    // ore
    RedrawResourceSprite(AGG::GetICN(tradpost, 9), pt.x + 74, pt.y, rs.ore);
    // sulfur
    RedrawResourceSprite(AGG::GetICN(tradpost, 10), pt.x, pt.y + 37, rs.sulfur);
    // crystal
    RedrawResourceSprite(AGG::GetICN(tradpost, 11), pt.x + 37, pt.y + 37, rs.crystal);
    // gems
    RedrawResourceSprite(AGG::GetICN(tradpost, 12), pt.x + 74, pt.y + 37, rs.gems);
    // gold
    RedrawResourceSprite(AGG::GetICN(tradpost, 13), pt.x + 37, pt.y + 74, rs.gold);
}

void RedrawResourceSprite2(const Surface & sf, s32 px, s32 py, bool show, int from, int res, bool trading)
{
    Display & display = Display::Get();
    Point dst_pt(px, py);

    sf.Blit(dst_pt, display);

    if(show)
    {
	Text text(GetStringTradeCosts(from, res, trading), Font::SMALL);
	dst_pt.x += (34 - text.w()) / 2;
	dst_pt.y += 21;
	text.Blit(dst_pt);
    }
}

void RedrawToResource(const Point & pt, bool showcost, bool tradingPost, int from_resource)
{
    const int tradpost = Settings::Get().ExtGameEvilInterface() ? ICN::TRADPOSE : ICN::TRADPOST;

    // wood
    RedrawResourceSprite2(AGG::GetICN(tradpost, 7), pt.x, pt.y, showcost, from_resource, Resource::WOOD, tradingPost);
    // mercury
    RedrawResourceSprite2(AGG::GetICN(tradpost, 8), pt.x + 37, pt.y, showcost, from_resource, Resource::MERCURY, tradingPost);
    // ore
    RedrawResourceSprite2(AGG::GetICN(tradpost, 9), pt.x + 74, pt.y, showcost, from_resource, Resource::ORE, tradingPost);
    // sulfur
    RedrawResourceSprite2(AGG::GetICN(tradpost, 10), pt.x, pt.y + 37, showcost, from_resource, Resource::SULFUR, tradingPost);
    // crystal
    RedrawResourceSprite2(AGG::GetICN(tradpost, 11), pt.x + 37, pt.y + 37, showcost, from_resource, Resource::CRYSTAL, tradingPost);
    // gems
    RedrawResourceSprite2(AGG::GetICN(tradpost, 12), pt.x + 74, pt.y + 37, showcost, from_resource, Resource::GEMS, tradingPost);
    // gold
    RedrawResourceSprite2(AGG::GetICN(tradpost, 13), pt.x + 37, pt.y + 74, showcost, from_resource, Resource::GOLD, tradingPost);
}

std::string GetStringTradeCosts(int rs_from, int rs_to, bool tradingPost)
{
    std::string res;

    if(rs_from == rs_to)
    {
	res = _("n/a");
    }
    else
    {
	if(Resource::GOLD != rs_from && Resource::GOLD != rs_to)
	    res = "1/";
	res.append(GetString(GetTradeCosts(rs_from, rs_to, tradingPost)));
    }

    return res;
}

u32 GetTradeCosts(int rs_from, int rs_to, bool tradingPost)
{
    const u32 markets = tradingPost ? 3 : world.GetKingdom(Settings::Get().CurrentColor()).GetCountMarketplace();

    if(rs_from == rs_to) return 0;

    switch(rs_from)
    {
	// uncostly
	case Resource::WOOD:
	case Resource::ORE:

    	    switch(rs_to)
    	    {
    		// sale uncostly
    		case Resource::GOLD:
    		    if(1 == markets) return SALE_UNCOSTLY1;
        	    else
        	    if(2 == markets) return SALE_UNCOSTLY2;
        	    else
        	    if(3 == markets) return SALE_UNCOSTLY3;
        	    else
        	    if(4 == markets) return SALE_UNCOSTLY4;
        	    else
        	    if(5 == markets) return SALE_UNCOSTLY5;
        	    else
        	    if(6 == markets) return SALE_UNCOSTLY6;
        	    else
        	    if(7 == markets) return SALE_UNCOSTLY7;
        	    else
        	    if(8 == markets) return SALE_UNCOSTLY8;
        	    else
        	    if(8 <  markets) return SALE_UNCOSTLY9;
    		    break;

		// change uncostly to costly
		case Resource::MERCURY:
		case Resource::SULFUR:
		case Resource::CRYSTAL:
		case Resource::GEMS:
    		    if(1 == markets) return UNCOSTLY_COSTLY1;
        	    else
        	    if(2 == markets) return UNCOSTLY_COSTLY2;
        	    else
        	    if(3 == markets) return UNCOSTLY_COSTLY3;
        	    else
        	    if(4 == markets) return UNCOSTLY_COSTLY4;
        	    else
        	    if(5 == markets) return UNCOSTLY_COSTLY5;
        	    else
        	    if(6 == markets) return UNCOSTLY_COSTLY6;
        	    else
        	    if(7 == markets) return UNCOSTLY_COSTLY7;
        	    else
        	    if(8 == markets) return UNCOSTLY_COSTLY8;
        	    else
        	    if(8 <  markets) return UNCOSTLY_COSTLY9;
    		    break;

		// change uncostly to uncostly
		case Resource::WOOD:
		case Resource::ORE:
    		    if(1 == markets) return COSTLY_COSTLY1;
        	    else
        	    if(2 == markets) return COSTLY_COSTLY2;
        	    else
        	    if(3 == markets) return COSTLY_COSTLY3;
        	    else
        	    if(4 == markets) return COSTLY_COSTLY4;
        	    else
        	    if(5 == markets) return COSTLY_COSTLY5;
        	    else
        	    if(6 == markets) return COSTLY_COSTLY6;
        	    else
        	    if(7 == markets) return COSTLY_COSTLY7;
        	    else
        	    if(8 == markets) return COSTLY_COSTLY8;
        	    else
        	    if(8 <  markets) return COSTLY_COSTLY9;
        	    break;
    	    }
	    break;

	// costly
	case Resource::MERCURY:
	case Resource::SULFUR:
	case Resource::CRYSTAL:
	case Resource::GEMS:

    	    switch(rs_to)
    	    {
    		// sale costly
    		case Resource::GOLD:
    		    if(1 == markets) return SALE_COSTLY1;
        	    else
        	    if(2 == markets) return SALE_COSTLY2;
        	    else
        	    if(3 == markets) return SALE_COSTLY3;
        	    else
        	    if(4 == markets) return SALE_COSTLY4;
        	    else
        	    if(5 == markets) return SALE_COSTLY5;
        	    else
        	    if(6 == markets) return SALE_COSTLY6;
        	    else
        	    if(7 == markets) return SALE_COSTLY7;
        	    else
        	    if(8 == markets) return SALE_COSTLY8;
        	    else
        	    if(8 <  markets) return SALE_COSTLY9;
        	    break;

		// change costly to costly
		case Resource::MERCURY:
		case Resource::SULFUR:
		case Resource::CRYSTAL:
		case Resource::GEMS:
    		    if(1 == markets) return COSTLY_COSTLY1;
        	    else
        	    if(2 == markets) return COSTLY_COSTLY2;
        	    else
        	    if(3 == markets) return COSTLY_COSTLY3;
        	    else
        	    if(4 == markets) return COSTLY_COSTLY4;
        	    else
        	    if(5 == markets) return COSTLY_COSTLY5;
        	    else
        	    if(6 == markets) return COSTLY_COSTLY6;
        	    else
        	    if(7 == markets) return COSTLY_COSTLY7;
        	    else
        	    if(8 == markets) return COSTLY_COSTLY8;
        	    else
        	    if(8 <  markets) return COSTLY_COSTLY9;
        	    break;

		// change costly to uncostly
		case Resource::WOOD:
		case Resource::ORE:
    		    if(1 == markets) return COSTLY_UNCOSTLY1;
        	    else
        	    if(2 == markets) return COSTLY_UNCOSTLY2;
        	    else
        	    if(3 == markets) return COSTLY_UNCOSTLY3;
        	    else
        	    if(4 == markets) return COSTLY_UNCOSTLY4;
        	    else
        	    if(5 == markets) return COSTLY_UNCOSTLY5;
        	    else
        	    if(6 == markets) return COSTLY_UNCOSTLY6;
        	    else
        	    if(7 == markets) return COSTLY_UNCOSTLY7;
        	    else
        	    if(8 == markets) return COSTLY_UNCOSTLY8;
        	    else
        	    if(8 <  markets) return COSTLY_UNCOSTLY9;
        	    break;
    	    }
	    break;

	// gold
	case Resource::GOLD:

    	    switch(rs_to)
    	    {
    		default: break;

		// buy costly
		case Resource::MERCURY:
		case Resource::SULFUR:
		case Resource::CRYSTAL:
		case Resource::GEMS:
    		    if(1 == markets) return BUY_COSTLY1;
        	    else
        	    if(2 == markets) return BUY_COSTLY2;
        	    else
        	    if(3 == markets) return BUY_COSTLY3;
        	    else
        	    if(4 == markets) return BUY_COSTLY4;
        	    else
        	    if(5 == markets) return BUY_COSTLY5;
        	    else
        	    if(6 == markets) return BUY_COSTLY6;
        	    else
        	    if(7 == markets) return BUY_COSTLY7;
        	    else
        	    if(8 == markets) return BUY_COSTLY8;
        	    else
        	    if(8 <  markets) return BUY_COSTLY9;
        	    break;

		// buy uncostly
		case Resource::WOOD:
		case Resource::ORE:
    		    if(1 == markets) return BUY_UNCOSTLY1;
        	    else
        	    if(2 == markets) return BUY_UNCOSTLY2;
        	    else
        	    if(3 == markets) return BUY_UNCOSTLY3;
        	    else
        	    if(4 == markets) return BUY_UNCOSTLY4;
        	    else
        	    if(5 == markets) return BUY_UNCOSTLY5;
        	    else
        	    if(6 == markets) return BUY_UNCOSTLY6;
        	    else
        	    if(7 == markets) return BUY_UNCOSTLY7;
        	    else
        	    if(8 == markets) return BUY_UNCOSTLY8;
        	    else
        	    if(8 <  markets) return BUY_UNCOSTLY9;
        	    break;
    	    }
	    break;

	// not select
	default:  break;
    }

    return 0;
}
