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

#include "font.h"
#include "engine.h"
#include "surface.h"

#ifdef WITH_TTF

FontTTF::FontTTF() : ptr(NULL)
{
}

FontTTF::~FontTTF()
{
    if(ptr) TTF_CloseFont(ptr);
}

void FontTTF::Init(void)
{
    if(0 != TTF_Init()) ERROR(SDL_GetError());
}

void FontTTF::Quit(void)
{
    TTF_Quit();
}

bool FontTTF::isValid(void) const
{
    return ptr;
}

bool FontTTF::Open(const std::string & filename, int size)
{
    if(ptr) TTF_CloseFont(ptr);
    ptr = TTF_OpenFont(filename.c_str(), size);
    if(!ptr) ERROR(SDL_GetError());
    return ptr;
}

void FontTTF::SetStyle(int style)
{
    TTF_SetFontStyle(ptr, style);
}

int FontTTF::Height(void) const
{
    return TTF_FontHeight(ptr);
}

int FontTTF::Ascent(void) const
{
    return TTF_FontAscent(ptr);
}

int FontTTF::Descent(void) const
{
    return TTF_FontDescent(ptr);
}

int FontTTF::LineSkip(void) const
{
    return TTF_FontLineSkip(ptr);
}

Surface FontTTF::RenderText(const std::string & msg, const RGBA & clr, bool solid)
{
    return Surface(solid ? TTF_RenderUTF8_Solid(ptr, msg.c_str(), clr()) :
                        TTF_RenderUTF8_Blended(ptr, msg.c_str(), clr()));
}
    
Surface FontTTF::RenderChar(char ch, const RGBA & clr, bool solid)
{
    char buf[2] = { '\0', '\0' };
         buf[0] = ch;
            
    return Surface(solid ? TTF_RenderUTF8_Solid(ptr, buf, clr()) :
                        TTF_RenderUTF8_Blended(ptr, buf, clr()));
}

Surface FontTTF::RenderUnicodeText(const std::vector<u16> & msg, const RGBA & clr, bool solid)
{
    return Surface(solid ? TTF_RenderUNICODE_Solid(ptr, &msg[0], clr()) :
                        TTF_RenderUNICODE_Blended(ptr, &msg[0], clr()));
}

Surface FontTTF::RenderUnicodeChar(u16 ch, const RGBA & clr, bool solid)
{
    u16 buf[2] = { L'\0', L'\0' };
        buf[0] = ch;

    return Surface(solid ? TTF_RenderUNICODE_Solid(ptr, buf, clr()) :
                        TTF_RenderUNICODE_Blended(ptr, buf, clr()));
}

#endif

FontPSF::FontPSF(const std::string & fn, const Size & sz) : size(sz)
{
    buf = LoadFileToMem(fn);
    if(buf.empty())
	ERROR("empty buffer");
}

Surface FontPSF::RenderText(const std::string & msg, const RGBA & color) const
{
    Surface res;

    res.Set(msg.size() * size.w, size.h, false);
    int posx = 0;

    for(std::string::const_iterator
	it = msg.begin(); it != msg.end(); ++it)
    {
	// render char
	u32 offsetx = *it * size.w * size.h / 8; // bits -> byte

	for(u32 yy = 0; yy < size.h; ++yy)
	{
	    u32 offsety = yy * size.w / 8; // bits -> byte

	    if(offsetx + offsety < buf.size())
	    {
		int line = buf[offsetx + offsety];
    		for(u32 xx = 0; xx < size.w; ++xx)
    		{
        	    if(0x80 & (line << xx))
            		res.DrawPoint(Point(posx + xx, yy), color);
    		}
	    }
	}

	posx += size.w;
    }

    return res;
}
