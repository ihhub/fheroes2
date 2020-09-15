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

#include "display.h"
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

u32 GetPixel24( u8 * ptr )
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

void SetPixel24( u8 * ptr, u32 color )
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

RGBA RGBA::unpack( int v )
{
    int r = ( v >> 24 ) & 0x000000FF;
    int g = ( v >> 16 ) & 0x000000FF;
    int b = ( v >> 8 ) & 0x000000FF;
    int a = v & 0x000000FF;

    return RGBA( r, g, b, a );
}

Surface::Surface()
    : surface( NULL )
    , _isDisplay( false )
{}

Surface::Surface( const Size & sz, bool amask )
    : surface( NULL )
    , _isDisplay( false )
{
    Set( sz.w, sz.h, amask );
}

Surface::Surface( const Size & sz, u32 bpp, bool amask )
    : surface( NULL )
    , _isDisplay( false )
{
    Set( sz.w, sz.h, bpp, amask );
}

Surface::Surface( const Size & sz, const SurfaceFormat & fm )
    : surface( NULL )
    , _isDisplay( false )
{
    Set( sz.w, sz.h, fm );
}

Surface::Surface( const Surface & bs, bool useReference )
    : surface( NULL )
    , _isDisplay( false )
{
    Set( bs, useReference );
}

Surface::Surface( const std::string & file )
    : surface( NULL )
    , _isDisplay( false )
{
    Load( file );
}

Surface::Surface( SDL_Surface * sf )
    : surface( sf )
    , _isDisplay( false )
{}

Surface::Surface( const void * pixels, u32 width, u32 height, u32 bytes_per_pixel /* 1, 2, 3, 4 */, bool amask, bool useDefaultPalette )
    : surface( NULL )
    , _isDisplay( false )
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
    if ( !isDisplay() )
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

Size Surface::GetSize( void ) const
{
    return Size( w(), h() );
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

SurfaceFormat Surface::GetFormat( void ) const
{
    SurfaceFormat res;
    if ( surface->format ) {
        res.depth = surface->format->BitsPerPixel;
        res.rmask = surface->format->Rmask;
        res.gmask = surface->format->Gmask;
        res.bmask = surface->format->Bmask;
        res.amask = surface->format->Amask;
        res.ckey = default_color_key;
    }
    return res;
}

u32 Surface::MapRGB( const RGBA & color ) const
{
    return amask() ? SDL_MapRGBA( surface->format, color.r(), color.g(), color.b(), color.a() ) : SDL_MapRGB( surface->format, color.r(), color.g(), color.b() );
}

RGBA Surface::GetRGB( u32 pixel ) const
{
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;
    u8 a = 0;

    if ( amask() ) {
        SDL_GetRGBA( pixel, surface->format, &r, &g, &b, &a );
        return RGBA( r, g, b, a );
    }

    SDL_GetRGB( pixel, surface->format, &r, &g, &b );
    return RGBA( r, g, b );
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

void Surface::SetPalette( const std::vector<SDL_Color> & colors )
{
    if ( isValid() && !colors.empty() && surface->format->palette ) {
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        SDL_SetPaletteColors( surface->format->palette, colors.data(), 0, colors.size() );
#else
        SDL_SetPalette( surface, SDL_LOGPAL, const_cast<SDL_Color *>( colors.data() ), 0, colors.size() );
#endif
    }
}

u32 Surface::GetColorKey( void ) const
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( isValid() && !amask() ) {
        u32 res = 0;
        SDL_GetColorKey( surface, &res );
        return res;
    }
    return 0;
#else
    return isValid() && ( surface->flags & SDL_SRCCOLORKEY ) ? surface->format->colorkey : 0;
#endif
}

void Surface::SetColorKey( const RGBA & color )
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    SDL_SetColorKey( surface, SDL_TRUE, MapRGB( color ) );
#else
    SDL_SetColorKey( surface, SDL_SRCCOLORKEY, MapRGB( color ) );
#endif
}

/* draw u32 pixel */
void Surface::SetPixel4( s32 x, s32 y, u32 color )
{
    u32 * bufp = static_cast<u32 *>( surface->pixels ) + y * ( surface->pitch >> 2 ) + x;
    *bufp = color;
}

/* draw u24 pixel */
void Surface::SetPixel3( s32 x, s32 y, u32 color )
{
    u8 * bufp = static_cast<u8 *>( surface->pixels ) + y * surface->pitch + x * 3;
    SetPixel24( bufp, color );
}

/* draw u16 pixel */
void Surface::SetPixel2( s32 x, s32 y, u32 color )
{
    u16 * bufp = static_cast<u16 *>( surface->pixels ) + y * ( surface->pitch >> 1 ) + x;
    *bufp = static_cast<u16>( color );
}

/* draw u8 pixel */
void Surface::SetPixel1( s32 x, s32 y, u32 color )
{
    u8 * bufp = static_cast<u8 *>( surface->pixels ) + y * surface->pitch + x;
    *bufp = static_cast<u8>( color );
}

/* draw pixel */
void Surface::SetPixel( int x, int y, u32 pixel )
{
    if ( x < w() && y < h() ) {
        switch ( depth() ) {
        case 8:
            SetPixel1( x, y, pixel );
            break;
        case 15:
        case 16:
            SetPixel2( x, y, pixel );
            break;
        case 24:
            SetPixel3( x, y, pixel );
            break;
        case 32:
            SetPixel4( x, y, pixel );
            break;
        default:
            break;
        }
    }
    else {
        std::ostringstream os;
        os << "out of range: "
           << "x: " << x << ", "
           << "y: " << y << ", "
           << "width: " << w() << ", "
           << "height: " << h();
        Error::Except( __FUNCTION__, os.str().c_str() );
    }
}

u32 Surface::GetPixel4( s32 x, s32 y ) const
{
    u32 * bufp = static_cast<u32 *>( surface->pixels ) + y * ( surface->pitch >> 2 ) + x;
    return *bufp;
}

u32 Surface::GetPixel3( s32 x, s32 y ) const
{
    u8 * bufp = static_cast<u8 *>( surface->pixels ) + y * surface->pitch + x * 3;
    return GetPixel24( bufp );
}

u32 Surface::GetPixel2( s32 x, s32 y ) const
{
    u16 * bufp = static_cast<u16 *>( surface->pixels ) + y * ( surface->pitch >> 1 ) + x;
    return static_cast<u32>( *bufp );
}

u32 Surface::GetPixel1( s32 x, s32 y ) const
{
    u8 * bufp = static_cast<u8 *>( surface->pixels ) + y * surface->pitch + x;
    return static_cast<u32>( *bufp );
}

u32 Surface::GetPixel( int x, int y ) const
{
    u32 pixel = 0;

    if ( x < w() && y < h() ) {
        switch ( depth() ) {
        case 8:
            pixel = GetPixel1( x, y );
            break;
        case 15:
        case 16:
            pixel = GetPixel2( x, y );
            break;
        case 24:
            pixel = GetPixel3( x, y );
            break;
        case 32:
            pixel = GetPixel4( x, y );
            break;
        default:
            break;
        }
    }
    else
        Error::Except( __FUNCTION__, "out of range" );

    return pixel;
}

// Optimized version of SetPixel without error and boundries checking. Validate before use
void Surface::SetRawPixel( int position, uint32_t pixel )
{
    switch ( surface->format->BitsPerPixel ) {
    case 8:
        *( static_cast<uint8_t *>( surface->pixels ) + position ) = static_cast<uint8_t>( pixel );
        break;
    case 15:
    case 16:
        *( static_cast<uint16_t *>( surface->pixels ) + position ) = static_cast<uint16_t>( pixel );
        break;
    case 24:
        SetPixel24( static_cast<uint8_t *>( surface->pixels ) + position * 3, pixel );
        break;
    case 32:
        *( static_cast<uint32_t *>( surface->pixels ) + position ) = static_cast<uint32_t>( pixel );
        break;
    default:
        break;
    }
}

// Optimized version of GetPixel without error and boundries checking. Private call, validate before use
uint32_t Surface::GetRawPixelValue( int position ) const
{
    switch ( surface->format->BitsPerPixel ) {
    case 8:
        return *( static_cast<uint8_t *>( surface->pixels ) + position );
    case 15:
    case 16:
        return *( static_cast<uint16_t *>( surface->pixels ) + position );
    case 24:
        return GetPixel24( static_cast<uint8_t *>( surface->pixels ) + position * 3 );
    case 32:
        return *( static_cast<uint32_t *>( surface->pixels ) + position );
    default:
        break;
    }
    return 0;
}

void Surface::Blit( const Rect & srt, const Point & dpt, Surface & dst ) const
{
    SDL_Rect dstrect = SDLRect( dpt.x, dpt.y, srt.w, srt.h );
    SDL_Rect srcrect = SDLRect( srt );

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    SDL_BlitSurface( surface, &srcrect, dst.surface, &dstrect );
#else
    if ( !dst.isDisplay() && amask() && dst.amask() ) {
        SDL_SetAlpha( surface, 0, 0 );
        SDL_BlitSurface( surface, &srcrect, dst.surface, &dstrect );
        SDL_SetAlpha( surface, SDL_SRCALPHA, 255 );
    }
    else
        SDL_BlitSurface( surface, &srcrect, dst.surface, &dstrect );
#endif
}

void Surface::Blit( Surface & dst ) const
{
    Blit( Rect( Point( 0, 0 ), GetSize() ), Point( 0, 0 ), dst );
}

void Surface::Blit( s32 dx, s32 dy, Surface & dst ) const
{
    Blit( Point( dx, dy ), dst );
}

void Surface::Blit( const Rect & srt, s32 dx, s32 dy, Surface & dst ) const
{
    Blit( srt, Point( dx, dy ), dst );
}

void Surface::Blit( const Point & dpt, Surface & dst ) const
{
    Blit( Rect( Point( 0, 0 ), GetSize() ), dpt, dst );
}

void Surface::SetAlphaMod( int level, bool makeCopy )
{
    if ( makeCopy )
        Set( GetSurface(), true );

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( isValid() ) {
        SDL_SetSurfaceAlphaMod( surface, level );
        SDL_SetSurfaceBlendMode( surface, SDL_BLENDMODE_BLEND );
    }
#else
    if ( isValid() ) {
        if ( amask() ) {
            if ( depth() == 32 ) {
                Set( ModifyAlphaChannel( level ), true );
            }
            else {
                Surface res( GetSize(), false );
                SDL_SetAlpha( surface, 0, 0 );
                Blit( res );
                SDL_SetAlpha( surface, SDL_SRCALPHA, 255 );
                Set( res, true );
            }
        }

        SDL_SetAlpha( surface, SDL_SRCALPHA, level );
    }
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

u32 Surface::GetMemoryUsage( void ) const
{
    u32 res = sizeof( surface );

    if ( surface ) {
        res += sizeof( SDL_Surface ) + sizeof( SDL_PixelFormat ) + surface->pitch * surface->h;

        if ( surface->format && surface->format->palette && ( !pal_colors || pal_colors != surface->format->palette->colors ) )
            res += sizeof( SDL_Palette ) + surface->format->palette->ncolors * sizeof( SDL_Color );
    }

    return res;
}

void Surface::Swap( Surface & sf1, Surface & sf2 )
{
    std::swap( sf1.surface, sf2.surface );
}

bool Surface::isDisplay( void ) const
{
    return _isDisplay;
}

Surface Surface::RenderScale( const Size & size ) const
{
    Surface res( size, GetFormat() );

    if ( size.w >= 2 && size.h >= 2 ) {
        float stretch_factor_x = size.w / static_cast<float>( w() );
        float stretch_factor_y = size.h / static_cast<float>( h() );

        res.Lock();
        for ( s32 yy = 0; yy < h(); yy++ )
            for ( s32 xx = 0; xx < w(); xx++ )
                for ( s32 oy = 0; oy < stretch_factor_y; ++oy )
                    for ( s32 ox = 0; ox < stretch_factor_x; ++ox ) {
                        res.SetPixel( static_cast<s32>( stretch_factor_x * xx ) + ox, static_cast<s32>( stretch_factor_y * yy ) + oy, GetPixel( xx, yy ) );
                    }
        res.Unlock();
    }

    return res;
}

Surface Surface::RenderReflect( int shape /* 0: none, 1 : vert, 2: horz, 3: both */ ) const
{
    Surface res( GetSize(), GetFormat() );

    switch ( shape % 4 ) {
    // normal
    default:
        Blit( res );
        break;

    // vertical reflect
    case 1:
        res.Lock();
        for ( int yy = 0; yy < h(); ++yy )
            for ( int xx = 0; xx < w(); ++xx )
                res.SetPixel( xx, h() - yy - 1, GetPixel( xx, yy ) );
        res.Unlock();
        break;

    // horizontal reflect
    case 2:
        res.Lock();
        for ( int yy = 0; yy < h(); ++yy )
            for ( int xx = 0; xx < w(); ++xx )
                res.SetPixel( w() - xx - 1, yy, GetPixel( xx, yy ) );
        res.Unlock();
        break;

    // both variants
    case 3:
        res.Lock();
        for ( int yy = 0; yy < h(); ++yy )
            for ( int xx = 0; xx < w(); ++xx )
                res.SetPixel( w() - xx - 1, h() - yy - 1, GetPixel( xx, yy ) );
        res.Unlock();
        break;
    }
    return res;
}

Surface Surface::RenderRotate( int parm /* 0: none, 1 : 90 CW, 2: 90 CCW, 3: 180 */ ) const
{
    // 90 CW or 90 CCW
    if ( parm == 1 || parm == 2 ) {
        Surface res( Size( h(), w() ), GetFormat() ); /* height <-> width */

        res.Lock();
        const int height = h();
        const int width = w();

        if ( parm == 1 ) {
            for ( int yy = 0; yy < height; ++yy )
                for ( int xx = 0; xx < width; ++xx )
                    res.SetPixel( yy, width - xx - 1, GetPixel( xx, yy ) );
        }
        else {
            for ( int yy = 0; yy < height; ++yy )
                for ( int xx = 0; xx < width; ++xx )
                    res.SetPixel( height - yy - 1, xx, GetPixel( xx, yy ) );
        }
        res.Unlock();
        return res;
    }
    else if ( parm == 3 ) {
        return RenderReflect( 3 );
    }

    return RenderReflect( 0 );
}

Surface Surface::RenderStencil( const RGBA & color ) const
{
    Surface res( GetSize(), GetFormat() );
    u32 clkey0 = GetColorKey();
    RGBA clkey = GetRGB( clkey0 );
    const u32 pixel = res.MapRGB( color );

    res.Lock();
    for ( int y = 0; y < h(); ++y )
        for ( int x = 0; x < w(); ++x ) {
            RGBA col = GetRGB( GetPixel( x, y ) );
            if ( ( clkey0 && clkey == col ) || col.a() < 200 )
                continue;
            res.SetPixel( x, y, pixel );
        }
    res.Unlock();
    return res;
}

Surface Surface::ModifyAlphaChannel( uint32_t alpha ) const
{
    Surface res = GetSurface();

    if ( amask() && depth() == 32 && alpha < 256 ) {
        res.Lock();

        int height = h();
        int width = w();
        uint16_t pitch = res.surface->pitch >> 2;

        if ( pitch == width ) {
            width = width * height;
            pitch = width;
            height = 1;
        }

        uint32_t * y = static_cast<uint32_t *>( res.surface->pixels );
        const uint32_t * yEnd = y + pitch * height;
        for ( ; y != yEnd; y += pitch ) {
            uint32_t * x = y;
            const uint32_t * xEnd = x + width;
            for ( ; x != xEnd; ++x ) {
                const uint32_t rest = *x % 0x1000000;

                *x = ( ( ( *x >> 24 ) * alpha / 255 ) << 24 ) + rest;
            }
        }

        res.Unlock();
    }

    return res;
}

Surface Surface::GetSurface( void ) const
{
    return GetSurface( Rect( Point( 0, 0 ), GetSize() ) );
}

Surface Surface::GetSurface( const Rect & rt ) const
{
    SurfaceFormat fm = GetFormat();

    if ( isDisplay() ) {
        Surface res( rt, fm );
        Blit( rt, Point( 0, 0 ), res );
        return res;
    }

    Surface res( rt, fm );

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( amask() )
        SDL_SetSurfaceBlendMode( surface, SDL_BLENDMODE_NONE );
#else
    if ( amask() )
        SDL_SetAlpha( surface, 0, 0 );
#endif

    Blit( rt, Point( 0, 0 ), res );

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( amask() )
        SDL_SetSurfaceBlendMode( surface, SDL_BLENDMODE_BLEND );
#else
    if ( amask() )
        SDL_SetAlpha( surface, SDL_SRCALPHA, 255 );
#endif

    return res;
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

void Surface::DrawPoint( const Point & pt, const RGBA & color )
{
    Lock();
    SetPixel( pt.x, pt.y, MapRGB( color ) );
    Unlock();
}

void Surface::DrawRect( const Rect & rt, const RGBA & color )
{
    const u32 pixel = MapRGB( color );

    Lock();
    for ( int i = rt.x; i < rt.x + rt.w; ++i ) {
        SetPixel( i, rt.y, pixel );
        SetPixel( i, rt.y + rt.y + rt.h - 1, pixel );
    }

    for ( int i = rt.y; i < rt.y + rt.h; ++i ) {
        SetPixel( rt.x, i, pixel );
        SetPixel( rt.x + rt.w - 1, i, pixel );
    }
    Unlock();
}

void Surface::DrawBorder( const RGBA & color, bool solid )
{
    if ( solid )
        DrawRect( Rect( Point( 0, 0 ), GetSize() ), color );
    else {
        const u32 pixel = MapRGB( color );
        const int width = w();
        const int height = h();

        for ( int i = 0; i < width; i += 4 ) {
            SetPixel( i, 0, pixel );
            if ( i + 1 < width )
                SetPixel( i + 1, 0, pixel );
        }
        for ( int i = 0; i < width; i += 4 ) {
            SetPixel( i, height - 1, pixel );
            if ( i + 1 < width )
                SetPixel( i + 1, height - 1, pixel );
        }
        for ( int i = 0; i < height; i += 4 ) {
            SetPixel( 0, i, pixel );
            if ( i + 1 < height )
                SetPixel( 0, i + 1, pixel );
        }
        for ( int i = 0; i < height; i += 4 ) {
            SetPixel( width - 1, i, pixel );
            if ( i + 1 < height )
                SetPixel( width - 1, i + 1, pixel );
        }
    }
}
