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
#include <vector>
#include <cctype>

#include "SDL.h"
#include "engine.h"
#include "system.h"

#define FATSIZENAME     15

struct aggfat_t
{
    u32  crc;
    u32  offset;
    u32  size;
};
            
int main(int argc, char **argv)
{
    if(argc != 3)
    {
	std::cout << argv[0] << " path_heroes2.agg extract_to_dir" << std::endl;

	return EXIT_SUCCESS;
    }

    StreamFile sf1, sf2;

    if(! sf1.open(argv[1], "rb"))
    {
	std::cout << "error open file: " << argv[1] << std::endl;
	return EXIT_SUCCESS;
    }

    System::MakeDirectory(argv[2]);

    const u32 size = sf1.size();
    int total = 0;
    int count_items = sf1.getLE16();

    StreamBuf fats = sf1.toStreamBuf(count_items * 4 * 3 /* crc, offset, size */);
    sf1.seek(size - FATSIZENAME * count_items);
    StreamBuf names = sf1.toStreamBuf(FATSIZENAME * count_items);

    std::map<std::string, aggfat_t> maps;

    for(int ii = 0; ii < count_items; ++ii)
    {
        aggfat_t & f = maps[StringLower(names.toString(FATSIZENAME))];

        f.crc = fats.getLE32();
        f.offset = fats.getLE32();
        f.size = fats.getLE32();
    }

    for(std::map<std::string, aggfat_t>::const_iterator
	it = maps.begin(); it != maps.end(); ++it)
    {
	const aggfat_t & fat = (*it).second;
	const std::string & fn = System::ConcatePath(argv[2], (*it).first);
	sf1.seek(fat.offset);
	std::vector<u8> buf = sf1.getRaw(fat.size);

	if(buf.size() && sf2.open(fn, "wb"))
	{
    	    sf2.putRaw(reinterpret_cast<char*>(& buf[0]), buf.size());
	    sf2.close();

	    ++total;
	    std::cout << "extract: " << fn << std::endl;
	}
    }

    sf1.close();
    std::cout << "total: " << total << std::endl;
    return EXIT_SUCCESS;
}
