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
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iomanip>

#include "engine.h"
#include "system.h"
#include "palette_h2.h"

struct icnheader
{
    s16	offsetX;
    s16 offsetY;
    u16 width;
    u16 height;
    u8  type; // type of sprite : 0 = Normal, 32 = Monochromatic shape
    u32 offsetData;

    icnheader() : offsetX(0), offsetY(0), width(0), height(0), type(0), offsetData(0) {}
};

StreamBase & operator>>(StreamBase & sb, icnheader & st)
{
    return sb >> st.offsetX >> st.offsetY >> st.width >> st.height >> st.type >> st.offsetData;
}

void SpriteDrawICNv1(Surface & sf, const std::vector<u8> &, bool debug);
void SpriteDrawICNv2(Surface & sf, const std::vector<u8> &, bool debug);

namespace H2Palette
{
    std::vector<SDL_Color> pal_colors;

    void Init(void)
    {
	// load palette
	u32 ncolors = ARRAY_COUNT(kb_pal) / 3;
	pal_colors.reserve(ncolors);

	for(u32 ii = 0; ii < ncolors; ++ii)
	{
    	    u32 index = ii * 3;
    	    SDL_Color cols;

    	    cols.r = kb_pal[index] << 2;
    	    cols.g = kb_pal[index + 1] << 2;
    	    cols.b = kb_pal[index + 2] << 2;

    	    pal_colors.push_back(cols);
	}

	Surface::SetDefaultPalette(&pal_colors[0], pal_colors.size());
    }

    RGBA GetColor(u32 index)
    {
	return index < pal_colors.size() ?
    	RGBA(pal_colors[index].r, pal_colors[index].g, pal_colors[index].b) : RGBA(0,0,0);
    }
}

int main(int argc, char **argv)
{
    if(argc < 3)
    {
	std::cout << argv[0] << " [-s (skip shadow)] [-d (debug on)] infile.icn extract_to_dir" << std::endl;
	return EXIT_SUCCESS;
    }

    bool debug = false;
    //bool shadow = true;

    char** ptr = argv;
    ++ptr;

    while(ptr && *ptr)
    {
	if(0 == strcmp("-d", *ptr))
	    debug = true;
	//else
	//if(0 == strcmp("-s", *ptr))
	//    shadow = false;
	else
	    break;

	++ptr;
    }
    
    std::string shortname(*ptr);
    ++ptr;
    std::string prefix(*ptr);

    StreamFile sf;

    if(! sf.open(shortname, "rb"))
    {
	std::cout << "error open file: " << shortname << std::endl;
	return EXIT_SUCCESS;
    }

    int count_sprite = sf.getLE16();
    int total_size = sf.getLE32();
    
    shortname.replace(shortname.find(".icn"), 4, "");
    prefix = System::ConcatePath(prefix, shortname);

    if(0 != System::MakeDirectory(prefix))
    {
	std::cout << "error mkdir: " << prefix << std::endl;
	return EXIT_SUCCESS;
    }

    // write file "spec.xml"
    std::string name_spec_file = System::ConcatePath(prefix, "spec.xml");
    std::fstream fs(name_spec_file.c_str(), std::ios::out);
    if(fs.fail())
    {
	std::cout << "error write file: " << name_spec_file << std::endl;
	return EXIT_SUCCESS;
    }

    SDL::Init();
    H2Palette::Init();

    fs << "<?xml version=\"1.0\" ?>" << std::endl <<
	"<icn name=\"" << shortname << ".icn\" count=\"" << count_sprite << "\">" << std::endl;

    u32 save_pos = sf.tell();

    std::vector<icnheader> headers(count_sprite);
    for(int ii = 0; ii < count_sprite; ++ii)
	sf >> headers[ii];

    for(int ii = 0; ii < count_sprite; ++ii)
    {
	const icnheader & head = headers[ii];

	u32 data_size = (ii + 1 != count_sprite ? headers[ii + 1].offsetData - head.offsetData : total_size - head.offsetData);
	sf.seek(save_pos + head.offsetData);
	std::cerr << data_size << std::endl;
        std::vector<u8> buf = sf.getRaw(data_size);

	if(buf.size())
	{
	    Surface surf(Size(head.width, head.height), /*false*/true); // accepting transparency

	    const RGBA clkey = RGBA(0xFF, 0, 0xFF);
    	    surf.Fill(clkey);
	    surf.SetColorKey(clkey);

	    //surf.Fill(0xff, 0xff, 0xff);
	    surf.Fill(ColorBlack); // filling with transparent color

	    if(0x20 == head.type)
		SpriteDrawICNv2(surf, buf, debug);
	    else
		SpriteDrawICNv1(surf, buf, debug);

	    std::ostringstream os;
	    os << std::setw(3) << std::setfill('0') << ii;

	    std::string dstfile = System::ConcatePath(prefix, os.str());
	    std::string shortdstfile(os.str()); // the name of destfile without the path

#ifndef WITH_IMAGE
	    dstfile += ".bmp";
	    shortdstfile += ".bmp";
#else
	    dstfile += ".png";
	    shortdstfile += ".png";
#endif
	    surf.Save(dstfile.c_str());
	    fs << " <sprite index=\"" << ii+1 << "\" name=\"" << shortdstfile.c_str() << "\" ox=\"" << head.offsetX << "\" oy=\"" << head.offsetY << "\"/>" << std::endl; 
	}
    }

    sf.close();
    fs << "</icn>" << std::endl;
    fs.close();
    std::cout << "expand to: " << prefix << std::endl;

    SDL::Quit();
    return EXIT_SUCCESS;
}

void SpriteDrawICNv1(Surface & sf, const std::vector<u8> & buf, bool debug)
{
    const u8* cur = & buf[0];
    const u32 size = buf.size();
    const u8 *max = cur + size;

    u8  c = 0;
    u16 x = 0;
    u16 y = 0;

    RGBA shadow = RGBA(0, 0, 0, 0x40);

    // lock surface
    while(1)
    {
	if(debug)
	    std::cerr << "CMD:" << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(*cur);

	// 0x00 - end line
	if(0 == *cur)
	{
	    ++y;
	    x = 0;
	    ++cur;
	}
	else
	// 0x7F - count data
	if(0x80 > *cur)
	{
	    c = *cur;
	    ++cur;
	    while(c-- && cur < max)
	    {
		sf.DrawPoint(Point(x, y), H2Palette::GetColor(*cur));
		++x;
		++cur;
	    }
	}
	else
	// 0x80 - end data
	if(0x80 == *cur)
	{
	    if(debug)
		std::cerr << std::endl;

	    break;
	}
	else
	// 0xBF - skip data
	if(0xC0 > *cur)
	{
	    x += *cur - 0x80;
	    ++cur;
	}
	else
	// 0xC0 - shadow
	if(0xC0 == *cur)
	{
	    ++cur;
	    c = (*cur % 4) ? *cur % 4 : *(++cur);

	    while(c--){ sf.DrawPoint(Point(x, y), shadow); ++x; }

	    ++cur;
	}
	else
	// 0xC1
	if(0xC1 == *cur)
	{
	    ++cur;
	    c = *cur;
	    ++cur;
	    while(c--){ sf.DrawPoint(Point(x, y), H2Palette::GetColor(*cur)); ++x; }
	    ++cur;
	}
	else
	{
	    c = *cur - 0xC0;
	    ++cur;
	    while(c--){ sf.DrawPoint(Point(x, y), H2Palette::GetColor(*cur)); ++x; }
	    ++cur;
	}

	if(cur >= max)
	{
	    std::cerr << "out of range" << std::endl;
	    break;
	}

	if(debug)
	    std::cerr << std::endl;
    }
}

void SpriteDrawICNv2(Surface & sf, const std::vector<u8> & buf, bool debug)
{
    const u8* cur = & buf[0];
    const u32 size = buf.size();
    const u8 *max = cur + size;

    u8  c = 0;
    u16 x = 0;
    u16 y = 0;

    RGBA shadow = RGBA(0, 0, 0, 0xff);

    // lock surface
    while(1)
    {
	if(debug)
	    std::cerr << "CMD:" << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(*cur);

	// 0x00 - end line
	if(0 == *cur)
	{
	    ++y;
	    x = 0;
	    ++cur;
	}
	else
	// 0x7F - count data
	if(0x80 > *cur)
	{
	    c = *cur;
	    while(c--)
	    {
		sf.DrawPoint(Point(x, y), shadow);
		++x;
	    }
	    ++cur;
	}
	else
	// 0x80 - end data
	if(0x80 == *cur)
	{
	    if(debug)
		std::cerr << std::endl;

	    break;
	}
	else
	// other - skip data
	{
	    x += *cur - 0x80;
	    ++cur;
	}

	if(cur >= max)
	{
	    std::cerr << "out of range" << std::endl;
	    break;
	}

	if(debug)
	    std::cerr << std::endl;
    }
}
