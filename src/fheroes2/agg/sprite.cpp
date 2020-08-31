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

#include "sprite.h"
#include "cursor.h"
#include "display.h"
#include "icn.h"
#include "pal.h"
#include "settings.h"

SpritePos::SpritePos() {}

SpritePos::SpritePos( const Surface & sf, const Point & pt )
    : Surface( sf )
    , pos( pt )
{}

const Point & SpritePos::GetPos( void ) const
{
    return pos;
}

Rect SpritePos::GetArea( void ) const
{
    return Rect( GetPos(), GetSize() );
}

void SpritePos::SetSurface( const Surface & sf )
{
    Surface::Set( sf, true );
}

void SpritePos::SetPos( const Point & pt )
{
    pos = pt;
}

void SpritePos::Reset( void )
{
    pos = Point( 0, 0 );
    Surface::Reset();
}

u32 SpritePos::GetMemoryUsage( void ) const
{
    return Surface::GetMemoryUsage() + sizeof( pos );
}

Sprite::Sprite() {}

Sprite::Sprite( const Surface & sf, s32 ox, s32 oy )
    : SpritePos( sf, Point( ox, oy ) )
{}

int Sprite::x( void ) const
{
    return pos.x;
}

int Sprite::y( void ) const
{
    return pos.y;
}

void Sprite::Blit( void ) const
{
    Blit( Display::Get() );
}

void Sprite::Blit( s32 dx, s32 dy ) const
{
    Blit( Point( dx, dy ), Display::Get() );
}

void Sprite::Blit( const Point & dpt ) const
{
    Blit( Rect( Point( 0, 0 ), GetSize() ), dpt, Display::Get() );
}

void Sprite::Blit( const Rect & srt, s32 dx, s32 dy ) const
{
    Blit( srt, Point( dx, dy ), Display::Get() );
}

void Sprite::Blit( const Rect & srt, const Point & dpt ) const
{
    Blit( srt, dpt, Display::Get() );
}
