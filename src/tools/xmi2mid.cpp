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
#include "engine.h"

int main(int argc, char** argv)
{
    if(argc != 3)
    {
	std::cout << argv[0] << " infile.xmi outfile.mid" << std::endl;
	return EXIT_SUCCESS;
    }

    std::vector<u8> buf = LoadFileToMem(argv[1]);

    if(buf.size())
    {
	buf = Music::Xmi2Mid(buf);

	if(buf.empty())
    	    std::cerr << ", file: " << argv[1] << std::endl;
	else
    	    SaveMemToFile(buf, std::string(argv[2]));
    }

    return 0;
}
