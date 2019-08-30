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

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <memory>
#include "surface.h"
#include "error.h"
#include "system.h"
#include "localevent.h"
#include "display.h"

#ifdef WITH_IMAGE
#include "SDL_image.h"
#include "IMG_savepng.h"
#endif

namespace
{
    u32 default_depth = 16;
    RGBA default_color_key;
    SDL_Color* pal_colors = NULL;
    u32 pal_nums = 0;
}

SurfaceFormat GetRGBAMask(u32 bpp)
{
    SurfaceFormat fm;
    fm.depth = bpp;
    fm.ckey = default_color_key;

    switch(bpp)
    {
#if SDL_VERSION_ATLEAST(2, 0, 0)
        case 32:
 #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            fm.rmask = 0xff000000;
            fm.gmask = 0x00ff0000;
            fm.bmask = 0x0000ff00;
            fm.amask = 0x000000ff;

 #else
            fm.rmask = 0x000000ff;
            fm.gmask = 0x0000ff00;
            fm.bmask = 0x00ff0000;
            fm.amask = 0xff000000;
 #endif
            break;

        case 24:
 #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            fm.rmask = 0x00ff0000;
            fm.gmask = 0x0000ff00;
            fm.bmask = 0x000000ff;
            fm.amask = 0x00000000;
 #else
            fm.rmask = 0x000000ff;
            fm.gmask = 0x0000ff00;
            fm.bmask = 0x00ff0000;
            fm.amask = 0x00000000;
 #endif
            break;

        case 16:
 #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            fm.rmask = 0x00007c00;
            fm.gmask = 0x000003e0;
            fm.bmask = 0x0000001f;
            fm.amask = 0x00000000;
 #else
            fm.rmask = 0x0000001f;
            fm.gmask = 0x000003e0;
            fm.bmask = 0x00007c00;
            fm.amask = 0x00000000;
 #endif
            break;
#else
	case 32:
 #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            fm.rmask = 0xff000000;
            fm.gmask = 0x00ff0000;
            fm.bmask = 0x0000ff00;
            fm.amask = 0x000000ff;
 #else
            fm.rmask = 0x000000ff;
            fm.gmask = 0x0000ff00;
            fm.bmask = 0x00ff0000;
            fm.amask = 0xff000000;
 #endif
	    break;

	case 24:
 #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            fm.rmask = 0x00fc0000;
            fm.gmask = 0x0003f000;
            fm.bmask = 0x00000fc0;
            fm.amask = 0x0000003f;
 #else
            fm.rmask = 0x0000003f;
            fm.gmask = 0x00000fc0;
            fm.bmask = 0x0003f000;
            fm.amask = 0x00fc0000;
 #endif
	    break;

	case 16:
 #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            fm.rmask = 0x0000f000;
            fm.gmask = 0x00000f00;
            fm.bmask = 0x000000f0;
            fm.amask = 0x0000000f;
 #else
            fm.rmask = 0x0000000f;
            fm.gmask = 0x000000f0;
            fm.bmask = 0x00000f00;
            fm.amask = 0x0000f000;
 #endif
	    break;
#endif

	default:
	    fm.rmask = 0;
	    fm.gmask = 0;
	    fm.bmask = 0;
	    fm.amask = 0;
	    break;
    }
    return fm;
}

u32 GetPixel24(u8* ptr)
{
    u32 color = 0;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    color |= ptr[0];
    color <<= 8;
    color |= ptr[1];
    color <<= 8;
    color |= ptr[2];
#else
    color |= ptr[2];
    color <<= 8;
    color |= ptr[1];
    color <<= 8;
    color |= ptr[0];
#endif
    return color;                                                                                                
}                                                                                                                

void SetPixel24(u8* ptr, u32 color)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    ptr[2] = color;
    ptr[1] = color >> 8;
    ptr[0] = color >> 16;
#else
    ptr[0] = color;
    ptr[1] = color >> 8;
    ptr[2] = color >> 16;
#endif
}

RGBA::RGBA()
{
    color.r = 0;
    color.g = 0;
    color.b = 0;
#if SDL_VERSION_ATLEAST(2, 0, 0)
    color.a = 255;
#else
    color.unused = 255;
#endif
}

RGBA::RGBA(int r, int g, int b, int a)
{
    color.r = r;
    color.g = g;
    color.b = b;
#if SDL_VERSION_ATLEAST(2, 0, 0)
    color.a = a;
#else
    color.unused = a;
#endif
}

int RGBA::r(void) const
{
    return color.r;
}

int RGBA::g(void) const
{
    return color.g;
}

int RGBA::b(void) const
{
    return color.b;
}

int RGBA::a(void) const
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    return color.a;
#else
    return color.unused;
#endif
}

int RGBA::pack(void) const
{
    return (((r() << 24) & 0xFF000000) |
	    ((g() << 16) & 0x00FF0000) |
	    ((b() << 8) & 0x0000FF00) |
	    (a() & 0x000000FF));
}

RGBA RGBA::unpack(int v)
{
    int r = (v >> 24) & 0x000000FF;
    int g = (v >> 16) & 0x000000FF;
    int b = (v >> 8) & 0x000000FF;
    int a = v & 0x000000FF;

    return RGBA(r, g, b, a);
}

Surface::Surface() : surface(NULL)
{
}

Surface::Surface(const Size & sz, bool amask) : surface(NULL)
{
    Set(sz.w, sz.h, amask);
}

Surface::Surface(const Size & sz, const SurfaceFormat & fm) : surface(NULL)
{
    Set(sz.w, sz.h, fm);
}

Surface::Surface(const Surface & bs) : surface(NULL)
{
    Set(bs, true);
}

Surface::Surface(const std::string & file) : surface(NULL)
{
    Load(file);
}

Surface::Surface(SDL_Surface* sf) : surface(sf)
{
}

Surface::Surface(const void* pixels, u32 width, u32 height, u32 bytes_per_pixel /* 1, 2, 3, 4 */, bool amask) : surface(NULL)
{
    SurfaceFormat fm = GetRGBAMask(8 * bytes_per_pixel);

    if(8 == fm.depth)
    {
#if SDL_VERSION_ATLEAST(2, 0, 0)
	surface = SDL_CreateRGBSurface(0, width, height, fm.depth, fm.rmask, fm.gmask, fm.bmask, (amask ? fm.amask : 0));
#else
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, fm.depth, fm.rmask, fm.gmask, fm.bmask, (amask ? fm.amask : 0));
#endif
    }
    else
    {
	surface = SDL_CreateRGBSurfaceFrom(const_cast<void *>(pixels), width, height, fm.depth, width * bytes_per_pixel,
		fm.rmask, fm.gmask, fm.bmask, amask ? fm.amask : 0);
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
#endif
    }

    if(!surface)
	Error::Except(__FUNCTION__, SDL_GetError());

    if(8 == fm.depth)
    {
	SetPalette();
	Lock();
	std::memcpy(surface->pixels, pixels, width * height);
	Unlock();
    }
}

Surface::~Surface()
{
    if(! isDisplay()) FreeSurface(*this);
}

/* operator = */
Surface & Surface::operator= (const Surface & bs)
{
    Set(bs, true);
    return *this;
}

bool Surface::operator== (const Surface & bs) const
{
    return surface && bs.surface ? surface == bs.surface : false;
}

void Surface::Reset(void)
{
    FreeSurface(*this);
    surface = NULL; /* hard set: for ref copy */
}

void Surface::Set(SDL_Surface* sf)
{
    FreeSurface(*this);
    surface = sf;
}

void Surface::Set(const Surface & bs, bool refcopy)
{
    FreeSurface(*this);

    if(bs.isValid())
    {
	if(refcopy)
	{
	    surface = bs.surface;
	    if(surface) surface->refcount += 1;
	}
	else
	{
	    surface = SDL_ConvertSurface(bs.surface, bs.surface->format, bs.surface->flags);

	    if(!surface)
		Error::Except(__FUNCTION__, SDL_GetError());
	}
    }
}

void Surface::Set(u32 sw, u32 sh, bool amask)
{
    Set(sw, sh, default_depth, amask);
}

void Surface::Set(u32 sw, u32 sh, u32 bpp /* bpp: 8, 16, 24, 32 */, bool amask)
{
    if(bpp == 8)
	bpp = 32;

    SurfaceFormat fm = GetRGBAMask(bpp);

    if(8 == fm.depth || ! amask) fm.amask = 0;
    Set(sw, sh, fm);
}

void Surface::Set(u32 sw, u32 sh, const SurfaceFormat & fm)
{
    FreeSurface(*this);

#if SDL_VERSION_ATLEAST(2, 0, 0)
    surface = SDL_CreateRGBSurface(0, sw, sh, fm.depth, fm.rmask, fm.gmask, fm.bmask, fm.amask);
#else
    surface = SDL_CreateRGBSurface(SDL_SWSURFACE, sw, sh, fm.depth, fm.rmask, fm.gmask, fm.bmask, fm.amask);
#endif

    if(!surface)
	Error::Except(__FUNCTION__, SDL_GetError());

    if(8 == depth())
    {
	SetPalette();
	Fill(fm.ckey);
	SetColorKey(fm.ckey);
    }
    else
    if(amask())
    {
#if SDL_VERSION_ATLEAST(2, 0, 0)
	Fill(RGBA(0, 0, 0, 0)); // no color key only amask
#else
	Fill(RGBA(fm.ckey.r(), fm.ckey.g(), fm.ckey.b(), 0));
	SetColorKey(fm.ckey);
#endif
    }
    else
    if(fm.ckey.pack())
    {
	Fill(fm.ckey);
	SetColorKey(fm.ckey);
    }

    if(amask())
    {
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
#else
	SDL_SetAlpha(surface, SDL_SRCALPHA, 255);
#endif
    }
    else
    {
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
#else
	SDL_SetAlpha(surface, 0, 0);
#endif
    }
}

void Surface::SetDefaultPalette(SDL_Color* ptr, int num)
{
    pal_colors = ptr;
    pal_nums = num;
}

void Surface::SetDefaultColorKey(int r, int g, int b)
{
    default_color_key = RGBA(r, g, b);
}

void Surface::SetDefaultDepth(u32 depth)
{
    switch(depth)
    {
	case 24:
	    ERROR("switch to 32 bpp colors");
	    default_depth = 32;
	    break;

	case 8:
	case 15:
	case 16:
#if SDL_VERSION_ATLEAST(2, 0, 0)
	    ERROR("switch to 32 bpp colors");
	    default_depth = 32;
#else
	    default_depth = depth;
#endif
	    break;

	case 32:
	    default_depth = depth;
	    break;

	default:
	    break;
    }
}

Size Surface::GetSize(void) const
{
    return Size(w(), h());
}

bool Surface::isValid(void) const
{
    return surface && surface->format;
}

bool Surface::Load(const std::string & fn)
{
    FreeSurface(*this);

#ifdef WITH_IMAGE
    surface = IMG_Load(fn.c_str());
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
#endif
#else
    surface = SDL_LoadBMP(fn.c_str());
#endif

    if(!surface)
	ERROR(SDL_GetError());

    return surface;
}

bool Surface::Save(const std::string & fn) const
{
    int res = 0;

#ifdef WITH_IMAGE
#if SDL_VERSION_ATLEAST(2, 0, 0)
    res = IMG_SavePNG(surface, fn.c_str());
#else
    res = IMG_SavePNG(fn.c_str(), surface, -1);
#endif
#else
    res = SDL_SaveBMP(surface, fn.c_str());
#endif

    if(0 != res)
    {
	ERROR(SDL_GetError());
	return false;
    }

    return true;
}

int Surface::w(void) const
{
    return surface ? surface->w : 0;
}

int Surface::h(void) const
{
    return surface ? surface->h : 0;
}

u32 Surface::depth(void) const
{
    return isValid() ? surface->format->BitsPerPixel : 0;
}

u32 Surface::amask(void) const
{
    return isValid() ? surface->format->Amask : 0;
}

u32 Surface::alpha(void) const
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if(isValid())
    {
        u8 alpha = 0;
        SDL_GetSurfaceAlphaMod(surface, &alpha);
        return alpha;
    }
    return 0;
#else
    return isValid() ? surface->format->alpha : 0;
#endif
}

SurfaceFormat Surface::GetFormat(void) const
{
    SurfaceFormat res;
    if(surface->format)
    {
	res.depth = surface->format->BitsPerPixel;
	res.rmask = surface->format->Rmask;
	res.gmask = surface->format->Gmask;
	res.bmask = surface->format->Bmask;
	res.amask = surface->format->Amask;
	res.ckey = default_color_key;
    }
    return res;
}

u32 Surface::MapRGB(const RGBA & color) const
{
    return amask() ? SDL_MapRGBA(surface->format, color.r(), color.g(), color.b(), color.a()) : SDL_MapRGB(surface->format, color.r(), color.g(), color.b());
}

RGBA Surface::GetRGB(u32 pixel) const
{
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;
    u8 a = 0;

    if(amask())
    {
	SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
	return RGBA(r, g, b, a);
    }

    SDL_GetRGB(pixel, surface->format, &r, &g, &b);
    return RGBA(r, g, b);
}

/* load static palette (economize 1kb for each surface) only 8bit color! */
void Surface::SetPalette(void)
{
    if(isValid() &&
	pal_colors && pal_nums && surface->format->palette)
    {
	if(surface->format->palette->colors &&
	    pal_colors != surface->format->palette->colors) SDL_free(surface->format->palette->colors);

        surface->format->palette->colors = pal_colors;
        surface->format->palette->ncolors = pal_nums;
    }
}

u32 Surface::GetColorKey(void) const
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if(isValid() && ! amask())
    {
        u32 res = 0;
        SDL_GetColorKey(surface, &res);
        return res;
    }
    return 0;
#else
    return isValid() && (surface->flags & SDL_SRCCOLORKEY) ? surface->format->colorkey : 0;
#endif
}

void Surface::SetColorKey(const RGBA & color)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_SetColorKey(surface, SDL_TRUE, MapRGB(color));
#else
    SDL_SetColorKey(surface, SDL_SRCCOLORKEY, MapRGB(color));
#endif
}

/* draw u32 pixel */
void Surface::SetPixel4(s32 x, s32 y, u32 color)
{
    u32* bufp = static_cast<u32 *>(surface->pixels) + y * (surface->pitch >> 2) + x;
    *bufp = color;
}

/* draw u24 pixel */
void Surface::SetPixel3(s32 x, s32 y, u32 color)
{
    u8* bufp = static_cast<u8 *>(surface->pixels) + y * surface->pitch + x * 3; 
    SetPixel24(bufp, color);
}

/* draw u16 pixel */
void Surface::SetPixel2(s32 x, s32 y, u32 color)
{
    u16* bufp = static_cast<u16 *>(surface->pixels) + y * (surface->pitch >> 1) + x;
    *bufp = static_cast<u16>(color);
}

/* draw u8 pixel */
void Surface::SetPixel1(s32 x, s32 y, u32 color)
{
    u8* bufp = static_cast<u8 *>(surface->pixels) + y * surface->pitch + x;
    *bufp = static_cast<u8>(color);
}

/* draw pixel */
void Surface::SetPixel(int x, int y, u32 pixel)
{
    if(x < w() && y < h())
    {
	switch(depth())
	{
	    case 8:  SetPixel1(x, y, pixel); break;
	    case 15:
	    case 16: SetPixel2(x, y, pixel); break;
	    case 24: SetPixel3(x, y, pixel); break;
	    case 32: SetPixel4(x, y, pixel); break;
	    default: break;
	}
    }
    else
    {
	std::ostringstream os;
	os << "out of range: " << "x: " << x << ", " << "y: " << y << ", " << "width: " << w() << ", " << "height: " << h();
	Error::Except(__FUNCTION__, os.str().c_str());
    }
}

u32 Surface::GetPixel4(s32 x, s32 y) const
{
    u32 *bufp = static_cast<u32 *>(surface->pixels) + y * (surface->pitch >> 2) + x;
    return *bufp;
}

u32 Surface::GetPixel3(s32 x, s32 y) const
{
    u8 *bufp = static_cast<u8 *>(surface->pixels) + y * surface->pitch + x * 3; 
    return GetPixel24(bufp);
}

u32 Surface::GetPixel2(s32 x, s32 y) const
{
    u16* bufp = static_cast<u16 *>(surface->pixels) + y * (surface->pitch >> 1) + x;
    return static_cast<u32>(*bufp);
}

u32 Surface::GetPixel1(s32 x, s32 y) const
{
    u8* bufp = static_cast<u8 *>(surface->pixels) + y * surface->pitch + x;
    return static_cast<u32>(*bufp);
}

u32 Surface::GetPixel(int x, int y) const
{
    u32 pixel = 0;

    if(x < w() && y < h())
    {
	switch(depth())
	{
	    case 8:  pixel = GetPixel1(x, y); break;
	    case 15:
	    case 16: pixel = GetPixel2(x, y); break;
	    case 24: pixel = GetPixel3(x, y); break;
	    case 32: pixel = GetPixel4(x, y); break;
	    default: break;
	}
    }
    else
	Error::Except(__FUNCTION__, "out of range");
    
    return pixel;
}

void Surface::Blit(const Rect & srt, const Point & dpt, Surface & dst) const
{
    SDL_Rect dstrect = SDLRect(dpt.x, dpt.y, srt.w, srt.h);
    SDL_Rect srcrect = SDLRect(srt);

#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_BlitSurface(surface, & srcrect, dst.surface, & dstrect);
#else
    if(! dst.isDisplay() &&
	amask() && dst.amask())
    {
    	SDL_SetAlpha(surface, 0, 0);
	SDL_BlitSurface(surface, & srcrect, dst.surface, & dstrect);
	SDL_SetAlpha(surface, SDL_SRCALPHA, 255);
    }
    else
	SDL_BlitSurface(surface, & srcrect, dst.surface, & dstrect);
#endif
}

void Surface::Blit(Surface & dst) const
{
    Blit(Rect(Point(0, 0), GetSize()), Point(0, 0), dst);
}

void Surface::Blit(s32 dx, s32 dy, Surface & dst) const
{
    Blit(Point(dx, dy), dst);
}

void Surface::Blit(const Rect &srt, s32 dx, s32 dy, Surface & dst) const
{
    Blit(srt, Point(dx, dy), dst);
}

void Surface::Blit(const Point & dpt, Surface & dst) const
{
    Blit(Rect(Point(0, 0), GetSize()), dpt, dst);
}

void Surface::SetAlphaMod(int level)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if(isValid())
        SDL_SetSurfaceAlphaMod(surface, level);
#else
    if(isValid())
    {
	if(amask())
	{
	    Surface res(GetSize(), false);
    	    SDL_SetAlpha(surface, 0, 0);
    	    Blit(res);
    	    SDL_SetAlpha(surface, SDL_SRCALPHA, 255);
	    Set(res, true);
	}

	SDL_SetAlpha(surface, SDL_SRCALPHA, level);
    }
#endif
}

void Surface::Lock(void) const
{
    if(SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);
}

void Surface::Unlock(void) const
{
    if(SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
}

bool Surface::isRefCopy(void) const
{
    return surface ? 1 < surface->refcount : false;
}

void Surface::FreeSurface(Surface & sf)
{
    if(sf.surface)
    {
	if(sf.isRefCopy())
	{
	    --sf.surface->refcount;
	    sf.surface = NULL;
	}
	else
	{
    	    // clear static palette
    	    if(sf.surface->format && 8 == sf.surface->format->BitsPerPixel && pal_colors && pal_nums &&
        	sf.surface->format->palette && pal_colors == sf.surface->format->palette->colors)
    	    {
        	sf.surface->format->palette->colors = NULL;
        	sf.surface->format->palette->ncolors = 0;
    	    }

	    SDL_FreeSurface(sf.surface);
	    sf.surface = NULL;
	}
    }
}

u32 Surface::GetMemoryUsage(void) const
{
    u32 res = sizeof(surface);

    if(surface)
    {
	res += sizeof(SDL_Surface) + sizeof(SDL_PixelFormat) + surface->pitch * surface->h;

	if(surface->format && surface->format->palette &&
	    (! pal_colors || pal_colors != surface->format->palette->colors))
	    res += sizeof(SDL_Palette) + surface->format->palette->ncolors * sizeof(SDL_Color);
    }

    return res;
}

std::string Surface::Info(void) const
{
    std::ostringstream os;

    if(isValid())
    {
	os << 
#if SDL_VERSION_ATLEAST(2, 0, 0)
	    "flags" << "(" << surface->flags << "), " <<
#else
	    "flags" << "(" << surface->flags << ", " << (surface->flags & SDL_SRCALPHA ? "SRCALPHA" : "") << (surface->flags & SDL_SRCCOLORKEY ? "SRCCOLORKEY" : "") << "), " <<
#endif
	    "w"<< "(" << surface->w << "), " <<
	    "h"<< "(" << surface->h << "), " <<
	    "size" << "(" << GetMemoryUsage() << "), " <<
	    "bpp" << "(" << depth() << "), " <<
#if SDL_VERSION_ATLEAST(2, 0, 0)
#else
	    "Amask" << "(" << "0x" << std::setw(8) << std::setfill('0') << std::hex << surface->format->Amask << "), " <<
	    "colorkey" << "(" << "0x" << std::setw(8) << std::setfill('0') << surface->format->colorkey << "), " << std::dec <<
#endif
	    "alpha" << "(" << alpha() << "), ";
    }
    else
	os << "invalid surface";

    return os.str();
}

void Surface::Swap(Surface & sf1, Surface & sf2)
{
    std::swap(sf1.surface, sf2.surface);
}

bool Surface::isDisplay(void) const
{
    return false;
}

Surface Surface::RenderScale(const Size & size) const
{
    Surface res(size, GetFormat());

    if(size.w >= 2 && size.h >= 2)
    {
        float stretch_factor_x = size.w / static_cast<float>(w());
        float stretch_factor_y = size.h / static_cast<float>(h());

        res.Lock();
        for(s32 yy = 0; yy < h(); yy++)
            for(s32 xx = 0; xx < w(); xx++)
                for(s32 oy = 0; oy < stretch_factor_y; ++oy)
                    for(s32 ox = 0; ox < stretch_factor_x; ++ox)
        {
            res.SetPixel(static_cast<s32>(stretch_factor_x * xx) + ox,
                static_cast<s32>(stretch_factor_y * yy) + oy, GetPixel(xx, yy));
        }
        res.Unlock();
    }

    return res;
}

Surface Surface::RenderReflect(int shape /* 0: none, 1 : vert, 2: horz, 3: both */) const
{
    Surface res(GetSize(), GetFormat());

    switch(shape % 4)
    {
        // normal
        default:
	    Blit(res);
            break;

            // vertical reflect
        case 1:
            res.Lock();
            for(int yy = 0; yy < h(); ++yy)
                for(int xx = 0; xx < w(); ++xx)
                    res.SetPixel(xx, h() - yy - 1, GetPixel(xx, yy));
            res.Unlock();
            break;

        // horizontal reflect
        case 2:
            res.Lock();
            for(int yy = 0; yy < h(); ++yy)
                for(int xx = 0; xx < w(); ++xx)
                    res.SetPixel(w() - xx - 1, yy, GetPixel(xx, yy));
            res.Unlock();
            break;

        // both variants
        case 3:
            res.Lock();
            for(int yy = 0; yy < h(); ++yy)
                for(int xx = 0; xx < w(); ++xx)
                    res.SetPixel(w() - xx - 1, h() - yy - 1, GetPixel(xx, yy));
            res.Unlock();
            break;
    }
    return res;
}

Surface Surface::RenderRotate(int parm /* 0: none, 1 : 90 CW, 2: 90 CCW, 3: 180 */) const
{
    // 90 CW or 90 CCW
    if(parm == 1 || parm == 2)
    {
        Surface res(Size(h(), w()), GetFormat()); /* height <-> width */

        res.Lock();
        for(int yy = 0; yy < h(); ++yy)
            for(int xx = 0; xx < w(); ++xx)
        {
            if(parm == 1)
                res.SetPixel(yy, w() - xx - 1, GetPixel(xx, yy));
            else
                res.SetPixel(h() - yy - 1, xx, GetPixel(xx, yy));
        }
        res.Unlock();
        return res;
    }
    else
    if(parm == 3)
        return RenderReflect(3);

    return RenderReflect(0);
}

Surface Surface::RenderStencil(const RGBA & color) const
{
    Surface res(GetSize(), GetFormat());
    u32 clkey0 = GetColorKey();
    RGBA clkey = GetRGB(clkey0);
    const u32 pixel = res.MapRGB(color);

    res.Lock();
    for(int y = 0; y < h(); ++y)
        for(int x = 0; x < w(); ++x)
    {
        RGBA col = GetRGB(GetPixel(x, y));
        if((clkey0 && clkey == col) || col.a() < 200) continue;
        res.SetPixel(x, y, pixel);
    }
    res.Unlock();
    return res;
}

Surface Surface::RenderContour(const RGBA & color) const
{
    const RGBA fake = RGBA(0x00, 0xFF, 0xFF);
    Surface res(GetSize(), GetFormat());
    Surface trf = RenderStencil(fake);
    u32 clkey0 = trf.GetColorKey();
    RGBA clkey = trf.GetRGB(clkey0);
    const u32 pixel = res.MapRGB(color);
    const u32 fake2 = trf.MapRGB(fake);

    res.Lock();
    for(int y = 0; y < trf.h(); ++y)
        for(int x = 0; x < trf.w(); ++x)
            if(fake2 == trf.GetPixel(x, y))
    {
        if(0 == x || 0 == y ||
            trf.w() - 1 == x || trf.h() - 1 == y) res.SetPixel(x, y, pixel);
        else
        {
            if(0 < x)
            {
                RGBA col = trf.GetRGB(trf.GetPixel(x - 1, y));
                if((clkey0 && col == clkey) || col.a() < 200) res.SetPixel(x - 1, y, pixel);
            }
            if(trf.w() - 1 > x)
            {
                RGBA col = trf.GetRGB(trf.GetPixel(x + 1, y));
                if((clkey0 && col == clkey) || col.a() < 200) res.SetPixel(x + 1, y, pixel);
            }

            if(0 < y)
            {
                RGBA col = trf.GetRGB(trf.GetPixel(x, y - 1));
                if((clkey0 && col == clkey) || col.a() < 200) res.SetPixel(x, y - 1, pixel);
            }
            if(trf.h() - 1 > y)
            {
                RGBA col = trf.GetRGB(trf.GetPixel(x, y + 1));
                if((clkey0 && col == clkey) || col.a() < 200) res.SetPixel(x, y + 1, pixel);
            }
        }
    }
    res.Unlock();
    return res;
}

Surface Surface::RenderGrayScale(void) const
{
    Surface res(GetSize(), GetFormat());
    const u32 colkey = GetColorKey();
    u32 pixel = 0;

    res.Lock();
    for(int y = 0; y < h(); ++y)
        for(int x = 0; x < w(); ++x)
    {
        pixel = GetPixel(x, y);
        if(0 == colkey || pixel != colkey)
        {
            RGBA col = GetRGB(pixel);
            int z = col.r() * 0.299f + col.g() * 0.587f + col.b() * 0.114f;
            pixel = res.MapRGB(RGBA(z, z, z, col.a()));
            res.SetPixel(x, y, pixel);
        }
    }
    res.Unlock();
    return res;
}

Surface Surface::RenderSepia(void) const
{
    Surface res(GetSize(), GetFormat());
    const u32 colkey = GetColorKey();
    u32 pixel = 0;

    res.Lock();
    for(int x = 0; x < w(); x++)
        for(int y = 0; y < h(); y++)
    {
        pixel = GetPixel(x, y);
        if(colkey == 0 || pixel != colkey)
        {
	    RGBA col = GetRGB(pixel);
            //Numbers derived from http://blogs.techrepublic.com.com/howdoi/?p=120
            #define CLAMP255(val) std::min<u16>((val), 255)
            int outR = CLAMP255(static_cast<u16>(col.r() * 0.693f + col.g() * 0.769f + col.b() * 0.189f));
            int outG = CLAMP255(static_cast<u16>(col.r() * 0.449f + col.g() * 0.686f + col.b() * 0.168f));
            int outB = CLAMP255(static_cast<u16>(col.r() * 0.272f + col.g() * 0.534f + col.b() * 0.131f));
            pixel = res.MapRGB(RGBA(outR, outG, outB, col.a()));
            res.SetPixel(x, y, pixel);
            #undef CLAMP255
        }
    }
    res.Unlock();
    return res;
}

Surface Surface::RenderChangeColor(const RGBA & col1, const RGBA & col2) const
{
    Surface res = GetSurface();
    u32 fc = MapRGB(col1);
    u32 tc = res.MapRGB(col2);

    if(amask())
        fc |= amask();

    if(res.amask())
        tc |= res.amask();

    res.Lock();
    if(fc != tc)
        for(int y = 0; y < h(); ++y)
            for(int x = 0; x < w(); ++x)
                if(fc == GetPixel(x, y)) res.SetPixel(x, y, tc);
    res.Unlock();
    return res;
}

Surface Surface::GetSurface(void) const
{
    return GetSurface(Rect(Point(0, 0), GetSize()));
}

Surface Surface::GetSurface(const Rect & rt) const
{
    SurfaceFormat fm = GetFormat();

    if(isDisplay())
    {
	Surface res(rt, fm);
	Blit(rt, Point(0, 0), res);
	return res;
    }

    Surface res(rt, fm);

#if SDL_VERSION_ATLEAST(2, 0, 0)
    if(amask())
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
#else
    if(amask())
	SDL_SetAlpha(surface, 0, 0);
#endif

    Blit(rt, Point(0, 0), res);

#if SDL_VERSION_ATLEAST(2, 0, 0)
    if(amask())
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
#else
    if(amask())
	SDL_SetAlpha(surface, SDL_SRCALPHA, 255);
#endif

    return res;
}

void Surface::Fill(const RGBA & col)
{
    FillRect(Rect(0, 0, w(), h()), col);
}

void Surface::FillRect(const Rect & rect, const RGBA & col)
{
    SDL_Rect dstrect = SDLRect(rect);
    SDL_FillRect(surface, &dstrect, MapRGB(col));
}

void Surface::DrawLine(const Point & p1, const Point & p2, const RGBA & color)
{
    int x1 = p1.x; int y1 = p1.y;
    int x2 = p2.x; int y2 = p2.y;

    const u32 pixel = MapRGB(color);
    const int dx = std::abs(x2 - x1);
    const int dy = std::abs(y2 - y1);

    Lock();
    if(dx > dy)
    {
        int ns = std::div(dx, 2).quot;

        for(int i = 0; i <= dx; ++i)
        {
            SetPixel(x1, y1, pixel);
            x1 < x2 ? ++x1 : --x1;
            ns -= dy;
            if(ns < 0)
            {
                y1 < y2 ? ++y1 : --y1;
                ns += dx;
            }
        }
    }
    else
    {
        int ns = std::div(dy, 2).quot;

        for(int i = 0; i <= dy; ++i)
        {
            SetPixel(x1, y1, pixel);
            y1 < y2 ? ++y1 : --y1;
            ns -= dx;
            if(ns < 0)
            {
                x1 < x2 ? ++x1 : --x1;
                ns += dy;
            }
        }
    }
    Unlock();
}

void Surface::DrawPoint(const Point & pt, const RGBA & color)
{
    Lock();
    SetPixel(pt.x, pt.y, MapRGB(color));
    Unlock();
}

void Surface::DrawRect(const Rect & rt, const RGBA & color)
{
    const u32 pixel = MapRGB(color);

    Lock();
    for(int i = rt.x; i < rt.x + rt.w; ++i)
    {
        SetPixel(i, rt.y, pixel);
        SetPixel(i, rt.y + rt.y + rt.h - 1, pixel);
    }

    for(int i = rt.y; i < rt.y + rt.h; ++i)
    {
        SetPixel(rt.x, i, pixel);
        SetPixel(rt.x + rt.w - 1, i, pixel);
    }
    Unlock();
}

void Surface::DrawBorder(const RGBA & color, bool solid)
{
    if(solid)
	DrawRect(Rect(Point(0, 0), GetSize()), color);
    else
    {
	const u32 pixel = MapRGB(color);

        for(int i = 0; i < w(); ++i)
        {
            SetPixel(i, 0, pixel);
            if(i + 1 < w()) SetPixel(i + 1, 0, pixel);
            i += 3;
        }
        for(int i = 0; i < w(); ++i)
        {
            SetPixel(i, h() - 1, pixel);
            if(i + 1 < w()) SetPixel(i + 1, h() - 1, pixel);
            i += 3;
        }
        for(int i = 0; i < h(); ++i)
        {
            SetPixel(0, i, pixel);
            if(i + 1 < h()) SetPixel(0, i + 1, pixel);
            i += 3;
        }
        for(int i = 0; i < h(); ++i)
        {
            SetPixel(w() - 1, i, pixel);
            if(i + 1 < h()) SetPixel(w() - 1, i + 1, pixel);
            i += 3;
        }
    }
}

Surface Surface::RenderSurface(const Size & sz) const
{
    return RenderSurface(Rect(Point(0,0), GetSize()), sz);
}

Surface Surface::RenderSurface(const Rect & srcrt, const Size & sz) const
{
    const Surface & srcsf = *this;
    Surface dstsf(sz, false);
    Rect dstrt = Rect(0, 0, sz.w, sz.h);
    u32 mw = dstrt.w < srcrt.w ? dstrt.w : srcrt.w;
    u32 mh = dstrt.h < srcrt.h ? dstrt.h : srcrt.h;

    u32 cw = mw / 3;
    u32 ch = mh / 3;
    s32 cx = srcrt.x + (srcrt.w - cw) / 2;
    s32 cy = srcrt.y + (srcrt.h - ch) / 2;
    u32 bw = mw - 2 * cw;
    u32 bh = mh - 2 * ch;

    u32 ox = (dstrt.w - (dstrt.w / bw) * bw) / 2;
    u32 oy = (dstrt.h - (dstrt.h / bh) * bh) / 2;

    // body
    if(bw < dstrt.w && bh < dstrt.h)
        for(u32 yy = 0; yy < (dstrt.h / bh); ++yy)
            for(u32 xx = 0; xx < (dstrt.w / bw); ++xx)
                srcsf.Blit(Rect(cx, cy, bw, bh), dstrt.x + ox + xx * bw, dstrt.y + oy + yy * bh, dstsf);

    // top, bottom bar
    for(u32 xx = 0; xx < (dstrt.w / bw); ++xx)
    {
        s32 dstx = dstrt.x + ox + xx * bw;
        srcsf.Blit(Rect(cx, srcrt.y, bw, ch), dstx, dstrt.y, dstsf);
        srcsf.Blit(Rect(cx, srcrt.y + srcrt.h - ch, bw, ch), dstx, dstrt.y + dstrt.h - ch, dstsf);
    }

    // left, right bar
    for(u32 yy = 0; yy < (dstrt.h / bh); ++yy)
    {
        s32 dsty = dstrt.y + oy + yy * bh;
        srcsf.Blit(Rect(srcrt.x, cy, cw, bh), dstrt.x, dsty, dstsf);
        srcsf.Blit(Rect(srcrt.x + srcrt.w - cw, cy, cw, bh), dstrt.x + dstrt.w - cw, dsty, dstsf);
    }

    // top left angle
    srcsf.Blit(Rect(srcrt.x, srcrt.y, cw, ch), dstrt.x, dstrt.y, dstsf);

    // top right angle
    srcsf.Blit(Rect(srcrt.x + srcrt.w - cw, srcrt.y, cw, ch), dstrt.x + dstrt.w - cw, dstrt.y, dstsf);

    // bottom left angle
    srcsf.Blit(Rect(srcrt.x, srcrt.y + srcrt.h - ch, cw, ch), dstrt.x, dstrt.y + dstrt.h - ch, dstsf);

    // bottom right angle
    srcsf.Blit(Rect(srcrt.x + srcrt.w - cw, srcrt.y + srcrt.h - ch, cw, ch), dstrt.x + dstrt.w - cw, dstrt.y + dstrt.h - ch, dstsf);

    return dstsf;
}
