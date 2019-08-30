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
#include "dialog.h"

#define  ANGLEWIDTH 44

Dialog::FrameBorder::FrameBorder(int v) : border(v)
{
}

Dialog::FrameBorder::~FrameBorder()
{
    if(Cursor::Get().isVisible()){ Cursor::Get().Hide(); };
    background.Restore();
}

Dialog::FrameBorder::FrameBorder(const Size & sz, const Surface & sf) : border(BORDERWIDTH)
{
    Display & display = Display::Get();
    SetPosition((display.w() - sz.w - border * 2) / 2, (display.h() - sz.h - border * 2) / 2, sz.w, sz.h);
    RenderOther(sf, GetRect());
}

Dialog::FrameBorder::FrameBorder(const Size & sz) : border(BORDERWIDTH)
{
    Display & display = Display::Get();
    SetPosition((display.w() - sz.w - border * 2) / 2, (display.h() - sz.h - border * 2) / 2, sz.w, sz.h);
    RenderRegular(GetRect());
}

Dialog::FrameBorder::FrameBorder(s32 posx, s32 posy, u32 encw, u32 ench) : border(BORDERWIDTH)
{
    SetPosition(posx, posy, encw, ench);
    RenderRegular(GetRect());
}

int Dialog::FrameBorder::BorderWidth(void) const
{
    return border;
}

int Dialog::FrameBorder::BorderHeight(void) const
{
    return border;
}

bool Dialog::FrameBorder::isValid(void) const
{
    return background.isValid();
}

void Dialog::FrameBorder::SetPosition(s32 posx, s32 posy, u32 encw, u32 ench)
{
    if(background.isValid())
	background.Restore();

    rect.x = posx;
    rect.y = posy;

    if(encw && ench)
    {
	rect.w = encw + 2 * border;
	rect.h = ench + 2 * border;

    	background.Save(rect);

	area.w = encw;
	area.h = ench;
    }
    else
    	background.Save(Point(posx, posy));

    area.x = posx + border;
    area.y = posy + border;

    top = Rect(posx, posy, area.w, border);
}

void Dialog::FrameBorder::SetBorder(int v)
{
    border = v;
}

const Rect & Dialog::FrameBorder::GetTop(void) const
{
    return top;
}

const Rect & Dialog::FrameBorder::GetRect(void) const
{
    return rect;
}

const Rect & Dialog::FrameBorder::GetArea(void) const
{
    return area;
}

void Dialog::FrameBorder::RenderRegular(const Rect & dstrt)
{
    const Sprite & sf = AGG::GetICN((Settings::Get().ExtGameEvilInterface() ? ICN::SURDRBKE : ICN::SURDRBKG), 0);
    const u32 shadow = 16;
    sf.RenderSurface(Rect(shadow, 0, sf.w() - shadow, sf.h() - shadow), Size(dstrt.w, dstrt.h)).Blit(dstrt.x, dstrt.y, Display::Get());
}

void Dialog::FrameBorder::RenderOther(const Surface & srcsf, const Rect & dstrt)
{
    srcsf.RenderSurface(Size(dstrt.w, dstrt.h)).Blit(dstrt.x, dstrt.y, Display::Get());
}
