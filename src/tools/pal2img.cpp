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

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "image.h"
#include "image_palette.h"
#include "image_tool.h"
#include "serialize.h"
#include "system.h"

namespace
{
    constexpr uint8_t spriteBackground = 0;
}

int main( int argc, char ** argv )
{
    if ( argc < 3 ) {
        std::string baseName = System::GetBasename( argv[0] );

        std::cerr << baseName << " generate an image with colors based on a provided palette file." << std::endl
                  << "Syntax: " << baseName << " palette_file.pal output.bmp" << std::endl;
        return EXIT_FAILURE;
    }

    const char * paletteFileName = argv[1];
    const char * outputImage = argv[2];

    {
        StreamFile paletteStream;
        if ( !paletteStream.open( paletteFileName, "rb" ) ) {
            std::cerr << "Cannot open file " << paletteFileName << std::endl;
            return EXIT_FAILURE;
        }

        const std::vector<uint8_t> palette = paletteStream.getRaw();
        if ( palette.size() != 768 ) {
            std::cerr << "Invalid palette size of " << palette.size() << " instead of 768" << std::endl;
            return EXIT_FAILURE;
        }

        fheroes2::setGamePalette( palette );
    }

    fheroes2::Image output( 256, 256 );
    output.reset();
    // We do not need to care about the transform layer.
    output._disableTransformLayer();

    for ( uint8_t y = 0; y < 16; ++y ) {
        for ( uint8_t x = 0; x < 16; ++x ) {
            fheroes2::Fill( output, x * 16, y * 16, 16, 16, x + y * 16 );
        }
    }

    fheroes2::Save( output, outputImage );

    return EXIT_SUCCESS;
}
