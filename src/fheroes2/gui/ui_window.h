/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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

#include "image.h"
#include "math_base.h"
#include "screen.h"

namespace fheroes2
{
    // Standard window with shadow
    class StandardWindow
    {
    public:
        StandardWindow( const int32_t width, const int32_t height, const bool renderBackground, Image & output = Display::instance() );
        StandardWindow( const int32_t x, const int32_t y, const int32_t width, const int32_t height, const bool renderBackground, Image & output = Display::instance() );

        // Returns the window background ROI.
        const Rect & activeArea() const
        {
            return _activeArea;
        }

        // Returns ROI that includes window background and window borders.
        const Rect & windowArea() const
        {
            return _windowArea;
        }

        // Returns ROI that includes window background, borders and window shadow.
        const Rect & totalArea() const
        {
            return _totalArea;
        }

        void render();

    private:
        Image & _output;
        const Rect _activeArea;
        const Rect _windowArea;
        const Rect _totalArea;
        ImageRestorer _restorer;
        const bool _hasBackground{ true };

        void _renderBackground( const bool isEvilInterface );
    };
}
