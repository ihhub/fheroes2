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

namespace
{
    void repeatPattern( const fheroes2::Image & in, int32_t inX, int32_t inY, int32_t inWidth, int32_t inHeight,
                        fheroes2::Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height )
    {
        // TODO: verify/validate
        // TODO: support offsetXY in the pattern

        const int32_t widthIn = in.width();
        const int32_t widthOut = out.width();
        const size_t restWidth = static_cast<size_t>( width % inWidth );

        const int32_t offsetIn = inY * widthIn + inX;
        const uint8_t * imageIn = in.image() + offsetIn;
        const uint8_t * const imageInEnd = imageIn + inHeight * widthIn;

        const int32_t offsetOut = outY * widthOut + outX;
        uint8_t * imageOut = out.image() + offsetOut;
        uint8_t * const imageOutEnd = imageOut + height * widthOut;

        for ( ; imageOut != imageOutEnd; imageOut += widthOut - width ) {

            const uint8_t * const imageOutRepeatEnd = imageOut + width - restWidth;
            for ( ; imageOut != imageOutRepeatEnd; imageOut += inWidth ) {
                memcpy( imageOut, imageIn, inWidth );
            }
            memcpy( imageOut, imageIn, restWidth );
            imageOut += restWidth;

            imageIn += widthIn;
            if ( imageInEnd == imageIn ) {
                imageIn = in.image() + offsetIn;
            }
        }

    }
}

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
    const int32_t topRepeatCount = ( extraDisplayWidth > 0 ? extraDisplayWidth : 0 ) / TILEWIDTH;
    const int32_t vertRepeatCount = ( extraDisplayHeight > 0 ? extraDisplayHeight : 0 ) / TILEWIDTH;
    const int32_t iconsCount = vertRepeatCount > 3 ? 8 : ( vertRepeatCount < 3 ? 4 : 7 );

    const int32_t topPadWidth = ( extraDisplayWidth % TILEWIDTH ) / 2; // top and bottom padding is split in two halves around the repeated "tiles"
    int32_t bottomRepeatWidth;
    int32_t bottomRepeatCount;
    if ( isEvilInterface ) {
        bottomRepeatWidth = 7; // width of a single bone piece
        bottomRepeatCount = ( extraDisplayWidth > 0 ? extraDisplayWidth : 0 ) / bottomRepeatWidth;
    }
    else {
        bottomRepeatWidth = TILEWIDTH;
        bottomRepeatCount = topRepeatCount;
    }
    const int32_t bottomPadWidth = ( extraDisplayWidth % bottomRepeatWidth ) / 2;

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
    for ( int32_t x = 0; x < srcrt.width + topPadWidth; x += srcrt.width ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.x += srcrt.width;
    }
    // reset position back to align
    dstpt.x = srcrt.x + srcrt.width + topPadWidth;

    srcrt.x += srcrt.width;
    srcrt.width = isEvilInterface ? 64 : 24;
    srcrt.height = BORDERWIDTH;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;

    srcrt.x += srcrt.width;
    srcrt.width = TILEWIDTH;
    // for ( int32_t i = 0; i <= topRepeatCount; ++i ) {
    //     fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    //     dstpt.x += TILEWIDTH;
    // }
    repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, ( topRepeatCount + 1 ) * TILEWIDTH, BORDERWIDTH );
    dstpt.x += ( topRepeatCount + 1 ) * TILEWIDTH;

    srcrt.x += TILEWIDTH;
    srcrt.width = isEvilInterface ? 65 : 25;
    srcrt.height = BORDERWIDTH;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;

    srcrt.x += srcrt.width;
    srcrt.width = 6;
    for ( int32_t x = 0; x < srcrt.width + topPadWidth; x += srcrt.width ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.x += srcrt.width;
    }
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
    for ( int32_t i = 0; i <= vertRepeatCount; ++i ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;

    srcrt.width = BORDERWIDTH;
    srcrt.height = isEvilInterface ? 33 : 123;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    srcrt.y += srcrt.height;
    dstpt.y += srcrt.height;

    srcrt.height = 8;
    for ( int32_t y = 0; y < srcrt.height + vertPadHeight; y += srcrt.height ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
    }
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
    for ( int32_t i = 0; i <= vertRepeatCount; ++i ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;

    srcrt.width = BORDERWIDTH;
    srcrt.height = isEvilInterface ? 33 : 125; // middle border is special on good interface due to all the green leaves
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // fix extra border part on higher resolutions
    if ( displayHeight > fheroes2::Display::DEFAULT_HEIGHT ) {
        fheroes2::Rect fixrt( 478, isEvilInterface ? 328 : 345, 3, isEvilInterface ? 15 : 20 );
        fheroes2::Point fixpt( dstpt.x + 14, dstpt.y + 18 );
        fheroes2::Blit( icnadv, fixrt.x, fixrt.y, display, fixpt.x, fixpt.y, fixrt.width, fixrt.height );
    }

    srcrt.y += srcrt.height;
    dstpt.y += srcrt.height;

    srcrt.height = 8;
    for ( int32_t y = 0; y < srcrt.height + vertPadHeight; y += srcrt.height ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
    }
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
    for ( int32_t i = 0; i < vertRepeatCount + 1; ++i ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += TILEWIDTH;
    }
    srcrt.y += TILEWIDTH;

    srcrt.width = BORDERWIDTH;
    srcrt.height = isEvilInterface ? 33 : 123;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    // fix extra border part on higher resolutions
    if ( displayHeight > fheroes2::Display::DEFAULT_HEIGHT ) {
        fheroes2::Rect fixrt( 624, isEvilInterface ? 328 : 345, 3, isEvilInterface ? 15 : 20 );
        fheroes2::Point fixpt( dstpt.x, dstpt.y + 18 );
        fheroes2::Blit( icnadv, fixrt.x, fixrt.y, display, fixpt.x, fixpt.y, fixrt.width, fixrt.height );
    }

    srcrt.y += srcrt.height;
    dstpt.y += srcrt.height;

    srcrt.height = 8;
    for ( int32_t y = 0; y < srcrt.height + vertPadHeight; y += srcrt.height ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
    }
    srcrt.y += srcrt.height;

    srcrt.height = icnadv.height() - srcrt.y;
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
    for ( int32_t x = 0; x < srcrt.width + bottomPadWidth; x += srcrt.width ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.x += srcrt.width;
    }
    // reset position back to align
    dstpt.x = srcrt.x + srcrt.width + bottomPadWidth;

    srcrt.x += srcrt.width;
    srcrt.width = isEvilInterface ? 90 : 24;
    srcrt.height = BORDERWIDTH;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;
    srcrt.x += srcrt.width;

    srcrt.width = bottomRepeatWidth;
    for ( int32_t i = 0; i <= bottomRepeatCount; ++i ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.x += srcrt.width;
    }
    srcrt.x += srcrt.width;

    srcrt.width = isEvilInterface ? 86 : 25; // evil bottom border is asymmetric
    srcrt.height = BORDERWIDTH;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    dstpt.x += srcrt.width;

    srcrt.x += srcrt.width;
    srcrt.width = 6;
    for ( int32_t x = 0; x <= srcrt.width + bottomPadWidth; x += srcrt.width ) {
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.x += srcrt.width;
    }
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
    dstpt.y = srcrt.y + BORDERWIDTH + iconsCount * 32;
    srcrt.y = srcrt.y + BORDERWIDTH + 4 * 32;
    if ( viewWorldMode && displayHeight > fheroes2::Display::DEFAULT_HEIGHT ) {
        dstpt.y = 464;
    }
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
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
