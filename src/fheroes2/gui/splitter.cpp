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

Splitter::Splitter()
    : step( 0 )
    , min( 0 )
    , max( 0 )
    , cur( 0 )
{}

Splitter::Splitter( const fheroes2::Image & image, const fheroes2::Rect & rt )
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

void Splitter::SetArea( const fheroes2::Rect & rt )
{
    area = rt;
}

bool Splitter::isVertical( void ) const
{
    return area.width < area.height;
}

/* set range */
void Splitter::SetRange( int smin, int smax )
{
    min = smin;
    max = smax;
    fheroes2::Point move;

    if ( min < max ) {
        step = 100 * ( isVertical() ? ( area.height - height() ) : ( area.width - width() ) ) / ( max - min );
        cur = min;
        move = GetPositionCursor();
    }
    else {
        step = 0;
        move = fheroes2::Point( area.x + ( area.width - width() ) / 2, area.y + ( area.height - height() ) / 2 );
    }

    setPosition( move.x, move.y );
}

fheroes2::Point Splitter::GetPositionCursor()
{
    if ( isVertical() ) {
        return fheroes2::Point( area.x + ( area.width - width() ) / 2, area.y + cur * step / 100 );
    }
    else {
        return fheroes2::Point( area.x + cur * step / 100, area.y + ( area.height - height() ) / 2 );
    }
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
    setPosition( area.x + ( area.width - width() ) / 2, area.y + ( area.height - height() ) / 2 );
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
        const fheroes2::Point & position = GetPositionCursor();
        setPosition( position.x, position.y );
    }
}

void Splitter::MoveToPos( const Point & pos )
{
    if ( isVertical() ) {
        int32_t posY = pos.y;
        const int32_t maxYPos = area.y + ( max - min ) * step / 100;

        if ( posY < area.y )
            posY = area.y;
        else if ( posY > maxYPos )
            posY = maxYPos;

        cur = ( ( posY - area.y ) * 100 + step / 2 ) / step;

        const int32_t posX = area.x + ( area.width - width() ) / 2;
        setPosition( posX, posY );
    }
    else {
        int32_t posX = pos.x;
        const int32_t maxXPos = area.x + ( max - min ) * step / 100;

        if ( posX < area.x )
            posX = area.x;
        else if ( posX > maxXPos )
            posX = maxXPos;

         cur = ( ( posX - area.x ) * 100 + step / 2 ) / step;
        const int32_t posY = area.y + ( area.height - height() ) / 2;
        setPosition( posX, posY );
    }
}

/* forward spliter */
void Splitter::Forward( void )
{
    if ( cur != max ) {
        ++cur;
        const fheroes2::Point & position = GetPositionCursor();
        setPosition( position.x, position.y );
    }
}

/* backward spliter */
void Splitter::Backward( void )
{
    if ( cur ) {
        --cur;
        const fheroes2::Point & position = GetPositionCursor();
        setPosition( position.x, position.y );
    }
}
