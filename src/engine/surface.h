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

#include <map>
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
    RGBA( int r, int g, int b, int a = 255 );

    SDL_Color operator()( void ) const
    {
        return color;
    }
    bool operator==( const RGBA & col ) const
    {
        return pack() == col.pack();
    }
    bool operator!=( const RGBA & col ) const
    {
        return pack() != col.pack();
    }

    bool operator<( const RGBA & col ) const
    {
        return pack() < col.pack();
    }

    int r( void ) const;
    int g( void ) const;
    int b( void ) const;
    int a( void ) const;

    int pack( void ) const;

protected:
    SDL_Color color;
};

struct SurfaceFormat
{
    u32 depth;
    u32 rmask;
    u32 gmask;
    u32 bmask;
    u32 amask;
    RGBA ckey;

    SurfaceFormat()
        : depth( 0 )
        , rmask( 0 )
        , gmask( 0 )
        , bmask( 0 )
        , amask( 0 )
    {}
};

class Surface
{
public:
    Surface();
    Surface( const Size &, bool amask );
    Surface( const Size & sz, u32 bpp, bool amask );
    Surface( const Size &, const SurfaceFormat & );
    Surface( const std::string & );
    Surface( const void * pixels, u32 width, u32 height, u32 bytes_per_pixel /* 1, 2, 3, 4 */, bool amask, bool useDefaultPalette = true ); /* agg: create raw tile */
    Surface( const Surface & bs, bool useReference = true );
    Surface( SDL_Surface * );

    Surface & operator=( const Surface & );
    bool operator==( const Surface & ) const;
    SDL_Surface * operator()( void ) const
    {
        return surface;
    }

    virtual ~Surface();

    void Set( u32 sw, u32 sh, const SurfaceFormat & );
    void Set( u32 sw, u32 sh, bool amask );
    void Reset( void );

    bool Load( const std::string & );
    bool Save( const std::string & ) const;

    int w( void ) const;
    int h( void ) const;
    u32 depth( void ) const;
    u32 amask( void ) const;
    u32 alpha( void ) const;

    bool isRefCopy( void ) const;

    bool isValid( void ) const;

    void SetColorKey( const RGBA & );

    void Fill( const RGBA & );
    void FillRect( const Rect &, const RGBA & );

    static void SetDefaultPalette( SDL_Color *, int );
    static void SetDefaultColorKey( int, int, int );

protected:
    static void FreeSurface( Surface & );

    void Lock( void ) const;
    void Unlock( void ) const;

    u32 MapRGB( const RGBA & ) const;

    void Set( const Surface &, bool refcopy );
    void Set( u32 sw, u32 sh, u32 bpp /* bpp: 8, 16, 24, 32 */, bool amask );
    void Set( SDL_Surface * );
    void SetPalette( void );

    SDL_Surface * surface;
};

#endif
