/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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
#include <fstream> // IWYU pragma: keep
#include <iostream>
#include <string>
#include <vector>

#include "serialize.h"

int main( int argc, char ** argv )
{
    if ( argc < 2 ) {
        std::cout << "Please specify input file: " << argv[0] << " <input_monster_frame.bin>" << std::endl;
        return EXIT_SUCCESS;
    }

    const std::string fileName = argv[1];
    std::fstream file;
    file.open( fileName, std::fstream::in | std::fstream::binary );

    if ( !file ) {
        std::cout << "Cannot open " << fileName << std::endl;
        return EXIT_FAILURE;
    }

    file.seekg( 0, std::fstream::end );
    std::streamoff length = file.tellg();

    const std::streamoff correctLength = 821;

    if ( length != correctLength ) {
        std::cout << "Size of " << fileName << " is not equal to " << correctLength << std::endl;
        return EXIT_FAILURE;
    }

    file.seekg( 0, std::fstream::beg );

    std::vector<char> data( correctLength, 0 );

    file.read( data.data(), static_cast<std::streamsize>( correctLength ) );
    file.close();

    if ( data[0] != 0x01 ) {
        std::cout << "This is not a BIN file for monster animation" << std::endl;
        return EXIT_FAILURE;
    }

    file.open( fileName + ".txt", std::fstream::out );
    if ( !file ) {
        std::cout << "Cannot create a new file" << std::endl;
        return EXIT_FAILURE;
    }

    file << "Monster eye position: [" << fheroes2::getLEValue<int16_t>( data.data(), 1 ) << ", " << fheroes2::getLEValue<int16_t>( data.data(), 3 ) << "]\n\n";

    file << "Animation frame offsets:\n";
    for ( size_t setId = 0u; setId < 7; ++setId ) {
        file << setId + 1 << " : ";
        for ( size_t frameId = 0; frameId < 16; ++frameId ) {
            const int frameValue = static_cast<int>( data[5 + setId * 16 + frameId] );
            if ( frameValue < 10 )
                file << " " << frameValue << " ";
            else
                file << frameValue << " ";
        }
        file << "\n";
    }

    file << "\n";

    const int idleAnimationCount = static_cast<int>( *( data.data() + 117 ) );
    if ( idleAnimationCount > 5 ) {
        std::cout << "Idle animation count cannot be more than 5" << std::endl;
        return EXIT_FAILURE;
    }

    file << "Number of idle animations is " << idleAnimationCount << "\n\n";

    file << "Probabilities of each idle animation:\n";
    for ( int i = 0; i < idleAnimationCount; ++i ) {
        file << i + 1 << ": " << fheroes2::getLEValue<float>( data.data(), 118 ) << "\n";
    }
    file << "\n";

    file << "Idle animation delay (?) (ms): " << fheroes2::getLEValue<uint32_t>( data.data(), 138, 0 ) << " " << fheroes2::getLEValue<uint32_t>( data.data(), 138, 1 )
         << " " << fheroes2::getLEValue<uint32_t>( data.data(), 138, 2 ) << " " << fheroes2::getLEValue<uint32_t>( data.data(), 138, 3 ) << " "
         << fheroes2::getLEValue<uint32_t>( data.data(), 138, 4 ) << "\n\n";

    file << "Idle animation delay (?) (ms): " << fheroes2::getLEValue<uint32_t>( data.data(), 158 ) << "\n\n";

    file << "Walking animation speed (ms): " << fheroes2::getLEValue<uint32_t>( data.data(), 162 ) << "\n\n";

    file << "Shooting animation speed (ms): " << fheroes2::getLEValue<uint32_t>( data.data(), 166 ) << "\n\n";

    file << "Flying animation speed (ms): " << fheroes2::getLEValue<uint32_t>( data.data(), 170 ) << "\n\n";

    file << "Projectile start positions:\n";
    file << "[" << fheroes2::getLEValue<int16_t>( data.data(), 174, 0 ) << ", " << fheroes2::getLEValue<int16_t>( data.data(), 174, 1 ) << "]\n";
    file << "[" << fheroes2::getLEValue<int16_t>( data.data(), 174, 2 ) << ", " << fheroes2::getLEValue<int16_t>( data.data(), 174, 3 ) << "]\n";
    file << "[" << fheroes2::getLEValue<int16_t>( data.data(), 174, 4 ) << ", " << fheroes2::getLEValue<int16_t>( data.data(), 174, 5 ) << "]\n\n";

    file << "Number of projectile frames is " << static_cast<int>( *( data.data() + 186 ) ) << "\n\n";

    file << "Projectile angles:\n";
    for ( size_t angleId = 0; angleId < 12; ++angleId )
        file << fheroes2::getLEValue<float>( data.data(), 187, angleId ) << "\n";
    file << "\n";

    file << "Troop count offset: [" << fheroes2::getLEValue<int32_t>( data.data(), 235 ) << ", " << fheroes2::getLEValue<int32_t>( data.data(), 239 ) << "]\n\n";

    file << "Animation sequence (frame IDs):\n";
    const char invalidFrameId = static_cast<char>( 0xFF );
    for ( size_t setId = 0u; setId < 34; ++setId ) {
        file << setId + 1 << " : ";
        int frameCount = 0;
        for ( size_t frameId = 0; frameId < 16; ++frameId ) {
            const char frameValue = data[277 + setId * 16 + frameId];
            if ( frameValue == invalidFrameId )
                break;

            file << static_cast<int>( frameValue ) << " ";
            ++frameCount;
        }

        if ( frameCount != static_cast<int>( data[243 + setId] ) )
            std::cout << "WARNING: In " << fileName << " file number of for animation frames for animation " << setId + 1 << " should be "
                      << static_cast<int>( data[243 + setId] ) << " while found number is " << frameCount << std::endl;

        file << "\n";
    }

    return EXIT_SUCCESS;
}
