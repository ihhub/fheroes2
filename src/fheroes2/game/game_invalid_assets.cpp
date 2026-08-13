/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "game_invalid_assets.h"

#include "embedded_image.h"
#include "image.h"
#include "localevent.h"
#include "screen.h"
#include "timing.h"
#include "zzlib.h"

void showMissingAssetsImage()
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Image & image = Compression::CreateImageFromZlib( 290, 190, missingGameAssets, sizeof( missingGameAssets ), false );

    display.fill( 0 );
    fheroes2::Resize( image, display );

    display.render();

    LocalEvent & le = LocalEvent::Get();

    // Display the message for 5 seconds so that the user sees it enough and not immediately closes without reading properly.
    const fheroes2::Time timer;

    bool closeWindow = false;

    while ( le.HandleEvents( true, true ) ) {
        if ( closeWindow && timer.getS() >= 5 ) {
            break;
        }

        if ( le.isAnyKeyPressed() || le.MouseClickLeft() ) {
            closeWindow = true;
        }
    }
}
