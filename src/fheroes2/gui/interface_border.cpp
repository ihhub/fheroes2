/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include <algorithm>

#include "agg_image.h"
#include "gamedefs.h"
#include "icn.h"
#include "image.h"
#include "interface_border.h"
#include "localevent.h"
#include "maps.h"
#include "screen.h"
#include "settings.h"
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

    const int32_t topRepeatCount = extraDisplayWidth > 0 ? extraDisplayWidth / TILEWIDTH : 0;
    const int32_t topRepeatWidth = ( topRepeatCount + 1 ) * TILEWIDTH;

    const int32_t vertRepeatCount = extraDisplayHeight > 0 ? extraDisplayHeight / TILEWIDTH : 0;
    const int32_t iconsCount = vertRepeatCount > 3 ? 8 : ( vertRepeatCount < 3 ? 4 : 7 );

    const int32_t vertRepeatHeight = ( vertRepeatCount + 1 ) * TILEWIDTH;
    const int32_t vertRepeatHeightTop = ( iconsCount - 3 ) * TILEWIDTH;
    const int32_t vertRepeatHeightBottom = vertRepeatHeight - vertRepeatHeightTop;

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

    auto drawHorizontalBorder = [&]( const bool isTop ) {
        // leftElement
        srcrt.x = 0;
        srcrt.y = isTop ? 0 : ( icnadv.height() - BORDERWIDTH );
        srcrt.width = isEvilInterface ? ( isTop ? 153 : 129 ) : 193;
        srcrt.height = BORDERWIDTH;
        dstpt.x = srcrt.x;
        dstpt.y = isTop ? srcrt.y : displayHeight - BORDERWIDTH;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.x += srcrt.width;
        srcrt.x += srcrt.width;

        // transitioning line to the left block
        srcrt.width = 6;
        const int32_t padWidthLeft = isTop ? topPadWidthLeft : bottomPadWidthLeft;
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, srcrt.width + padWidthLeft, BORDERWIDTH );
        dstpt.x += srcrt.width + padWidthLeft;
        srcrt.x += srcrt.width;

        // left middle block
        srcrt.width = isEvilInterface ? ( isTop ? 64 : 90 ) : 24;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.x += srcrt.width;
        srcrt.x += srcrt.width;

        // sculls/diamonds pattern
        srcrt.width = isTop ? TILEWIDTH : bottomTileWidth;
        const int32_t someWidth = isTop ? topRepeatWidth : bottomRepeatWidth;
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, someWidth, BORDERWIDTH );
        dstpt.x += someWidth;
        srcrt.x += srcrt.width;

        // right middle block
        srcrt.width = isEvilInterface ? ( isTop ? 65 : 86 ) : 25; // evil bottom border is asymmetric
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.x += srcrt.width;
        srcrt.x += srcrt.width;

        // transitioning line to the right block
        srcrt.width = 6;
        const int32_t padWidthRight = isTop ? topPadWidthRight : bottomPadWidthRight;
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, srcrt.width + padWidthRight, BORDERWIDTH );
        dstpt.x += srcrt.width + padWidthRight;
        srcrt.x += srcrt.width;

        // right large block overlapping map right panel border
        srcrt.width = icnadv.width() - srcrt.x;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    };

    drawHorizontalBorder( true );
    drawHorizontalBorder( false );

    auto drawLeftBorder = [&]() {
        srcrt.x = 0;
        srcrt.y = BORDERWIDTH;
        srcrt.width = BORDERWIDTH;
        dstpt.x = srcrt.x;
        dstpt.y = srcrt.y;

        // upper part, column with skull and spine/vine on top with with few rubies / cups
        srcrt.height = 255 - BORDERWIDTH;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
        srcrt.y += srcrt.height;

        if ( isEvilInterface ) {
            // upper middle pattern block - column with a blinking ruby in the bottom
            srcrt.height = TILEWIDTH;
            repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, vertRepeatHeightTop );
            dstpt.y += vertRepeatHeightTop;
            srcrt.y += TILEWIDTH;

            // upper middle curvy block / column piece with blinking ruby
            srcrt.width = BORDERWIDTH;
            srcrt.height = 35;
            fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
            dstpt.y += srcrt.height;
            srcrt.y += srcrt.height;

            // middle transition area (stright lines)
            srcrt.height = 6;
            repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, srcrt.height + vertPadHeight );
            dstpt.y += srcrt.height + vertPadHeight;
            srcrt.y += srcrt.height;

            // column with a scull and ruby in the middle
            srcrt.height = 103;
            fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
            dstpt.y += srcrt.height;
            srcrt.y += srcrt.height;

            // lower middle middle evil square brick
            srcrt.height = TILEWIDTH;
            fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
            dstpt.y += TILEWIDTH;

            // lower evil square blicks
            repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, vertRepeatHeightBottom );
            dstpt.y += vertRepeatHeightBottom;
            srcrt.y += TILEWIDTH;
        }
        else {
            // column body pattern
            srcrt.height = TILEWIDTH;
            repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, vertRepeatHeight );
            dstpt.y += vertRepeatHeight;
            srcrt.y += TILEWIDTH;

            // column body lower part
            srcrt.height = 125;
            fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
            dstpt.y += srcrt.height;
            srcrt.y += srcrt.height;

            // lower transition area, stright lines pattern
            srcrt.height = 4;
            repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, srcrt.height + vertPadHeight );
            dstpt.y += srcrt.height + vertPadHeight;
            srcrt.y += srcrt.height;
        }

        // good: column lower piece with vines, evil: element is not present
        srcrt.height = icnadv.height() - BORDERWIDTH - srcrt.y;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    };
    drawLeftBorder();

    auto drawPanelBorder = [&]( const bool isMiddle ) {
        const int32_t xOffset = isMiddle ? ( RADARWIDTH + BORDERWIDTH ) : 0;
        srcrt.x = icnadv.width() - BORDERWIDTH - xOffset;
        srcrt.y = BORDERWIDTH;
        srcrt.width = BORDERWIDTH;
        srcrt.height = 255 - BORDERWIDTH;
        dstpt.x = displayWidth - BORDERWIDTH - xOffset;

        // top of the column
        dstpt.y = srcrt.y;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        dstpt.y += srcrt.height;
        srcrt.y += srcrt.height;

        // column piece
        srcrt.height = TILEWIDTH;
        repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, vertRepeatHeightTop );
        dstpt.y += vertRepeatHeightTop;
        srcrt.y += TILEWIDTH;

        // top-to-horizontal angle (evil) - "T" (good), the lower angle of heroes area, column, in the evil border - with gems
        srcrt.height = isEvilInterface ? 35 : 50;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

        // hide embranchment
        if ( viewWorldMode ) {
            fheroes2::Rect fixrt( ( isMiddle ? 478 : 624 ), isEvilInterface ? ( isMiddle ? 137 : 139 ) : 345, 3, isEvilInterface ? 15 : 20 );
            fheroes2::Point fixpt( dstpt.x + ( isMiddle ? 14 : 0 ), dstpt.y + 18 );
            fheroes2::Blit( icnadv, fixrt.x, fixrt.y, display, fixpt.x, fixpt.y, fixrt.width, fixrt.height );
        }

        dstpt.y += srcrt.height;
        srcrt.y += srcrt.height;

        if ( isEvilInterface ) {
            // stright lines, transition area
            srcrt.height = 6;
            repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, srcrt.height + vertPadHeight );
            dstpt.y += srcrt.height + vertPadHeight;
            srcrt.y += srcrt.height;

            // lower button to calendar area, column head with scull and gem
            srcrt.height = 103;
            fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
            dstpt.y += srcrt.height;
            srcrt.y += srcrt.height;

            // A single brick about the first row of resources
            srcrt.height = TILEWIDTH;
            fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
            dstpt.y += TILEWIDTH;

            // bricks untill the bottom
            repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, vertRepeatHeightBottom );
            dstpt.y += vertRepeatHeightBottom;
            srcrt.y += TILEWIDTH;
        }
        else {
            // Piece of column exactly in the middle of buttons
            srcrt.height = TILEWIDTH;
            fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
            dstpt.y += TILEWIDTH;

            // column from the lower row of buttons till some distance from the bottom
            repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, vertRepeatHeightBottom );
            dstpt.y += vertRepeatHeightBottom;
            srcrt.y += TILEWIDTH;

            // bottom middle - transition to column covered with vine
            srcrt.height = 43;
            fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
            dstpt.y += srcrt.height;
            srcrt.y += srcrt.height;

            // piece of colunn covered with wine
            srcrt.height = ( isMiddle ? 8 : 4 ); // middle border is special on good interface due to all the green leaves
            repeatPattern( icnadv, srcrt.x, srcrt.y, srcrt.width, srcrt.height, display, dstpt.x, dstpt.y, BORDERWIDTH, srcrt.height + vertPadHeight );
            dstpt.y += srcrt.height + vertPadHeight;
            srcrt.y += srcrt.height;
        }

        // good - blocks with vine and leaves, evil - nothing
        srcrt.height = icnadv.height() - BORDERWIDTH - srcrt.y;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    };
    drawPanelBorder( true );
    drawPanelBorder( false );

    auto drawHeroesAndTownsHorizontalBorders = [&]() {
        // bottom border of the map, top border of heroes and towns
        srcrt.x = icnadv.width() - RADARWIDTH - BORDERWIDTH;
        srcrt.y = RADARWIDTH + BORDERWIDTH;
        srcrt.width = RADARWIDTH;
        srcrt.height = BORDERWIDTH;
        dstpt.x = displayWidth - RADARWIDTH - BORDERWIDTH;
        dstpt.y = srcrt.y;
        fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

        if ( !viewWorldMode ) {
            // bottom border of heroes and towns area, top border of resources area
            dstpt.y = srcrt.y + BORDERWIDTH + iconsCount * 32;
            srcrt.y = srcrt.y + BORDERWIDTH + 4 * 32;
            fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        }
    };
    drawHeroesAndTownsHorizontalBorders();
}

Interface::BorderWindow::BorderWindow( const fheroes2::Rect & rt )
    : area( rt )
{}

const fheroes2::Rect & Interface::BorderWindow::GetRect() const
{
    return Settings::Get().isHideInterfaceEnabled() && border.isValid() ? border.GetRect() : GetArea();
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
    if ( Settings::Get().isHideInterfaceEnabled() ) {
        const fheroes2::Display & display = fheroes2::Display::instance();

        px = std::max( 0, std::min( px, display.width() - ( area.width + border.BorderWidth() * 2 ) ) );
        py = std::max( 0, std::min( py, display.height() - ( area.height + border.BorderHeight() * 2 ) ) );

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

bool Interface::BorderWindow::QueueEventProcessing()
{
    const Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();

    if ( conf.isHideInterfaceEnabled() && le.MousePressLeft( border.GetTop() ) ) {
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

        return true;
    }

    return false;
}
