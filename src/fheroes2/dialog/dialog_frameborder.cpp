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

#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "settings.h"
#include "text.h"

#define ANGLEWIDTH 44

Dialog::FrameBorder::FrameBorder( int v )
    : border( v )
    , restorer( fheroes2::Display::instance() )
{}

Dialog::FrameBorder::~FrameBorder()
{
    if ( Cursor::Get().isVisible() ) {
        Cursor::Get().Hide();
    };
}

Dialog::FrameBorder::FrameBorder( const Size & sz, const fheroes2::Image & sf )
    : border( BORDERWIDTH )
    , restorer( fheroes2::Display::instance() )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    SetPosition( ( display.width() - sz.w - border * 2 ) / 2, ( display.height() - sz.h - border * 2 ) / 2, sz.w, sz.h );
    RenderOther( sf, GetRect() );
}

Dialog::FrameBorder::FrameBorder( const Size & sz )
    : border( BORDERWIDTH )
    , restorer( fheroes2::Display::instance() )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    SetPosition( ( display.width() - sz.w - border * 2 ) / 2, ( display.height() - sz.h - border * 2 ) / 2, sz.w, sz.h );
    RenderRegular( GetRect() );
}

Dialog::FrameBorder::FrameBorder( s32 posx, s32 posy, u32 encw, u32 ench )
    : border( BORDERWIDTH )
    , restorer( fheroes2::Display::instance() )
{
    SetPosition( posx, posy, encw, ench );
    RenderRegular( GetRect() );
}

bool Dialog::FrameBorder::isValid() const
{
    return rect.w != 0 && rect.h != 0;
}

int Dialog::FrameBorder::BorderWidth( void ) const
{
    return border;
}

int Dialog::FrameBorder::BorderHeight( void ) const
{
    return border;
}

void Dialog::FrameBorder::SetPosition( s32 posx, s32 posy, u32 encw, u32 ench )
{
    restorer.restore();

    rect.x = posx;
    rect.y = posy;

    if ( encw && ench ) {
        rect.w = encw + 2 * border;
        rect.h = ench + 2 * border;

        restorer.update( rect.x, rect.y, rect.w, rect.h );

        area.w = encw;
        area.h = ench;
    }
    else
        restorer.update( posx, posy, restorer.width(), restorer.height() );

    area.x = posx + border;
    area.y = posy + border;

    top = Rect( posx, posy, area.w, border );
}

void Dialog::FrameBorder::SetBorder( int v )
{
    border = v;
}

const Rect & Dialog::FrameBorder::GetTop( void ) const
{
    return top;
}

const Rect & Dialog::FrameBorder::GetRect( void ) const
{
    return rect;
}

const Rect & Dialog::FrameBorder::GetArea( void ) const
{
    return area;
}

void Dialog::FrameBorder::RenderRegular( const Rect & dstrt )
{
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ( Settings::Get().ExtGameEvilInterface() ? ICN::SURDRBKE : ICN::SURDRBKG ), 0 );
    const uint32_t shadow = 16;
    const fheroes2::Image renderedImage = fheroes2::Stretch( sprite, shadow, 0, sprite.width() - shadow, sprite.height() - shadow, dstrt.w, dstrt.h );
    fheroes2::Blit( renderedImage, fheroes2::Display::instance(), dstrt.x, dstrt.y );
}

void Dialog::FrameBorder::RenderOther( const fheroes2::Image & srcsf, const Rect & dstrt )
{
    const fheroes2::Image renderedImage = fheroes2::Stretch( srcsf, 0, 0, srcsf.width(), srcsf.height(), dstrt.w, dstrt.h );
    fheroes2::Blit( renderedImage, fheroes2::Display::instance(), dstrt.x, dstrt.y );
}
