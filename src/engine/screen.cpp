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
#include "../../tools/palette_h2.h"

#include <SDL_version.h>
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#include <SDL_render.h>
#include <SDL_video.h>
#else
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
        if ( !resolutionSet.empty() )
            return std::vector<std::pair<int, int> >();

        std::vector<std::pair<int, int> > resolutions( resolutionSet.begin(), resolutionSet.end() );
        std::sort( resolutions.begin(), resolutions.end(), SortResolutions );

        if ( resolutions.front().first >= fheroes2::Display::DEFAULT_WIDTH && resolutions.front().first >= fheroes2::Display::DEFAULT_HEIGHT ) {
            resolutions.erase( std::remove_if( resolutions.begin(), resolutions.end(), IsLowerThanDefaultRes ), resolutions.end() );
        }

        return resolutions;
    }
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

        virtual void toggleFullScreen()
        {
            if ( _window != NULL ) {
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
        }

        virtual bool isFullScreen() const
        {
            if ( _window == NULL )
                return false;

            const uint32_t flags = SDL_GetWindowFlags( _window );
            return ( flags & SDL_WINDOW_FULLSCREEN ) != 0 || ( flags & SDL_WINDOW_FULLSCREEN_DESKTOP ) != 0;
        }

        virtual std::vector<std::pair<int, int> > getAvailableResolutions() const
        {
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

            return FilterResolutions( resolutionSet );
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
        {}

        virtual void clear()
        {
            if ( _window != NULL ) {
                SDL_DestroyWindow( _window );
                _window = NULL;
            }

            if ( _renderer != NULL ) {
                _renderer = NULL;
                SDL_DestroyRenderer( _renderer );
            }

            if ( _texture != NULL ) {
                SDL_DestroyTexture( _texture );
                _texture = NULL;
            }

            if ( _surface != NULL ) {
                SDL_FreeSurface( _surface );
                _surface = NULL;
            }
        }

        virtual void render( const fheroes2::Display & display )
        {
            if ( _surface == NULL )
                return;

            const uint32_t width = display.width();
            const uint32_t height = display.height();

            if ( SDL_MUSTLOCK( _surface ) )
                SDL_LockSurface( _surface );

            if ( _surface->format->BitsPerPixel == 32 ) {
                const uint16_t pitch = _surface->pitch >> 2;
                uint32_t * out = static_cast<uint32_t *>( _surface->pixels );
                uint32_t * outEnd = out + width * height;
                const uint8_t * in = display.image();
                const uint32_t * transform = _palette32Bit.data();

                for ( ; out != outEnd; ++out, ++in )
                    *out = *( transform + *in );
            }
            else if ( _surface->format->BitsPerPixel == 8 ) {
                if ( _surface->pixels != display.image() ) {
                    memcpy( _surface->pixels, display.image(), width * height );
                }
            }

            if ( SDL_MUSTLOCK( _surface ) )
                SDL_UnlockSurface( _surface );

            if ( SDL_UpdateTexture( _texture, NULL, _surface->pixels, _surface->pitch ) == 0 ) {
                if ( SDL_SetRenderTarget( _renderer, NULL ) == 0 ) {
                    if ( SDL_RenderCopy( _renderer, _texture, NULL, NULL ) == 0 ) {
                        SDL_RenderPresent( _renderer );
                    }
                }
            }
        }

        virtual bool allocate( uint32_t width_, uint32_t height_, bool isFullScreen )
        {
            std::string previousWindowTitle;
            int prevX = SDL_WINDOWPOS_CENTERED;
            int prevY = SDL_WINDOWPOS_CENTERED;
            if ( _window ) {
                previousWindowTitle = SDL_GetWindowTitle( _window );
                SDL_GetWindowPosition( _window, &prevX, &prevY );
            }

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

            _window = SDL_CreateWindow( "", prevX, prevY, width_, height_, flags );
            if ( _window == NULL ) {
                clear();
                return false;
            }

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

            if ( _surface->w <= 0 || _surface->h <= 0 || static_cast<uint32_t>( _surface->w ) != width_ || static_cast<uint32_t>( _surface->h ) != height_ ) {
                clear();
                return false;
            }

            _createPalette();

            _texture = SDL_CreateTextureFromSurface( _renderer, _surface );
            if ( _texture == NULL ) {
                clear();
                return false;
            }

            return true;
        }

    private:
        SDL_Window * _window;
        SDL_Surface * _surface;
        SDL_Renderer * _renderer;
        SDL_Texture * _texture;

        std::vector<uint32_t> _palette32Bit;
        std::vector<SDL_Color> _palette8Bit;

        int renderFlags() const
        {
#if defined( __MINGW32CE__ ) || defined( __SYMBIAN32__ )
            return SDL_RENDERER_SOFTWARE;
#elif defined( __WIN32__ ) || defined( ANDROID )
            return SDL_RENDERER_ACCELERATED;
#else
            return SDL_RENDERER_ACCELERATED;
#endif
        }

        void _createPalette()
        {
            if ( _surface == NULL )
                return;

            if ( _surface->format->BitsPerPixel == 32 ) {
                _palette32Bit.resize( 256u );

                if ( _surface->format->Amask > 0 ) {
                    for ( size_t i = 0; i < 256u; ++i ) {
                        _palette32Bit[i] = SDL_MapRGBA( _surface->format, kb_pal[i * 3] << 2, kb_pal[i * 3 + 1] << 2, kb_pal[i * 3 + 2] << 2, 255 );
                    }
                }
                else {
                    for ( size_t i = 0; i < 256u; ++i ) {
                        _palette32Bit[i] = SDL_MapRGB( _surface->format, kb_pal[i * 3] << 2, kb_pal[i * 3 + 1] << 2, kb_pal[i * 3 + 2] << 2 );
                    }
                }
            }
            else if ( _surface->format->BitsPerPixel == 8 ) {
                _palette8Bit.resize( 256 );
                for ( uint32_t i = 0; i < 256; ++i ) {
                    const uint32_t index = i * 3;
                    SDL_Color & col = _palette8Bit[i];

                    col.r = kb_pal[index] << 2;
                    col.g = kb_pal[index + 1] << 2;
                    col.b = kb_pal[index + 2] << 2;
                }

                SDL_SetPaletteColors( _surface->format->palette, _palette8Bit.data(), 0, 256 );

                if ( !SDL_MUSTLOCK( _surface ) ) {
                    // copy the image from display buffer to SDL surface
                    fheroes2::Display & display = fheroes2::Display::instance();
                    memcpy( _surface->pixels, display.image(), display.width() * display.height() );

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

        virtual void toggleFullScreen()
        {
            if ( _surface == NULL ) // nothing to render
                return;

            const uint32_t flags = _surface->flags;
            clear();

            _surface = SDL_SetVideoMode( 0, 0, 8, flags ^ SDL_FULLSCREEN );
            if ( _surface == NULL ) {
                _surface = SDL_SetVideoMode( 0, 0, 8, flags );
            }

            _createPalette();
        }

        virtual bool isFullScreen() const
        {
            if ( _surface == NULL )
                return false;

            return ( ( _surface->flags & SDL_FULLSCREEN ) != 0 );
        }

        virtual std::vector<std::pair<int, int> > getAvailableResolutions() const
        {
            std::set<std::pair<int, int> > resolutionSet;

            SDL_Rect ** modes = SDL_ListModes( NULL, SDL_FULLSCREEN | SDL_HWSURFACE );
            if ( modes != NULL && modes != reinterpret_cast<SDL_Rect **>( -1 ) ) {
                for ( int i = 0; modes[i]; ++i ) {
                    resolutionSet.insert( std::make_pair( modes[i]->w, modes[i]->h ) );
                }
            }

            return FilterResolutions( resolutionSet );
        }

        static RenderEngine * create()
        {
            return new RenderEngine;
        }

    protected:
        RenderEngine()
            : _surface( NULL )
        {}

        virtual void render( const fheroes2::Display & display )
        {
            if ( _surface == NULL ) // nothing to render on
                return;

            const uint32_t width = display.width();
            const uint32_t height = display.height();

            if ( SDL_MUSTLOCK( _surface ) )
                SDL_LockSurface( _surface );

            if ( _surface->format->BitsPerPixel == 32 ) {
                const uint16_t pitch = _surface->pitch >> 2;
                uint32_t * out = static_cast<uint32_t *>( _surface->pixels );
                uint32_t * outEnd = out + width * height;
                const uint8_t * in = display.image();
                const uint32_t * transform = _palette32Bit.data();

                for ( ; out != outEnd; ++out, ++in )
                    *out = *( transform + *in );
            }
            else if ( _surface->format->BitsPerPixel == 8 ) {
                if ( _surface->pixels != display.image() ) {
                    memcpy( _surface->pixels, display.image(), width * height );
                }
            }

            if ( SDL_MUSTLOCK( _surface ) )
                SDL_UnlockSurface( _surface );

            SDL_Flip( _surface );
        }

        virtual void clear()
        {
            linkRenderSurface( NULL );

            if ( _surface != NULL ) {
                // we need to copy the image to display buffer
                if ( _surface->format->BitsPerPixel == 8 && !SDL_MUSTLOCK( _surface ) ) {
                    fheroes2::Display & display = fheroes2::Display::instance();
                    memcpy( display.image(), _surface->pixels, display.width() * display.height() );
                }

                SDL_FreeSurface( _surface );
                _surface = NULL;
            }

            _palette32Bit.clear();
            _palette8Bit.clear();
        }

        virtual bool allocate( uint32_t width_, uint32_t height_, bool isFullScreen )
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

            _surface = SDL_SetVideoMode( width_, height_, 8, flags );

            if ( !_surface )
                return false;

            if ( _surface->w <= 0 || _surface->h <= 0 || static_cast<uint32_t>( _surface->w ) != width_ || static_cast<uint32_t>( _surface->h ) != height_ ) {
                clear();
                return false;
            }

            _createPalette();

            return true;
        }

    private:
        SDL_Surface * _surface;
        std::vector<uint32_t> _palette32Bit;
        std::vector<SDL_Color> _palette8Bit;

        int renderFlags() const
        {
#if defined( __MINGW32CE__ ) || defined( __SYMBIAN32__ )
            return SDL_SWSURFACE;
#elif defined( __WIN32__ ) || defined( ANDROID )
            return SDL_HWSURFACE | SDL_HWPALETTE;
#else
            return SDL_SWSURFACE;
#endif
        }

        void _createPalette()
        {
            if ( _surface == NULL )
                return;

            if ( _surface->format->BitsPerPixel == 32 ) {
                _palette32Bit.resize( 256u );

                if ( _surface->format->Amask > 0 ) {
                    for ( size_t i = 0; i < 256u; ++i ) {
                        _palette32Bit[i] = SDL_MapRGBA( _surface->format, kb_pal[i * 3] << 2, kb_pal[i * 3 + 1] << 2, kb_pal[i * 3 + 2] << 2, 255 );
                    }
                }
                else {
                    for ( size_t i = 0; i < 256u; ++i ) {
                        _palette32Bit[i] = SDL_MapRGB( _surface->format, kb_pal[i * 3] << 2, kb_pal[i * 3 + 1] << 2, kb_pal[i * 3 + 2] << 2 );
                    }
                }
            }
            else if ( _surface->format->BitsPerPixel == 8 ) {
                _palette8Bit.resize( 256 );
                for ( uint32_t i = 0; i < 256; ++i ) {
                    const uint32_t index = i * 3;
                    SDL_Color & col = _palette8Bit[i];

                    col.r = kb_pal[index] << 2;
                    col.g = kb_pal[index + 1] << 2;
                    col.b = kb_pal[index + 2] << 2;
                }

                SDL_SetPalette( _surface, SDL_LOGPAL | SDL_PHYSPAL, _palette8Bit.data(), 0, 256 );

                if ( !SDL_MUSTLOCK( _surface ) ) {
                    // copy the image from display buffer to SDL surface
                    fheroes2::Display & display = fheroes2::Display::instance();
                    memcpy( _surface->pixels, display.image(), display.width() * display.height() );

                    linkRenderSurface( static_cast<uint8_t *>( _surface->pixels ) );
                }
            }
        }
    };
#endif
}

namespace fheroes2
{
    void BaseRenderEngine::linkRenderSurface( uint8_t * surface )
    {
        Display::instance().linkRenderSurface( surface );
    }

    Display::Display()
        : _engine( RenderEngine::create() )
        , _preprocessing( NULL )
        , _renderSurface( NULL )
    {}

    Display::~Display()
    {
        delete _engine;
    }

    void Display::resize( uint32_t width_, uint32_t height_ )
    {
        if ( width_ == 0 || height_ == 0 || ( width_ == width() && height_ == height() ) ) // nothing to resize
            return;

        // deallocate engine resources
        if ( !empty() ) {
            _engine->clear();
        }

        Image::resize( width_, height_ );

        // allocate engine resources
        if ( !_engine->allocate( width_, height_, false ) ) {
            clear();
        }
    }

    Display & Display::instance()
    {
        static Display display;
        return display;
    }

    void Display::render()
    {
        if ( _preprocessing != NULL ) {
            _preprocessing( *this );
        }

        Cursor & cursor = Cursor::instance();
        if ( cursor.isVisible() ) {
            const Sprite backup = Crop( *this, cursor.x(), cursor.y(), cursor.width(), cursor.height() );
            Blit( cursor, *this, cursor.x(), cursor.y() );

            _engine->render( *this );

            Blit( backup, *this, backup.x(), backup.y() );
        }
        else {
            _engine->render( *this );
        }
    }

    void Display::subscribe( PreRenderProcessing preprocessing )
    {
        _preprocessing = preprocessing;
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

    BaseRenderEngine * Display::engine()
    {
        return _engine;
    }

    Cursor::Cursor()
        : _show( true )
    {}

    Cursor::~Cursor() {}

    Cursor & Cursor::instance()
    {
        static Cursor cursor;
        return cursor;
    }

    void Cursor::show( bool enable )
    {
        _show = enable;
    }

    bool Cursor::isVisible() const
    {
        return _show;
    }

    BaseRenderEngine & engine()
    {
        return *( Display::instance().engine() );
    }
}
