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
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream> // IWYU pragma: keep
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "serialize.h"
#include "system.h"
#include "tools.h"

namespace
{
    constexpr size_t AGGItemNameLen = 15;

    struct AGGItemInfo
    {
        // Hash of this item's name, see calculateHash() for details
        uint32_t hash;
        uint32_t offset;
        uint32_t size;
    };

    uint32_t calculateHash( const std::string_view str )
    {
        uint32_t hash = 0;
        int32_t sum = 0;

        for ( auto iter = str.rbegin(); iter != str.rend(); ++iter ) {
            const char c = static_cast<char>( std::toupper( static_cast<unsigned char>( *iter ) ) );

            hash = ( hash << 5 ) + ( hash >> 25 );

            sum += c;
            hash += sum + c;
        }

        return hash;
    }

    std::optional<std::streamsize> sizeToStreamSize( const size_t size )
    {
        constexpr std::streamsize streamSizeMax = std::numeric_limits<std::streamsize>::max();
        static_assert( streamSizeMax >= 0 );

        const std::make_unsigned_t<std::streamsize> streamSizeMaxUnsigned = streamSizeMax;
        if ( size > streamSizeMaxUnsigned ) {
            return {};
        }

        return static_cast<std::streamsize>( size );
    }
}

int main( int argc, char ** argv )
{
    if ( argc < 3 ) {
        std::string baseName = System::GetBasename( argv[0] );

        std::cerr << baseName << " extracts the contents of the specified AGG file(s)." << std::endl
                  << "Syntax: " << baseName << " dst_dir input_file.agg ..." << std::endl;
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

    uint32_t itemsExtracted = 0;
    uint32_t itemsFailed = 0;

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

        const size_t inputStreamSize = inputStream.size();
        const uint16_t itemsCount = inputStream.getLE16();

        StreamBuf itemsStream = inputStream.toStreamBuf( static_cast<size_t>( itemsCount ) * 4 * 3 /* hash, offset, size */ );
        inputStream.seek( inputStreamSize - AGGItemNameLen * itemsCount );
        StreamBuf namesStream = inputStream.toStreamBuf( AGGItemNameLen * itemsCount );

        std::map<std::string, AGGItemInfo, std::less<>> aggItemsMap;

        for ( uint16_t i = 0; i < itemsCount; ++i ) {
            AGGItemInfo & info = aggItemsMap[StringLower( namesStream.toString( AGGItemNameLen ) )];

            info.hash = itemsStream.getLE32();
            info.offset = itemsStream.getLE32();
            info.size = itemsStream.getLE32();
        }

        for ( const auto & item : aggItemsMap ) {
            const auto & [name, info] = item;

            if ( info.size == 0 ) {
                ++itemsFailed;

                std::cerr << inputFileName << ": item " << name << " is empty" << std::endl;
                continue;
            }

            const uint32_t hash = calculateHash( name );
            if ( hash != info.hash ) {
                ++itemsFailed;

                std::cerr << inputFileName << ": invalid hash for item " << name << ": expected " << GetHexString( info.hash ) << ", got " << GetHexString( hash )
                          << std::endl;
                continue;
            }

            inputStream.seek( info.offset );

            static_assert( std::is_same_v<uint8_t, unsigned char>, "uint8_t is not the same as char, check the logic below" );

            const std::vector<uint8_t> buf = inputStream.getRaw( info.size );
            if ( buf.size() != info.size ) {
                ++itemsFailed;

                std::cerr << inputFileName << ": item " << name << " has an invalid size of " << info.size << std::endl;
                continue;
            }

            const std::filesystem::path outputFilePath = prefixPath / std::filesystem::path( name );

            std::ofstream outputStream( outputFilePath, std::ios_base::binary | std::ios_base::trunc );
            if ( !outputStream ) {
                std::cerr << "Cannot open file " << outputFilePath << std::endl;
                return EXIT_FAILURE;
            }

            {
                const auto streamSize = sizeToStreamSize( buf.size() );
                if ( !streamSize ) {
                    std::cerr << inputFileName << ": item " << name << " is too large" << std::endl;
                    return EXIT_FAILURE;
                }

                outputStream.write( reinterpret_cast<const char *>( buf.data() ), streamSize.value() );
            }

            if ( !outputStream ) {
                std::cerr << "Error writing to file " << outputFilePath << std::endl;
                return EXIT_FAILURE;
            }

            ++itemsExtracted;
        }
    }

    std::cout << "Total extracted: " << itemsExtracted << ", failed: " << itemsFailed << std::endl;

    return ( itemsFailed == 0 ) ? EXIT_SUCCESS : EXIT_FAILURE;
}
