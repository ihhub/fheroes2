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

#include "game_mainmenu_ui.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

#include "agg_image.h"
#include "game.h"
#include "icn.h"
#include "image.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "ui_constants.h"
#include "ui_tool.h"

namespace
{
    void drawSprite( fheroes2::Image & output, const int icnId, const uint32_t index )
    {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icnId, index );
        fheroes2::Blit( sprite, 0, 0, output, sprite.x(), sprite.y(), sprite.width(), sprite.height() );
    }

    void renderWindowBackground( fheroes2::Image & output, const fheroes2::Rect & roi )
    {
        if ( roi.width == 0 || roi.height == 0 ) {
            // Nothing to render.
            return;
        }

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        const fheroes2::Sprite & background = fheroes2::AGG::GetICN( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 );

        const int32_t stepX = ( roi.width + background.width() ) / background.width();
        const int32_t stepY = ( roi.height + background.height() ) / background.height();

        fheroes2::Point offset{ roi.x, roi.y };

        for ( int y = 0; y < stepY; ++y ) {
            offset.x = roi.x;

            const int32_t height = std::min( background.height(), roi.height - y * background.height() );

            for ( int x = 0; x < stepX; ++x ) {
                fheroes2::Copy( background, 0, 0, output, offset.x, offset.y, std::min( background.width(), roi.width - x * background.width() ), height );

                offset.x += background.width();
            }

            offset.y += background.height();
        }
    }

    void fillScreenBorders( fheroes2::Image & output, const fheroes2::Rect & backgroundRoi )
    {
        if ( backgroundRoi.y == 0 ) {
            renderWindowBackground( output, { 0, 0, backgroundRoi.x, output.height() } );
            renderWindowBackground( output, { backgroundRoi.x + backgroundRoi.width, 0, output.width() - backgroundRoi.x - backgroundRoi.width, output.height() } );
        }
        else {
            assert( backgroundRoi.x == 0 );

            renderWindowBackground( output, { 0, 0, output.width(), backgroundRoi.y } );
            renderWindowBackground( output, { 0, backgroundRoi.y + backgroundRoi.height, output.width(), output.height() - backgroundRoi.y - backgroundRoi.height } );
        }
    }
}

namespace fheroes2
{
    void drawMainMenuScreen()
    {
        Display & display = Display::instance();

        const Sprite & mainMenuBackground = AGG::GetICN( ICN::HEROES, 0 );
        Copy( mainMenuBackground, 0, 0, display, mainMenuBackground.x(), mainMenuBackground.y(), mainMenuBackground.width(), mainMenuBackground.height() );

        drawSprite( display, ICN::BTNSHNGL, 1 );
        drawSprite( display, ICN::BTNSHNGL, 5 );
        drawSprite( display, ICN::BTNSHNGL, 9 );
        drawSprite( display, ICN::BTNSHNGL, 13 );
        drawSprite( display, ICN::BTNSHNGL, 17 );

        fillScreenBorders( display, { mainMenuBackground.x(), mainMenuBackground.y(), mainMenuBackground.width(), mainMenuBackground.height() } );
    }

    void drawEditorMainMenuScreen()
    {
        Display & display = Display::instance();

        const Sprite & background = AGG::GetICN( ICN::EDITOR, 0 );
        Copy( background, 0, 0, display, background.x(), background.y(), background.width(), background.height() );

        fillScreenBorders( display, { background.x(), background.y(), background.width(), background.height() } );
    }

    Point drawButtonPanel()
    {
        const fheroes2::Sprite & back = fheroes2::AGG::GetICN( ICN::HEROES, 0 );
        const fheroes2::Sprite & panel = fheroes2::AGG::GetICN( ICN::REDBACK, 0 );

        const int32_t panelOffset = fheroes2::Display::DEFAULT_HEIGHT - panel.height();
        const int32_t panelXPos = back.width() + back.x() - ( panel.width() + panelOffset );
        fheroes2::Blit( panel, fheroes2::Display::instance(), panelXPos, panelOffset + back.y() );

        const int32_t buttonMiddlePos = panelXPos + fheroes2::shadowWidthPx + ( panel.width() - fheroes2::shadowWidthPx ) / 2;

        const fheroes2::Sprite & buttonSample = fheroes2::AGG::GetICN( ICN::BTNNEWGM, 0 );
        const int32_t buttonWidth = buttonSample.width();
        const int32_t buttonXPos = buttonMiddlePos - buttonWidth / 2 - 3; // 3 is button shadow
        const int32_t buttonYPos = 46 + back.y();

        return { buttonXPos, buttonYPos };
    }

    void validateFadeInAndRender()
    {
        if ( Game::validateDisplayFadeIn() ) {
            fheroes2::fadeInDisplay();
        }
        else {
            fheroes2::Display::instance().render();
        }
    }
}
