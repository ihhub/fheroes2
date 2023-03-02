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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>

#include "image_palette.h"

namespace
{
    const size_t paletteSize = 768;

    struct PaletteHolder
    {
    public:
        static PaletteHolder & instance()
        {
            static PaletteHolder paletteHolder;
            return paletteHolder;
        }

        std::array<uint8_t, paletteSize> gamePalette;

    private:
        PaletteHolder()
        {
            // This is a temporary palette which is used ONLY for cursor and game startup error message. A real palette must be loaded from AGG file.
            gamePalette
                = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x35, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1c, 0x1c, 0x1c,
                    0x1a, 0x1a, 0x1a, 0x17, 0x17, 0x17, 0x15, 0x15, 0x15, 0x12, 0x12, 0x12, 0x10, 0x10, 0x10, 0x0e, 0x0e, 0x0e, 0x0b, 0x0b, 0x0b, 0x09, 0x09, 0x09, 0x06,
                    0x06, 0x06, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x3f, 0x3b, 0x37, 0x3c, 0x37, 0x32, 0x3a, 0x34, 0x2e, 0x38, 0x31, 0x2a, 0x36, 0x2e,
                    0x26, 0x34, 0x2a, 0x22, 0x32, 0x28, 0x1e, 0x30, 0x25, 0x1b, 0x2e, 0x22, 0x18, 0x2b, 0x1f, 0x15, 0x29, 0x1c, 0x12, 0x27, 0x1a, 0x0f, 0x25, 0x18, 0x0d,
                    0x23, 0x15, 0x0b, 0x21, 0x13, 0x08, 0x1f, 0x11, 0x07, 0x1d, 0x0f, 0x05, 0x1a, 0x0d, 0x04, 0x18, 0x0c, 0x03, 0x16, 0x0a, 0x02, 0x14, 0x09, 0x01, 0x12,
                    0x07, 0x01, 0x0f, 0x06, 0x00, 0x0d, 0x05, 0x00, 0x0b, 0x04, 0x00, 0x09, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f,
                    0x3d, 0x34, 0x3e, 0x3a, 0x2b, 0x3d, 0x38, 0x23, 0x00, 0x00, 0x00, 0x3b, 0x35, 0x14, 0x3a, 0x33, 0x0d, 0x39, 0x32, 0x05, 0x38, 0x31, 0x00, 0x36, 0x2f,
                    0x08, 0x34, 0x2c, 0x07, 0x32, 0x28, 0x06, 0x2f, 0x26, 0x06, 0x2d, 0x23, 0x06, 0x2a, 0x1f, 0x05, 0x27, 0x1c, 0x04, 0x25, 0x19, 0x03, 0x22, 0x16, 0x03,
                    0x1f, 0x13, 0x02, 0x1d, 0x11, 0x02, 0x1a, 0x0f, 0x00, 0x18, 0x0c, 0x00, 0x15, 0x0a, 0x00, 0x13, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x39, 0x27, 0x3e, 0x36, 0x23,
                    0x3d, 0x34, 0x1f, 0x3c, 0x31, 0x1c, 0x3b, 0x2e, 0x18, 0x3a, 0x2b, 0x14, 0x39, 0x28, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x12, 0x02, 0x20, 0x0f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x12, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        }
    };
}

namespace fheroes2
{
    const uint8_t * getGamePalette()
    {
        return PaletteHolder::instance().gamePalette.data();
    }

    void setGamePalette( const std::vector<uint8_t> & palette )
    {
        assert( palette.size() == paletteSize );
        if ( palette.size() != paletteSize ) {
            return;
        }

        std::copy_n( palette.begin(), paletteSize, PaletteHolder::instance().gamePalette.begin() );
    }
}
