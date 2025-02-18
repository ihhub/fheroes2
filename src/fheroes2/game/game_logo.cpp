/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include "game_logo.h"

#include <cassert>
#include <cstdint>

#include "game_delays.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "translations.h"
#include "ui_text.h"
#include "ui_tool.h"

void fheroes2::showTeamInfo()
{
    const ScreenPaletteRestorer restorer;

    Display & display = Display::instance();

    Text text( _( "fheroes2 Resurrection Team presents" ), FontType::largeWhite() );
    const int32_t correctedTextWidth = text.width( 500 );

    const Rect roi{ ( display.width() - correctedTextWidth ) / 2, ( display.height() - text.height( correctedTextWidth ) ) / 2, text.width(),
                    text.height( correctedTextWidth ) };

    Image textImage( roi.width, roi.height );
    textImage.fill( 0 );
    text.draw( 0, 0, correctedTextWidth, textImage );

    // First frame must be fully rendered.
    display.fill( 0 );
    Copy( textImage, 0, 0, display, roi.x, roi.y, roi.width, roi.height );
    display.render();

    uint8_t alpha = 250;
    const uint64_t animationDelay = 40;

    // Immediately indicate that the delay has passed to render first frame immediately.
    Game::passCustomAnimationDelay( animationDelay );
    // Make sure that the first run is passed immediately.
    assert( !Game::isCustomDelayNeeded( animationDelay ) );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents( Game::isCustomDelayNeeded( animationDelay ) ) && alpha > 20 ) {
        if ( le.isAnyKeyPressed() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() )
            break;

        // Subsequent frames must update only the area within the text.
        if ( Game::validateCustomAnimationDelay( animationDelay ) ) {
            Copy( textImage, 0, 0, display, roi.x, roi.y, roi.width, roi.height );
            ApplyAlpha( display, roi.x, roi.y, display, roi.x, roi.y, roi.width, roi.height, alpha );
            display.render( roi );

            alpha -= 5;
        }
    }
}
