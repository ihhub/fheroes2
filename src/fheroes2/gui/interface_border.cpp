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
#include "maps.h"
#include "settings.h"
#include "game_interface.h"
#include "interface_border.h"

void Interface::GameBorderRedraw(void)
{
    const Settings & conf = Settings::Get();
    if(conf.ExtGameHideInterface()) return;

    Display & display = Display::Get();

    const bool evil = Settings::Get().ExtGameEvilInterface();
    u32 count_w = (display.w() - 640) / TILEWIDTH;
    u32 count_h = (display.h() - 480) / TILEWIDTH;
    const u32 count_icons = count_h > 3 ? 8 : ( count_h < 3 ? 4 : 7);

    if(display.w() % TILEWIDTH) ++count_w;
    if(display.h() % TILEWIDTH) ++count_h;

    Rect srcrt;
    Point dstpt;
    const Sprite &icnadv = AGG::GetICN(evil ? ICN::ADVBORDE : ICN::ADVBORD, 0);

    // TOP BORDER
    srcrt.x = 0;
    srcrt.y = 0;
    srcrt.w = 223;
    srcrt.h = BORDERWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    icnadv.Blit(srcrt, dstpt);
    srcrt.x = 223;
    srcrt.w = TILEWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = 0;
    for(u32 ii = 0; ii < count_w + 1; ++ii)
    {
        icnadv.Blit(srcrt, dstpt);
	dstpt.x += TILEWIDTH;
    }
    srcrt.x += TILEWIDTH;
    srcrt.w = icnadv.w() - srcrt.x;
    icnadv.Blit(srcrt, dstpt);


    // LEFT BORDER
    srcrt.x = 0;
    srcrt.y = 0;
    srcrt.w = BORDERWIDTH;
    srcrt.h = 255;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    icnadv.Blit(srcrt, dstpt);
    srcrt.y = 255;
    srcrt.h = TILEWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    for(u32 ii = 0; ii < count_h + 1; ++ii)
    {
        icnadv.Blit(srcrt, dstpt);
	dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;
    srcrt.h = icnadv.h() - srcrt.y;
    icnadv.Blit(srcrt, dstpt);

    // MIDDLE BORDER
    srcrt.x = icnadv.w() - RADARWIDTH - 2 * BORDERWIDTH;
    srcrt.y = 0;
    srcrt.w = BORDERWIDTH;
    srcrt.h = 255;
    dstpt.x = display.w() - RADARWIDTH - 2 * BORDERWIDTH;
    dstpt.y = srcrt.y;
    icnadv.Blit(srcrt, dstpt);
    srcrt.y = 255;
    srcrt.h = TILEWIDTH;
    dstpt.x = display.w() - RADARWIDTH - 2 * BORDERWIDTH;
    dstpt.y = srcrt.y;
    for(u32 ii = 0; ii < count_h + 1; ++ii)
    {
        icnadv.Blit(srcrt, dstpt);
        dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;
    srcrt.h = icnadv.h() - srcrt.y;
    icnadv.Blit(srcrt, dstpt);

    // RIGHT BORDER
    srcrt.x = icnadv.w() - BORDERWIDTH;
    srcrt.y = 0;
    srcrt.w = BORDERWIDTH;
    srcrt.h = 255;
    dstpt.x = display.w() - BORDERWIDTH;
    dstpt.y = srcrt.y;
    icnadv.Blit(srcrt, dstpt);
    srcrt.y = 255;
    srcrt.h = TILEWIDTH;
    dstpt.x = display.w() - BORDERWIDTH;
    dstpt.y = srcrt.y;
    for(u32 ii = 0; ii < count_h + 1; ++ii)
    {
        icnadv.Blit(srcrt, dstpt);
        dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;
    srcrt.h = icnadv.h() - srcrt.y;
    icnadv.Blit(srcrt, dstpt);

    // BOTTOM BORDER
    srcrt.x = 0;
    srcrt.y = icnadv.h() - BORDERWIDTH;
    srcrt.w = 223;
    srcrt.h = BORDERWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = display.h() - BORDERWIDTH;
    icnadv.Blit(srcrt, dstpt);
    srcrt.x = 223;
    srcrt.w = TILEWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = display.h() - BORDERWIDTH;
    for(u32 ii = 0; ii < count_w + 1; ++ii)
    {
        icnadv.Blit(srcrt, dstpt);
	dstpt.x += TILEWIDTH;
    }
    srcrt.x += TILEWIDTH;
    srcrt.w = icnadv.w() - srcrt.x;
    icnadv.Blit(srcrt, dstpt);

    // ICON BORDER
    srcrt.x = icnadv.w() - RADARWIDTH - BORDERWIDTH;
    srcrt.y = RADARWIDTH + BORDERWIDTH;
    srcrt.w = RADARWIDTH;
    srcrt.h = BORDERWIDTH;
    dstpt.x = display.w() - RADARWIDTH - BORDERWIDTH;
    dstpt.y = srcrt.y;
    icnadv.Blit(srcrt, dstpt);
    dstpt.y = srcrt.y + BORDERWIDTH + count_icons * 32;
    srcrt.y = srcrt.y + BORDERWIDTH + 4 * 32;
    icnadv.Blit(srcrt, dstpt);
}

Interface::BorderWindow::BorderWindow(const Rect & rt) : area(rt)
{
    if(Settings::Get().QVGA())
        border.SetBorder(6);
}

const Rect & Interface::BorderWindow::GetRect(void) const
{
    return Settings::Get().ExtGameHideInterface() && border.isValid() ?
	border.GetRect() : GetArea();
}

const Rect & Interface::BorderWindow::GetArea(void) const
{
    return area;
}

void Interface::BorderWindow::Redraw(void)
{
    if(Settings::Get().QVGA())
    {
	const Surface & sf = AGG::GetICN(ICN::RESOURCE, 7);
	Dialog::FrameBorder::RenderOther(sf, border.GetRect());
    }
    else
	Dialog::FrameBorder::RenderRegular(border.GetRect());
}

void Interface::BorderWindow::SetPosition(s32 px, s32 py, u32 pw, u32 ph)
{
    area.w = pw;
    area.h = ph;

    SetPosition(px, py);
}

void Interface::BorderWindow::SetPosition(s32 px, s32 py)
{
    if(Settings::Get().ExtGameHideInterface())
    {
	Display & display = Display::Get();

	if(px + area.w < 0) px = 0;
	else
	if(px > display.w() - area.w + border.BorderWidth()) px = display.w() - area.w;

	if(py + area.h < 0) py = 0;
	else
	if(py > display.h() - area.h + border.BorderHeight()) py = display.h() - area.h;

        area.x = px + border.BorderWidth();
        area.y = py + border.BorderHeight();

        border.SetPosition(px, py, area.w, area.h);
	SavePosition();
    }
    else
    {
        area.x = px;
        area.y = py;
    }
}

bool Interface::BorderWindow::QueueEventProcessing(void)
{
    Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();

    if(conf.ExtGameHideInterface() &&
	le.MousePressLeft(border.GetTop()))
    {
	Display & display = Display::Get();
	Cursor & cursor = Cursor::Get();

        const Point & mp = le.GetMouseCursor();
	const Rect & pos = GetRect();

        SpriteMove moveIndicator(Surface(pos, false));
        moveIndicator.DrawBorder(RGBA(0xD0, 0xC0, 0x48), false);

        const s32 ox = mp.x - pos.x;
        const s32 oy = mp.y - pos.y;

        cursor.Hide();
	moveIndicator.Move(pos.x, pos.y);
	moveIndicator.Redraw();
        cursor.Show();
        display.Flip();

        while(le.HandleEvents() && le.MousePressLeft())
	{
    	    if(le.MouseMotion())
    	    {
        	    cursor.Hide();
		    moveIndicator.Move(mp.x - ox, mp.y - oy);
        	    cursor.Show();
        	    display.Flip();
    	    }
        }

        cursor.Hide();
        SetPos(mp.x - ox, mp.y - oy);
        Interface::Basic::Get().SetRedraw(REDRAW_GAMEAREA);

	return true;
    }

    return false;
}
