/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2022                                             *
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

#include "game_mainmenu_ui.h"

#include <cstdint>

#include "agg_image.h"
#include "icn.h"
#include "image.h"
#include "screen.h"

namespace fheroes2
{
    void drawSprite( Display & display, const int32_t offX, const int32_t offY, const int icnId, const uint32_t index )
    {
        const Sprite & sprite = AGG::GetICN( icnId, index );
        Blit( sprite, display, offX + sprite.x(), offY + sprite.y() );
    }

    void drawMainMenuScreen()
    {
        Display & display = Display::instance();

        // clear out any artifacts outside of the main background
        Fill( display, 0, 0, display.width(), display.height(), 0 );

        const Sprite & background = AGG::GetICN( ICN::HEROES, 0 );
        const int32_t offX = ( display.width() - background.width() ) / 2;
        const int32_t offY = ( display.height() - background.height() ) / 2;
        Blit( background, display, offX, offY );

        drawSprite( display, offX, offY, ICN::BTNSHNGL, 1 );
        drawSprite( display, offX, offY, ICN::BTNSHNGL, 5 );
        drawSprite( display, offX, offY, ICN::BTNSHNGL, 9 );
        drawSprite( display, offX, offY, ICN::BTNSHNGL, 13 );
        drawSprite( display, offX, offY, ICN::BTNSHNGL, 17 );
    }
}
