/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#pragma once

#include <cstdint>

#include "gamedefs.h"
#include "math_base.h"
#include "screen.h"

namespace Interface
{
    enum redraw_t : uint32_t
    {
        // To render the cursor over the previously generated radar map image.
        REDRAW_RADAR_CURSOR = 0x01,
        // To render radar map fully or in ROI and then the cursor over it.
        REDRAW_RADAR = 0x02,
        REDRAW_HEROES = 0x04,
        REDRAW_CASTLES = 0x08,
        REDRAW_BUTTONS = 0x10,
        REDRAW_STATUS = 0x20,
        REDRAW_BORDER = 0x40,
        REDRAW_GAMEAREA = 0x80,

        REDRAW_ICONS = REDRAW_HEROES | REDRAW_CASTLES,
        REDRAW_ALL = 0xFF
    };

    class BaseInterface
    {
    public:
        bool needRedraw() const
        {
            return _redraw != 0;
        }

        void setRedraw( const uint32_t r )
        {
            _redraw |= r;
        }

        uint32_t getRedrawMask() const
        {
            return _redraw;
        }

        static bool isScrollLeft( const fheroes2::Point & cursorPos )
        {
            return cursorPos.x < BORDERWIDTH;
        }

        static bool isScrollRight( const fheroes2::Point & cursorPos )
        {
            const fheroes2::Display & display = fheroes2::Display::instance();

            return cursorPos.x >= display.width() - BORDERWIDTH;
        }

        static bool isScrollTop( const fheroes2::Point & cursorPos )
        {
            return cursorPos.y < BORDERWIDTH;
        }

        static bool isScrollBottom( const fheroes2::Point & cursorPos )
        {
            const fheroes2::Display & display = fheroes2::Display::instance();

            return cursorPos.y >= display.height() - BORDERWIDTH;
        }

    protected:
        uint32_t _redraw{ 0 };
    };
}
