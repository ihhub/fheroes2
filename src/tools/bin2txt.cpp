/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2024                                             *
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
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "serialize.h"
#include "system.h"
#include "tools.h"

namespace
{
    constexpr size_t correctBINSize = 821;
}

int main( int argc, char ** argv )
{
    if ( argc < 3 ) {
        const std::string toolName = System::GetFileName( argv[0] );

        std::cerr << toolName << " extracts various data from monster animation files." << std::endl
                  << "Syntax: " << toolName << " dst_dir input_file.bin ..." << std::endl;
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

    std::error_code ec;

    // Using the non-throwing overloads
    if ( !std::filesystem::exists( dstDir, ec ) && !std::filesystem::create_directories( dstDir, ec ) ) {
        std::cerr << "Cannot create directory " << dstDir << std::endl;
        return EXIT_FAILURE;
    }

    uint32_t filesProcessed = 0;

    for ( const std::string & inputFileName : inputFileNames ) {
        std::ifstream inputStream( inputFileName, std::ios_base::binary | std::ios_base::ate );
        if ( !inputStream ) {
            std::cerr << "Cannot open file " << inputFileName << std::endl;
            // A non-existent or inaccessible file is not considered a fatal error
            continue;
        }

        const auto size = fheroes2::checkedCast<size_t>( static_cast<std::streamoff>( inputStream.tellg() ) );
        if ( !size ) {
            std::cerr << "File " << inputFileName << " is too large" << std::endl;
            // Ignore files of invalid size
            continue;
        }

        if ( size == 0U ) {
            std::cerr << "File " << inputFileName << " is empty" << std::endl;
            // Ignore files of invalid size
            continue;
        }

        if ( size != correctBINSize ) {
            // Ignore files of invalid size
            continue;
        }

        std::cout << "Processing " << inputFileName << "..." << std::endl;

        std::vector<char> buf( size.value() );

        inputStream.seekg( 0, std::ios_base::beg );

        {
            const auto streamSize = fheroes2::checkedCast<std::streamsize>( buf.size() );
            if ( !streamSize ) {
                std::cerr << "File " << inputFileName << " is too large" << std::endl;
                return EXIT_FAILURE;
            }

            inputStream.read( buf.data(), streamSize.value() );
        }

        if ( !inputStream ) {
            std::cerr << "Error reading from file " << inputFileName << std::endl;
            return EXIT_FAILURE;
        }

        const std::filesystem::path outputFilePath = std::filesystem::path( dstDir ) / std::filesystem::path( inputFileName ).filename().replace_extension( "txt" );

        std::ofstream outputStream( outputFilePath, std::ios_base::trunc );
        if ( !outputStream ) {
            std::cerr << "Cannot open file " << outputFilePath << std::endl;
            return EXIT_FAILURE;
        }

        const char * data = buf.data();

        outputStream << "Monster eye position: [" << fheroes2::getLEValue<int16_t>( data, 1 ) << ", " << fheroes2::getLEValue<int16_t>( data, 3 ) << "]\n\n";

        outputStream << "Animation frame offsets:\n";
        for ( size_t setId = 0; setId < 7; ++setId ) {
            outputStream << setId + 1 << " : ";
            for ( size_t frameId = 0; frameId < 16; ++frameId ) {
                const int frameValue = static_cast<unsigned char>( data[5 + setId * 16 + frameId] );

                if ( frameValue < 10 ) {
                    outputStream << " " << frameValue << " ";
                }
                else {
                    outputStream << frameValue << " ";
                }
            }
            outputStream << "\n";
        }
        outputStream << "\n";

        constexpr size_t maxIdleAnimationCount = 5;

        size_t idleAnimationCount = static_cast<unsigned char>( *( data + 117 ) );
        outputStream << "Number of idle animations is " << idleAnimationCount;
        if ( idleAnimationCount > maxIdleAnimationCount ) {
            outputStream << " (INVALID, maximum number should be " << maxIdleAnimationCount << ")";

            idleAnimationCount = maxIdleAnimationCount;
        }
        outputStream << "\n\n";

        outputStream << "Probabilities of each idle animation:\n";
        for ( size_t animId = 0; animId < idleAnimationCount; ++animId ) {
            outputStream << animId + 1 << ": " << fheroes2::getLEValue<float>( data, 118, animId ) << "\n";
        }
        outputStream << "\n";

        outputStream << "Idle animation delay (?) (ms): " << fheroes2::getLEValue<uint32_t>( data, 138, 0 ) << " " << fheroes2::getLEValue<uint32_t>( data, 138, 1 )
                     << " " << fheroes2::getLEValue<uint32_t>( data, 138, 2 ) << " " << fheroes2::getLEValue<uint32_t>( data, 138, 3 ) << " "
                     << fheroes2::getLEValue<uint32_t>( data, 138, 4 ) << "\n\n";

        outputStream << "Idle animation delay (?) (ms): " << fheroes2::getLEValue<uint32_t>( data, 158 ) << "\n\n";
        outputStream << "Walking animation speed (ms): " << fheroes2::getLEValue<uint32_t>( data, 162 ) << "\n\n";
        outputStream << "Shooting animation speed (ms): " << fheroes2::getLEValue<uint32_t>( data, 166 ) << "\n\n";
        outputStream << "Flying animation speed (ms): " << fheroes2::getLEValue<uint32_t>( data, 170 ) << "\n\n";

        outputStream << "Projectile start positions:\n";
        outputStream << "[" << fheroes2::getLEValue<int16_t>( data, 174, 0 ) << ", " << fheroes2::getLEValue<int16_t>( data, 174, 1 ) << "]\n";
        outputStream << "[" << fheroes2::getLEValue<int16_t>( data, 174, 2 ) << ", " << fheroes2::getLEValue<int16_t>( data, 174, 3 ) << "]\n";
        outputStream << "[" << fheroes2::getLEValue<int16_t>( data, 174, 4 ) << ", " << fheroes2::getLEValue<int16_t>( data, 174, 5 ) << "]\n";
        outputStream << "\n";

        const int projectileFramesCount = static_cast<unsigned char>( *( data + 186 ) );
        outputStream << "Number of projectile frames is " << projectileFramesCount << "\n\n";

        outputStream << "Projectile angles:\n";
        for ( size_t angleId = 0; angleId < 12; ++angleId ) {
            outputStream << fheroes2::getLEValue<float>( data, 187, angleId ) << "\n";
        }
        outputStream << "\n";

        outputStream << "Troop count offset: [" << fheroes2::getLEValue<int32_t>( data, 235 ) << ", " << fheroes2::getLEValue<int32_t>( data, 239 ) << "]\n\n";

        constexpr char invalidFrameId = '\xFF';

        outputStream << "Animation sequence (frame IDs):\n";
        for ( size_t setId = 0; setId < 34; ++setId ) {
            outputStream << setId + 1 << " : ";

            int frameCount = 0;
            for ( size_t frameId = 0; frameId < 16; ++frameId ) {
                const size_t offset = 277 + setId * 16 + frameId;

                if ( data[offset] == invalidFrameId ) {
                    break;
                }

                const int frameValue = static_cast<unsigned char>( data[offset] );
                outputStream << frameValue << " ";

                ++frameCount;
            }

            const int expectedFrameCount = static_cast<unsigned char>( data[243 + setId] );
            if ( frameCount != expectedFrameCount ) {
                std::cerr << "WARNING: In " << inputFileName << " file number of animation frames for animation " << setId + 1 << " should be " << expectedFrameCount
                          << " while found number is " << frameCount << std::endl;
            }

            outputStream << "\n";
        }

        if ( !outputStream ) {
            std::cerr << "Error writing to file " << outputFilePath << std::endl;
            return EXIT_FAILURE;
        }

        ++filesProcessed;
    }

    std::cout << "Total processed files: " << filesProcessed << std::endl;

    return EXIT_SUCCESS;
}
