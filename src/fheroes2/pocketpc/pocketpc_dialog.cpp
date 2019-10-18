/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov                               *
 *   afletdinov@mail.dc.baikal.ru                                          *
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
#include "engine.h"
#include "cursor.h"
#include "battle.h"
#include "battle_board.h"
#include "game.h"
#include "text.h"
#include "settings.h"
#include "pocketpc.h"

u32 PocketPC::GetCursorAttackDialog(const Point & dst, int allow)
{
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    const Rect rt(dst.x - 32, dst.y - 32, 86, 86);

    const Sprite & sp_info = AGG::GetICN(ICN::CMSECO, 5);
    const Sprite & sp_bleft = AGG::GetICN(ICN::CMSECO, 10);
    const Sprite & sp_left = AGG::GetICN(ICN::CMSECO, 11);
    const Sprite & sp_tleft = AGG::GetICN(ICN::CMSECO, 12);
    const Sprite & sp_tright = AGG::GetICN(ICN::CMSECO, 7);
    const Sprite & sp_right = AGG::GetICN(ICN::CMSECO, 8);
    const Sprite & sp_bright = AGG::GetICN(ICN::CMSECO, 9);

    Surface shadow(rt, false);
    shadow.Fill(ColorBlack);

    SpriteBack back(rt);

    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes(Cursor::POINTER);

    // blit alpha
    shadow.SetAlphaMod(120);
    shadow.Blit(rt.x, rt.y, display);

    const Rect rt_info(rt.x + (rt.w - sp_info.w()) / 2, rt.y + (rt.h - sp_info.h()) / 2, sp_info.w(), sp_info.h());
    sp_info.Blit(rt_info.x, rt_info.y);

    const Rect rt_tright(rt.x + 1, rt.y + rt.h - 1 - sp_tright.h(), sp_tright.w(), sp_tright.h());
    if(allow & Battle::BOTTOM_LEFT) sp_tright.Blit(rt_tright.x, rt_tright.y);

    const Rect rt_right(rt.x + 1, rt.y + (rt.h - sp_right.h()) / 2, sp_right.w(), sp_right.h());
    if(allow & Battle::LEFT) sp_right.Blit(rt_right.x, rt_right.y);

    const Rect rt_bright(rt.x + 1, rt.y + 1, sp_bright.w(), sp_bright.h());
    if(allow & Battle::TOP_LEFT) sp_bright.Blit(rt_bright.x, rt_bright.y);

    const Rect rt_tleft(rt.x + rt.w - 1 - sp_tleft.w(), rt.y + rt.h - 1 - sp_tleft.h(), sp_tleft.w(), sp_tleft.h());
    if(allow & Battle::BOTTOM_RIGHT) sp_tleft.Blit(rt_tleft.x, rt_tleft.y);

    const Rect rt_left(rt.x + rt.w - 1 - sp_left.w(), rt.y + (rt.h - sp_left.h()) / 2, sp_left.w(), sp_left.h());
    if(allow & Battle::RIGHT) sp_left.Blit(rt_left.x, rt_left.y);

    const Rect rt_bleft(rt.x + rt.w - 1 - sp_bleft.w(), rt.y + 1, sp_bleft.w(), sp_bleft.h());
    if(allow & Battle::TOP_RIGHT) sp_bleft.Blit(rt_bleft.x, rt_bleft.y);

    cursor.Show();
    display.Flip();

    while(le.HandleEvents() && !le.MouseClickLeft());

    if((allow & Battle::BOTTOM_LEFT) && (rt_tright & le.GetMouseCursor()))	return Cursor::SWORD_TOPRIGHT;
    else
    if((allow & Battle::LEFT) && (rt_right & le.GetMouseCursor()))		return Cursor::SWORD_RIGHT;
    else
    if((allow & Battle::TOP_LEFT) && (rt_bright & le.GetMouseCursor()))	return Cursor::SWORD_BOTTOMRIGHT;
    else
    if((allow & Battle::BOTTOM_RIGHT) && (rt_tleft & le.GetMouseCursor()))	return Cursor::SWORD_TOPLEFT;
    else
    if((allow & Battle::RIGHT) && (rt_left & le.GetMouseCursor()))		return Cursor::SWORD_LEFT;
    else
    if((allow & Battle::TOP_RIGHT) && (rt_bleft & le.GetMouseCursor()))	return Cursor::SWORD_BOTTOMLEFT;

    return Cursor::WAR_INFO;
}

Surface CreateTouchButton(void)
{
    Surface sf(Size(24, 24), false);

    const u32 ww = sf.w() / 2;
    const u32 hh = sf.h() / 2;

    const Sprite & sp0 = AGG::GetICN(ICN::LOCATORS, 22);

    sp0.Blit(Rect(0, 0, ww, hh), Point(0, 0), sf);
    sp0.Blit(Rect(sp0.w() - ww, 0, ww, hh), Point(ww, 0), sf);
    sp0.Blit(Rect(0, sp0.h() - hh, ww, hh), Point(0, hh), sf);
    sp0.Blit(Rect(sp0.w() - ww, sp0.h() - hh, ww, hh), Point(ww, hh), sf);

    return sf;
}

void RedrawTouchButton(const Surface & sf, const Rect & rt, const char* lb)
{
    Display & display = Display::Get();

    if(sf.w() != rt.w)
    {
	const u32 ww = 4;
	sf.Blit(Rect(0, 0, ww, sf.h()), rt.x, rt.y, display);

	if(rt.w > 8)
	{
	    const u32 count = (rt.w - ww) / ww;
	    for(u32 ii = 0; ii < count; ++ii)
		sf.Blit(Rect(ww, 0, ww, sf.h()), rt.x + ww * (ii + 1), rt.y, display);
	}

	sf.Blit(Rect(sf.w() - ww, 0, ww, sf.h()), rt.x + rt.w - ww, rt.y, display);
    }
    else
	sf.Blit(rt.x, rt.y, display);

    if(lb)
    {
	Text tx(lb, Font::BIG);
	tx.Blit(rt.x + (rt.w - tx.w()) / 2, rt.y + (rt.h - tx.h()) / 2);
    }
}

void PocketPC::KeyboardDialog(std::string & str)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();
    cursor.Hide();

    const u32 width = 337;
    const u32 height = 118;

    SpriteBack back(Rect((display.w() - width) / 2, 0, width, height));
    const Rect & top = back.GetArea();
    display.FillRect(top, ColorBlack);

    const Surface sp = CreateTouchButton();

    // 1 row
    const Rect rt_1(top.x + 2, top.y + 2, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_1, "1");

    const Rect rt_2(rt_1.x + rt_1.w + 1, top.y + 2, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_2, "2");

    const Rect rt_3(rt_2.x + rt_2.w + 1, top.y + 2, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_3, "3");

    const Rect rt_4(rt_3.x + rt_3.w + 1, top.y + 2, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_4, "4");

    const Rect rt_5(rt_4.x + rt_4.w + 1, top.y + 2, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_5, "5");

    const Rect rt_6(rt_5.x + rt_5.w + 1, top.y + 2, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_6, "6");

    const Rect rt_7(rt_6.x + rt_6.w + 1, top.y + 2, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_7, "7");

    const Rect rt_8(rt_7.x + rt_7.w + 1, top.y + 2, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_8, "8");

    const Rect rt_9(rt_8.x + rt_8.w + 1, top.y + 2, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_9, "9");

    const Rect rt_0(rt_9.x + rt_9.w + 1, top.y + 2, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_0, "0");

    const Rect rt_MINUS(rt_0.x + rt_0.w + 1, top.y + 2, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_MINUS, "-");

    const Rect rt_BACKSPACE(rt_MINUS.x + rt_MINUS.w + 1, top.y + 2, 58, sp.h());
    RedrawTouchButton(sp, rt_BACKSPACE, "back");

    // 2 row
    const Rect rt_EMPTY1(top.x + 2, top.y + 27, 8, sp.h());
    RedrawTouchButton(sp, rt_EMPTY1, NULL);

    const Rect rt_Q(rt_EMPTY1.x + rt_EMPTY1.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_Q, "q");

    const Rect rt_W(rt_Q.x + rt_Q.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_W, "w");

    const Rect rt_E(rt_W.x + rt_W.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_E, "e");

    const Rect rt_R(rt_E.x + rt_E.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_R, "r");

    const Rect rt_T(rt_R.x + rt_R.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_T, "t");

    const Rect rt_Y(rt_T.x + rt_T.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_Y, "y");

    const Rect rt_U(rt_Y.x + rt_Y.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_U, "u");

    const Rect rt_I(rt_U.x + rt_U.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_I, "i");

    const Rect rt_O(rt_I.x + rt_I.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_O, "o");

    const Rect rt_P(rt_O.x + rt_O.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_P, "p");

    const Rect rt_LB(rt_P.x + rt_P.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_LB, "[");

    const Rect rt_RB(rt_LB.x + rt_LB.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_RB, "]");

    const Rect rt_EQUAL(rt_RB.x + rt_RB.w + 1, top.y + 27, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_EQUAL, "=");

    // 3 row
    const Rect rt_EMPTY3(top.x + 2, top.y + 52, 15, sp.h());
    RedrawTouchButton(sp, rt_EMPTY3, NULL);

    const Rect rt_A(rt_EMPTY3.x + rt_EMPTY3.w + 1, top.y + 52, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_A, "a");

    const Rect rt_S(rt_A.x + rt_A.w + 1, top.y + 52, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_S, "s");

    const Rect rt_D(rt_S.x + rt_S.w + 1, top.y + 52, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_D, "d");

    const Rect rt_F(rt_D.x + rt_D.w + 1, top.y + 52, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_F, "f");

    const Rect rt_G(rt_F.x + rt_F.w + 1, top.y + 52, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_G, "g");

    const Rect rt_H(rt_G.x + rt_G.w + 1, top.y + 52, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_H, "h");

    const Rect rt_J(rt_H.x + rt_H.w + 1, top.y + 52, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_J, "j");

    const Rect rt_K(rt_J.x + rt_J.w + 1, top.y + 52, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_K, "k");

    const Rect rt_L(rt_K.x + rt_K.w + 1, top.y + 52, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_L, "l");

    const Rect rt_SP(rt_L.x + rt_L.w + 1, top.y + 52, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_SP, ";");

    const Rect rt_CM(rt_SP.x + rt_SP.w + 1, top.y + 52, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_CM, "'");

    const Rect rt_RETURN(rt_CM.x + rt_CM.w + 1, top.y + 52, 42, sp.h());
    RedrawTouchButton(sp, rt_RETURN, "rtrn");

    // 4 row
    const Rect rt_EMPTY5(top.x + 2, top.y + 77, 26, sp.h());
    RedrawTouchButton(sp, rt_EMPTY5, NULL);

    const Rect rt_Z(rt_EMPTY5.x + rt_EMPTY5.w + 1, top.y + 77, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_Z, "z");

    const Rect rt_X(rt_Z.x + rt_Z.w + 1, top.y + 77, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_X, "x");

    const Rect rt_C(rt_X.x + rt_X.w + 1, top.y + 77, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_C, "c");

    const Rect rt_V(rt_C.x + rt_C.w + 1, top.y + 77, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_V, "v");

    const Rect rt_B(rt_V.x + rt_V.w + 1, top.y + 77, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_B, "b");

    const Rect rt_N(rt_B.x + rt_B.w + 1, top.y + 77, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_N, "n");

    const Rect rt_M(rt_N.x + rt_N.w + 1, top.y + 77, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_M, "m");

    const Rect rt_CS(rt_M.x + rt_M.w + 1, top.y + 77, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_CS, ",");

    const Rect rt_DT(rt_CS.x + rt_CS.w + 1, top.y + 77, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_DT, ".");

    const Rect rt_SL(rt_DT.x + rt_DT.w + 1, top.y + 77, sp.w(), sp.h());
    RedrawTouchButton(sp, rt_SL, "/");

    const Rect rt_SPACE(rt_SL.x + rt_SL.w + 1, top.y + 77, 56, sp.h());
    RedrawTouchButton(sp, rt_SPACE, "space");

    cursor.Show();
    display.Flip();

    bool redraw = true;

    // mainmenu loop
    while(le.HandleEvents())
    {
        if(Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT))
	{
	    str.clear();
	    break;
	}

	char ch = 0;

        if(le.MouseClickLeft(rt_0))	ch = '0';
	else
        if(le.MouseClickLeft(rt_1))	ch = '1';
	else
        if(le.MouseClickLeft(rt_2))	ch = '2';
	else
        if(le.MouseClickLeft(rt_3))	ch = '3';
	else
        if(le.MouseClickLeft(rt_4))	ch = '4';
	else
        if(le.MouseClickLeft(rt_5))	ch = '5';
	else
        if(le.MouseClickLeft(rt_6))	ch = '6';
	else
        if(le.MouseClickLeft(rt_7))	ch = '7';
	else
        if(le.MouseClickLeft(rt_8))	ch = '8';
	else
        if(le.MouseClickLeft(rt_9))	ch = '9';
	else
        if(le.MouseClickLeft(rt_A))	ch = 'a';
	else
        if(le.MouseClickLeft(rt_B))	ch = 'b';
	else
        if(le.MouseClickLeft(rt_C))	ch = 'c';
	else
        if(le.MouseClickLeft(rt_D))	ch = 'd';
	else
        if(le.MouseClickLeft(rt_E))	ch = 'e';
	else
        if(le.MouseClickLeft(rt_F))	ch = 'f';
	else
        if(le.MouseClickLeft(rt_G))	ch = 'g';
	else
        if(le.MouseClickLeft(rt_H))	ch = 'h';
	else
        if(le.MouseClickLeft(rt_I))	ch = 'i';
	else
        if(le.MouseClickLeft(rt_J))	ch = 'j';
	else
        if(le.MouseClickLeft(rt_K))	ch = 'k';
	else
        if(le.MouseClickLeft(rt_L))	ch = 'l';
	else
        if(le.MouseClickLeft(rt_M))	ch = 'm';
	else
        if(le.MouseClickLeft(rt_N))	ch = 'n';
	else
        if(le.MouseClickLeft(rt_O))	ch = 'o';
	else
        if(le.MouseClickLeft(rt_P))	ch = 'p';
	else
        if(le.MouseClickLeft(rt_Q))	ch = 'q';
	else
        if(le.MouseClickLeft(rt_R))	ch = 'r';
	else
        if(le.MouseClickLeft(rt_S))	ch = 's';
	else
        if(le.MouseClickLeft(rt_T))	ch = 't';
	else
        if(le.MouseClickLeft(rt_U))	ch = 'u';
	else
        if(le.MouseClickLeft(rt_V))	ch = 'v';
	else
        if(le.MouseClickLeft(rt_W))	ch = 'w';
	else
        if(le.MouseClickLeft(rt_X))	ch = 'x';
	else
        if(le.MouseClickLeft(rt_Y))	ch = 'y';
	else
        if(le.MouseClickLeft(rt_Z))	ch = 'z';
	else
	if(le.MouseClickLeft(rt_EQUAL))	ch = '=';
	else
	if(le.MouseClickLeft(rt_MINUS))	ch = '-';
	else
	if(le.MouseClickLeft(rt_LB))	ch = '[';
	else
	if(le.MouseClickLeft(rt_RB))	ch = ']';
        else
	if(le.MouseClickLeft(rt_SP))	ch = ';';
	else
	if(le.MouseClickLeft(rt_CM))	ch = '\'';
	else
        if(le.MouseClickLeft(rt_CS))	ch = ',';
	else
        if(le.MouseClickLeft(rt_DT))	ch = '.';
	else
	if(le.MouseClickLeft(rt_SL))	ch = '/';
	else
	if(le.MouseClickLeft(rt_SPACE))	ch = 0x20;

	if(le.MouseClickLeft(rt_BACKSPACE) && str.size())
	{
	    str.resize(str.size() - 1);
	    redraw = true;
	}
	else
	if(le.MouseClickLeft(rt_RETURN))
	    break;
	else
	if(ch)
	{
	    str += ch;
	    redraw = true;
	}

	if(redraw)
	{
	    Text tx(str, Font::SMALL);
	    if(tx.w() < top.w) 
	    {
		cursor.Hide();
		display.FillRect(Rect(top.x, top.y + top.h - 16, top.w, 16), ColorBlack);
		tx.Blit(top.x + (top.w - tx.w()) / 2, top.y + top.h - 16 + 2);
		cursor.Show();
		display.Flip();
	    }
	    redraw = false;
	}
    }

    cursor.Hide();
    back.Restore();
    cursor.Show();
    display.Flip();
}
