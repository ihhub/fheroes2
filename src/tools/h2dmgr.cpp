/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

#include "h2d_file.h"
#include "image.h"
#include "image_palette.h"
#include "image_tool.h"
#include "serialize.h"
#include "system.h"
#include "tools.h"

namespace
{
    constexpr size_t validPaletteSize = 768;
    constexpr uint8_t imageBackground = 142;

    bool isH2DImageItem( const std::string_view name )
    {
        return std::filesystem::path( name ).extension() == ".image";
    }

    bool isImageFile( const std::string_view fileName )
    {
        const std::string extension = StringLower( std::filesystem::path( fileName ).extension().string() );

        return ( extension == ".bmp" || extension == ".png" );
    }

    void printUsage( char ** argv )
    {
        const std::string baseName = System::GetBasename( argv[0] );

        std::cerr << baseName << " manages the contents of the specified H2D file(s)." << std::endl
                  << "Syntax: " << baseName << " extract dst_dir palette_file.pal input_file.h2d ..." << std::endl
                  << "        " << baseName << " combine target_file.h2d palette_file.pal input_file ..." << std::endl;
    }

    bool loadPalette( const char * paletteFileName )
    {
        StreamFile paletteStream;
        if ( !paletteStream.open( paletteFileName, "rb" ) ) {
            std::cerr << "Cannot open file " << paletteFileName << std::endl;
            return false;
        }

        const std::vector<uint8_t> palette = paletteStream.getRaw( 0 );
        if ( palette.size() != validPaletteSize ) {
            std::cerr << "Invalid palette size of " << palette.size() << " instead of " << validPaletteSize << std::endl;
            return false;
        }

        fheroes2::setGamePalette( palette );
        return true;
    }

    int extractH2D( const int argc, char ** argv )
    {
        assert( argc >= 5 );

        const char * dstDir = argv[2];

        if ( !loadPalette( argv[3] ) ) {
            return EXIT_FAILURE;
        }

        std::vector<std::string> inputFileNames;
        for ( int i = 4; i < argc; ++i ) {
            if ( System::isShellLevelGlobbingSupported() ) {
                inputFileNames.emplace_back( argv[i] );
            }
            else {
                System::globFiles( argv[i], inputFileNames );
            }
        }

        uint32_t itemsExtracted = 0;

        for ( const std::string & inputFileName : inputFileNames ) {
            std::cout << "Processing " << inputFileName << "..." << std::endl;

            fheroes2::H2DReader reader;
            if ( !reader.open( inputFileName ) ) {
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

            for ( const std::string & name : reader.getAllFileNames() ) {
                // Image items need special processing
                if ( isH2DImageItem( name ) ) {
                    fheroes2::Sprite image;

                    if ( !fheroes2::readImageFromH2D( reader, name, image ) ) {
                        std::cerr << inputFileName << ": item " << name << " contains an invalid image" << std::endl;
                        return EXIT_FAILURE;
                    }

                    std::string outputFileName = ( prefixPath / std::filesystem::path( name ).stem() ).string();

                    if ( fheroes2::isPNGFormatSupported() ) {
                        outputFileName += ".png";
                    }
                    else {
                        outputFileName += ".bmp";
                    }

                    if ( !fheroes2::Save( image, outputFileName, imageBackground ) ) {
                        std::cerr << inputFileName << ": error saving image " << name << std::endl;
                        return EXIT_FAILURE;
                    }
                }
                else {
                    static_assert( std::is_same_v<uint8_t, unsigned char> );

                    const std::vector<uint8_t> buf = reader.getFile( name );

                    const std::filesystem::path outputFilePath = prefixPath / std::filesystem::path( name );

                    std::ofstream outputStream( outputFilePath, std::ios_base::binary | std::ios_base::trunc );
                    if ( !outputStream ) {
                        std::cerr << "Cannot open file " << outputFilePath << std::endl;
                        return EXIT_FAILURE;
                    }

                    const auto streamSize = fheroes2::checkedCast<std::streamsize>( buf.size() );
                    if ( !streamSize ) {
                        std::cerr << inputFileName << ": item " << name << " is too large" << std::endl;
                        return EXIT_FAILURE;
                    }

                    outputStream.write( reinterpret_cast<const char *>( buf.data() ), streamSize.value() );
                    if ( !outputStream ) {
                        std::cerr << "Error writing to file " << outputFilePath << std::endl;
                        return EXIT_FAILURE;
                    }
                }

                ++itemsExtracted;
            }
        }

        std::cout << "Total extracted items: " << itemsExtracted << std::endl;

        return EXIT_SUCCESS;
    }

    int combineH2D( const int argc, char ** argv )
    {
        assert( argc >= 5 );

        const char * h2dFileName = argv[2];

        if ( !loadPalette( argv[3] ) ) {
            return EXIT_FAILURE;
        }

        std::vector<std::string> inputFileNames;
        for ( int i = 4; i < argc; ++i ) {
            if ( System::isShellLevelGlobbingSupported() ) {
                inputFileNames.emplace_back( argv[i] );
            }
            else {
                System::globFiles( argv[i], inputFileNames );
            }
        }

        fheroes2::H2DWriter writer;

        std::error_code ec;

        // Using the non-throwing overload
        if ( std::filesystem::exists( h2dFileName, ec ) ) {
            const std::filesystem::path h2dFileBackupPath = std::filesystem::path( h2dFileName ).replace_extension( "bak" );

            // Using the non-throwing overload
            if ( !std::filesystem::copy_file( h2dFileName, h2dFileBackupPath, std::filesystem::copy_options::overwrite_existing, ec ) ) {
                std::cerr << "Cannot create backup file " << h2dFileBackupPath << std::endl;
                return EXIT_FAILURE;
            }

            fheroes2::H2DReader reader;
            if ( !reader.open( h2dFileName ) ) {
                std::cerr << "Cannot open file " << h2dFileName << std::endl;
                return EXIT_FAILURE;
            }

            if ( !writer.add( reader ) ) {
                std::cerr << "Error reading from file " << h2dFileName << std::endl;
                return EXIT_FAILURE;
            }
        }

        uint32_t itemsAdded = 0;

        for ( const std::string & inputFileName : inputFileNames ) {
            std::cout << "Processing " << inputFileName << "..." << std::endl;

            // Image files need special processing
            if ( isImageFile( inputFileName ) ) {
                fheroes2::Image image;

                if ( !fheroes2::Load( inputFileName, image ) ) {
                    std::cerr << "Cannot open file " << inputFileName << std::endl;
                    // A non-existent or inaccessible file is not considered a fatal error
                    continue;
                }

                if ( image.empty() ) {
                    std::cerr << "File " << inputFileName << " contains an empty image" << std::endl;
                    return EXIT_FAILURE;
                }

                if ( !fheroes2::writeImageToH2D( writer, std::filesystem::path( inputFileName ).filename().replace_extension( "image" ).string(), image ) ) {
                    std::cerr << "Error adding file " << inputFileName << std::endl;
                    return EXIT_FAILURE;
                }
            }
            else {
                std::ifstream inputStream( inputFileName, std::ios_base::binary | std::ios_base::ate );
                if ( !inputStream ) {
                    std::cerr << "Cannot open file " << inputFileName << std::endl;
                    // A non-existent or inaccessible file is not considered a fatal error
                    continue;
                }

                const auto size = fheroes2::checkedCast<size_t>( static_cast<std::streamoff>( inputStream.tellg() ) );
                if ( !size ) {
                    std::cerr << "File " << inputFileName << " is too large" << std::endl;
                    return EXIT_FAILURE;
                }

                if ( size == 0U ) {
                    std::cerr << "File " << inputFileName << " is empty" << std::endl;
                    return EXIT_FAILURE;
                }

                static_assert( std::is_same_v<uint8_t, unsigned char> );

                std::vector<uint8_t> buf( size.value() );

                inputStream.seekg( 0, std::ios_base::beg );

                const auto streamSize = fheroes2::checkedCast<std::streamsize>( buf.size() );
                if ( !streamSize ) {
                    std::cerr << "File " << inputFileName << " is too large" << std::endl;
                    return EXIT_FAILURE;
                }

                inputStream.read( reinterpret_cast<char *>( buf.data() ), streamSize.value() );
                if ( !inputStream ) {
                    std::cerr << "Error reading from file " << inputFileName << std::endl;
                    return EXIT_FAILURE;
                }

                if ( !writer.add( std::filesystem::path( inputFileName ).filename().string(), buf ) ) {
                    std::cerr << "Error adding file " << inputFileName << std::endl;
                    return EXIT_FAILURE;
                }
            }

            ++itemsAdded;
        }

        if ( !writer.write( h2dFileName ) ) {
            std::cerr << "Error writing to file " << h2dFileName << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Total added items: " << itemsAdded << std::endl;

        return EXIT_SUCCESS;
    }
}

int main( int argc, char ** argv )
{
    if ( argc >= 5 && strcmp( argv[1], "extract" ) == 0 ) {
        return extractH2D( argc, argv );
    }

    if ( argc >= 5 && strcmp( argv[1], "combine" ) == 0 ) {
        return combineH2D( argc, argv );
    }

    printUsage( argv );

    return EXIT_FAILURE;
}
