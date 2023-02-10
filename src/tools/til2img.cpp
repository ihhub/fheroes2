/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2023                                             *
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
#include <cassert>
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

        std::cerr << baseName << " extracts sprites in BMP or PNG format (if supported) from the specified TIL file(s)." << std::endl
                  << "Syntax: " << baseName << " dst_dir input_file.til ..." << std::endl;
        return EXIT_FAILURE;
    }

    const char * dstDir = argv[1];

    std::vector<std::string> inputFileNames;
    for ( int i = 2; i < argc; ++i ) {
        if ( System::isShellLevelGlobbingSupported() ) {
            inputFileNames.emplace_back( argv[i] );
        }
        else {
            System::globFiles( argv[i], inputFileNames );
        }
    }

    uint32_t spritesExtracted = 0;
    uint32_t spritesFailed = 0;

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
        assert( buf.size() == spriteSize * spritesCount );

        for ( uint16_t spriteIdx = 0; spriteIdx < spritesCount; ++spriteIdx ) {
            const size_t spriteOffset = spriteSize * spriteIdx;

            if ( spriteOffset + spriteSize > buf.size() ) {
                ++spritesFailed;

                std::cerr << inputFileName << ": invalid offset for sprite " << spriteIdx << std::endl;
                continue;
            }

            fheroes2::Image sprite( spriteWidth, spriteHeight );

            std::copy( buf.data() + spriteOffset, buf.data() + spriteOffset + spriteSize, sprite.image() );
            std::fill( sprite.transform(), sprite.transform() + spriteSize, static_cast<uint8_t>( 0 ) );

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

            if ( !fheroes2::Save( sprite, outputFileName, spriteBackground ) ) {
                ++spritesFailed;

                std::cerr << inputFileName << ": error saving sprite " << spriteIdx << std::endl;
                continue;
            }

            ++spritesExtracted;
        }
    }

    std::cout << "Total extracted: " << spritesExtracted << ", failed: " << spritesFailed << std::endl;

    return ( spritesExtracted > 0 && spritesFailed == 0 ) ? EXIT_SUCCESS : EXIT_FAILURE;
}
