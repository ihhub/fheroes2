/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "screen.h"
#include "palette_h2.h"

#include <SDL_version.h>
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#include <SDL_hints.h>
#include <SDL_mouse.h>
#include <SDL_render.h>
#include <SDL_video.h>
#else
#include <SDL_active.h>
#include <SDL_video.h>
#endif

#include <algorithm>
#include <cmath>
#include <set>

namespace
{
    // Returns nearest screen supported resolution
    std::pair<int, int> GetNearestResolution( int width, int height, const std::vector<std::pair<int, int> > & resolutions )
    {
        if ( resolutions.empty() )
            return std::make_pair( width, height );

        if ( width < 1 )
            width = 1;
        if ( height < 1 )
            height = 1;

        const double x = width;
        const double y = height;

        std::vector<double> similarity( resolutions.size(), 0 );
        for ( size_t i = 0; i < resolutions.size(); ++i ) {
            similarity[i] = std::fabs( resolutions[i].first - x ) / x + std::fabs( resolutions[i].second - y ) / y;
        }

        const std::vector<double>::difference_type id = std::distance( similarity.begin(), std::min_element( similarity.begin(), similarity.end() ) );

        return resolutions[id];
    }

    bool SortResolutions( const std::pair<int, int> & first, const std::pair<int, int> & second )
    {
        return first.first > second.first || ( first.first == second.first && first.second >= second.second );
    }

    bool IsLowerThanDefaultRes( const std::pair<int, int> & value )
    {
        return value.first < fheroes2::Display::DEFAULT_WIDTH || value.second < fheroes2::Display::DEFAULT_HEIGHT;
    }

    std::vector<std::pair<int, int> > FilterResolutions( const std::set<std::pair<int, int> > & resolutionSet )
    {
        if ( resolutionSet.empty() )
            return std::vector<std::pair<int, int> >();

        std::vector<std::pair<int, int> > resolutions( resolutionSet.begin(), resolutionSet.end() );
        std::sort( resolutions.begin(), resolutions.end(), SortResolutions );

        if ( resolutions.front().first >= fheroes2::Display::DEFAULT_WIDTH && resolutions.front().first >= fheroes2::Display::DEFAULT_HEIGHT ) {
            resolutions.erase( std::remove_if( resolutions.begin(), resolutions.end(), IsLowerThanDefaultRes ), resolutions.end() );
        }

        return resolutions;
    }

    std::vector<uint8_t> StandardPaletteIndexes()
    {
        std::vector<uint8_t> indexes( 256 );
        for ( uint32_t i = 0; i < 256; ++i ) {
            indexes[i] = static_cast<uint8_t>( i );
        }
        return indexes;
    }

    const uint8_t * PALPAlette()
    {
        static std::vector<uint8_t> palette;
        if ( palette.empty() ) {
            palette.resize( 256 * 3 );
            for ( size_t i = 0; i < palette.size(); ++i ) {
                palette[i] = kb_pal[i] << 2;
            }
        }

        return palette.data();
    }

    const uint8_t * currentPalette = PALPAlette();
}

namespace
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    class RenderCursor : public fheroes2::Cursor
    {
    public:
        virtual ~RenderCursor()
        {
            clear();
        }

        virtual bool isVisible() const override
        {
            if ( _emulation )
                return fheroes2::Cursor::isVisible();
            else
                return fheroes2::Cursor::isVisible() && ( SDL_ShowCursor( -1 ) == 1 );
        }

        virtual void update( const fheroes2::Image & image, int32_t offsetX, int32_t offsetY ) override
        {
            if ( _emulation ) {
                fheroes2::Cursor::update( image, offsetX, offsetY );
                return;
            }

            SDL_Surface * surface = SDL_CreateRGBSurface( 0, image.width(), image.height(), 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000 );
            if ( surface == NULL )
                return;

            const uint32_t width = image.width();
            const uint32_t height = image.height();

            uint32_t * out = static_cast<uint32_t *>( surface->pixels );
            const uint32_t * outEnd = out + width * height;
            const uint8_t * in = image.image();
            const uint8_t * transform = image.transform();

            if ( surface->format->Amask > 0 ) {
                for ( ; out != outEnd; ++out, ++in, ++transform ) {
                    if ( *transform == 0 ) {
                        const uint8_t * value = currentPalette + *in * 3;
                        *out = SDL_MapRGBA( surface->format, *( value ), *( value + 1 ), *( value + 2 ), 255 );
                    }
                }
            }
            else {
                for ( ; out != outEnd; ++out, ++in, ++transform ) {
                    if ( *transform == 0 ) {
                        const uint8_t * value = currentPalette + *in * 3;
                        *out = SDL_MapRGB( surface->format, *( value ), *( value + 1 ), *( value + 2 ) );
                    }
                    else {
                        *out = SDL_MapRGB( surface->format, 0, 0, 0 );
                    }
                }
            }

            SDL_Cursor * tempCursor = SDL_CreateColorCursor( surface, offsetX, offsetY );
            SDL_SetCursor( tempCursor );
            SDL_ShowCursor( 1 );

            clear();
            std::swap( _cursor, tempCursor );
        }

        virtual void enableSoftwareEmulation( const bool enable ) override
        {
            if ( enable == _emulation )
                return;

            if ( enable ) {
                clear();
                _emulation = true;
            }
            else {
                _emulation = false;
                update( _image, _image.x(), _image.y() );
            }
        }

        static RenderCursor * create()
        {
            return new RenderCursor;
        }

    protected:
        RenderCursor()
            : _cursor( NULL )
        {
            // SDL 2 handles mouse properly without any emulation.
            _emulation = false;
        }

    private:
        SDL_Cursor * _cursor;

        void clear()
        {
            if ( _cursor != NULL ) {
                SDL_FreeCursor( _cursor );
                _cursor = NULL;
            }
        }
    };
#else
    // SDL 1 doesn't support hardware level cursor.
    class RenderCursor : public fheroes2::Cursor
    {
    public:
        static RenderCursor * create()
        {
            return new RenderCursor;
        }
    };
#endif
}

namespace
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    class RenderEngine : public fheroes2::BaseRenderEngine
    {
    public:
        virtual ~RenderEngine()
        {
            clear();
        }

        virtual void toggleFullScreen() override
        {
            if ( _window == NULL ) {
                BaseRenderEngine::toggleFullScreen();
                return;
            }

            uint32_t flags = SDL_GetWindowFlags( _window );
            if ( ( flags & SDL_WINDOW_FULLSCREEN ) == SDL_WINDOW_FULLSCREEN || ( flags & SDL_WINDOW_FULLSCREEN_DESKTOP ) == SDL_WINDOW_FULLSCREEN_DESKTOP ) {
                flags = 0;
            }
            else {
#if defined( __WIN32__ )
                flags = SDL_WINDOW_FULLSCREEN;
#else
                flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
            }

            SDL_SetWindowFullscreen( _window, flags );
        }

        virtual bool isFullScreen() const override
        {
            if ( _window == NULL )
                return BaseRenderEngine::isFullScreen();

            const uint32_t flags = SDL_GetWindowFlags( _window );
            return ( flags & SDL_WINDOW_FULLSCREEN ) != 0 || ( flags & SDL_WINDOW_FULLSCREEN_DESKTOP ) != 0;
        }

        virtual std::vector<std::pair<int, int> > getAvailableResolutions() const override
        {
            static std::vector<std::pair<int, int> > filteredResolutions;

            if ( filteredResolutions.empty() ) {
                std::set<std::pair<int, int> > resolutionSet;

                const int displayCount = SDL_GetNumVideoDisplays();
                if ( displayCount > 0 ) {
                    const int displayModeCount = SDL_GetNumDisplayModes( 0 );
                    for ( int i = 0; i < displayModeCount; ++i ) {
                        SDL_DisplayMode videoMode;
                        if ( SDL_GetDisplayMode( 0, i, &videoMode ) == 0 ) {
                            resolutionSet.insert( std::make_pair( videoMode.w, videoMode.h ) );
                        }
                    }
                }

                filteredResolutions = FilterResolutions( resolutionSet );
            }

            return filteredResolutions;
        }

        virtual void setTitle( const std::string & title ) override
        {
            if ( _window != NULL )
                SDL_SetWindowTitle( _window, title.c_str() );
        }

        virtual void setIcon( const fheroes2::Image & icon ) override
        {
            if ( _window == NULL )
                return;

            SDL_Surface * surface = SDL_CreateRGBSurface( 0, icon.width(), icon.height(), 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000 );
            if ( surface == NULL )
                return;

            const uint32_t width = icon.width();
            const uint32_t height = icon.height();

            uint32_t * out = static_cast<uint32_t *>( surface->pixels );
            const uint32_t * outEnd = out + width * height;
            const uint8_t * in = icon.image();
            const uint8_t * transform = icon.transform();

            if ( surface->format->Amask > 0 ) {
                for ( ; out != outEnd; ++out, ++in, ++transform ) {
                    if ( *transform == 0 ) {
                        const uint8_t * value = currentPalette + *in * 3;
                        *out = SDL_MapRGBA( surface->format, *( value ), *( value + 1 ), *( value + 2 ), 255 );
                    }
                }
            }
            else {
                for ( ; out != outEnd; ++out, ++in, ++transform ) {
                    if ( *transform == 0 ) {
                        const uint8_t * value = currentPalette + *in * 3;
                        *out = SDL_MapRGB( surface->format, *( value ), *( value + 1 ), *( value + 2 ) );
                    }
                    else {
                        *out = SDL_MapRGB( surface->format, 0, 0, 0 );
                    }
                }
            }

            SDL_SetWindowIcon( _window, surface );

            SDL_FreeSurface( surface );
        }

        static RenderEngine * create()
        {
            return new RenderEngine;
        }

    protected:
        RenderEngine()
            : _window( NULL )
            , _surface( NULL )
            , _renderer( NULL )
            , _texture( NULL )
            , _prevWindowPos( SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED )
        {}

        virtual void clear() override
        {
            if ( _texture != NULL ) {
                SDL_DestroyTexture( _texture );
                _texture = NULL;
            }

            if ( _renderer != NULL ) {
                SDL_DestroyRenderer( _renderer );
                _renderer = NULL;
            }

            if ( _window != NULL ) {
                // Let's collect needed info about previous setup
                if ( !isFullScreen() ) {
                    SDL_GetWindowPosition( _window, &_prevWindowPos.x, &_prevWindowPos.y );
                }
                _previousWindowTitle = SDL_GetWindowTitle( _window );

                SDL_DestroyWindow( _window );
                _window = NULL;
            }

            if ( _surface != NULL ) {
                SDL_FreeSurface( _surface );
                _surface = NULL;
            }
        }

        virtual void render( const fheroes2::Display & display ) override
        {
            if ( _surface == NULL )
                return;

            const int32_t width = display.width();
            const int32_t height = display.height();

            if ( SDL_MUSTLOCK( _surface ) )
                SDL_LockSurface( _surface );

            if ( _surface->format->BitsPerPixel == 32 ) {
                uint32_t * out = static_cast<uint32_t *>( _surface->pixels );
                const uint32_t * outEnd = out + width * height;
                const uint8_t * in = display.image();
                const uint32_t * transform = _palette32Bit.data();

                for ( ; out != outEnd; ++out, ++in )
                    *out = *( transform + *in );
            }
            else if ( _surface->format->BitsPerPixel == 8 ) {
                if ( _surface->pixels != display.image() ) {
                    memcpy( _surface->pixels, display.image(), static_cast<size_t>( width * height ) );
                }
            }

            if ( SDL_MUSTLOCK( _surface ) )
                SDL_UnlockSurface( _surface );

            if ( _texture == NULL ) {
                if ( _renderer != NULL )
                    SDL_DestroyRenderer( _renderer );

                _renderer = SDL_CreateRenderer( _window, -1, renderFlags() );
            }
            else {
                SDL_UpdateTexture( _texture, NULL, _surface->pixels, _surface->pitch );
                if ( SDL_SetRenderTarget( _renderer, NULL ) == 0 ) {
                    if ( SDL_RenderClear( _renderer ) == 0 && SDL_RenderCopy( _renderer, _texture, NULL, NULL ) == 0 ) {
                        SDL_RenderPresent( _renderer );
                    }
                }
            }
        }

        virtual bool allocate( int32_t & width_, int32_t & height_, bool isFullScreen ) override
        {
            clear();

            const std::vector<std::pair<int, int> > resolutions = getAvailableResolutions();
            if ( !resolutions.empty() ) {
                const std::pair<int, int> correctResolution = GetNearestResolution( width_, height_, resolutions );
                width_ = correctResolution.first;
                height_ = correctResolution.second;
            }

            uint32_t flags = SDL_WINDOW_SHOWN;
            if ( isFullScreen ) {
#if defined( __WIN32__ )
                flags |= SDL_WINDOW_FULLSCREEN;
#else
                flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
            }

            _window = SDL_CreateWindow( "", _prevWindowPos.x, _prevWindowPos.y, width_, height_, flags );
            if ( _window == NULL ) {
                clear();
                return false;
            }

            SDL_SetWindowTitle( _window, _previousWindowTitle.data() );

            _renderer = SDL_CreateRenderer( _window, -1, renderFlags() );
            if ( _renderer == NULL ) {
                clear();
                return false;
            }

            _surface = SDL_CreateRGBSurface( 0, width_, height_, 32, 0, 0, 0, 0 );
            if ( _surface == NULL ) {
                clear();
                return false;
            }

            if ( _surface->w <= 0 || _surface->h <= 0 || _surface->w != width_ || _surface->h != height_ ) {
                clear();
                return false;
            }

            _createPalette();
            SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "linear" );
            if ( SDL_RenderSetLogicalSize( _renderer, width_, height_ ) ) {
                clear();
                return false;
            }
            _texture = SDL_CreateTextureFromSurface( _renderer, _surface );
            if ( _texture == NULL ) {
                clear();
                return false;
            }

            return true;
        }

        virtual void updatePalette( const std::vector<uint8_t> & colorIds ) override
        {
            if ( _surface == NULL || colorIds.size() != 256 )
                return;

            if ( _surface->format->BitsPerPixel == 32 ) {
                _palette32Bit.resize( 256u );

                if ( _surface->format->Amask > 0 ) {
                    for ( size_t i = 0; i < 256u; ++i ) {
                        const uint8_t * value = currentPalette + colorIds[i] * 3;
                        _palette32Bit[i] = SDL_MapRGBA( _surface->format, *( value ), *( value + 1 ), *( value + 2 ), 255 );
                    }
                }
                else {
                    for ( size_t i = 0; i < 256u; ++i ) {
                        const uint8_t * value = currentPalette + colorIds[i] * 3;
                        _palette32Bit[i] = SDL_MapRGB( _surface->format, *( value ), *( value + 1 ), *( value + 2 ) );
                    }
                }
            }
            else if ( _surface->format->BitsPerPixel == 8 ) {
                _palette8Bit.resize( 256 );
                for ( uint32_t i = 0; i < 256; ++i ) {
                    const uint8_t * value = currentPalette + colorIds[i] * 3;
                    SDL_Color & col = _palette8Bit[i];

                    col.r = *( value );
                    col.g = *( value + 1 );
                    col.b = *( value + 2 );
                }

                SDL_SetPaletteColors( _surface->format->palette, _palette8Bit.data(), 0, 256 );
            }
        }

        virtual bool isMouseCursorActive() const override
        {
            return ( _window != NULL ) && ( ( SDL_GetWindowFlags( _window ) & SDL_WINDOW_MOUSE_FOCUS ) == SDL_WINDOW_MOUSE_FOCUS );
        }

    private:
        SDL_Window * _window;
        SDL_Surface * _surface;
        SDL_Renderer * _renderer;
        SDL_Texture * _texture;

        std::vector<uint32_t> _palette32Bit;
        std::vector<SDL_Color> _palette8Bit;

        std::string _previousWindowTitle;
        fheroes2::Point _prevWindowPos;

        int renderFlags() const
        {
            return SDL_RENDERER_ACCELERATED;
        }

        void _createPalette()
        {
            if ( _surface == NULL )
                return;

            updatePalette( StandardPaletteIndexes() );

            if ( _surface->format->BitsPerPixel == 8 ) {
                if ( !SDL_MUSTLOCK( _surface ) ) {
                    // copy the image from display buffer to SDL surface
                    fheroes2::Display & display = fheroes2::Display::instance();
                    if ( _surface->w == display.width() && _surface->h == display.height() ) {
                        memcpy( _surface->pixels, display.image(), static_cast<size_t>( display.width() * display.height() ) );
                    }

                    linkRenderSurface( static_cast<uint8_t *>( _surface->pixels ) );
                }
            }
        }
    };
#else
    class RenderEngine : public fheroes2::BaseRenderEngine
    {
    public:
        virtual ~RenderEngine()
        {
            clear();
        }

        virtual void toggleFullScreen() override
        {
            if ( _surface == NULL ) { // nothing to render
                BaseRenderEngine::toggleFullScreen();
                return;
            }

            fheroes2::Display & display = fheroes2::Display::instance();
            if ( _surface->format->BitsPerPixel == 8 && _surface->pixels == display.image() ) {
                if ( display.width() == _surface->w && display.height() == _surface->h ) {
                    linkRenderSurface( NULL );
                    memcpy( display.image(), _surface->pixels, static_cast<size_t>( display.width() * display.height() ) );
                }
            }

            const uint32_t flags = _surface->flags;
            clear();

            _surface = SDL_SetVideoMode( 0, 0, _bitDepth, flags ^ SDL_FULLSCREEN );
            if ( _surface == NULL ) {
                _surface = SDL_SetVideoMode( 0, 0, _bitDepth, flags );
            }

            _createPalette();
        }

        virtual bool isFullScreen() const override
        {
            if ( _surface == NULL )
                return BaseRenderEngine::isFullScreen();

            return ( ( _surface->flags & SDL_FULLSCREEN ) != 0 );
        }

        virtual std::vector<std::pair<int, int> > getAvailableResolutions() const override
        {
            static std::vector<std::pair<int, int> > filteredResolutions;

            if ( filteredResolutions.empty() ) {
                std::set<std::pair<int, int> > resolutionSet;
                SDL_Rect ** modes = SDL_ListModes( NULL, SDL_FULLSCREEN | SDL_HWSURFACE );
                if ( modes != NULL && modes != reinterpret_cast<SDL_Rect **>( -1 ) ) {
                    for ( int i = 0; modes[i]; ++i ) {
                        resolutionSet.insert( std::make_pair( modes[i]->w, modes[i]->h ) );
                    }
                }

                filteredResolutions = FilterResolutions( resolutionSet );
            }

            return filteredResolutions;
        }

        virtual void setTitle( const std::string & title ) override
        {
            SDL_WM_SetCaption( title.c_str(), NULL );
        }

        virtual void setIcon( const fheroes2::Image & icon ) override
        {
            SDL_Surface * surface = SDL_CreateRGBSurface( 0, icon.width(), icon.height(), 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000 );
            if ( surface == NULL )
                return;

            const uint32_t width = icon.width();
            const uint32_t height = icon.height();

            uint32_t * out = static_cast<uint32_t *>( surface->pixels );
            const uint32_t * outEnd = out + width * height;
            const uint8_t * in = icon.image();
            const uint8_t * transform = icon.transform();

            if ( surface->format->Amask > 0 ) {
                for ( ; out != outEnd; ++out, ++in, ++transform ) {
                    if ( *transform == 0 ) {
                        const uint8_t * value = currentPalette + *in * 3;
                        *out = SDL_MapRGBA( surface->format, *( value ), *( value + 1 ), *( value + 2 ), 255 );
                    }
                }
            }
            else {
                for ( ; out != outEnd; ++out, ++in, ++transform ) {
                    if ( *transform == 0 ) {
                        const uint8_t * value = currentPalette + *in * 3;
                        *out = SDL_MapRGB( surface->format, *( value ), *( value + 1 ), *( value + 2 ) );
                    }
                    else {
                        *out = SDL_MapRGB( surface->format, 0, 0, 0 );
                    }
                }
            }

            SDL_WM_SetIcon( surface, NULL );

            SDL_FreeSurface( surface );
        }

        static RenderEngine * create()
        {
            return new RenderEngine;
        }

    protected:
        RenderEngine()
            : _surface( NULL )
            , _bitDepth( 8 )
        {}

        virtual void render( const fheroes2::Display & display ) override
        {
            if ( _surface == NULL ) // nothing to render on
                return;

            const int32_t width = display.width();
            const int32_t height = display.height();

            if ( SDL_MUSTLOCK( _surface ) )
                SDL_LockSurface( _surface );

            if ( _surface->format->BitsPerPixel == 32 ) {
                uint32_t * out = static_cast<uint32_t *>( _surface->pixels );
                const uint32_t * outEnd = out + width * height;
                const uint8_t * in = display.image();
                const uint32_t * transform = _palette32Bit.data();

                for ( ; out != outEnd; ++out, ++in )
                    *out = *( transform + *in );
            }
            else if ( _surface->format->BitsPerPixel == 8 ) {
                if ( _surface->pixels != display.image() ) {
                    memcpy( _surface->pixels, display.image(), static_cast<size_t>( width * height ) );
                }
            }

            if ( SDL_MUSTLOCK( _surface ) )
                SDL_UnlockSurface( _surface );

            SDL_Flip( _surface );
        }

        virtual void clear() override
        {
            linkRenderSurface( NULL );

            if ( _surface != NULL ) {
                SDL_FreeSurface( _surface );
                _surface = NULL;
            }

            _palette32Bit.clear();
            _palette8Bit.clear();
        }

        virtual bool allocate( int32_t & width_, int32_t & height_, bool isFullScreen ) override
        {
            clear();

            const std::vector<std::pair<int, int> > resolutions = getAvailableResolutions();
            if ( !resolutions.empty() ) {
                const std::pair<int, int> correctResolution = GetNearestResolution( width_, height_, resolutions );
                width_ = correctResolution.first;
                height_ = correctResolution.second;
            }

            uint32_t flags = renderFlags();
            if ( isFullScreen )
                flags |= SDL_FULLSCREEN;

            _surface = SDL_SetVideoMode( width_, height_, _bitDepth, flags );

            if ( _surface == NULL )
                return false;

            if ( _surface->w <= 0 || _surface->h <= 0 || _surface->w != width_ || _surface->h != height_ ) {
                clear();
                return false;
            }

            _createPalette();

            return true;
        }

        virtual void updatePalette( const std::vector<uint8_t> & colorIds ) override
        {
            if ( _surface == NULL || colorIds.size() != 256 )
                return;

            if ( _surface->format->BitsPerPixel == 32 ) {
                _palette32Bit.resize( 256u );

                if ( _surface->format->Amask > 0 ) {
                    for ( size_t i = 0; i < 256u; ++i ) {
                        const uint8_t * value = currentPalette + colorIds[i] * 3;
                        _palette32Bit[i] = SDL_MapRGBA( _surface->format, *( value ), *( value + 1 ), *( value + 2 ), 255 );
                    }
                }
                else {
                    for ( size_t i = 0; i < 256u; ++i ) {
                        const uint8_t * value = currentPalette + colorIds[i] * 3;
                        _palette32Bit[i] = SDL_MapRGB( _surface->format, *( value ), *( value + 1 ), *( value + 2 ) );
                    }
                }
            }
            else if ( _surface->format->BitsPerPixel == 8 ) {
                _palette8Bit.resize( 256 );
                for ( uint32_t i = 0; i < 256; ++i ) {
                    const uint8_t * value = currentPalette + colorIds[i] * 3;
                    SDL_Color & col = _palette8Bit[i];

                    col.r = *( value );
                    col.g = *( value + 1 );
                    col.b = *( value + 2 );
                }

                SDL_SetPalette( _surface, SDL_LOGPAL | SDL_PHYSPAL, _palette8Bit.data(), 0, 256 );
            }
        }

        virtual bool isMouseCursorActive() const override
        {
            return ( SDL_GetAppState() & SDL_APPMOUSEFOCUS ) == SDL_APPMOUSEFOCUS;
        }

    private:
        SDL_Surface * _surface;
        std::vector<uint32_t> _palette32Bit;
        std::vector<SDL_Color> _palette8Bit;
        int _bitDepth;

        int renderFlags() const
        {
#if defined( __WIN32__ ) || defined( ANDROID )
            return SDL_HWSURFACE | SDL_HWPALETTE;
#else
            return SDL_SWSURFACE;
#endif
        }

        void _createPalette()
        {
            if ( _surface == NULL )
                return;

            updatePalette( StandardPaletteIndexes() );

            if ( _surface->format->BitsPerPixel == 8 ) {
                if ( !SDL_MUSTLOCK( _surface ) ) {
                    // copy the image from display buffer to SDL surface
                    fheroes2::Display & display = fheroes2::Display::instance();
                    if ( _surface->w == display.width() && _surface->h == display.height() ) {
                        memcpy( _surface->pixels, display.image(), static_cast<size_t>( display.width() * display.height() ) );
                    }

                    linkRenderSurface( static_cast<uint8_t *>( _surface->pixels ) );
                }
            }
        }
    };
#endif
}

namespace fheroes2
{
    void BaseRenderEngine::linkRenderSurface( uint8_t * surface ) const
    {
        Display::instance().linkRenderSurface( surface );
    }

    Display::Display()
        : _engine( RenderEngine::create() )
        , _cursor( RenderCursor::create() )
        , _preprocessing( NULL )
        , _postprocessing( NULL )
        , _renderSurface( NULL )
    {}

    Display::~Display()
    {
        delete _cursor;
        delete _engine;
    }

    void Display::resize( int32_t width_, int32_t height_ )
    {
        if ( width() > 0 && height() > 0 && width_ == width() && height_ == height() ) // nothing to resize
            return;

        const bool isFullScreen = _engine->isFullScreen();

        // deallocate engine resources
        _engine->clear();

        // allocate engine resources
        if ( !_engine->allocate( width_, height_, isFullScreen ) ) {
            clear();
        }

        Image::resize( width_, height_ );
    }

    bool Display::isDefaultSize() const
    {
        return width() == DEFAULT_WIDTH && height() == DEFAULT_HEIGHT;
    }

    Display & Display::instance()
    {
        static Display display;
        return display;
    }

    void Display::render()
    {
        if ( _cursor->isVisible() && !_cursor->_image.empty() ) {
            const Sprite & cursorImage = _cursor->_image;
            const Sprite backup = Crop( *this, cursorImage.x(), cursorImage.y(), cursorImage.width(), cursorImage.height() );
            Blit( cursorImage, *this, cursorImage.x(), cursorImage.y() );

            _renderFrame();

            Blit( backup, *this, backup.x(), backup.y() );
        }
        else {
            _renderFrame();
        }

        if ( _postprocessing != NULL )
            _postprocessing();
    }

    void Display::_renderFrame()
    {
        bool updateImage = true;
        if ( _preprocessing != NULL ) {
            std::vector<uint8_t> palette;
            if ( _preprocessing( palette ) ) {
                _engine->updatePalette( palette );
                // when we change a palette for 8-bit image we unwillingly call render so we don't need to re-render the same frame again
                updateImage = ( _renderSurface == NULL );
            }
        }

        if ( updateImage ) {
            _engine->render( *this );
        }
    }

    void Display::subscribe( PreRenderProcessing preprocessing, PostRenderProcessing postprocessing )
    {
        _preprocessing = preprocessing;
        _postprocessing = postprocessing;
    }

    uint8_t * Display::image()
    {
        return _renderSurface != NULL ? _renderSurface : Image::image();
    }

    const uint8_t * Display::image() const
    {
        return _renderSurface != NULL ? _renderSurface : Image::image();
    }

    void Display::linkRenderSurface( uint8_t * surface )
    {
        _renderSurface = surface;
    }

    void Display::release()
    {
        _engine->clear();
        clear();
    }

    void Display::changePalette( const uint8_t * palette )
    {
        if ( currentPalette == palette || ( palette == NULL && currentPalette == PALPAlette() ) )
            return;

        currentPalette = ( palette == NULL ) ? PALPAlette() : palette;

        _engine->updatePalette( StandardPaletteIndexes() );
    }

    bool Cursor::isFocusActive() const
    {
        return engine().isMouseCursorActive();
    }

    BaseRenderEngine & engine()
    {
        return *( Display::instance()._engine );
    }

    Cursor & cursor()
    {
        return *( Display::instance()._cursor );
    }
}
