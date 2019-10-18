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
#ifndef H2RECT_H
#define H2RECT_H

#include <vector>
#include <string>
#include <utility>
#include <functional>
#include "types.h"

struct Point
{
    s16 x, y;

    Point();
    Point(s16, s16);

    bool operator== (const Point &) const;
    bool operator!= (const Point &) const;

    bool inABC(const Point &, const Point &, const Point &) const;

    Point & operator+= (const Point &);
    Point & operator-= (const Point &);

    Point operator+ (const Point &) const;
    Point operator- (const Point &) const;
};


struct Size
{
    u16 w, h;

    Size();
    Size(u16, u16);
    Size(const Point &);

    bool isEmpty(void) const;

    bool operator== (const Size &) const;
    bool operator!= (const Size &) const;

    Size & operator+= (const Size &);
    Size & operator-= (const Size &);

    Size operator+ (const Size &) const;
    Size operator- (const Size &) const;
};

struct Rect : Point, Size
{
    Rect();
    Rect(s16, s16, u16, u16);
    Rect(const Point &, u16, u16);
    Rect(const Point &, const Size &);
    Rect(const SDL_Rect &);

    Rect & operator= (const Point &);
    bool operator== (const Rect &) const;
    bool operator!= (const Rect &) const;

    // rect include point
    bool operator& (const Point &) const;
    // rect intersects rect
    bool operator& (const Rect &) const;
    //
    static Rect Get(const Point &, const Point &);
    static Rect Get(const Rect &, const Rect &, bool intersect);
    static std::pair<Rect, Point> Fixed4Blit(const Rect &, const Rect &);
};

SDL_Rect SDLRect(s32, s32, u32, u32);
SDL_Rect SDLRect(const Rect &);

struct Points : std::vector<Point>
{
    Rect GetRect(void) const;
};

struct Rects : std::vector<Rect>
{
    s32  GetIndex(const Point &) const;
    Rect GetRect(void) const;
};

#endif
