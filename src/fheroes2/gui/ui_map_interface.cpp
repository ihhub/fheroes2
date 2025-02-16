/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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

#include "ui_map_interface.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <utility>

#include "agg_image.h"
#include "cursor.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "screen.h"
#include "ui_constants.h"
#include "ui_text.h"

namespace Interface
{
    fheroes2::Rect getPopupWindowPosition( const fheroes2::Point & mousePos, const fheroes2::Rect & interfaceArea, const fheroes2::Size & windowSize )
    {
        // If this assertion blows up then you are trying to render a window bigger than the interface area.
        assert( interfaceArea.width >= windowSize.width && interfaceArea.height >= windowSize.height );

        fheroes2::Point windowsPos{ ( ( mousePos.x - fheroes2::borderWidthPx ) / fheroes2::tileWidthPx ) * fheroes2::tileWidthPx + fheroes2::tileWidthPx
                                        - ( windowSize.width / 2 ),
                                    ( ( mousePos.y - fheroes2::borderWidthPx ) / fheroes2::tileWidthPx ) * fheroes2::tileWidthPx + fheroes2::tileWidthPx
                                        - ( windowSize.height / 2 ) };

        // Clamp area to the edges of the area.
        windowsPos.x = std::clamp( windowsPos.x, fheroes2::borderWidthPx, ( interfaceArea.width - windowSize.width ) + fheroes2::borderWidthPx );
        windowsPos.y = std::clamp( windowsPos.y, fheroes2::borderWidthPx, ( interfaceArea.height - windowSize.height ) + fheroes2::borderWidthPx );

        return { windowsPos.x, windowsPos.y, windowSize.width, windowSize.height };
    }

    void displayStandardPopupWindow( std::string text, const fheroes2::Rect & interfaceArea )
    {
        const CursorRestorer cursorRestorer( false );

        const fheroes2::Sprite & windowImage = fheroes2::AGG::GetICN( ICN::QWIKINFO, 0 );

        LocalEvent & le = LocalEvent::Get();
        const fheroes2::Rect windowRoi = Interface::getPopupWindowPosition( le.getMouseCursorPos(), interfaceArea, { windowImage.width(), windowImage.height() } );

        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::ImageRestorer restorer( display, windowRoi.x, windowRoi.y, windowRoi.width, windowRoi.height );
        fheroes2::Blit( windowImage, display, windowRoi.x, windowRoi.y );

        const int32_t objectTextBorderedWidth = windowRoi.width - 2 * fheroes2::borderWidthPx;
        const fheroes2::Text textUi( std::move( text ), fheroes2::FontType::smallWhite() );
        textUi.draw( windowRoi.x + 22, windowRoi.y - 6 + ( ( windowRoi.height - textUi.height( objectTextBorderedWidth ) ) / 2 ), objectTextBorderedWidth, display );

        display.render( restorer.rect() );

        while ( le.HandleEvents() && le.isMouseRightButtonPressed() ) {
            // Do nothing and wait till the user releases the button.
        }

        restorer.restore();
        display.render( restorer.rect() );
    }
}
