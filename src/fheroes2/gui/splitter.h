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
#ifndef H2SPLITTER_H
#define H2SPLITTER_H

#include "gamedefs.h"
#include "ui_tool.h"

class Splitter : protected fheroes2::MovableSprite
{
public:
    Splitter();
    Splitter( const fheroes2::Image & image, const fheroes2::Rect & );

    void Forward( void );
    void Backward( void );
    void MoveIndex( int );
    void MoveCenter( void );

    void RedrawCursor( void );
    void HideCursor( void );
    void ShowCursor( void );

    void SetSprite( const fheroes2::Image & image );
    void SetArea( const fheroes2::Rect & );
    void SetRange( int smin, int smax );

    bool isVertical( void ) const;
    int GetCurrent( void ) const
    {
        return cur;
    }
    int GetStep( void ) const
    {
        return step;
    }
    int Max( void ) const
    {
        return max;
    }
    int Min( void ) const
    {
        return min;
    }

    const fheroes2::Rect & GetRect( void ) const
    {
        return area;
    }

private:
    fheroes2::Point GetPositionCursor();

    fheroes2::Rect area;
    int step;
    int min;
    int max;
    int cur;
};

#endif
