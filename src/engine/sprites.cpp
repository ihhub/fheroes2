/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "sprites.h"
#include "display.h"

SpritePos::SpritePos()
{
}

SpritePos::SpritePos(const Surface & sf, const Point & pt) : Surface(sf), pos(pt)
{
}

const Point & SpritePos::GetPos(void) const
{
    return pos;
}

Rect SpritePos::GetArea(void) const
{
    return Rect(GetPos(), GetSize());
}

void SpritePos::SetSurface(const Surface & sf)
{
    Surface::Set(sf, true);
}

void SpritePos::SetPos(const Point & pt)
{
    pos = pt;
}

void SpritePos::Reset(void)
{
    pos = Point(0, 0);
    Surface::Reset();
}

u32 SpritePos::GetMemoryUsage(void) const
{
    return Surface::GetMemoryUsage() + sizeof(pos);
}

SpriteBack::SpriteBack()
{
}

u32 SpriteBack::GetMemoryUsage(void) const
{
    return Surface::GetMemoryUsage() + sizeof(pos);
}

SpriteBack::SpriteBack(const Rect & rt)
{
    Save(rt);
}

void SpriteBack::SetPos(const Point & pt)
{
    pos.x = pt.x;
    pos.y = pt.y;
}

bool SpriteBack::isValid(void) const
{
    return Surface::isValid();
}

void SpriteBack::Save(const Rect & rt)
{
    // resize SpriteBack
    if(Surface::isValid() &&
	GetSize() != rt) FreeSurface(*this);

    if(rt.w && rt.h)
    {
	Set(Display::Get().GetSurface(rt), true);

	pos.w = rt.w;
	pos.h = rt.h;
    }

    pos.x = rt.x;
    pos.y = rt.y;
}

void SpriteBack::Save(const Point & pt)
{
    Save(Rect(pt, GetSize()));
}

void SpriteBack::Restore(void)
{
    if(Surface::isValid())
	Blit(GetPos(), Display::Get());
}

void SpriteBack::Destroy(void)
{
    Surface::FreeSurface(*this);
    pos.w = 0;
    pos.h = 0;
}

const Point & SpriteBack::GetPos(void) const
{
    return pos;
}

const Size & SpriteBack::GetSize(void) const
{
    return pos;
}

const Rect & SpriteBack::GetArea(void) const
{
    return pos;
}

enum { _VISIBLE = 0x00001 };

SpriteMove::SpriteMove() : mode(0)
{
}

SpriteMove::SpriteMove(const Surface & sf) : mode(0)
{
    Set(sf, true);
}

void SpriteMove::Move(const Point & pt)
{
    if(GetPos() != pt)
        Hide();

    Show(pt);
}

void SpriteMove::Move(int ax, int ay)
{
    Move(Point(ax, ay));
}

void SpriteMove::Hide(void)
{
    if(isVisible())
    {
        background.Restore();
	mode &= ~(_VISIBLE);
    }
}

void SpriteMove::Show(const Point & pos)
{
    if(! isVisible() && Surface::isValid())
    {
        background.Save(Rect(pos, GetSize()));
        Surface::Blit(GetPos(), Display::Get());
	mode |= _VISIBLE;
    }
}

void SpriteMove::Redraw(void)
{
    Hide();
    Show();
}

void SpriteMove::Show(void)
{
    Show(GetPos());
}

bool SpriteMove::isVisible(void) const
{
    return mode & _VISIBLE;
}

const Point & SpriteMove::GetPos(void) const
{
    return background.GetPos();
}

const Rect & SpriteMove::GetArea(void) const
{
    return background.GetArea();
}

u32 SpriteMove::GetMemoryUsage(void) const
{
    return Surface::GetMemoryUsage() +
	background.GetMemoryUsage() + sizeof(mode);
}
