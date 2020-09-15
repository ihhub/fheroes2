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

#include <iostream>

#include "cursor.h"
#include "settings.h"
#include "splitter.h"

/* splitter constructor */
Splitter::Splitter()
    : step( 0 )
    , min( 0 )
    , max( 0 )
    , cur( 0 )
{}

Splitter::Splitter( const fheroes2::Image & image, const Rect & rt )
    : fheroes2::MovableSprite( image )
    , area( rt )
    , step( 0 )
    , min( 0 )
    , max( 0 )
    , cur( 0 )
{}

void Splitter::SetSprite( const fheroes2::Image & image )
{
    fheroes2::Copy( image, *this );
}

void Splitter::SetArea( const Rect & rt )
{
    area = rt;
}

bool Splitter::isVertical( void ) const
{
    return area.w < area.h;
}

/* set range */
void Splitter::SetRange( int smin, int smax )
{
    min = smin;
    max = smax;
    Point move;

    if ( min < max ) {
        step = 100 * ( isVertical() ? ( area.h - height() ) : ( area.w - width() ) ) / ( max - min );
        cur = min;
        move = GetPositionCursor();
    }
    else {
        step = 0;
        move = Point( area.x + ( area.w - width() ) / 2, area.y + ( area.h - height() ) / 2 );
    }

    setPosition( move.x, move.y );
}

Point Splitter::GetPositionCursor( void )
{
    Point res;

    if ( isVertical() ) {
        res.x = area.x + ( area.w - width() ) / 2;
        res.y = area.y + cur * step / 100;
    }
    else {
        res.x = area.x + cur * step / 100;
        res.y = area.y + ( area.h - height() ) / 2;
    }

    return res;
}

void Splitter::RedrawCursor( void )
{
    redraw();
}

void Splitter::HideCursor( void )
{
    hide();
}

void Splitter::ShowCursor( void )
{
    show();
}

void Splitter::MoveCenter( void )
{
    setPosition( area.x + ( area.w - width() ) / 2, area.y + ( area.h - height() ) / 2 );
}

/* move splitter to pos */
void Splitter::MoveIndex( int num )
{
    if ( num > max || num < min ) {
        DEBUG( DBG_ENGINE, DBG_WARN,
               "out of range"
                   << ": " << num << ", min: " << min << ", max: " << max << ", cur: " << cur << ", step: " << step );
    }
    else {
        cur = num;
        setPosition( GetPositionCursor().x, GetPositionCursor().y );
    }
}

/* forward spliter */
void Splitter::Forward( void )
{
    if ( cur != max ) {
        ++cur;
        setPosition( GetPositionCursor().x, GetPositionCursor().y );
    }
}

/* backward spliter */
void Splitter::Backward( void )
{
    if ( cur ) {
        --cur;
        setPosition( GetPositionCursor().x, GetPositionCursor().y );
    }
}
