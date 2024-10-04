/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
    constexpr size_t validPaletteSize = 768;
    constexpr uint8_t spriteBackground = 0;
}

int main( int argc, char ** argv )
{
    if ( argc < 4 ) {
        const std::string baseName = System::GetBasename( argv[0] );

        std::cerr << baseName << " extracts sprites in BMP or PNG format (if supported) from the specified TIL file(s) using the specified palette." << std::endl
                  << "Syntax: " << baseName << " dst_dir palette_file.pal input_file.til ..." << std::endl;
        return EXIT_FAILURE;
    }

    const char * dstDir = argv[1];
    const char * paletteFileName = argv[2];

    {
        StreamFile paletteStream;
        if ( !paletteStream.open( paletteFileName, "rb" ) ) {
            std::cerr << "Cannot open file " << paletteFileName << std::endl;
            return EXIT_FAILURE;
        }

        const std::vector<uint8_t> palette = paletteStream.getRaw( 0 );
        if ( palette.size() != validPaletteSize ) {
            std::cerr << "Invalid palette size of " << palette.size() << " instead of " << validPaletteSize << std::endl;
            return EXIT_FAILURE;
        }

        fheroes2::setGamePalette( palette );
    }

    std::vector<std::string> inputFileNames;
    for ( int i = 3; i < argc; ++i ) {
        if ( System::isShellLevelGlobbingSupported() ) {
            inputFileNames.emplace_back( argv[i] );
        }
        else {
            System::globFiles( argv[i], inputFileNames );
        }
    }

    uint32_t spritesExtracted = 0;

    for ( const std::string & inputFileName : inputFileNames ) {
        std::cout << "Processing " << inputFileName << "..." << std::endl;

        StreamFile inputStream;
        if ( !inputStream.open( inputFileName, "rb" ) ) {
            std::cerr << "Cannot open file " << inputFileName << std::endl;
            // A non-existent or inaccessible file is not considered a fatal error
            continue;
        }

        const std::filesystem::path prefixPath = std::filesystem::path( dstDir ) / std::filesystem::path( inputFileName ).stem();

        std::error_code ec;

        // Using the non-throwing overloads
        if ( !std::filesystem::exists( prefixPath, ec ) && !std::filesystem::create_directories( prefixPath, ec ) ) {
            std::cerr << "Cannot create directory " << prefixPath << std::endl;
            return EXIT_FAILURE;
        }

        const uint16_t spritesCount = inputStream.getLE16();
        if ( spritesCount == 0 ) {
            std::cerr << inputFileName << ": no sprites found" << std::endl;
            return EXIT_FAILURE;
        }

        const uint16_t spriteWidth = inputStream.getLE16();
        const uint16_t spriteHeight = inputStream.getLE16();

        const size_t spriteSize = static_cast<size_t>( spriteWidth ) * spriteHeight;
        if ( spriteSize == 0 ) {
            std::cerr << inputFileName << ": invalid sprite size " << spriteWidth << "x" << spriteHeight << std::endl;
            return EXIT_FAILURE;
        }

        const std::vector<uint8_t> buf = inputStream.getRaw( spriteSize * spritesCount );
        if ( buf.size() != spriteSize * spritesCount ) {
            std::cerr << inputFileName << ": failed to extract sprites" << std::endl;
            return EXIT_FAILURE;
        }

        std::vector<fheroes2::Image> sprites;
        sprites.reserve( spritesCount );

        fheroes2::decodeTILImages( buf.data(), spritesCount, spriteWidth, spriteHeight, sprites );
        if ( sprites.size() != spritesCount ) {
            std::cerr << inputFileName << ": failed to extract sprites" << std::endl;
            return EXIT_FAILURE;
        }

        for ( size_t spriteIdx = 0; spriteIdx < sprites.size(); ++spriteIdx ) {
            std::ostringstream spriteIdxStream;
            spriteIdxStream << std::setw( 3 ) << std::setfill( '0' ) << spriteIdx;

            const std::string spriteIdxStr = spriteIdxStream.str();
            std::string outputFileName = ( prefixPath / spriteIdxStr ).string();

            if ( fheroes2::isPNGFormatSupported() ) {
                outputFileName += ".png";
            }
            else {
                outputFileName += ".bmp";
            }

            if ( !fheroes2::Save( sprites[spriteIdx], outputFileName, spriteBackground ) ) {
                std::cerr << inputFileName << ": error saving sprite " << spriteIdx << std::endl;
                return EXIT_FAILURE;
            }

            ++spritesExtracted;
        }
    }

    std::cout << "Total extracted sprites: " << spritesExtracted << std::endl;

    return EXIT_SUCCESS;
}
