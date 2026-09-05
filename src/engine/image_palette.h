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
    constexpr size_t paletteSize = 256;

    struct RGB final
    {
        uint8_t r{ 0 };
        uint8_t g{ 0 };
        uint8_t b{ 0 };

        RGB() = default;

        RGB( const uint8_t red, const uint8_t green, const uint8_t blue )
            : r( red )
            , g( green )
            , b( blue )
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

        uint32_t getBGRX() const
        {
            return ( static_cast<uint32_t>( r ) << 16U ) + ( static_cast<uint32_t>( g ) << 8U ) + static_cast<uint32_t>( b );
        }

        uint32_t getRGBX() const
        {
            return ( static_cast<uint32_t>( b ) << 16U ) + ( static_cast<uint32_t>( g ) << 8U ) + static_cast<uint32_t>( r );
        }
    };

    // Game palette has 256 values for red, green and blue, so its size is: 256 * 3 = 768.
    constexpr size_t paletteSizeBytes = paletteSize * sizeof( RGB );

    // Get the original game palette with color data in range: 0-63.
    const RGB * getRGBGamePalette();

    // Get the normalized game palette with color data in range: 0-255.
    std::array<RGB, paletteSize> getNormalizedRGBGamePalette();

    // This function must be called only at the start of the application after loading AGG file content.
    void setGamePalette( const std::vector<uint8_t> & palette );
}
