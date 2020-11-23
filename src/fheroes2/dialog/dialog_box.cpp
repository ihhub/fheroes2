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

#include <algorithm>

namespace
{
    const int32_t windowWidth = 288; // this is measured value
    const int32_t buttonHeight = 40;
    const int32_t activeAreaHeight = 35;

    int32_t topHeight( const bool isEvilInterface )
    {
        const fheroes2::Sprite & image = fheroes2::AGG::GetICN( isEvilInterface ? ICN::BUYBUILE : ICN::BUYBUILD, 0 );
        return image.height();
    }

    int32_t bottomHeight( const bool isEvilInterface )
    {
        const fheroes2::Sprite & image = fheroes2::AGG::GetICN( isEvilInterface ? ICN::BUYBUILE : ICN::BUYBUILD, 2 );
        return image.height();
    }

    int32_t leftWidth( const bool isEvilInterface )
    {
        const int icnId = isEvilInterface ? ICN::BUYBUILE : ICN::BUYBUILD;

        const fheroes2::Sprite & image3 = fheroes2::AGG::GetICN( icnId, 3 );
        const fheroes2::Sprite & image4 = fheroes2::AGG::GetICN( icnId, 4 );
        const fheroes2::Sprite & image5 = fheroes2::AGG::GetICN( icnId, 5 );

        int32_t widthLeft = image3.width();
        widthLeft = std::max( widthLeft, image4.width() );
        widthLeft = std::max( widthLeft, image5.width() );

        return widthLeft;
    }

    int32_t rightWidth( const bool isEvilInterface )
    {
        const int icnId = isEvilInterface ? ICN::BUYBUILE : ICN::BUYBUILD;

        const fheroes2::Sprite & image0 = fheroes2::AGG::GetICN( icnId, 0 );
        const fheroes2::Sprite & image1 = fheroes2::AGG::GetICN( icnId, 1 );
        const fheroes2::Sprite & image2 = fheroes2::AGG::GetICN( icnId, 2 );

        int32_t widthRight = image0.width();
        widthRight = std::max( widthRight, image1.width() );
        widthRight = std::max( widthRight, image2.width() );

        return widthRight;
    }

    int32_t overallWidth( const bool isEvilInterface )
    {
        return leftWidth( isEvilInterface ) + rightWidth( isEvilInterface );
    }

    int32_t leftOffset( const bool isEvilInterface )
    {
        return leftWidth( isEvilInterface ) - windowWidth / 2;
    }
}

void BoxRedraw( int32_t posx, int32_t posy, uint32_t count, int32_t middleHeight );

Dialog::NonFixedFrameBox::NonFixedFrameBox( int height, int startYPos, bool showButtons )
{
    if ( showButtons )
        height += buttonHeight;

    const bool evil = Settings::Get().ExtGameEvilInterface();
    const int32_t count_middle = ( height <= 2 * activeAreaHeight ? 0 : 1 + ( height - 2 * activeAreaHeight ) / activeAreaHeight );
    const int32_t height_middle = height <= 2 * activeAreaHeight ? 0 : height - 2 * activeAreaHeight;
    const int32_t height_top_bottom = topHeight( evil ) + bottomHeight( evil );

    area.width = BOXAREA_WIDTH;
    area.height = activeAreaHeight + activeAreaHeight + height_middle;

    fheroes2::Display & display = fheroes2::Display::instance();
    const int32_t leftSideOffset = leftOffset( evil );

    int32_t posx = ( display.width() - windowWidth ) / 2 - leftSideOffset;
    int32_t posy = startYPos;

    if ( startYPos < 0 ) {
        posy = ( ( display.height() - height_middle ) / 2 ) - topHeight( evil );
    }

    _restorer.reset( new fheroes2::ImageRestorer( display, posx, posy, overallWidth( evil ), height_top_bottom + height_middle ) );

    area.x = posx + ( windowWidth - BOXAREA_WIDTH ) / 2 + leftSideOffset;
    area.y = posy + ( topHeight( evil ) - activeAreaHeight );

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

void BoxRedraw( int32_t posx, int32_t posy, uint32_t count, int32_t middleHeight )
{
    const bool isEvilInterface = Settings::Get().ExtGameEvilInterface();
    const int buybuild = isEvilInterface ? ICN::BUYBUILE : ICN::BUYBUILD;

    const int32_t overallLeftWidth = leftWidth( isEvilInterface );

    fheroes2::Point pt( posx, posy );

    // right-top
    const fheroes2::Sprite & part0 = fheroes2::AGG::GetICN( buybuild, 0 );
    // left-top
    const fheroes2::Sprite & part4 = fheroes2::AGG::GetICN( buybuild, 4 );

    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::Blit( part4, display, pt.x + overallLeftWidth - part4.width(), pt.y );
    fheroes2::Blit( part0, display, pt.x + overallLeftWidth, pt.y );

    pt.y += part4.height();

    const int32_t posBeforeMiddle = pt.y;
    int32_t middleLeftHeight = middleHeight;
    for ( uint32_t i = 0; i < count; ++i ) {
        const int32_t chunkHeight = middleLeftHeight >= activeAreaHeight ? activeAreaHeight : middleLeftHeight;
        // left-middle
        const fheroes2::Sprite & sl = fheroes2::AGG::GetICN( buybuild, 5 );
        fheroes2::Blit( sl, 0, 10, display, pt.x + overallLeftWidth - sl.width(), pt.y, sl.width(), chunkHeight );

        // right-middle
        const fheroes2::Sprite & sr = fheroes2::AGG::GetICN( buybuild, 1 );
        fheroes2::Blit( sr, 0, 10, display, pt.x + overallLeftWidth, pt.y, sr.width(), chunkHeight );

        middleLeftHeight -= chunkHeight;
        pt.y += chunkHeight;
    }

    pt.y = posBeforeMiddle + middleHeight;

    // right-bottom
    const fheroes2::Sprite & part2 = fheroes2::AGG::GetICN( buybuild, 2 );
    // left-bottom
    const fheroes2::Sprite & part6 = fheroes2::AGG::GetICN( buybuild, 6 );

    fheroes2::Blit( part6, display, pt.x + overallLeftWidth - part6.width(), pt.y );
    fheroes2::Blit( part2, display, pt.x + overallLeftWidth, pt.y );
}
