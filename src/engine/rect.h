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

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "math_base.h"
#include "types.h"

struct Point
{
    int16_t x, y;

    Point();
    Point( int16_t, int16_t );

    bool operator==( const Point & ) const;
    bool operator!=( const Point & ) const;

    bool inABC( const Point &, const Point &, const Point & ) const;

    double distance( const Point & point ) const;
    Point rotate( double angle ) const;
    double getAngle( const Point & point ) const;

    Point & operator+=( const Point & );
    Point & operator-=( const Point & );

    Point operator+( const Point & ) const;
    Point operator-( const Point & ) const;
};

struct Size
{
    u16 w, h;

    Size( u16 width = 0, u16 height = 0 );
    Size( const Point & );

    bool operator==( const Size & ) const;
    bool operator!=( const Size & ) const;

    Size & operator+=( const Size & );
    Size & operator-=( const Size & );

    Size operator+( const Size & ) const;
    Size operator-( const Size & ) const;
};

struct Rect : Point, Size
{
    Rect();
    Rect( int16_t, int16_t, u16, u16 );
    Rect( const Point &, u16, u16 );
    Rect( const Point &, const Size & );

    // TODO: this method must be removed before merging to master
    Rect( const fheroes2::Rect & rect );

    Rect & operator=( const Point & );
    bool operator==( const Rect & ) const;
    bool operator!=( const Rect & ) const;

    // move position by offset
    Rect operator+( const Point & offset ) const;
    // rect include point
    bool operator&(const Point &)const;
    // rect intersects rect
    bool operator&(const Rect &)const;

    // calculate intersection rectangle
    Rect operator^( const Rect & other ) const;

    // explicit conversion
    const Point & getPosition() const;

    static Rect Get( const Point &, const Point & );
    static Rect Get( const Rect &, const Rect &, bool intersect );
    static std::pair<Rect, Point> Fixed4Blit( const Rect &, const Rect & );
};

struct Points : std::vector<Point>
{
    Rect GetRect( void ) const;
};

struct Rects : std::vector<Rect>
{
    s32 GetIndex( const Point & ) const;
    Rect GetRect( void ) const;
};

#endif
