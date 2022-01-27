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

#include "image_palette.h"
#include "palette_h2.h"

#include <algorithm>
#include <array>
#include <cassert>

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
            std::copy_n( kb_pal, paletteSize, gamePalette.begin() );
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
