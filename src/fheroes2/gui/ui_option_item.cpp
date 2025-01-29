/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2025                                             *
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

#include "ui_option_item.h"

#include <cassert>
#include <cstdint>
#include <utility>

#include "agg_image.h"
#include "icn.h"
#include "image.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_text.h"

namespace
{
    const int32_t textVerticalOffset = 12;
    const int32_t nameVerticalOffset = 6;
}

namespace fheroes2
{
    void drawOption( const Rect & optionRoi, const Sprite & icon, std::string titleText, std::string valueText, const int32_t textMaxWidth )
    {
        Display & display = Display::instance();

        const Text title( std::move( titleText ), FontType::smallWhite() );
        const Text name( std::move( valueText ), FontType::smallWhite() );

        // Calculate the text field left border position to horizontally align the text to the icon center.
        const int32_t textHorizontalOffset = optionRoi.x + ( icon.width() - textMaxWidth ) / 2;

        title.draw( textHorizontalOffset, optionRoi.y - textVerticalOffset + title.height() - title.height( textMaxWidth ), textMaxWidth, display );
        name.draw( textHorizontalOffset, optionRoi.y + optionRoi.height + nameVerticalOffset, textMaxWidth, display );

        Blit( icon, 0, 0, display, optionRoi.x, optionRoi.y, icon.width(), icon.height() );
        fheroes2::addGradientShadow( icon, display, { optionRoi.x, optionRoi.y }, { -5, 5 } );
    }

    void drawScrollSpeed( const fheroes2::Rect & optionRoi, const int speed )
    {
        int32_t scrollSpeedIconIcn = ICN::UNKNOWN;
        uint32_t scrollSpeedIconId = 0;
        std::string scrollSpeedName;

        if ( speed == SCROLL_SPEED_NONE ) {
            scrollSpeedName = _( "Off" );
            scrollSpeedIconIcn = ICN::SPANEL;
            scrollSpeedIconId = 9;
        }
        else if ( speed == SCROLL_SPEED_SLOW ) {
            scrollSpeedName = _( "Slow" );
            scrollSpeedIconIcn = ICN::CSPANEL;
            scrollSpeedIconId = 0;
        }
        else if ( speed == SCROLL_SPEED_NORMAL ) {
            scrollSpeedName = _( "Normal" );
            scrollSpeedIconIcn = ICN::CSPANEL;
            scrollSpeedIconId = 0;
        }
        else if ( speed == SCROLL_SPEED_FAST ) {
            scrollSpeedName = _( "Fast" );
            scrollSpeedIconIcn = ICN::CSPANEL;
            scrollSpeedIconId = 1;
        }
        else if ( speed == SCROLL_SPEED_VERY_FAST ) {
            scrollSpeedName = _( "Very Fast" );
            scrollSpeedIconIcn = ICN::CSPANEL;
            scrollSpeedIconId = 2;
        }

        assert( scrollSpeedIconIcn != ICN::UNKNOWN );

        const fheroes2::Sprite & scrollSpeedIcon = fheroes2::AGG::GetICN( scrollSpeedIconIcn, scrollSpeedIconId );
        fheroes2::drawOption( optionRoi, scrollSpeedIcon, _( "Scroll Speed" ), std::move( scrollSpeedName ), fheroes2::UiOptionTextWidth::TWO_ELEMENTS_ROW );
    }

    void drawInterfaceType( const fheroes2::Rect & optionRoi, const bool isEvilInterface )
    {
        const fheroes2::Sprite & interfaceThemeIcon = fheroes2::AGG::GetICN( ICN::SPANEL, isEvilInterface ? 17 : 16 );

        std::string value;
        if ( isEvilInterface ) {
            value = _( "Evil" );
        }
        else {
            value = _( "Good" );
        }

        fheroes2::drawOption( optionRoi, interfaceThemeIcon, _( "Interface Type" ), std::move( value ), fheroes2::UiOptionTextWidth::TWO_ELEMENTS_ROW );
    }

    void drawCursorType( const fheroes2::Rect & optionRoi, const bool isMonochromeCursor )
    {
        if ( isMonochromeCursor ) {
            fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::SPANEL, 20 ), _( "Mouse Cursor" ), _( "Black & White" ),
                                  fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
        }
        else {
            fheroes2::drawOption( optionRoi, fheroes2::AGG::GetICN( ICN::SPANEL, 21 ), _( "Mouse Cursor" ), _( "Color" ),
                                  fheroes2::UiOptionTextWidth::THREE_ELEMENTS_ROW );
        }
    }
}
