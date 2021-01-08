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

    const int32_t displayWidth = display.width();
    const int32_t displayHeight = display.height();
    int32_t count_w = ( displayWidth >= fheroes2::Display::DEFAULT_WIDTH ? displayWidth - fheroes2::Display::DEFAULT_WIDTH : 0 ) / TILEWIDTH;
    int32_t count_h = ( displayHeight >= fheroes2::Display::DEFAULT_HEIGHT ? displayHeight - fheroes2::Display::DEFAULT_HEIGHT : 0 ) / TILEWIDTH;
    const int32_t count_icons = count_h > 3 ? 8 : ( count_h < 3 ? 4 : 7 );

    if ( displayWidth % TILEWIDTH )
        ++count_w;
    if ( displayHeight % TILEWIDTH )
        ++count_h;

    fheroes2::Rect srcrt;
    fheroes2::Point dstpt;
    const fheroes2::Sprite & icnadv = fheroes2::AGG::GetICN( conf.ExtGameEvilInterface() ? ICN::ADVBORDE : ICN::ADVBORD, 0 );

    // TOP BORDER
    srcrt.x = 0;
    srcrt.y = 0;
    srcrt.width = 223;
    srcrt.height = BORDERWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    srcrt.x = 223;
    srcrt.width = TILEWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = 0;
    for ( int32_t i = 0; i < count_w + 1; ++i ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.x += TILEWIDTH;
    }
    srcrt.x += TILEWIDTH;
    srcrt.width = icnadv.width() - srcrt.x;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // LEFT BORDER
    srcrt.x = 0;
    srcrt.y = 0;
    srcrt.width = BORDERWIDTH;
    srcrt.height = 255;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    srcrt.y = 255;
    srcrt.height = TILEWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    for ( int32_t i = 0; i < count_h + 1; ++i ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;
    srcrt.height = icnadv.height() - srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // MIDDLE BORDER
    srcrt.x = icnadv.width() - RADARWIDTH - 2 * BORDERWIDTH;
    srcrt.y = 0;
    srcrt.width = BORDERWIDTH;
    srcrt.height = 255;
    dstpt.x = displayWidth - RADARWIDTH - 2 * BORDERWIDTH;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    srcrt.y = 255;
    srcrt.height = TILEWIDTH;
    dstpt.x = displayWidth - RADARWIDTH - 2 * BORDERWIDTH;
    dstpt.y = srcrt.y;
    for ( int32_t i = 0; i < count_h + 1; ++i ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;
    srcrt.height = icnadv.height() - srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // RIGHT BORDER
    srcrt.x = icnadv.width() - BORDERWIDTH;
    srcrt.y = 0;
    srcrt.width = BORDERWIDTH;
    srcrt.height = 255;
    dstpt.x = displayWidth - BORDERWIDTH;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    srcrt.y = 255;
    srcrt.height = TILEWIDTH;
    dstpt.x = displayWidth - BORDERWIDTH;
    dstpt.y = srcrt.y;
    for ( int32_t i = 0; i < count_h + 1; ++i ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;
    srcrt.height = icnadv.height() - srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // BOTTOM BORDER
    srcrt.x = 0;
    srcrt.y = icnadv.height() - BORDERWIDTH;
    srcrt.width = 223;
    srcrt.height = BORDERWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = displayHeight - BORDERWIDTH;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    srcrt.x = 223;
    srcrt.width = TILEWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = displayHeight - BORDERWIDTH;
    for ( int32_t i = 0; i < count_w + 1; ++i ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.x += TILEWIDTH;
    }
    srcrt.x += TILEWIDTH;
    srcrt.width = icnadv.width() - srcrt.x;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // ICON BORDER
    srcrt.x = icnadv.width() - RADARWIDTH - BORDERWIDTH;
    srcrt.y = RADARWIDTH + BORDERWIDTH;
    srcrt.width = RADARWIDTH;
    srcrt.height = BORDERWIDTH;
    dstpt.x = displayWidth - RADARWIDTH - BORDERWIDTH;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.y = srcrt.y + BORDERWIDTH + count_icons * 32;
    srcrt.y = srcrt.y + BORDERWIDTH + 4 * 32;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
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
        const fheroes2::Display & display = fheroes2::Display::instance();

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
    const Settings & conf = Settings::Get();
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
