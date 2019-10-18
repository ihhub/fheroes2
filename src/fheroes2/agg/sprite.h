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
#ifndef H2SPRITE_H
#define H2SPRITE_H

#include "gamedefs.h"

class Sprite : public SpritePos
{
public:
    Sprite();
    Sprite(const Surface &, s32, s32);

    int x(void) const;
    int y(void) const;

    using Surface::Blit;

    void Blit(void) const;
    void Blit(s32, s32) const;
    void Blit(const Point &) const;
    void Blit(const Rect & srt, s32, s32) const;
    void Blit(const Rect & srt, const Point &) const;

    void ChangeColorIndex(u32 index1, u32 index2);

    static Surface ScaleQVGASurface(const Surface &);
    static Sprite ScaleQVGASprite(const Sprite &);
};

#endif
