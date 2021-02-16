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

#include <fstream>
#include <iomanip>
#include <iostream>

#include "agg_file.h"
#include "image_tool.h"
#include "serialize.h"
#include "system.h"

int main( int argc, char ** argv )
{
    if ( argc < 3 ) {
        std::cout << argv[0] << " [-s (skip shadow)] [-d (debug on)] infile.icn extract_to_dir" << std::endl;
        return EXIT_SUCCESS;
    }

    bool debug = false;
    // bool shadow = true;

    char ** ptr = argv;
    ++ptr;

    while ( ptr && *ptr ) {
        if ( 0 == strcmp( "-d", *ptr ) )
            debug = true;
        // else
        // if(0 == strcmp("-s", *ptr))
        //    shadow = false;
        else
            break;

        ++ptr;
    }

    std::string shortname( *ptr );
    ++ptr;
    std::string prefix( *ptr );

    StreamFile sf;

    if ( !sf.open( shortname, "rb" ) ) {
        std::cout << "error open file: " << shortname << std::endl;
        return EXIT_SUCCESS;
    }

    int count_sprite = sf.getLE16();
    int total_size = sf.getLE32();

    shortname.replace( shortname.find( ".icn" ), 4, "" );
    prefix = System::ConcatePath( prefix, shortname );

    if ( 0 != System::MakeDirectory( prefix ) ) {
        std::cout << "error mkdir: " << prefix << std::endl;
        return EXIT_SUCCESS;
    }

    // write file "spec.xml"
    std::string name_spec_file = System::ConcatePath( prefix, "spec.xml" );
    std::fstream fs( name_spec_file.c_str(), std::ios::out );
    if ( fs.fail() ) {
        std::cout << "error write file: " << name_spec_file << std::endl;
        return EXIT_SUCCESS;
    }

    fs << "<?xml version=\"1.0\" ?>" << std::endl << "<icn name=\"" << shortname << ".icn\" count=\"" << count_sprite << "\">" << std::endl;

    u32 save_pos = sf.tell();

    std::vector<fheroes2::ICNHeader> headers( count_sprite );
    for ( int ii = 0; ii < count_sprite; ++ii )
        sf >> headers[ii];

    for ( int ii = 0; ii < count_sprite; ++ii ) {
        const fheroes2::ICNHeader & head = headers[ii];

        u32 data_size = ( ii + 1 != count_sprite ? headers[ii + 1].offsetData - head.offsetData : total_size - head.offsetData );
        sf.seek( save_pos + head.offsetData );
        std::cerr << data_size << std::endl;
        std::vector<u8> buf = sf.getRaw( data_size );

        if ( buf.size() ) {
            const fheroes2::Sprite image = fheroes2::decodeICNSprite( &buf[0], data_size, head.width, head.height, head.offsetX, head.offsetY );

            std::ostringstream os;
            os << std::setw( 3 ) << std::setfill( '0' ) << ii;

            std::string dstfile = System::ConcatePath( prefix, os.str() );
            std::string shortdstfile( os.str() ); // the name of destfile without the path

#ifndef WITH_IMAGE
            dstfile += ".bmp";
            shortdstfile += ".bmp";
#else
            dstfile += ".png";
            shortdstfile += ".png";
#endif
            if ( fheroes2::Save( image, dstfile, 0 ) ) {
                fs << " <sprite index=\"" << ii + 1 << "\" name=\"" << shortdstfile.c_str() << "\" ox=\"" << head.offsetX << "\" oy=\"" << head.offsetY << "\"/>"
                   << std::endl;
            }
        }
    }

    sf.close();
    fs << "</icn>" << std::endl;
    fs.close();
    std::cout << "expand to: " << prefix << std::endl;

    return EXIT_SUCCESS;
}
