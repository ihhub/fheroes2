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
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream> // IWYU pragma: keep
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "h2d_file.h"
#include "system.h"

namespace
{
    void printUsage( char ** argv )
    {
        std::string baseName = System::GetBasename( argv[0] );

        std::cerr << baseName << " manages the contents of the specified H2D file(s)." << std::endl
                  << "Syntax: " << baseName << " extract dst_dir input_file.h2d ..." << std::endl
                  << "        " << baseName << " append target_file.h2d input_file ..." << std::endl;
    }

    int extractH2D( const int argc, char ** argv )
    {
        assert( argc >= 4 );

        const char * dstDir = argv[2];

        std::vector<std::string> inputFileNames;
        for ( int i = 3; i < argc; ++i ) {
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
                static_assert( std::is_same_v<uint8_t, unsigned char>, "uint8_t is not the same as char, check the logic below" );

                const std::vector<uint8_t> buf = reader.getFile( name );

                const std::filesystem::path outputFilePath = prefixPath / std::filesystem::path( name );

                std::ofstream outputStream( outputFilePath, std::ios_base::binary | std::ios_base::trunc );
                if ( !outputStream ) {
                    std::cerr << "Cannot open file " << outputFilePath << std::endl;
                    return EXIT_FAILURE;
                }

                outputStream.write( reinterpret_cast<const char *>( buf.data() ), buf.size() );
                if ( !outputStream ) {
                    std::cerr << "Error writing to file " << outputFilePath << std::endl;
                    return EXIT_FAILURE;
                }

                ++itemsExtracted;
            }
        }

        std::cout << "Total extracted: " << itemsExtracted << std::endl;

        return EXIT_SUCCESS;
    }

    int appendH2D( const int argc, char ** argv )
    {
        assert( argc >= 4 );

        const char * h2dFileName = argv[2];

        std::vector<std::string> inputFileNames;
        for ( int i = 3; i < argc; ++i ) {
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

        uint32_t itemsAppended = 0;

        for ( const std::string & inputFileName : inputFileNames ) {
            std::cout << "Processing " << inputFileName << "..." << std::endl;

            std::ifstream inputStream( inputFileName, std::ios_base::binary | std::ios_base::ate );
            if ( !inputStream ) {
                std::cerr << "Cannot open file " << inputFileName << std::endl;
                // A non-existent or inaccessible file is not considered a fatal error
                continue;
            }

            const std::streampos pos = inputStream.tellg();
            if ( pos <= 0 ) {
                std::cerr << "File " << inputFileName << " is empty" << std::endl;
                return EXIT_FAILURE;
            }

            const std::make_unsigned_t<std::streamoff> posOffset = pos;
            if ( posOffset > std::numeric_limits<size_t>::max() ) {
                std::cerr << "File " << inputFileName << " is too large" << std::endl;
                return EXIT_FAILURE;
            }

            const size_t size = static_cast<size_t>( posOffset );

            static_assert( std::is_same_v<uint8_t, unsigned char>, "uint8_t is not the same as char, check the logic below" );

            std::vector<uint8_t> buf( size );

            inputStream.seekg( 0, std::ios_base::beg );
            inputStream.read( reinterpret_cast<char *>( buf.data() ), buf.size() );
            if ( !inputStream ) {
                std::cerr << "Error reading from file " << inputFileName << std::endl;
                return EXIT_FAILURE;
            }

            if ( !writer.add( std::filesystem::path( inputFileName ).filename().string(), buf ) ) {
                std::cerr << "Error appending file " << inputFileName << std::endl;
                return EXIT_FAILURE;
            }

            ++itemsAppended;
        }

        if ( !writer.write( h2dFileName ) ) {
            std::cerr << "Error writing to file " << h2dFileName << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Total appended: " << itemsAppended << std::endl;

        return EXIT_SUCCESS;
    }
}

int main( int argc, char ** argv )
{
    if ( argc >= 4 && strcmp( argv[1], "extract" ) == 0 ) {
        return extractH2D( argc, argv );
    }

    if ( argc >= 4 && strcmp( argv[1], "append" ) == 0 ) {
        return appendH2D( argc, argv );
    }

    printUsage( argv );

    return EXIT_FAILURE;
}
