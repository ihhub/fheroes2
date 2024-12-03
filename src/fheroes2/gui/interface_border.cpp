/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>

#include "agg_image.h"
#include "cursor.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "screen.h"
#include "settings.h"
#include "ui_constants.h"
#include "ui_tool.h"

namespace
{
    void repeatPattern( const fheroes2::Image & in, int32_t inX, int32_t inY, int32_t inWidth, int32_t inHeight, fheroes2::Image & out, int32_t outX, int32_t outY,
                        int32_t width, int32_t height )
    {
        if ( inX < 0 || inY < 0 || outX < 0 || outY < 0 || inWidth <= 0 || inHeight <= 0 || width <= 0 || height <= 0 )
            return;

        const int32_t countX = width / inWidth;
        const int32_t countY = height / inHeight;
        const int32_t restWidth = width % inWidth;
        const int32_t restHeight = height % inHeight;

        for ( int32_t y = 0; y < countY; ++y ) {
            for ( int32_t x = 0; x < countX; ++x ) {
                fheroes2::Blit( in, inX, inY, out, outX + x * inWidth, outY + y * inHeight, inWidth, inHeight );
            }
            if ( restWidth != 0 ) {
                fheroes2::Blit( in, inX, inY, out, outX + width - restWidth, outY + y * inHeight, restWidth, inHeight );
            }
        }
        if ( restHeight != 0 ) {
            for ( int32_t x = 0; x < countX; ++x ) {
                fheroes2::Blit( in, inX, inY, out, outX + x * inWidth, outY + height - restHeight, inWidth, restHeight );
            }
            if ( restWidth != 0 ) {
                fheroes2::Blit( in, inX, inY, out, outX + width - restWidth, outY + height - restHeight, restWidth, restHeight );
            }
        }
    }
}

void Interface::GameBorderRedraw( const bool viewWorldMode )
{
    const Settings & conf = Settings::Get();
    if ( conf.isHideInterfaceEnabled() && !viewWorldMode )
        return;

    const bool isEvilInterface = conf.isEvilInterfaceEnabled();

    fheroes2::Display & display = fheroes2::Display::instance();

    const int32_t displayWidth = display.width();
    const int32_t displayHeight = display.height();
    const int32_t extraDisplayWidth = displayWidth - fheroes2::Display::DEFAULT_WIDTH;
    const int32_t extraDisplayHeight = displayHeight - fheroes2::Display::DEFAULT_HEIGHT;

    const int32_t topRepeatCount = extraDisplayWidth > 0 ? extraDisplayWidth / fheroes2::tileWidthPx : 0;
    const int32_t topRepeatWidth = ( topRepeatCount + 1 ) * fheroes2::tileWidthPx;

    const int32_t vertRepeatCount = extraDisplayHeight > 0 ? extraDisplayHeight / fheroes2::tileWidthPx : 0;
    const int32_t iconsCount = vertRepeatCount > 3 ? 8 : ( vertRepeatCount < 3 ? 4 : 7 );

    const int32_t vertRepeatHeight = ( vertRepeatCount + 1 ) * fheroes2::tileWidthPx;
    const int32_t vertRepeatHeightTop = ( iconsCount - 3 ) * fheroes2::tileWidthPx;
    const int32_t vertRepeatHeightBottom = vertRepeatHeight - vertRepeatHeightTop;

    const int32_t topPadWidth = extraDisplayWidth % fheroes2::tileWidthPx;

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
        bottomTileWidth = fheroes2::tileWidthPx;
        bottomRepeatCount = topRepeatCount;
    }
    const int32_t bottomRepeatWidth = ( bottomRepeatCount + 1 ) * bottomTileWidth;

    const int32_t bottomPadWidth = extraDisplayWidth % bottomTileWidth;
    const int32_t bottomPadWidthLeft = bottomPadWidth / 2;
    const int32_t bottomPadWidthRight = bottomPadWidth - bottomPadWidthLeft;

    const int32_t vertPadHeight = extraDisplayHeight % fheroes2::tileWidthPx;

    fheroes2::Rect srcrt;
    fheroes2::Point dstpt;
    const fheroes2::Sprite & icnadv = fheroes2::AGG::GetICN( isEvilInterface ? ICN::ADVBORDE : ICN::ADVBORD, 0 );

    // TOP BORDER
    srcrt.x = 0;
    srcrt.y = 0;
    srcrt.width = isEvilInterface ? 153 : 193;
    srcrt.height = fheroes2::borderWidthPx;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    srcrt.x += srcrt.width;

    srcrt.width = 6;
    dstpt.x = srcrt.x;
    repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, srcrt.width + topPadWidthLeft, fheroes2::borderWidthPx );
    dstpt.x += srcrt.width + topPadWidthLeft;
    srcrt.x += srcrt.width;

    srcrt.width = isEvilInterface ? 64 : 24;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;
    srcrt.x += srcrt.width;

    srcrt.width = fheroes2::tileWidthPx;
    repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, topRepeatWidth, fheroes2::borderWidthPx );
    dstpt.x += topRepeatWidth;
    srcrt.x += fheroes2::tileWidthPx;

    srcrt.width = isEvilInterface ? 65 : 25;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;
    srcrt.x += srcrt.width;

    srcrt.width = 6;
    repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, srcrt.width + topPadWidthRight, fheroes2::borderWidthPx );
    dstpt.x += srcrt.width + topPadWidthRight;
    srcrt.x += srcrt.width;

    srcrt.width = icnadv.width() - srcrt.x;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // LEFT BORDER
    srcrt.x = 0;
    srcrt.y = fheroes2::borderWidthPx;
    srcrt.width = fheroes2::borderWidthPx;
    srcrt.height = 255 - fheroes2::borderWidthPx;
    dstpt.x = srcrt.x;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.y += srcrt.height;
    srcrt.y += srcrt.height;

    if ( isEvilInterface ) {
        srcrt.height = fheroes2::tileWidthPx;
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, vertRepeatHeightTop );
        dstpt.y += vertRepeatHeightTop;
        srcrt.y += fheroes2::tileWidthPx;

        srcrt.width = fheroes2::borderWidthPx;
        srcrt.height = 35;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
        srcrt.y += srcrt.height;

        srcrt.height = 6;
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, srcrt.height + vertPadHeight );
        dstpt.y += srcrt.height + vertPadHeight;
        srcrt.y += srcrt.height;

        srcrt.height = 103;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
        srcrt.y += srcrt.height;

        srcrt.height = fheroes2::tileWidthPx;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += fheroes2::tileWidthPx;

        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, vertRepeatHeightBottom );
        dstpt.y += vertRepeatHeightBottom;
        srcrt.y += fheroes2::tileWidthPx;
    }
    else {
        srcrt.height = fheroes2::tileWidthPx;
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, vertRepeatHeight );
        dstpt.y += vertRepeatHeight;
        srcrt.y += fheroes2::tileWidthPx;

        srcrt.height = 125;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
        srcrt.y += srcrt.height;

        srcrt.height = 4;
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, srcrt.height + vertPadHeight );
        dstpt.y += srcrt.height + vertPadHeight;
        srcrt.y += srcrt.height;
    }

    srcrt.height = icnadv.height() - fheroes2::borderWidthPx - srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // MIDDLE BORDER
    srcrt.x = icnadv.width() - fheroes2::radarWidthPx - 2 * fheroes2::borderWidthPx;
    srcrt.y = fheroes2::borderWidthPx;
    srcrt.width = fheroes2::borderWidthPx;
    srcrt.height = 255 - fheroes2::borderWidthPx;
    dstpt.x = displayWidth - fheroes2::radarWidthPx - 2 * fheroes2::borderWidthPx;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.y += srcrt.height;
    srcrt.y += srcrt.height;

    srcrt.height = fheroes2::tileWidthPx;
    repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, vertRepeatHeightTop );
    dstpt.y += vertRepeatHeightTop;
    srcrt.y += fheroes2::tileWidthPx;

    srcrt.height = isEvilInterface ? 35 : 50;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // hide embranchment
    if ( viewWorldMode ) {
        fheroes2::Rect fixrt( 478, isEvilInterface ? 137 : 345, 3, isEvilInterface ? 15 : 20 );
        fheroes2::Point fixpt( dstpt.x + 14, dstpt.y + 18 );
        fheroes2::Blit( icnadv, fixrt.x, fixrt.y, display, fixpt.x, fixpt.y, fixrt.width, fixrt.height );
    }

    dstpt.y += srcrt.height;
    srcrt.y += srcrt.height;

    if ( isEvilInterface ) {
        srcrt.height = 6;
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, srcrt.height + vertPadHeight );
        dstpt.y += srcrt.height + vertPadHeight;
        srcrt.y += srcrt.height;

        srcrt.height = 103;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
        srcrt.y += srcrt.height;

        srcrt.height = fheroes2::tileWidthPx;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += fheroes2::tileWidthPx;

        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, vertRepeatHeightBottom );
        dstpt.y += vertRepeatHeightBottom;
        srcrt.y += fheroes2::tileWidthPx;
    }
    else {
        srcrt.height = fheroes2::tileWidthPx;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += fheroes2::tileWidthPx;

        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, vertRepeatHeightBottom );
        dstpt.y += vertRepeatHeightBottom;
        srcrt.y += fheroes2::tileWidthPx;

        srcrt.height = 43;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
        srcrt.y += srcrt.height;

        srcrt.height = 8; // middle border is special on good interface due to all the green leaves
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, srcrt.height + vertPadHeight );
        dstpt.y += srcrt.height + vertPadHeight;
        srcrt.y += srcrt.height;
    }

    srcrt.height = icnadv.height() - fheroes2::borderWidthPx - srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // RIGHT BORDER
    srcrt.x = icnadv.width() - fheroes2::borderWidthPx;
    srcrt.y = fheroes2::borderWidthPx;
    srcrt.width = fheroes2::borderWidthPx;
    srcrt.height = 255 - fheroes2::borderWidthPx;
    dstpt.x = displayWidth - fheroes2::borderWidthPx;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.y += srcrt.height;
    srcrt.y += srcrt.height;

    srcrt.height = fheroes2::tileWidthPx;
    repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, vertRepeatHeightTop );
    dstpt.y += vertRepeatHeightTop;
    srcrt.y += fheroes2::tileWidthPx;

    srcrt.height = isEvilInterface ? 35 : 50;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // hide embranchment
    if ( viewWorldMode ) {
        fheroes2::Rect fixrt( 624, isEvilInterface ? 139 : 345, 3, isEvilInterface ? 15 : 20 );
        fheroes2::Point fixpt( dstpt.x, dstpt.y + 18 );
        fheroes2::Blit( icnadv, fixrt.x, fixrt.y, display, fixpt.x, fixpt.y, fixrt.width, fixrt.height );
    }

    dstpt.y += srcrt.height;
    srcrt.y += srcrt.height;

    if ( isEvilInterface ) {
        srcrt.height = 6;
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, srcrt.height + vertPadHeight );
        dstpt.y += srcrt.height + vertPadHeight;
        srcrt.y += srcrt.height;

        srcrt.height = 103;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
        srcrt.y += srcrt.height;

        srcrt.height = fheroes2::tileWidthPx;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += fheroes2::tileWidthPx;

        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, vertRepeatHeightBottom );
        dstpt.y += vertRepeatHeightBottom;
        srcrt.y += fheroes2::tileWidthPx;
    }
    else {
        srcrt.height = fheroes2::tileWidthPx;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += fheroes2::tileWidthPx;

        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, vertRepeatHeightBottom );
        dstpt.y += vertRepeatHeightBottom;
        srcrt.y += fheroes2::tileWidthPx;

        srcrt.height = 43;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
        srcrt.y += srcrt.height;

        srcrt.height = 4;
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, fheroes2::borderWidthPx, srcrt.height + vertPadHeight );
        dstpt.y += srcrt.height + vertPadHeight;
        srcrt.y += srcrt.height;
    }

    srcrt.height = icnadv.height() - fheroes2::borderWidthPx - srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // BOTTOM BORDER
    srcrt.x = 0;
    srcrt.y = icnadv.height() - fheroes2::borderWidthPx;
    srcrt.width = isEvilInterface ? 129 : 193;
    srcrt.height = fheroes2::borderWidthPx;
    dstpt.x = srcrt.x;
    dstpt.y = displayHeight - fheroes2::borderWidthPx;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;
    srcrt.x += srcrt.width;

    srcrt.width = 6;
    repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, srcrt.width + bottomPadWidthLeft, fheroes2::borderWidthPx );
    dstpt.x += srcrt.width + bottomPadWidthLeft;
    srcrt.x += srcrt.width;

    srcrt.width = isEvilInterface ? 90 : 24;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;
    srcrt.x += srcrt.width;

    srcrt.width = bottomTileWidth;
    repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, bottomRepeatWidth, fheroes2::borderWidthPx );
    dstpt.x += bottomRepeatWidth;
    srcrt.x += srcrt.width;

    srcrt.width = isEvilInterface ? 86 : 25; // evil bottom border is asymmetric
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;
    srcrt.x += srcrt.width;

    srcrt.width = 6;
    repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, srcrt.width + bottomPadWidthRight, fheroes2::borderWidthPx );
    dstpt.x += srcrt.width + bottomPadWidthRight;
    srcrt.x += srcrt.width;

    srcrt.width = icnadv.width() - srcrt.x;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // ICON BORDER
    srcrt.x = icnadv.width() - fheroes2::radarWidthPx - fheroes2::borderWidthPx;
    srcrt.y = fheroes2::radarWidthPx + fheroes2::borderWidthPx;
    srcrt.width = fheroes2::radarWidthPx;
    srcrt.height = fheroes2::borderWidthPx;
    dstpt.x = displayWidth - fheroes2::radarWidthPx - fheroes2::borderWidthPx;
    dstpt.y = srcrt.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    if ( !viewWorldMode ) {
        dstpt.y = srcrt.y + fheroes2::borderWidthPx + iconsCount * 32;
        srcrt.y = srcrt.y + fheroes2::borderWidthPx + 4 * 32;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    }
}

Interface::BorderWindow::BorderWindow( const fheroes2::Rect & rt )
    : area( rt )
{}

const fheroes2::Rect & Interface::BorderWindow::GetRect() const
{
    return Settings::Get().isHideInterfaceEnabled() && border.isValid() ? border.GetRect() : GetArea();
}

bool Interface::BorderWindow::isMouseCaptured()
{
    if ( !_isMouseCaptured ) {
        return false;
    }

    const LocalEvent & le = LocalEvent::Get();

    _isMouseCaptured = le.isMouseLeftButtonPressed();

    // Even if the mouse has just been released from the capture, consider it still captured at this
    // stage to ensure that events directly related to the release (for instance, releasing the mouse
    // button) will not be handled by other UI elements.
    return true;
}

void Interface::BorderWindow::Redraw() const
{
    Dialog::FrameBorder::RenderRegular( border.GetRect() );
}

void Interface::BorderWindow::SetPosition( const int32_t x, const int32_t y, const int32_t width, const int32_t height )
{
    area.width = width;
    area.height = height;

    SetPosition( x, y );
}

void Interface::BorderWindow::SetPosition( int32_t x, int32_t y )
{
    if ( Settings::Get().isHideInterfaceEnabled() ) {
        const fheroes2::Display & display = fheroes2::Display::instance();

        x = std::max( 0, std::min( x, display.width() - ( area.width + border.BorderWidth() * 2 ) ) );
        y = std::max( 0, std::min( y, display.height() - ( area.height + border.BorderHeight() * 2 ) ) );

        area.x = x + border.BorderWidth();
        area.y = y + border.BorderHeight();

        border.SetPosition( x, y, area.width, area.height );
        SavePosition();
    }
    else {
        area.x = x;
        area.y = y;
    }
}

void Interface::BorderWindow::captureMouse()
{
    const LocalEvent & le = LocalEvent::Get();

    if ( le.isMouseLeftButtonPressedInArea( GetRect() ) ) {
        _isMouseCaptured = true;
    }
    else {
        _isMouseCaptured = _isMouseCaptured && le.isMouseLeftButtonPressed();
    }
}

bool Interface::BorderWindow::QueueEventProcessing()
{
    LocalEvent & le = LocalEvent::Get();

    if ( !Settings::Get().isHideInterfaceEnabled() || !le.isMouseLeftButtonPressedInArea( border.GetTop() ) ) {
        return false;
    }

    // Reset the cursor when dragging, because if the window border is close to the edge of the game window,
    // then the mouse cursor could be changed to a scroll cursor.
    Cursor::Get().SetThemes( Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Point & mp = le.getMouseCursorPos();
    const fheroes2::Rect & pos = GetRect();

    fheroes2::MovableSprite moveIndicator( pos.width, pos.height, pos.x, pos.y );
    moveIndicator.reset();
    fheroes2::DrawBorder( moveIndicator, fheroes2::GetColorId( 0xD0, 0xC0, 0x48 ), 6 );

    const int32_t ox = mp.x - pos.x;
    const int32_t oy = mp.y - pos.y;

    moveIndicator.setPosition( pos.x, pos.y );
    moveIndicator.redraw();
    display.render();

    while ( le.HandleEvents() && le.isMouseLeftButtonPressed() ) {
        if ( le.hasMouseMoved() ) {
            moveIndicator.setPosition( mp.x - ox, mp.y - oy );
            display.render();
        }
    }

    SetPos( mp.x - ox, mp.y - oy );

    return true;
}
