/***************************************************************************
 *   Copyright (C) 2008 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <cmath>
#include <set>
#include <sstream>
#include <string>

#include "display.h"
#include "error.h"
#include "system.h"
#include "tools.h"
#include "types.h"

// This is new Graphics engine. To change the code slowly we have to do some hacks here for now
#include "screen.h"

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
Display::Display()
    : window( NULL )
    , renderer( NULL )
    , displayTexture( NULL )
    , keepAspectRatio( false )
{
    _isDisplay = true;
}
#else
Display::Display()
{
    _isDisplay = true;
}
#endif

Display::~Display()
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    if ( displayTexture ) {
        SDL_DestroyTexture( displayTexture );
        displayTexture = NULL;
    }

    if ( renderer ) {
        SDL_DestroyRenderer( renderer );
        renderer = NULL;
    }

    if ( window ) {
        SDL_DestroyWindow( window );
        window = NULL;
    }

    FreeSurface( *this );
#endif
}

void Display::SetVideoMode( int w, int h, bool fullscreen, bool aspect, bool changeVideo )
{
    // new display
    fheroes2::Display & display = fheroes2::Display::instance();
    display.resize( w, h );
    display.fill( 0 );

    // old display
    Set( display.width(), display.height(), false );
    Fill( RGBA( 0, 0, 0 ) );

    /*
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    u32 flags = SDL_WINDOW_SHOWN;
    if ( fullscreen ) {
        if ( changeVideo ) {
            flags |= SDL_WINDOW_FULLSCREEN;
        }
        else {
#if defined( __WIN32__ )
            flags |= SDL_WINDOW_FULLSCREEN;
#else
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
        }
        keepAspectRatio = aspect;
    }
    else {
        keepAspectRatio = false;
    }

    if ( !keepAspectRatio ) { // we don't allow random resolutions
        const std::vector<std::pair<int, int> > resolutions = GetAvailableResolutions();
        if ( resolutions.empty() ) {
            SDL_DisplayMode currentVideoMode;
            SDL_GetCurrentDisplayMode( 0, &currentVideoMode );

            currentVideoMode.w = w;
            currentVideoMode.h = h;

            SDL_DisplayMode closestVideoMode;
            if ( SDL_GetClosestDisplayMode( 0, &currentVideoMode, &closestVideoMode ) != NULL ) {
                w = closestVideoMode.w;
                h = closestVideoMode.h;
            }
        }
        else {
            const std::pair<int, int> correctResolution = GetNearestResolution( w, h, resolutions );
            w = correctResolution.first;
            h = correctResolution.second;
        }
    }

    if ( displayTexture )
        SDL_DestroyTexture( displayTexture );
    if ( renderer )
        SDL_DestroyRenderer( renderer );

    std::string previousWindowTitle;
    int prevX = SDL_WINDOWPOS_CENTERED;
    int prevY = SDL_WINDOWPOS_CENTERED;
    if ( window ) {
        previousWindowTitle = SDL_GetWindowTitle( window );
        SDL_GetWindowPosition( window, &prevX, &prevY );
        SDL_DestroyWindow( window );
    }

    window = SDL_CreateWindow( "", prevX, prevY, w, h, flags );
    renderer = SDL_CreateRenderer( window, -1, System::GetRenderFlags() );
    displayTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, w, h );

    if ( keepAspectRatio ) {
        SDL_DisplayMode currentVideoMode;
        SDL_GetCurrentDisplayMode( 0, &currentVideoMode );

        const float ratio = static_cast<float>( w ) / static_cast<float>( h );

        srcRenderSurface.w = w;
        srcRenderSurface.h = h;
        srcRenderSurface.x = 0;
        srcRenderSurface.y = 0;

        dstRenderSurface.w = static_cast<int>( currentVideoMode.h * ratio + 0.5f );
        dstRenderSurface.h = currentVideoMode.h;
        dstRenderSurface.x = ( currentVideoMode.w - dstRenderSurface.w ) / 2;
        dstRenderSurface.y = 0;

        SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
        SDL_RenderClear( renderer );
    }

    if ( !renderer )
        Error::Except( __FUNCTION__, SDL_GetError() );

    if ( !previousWindowTitle.empty() )
        SDL_SetWindowTitle( window, previousWindowTitle.data() );

    Set( w, h, false );
    Fill( RGBA( 0, 0, 0 ) );
#else
    const std::vector<std::pair<int, int> > resolutions = GetAvailableResolutions();
    if ( !resolutions.empty() ) {
        const std::pair<int, int> correctResolution = GetNearestResolution( w, h, resolutions );
        w = correctResolution.first;
        h = correctResolution.second;
    }

    u32 flags = System::GetRenderFlags();

    if ( fullscreen )
        flags |= SDL_FULLSCREEN;

    surface = SDL_SetVideoMode( w, h, 0, flags );

    if ( !surface )
        Error::Except( __FUNCTION__, SDL_GetError() );
#endif
    */
}

Size Display::GetSize( void ) const
{
    const fheroes2::Display & display = fheroes2::Display::instance();
    return Size( display.width(), display.height() );
}

Size Display::GetDefaultSize( void )
{
    return Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );
}

std::string Display::GetInfo( void ) const
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    std::ostringstream os;
    os << "Display::GetInfo: " << GetString( GetSize() ) << ", "
       << "driver: " << SDL_GetCurrentVideoDriver();
    return os.str();
#else
    std::ostringstream os;
    char namebuf[12];

    os << "Display::"
       << "GetInfo: " << GetString( GetSize() ) << ", "
       << "driver: " << SDL_VideoDriverName( namebuf, 12 );

    return os.str();
#endif
}

/* hide system cursor */
void Display::HideCursor( void )
{
    SDL_ShowCursor( SDL_DISABLE );
}

/* show system cursor */
void Display::ShowCursor( void )
{
    SDL_ShowCursor( SDL_ENABLE );
}

/* get video display */
Display & Display::Get( void )
{
    static Display inside;
    return inside;
}

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
Texture::Texture()
    : texture( NULL )
    , counter( NULL )
{
    counter = new int;
    *counter = 0;
}

Texture::Texture( const Surface & sf )
    : texture( NULL )
    , counter( NULL )
{
    Display & display = Display::Get();
    texture = SDL_CreateTextureFromSurface( display.renderer, sf() );

    if ( !texture )
        ERROR( SDL_GetError() );

    counter = new int;
    *counter = 1;
}

Texture::~Texture()
{
    if ( 1 < *counter )
        *counter -= 1;
    else {
        SDL_DestroyTexture( texture );
        delete counter;
    }
}

Texture::Texture( const Texture & tx )
    : texture( tx.texture )
    , counter( tx.counter )
{
    if ( texture )
        *counter += 1;
    else {
        counter = new int;
        *counter = 0;
    }
}

Texture & Texture::operator=( const Texture & tx )
{
    if ( this == &tx )
        return *this;

    if ( 1 < *counter )
        *counter -= 1;
    else {
        SDL_DestroyTexture( texture );
        delete counter;
    }

    texture = tx.texture;
    counter = tx.counter;

    if ( texture )
        *counter += 1;
    else {
        counter = new int;
        *counter = 0;
    }

    return *this;
}

Size Texture::GetSize( void ) const
{
    int tw, th;
    SDL_QueryTexture( texture, NULL, NULL, &tw, &th );
    return Size( tw, th );
}

void Texture::Blit( Display & display ) const
{
    Blit( Rect( Point( 0, 0 ), GetSize() ), Point( 0, 0 ), display );
}

void Texture::Blit( s32 dx, s32 dy, Display & display ) const
{
    Blit( Rect( Point( 0, 0 ), GetSize() ), Point( dx, dy ), display );
}

void Texture::Blit( const Point & dstpt, Display & display ) const
{
    Blit( Rect( Point( 0, 0 ), GetSize() ), dstpt, display );
}

void Texture::Blit( const Rect & srcrt, s32 dx, s32 dy, Display & display ) const
{
    Blit( srcrt, Point( dx, dy ), display );
}

void Texture::Blit( const Rect & srt, const Point & dpt, Display & display ) const
{
    SDL_Rect srcrt = SDLRect( srt );
    SDL_Rect dstrt = SDLRect( dpt.x, dpt.y, srt.w, srt.h );

    if ( 0 != SDL_SetRenderTarget( display.renderer, NULL ) ) {
        ERROR( SDL_GetError() );
    }
    else {
        if ( 0 != SDL_RenderCopy( display.renderer, texture, &srcrt, &dstrt ) )
            ERROR( SDL_GetError() );
    }
}

#else
Texture::Texture( const Surface & sf )
{
    Set( SDL_DisplayFormatAlpha( sf() ) );
    // Set(SDL_DisplayFormat(sf()));
}
#endif
