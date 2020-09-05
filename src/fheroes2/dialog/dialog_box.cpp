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

#define BUTTON_HEIGHT 40
#define BOX_WIDTH 306
#define BOX_TOP 99
#define BOX_MIDDLE 45
#define BOX_BOTTOM 81
#define BOXE_TOP 88
#define BOXE_MIDDLE 45
#define BOXE_BOTTOM 81
#define BOXAREA_TOP 35
#define BOXAREA_MIDDLE 35
#define BOXAREA_BOTTOM 35

void BoxRedraw( s32 posx, s32 posy, u32 count, int32_t middleHeight );

Dialog::NonFixedFrameBox::NonFixedFrameBox( int height, int startYPos, bool showButtons )
{
    if ( showButtons )
        height += BUTTON_HEIGHT;

    const bool evil = Settings::Get().ExtGameEvilInterface();
    const u32 count_middle = ( height <= BOXAREA_TOP + BOXAREA_BOTTOM ? 0 : 1 + ( height - BOXAREA_TOP - BOXAREA_BOTTOM ) / BOXAREA_MIDDLE );
    const int32_t height_middle = height <= BOXAREA_TOP + BOXAREA_BOTTOM ? 0 : height - BOXAREA_TOP - BOXAREA_BOTTOM;
    const u32 height_top_bottom = ( evil ? BOXE_TOP + BOXE_BOTTOM : BOX_TOP + BOX_BOTTOM );

    area.w = BOXAREA_WIDTH;
    area.h = BOXAREA_TOP + BOXAREA_BOTTOM + height_middle;

    fheroes2::Display & display = fheroes2::Display::instance();
    s32 posx = ( display.width() - BOX_WIDTH ) / 2;
    s32 posy = startYPos;

    if ( startYPos < 0 ) {
        posy = ( display.height() - height_top_bottom - height_middle ) / 2;
    }

    _restorer.reset( new fheroes2::ImageRestorer( display, posx, posy, BOX_WIDTH, height_top_bottom + height_middle ) );

    area.x = posx + 36;
    area.y = posy + ( evil ? BOXE_TOP - BOXAREA_TOP : BOX_TOP - BOXAREA_TOP );

    BoxRedraw( posx, posy, count_middle, height_middle );
}

Dialog::NonFixedFrameBox::~NonFixedFrameBox()
{
    _restorer->restore();

    fheroes2::Display::instance().render();
}

Dialog::FrameBox::FrameBox( int height, bool buttons )
    : Dialog::NonFixedFrameBox( height, -1, buttons )
{}

Dialog::FrameBox::~FrameBox() {}

void BoxRedraw( s32 posx, s32 posy, u32 count, int32_t middleHeight )
{
    const int buybuild = Settings::Get().ExtGameEvilInterface() ? ICN::BUYBUILE : ICN::BUYBUILD;

    // left top sprite
    Point pt( posx, posy );
    if ( !Settings::Get().ExtGameEvilInterface() )
        ++pt.x;

    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::Blit( fheroes2::AGG::GetICN( buybuild, 4 ), display, pt.x, pt.y );

    // right top sprite
    pt.x = posx + fheroes2::AGG::GetICN( buybuild, 4 ).width();
    fheroes2::Blit( fheroes2::AGG::GetICN( buybuild, 0 ), display, pt.x, pt.y );

    pt.y += fheroes2::AGG::GetICN( buybuild, 4 ).height();
    const int16_t posBeforeMiddle = pt.y;
    int32_t middleLeftHeight = middleHeight;
    for ( u32 i = 0; i < count; ++i ) {
        const int32_t chunkHeight = middleLeftHeight >= BOXAREA_MIDDLE ? BOXAREA_MIDDLE : middleLeftHeight;
        // left middle sprite
        pt.x = posx;
        const fheroes2::Sprite & sl = fheroes2::AGG::GetICN( buybuild, 5 );
        fheroes2::Blit( sl, 0, 10, display, pt.x, pt.y, sl.width(), chunkHeight );

        // right middle sprite
        pt.x += sl.width();
        if ( !Settings::Get().ExtGameEvilInterface() )
            pt.x -= 1;
        const fheroes2::Sprite & sr = fheroes2::AGG::GetICN( buybuild, 1 );
        fheroes2::Blit( sr, 0, 10, display, pt.x, pt.y, sr.width(), chunkHeight );

        middleLeftHeight -= chunkHeight;
        pt.y += chunkHeight;
    }

    pt.y = posBeforeMiddle + middleHeight;

    // right bottom sprite
    fheroes2::Blit( fheroes2::AGG::GetICN( buybuild, 2 ), display, pt.x, pt.y );

    // left bottom sprite
    pt.x = posx;
    fheroes2::Blit( fheroes2::AGG::GetICN( buybuild, 6 ), display, pt.x, pt.y );
}
