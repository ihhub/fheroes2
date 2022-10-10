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

#include <iostream>
#include <vector>

#include "agg_file.h"
#include "serialize.h"
#include "system_dir.h"

int main( int argc, char ** argv )
{
    if ( argc != 3 ) {
        std::cerr << argv[0] << ": wrong arguments" << std::endl
                  << "Usage: extractor <path_to_agg> <dir_to_extract>" << std::endl
                  << "Example: extractor heroes2.agg output_dir" << std::endl;
        return EXIT_FAILURE;
    }

    fheroes2::AGGFile agg;
    agg.open( argv[1] );
    if ( !agg.isGood() ) {
        std::cerr << "error open file: " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }

    System::MakeDirectory( argv[2] );

    int total = 0;
    for ( const std::string & name : agg.listFilenames() ) {
        StreamFile sf;
        sf.open( System::ConcatePath( argv[2], name ), "wb" );
        auto data = agg.read( name );
        sf.putRaw( reinterpret_cast<char *>( &data[0] ), data.size() );
        sf.close();
        std::cout << "extract: " << name << std::endl;
        ++total;
    }

    std::cout << "total: " << total << std::endl;
    return EXIT_SUCCESS;
}
