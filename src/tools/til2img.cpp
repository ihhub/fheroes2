/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include <iomanip>
#include <iostream>

#include "image_tool.h"
#include "serialize.h"
#include "system.h"

int main( int argc, char ** argv )
{
    if ( argc != 3 ) {
        std::cout << argv[0] << " [-d] infile.til extract_to_dir" << std::endl;
        return EXIT_SUCCESS;
    }

    StreamFile sf;

    if ( !sf.open( argv[1], "rb" ) ) {
        std::cout << "error open file: " << argv[1] << std::endl;
        return EXIT_SUCCESS;
    }

    std::string prefix( argv[2] );
    std::string shortname( argv[1] );

    bool debugMode = false;
    if ( shortname == "-d" ) {
        debugMode = true;
    }

    shortname.replace( shortname.find( "." ), 4, "" );
    prefix = System::ConcatePath( prefix, shortname );

    if ( 0 != System::MakeDirectory( prefix ) ) {
        std::cout << "error mkdir: " << prefix << std::endl;
        return EXIT_SUCCESS;
    }

    int size = sf.size();
    int count = sf.getLE16();
    int width = sf.getLE16();
    int height = sf.getLE16();
    std::vector<uint8_t> buf = sf.getRaw( width * height * count );
    if ( debugMode ) {
        std::cout << "Size of stream " << size << "(" << buf.size() << ")" << std::endl;
        std::cout << "Count of images " << count << "(" << width << "," << height << ")" << std::endl;
    }

    for ( int cur = 0; cur < count; ++cur ) {
        uint32_t offset = width * height * cur;
        if ( offset < buf.size() ) {
            fheroes2::Image image( width, height );
            memcpy( image.image(), &buf[offset], static_cast<size_t>( width * height ) );
            std::fill( image.transform(), image.transform() + width * height, 0 );

            std::ostringstream stream;
            stream << std::setw( 3 ) << std::setfill( '0' ) << cur;
            std::string dstfile = System::ConcatePath( prefix, stream.str() );

#ifndef WITH_IMAGE
            dstfile += ".bmp";
#else
            dstfile += ".png";
#endif
            if ( debugMode ) {
                std::cout << "Saving " << dstfile << std::endl;
            }

            if ( !fheroes2::Save( image, dstfile, 0 ) )
                std::cout << "error" << std::endl;
        }
    }

    sf.close();
    std::cout << "expand to: " << prefix << std::endl;
    return EXIT_SUCCESS;
}
