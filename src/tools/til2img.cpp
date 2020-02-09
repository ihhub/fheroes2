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
#include <sstream>

#include <SDL.h>

#include "engine.h"
#include "palette_h2.h"
#include "system.h"

namespace H2Palette
{
    std::vector<SDL_Color> pal_colors;

    void Init( void )
    {
        // load palette
        u32 ncolors = ARRAY_COUNT( kb_pal ) / 3;
        pal_colors.reserve( ncolors );

        for ( u32 ii = 0; ii < ncolors; ++ii ) {
            u32 index = ii * 3;
            SDL_Color cols;

            cols.r = kb_pal[index] << 2;
            cols.g = kb_pal[index + 1] << 2;
            cols.b = kb_pal[index + 2] << 2;

            pal_colors.push_back( cols );
        }

        Surface::SetDefaultPalette( &pal_colors[0], pal_colors.size() );
    }

    RGBA GetColor( u32 index )
    {
        return index < pal_colors.size() ? RGBA( pal_colors[index].r, pal_colors[index].g, pal_colors[index].b ) : RGBA( 0, 0, 0 );
    }
}

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

    if ( shortname == "-d" ) {
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
    std::vector<u8> buf = sf.getRaw( size );

    SDL::Init();
    H2Palette::Init();

    for ( int cur = 0; cur < count; ++cur ) {
        u32 offset = width * height * cur;
        if ( offset < buf.size() ) {
            Surface sf( &buf[offset], width, height, 1, false );

            std::ostringstream stream;
            stream << std::setw( 3 ) << std::setfill( '0' ) << cur;
            std::string dstfile = System::ConcatePath( prefix, stream.str() );

#ifndef WITH_IMAGE
            dstfile += ".bmp";
#else
            dstfile += ".png";
#endif
            if ( !sf.Save( dstfile.c_str() ) )
                std::cout << "error" << std::endl;
        }
    }

    sf.close();
    std::cout << "expand to: " << prefix << std::endl;
    SDL::Quit();
    return EXIT_SUCCESS;
}
