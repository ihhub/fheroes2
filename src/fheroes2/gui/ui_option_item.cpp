/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

#include <cstdint>
#include <utility>

#include "image.h"
#include "screen.h"
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
    }
}
