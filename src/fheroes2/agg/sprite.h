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

class SpritePos : public Surface
{
public:
    SpritePos();
    SpritePos( const Surface &, const Point & );

    void SetSurface( const Surface & );
    void SetPos( const Point & );

    void Reset( void );

    const Point & GetPos( void ) const;
    Rect GetArea( void ) const;

    u32 GetMemoryUsage( void ) const;

protected:
    Point pos;
};

class Sprite : public SpritePos
{
public:
    Sprite();
    Sprite( const Surface &, s32, s32 );

    int x( void ) const;
    int y( void ) const;
};

#endif
