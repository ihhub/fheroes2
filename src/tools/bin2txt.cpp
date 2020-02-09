#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main( int argc, char ** argv )
{
    if ( argc < 2 ) {
        std::cout << argv[0] << " monster_frm.bin" << std::endl;
        return EXIT_SUCCESS;
    }

    const std::string fileName = argv[1];
    std::fstream file;
    file.open( fileName, std::fstream::in | std::fstream::binary );

    if ( !file ) {
        std::cout << "Cannot open " << fileName << std::endl;
        return EXIT_FAILURE;
    }

    file.seekg( 0, file.end );
    std::streamoff length = file.tellg();

    const std::streamoff correctLength = 821;

    if ( length != correctLength ) {
        std::cout << "Size of " << fileName << " is not equal to " << correctLength << std::endl;
        return EXIT_FAILURE;
    }

    file.seekg( 0, file.beg );

    std::vector<char> data( correctLength, 0 );

    file.read( data.data(), static_cast<std::streamsize>( correctLength ) );
    file.close();

    file.open( fileName + ".txt", std::fstream::out );
    if ( !file ) {
        std::cout << "Cannot create a new file" << std::endl;
        return EXIT_FAILURE;
    }

    file << "Animation frame offsets:\n";
    for ( size_t setId = 0u; setId < 7; ++setId ) {
        file << setId + 1 << " : ";
        for ( size_t frameId = 0; frameId < 16; ++frameId ) {
            const int frameValue = static_cast<int>( data[5 + setId * 16 + frameId] );
            if ( frameValue < 10 )
                file << " " << static_cast<int>( frameValue ) << " ";
            else
                file << static_cast<int>( frameValue ) << " ";
        }
        file << "\n";
    }

    file << "\nAnimation speed (ms): " <<
        *( reinterpret_cast<uint32_t*>( data.data() + 138 ) ) << " " <<
        *( reinterpret_cast<uint32_t*>( data.data() + 138 + 4 ) ) << " " <<
        *( reinterpret_cast<uint32_t*>( data.data() + 138 + 8 ) ) << " " <<
        *( reinterpret_cast<uint32_t*>( data.data() + 138 + 12 ) ) << "\n\n";

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
            std::cout << "WARNING: In " << fileName << " file number of for animation frames for animation " << setId + 1 <<
                         " should be " << static_cast<int>( data[243 + setId] ) << " while found number is " << frameCount << std::endl;

        file << "\n";
    }

    return EXIT_SUCCESS;
}
