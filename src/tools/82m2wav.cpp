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
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

#include "serialize.h"
#include "system.h"
#include "tools.h"

namespace
{
    constexpr size_t wavHeaderLen = 44;
}

int main( int argc, char ** argv )
{
    if ( argc < 3 ) {
        const std::string baseName = System::GetBasename( argv[0] );

        std::cerr << baseName << " converts the specified 82M file(s) to WAV format." << std::endl
                  << "Syntax: " << baseName << " dst_dir input_file.82m ..." << std::endl;
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

    uint32_t tracksConverted = 0;

    for ( const std::string & inputFileName : inputFileNames ) {
        std::cout << "Processing " << inputFileName << "..." << std::endl;

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

        if ( size.value() > std::numeric_limits<uint32_t>::max() - wavHeaderLen ) {
            std::cerr << inputFileName << ": resulting WAV is too large" << std::endl;
            return EXIT_FAILURE;
        }

        const auto buf = std::make_unique<char[]>( size.value() );

        inputStream.seekg( 0, std::ios_base::beg );

        {
            const auto streamSize = fheroes2::checkedCast<std::streamsize>( size.value() );
            if ( !streamSize ) {
                std::cerr << "File " << inputFileName << " is too large" << std::endl;
                return EXIT_FAILURE;
            }

            inputStream.read( buf.get(), streamSize.value() );
        }

        if ( !inputStream ) {
            std::cerr << "Error reading from file " << inputFileName << std::endl;
            return EXIT_FAILURE;
        }

        const std::filesystem::path outputFilePath = std::filesystem::path( dstDir ) / std::filesystem::path( inputFileName ).filename().replace_extension( "wav" );

        std::ofstream outputStream( outputFilePath, std::ios_base::binary | std::ios_base::trunc );
        if ( !outputStream ) {
            std::cerr << "Cannot open file " << outputFilePath << std::endl;
            return EXIT_FAILURE;
        }

        static_assert( std::is_same_v<uint8_t, unsigned char> );

        RWStreamBuf wavHeader( wavHeaderLen );
        wavHeader.putLE32( 0x46464952 ); // RIFF marker ("RIFF")
        wavHeader.putLE32( static_cast<uint32_t>( size.value() ) + ( wavHeaderLen - 8 ) ); // Total size minus the size of this and previous fields
        wavHeader.putLE32( 0x45564157 ); // File type header ("WAVE")
        wavHeader.putLE32( 0x20746D66 ); // Format sub-chunk marker ("fmt ")
        wavHeader.putLE32( 0x10 ); // Size of the format sub-chunk
        wavHeader.putLE16( 0x01 ); // Audio format (1 for PCM)
        wavHeader.putLE16( 0x01 ); // Number of channels
        wavHeader.putLE32( 22050 ); // Sample rate
        wavHeader.putLE32( 22050 ); // Byte rate (SampleRate * BitsPerSample * NumberOfChannels) / 8
        wavHeader.putLE16( 0x01 ); // Block align (BitsPerSample * NumberOfChannels) / 8
        wavHeader.putLE16( 0x08 ); // Bits per sample
        wavHeader.putLE32( 0x61746164 ); // Data sub-chunk marker ("data")
        wavHeader.putLE32( static_cast<uint32_t>( size.value() ) ); // Size of the data sub-chunk

        {
            const auto streamSize = fheroes2::checkedCast<std::streamsize>( wavHeader.size() );
            if ( !streamSize ) {
                std::cerr << inputFileName << ": resulting WAV is too large" << std::endl;
                return EXIT_FAILURE;
            }

            outputStream.write( reinterpret_cast<const char *>( wavHeader.data() ), streamSize.value() );
        }

        {
            const auto streamSize = fheroes2::checkedCast<std::streamsize>( size.value() );
            if ( !streamSize ) {
                std::cerr << inputFileName << ": resulting WAV is too large" << std::endl;
                return EXIT_FAILURE;
            }

            outputStream.write( buf.get(), streamSize.value() );
        }

        if ( !outputStream ) {
            std::cerr << "Error writing to file " << outputFilePath << std::endl;
            return EXIT_FAILURE;
        }

        ++tracksConverted;
    }

    std::cout << "Total converted tracks: " << tracksConverted << std::endl;

    return EXIT_SUCCESS;
}
