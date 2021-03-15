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
#include "screen.h"
#include "settings.h"

Dialog::FrameBorder::FrameBorder( int v )
    : restorer( fheroes2::Display::instance(), 0, 0, 0, 0 )
    , border( v )
{}

Dialog::FrameBorder::FrameBorder( const fheroes2::Size & sz, const fheroes2::Image & sf )
    : restorer( fheroes2::Display::instance(), 0, 0, 0, 0 )
    , border( BORDERWIDTH )
{
    const fheroes2::Display & display = fheroes2::Display::instance();
    SetPosition( ( display.width() - sz.width - border * 2 ) / 2, ( display.height() - sz.height - border * 2 ) / 2, sz.width, sz.height );
    const Rect & currentArea = GetRect();
    RenderOther( sf, fheroes2::Rect( currentArea.x, currentArea.y, currentArea.w, currentArea.h ) );
}

Dialog::FrameBorder::FrameBorder( const fheroes2::Size & sz )
    : restorer( fheroes2::Display::instance(), 0, 0, 0, 0 )
    , border( BORDERWIDTH )
{
    const fheroes2::Display & display = fheroes2::Display::instance();
    SetPosition( ( display.width() - sz.width - border * 2 ) / 2, ( display.height() - sz.height - border * 2 ) / 2, sz.width, sz.height );
    RenderRegular( GetRect() );
}

Dialog::FrameBorder::FrameBorder( s32 posx, s32 posy, u32 encw, u32 ench )
    : restorer( fheroes2::Display::instance(), 0, 0, 0, 0 )
    , border( BORDERWIDTH )
{
    SetPosition( posx, posy, encw, ench );
    RenderRegular( GetRect() );
}

Dialog::FrameBorder::~FrameBorder()
{
    if ( Cursor::Get().isVisible() ) {
        Cursor::Get().Hide();
    }
}

bool Dialog::FrameBorder::isValid() const
{
    return _rect.width != 0 && _rect.height != 0;
}

int Dialog::FrameBorder::BorderWidth() const
{
    return border;
}

int Dialog::FrameBorder::BorderHeight() const
{
    return border;
}

void Dialog::FrameBorder::SetPosition( int32_t posx, int32_t posy, uint32_t encw, uint32_t ench )
{
    restorer.restore();

    _rect.x = posx;
    _rect.y = posy;

    if ( encw > 0 && ench > 0 ) {
        _rect.width = encw + 2 * border;
        _rect.height = ench + 2 * border;

        restorer.update( _rect.x, _rect.y, _rect.width, _rect.height );

        _area.width = encw;
        _area.height = ench;
    }
    else {
        restorer.update( posx, posy, restorer.width(), restorer.height() );
    }

    _area.x = posx + border;
    _area.y = posy + border;

    _top = fheroes2::Rect( posx, posy, _area.width, border );
}

const fheroes2::Rect & Dialog::FrameBorder::GetTop() const
{
    return _top;
}

const fheroes2::Rect & Dialog::FrameBorder::GetRect() const
{
    return _rect;
}

const fheroes2::Rect & Dialog::FrameBorder::GetArea() const
{
    return _area;
}

void Dialog::FrameBorder::RenderRegular( const fheroes2::Rect & dstrt )
{
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ( Settings::Get().ExtGameEvilInterface() ? ICN::SURDRBKE : ICN::SURDRBKG ), 0 );
    const fheroes2::Image renderedImage =
        fheroes2::Stretch( sprite, SHADOWWIDTH, 0, sprite.width() - SHADOWWIDTH, sprite.height() - SHADOWWIDTH, dstrt.width, dstrt.height );
    fheroes2::Blit( renderedImage, fheroes2::Display::instance(), dstrt.x, dstrt.y );
}

void Dialog::FrameBorder::RenderOther( const fheroes2::Image & srcsf, const fheroes2::Rect & dstrt )
{
    const fheroes2::Image renderedImage = fheroes2::Stretch( srcsf, 0, 0, srcsf.width(), srcsf.height(), dstrt.width, dstrt.height );
    fheroes2::Blit( renderedImage, fheroes2::Display::instance(), dstrt.x, dstrt.y );
}
