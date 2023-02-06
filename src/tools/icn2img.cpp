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

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream> // IWYU pragma: keep
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

#include "agg_file.h"
#include "image.h"
#include "image_palette.h"
#include "image_tool.h"
#include "serialize.h"
#include "system.h"

int main( int argc, char ** argv )
{
    if ( argc < 4 ) {
        std::cerr << argv[0] << " destinationDirectory paletteFileName.pal inputFile.icn ..." << std::endl;
        return EXIT_FAILURE;
    }

    const char * dstDir = argv[1];
    const char * paletteFileName = argv[2];

    StreamFile paletteFile;
    if ( !paletteFile.open( paletteFileName, "rb" ) ) {
        std::cerr << "Cannot open " << paletteFileName << std::endl;
        return EXIT_FAILURE;
    }

    const std::vector<uint8_t> palette = paletteFile.getRaw();
    if ( palette.size() != 768 ) {
        std::cerr << "Invalid palette size of " << palette.size() << " instead of 768." << std::endl;
        return EXIT_FAILURE;
    }

    fheroes2::setGamePalette( palette );

    std::vector<std::string> inputFileNames;
    for ( int i = 3; i < argc; ++i ) {
        if ( System::isShellLevelGlobbingSupported() ) {
            inputFileNames.emplace_back( argv[i] );
        }
        else {
            System::globFiles( argv[i], inputFileNames );
        }
    }

    for ( const std::string & inputFileName : inputFileNames ) {
        StreamFile sf;

        if ( !sf.open( inputFileName, "rb" ) ) {
            std::cerr << "Cannot open " << inputFileName << std::endl;
            return EXIT_FAILURE;
        }

        const std::filesystem::path prefix = std::filesystem::path( dstDir ) / std::filesystem::path( inputFileName ).stem();

        std::error_code ec;

        // Using the non-throwing overloads
        if ( !std::filesystem::exists( prefix, ec ) && !std::filesystem::create_directories( prefix, ec ) ) {
            std::cerr << "Cannot create directory " << prefix << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << std::endl << "Processing " << inputFileName << "..." << std::endl << std::endl;

        const uint16_t spritesCount = sf.getLE16();
        const uint32_t totalSize = sf.getLE32();

        const size_t beginPos = sf.tell();

        std::vector<fheroes2::ICNHeader> headers( spritesCount );
        for ( fheroes2::ICNHeader & header : headers ) {
            sf >> header;
        }

        for ( uint16_t spriteIdx = 0; spriteIdx < spritesCount; ++spriteIdx ) {
            const fheroes2::ICNHeader & header = headers[spriteIdx];

            sf.seek( beginPos + header.offsetData );

            const uint32_t dataSize = ( spriteIdx + 1 < spritesCount ? headers[spriteIdx + 1].offsetData - header.offsetData : totalSize - header.offsetData );

            const std::vector<uint8_t> buf = sf.getRaw( dataSize );
            if ( buf.empty() ) {
                continue;
            }

            const fheroes2::Sprite image = fheroes2::decodeICNSprite( buf.data(), dataSize, header.width, header.height, header.offsetX, header.offsetY );

            std::ostringstream os;
            os << std::setw( 3 ) << std::setfill( '0' ) << spriteIdx;

            std::string dstFileName = ( prefix / os.str() ).string();

            if ( fheroes2::isPNGFormatSupported() ) {
                dstFileName += ".png";
            }
            else {
                dstFileName += ".bmp";
            }

            static_assert( std::is_same_v<decltype( header.offsetX ), uint16_t> && std::is_same_v<decltype( header.offsetY ), uint16_t>,
                           "Offset types have been changed, check the casts below" );

            std::cout << "Image " << spriteIdx + 1 << " has offset of [" << static_cast<int16_t>( header.offsetX ) << ", " << static_cast<int16_t>( header.offsetY )
                      << "]" << std::endl;

            fheroes2::Save( image, dstFileName, 23 );
        }
    }

    return EXIT_SUCCESS;
}
