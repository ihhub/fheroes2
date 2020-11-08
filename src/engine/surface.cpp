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
#include <cstring>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <memory>
#include <set>
#include <sstream>

#include "error.h"
#include "localevent.h"
#include "surface.h"
#include "system.h"
#include "tools.h"

#ifdef WITH_IMAGE
#include "IMG_savepng.h"
#include <SDL_image.h>
#endif

namespace
{
    u32 default_depth = 32;
    RGBA default_color_key;
    SDL_Color * pal_colors = NULL;
    u32 pal_nums = 0;

    std::set<const SDL_Surface *> paletteBasedSurface;
    std::set<const SDL_Surface *> surfaceToUpdate;

    SDL_Rect SDLRect( const Rect & rt2 )
    {
        SDL_Rect res;

        res.x = rt2.x;
        res.y = rt2.y;
        res.w = rt2.w;
        res.h = rt2.h;

        return res;
    }
}

SurfaceFormat GetRGBAMask( u32 bpp )
{
    SurfaceFormat fm;
    fm.depth = bpp;
    fm.ckey = default_color_key;

    switch ( bpp ) {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
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

RGBA::RGBA()
{
    color.r = 0;
    color.g = 0;
    color.b = 0;
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    color.a = 255;
#else
    color.unused = 255;
#endif
}

RGBA::RGBA( int r, int g, int b, int a )
{
    color.r = r;
    color.g = g;
    color.b = b;
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    color.a = a;
#else
    color.unused = a;
#endif
}

int RGBA::r( void ) const
{
    return color.r;
}

int RGBA::g( void ) const
{
    return color.g;
}

int RGBA::b( void ) const
{
    return color.b;
}

int RGBA::a( void ) const
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    return color.a;
#else
    return color.unused;
#endif
}

int RGBA::pack( void ) const
{
    return ( ( ( a() << 24 ) & 0xFF000000 ) | ( ( b() << 16 ) & 0x00FF0000 ) | ( ( g() << 8 ) & 0x0000FF00 ) | ( r() & 0x000000FF ) );
}

Surface::Surface()
    : surface( NULL )
{}

Surface::Surface( const Size & sz, bool amask )
    : surface( NULL )
{
    Set( sz.w, sz.h, amask );
}

Surface::Surface( const Size & sz, u32 bpp, bool amask )
    : surface( NULL )
{
    Set( sz.w, sz.h, bpp, amask );
}

Surface::Surface( const Size & sz, const SurfaceFormat & fm )
    : surface( NULL )
{
    Set( sz.w, sz.h, fm );
}

Surface::Surface( const Surface & bs, bool useReference )
    : surface( NULL )
{
    Set( bs, useReference );
}

Surface::Surface( const std::string & file )
    : surface( NULL )
{
    Load( file );
}

Surface::Surface( SDL_Surface * sf )
    : surface( sf )
{}

Surface::Surface( const void * pixels, u32 width, u32 height, u32 bytes_per_pixel /* 1, 2, 3, 4 */, bool amask, bool useDefaultPalette )
    : surface( NULL )
{
    SurfaceFormat fm = GetRGBAMask( 8 * bytes_per_pixel );

    if ( 8 == fm.depth ) {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        surface = SDL_CreateRGBSurface( 0, width, height, fm.depth, fm.rmask, fm.gmask, fm.bmask, ( amask ? fm.amask : 0 ) );
#else
        surface = SDL_CreateRGBSurface( SDL_SWSURFACE, width, height, fm.depth, fm.rmask, fm.gmask, fm.bmask, ( amask ? fm.amask : 0 ) );
#endif
    }
    else {
        surface = SDL_CreateRGBSurfaceFrom( const_cast<void *>( pixels ), width, height, fm.depth, width * bytes_per_pixel, fm.rmask, fm.gmask, fm.bmask,
                                            amask ? fm.amask : 0 );
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        SDL_SetSurfaceBlendMode( surface, SDL_BLENDMODE_NONE );
#endif
    }

    if ( !surface )
        Error::Except( __FUNCTION__, SDL_GetError() );

    if ( 8 == fm.depth ) {
        if ( useDefaultPalette ) {
            SetPalette();
        }
        Lock();
        std::memcpy( surface->pixels, pixels, width * height );
        Unlock();
    }
}

Surface::~Surface()
{
    FreeSurface( *this );
}

/* operator = */
Surface & Surface::operator=( const Surface & bs )
{
    Set( bs, true );
    return *this;
}

bool Surface::operator==( const Surface & bs ) const
{
    return surface && bs.surface ? surface == bs.surface : false;
}

void Surface::Reset( void )
{
    FreeSurface( *this );
    surface = NULL; /* hard set: for ref copy */
}

void Surface::Set( SDL_Surface * sf )
{
    FreeSurface( *this );
    surface = sf;
}

void Surface::Set( const Surface & bs, bool refcopy )
{
    FreeSurface( *this );

    if ( bs.isValid() ) {
        if ( refcopy ) {
            surface = bs.surface;
            if ( surface )
                surface->refcount += 1;
        }
        else {
            surface = SDL_ConvertSurface( bs.surface, bs.surface->format, bs.surface->flags );

            if ( !surface )
                Error::Except( __FUNCTION__, SDL_GetError() );
        }
    }
}

void Surface::Set( u32 sw, u32 sh, bool amask )
{
    Set( sw, sh, default_depth, amask );
}

void Surface::Set( u32 sw, u32 sh, u32 bpp /* bpp: 8, 16, 24, 32 */, bool amask )
{
    if ( bpp == 8 )
        bpp = 32;

    SurfaceFormat fm = GetRGBAMask( bpp );

    if ( 8 == fm.depth || !amask )
        fm.amask = 0;
    Set( sw, sh, fm );
}

void Surface::Set( u32 sw, u32 sh, const SurfaceFormat & fm )
{
    FreeSurface( *this );

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    surface = SDL_CreateRGBSurface( 0, sw, sh, fm.depth, fm.rmask, fm.gmask, fm.bmask, fm.amask );
#else
    surface = SDL_CreateRGBSurface( SDL_SWSURFACE, sw, sh, fm.depth, fm.rmask, fm.gmask, fm.bmask, fm.amask );
#endif

    if ( !surface )
        Error::Except( __FUNCTION__, SDL_GetError() );

    if ( 8 == depth() ) {
        SetPalette();
        Fill( fm.ckey );
        SetColorKey( fm.ckey );
    }
    else if ( amask() ) {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        Fill( RGBA( 0, 0, 0, 0 ) ); // no color key only amask
#else
        Fill( RGBA( fm.ckey.r(), fm.ckey.g(), fm.ckey.b(), 0 ) );
        SetColorKey( fm.ckey );
#endif
    }
    else if ( fm.ckey.pack() ) {
        Fill( fm.ckey );
        SetColorKey( fm.ckey );
    }

    if ( amask() ) {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        SDL_SetSurfaceBlendMode( surface, SDL_BLENDMODE_BLEND );
#else
        SDL_SetAlpha( surface, SDL_SRCALPHA, 255 );
#endif
    }
    else {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        SDL_SetSurfaceBlendMode( surface, SDL_BLENDMODE_NONE );
#else
        SDL_SetAlpha( surface, 0, 0 );
#endif
    }
}

void Surface::SetDefaultPalette( SDL_Color * ptr, int num )
{
    pal_colors = ptr;
    pal_nums = num;

    surfaceToUpdate = paletteBasedSurface;
}

void Surface::SetDefaultColorKey( int r, int g, int b )
{
    default_color_key = RGBA( r, g, b );
}

bool Surface::isValid( void ) const
{
    return surface && surface->format;
}

bool Surface::Load( const std::string & fn )
{
    FreeSurface( *this );

#ifdef WITH_IMAGE
    surface = IMG_Load( fn.c_str() );
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    SDL_SetSurfaceBlendMode( surface, SDL_BLENDMODE_NONE );
#endif
#else
    surface = SDL_LoadBMP( fn.c_str() );
#endif

    if ( !surface )
        ERROR( SDL_GetError() );

    return surface;
}

bool Surface::Save( const std::string & fn ) const
{
    int res = 0;

#ifdef WITH_IMAGE
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    res = IMG_SavePNG( surface, fn.c_str() );
#else
    res = IMG_SavePNG( fn.c_str(), surface, -1 );
#endif
#else
    res = SDL_SaveBMP( surface, fn.c_str() );
#endif

    if ( 0 != res ) {
        ERROR( SDL_GetError() );
        return false;
    }

    return true;
}

int Surface::w( void ) const
{
    return surface ? surface->w : 0;
}

int Surface::h( void ) const
{
    return surface ? surface->h : 0;
}

u32 Surface::depth( void ) const
{
    return isValid() ? surface->format->BitsPerPixel : 0;
}

u32 Surface::amask( void ) const
{
    return isValid() ? surface->format->Amask : 0;
}

u32 Surface::alpha( void ) const
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( isValid() ) {
        u8 alpha = 0;
        SDL_GetSurfaceAlphaMod( surface, &alpha );
        return alpha;
    }
    return 0;
#else
    return isValid() ? surface->format->alpha : 0;
#endif
}

u32 Surface::MapRGB( const RGBA & color ) const
{
    return amask() ? SDL_MapRGBA( surface->format, color.r(), color.g(), color.b(), color.a() ) : SDL_MapRGB( surface->format, color.r(), color.g(), color.b() );
}

/* load static palette (economize 1kb for each surface) only 8bit color! */
void Surface::SetPalette( void )
{
    if ( isValid() && pal_colors && pal_nums > 0 && surface->format->palette ) {
        if ( surface->format->palette->colors && pal_colors != surface->format->palette->colors && surface->format->palette->ncolors != pal_nums ) {
            SDL_free( surface->format->palette->colors );
            surface->format->palette->ncolors = pal_nums;
        }

        paletteBasedSurface.insert( surface );
        surface->format->palette->colors = pal_colors;
    }
}

void Surface::SetColorKey( const RGBA & color )
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    SDL_SetColorKey( surface, SDL_TRUE, MapRGB( color ) );
#else
    SDL_SetColorKey( surface, SDL_SRCCOLORKEY, MapRGB( color ) );
#endif
}

void Surface::Lock( void ) const
{
    if ( SDL_MUSTLOCK( surface ) )
        SDL_LockSurface( surface );
}

void Surface::Unlock( void ) const
{
    if ( SDL_MUSTLOCK( surface ) )
        SDL_UnlockSurface( surface );
}

bool Surface::isRefCopy( void ) const
{
    return surface ? 1 < surface->refcount : false;
}

void Surface::FreeSurface( Surface & sf )
{
    if ( sf.surface ) {
        if ( sf.isRefCopy() ) {
            --sf.surface->refcount;
            sf.surface = NULL;
        }
        else {
            // clear static palette
            if ( sf.surface->format && 8 == sf.surface->format->BitsPerPixel && pal_colors && pal_nums && sf.surface->format->palette
                 && pal_colors == sf.surface->format->palette->colors ) {
                sf.surface->format->palette->colors = NULL;
                sf.surface->format->palette->ncolors = 0;
            }

            paletteBasedSurface.erase( sf.surface );

            SDL_FreeSurface( sf.surface );
            sf.surface = NULL;
        }
    }
}

void Surface::Fill( const RGBA & col )
{
    FillRect( Rect( 0, 0, w(), h() ), col );
}

void Surface::FillRect( const Rect & rect, const RGBA & col )
{
    SDL_Rect dstrect = SDLRect( rect );
    SDL_FillRect( surface, &dstrect, MapRGB( col ) );
}
