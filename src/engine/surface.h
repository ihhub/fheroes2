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
#ifndef H2SURFACE_H
#define H2SURFACE_H

#include <string>
#include "rect.h"
#include "types.h"

struct Point;
struct Rect;
struct SDL_Surface;

class RGBA
{
public:
    RGBA();
    RGBA(int r, int g, int b, int a = 255);

    SDL_Color operator() (void) const { return color; }
    bool      operator== (const RGBA & col) const { return pack() == col.pack(); }
    bool      operator!= (const RGBA & col) const { return pack() != col.pack(); }

    int         r(void) const;
    int         g(void) const;
    int         b(void) const;
    int         a(void) const;

    int		pack(void) const;
    static RGBA unpack(int);

protected:
    SDL_Color   color;
};

#define ColorBlack RGBA(0,0,0,255)

struct SurfaceFormat
{
    u32		depth;
    u32	 	rmask;
    u32		gmask;
    u32 	bmask;
    u32		amask;
    RGBA	ckey;

    SurfaceFormat() : depth(0), rmask(0), gmask(0), bmask(0), amask(0) {}
};

class Surface
{
public:
    Surface();
    Surface(const Size &, bool amask);
    Surface(const Size &, const SurfaceFormat &);
    Surface(const std::string &);
    Surface(const void* pixels, u32 width, u32 height, u32 bytes_per_pixel /* 1, 2, 3, 4 */, bool amask);  /* agg: create raw tile */
    Surface(const Surface &);
    Surface(SDL_Surface*);

    Surface & operator= (const Surface &);
    bool operator== (const Surface &) const;
    SDL_Surface* operator() (void) const { return surface; }

    virtual ~Surface();

    void Set(u32 sw, u32 sh, const SurfaceFormat &);
    void Set(u32 sw, u32 sh, bool amask);
    void Reset(void);

    bool Load(const std::string &);
    bool Save(const std::string &) const;

    int w(void) const;
    int h(void) const;
    u32 depth(void) const;
    u32 amask(void) const;
    u32 alpha(void) const;

    Size GetSize(void) const;
    bool isRefCopy(void) const;
    SurfaceFormat GetFormat(void) const;

    bool isValid(void) const;

    void SetColorKey(const RGBA &);
    u32	 GetColorKey(void) const;

    void Blit(Surface &) const;
    void Blit(s32, s32, Surface &) const;
    void Blit(const Point &, Surface &) const;
    void Blit(const Rect & srt, s32, s32, Surface &) const;
    void Blit(const Rect & srt, const Point &, Surface &) const;

    void Fill(const RGBA &);
    void FillRect(const Rect &, const RGBA &);
    void DrawLine(const Point &, const Point &, const RGBA &);
    void DrawPoint(const Point &, const RGBA &);
    void DrawRect(const Rect &, const RGBA &);
    void DrawBorder(const RGBA &, bool solid = true);

    virtual u32  GetMemoryUsage(void) const;
    std::string  Info(void) const;

    Surface RenderScale(const Size &) const;
    Surface RenderReflect(int shape /* 0: none, 1 : vert, 2: horz, 3: both */) const;
    Surface RenderRotate(int parm /* 0: none, 1 : 90 CW, 2: 90 CCW, 3: 180 */) const;
    Surface RenderStencil(const RGBA &) const;
    Surface RenderContour(const RGBA &) const;
    Surface RenderGrayScale(void) const;
    Surface RenderSepia(void) const;
    Surface RenderChangeColor(const RGBA &, const RGBA &) const;
    Surface RenderSurface(const Rect & srt, const Size &) const;
    Surface RenderSurface(const Size &) const;

    virtual Surface GetSurface(void) const;
    virtual Surface GetSurface(const Rect &) const;

    static void SetDefaultPalette(SDL_Color*, int);
    static void SetDefaultDepth(u32);
    static void SetDefaultColorKey(int, int, int);
    static void Swap(Surface &, Surface &);

    void SetAlphaMod(int);

protected:
    static void FreeSurface(Surface &);

    virtual bool isDisplay(void) const;

    void Lock(void) const;
    void Unlock(void) const;

    //void SetColorMod(const RGBA &);
    //void SetBlendMode(int);

    u32	 MapRGB(const RGBA &) const;
    RGBA GetRGB(u32 pixel) const;

    void Set(const Surface &, bool refcopy);
    void Set(u32 sw, u32 sh, u32 bpp /* bpp: 8, 16, 24, 32 */, bool amask);
    void Set(SDL_Surface*);
    void SetPalette(void);

    void SetPixel4(s32 x, s32 y, u32 color);
    void SetPixel3(s32 x, s32 y, u32 color);
    void SetPixel2(s32 x, s32 y, u32 color);
    void SetPixel1(s32 x, s32 y, u32 color);
    void SetPixel(int x, int y, u32);

    u32 GetPixel4(s32 x, s32 y) const;
    u32 GetPixel3(s32 x, s32 y) const;
    u32 GetPixel2(s32 x, s32 y) const;
    u32 GetPixel1(s32 x, s32 y) const;
    u32 GetPixel(int x, int y) const;

    SDL_Surface* surface;
};

#endif
