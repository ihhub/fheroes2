/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
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

    uint32_t calculateHash( const std::string & str )
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
}

int main( int argc, char ** argv )
{
    if ( argc != 3 ) {
        std::string baseName = System::GetBasename( argv[0] );

        std::cerr << baseName << " extracts the contents of the specified AGG file." << std::endl << "Syntax: " << baseName << " input_file.agg dst_dir" << std::endl;
        return EXIT_FAILURE;
    }

    StreamFile inputStream;
    if ( !inputStream.open( argv[1], "rb" ) ) {
        std::cerr << "Cannot open file " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }

    std::error_code ec;

    // Using the non-throwing overloads
    if ( !std::filesystem::exists( argv[2], ec ) && !std::filesystem::create_directories( argv[2], ec ) ) {
        std::cerr << "Cannot create directory " << argv[2] << std::endl;
        return EXIT_FAILURE;
    }

    const size_t inputStreamSize = inputStream.size();
    const uint16_t itemsCount = inputStream.getLE16();

    StreamBuf itemsStream = inputStream.toStreamBuf( itemsCount * 4 * 3 /* hash, offset, size */ );
    inputStream.seek( inputStreamSize - AGGItemNameLen * itemsCount );
    StreamBuf namesStream = inputStream.toStreamBuf( AGGItemNameLen * itemsCount );

    std::map<std::string, AGGItemInfo> aggItemsMap;

    for ( uint16_t i = 0; i < itemsCount; ++i ) {
        AGGItemInfo & info = aggItemsMap[StringLower( namesStream.toString( AGGItemNameLen ) )];

        info.hash = itemsStream.getLE32();
        info.offset = itemsStream.getLE32();
        info.size = itemsStream.getLE32();
    }

    uint16_t itemsExtracted = 0;
    uint16_t itemsFailed = 0;

    for ( const auto & item : aggItemsMap ) {
        const auto & [name, info] = item;

        std::cout << "Processing " << name << "..." << std::endl;

        const uint32_t hash = calculateHash( name );
        if ( hash != info.hash ) {
            ++itemsFailed;

            std::cerr << "Invalid hash for item " << name << ": expected " << GetHexString( info.hash ) << ", got " << GetHexString( hash ) << std::endl;
            continue;
        }

        inputStream.seek( info.offset );

        const std::vector<uint8_t> buf = inputStream.getRaw( info.size );
        if ( buf.empty() ) {
            ++itemsFailed;

            std::cerr << "Empty item " << name << std::endl;
            continue;
        }

        const std::filesystem::path outputFileName = std::filesystem::path( argv[2] ) / std::filesystem::path( name );

        StreamFile outputStream;
        if ( !outputStream.open( outputFileName.string(), "wb" ) ) {
            ++itemsFailed;

            std::cerr << "Cannot open file " << outputFileName << std::endl;
            continue;
        }

        outputStream.putRaw( reinterpret_cast<const char *>( buf.data() ), buf.size() );

        ++itemsExtracted;
    }

    std::cout << "Total extracted: " << itemsExtracted << ", failed: " << itemsFailed << std::endl;
    return EXIT_SUCCESS;
}
