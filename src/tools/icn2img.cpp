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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "agg_file.h"
#include "image_palette.h"
#include "image_tool.h"
#include "serialize.h"
#include "system_dir.h"

int main( int argc, char ** argv )
{
    if ( argc != 4 ) {
        std::cout << argv[0] << " inputFile.icn destinationDirectory paletteFileName.pal" << std::endl;
        return EXIT_FAILURE;
    }

    std::string inputFileName( argv[1] );
    std::string prefix( argv[2] );

    std::string paletteFileName( argv[3] );

    StreamFile paletteFile;
    if ( !paletteFile.open( paletteFileName, "rb" ) ) {
        std::cout << "Cannot open " << paletteFileName << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<uint8_t> palette = paletteFile.getRaw();
    if ( palette.size() != 768 ) {
        std::cout << "Invalid palette size of " << palette.size() << " instead of 768." << std::endl;
        return EXIT_FAILURE;
    }

    fheroes2::setGamePalette( palette );

    StreamFile sf;

    if ( !sf.open( inputFileName, "rb" ) ) {
        std::cout << "Cannot open " << inputFileName << std::endl;
        return EXIT_FAILURE;
    }

    int count_sprite = sf.getLE16();
    int total_size = sf.getLE32();

    inputFileName.replace( inputFileName.find( ".icn" ), 4, "" );
    prefix = System::ConcatePath( prefix, inputFileName );

    if ( 0 != System::MakeDirectory( prefix ) ) {
        std::cout << "error mkdir: " << prefix << std::endl;
        return EXIT_SUCCESS;
    }

    size_t save_pos = sf.tell();

    std::vector<fheroes2::ICNHeader> headers( count_sprite );
    for ( int ii = 0; ii < count_sprite; ++ii )
        sf >> headers[ii];

    for ( int ii = 0; ii < count_sprite; ++ii ) {
        const fheroes2::ICNHeader & head = headers[ii];

        uint32_t data_size = ( ii + 1 != count_sprite ? headers[ii + 1].offsetData - head.offsetData : total_size - head.offsetData );
        sf.seek( save_pos + head.offsetData );
        std::cerr << data_size << std::endl;
        std::vector<uint8_t> buf = sf.getRaw( data_size );

        if ( !buf.empty() ) {
            const fheroes2::Sprite image = fheroes2::decodeICNSprite( &buf[0], data_size, head.width, head.height, head.offsetX, head.offsetY );

            std::ostringstream os;
            os << std::setw( 3 ) << std::setfill( '0' ) << ii;

            std::string dstfile = System::ConcatePath( prefix, os.str() );

            if ( fheroes2::isPNGFormatSupported() ) {
                dstfile += ".png";
            }
            else {
                dstfile += ".bmp";
            }

            std::cout << "Image " << ii + 1 << " has offset of [" << static_cast<int32_t>( head.offsetX ) << ", " << static_cast<int32_t>( head.offsetY ) << "]"
                      << std::endl;

            fheroes2::Save( image, dstfile, 23 );
        }
    }

    sf.close();

    return EXIT_SUCCESS;
}
