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

#ifndef H2SPRITES_H
#define H2SPRITES_H

#include "display.h"
#include "surface.h"

class SpritePos : public Surface
{
public:
    SpritePos();
    SpritePos(const Surface &, const Point &);

    void SetSurface(const Surface &);
    void SetPos(const Point &);

    void Reset(void);

    const Point & GetPos(void) const;
    Rect GetArea(void) const;

    u32  GetMemoryUsage(void) const;

protected:
    Point pos;
};

class SpriteBack : protected Surface
{
public:
    SpriteBack();
    SpriteBack(const Rect &);

    bool isValid(void) const;

    void Save(const Point &);
    void Save(const Rect &);
    void Restore(void);
    void Destroy(void);
    void SetPos(const Point &);

    const Point & GetPos(void) const;
    const Size & GetSize(void) const;
    const Rect & GetArea(void) const;

    u32  GetMemoryUsage(void) const;

protected:
    Rect pos;
};


class SpriteMove : public Surface
{
public:
    SpriteMove();
    SpriteMove(const Surface &);

    void Move(const Point &);
    void Move(int, int);

    void Hide(void);
    void Show(void);
    void Redraw(void);

    bool isVisible(void) const;

    const Point & GetPos(void) const;
    const Rect & GetArea(void) const;

    u32  GetMemoryUsage(void) const;

protected:
    void Show(const Point &);

    SpriteBack background;
    u32 mode;
};

#endif
