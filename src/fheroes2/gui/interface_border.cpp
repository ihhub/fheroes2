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
#include "agg_image.h"
#include "game_interface.h"
#include "icn.h"
#include "localevent.h"
#include "maps.h"
#include "settings.h"
#include "ui_tool.h"

void Interface::GameBorderRedraw( const bool viewWorldMode )
{
    const Settings & conf = Settings::Get();
    if ( conf.ExtGameHideInterface() && !viewWorldMode )
        return;

    const bool isEvilInterface = conf.ExtGameEvilInterface();

    fheroes2::Display & display = fheroes2::Display::instance();

    const int32_t displayWidth = display.width();
    const int32_t displayHeight = display.height();
    const int32_t extraDisplayWidth = displayWidth - fheroes2::Display::DEFAULT_WIDTH;
    const int32_t extraDisplayHeight = displayHeight - fheroes2::Display::DEFAULT_HEIGHT;

    const int32_t topRepeatCount = extraDisplayWidth > 0 ? extraDisplayWidth / TILEWIDTH : 0;
    const int32_t topRepeatWidth = ( topRepeatCount + 1 ) * TILEWIDTH;

    const int32_t vertRepeatCount = extraDisplayHeight > 0 ? extraDisplayHeight / TILEWIDTH : 0;
    const int32_t vertRepeatHeight = ( vertRepeatCount + 1 ) * TILEWIDTH;

    const int32_t iconsCount = vertRepeatCount > 3 ? 8 : ( vertRepeatCount < 3 ? 4 : 7 );

    const int32_t topPadWidth = extraDisplayWidth % TILEWIDTH;

    // top and bottom padding is split in two halves around the repeated "tiles"
    const int32_t topPadWidthLeft = topPadWidth / 2;
    const int32_t topPadWidthRight = topPadWidth - topPadWidthLeft;

    int32_t bottomTileWidth;
    int32_t bottomRepeatCount;
    if ( isEvilInterface ) {
        bottomTileWidth = 7; // width of a single bone piece
        bottomRepeatCount = extraDisplayWidth > 0 ? extraDisplayWidth / bottomTileWidth : 0;
    }
    else {
        bottomTileWidth = TILEWIDTH;
        bottomRepeatCount = topRepeatCount;
    }
    const int32_t bottomRepeatWidth = ( bottomRepeatCount + 1 ) * bottomTileWidth;

    const int32_t bottomPadWidth = extraDisplayWidth % bottomTileWidth;
    const int32_t bottomPadWidthLeft = bottomPadWidth / 2;
    const int32_t bottomPadWidthRight = bottomPadWidth - bottomPadWidthLeft;

    const int32_t vertPadHeight = extraDisplayHeight % TILEWIDTH;

    fheroes2::Rect srcrt;
    fheroes2::Point dstpt;
    const fheroes2::Sprite & icnadv = fheroes2::AGG::GetICN( isEvilInterface ? ICN::ADVBORDE : ICN::ADVBORD, 0 );

    // TOP BORDER
    srcrt.x = 0;
    srcrt.y = 0;
    srcrt.width = isEvilInterface ? 153 : 193;
    srcrt.height = BORDERWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    srcrt.x += srcrt.width;

    srcrt.width = 6;
    dstpt.x = srcrt.x;
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, srcrt.width + topPadWidthLeft, BORDERWIDTH );
    dstpt.x += srcrt.width + topPadWidthLeft;
    srcrt.x += srcrt.width;

    srcrt.width = isEvilInterface ? 64 : 24;
    srcrt.height = BORDERWIDTH;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;
    srcrt.x += srcrt.width;

    srcrt.width = TILEWIDTH;
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, topRepeatWidth, BORDERWIDTH );
    dstpt.x += topRepeatWidth;
    srcrt.x += TILEWIDTH;

    srcrt.width = isEvilInterface ? 65 : 25;
    srcrt.height = BORDERWIDTH;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;
    srcrt.x += srcrt.width;

    srcrt.width = 6;
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, srcrt.width + topPadWidthRight, BORDERWIDTH );
    srcrt.x += srcrt.width;

    srcrt.width = icnadv.width() - srcrt.x;
    dstpt.x = displayWidth - srcrt.width;
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
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, vertRepeatHeight );
    dstpt.y += vertRepeatHeight;
    srcrt.y += TILEWIDTH;

    srcrt.width = BORDERWIDTH;
    srcrt.height = isEvilInterface ? 33 : 123;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    srcrt.y += srcrt.height;
    dstpt.y += srcrt.height;

    srcrt.height = 8;
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, srcrt.height + vertPadHeight );
    srcrt.y += srcrt.height;

    srcrt.height = icnadv.height() - srcrt.y;
    dstpt.y = displayHeight - srcrt.height;
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
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, vertRepeatHeight );
    dstpt.y += vertRepeatHeight;
    srcrt.y += TILEWIDTH;

    srcrt.width = BORDERWIDTH;
    srcrt.height = isEvilInterface ? 33 : 125; // middle border is special on good interface due to all the green leaves
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // fix extra border part on higher resolutions
    if ( viewWorldMode || displayHeight > fheroes2::Display::DEFAULT_HEIGHT ) {
        fheroes2::Rect fixrt( 478, isEvilInterface ? 328 : 345, 3, isEvilInterface ? 15 : 20 );
        fheroes2::Point fixpt( dstpt.x + 14, dstpt.y + 18 );
        fheroes2::Blit( icnadv, fixrt.x, fixrt.y, display, fixpt.x, fixpt.y, fixrt.width, fixrt.height );
    }

    srcrt.y += srcrt.height;
    dstpt.y += srcrt.height;

    srcrt.height = 8;
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, srcrt.height + vertPadHeight );
    srcrt.y += srcrt.height;

    srcrt.height = icnadv.height() - srcrt.y;
    dstpt.y = displayHeight - srcrt.height;
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
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, vertRepeatHeight );
    dstpt.y += vertRepeatHeight;
    srcrt.y += TILEWIDTH;

    srcrt.width = BORDERWIDTH;
    srcrt.height = isEvilInterface ? 33 : 123;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // fix extra border part on higher resolutions
    if ( viewWorldMode || displayHeight > fheroes2::Display::DEFAULT_HEIGHT ) {
        fheroes2::Rect fixrt( 624, isEvilInterface ? 328 : 345, 3, isEvilInterface ? 15 : 20 );
        fheroes2::Point fixpt( dstpt.x, dstpt.y + 18 );
        fheroes2::Blit( icnadv, fixrt.x, fixrt.y, display, fixpt.x, fixpt.y, fixrt.width, fixrt.height );
    }

    srcrt.y += srcrt.height;
    dstpt.y += srcrt.height;

    srcrt.height = 8;
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, srcrt.height + vertPadHeight );
    srcrt.y += srcrt.height;

    srcrt.height = icnadv.height() - srcrt.y;
    dstpt.y = displayHeight - srcrt.height;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // BOTTOM BORDER
    srcrt.x = 0;
    srcrt.y = icnadv.height() - BORDERWIDTH;
    srcrt.width = isEvilInterface ? 129 : 193;
    srcrt.height = BORDERWIDTH;
    dstpt.x = srcrt.x;
    dstpt.y = displayHeight - BORDERWIDTH;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    srcrt.x += srcrt.width;

    srcrt.width = 6;
    dstpt.x = srcrt.x;
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, srcrt.width + bottomPadWidthLeft, BORDERWIDTH );
    dstpt.x += srcrt.width + bottomPadWidthLeft;
    srcrt.x += srcrt.width;

    srcrt.width = isEvilInterface ? 90 : 24;
    srcrt.height = BORDERWIDTH;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;
    srcrt.x += srcrt.width;

    srcrt.width = bottomTileWidth;
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, bottomRepeatWidth, BORDERWIDTH );
    dstpt.x += bottomRepeatWidth;
    srcrt.x += srcrt.width;

    srcrt.width = isEvilInterface ? 86 : 25; // evil bottom border is asymmetric
    srcrt.height = BORDERWIDTH;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;
    srcrt.x += srcrt.width;

    srcrt.width = 6;
    fheroes2::repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, srcrt.width + bottomPadWidthRight, BORDERWIDTH );
    srcrt.x += srcrt.width;

    srcrt.width = icnadv.width() - srcrt.x;
    dstpt.x = displayWidth - srcrt.width;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // ICON BORDER
    srcrt.x = icnadv.width() - RADARWIDTH - BORDERWIDTH;
    srcrt.y = RADARWIDTH + BORDERWIDTH;
    srcrt.width = RADARWIDTH;
    srcrt.height = BORDERWIDTH;
    dstpt.x = displayWidth - RADARWIDTH - BORDERWIDTH;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    if ( !viewWorldMode ) {
        dstpt.y = srcrt.y + BORDERWIDTH + iconsCount * 32;
        srcrt.y = srcrt.y + BORDERWIDTH + 4 * 32;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    }
}

Interface::BorderWindow::BorderWindow( const fheroes2::Rect & rt )
    : area( rt )
{}

const fheroes2::Rect & Interface::BorderWindow::GetRect() const
{
    return Settings::Get().ExtGameHideInterface() && border.isValid() ? border.GetRect() : GetArea();
}

const fheroes2::Rect & Interface::BorderWindow::GetArea() const
{
    return area;
}

void Interface::BorderWindow::Redraw() const
{
    Dialog::FrameBorder::RenderRegular( border.GetRect() );
}

void Interface::BorderWindow::SetPosition( int32_t px, int32_t py, uint32_t pw, uint32_t ph )
{
    area.width = pw;
    area.height = ph;

    SetPosition( px, py );
}

void Interface::BorderWindow::SetPosition( int32_t px, int32_t py )
{
    if ( Settings::Get().ExtGameHideInterface() ) {
        const fheroes2::Display & display = fheroes2::Display::instance();

        if ( px + area.width < 0 )
            px = 0;
        else if ( px > display.width() - area.width + border.BorderWidth() )
            px = display.width() - area.width;

        if ( py + area.height < 0 )
            py = 0;
        else if ( py > display.height() - area.height + border.BorderHeight() )
            py = display.height() - area.height;

        area.x = px + border.BorderWidth();
        area.y = py + border.BorderHeight();

        border.SetPosition( px, py, area.width, area.height );
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

        const fheroes2::Point & mp = le.GetMouseCursor();
        const fheroes2::Rect & pos = GetRect();

        fheroes2::MovableSprite moveIndicator( pos.width, pos.height, pos.x, pos.y );
        moveIndicator.reset();
        fheroes2::DrawBorder( moveIndicator, fheroes2::GetColorId( 0xD0, 0xC0, 0x48 ), 6 );

        const int32_t ox = mp.x - pos.x;
        const int32_t oy = mp.y - pos.y;

        moveIndicator.setPosition( pos.x, pos.y );
        moveIndicator.redraw();
        display.render();

        while ( le.HandleEvents() && le.MousePressLeft() ) {
            if ( le.MouseMotion() ) {
                moveIndicator.setPosition( mp.x - ox, mp.y - oy );
                display.render();
            }
        }

        SetPos( mp.x - ox, mp.y - oy );
        Interface::Basic::Get().SetRedraw( REDRAW_GAMEAREA );

        return true;
    }

    return false;
}
