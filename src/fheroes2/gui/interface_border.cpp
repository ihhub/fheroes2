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

#include "interface_border.h"
#include "agg.h"
#include "game_interface.h"
#include "maps.h"
#include "settings.h"
#include "ui_tool.h"

void Interface::GameBorderRedraw( void )
{
    const Settings & conf = Settings::Get();
    if ( conf.ExtGameHideInterface() )
        return;

    fheroes2::Display & display = fheroes2::Display::instance();

    const bool evil = Settings::Get().ExtGameEvilInterface();
    const int displayWidth = display.width();
    const int displayHeight = display.height();
    u32 count_w = ( displayWidth >= fheroes2::Display::DEFAULT_WIDTH ? displayWidth - fheroes2::Display::DEFAULT_WIDTH : 0 ) / TILEWIDTH;
    u32 count_h = ( displayHeight >= fheroes2::Display::DEFAULT_HEIGHT ? displayHeight - fheroes2::Display::DEFAULT_HEIGHT : 0 ) / TILEWIDTH;
    const u32 count_icons = count_h > 3 ? 8 : ( count_h < 3 ? 4 : 7 );

    if ( displayWidth % TILEWIDTH )
        ++count_w;
    if ( displayHeight % TILEWIDTH )
        ++count_h;

    Rect srcrt;
    Point dstpt;
    const fheroes2::Sprite & icnadv = fheroes2::AGG::GetICN( evil ? ICN::ADVBORDE : ICN::ADVBORD, 0 );

    // TOP BORDER
    srcrt.x = 0;
    srcrt.y = 0;
    srcrt.w = 223;
    srcrt.h = BORDERWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );

    srcrt.x = 223;
    srcrt.w = TILEWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = 0;
    for ( u32 ii = 0; ii < count_w + 1; ++ii ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );
        dstpt.x += TILEWIDTH;
    }
    srcrt.x += TILEWIDTH;
    srcrt.w = icnadv.width() - srcrt.x;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );

    // LEFT BORDER
    srcrt.x = 0;
    srcrt.y = 0;
    srcrt.w = BORDERWIDTH;
    srcrt.h = 255;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );
    srcrt.y = 255;
    srcrt.h = TILEWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    for ( u32 ii = 0; ii < count_h + 1; ++ii ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );
        dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;
    srcrt.h = icnadv.height() - srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );

    // MIDDLE BORDER
    srcrt.x = icnadv.width() - RADARWIDTH - 2 * BORDERWIDTH;
    srcrt.y = 0;
    srcrt.w = BORDERWIDTH;
    srcrt.h = 255;
    dstpt.x = displayWidth - RADARWIDTH - 2 * BORDERWIDTH;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );
    srcrt.y = 255;
    srcrt.h = TILEWIDTH;
    dstpt.x = displayWidth - RADARWIDTH - 2 * BORDERWIDTH;
    dstpt.y = srcrt.y;
    for ( u32 ii = 0; ii < count_h + 1; ++ii ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );
        dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;
    srcrt.h = icnadv.height() - srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );

    // RIGHT BORDER
    srcrt.x = icnadv.width() - BORDERWIDTH;
    srcrt.y = 0;
    srcrt.w = BORDERWIDTH;
    srcrt.h = 255;
    dstpt.x = displayWidth - BORDERWIDTH;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );
    srcrt.y = 255;
    srcrt.h = TILEWIDTH;
    dstpt.x = displayWidth - BORDERWIDTH;
    dstpt.y = srcrt.y;
    for ( u32 ii = 0; ii < count_h + 1; ++ii ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );
        dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;
    srcrt.h = icnadv.height() - srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );

    // BOTTOM BORDER
    srcrt.x = 0;
    srcrt.y = icnadv.height() - BORDERWIDTH;
    srcrt.w = 223;
    srcrt.h = BORDERWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = displayHeight - BORDERWIDTH;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );
    srcrt.x = 223;
    srcrt.w = TILEWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = displayHeight - BORDERWIDTH;
    for ( u32 ii = 0; ii < count_w + 1; ++ii ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );
        dstpt.x += TILEWIDTH;
    }
    srcrt.x += TILEWIDTH;
    srcrt.w = icnadv.width() - srcrt.x;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );

    // ICON BORDER
    srcrt.x = icnadv.width() - RADARWIDTH - BORDERWIDTH;
    srcrt.y = RADARWIDTH + BORDERWIDTH;
    srcrt.w = RADARWIDTH;
    srcrt.h = BORDERWIDTH;
    dstpt.x = displayWidth - RADARWIDTH - BORDERWIDTH;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );
    dstpt.y = srcrt.y + BORDERWIDTH + count_icons * 32;
    srcrt.y = srcrt.y + BORDERWIDTH + 4 * 32;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.w, srcrt.h );
}

Interface::BorderWindow::BorderWindow( const Rect & rt )
    : area( rt )
{}

const Rect & Interface::BorderWindow::GetRect( void ) const
{
    return Settings::Get().ExtGameHideInterface() && border.isValid() ? border.GetRect() : GetArea();
}

const Rect & Interface::BorderWindow::GetArea( void ) const
{
    return area;
}

void Interface::BorderWindow::Redraw( void )
{
    Dialog::FrameBorder::RenderRegular( border.GetRect() );
}

void Interface::BorderWindow::SetPosition( s32 px, s32 py, u32 pw, u32 ph )
{
    area.w = pw;
    area.h = ph;

    SetPosition( px, py );
}

void Interface::BorderWindow::SetPosition( s32 px, s32 py )
{
    if ( Settings::Get().ExtGameHideInterface() ) {
        fheroes2::Display & display = fheroes2::Display::instance();

        if ( px + area.w < 0 )
            px = 0;
        else if ( px > display.width() - area.w + border.BorderWidth() )
            px = display.width() - area.w;

        if ( py + area.h < 0 )
            py = 0;
        else if ( py > display.height() - area.h + border.BorderHeight() )
            py = display.height() - area.h;

        area.x = px + border.BorderWidth();
        area.y = py + border.BorderHeight();

        border.SetPosition( px, py, area.w, area.h );
        SavePosition();
    }
    else {
        area.x = px;
        area.y = py;
    }
}

bool Interface::BorderWindow::QueueEventProcessing( void )
{
    Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();

    if ( conf.ExtGameHideInterface() && le.MousePressLeft( border.GetTop() ) ) {
        fheroes2::Display & display = fheroes2::Display::instance();
        Cursor & cursor = Cursor::Get();

        const Point & mp = le.GetMouseCursor();
        const Rect & pos = GetRect();

        fheroes2::MovableSprite moveIndicator( pos.w, pos.h, pos.x, pos.y );
        moveIndicator.reset();
        fheroes2::DrawBorder( moveIndicator, fheroes2::GetColorId( 0xD0, 0xC0, 0x48 ), 6 );

        const s32 ox = mp.x - pos.x;
        const s32 oy = mp.y - pos.y;

        cursor.Hide();
        moveIndicator.setPosition( pos.x, pos.y );
        moveIndicator.redraw();
        cursor.Show();
        display.render();

        while ( le.HandleEvents() && le.MousePressLeft() ) {
            if ( le.MouseMotion() ) {
                cursor.Hide();
                moveIndicator.setPosition( mp.x - ox, mp.y - oy );
                cursor.Show();
                display.render();
            }
        }

        cursor.Hide();
        SetPos( mp.x - ox, mp.y - oy );
        Interface::Basic::Get().SetRedraw( REDRAW_GAMEAREA );

        return true;
    }

    return false;
}
