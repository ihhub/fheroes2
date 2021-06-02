/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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
#include "game_delays.h"
#include "localevent.h"
#include "screen.h"
#include "text.h"

void fheroes2::showTeamInfo()
{
    LocalEvent & le = LocalEvent::Get();
    le.PauseCycling();

    fheroes2::Display & display = fheroes2::Display::instance();

    TextBox text( "fheroes2 Resurrection Team presents", Font::WHITE_LARGE, 500 );
    const Rect roi( ( display.width() - text.w() ) / 2, ( display.height() - text.h() ) / 2, text.w(), text.h() );

    Image textImage( roi.width, roi.height );
    textImage.fill( 0 );
    text.Blit( 0, 0, textImage );

    // First frame must be fully rendered.
    display.fill( 0 );
    Copy( textImage, 0, 0, display, roi.x, roi.y, roi.width, roi.height );
    display.render();

    uint8_t alpha = 250;
    const uint64_t animationDelay = 40;

    while ( le.HandleEvents( Game::isCustomDelayNeeded( animationDelay ) ) && alpha > 20 ) {
        if ( le.KeyPress() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() )
            break;

        // Subsequent frames must update only the area within the text.
        if ( Game::validateCustomAnimationDelay( animationDelay ) ) {
            Copy( textImage, 0, 0, display, roi.x, roi.y, roi.width, roi.height );
            fheroes2::ApplyAlpha( display, roi.x, roi.y, display, roi.x, roi.y, roi.width, roi.height, alpha );
            display.render( roi );

            alpha -= 5;
        }
    }

    le.ResumeCycling();
}
