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

#include <iostream>
#include <fstream>

#include "engine.h"

int main(int argc, char **argv)
{
    if(argc != 3)
    {
	std::cout << argv[0] << " infile.82m outfile.wav" << std::endl;

	return EXIT_SUCCESS;
    }

    std::fstream fd_data(argv[1], std::ios::in | std::ios::binary);

    if(fd_data.fail())
    {
	std::cout << "error open file: " << argv[1] << std::endl;

	return EXIT_SUCCESS;
    }

    fd_data.seekg(0, std::ios_base::end);
    u32 size = fd_data.tellg();
    fd_data.seekg(0, std::ios_base::beg);
    char *body = new char[size];
    fd_data.read(body, size);
    fd_data.close();

    std::fstream fd_body(argv[2], std::ios::out | std::ios::binary);
    if(! fd_body.fail())
    {
        StreamBuf wavHeader(44);
        wavHeader.putLE32(0x46464952);          // RIFF
        wavHeader.putLE32(size + 0x24);         // size
        wavHeader.putLE32(0x45564157);          // WAVE
        wavHeader.putLE32(0x20746D66);          // FMT
        wavHeader.putLE32(0x10);                // size_t
        wavHeader.putLE16(0x01);                // format
        wavHeader.putLE16(0x01);                // channels
        wavHeader.putLE32(22050);               // samples
        wavHeader.putLE32(22050);               // byteper
        wavHeader.putLE16(0x01);                // align
        wavHeader.putLE16(0x08);                // bitsper
        wavHeader.putLE32(0x61746164);          // DATA
        wavHeader.putLE32(size);                // size

    	fd_body.write((const char*) wavHeader.data(), wavHeader.size());
	fd_body.close();
    }

    delete [] body;

    return EXIT_SUCCESS;
}
