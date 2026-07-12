/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2026                                             *
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
#include <cstdint>
#include <memory>

#include "dialog.h" // IWYU pragma: associated
#include "game_assets.h"
#include "icn.h"
#include "image.h"
#include "screen.h"
#include "settings.h"

namespace
{
    constexpr int32_t offsetFromBorder{ 6 };
    constexpr int32_t windowWidth = fheroes2::boxAreaWidthPx + ( fheroes2::borderWidthPx + offsetFromBorder ) * 2;
    constexpr int32_t buttonHeight = 40;
    constexpr int32_t activeAreaHeight = 35;

    int32_t topHeight( const bool isEvilInterface )
    {
        const fheroes2::Sprite & image = Assets::getImage( isEvilInterface ? ICN::BUYBUILE : ICN::BUYBUILD, 0 );
        return image.height();
    }

    int32_t bottomHeight( const bool isEvilInterface )
    {
        const fheroes2::Sprite & image = Assets::getImage( isEvilInterface ? ICN::BUYBUILE : ICN::BUYBUILD, 2 );
        return image.height();
    }

    int32_t leftWidth( const bool isEvilInterface )
    {
        const int icnId = isEvilInterface ? ICN::BUYBUILE : ICN::BUYBUILD;

        const fheroes2::Sprite & image4 = Assets::getImage( icnId, 4 );
        const fheroes2::Sprite & image5 = Assets::getImage( icnId, 5 );
        const fheroes2::Sprite & image6 = Assets::getImage( icnId, 6 );

        int32_t widthLeft = image4.width();
        widthLeft = std::max( widthLeft, image5.width() );
        widthLeft = std::max( widthLeft, image6.width() );

        return widthLeft;
    }

    int32_t rightWidth( const bool isEvilInterface )
    {
        const int icnId = isEvilInterface ? ICN::BUYBUILE : ICN::BUYBUILD;

        const fheroes2::Sprite & image0 = Assets::getImage( icnId, 0 );
        const fheroes2::Sprite & image1 = Assets::getImage( icnId, 1 );
        const fheroes2::Sprite & image2 = Assets::getImage( icnId, 2 );

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

Dialog::ResizableFrameBox::ResizableFrameBox( int width, int height, int startYPos, const bool showButtons )
    : _middleFragmentCount( 0 )
    , _middleFragmentHeight( 0 )
{
    if ( showButtons ) {
        height += buttonHeight;
    }

    // TODO: add support for wider windows.
    if ( width != fheroes2::boxAreaWidthPx ) {
        // We don't generate windows narrower than the original game.
        width = fheroes2::boxAreaWidthPx;
    }

    const bool evil = Settings::Get().isEvilInterfaceEnabled();
    _middleFragmentCount = ( height <= 2 * activeAreaHeight ? 0 : 1 + ( height - 2 * activeAreaHeight ) / activeAreaHeight );
    _middleFragmentHeight = height <= 2 * activeAreaHeight ? 0 : height - 2 * activeAreaHeight;
    const int32_t height_top_bottom = topHeight( evil ) + bottomHeight( evil );

    area.width = width;
    area.height = activeAreaHeight + activeAreaHeight + _middleFragmentHeight;

    fheroes2::Display & display = fheroes2::Display::instance();
    const int32_t leftSideOffset = leftOffset( evil );

    _position.x = ( display.width() - windowWidth ) / 2 - leftSideOffset;
    _position.y = startYPos;

    if ( startYPos < 0 ) {
        _position.y = ( ( display.height() - _middleFragmentHeight ) / 2 ) - topHeight( evil );
    }

    _restorer.reset( new fheroes2::ImageRestorer( display, _position.x, _position.y, overallWidth( evil ), height_top_bottom + _middleFragmentHeight ) );

    area.x = _position.x + ( windowWidth - area.width ) / 2 + leftSideOffset;
    area.y = _position.y + ( topHeight( evil ) - activeAreaHeight );

    redraw();
}

void Dialog::ResizableFrameBox::redraw()
{
    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int buybuild = isEvilInterface ? ICN::BUYBUILE : ICN::BUYBUILD;

    const int32_t overallLeftWidth = leftWidth( isEvilInterface );

    const fheroes2::Sprite & rightTop = Assets::getImage( buybuild, 0 );
    const fheroes2::Sprite & leftTop = Assets::getImage( buybuild, 4 );

    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::Blit( leftTop, display, _position.x + overallLeftWidth - leftTop.width(), _position.y );
    fheroes2::Blit( rightTop, display, _position.x + overallLeftWidth, _position.y );

    _position.y += leftTop.height();

    const int32_t posBeforeMiddle = _position.y;
    int32_t middleLeftHeight = _middleFragmentHeight;
    for ( uint32_t i = 0; i < _middleFragmentCount; ++i ) {
        const int32_t chunkHeight = middleLeftHeight >= activeAreaHeight ? activeAreaHeight : middleLeftHeight;

        const fheroes2::Sprite & leftMiddle = Assets::getImage( buybuild, 5 );
        fheroes2::Blit( leftMiddle, 0, 10, display, _position.x + overallLeftWidth - leftMiddle.width(), _position.y, leftMiddle.width(), chunkHeight );

        const fheroes2::Sprite & rightMiddle = Assets::getImage( buybuild, 1 );
        fheroes2::Blit( rightMiddle, 0, 10, display, _position.x + overallLeftWidth, _position.y, rightMiddle.width(), chunkHeight );

        middleLeftHeight -= chunkHeight;
        _position.y += chunkHeight;
    }

    _position.y = posBeforeMiddle + _middleFragmentHeight;

    const fheroes2::Sprite & rightBottom = Assets::getImage( buybuild, 2 );
    const fheroes2::Sprite & leftBottom = Assets::getImage( buybuild, 6 );

    fheroes2::Blit( leftBottom, display, _position.x + overallLeftWidth - leftBottom.width(), _position.y );
    fheroes2::Blit( rightBottom, display, _position.x + overallLeftWidth, _position.y );
}

Dialog::ResizableFrameBox::~ResizableFrameBox()
{
    _restorer->restore();

    fheroes2::Display::instance().render( _restorer->rect() );
}

int32_t Dialog::ResizableFrameBox::getButtonAreaHeight()
{
    return buttonHeight;
}
