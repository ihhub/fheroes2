/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2026                                             *
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

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace fheroes2
{
    constexpr size_t RGBPaletteSize = 256;

    struct RGB final
    {
        uint8_t r{ 0 };
        uint8_t g{ 0 };
        uint8_t b{ 0 };

        RGB() = default;

        RGB( const uint8_t r, const uint8_t g, const uint8_t b )
            : r( r )
            , g( g )
            , b( b )
        {
            // Do nothing.
        }

        uint32_t getBGRA() const
        {
            return ( 255U << 24U ) + ( static_cast<uint32_t>( r ) << 16U ) + ( static_cast<uint32_t>( g ) << 8U ) + static_cast<uint32_t>( b );
        }

        uint32_t getRGBA() const
        {
            return ( 255U << 24U ) + ( static_cast<uint32_t>( b ) << 16U ) + ( static_cast<uint32_t>( g ) << 8U ) + static_cast<uint32_t>( r );
        }

        uint32_t getBGR() const
        {
            return ( static_cast<uint32_t>( r ) << 16U ) + ( static_cast<uint32_t>( g ) << 8U ) + static_cast<uint32_t>( b );
        }

        uint32_t getRGB() const
        {
            return ( static_cast<uint32_t>( b ) << 16U ) + ( static_cast<uint32_t>( g ) << 8U ) + static_cast<uint32_t>( r );
        }

        RGB operator<<( const uint8_t bits ) const
        {
            return { static_cast<uint8_t>( r << bits ), static_cast<uint8_t>( g << bits ), static_cast<uint8_t>( b << bits ) };
        }
    };

    const uint8_t * getGamePalette();

    // Get the original game palette with color data in range: 0-63.
    const RGB * getRGBGamePalette();

    // Get the normalized game palette with color data in range: 0-255.
    std::array<RGB, RGBPaletteSize> getNormalizedRGBGamePalette();

    // This function must be called only at the start of the application after loading AGG file content.
    void setGamePalette( const std::vector<uint8_t> & palette );
}
