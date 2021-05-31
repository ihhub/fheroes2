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
#include <SDL_video.h>
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_mouse.h>
#include <SDL_render.h>
#else
#include <SDL_active.h>
#endif

#include <algorithm>
#include <cassert>
#include <cmath>
#include <set>
#include <type_traits>

#if defined( FHEROES2_VITA )
#include <vita2d.h>
#endif

namespace
{
    // Returns nearest screen supported resolution
    fheroes2::Size GetNearestResolution( int width, int height, const std::vector<fheroes2::Size> & resolutions )
    {
        if ( resolutions.empty() )
            return fheroes2::Size( width, height );

        if ( width < 1 )
            width = 1;
        if ( height < 1 )
            height = 1;

        const double x = width;
        const double y = height;

        std::vector<double> similarity( resolutions.size(), 0 );
        for ( size_t i = 0; i < resolutions.size(); ++i ) {
            similarity[i] = std::fabs( resolutions[i].width - x ) / x + std::fabs( resolutions[i].height - y ) / y;
        }

        const std::vector<double>::difference_type id = std::distance( similarity.begin(), std::min_element( similarity.begin(), similarity.end() ) );

        return resolutions[id];
    }

    bool SortResolutions( const fheroes2::Size & first, const fheroes2::Size & second )
    {
        return first.width > second.width || ( first.width == second.width && first.height >= second.height );
    }

    bool IsLowerThanDefaultRes( const fheroes2::Size & value )
    {
        return value.width < fheroes2::Display::DEFAULT_WIDTH || value.height < fheroes2::Display::DEFAULT_HEIGHT;
    }

    std::vector<fheroes2::Size> FilterResolutions( const std::set<fheroes2::Size> & resolutionSet )
    {
        if ( resolutionSet.empty() ) {
            const std::vector<fheroes2::Size> resolutions = {fheroes2::Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT )};
            return resolutions;
        }

        std::vector<fheroes2::Size> resolutions( resolutionSet.begin(), resolutionSet.end() );
        std::sort( resolutions.begin(), resolutions.end(), SortResolutions );

        // Remove all resolutions lower than the original.
        if ( resolutions.front().width >= fheroes2::Display::DEFAULT_WIDTH && resolutions.front().height >= fheroes2::Display::DEFAULT_HEIGHT ) {
            resolutions.erase( std::remove_if( resolutions.begin(), resolutions.end(), IsLowerThanDefaultRes ), resolutions.end() );
        }

        // If here is only one resolution and it is bigger than the original we failed to find any resolutions except the current.
        // In this case populate the list with missing resolutions.
        if ( resolutions.size() == 1 && resolutions.front().width > fheroes2::Display::DEFAULT_WIDTH && resolutions.front().height > fheroes2::Display::DEFAULT_HEIGHT ) {
            static_assert( fheroes2::Display::DEFAULT_WIDTH == 640 && fheroes2::Display::DEFAULT_HEIGHT == 480, "Default resolution must be 640 x 480" );
            const std::vector<fheroes2::Size> possibleResolutions
                = {fheroes2::Size( 640, 480 ), fheroes2::Size( 800, 600 ), fheroes2::Size( 1024, 768 ), fheroes2::Size( 1280, 960 ), fheroes2::Size( 1920, 1080 )};
            const fheroes2::Size & currentResolution = resolutions.front();
            for ( size_t i = 0; i < possibleResolutions.size(); ++i ) {
                if ( currentResolution.width <= possibleResolutions[i].width || currentResolution.height <= possibleResolutions[i].height ) {
                    break;
                }
                resolutions.emplace_back( possibleResolutions[i] );
            }

            std::sort( resolutions.begin(), resolutions.end(), SortResolutions );
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

    bool getActiveArea( fheroes2::Rect & roi, const int32_t width, const int32_t height )
    {
        if ( roi.width <= 0 || roi.height <= 0 || roi.x >= width || roi.y >= height )
            return false;

        if ( roi.x < 0 ) {
            const int32_t offsetX = -roi.x;
            if ( offsetX >= roi.width )
                return false;

            roi.x = 0;
            roi.width -= offsetX;
        }

        if ( roi.y < 0 ) {
            const int32_t offsetY = -roi.y;
            if ( offsetY >= roi.height )
                return false;

            roi.y = 0;
            roi.height -= offsetY;
        }

        if ( roi.x + roi.width > width ) {
            const int32_t offsetX = roi.x + roi.width - width;
            if ( offsetX >= roi.width )
                return false;
            roi.width -= offsetX;
        }

        if ( roi.y + roi.height > height ) {
            const int32_t offsetY = roi.y + roi.height - height;
            if ( offsetY >= roi.height )
                return false;
            roi.height -= offsetY;
        }

        return true;
    }

    fheroes2::Rect getCommonRoi( const fheroes2::Rect & roi1, const fheroes2::Rect & roi2 )
    {
        fheroes2::Rect common;
        common.x = roi1.x < roi2.x ? roi1.x : roi2.x;
        common.y = roi1.y < roi2.y ? roi1.y : roi2.y;
        common.width = roi1.x + roi1.width > roi2.x + roi2.width ? roi1.x + roi1.width - common.x : roi2.x + roi2.width - common.x;
        common.height = roi1.y + roi1.height > roi2.y + roi2.height ? roi1.y + roi1.height - common.y : roi2.y + roi2.height - common.y;

        return common;
    }

    const uint8_t * currentPalette = PALPAlette();

// If SDL library is used
#if !defined( FHEROES2_VITA )
    class BaseSDLRenderer
    {
    protected:
        std::vector<uint32_t> _palette32Bit;
        std::vector<SDL_Color> _palette8Bit;

        void copyImageToSurface( const fheroes2::Image & image, SDL_Surface * surface, const fheroes2::Rect & roi )
        {
            assert( surface != nullptr && !image.empty() );

            if ( SDL_MUSTLOCK( surface ) )
                SDL_LockSurface( surface );

            const int32_t imageWidth = image.width();
            const int32_t imageHeight = image.height();

            const bool fullFrame = ( roi.width == imageWidth ) && ( roi.height == imageHeight );

            if ( fullFrame ) {
                if ( surface->format->BitsPerPixel == 32 ) {
                    uint32_t * out = static_cast<uint32_t *>( surface->pixels );
                    const uint32_t * outEnd = out + imageWidth * imageHeight;
                    const uint8_t * in = image.image();
                    const uint32_t * transform = _palette32Bit.data();

                    for ( ; out != outEnd; ++out, ++in )
                        *out = *( transform + *in );
                }
                else if ( surface->format->BitsPerPixel == 8 ) {
                    if ( surface->pixels != image.image() ) {
                        if ( imageWidth % 4 != 0 ) {
                            const int32_t screenWidth = ( imageWidth / 4 ) * 4 + 4;
                            for ( int32_t i = 0; i < imageHeight; ++i ) {
                                memcpy( reinterpret_cast<int8_t *>( surface->pixels ) + screenWidth * i, image.image() + imageWidth * i,
                                        static_cast<size_t>( imageWidth ) );
                            }
                        }
                        else {
                            memcpy( surface->pixels, image.image(), static_cast<size_t>( imageWidth * imageHeight ) );
                        }
                    }
                }
            }
            else {
                if ( surface->format->BitsPerPixel == 32 ) {
                    uint32_t * outY = static_cast<uint32_t *>( surface->pixels );
                    const uint32_t * outYEnd = outY + imageWidth * roi.height;
                    const uint8_t * inY = image.image() + roi.x + roi.y * imageWidth;
                    const uint32_t * transform = _palette32Bit.data();

                    for ( ; outY != outYEnd; outY += imageWidth, inY += imageWidth ) {
                        uint32_t * outX = outY;
                        const uint32_t * outXEnd = outX + roi.width;
                        const uint8_t * inX = inY;

                        for ( ; outX != outXEnd; ++outX, ++inX )
                            *outX = *( transform + *inX );
                    }
                }
                else if ( surface->format->BitsPerPixel == 8 ) {
                    if ( surface->pixels != image.image() ) {
                        const int32_t screenWidth = ( imageWidth / 4 ) * 4 + 4;
                        const int32_t screenOffset = roi.x + roi.y * screenWidth;
                        const int32_t imageOffset = roi.x + roi.y * imageWidth;
                        for ( int32_t i = 0; i < roi.height; ++i ) {
                            memcpy( reinterpret_cast<int8_t *>( surface->pixels ) + screenWidth * i + screenOffset, image.image() + imageOffset + imageWidth * i,
                                    static_cast<size_t>( roi.width ) );
                        }
                    }
                }
            }

            if ( SDL_MUSTLOCK( surface ) )
                SDL_UnlockSurface( surface );
        }

        void generatePalette( const std::vector<uint8_t> & colorIds, const SDL_Surface * surface )
        {
            assert( surface != nullptr );

            if ( surface->format->BitsPerPixel == 32 ) {
                _palette32Bit.resize( 256u );

                if ( surface->format->Amask > 0 ) {
                    for ( size_t i = 0; i < 256u; ++i ) {
                        const uint8_t * value = currentPalette + colorIds[i] * 3;
                        _palette32Bit[i] = SDL_MapRGBA( surface->format, *value, *( value + 1 ), *( value + 2 ), 255 );
                    }
                }
                else {
                    for ( size_t i = 0; i < 256u; ++i ) {
                        const uint8_t * value = currentPalette + colorIds[i] * 3;
                        _palette32Bit[i] = SDL_MapRGB( surface->format, *value, *( value + 1 ), *( value + 2 ) );
                    }
                }
            }
            else if ( surface->format->BitsPerPixel == 8 ) {
                _palette8Bit.resize( 256 );
                for ( uint32_t i = 0; i < 256; ++i ) {
                    const uint8_t * value = currentPalette + colorIds[i] * 3;
                    SDL_Color & col = _palette8Bit[i];

                    col.r = *value;
                    col.g = *( value + 1 );
                    col.b = *( value + 2 );
                }
            }
            else {
                // This is unsupported format. Please implement it.
                assert( 0 );
            }
        }

        SDL_Surface * generateIconSurface( const fheroes2::Image & icon )
        {
            SDL_Surface * surface = SDL_CreateRGBSurface( 0, icon.width(), icon.height(), 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000 );
            if ( surface == NULL )
                return NULL;

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
                        *out = SDL_MapRGBA( surface->format, *value, *( value + 1 ), *( value + 2 ), 255 );
                    }
                }
            }
            else {
                for ( ; out != outEnd; ++out, ++in, ++transform ) {
                    if ( *transform == 0 ) {
                        const uint8_t * value = currentPalette + *in * 3;
                        *out = SDL_MapRGB( surface->format, *value, *( value + 1 ), *( value + 2 ) );
                    }
                    else {
                        *out = SDL_MapRGB( surface->format, 0, 0, 0 );
                    }
                }
            }

            return surface;
        }
    };
#endif
}

namespace
{
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
    class RenderCursor : public fheroes2::Cursor
    {
    public:
        ~RenderCursor() override
        {
            clear();
        }

        void show( const bool enable ) override
        {
            fheroes2::Cursor::show( enable );

            if ( !_emulation ) {
                SDL_ShowCursor( _show ? SDL_ENABLE : SDL_DISABLE );
            }
        }

        bool isVisible() const override
        {
            if ( _emulation )
                return fheroes2::Cursor::isVisible();
            else
                return fheroes2::Cursor::isVisible() && ( SDL_ShowCursor( SDL_QUERY ) == SDL_ENABLE );
        }

        void update( const fheroes2::Image & image, int32_t offsetX, int32_t offsetY ) override
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
                        *out = SDL_MapRGBA( surface->format, *value, *( value + 1 ), *( value + 2 ), 255 );
                    }
                    else if ( *transform > 1 ) {
                        // SDL2 uses RGBA image on OS level separately from frame rendering.
                        // Here we are trying to simulate cursor's shadow as close as possible to the original game.
                        *out = SDL_MapRGBA( surface->format, 0, 0, 0, 64 );
                    }
                }
            }
            else {
                for ( ; out != outEnd; ++out, ++in, ++transform ) {
                    if ( *transform == 0 ) {
                        const uint8_t * value = currentPalette + *in * 3;
                        *out = SDL_MapRGB( surface->format, *value, *( value + 1 ), *( value + 2 ) );
                    }
                    else {
                        *out = SDL_MapRGB( surface->format, 0, 0, 0 );
                    }
                }
            }

            SDL_Cursor * tempCursor = SDL_CreateColorCursor( surface, offsetX, offsetY );
            SDL_SetCursor( tempCursor );
            SDL_ShowCursor( _show ? SDL_ENABLE : SDL_DISABLE );
            SDL_FreeSurface( surface );

            clear();
            std::swap( _cursor, tempCursor );
        }

        void enableSoftwareEmulation( const bool enable ) override
        {
            if ( enable == _emulation )
                return;

            if ( enable ) {
                clear();

                SDL_ShowCursor( SDL_DISABLE );

                _emulation = true;
            }
            else {
                SDL_ShowCursor( _show ? SDL_ENABLE : SDL_DISABLE );

                _emulation = false;
            }

            if ( _cursorUpdater != nullptr )
                _cursorUpdater();
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

            SDL_ShowCursor( _show ? SDL_ENABLE : SDL_DISABLE );
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
#if defined( FHEROES2_VITA )
    class RenderEngine : public fheroes2::BaseRenderEngine
    {
    public:
        RenderEngine( const RenderEngine & ) = delete;

        ~RenderEngine() override
        {
            clear();
        }

        RenderEngine & operator=( const RenderEngine & ) = delete;

        static RenderEngine * create()
        {
            return new RenderEngine;
        }

        fheroes2::Rect getActiveWindowROI() const override
        {
            return _destRect;
        }

        fheroes2::Size getCurrentScreenResolution() const override
        {
            return fheroes2::Size( VITA_FULLSCREEN_WIDTH, VITA_FULLSCREEN_HEIGHT );
        }

        std::vector<fheroes2::Size> getAvailableResolutions() const override
        {
            static std::vector<fheroes2::Size> filteredResolutions;

            if ( filteredResolutions.empty() ) {
                std::set<fheroes2::Size> resolutionSet;
                resolutionSet.emplace( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );
                resolutionSet.emplace( VITA_ASPECT_CORRECTED_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );
                resolutionSet.emplace( VITA_FULLSCREEN_WIDTH, VITA_FULLSCREEN_HEIGHT );
                filteredResolutions = FilterResolutions( resolutionSet );
            }

            return filteredResolutions;
        }

    protected:
        RenderEngine()
            : _window( nullptr )
            , _surface( nullptr )
            , _texBuffer( nullptr )
            , _palettedTexturePointer( nullptr )
        {}

        enum
        {
            VITA_FULLSCREEN_WIDTH = 960,
            VITA_FULLSCREEN_HEIGHT = 544,
            VITA_ASPECT_CORRECTED_WIDTH = 848
        };

        void clear() override
        {
            if ( _window != nullptr ) {
                SDL_DestroyWindow( _window );
                _window = nullptr;
            }

            if ( _surface != nullptr ) {
                SDL_FreeSurface( _surface );
                _surface = nullptr;
            }

            vita2d_fini();

            if ( _texBuffer != nullptr ) {
                vita2d_free_texture( _texBuffer );
                _texBuffer = nullptr;
            }
        }

        bool allocate( int32_t & width_, int32_t & height_, bool isFullScreen ) override
        {
            clear();

            const std::vector<fheroes2::Size> resolutions = getAvailableResolutions();
            if ( !resolutions.empty() ) {
                const fheroes2::Size correctResolution = GetNearestResolution( width_, height_, resolutions );
                width_ = correctResolution.width;
                height_ = correctResolution.height;
            }

            vita2d_init();

            _window = SDL_CreateWindow( "", 0, 0, width_, height_, 0 );
            if ( _window == nullptr ) {
                clear();
                return false;
            }

            _surface = SDL_CreateRGBSurface( 0, 1, 1, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 );

            if ( _surface == nullptr || _surface->w <= 0 || _surface->h <= 0 ) {
                clear();
                return false;
            }

            vita2d_texture_set_alloc_memblock_type( SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW );
            _texBuffer = vita2d_create_empty_texture_format( width_, height_, SCE_GXM_TEXTURE_FORMAT_P8_ABGR );
            _palettedTexturePointer = static_cast<uint8_t *>( vita2d_texture_get_datap( _texBuffer ) );
            memset( _palettedTexturePointer, 0, width_ * height_ * sizeof( uint8_t ) );
            _createPalette();

            // screen scaling calculation
            _destRect.x = 0;
            _destRect.y = 0;
            _destRect.width = width_;
            _destRect.height = height_;

            if ( width_ != VITA_FULLSCREEN_WIDTH || height_ != VITA_FULLSCREEN_HEIGHT ) {
                if ( isFullScreen ) {
                    vita2d_texture_set_filters( _texBuffer, SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR );
                    if ( ( static_cast<float>( VITA_FULLSCREEN_WIDTH ) / VITA_FULLSCREEN_HEIGHT ) >= ( static_cast<float>( width_ ) / height_ ) ) {
                        const float scale = static_cast<float>( VITA_FULLSCREEN_HEIGHT ) / height_;
                        _destRect.width = static_cast<int32_t>( static_cast<float>( width_ ) * scale );
                        _destRect.height = VITA_FULLSCREEN_HEIGHT;
                        _destRect.x = ( VITA_FULLSCREEN_WIDTH - _destRect.width ) / 2;
                    }
                    else {
                        const float scale = static_cast<float>( VITA_FULLSCREEN_WIDTH ) / width_;
                        _destRect.width = VITA_FULLSCREEN_WIDTH;
                        _destRect.height = static_cast<int32_t>( static_cast<float>( height_ ) * scale );
                        _destRect.y = ( VITA_FULLSCREEN_HEIGHT - _destRect.height ) / 2;
                    }
                }
                else {
                    // center game area
                    _destRect.x = ( VITA_FULLSCREEN_WIDTH - width_ ) / 2;
                    _destRect.y = ( VITA_FULLSCREEN_HEIGHT - height_ ) / 2;
                }
            }

            return true;
        }

        void render( const fheroes2::Display & display, const fheroes2::Rect & roi ) override
        {
            (void)roi;

            if ( _texBuffer == nullptr )
                return;

            const int32_t width = display.width();
            const int32_t height = display.height();

            SDL_memcpy( _palettedTexturePointer, display.image(), width * height * sizeof( uint8_t ) );

            vita2d_start_drawing();
            vita2d_draw_texture_scale( _texBuffer, _destRect.x, _destRect.y, static_cast<float>( _destRect.width ) / width,
                                       static_cast<float>( _destRect.height ) / height );
            vita2d_end_drawing();
            vita2d_swap_buffers();
        }

        void updatePalette( const std::vector<uint8_t> & colorIds ) override
        {
            if ( _surface == nullptr || colorIds.size() != 256 || _texBuffer == nullptr )
                return;

            uint32_t palette32Bit[256u];

            for ( size_t i = 0; i < 256u; ++i ) {
                const uint8_t * value = currentPalette + colorIds[i] * 3;
                palette32Bit[i] = SDL_MapRGBA( _surface->format, *value, *( value + 1 ), *( value + 2 ), 255 );
            }

            memcpy( vita2d_texture_get_palette( _texBuffer ), palette32Bit, sizeof( uint32_t ) * 256 );
        }

        bool isMouseCursorActive() const override
        {
            return true;
        }

    private:
        SDL_Window * _window;
        SDL_Surface * _surface;
        vita2d_texture * _texBuffer;
        uint8_t * _palettedTexturePointer;
        fheroes2::Rect _destRect;

        void _createPalette()
        {
            updatePalette( StandardPaletteIndexes() );
        }
    };
#elif SDL_VERSION_ATLEAST( 2, 0, 0 )
    class RenderEngine : public fheroes2::BaseRenderEngine, public BaseSDLRenderer
    {
    public:
        RenderEngine( const RenderEngine & ) = delete;

        ~RenderEngine() override
        {
            clear();
        }

        RenderEngine & operator=( const RenderEngine & ) = delete;

        void toggleFullScreen() override
        {
            if ( _window == NULL ) {
                BaseRenderEngine::toggleFullScreen();
                return;
            }

            uint32_t flags = SDL_GetWindowFlags( _window );
            if ( ( flags & SDL_WINDOW_FULLSCREEN ) == SDL_WINDOW_FULLSCREEN || ( flags & SDL_WINDOW_FULLSCREEN_DESKTOP ) == SDL_WINDOW_FULLSCREEN_DESKTOP ) {
                flags = 0;

                if ( _windowedSize.width != 0 && _windowedSize.height != 0 ) {
                    SDL_SetWindowSize( _window, _windowedSize.width, _windowedSize.height );
                }
            }
            else {
#if defined( __WIN32__ )
                flags = SDL_WINDOW_FULLSCREEN;
#else
                flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
                SDL_GetWindowSize( _window, &_windowedSize.width, &_windowedSize.height );

                fheroes2::Display & display = fheroes2::Display::instance();
                if ( display.width() != 0 && display.height() != 0 ) {
                    SDL_SetWindowSize( _window, display.width(), display.height() );
                }
            }

            SDL_SetWindowFullscreen( _window, flags );
            _retrieveWindowInfo();
        }

        bool isFullScreen() const override
        {
            if ( _window == NULL )
                return BaseRenderEngine::isFullScreen();

            const uint32_t flags = SDL_GetWindowFlags( _window );
            return ( flags & SDL_WINDOW_FULLSCREEN ) != 0 || ( flags & SDL_WINDOW_FULLSCREEN_DESKTOP ) != 0;
        }

        std::vector<fheroes2::Size> getAvailableResolutions() const override
        {
            static std::vector<fheroes2::Size> filteredResolutions;

            if ( filteredResolutions.empty() ) {
                std::set<fheroes2::Size> resolutionSet;

                const int displayCount = SDL_GetNumVideoDisplays();
                if ( displayCount > 0 ) {
                    const int displayModeCount = SDL_GetNumDisplayModes( 0 );
                    for ( int i = 0; i < displayModeCount; ++i ) {
                        SDL_DisplayMode videoMode;
                        if ( SDL_GetDisplayMode( 0, i, &videoMode ) == 0 ) {
                            resolutionSet.emplace( videoMode.w, videoMode.h );
                        }
                    }
                }

                filteredResolutions = FilterResolutions( resolutionSet );
            }

            return filteredResolutions;
        }

        void setTitle( const std::string & title ) override
        {
            if ( _window != NULL )
                SDL_SetWindowTitle( _window, title.c_str() );
        }

        void setIcon( const fheroes2::Image & icon ) override
        {
            if ( _window == NULL )
                return;

            SDL_Surface * surface = generateIconSurface( icon );
            if ( surface == NULL )
                return;

            SDL_SetWindowIcon( _window, surface );

            SDL_FreeSurface( surface );
        }

        fheroes2::Rect getActiveWindowROI() const override
        {
            return _activeWindowROI;
        }

        fheroes2::Size getCurrentScreenResolution() const override
        {
            return _currentScreenResolution;
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

        void clear() override
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

            _windowedSize = fheroes2::Size();
        }

        void render( const fheroes2::Display & display, const fheroes2::Rect & roi ) override
        {
            if ( _surface == NULL )
                return;

            copyImageToSurface( display, _surface, roi );

            if ( _texture == NULL ) {
                if ( _renderer != NULL )
                    SDL_DestroyRenderer( _renderer );

                _renderer = SDL_CreateRenderer( _window, -1, renderFlags() );
            }
            else {
                const bool fullFrame = ( roi.width == display.width() ) && ( roi.height == display.height() );
                if ( fullFrame ) {
                    SDL_UpdateTexture( _texture, NULL, _surface->pixels, _surface->pitch );
                    if ( SDL_SetRenderTarget( _renderer, NULL ) == 0 ) {
                        if ( SDL_RenderClear( _renderer ) == 0 && SDL_RenderCopy( _renderer, _texture, NULL, NULL ) == 0 ) {
                            SDL_RenderPresent( _renderer );
                        }
                    }
                }
                else {
                    SDL_Rect area;
                    area.x = roi.x;
                    area.y = roi.y;
                    area.w = roi.width;
                    area.h = roi.height;

                    SDL_UpdateTexture( _texture, &area, _surface->pixels, _surface->pitch );
                    if ( SDL_SetRenderTarget( _renderer, NULL ) == 0 && SDL_RenderCopy( _renderer, _texture, NULL, NULL ) == 0 ) {
                        SDL_RenderPresent( _renderer );
                    }
                }
            }
        }

        bool allocate( int32_t & width_, int32_t & height_, bool isFullScreen ) override
        {
            clear();

            const std::vector<fheroes2::Size> resolutions = getAvailableResolutions();
            if ( !resolutions.empty() ) {
                const fheroes2::Size correctResolution = GetNearestResolution( width_, height_, resolutions );
                width_ = correctResolution.width;
                height_ = correctResolution.height;
            }

            uint32_t flags = SDL_WINDOW_SHOWN;
            if ( isFullScreen ) {
#if defined( __WIN32__ )
                flags |= SDL_WINDOW_FULLSCREEN;
#else
                flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
            }
            else {
                flags |= SDL_WINDOW_RESIZABLE;
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

            bool isPaletteModeSupported = false;

            SDL_RendererInfo rendererInfo;
            if ( SDL_GetRenderDriverInfo( 0, &rendererInfo ) == 0 ) {
                for ( uint32_t i = 0; i < rendererInfo.num_texture_formats; ++i ) {
                    if ( rendererInfo.texture_formats[i] == SDL_PIXELFORMAT_INDEX8 ) {
                        isPaletteModeSupported = true;
                        break;
                    }
                }
            }

            _surface = SDL_CreateRGBSurface( 0, width_, height_, isPaletteModeSupported ? 8 : 32, 0, 0, 0, 0 );
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

            _retrieveWindowInfo();
            return true;
        }

        void updatePalette( const std::vector<uint8_t> & colorIds ) override
        {
            if ( _surface == NULL || colorIds.size() != 256 )
                return;

            generatePalette( colorIds, _surface );
            if ( _surface->format->BitsPerPixel == 8 ) {
                SDL_SetPaletteColors( _surface->format->palette, _palette8Bit.data(), 0, 256 );
            }
        }

        bool isMouseCursorActive() const override
        {
            return ( _window != NULL ) && ( ( SDL_GetWindowFlags( _window ) & SDL_WINDOW_MOUSE_FOCUS ) == SDL_WINDOW_MOUSE_FOCUS );
        }

    private:
        SDL_Window * _window;
        SDL_Surface * _surface;
        SDL_Renderer * _renderer;
        SDL_Texture * _texture;

        std::string _previousWindowTitle;
        fheroes2::Point _prevWindowPos;
        fheroes2::Size _currentScreenResolution;
        fheroes2::Rect _activeWindowROI;

        fheroes2::Size _windowedSize;

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

                    // Display class doesn't have support for image pitch so we mustn't link display to surface if width is not divisible by 4.
                    if ( _surface->w % 4 == 0 ) {
                        linkRenderSurface( static_cast<uint8_t *>( _surface->pixels ) );
                    }
                }
            }
        }

        void _retrieveWindowInfo()
        {
            const int32_t displayIndex = SDL_GetWindowDisplayIndex( _window );
            SDL_DisplayMode displayMode;
            SDL_GetCurrentDisplayMode( displayIndex, &displayMode );
            _currentScreenResolution.width = displayMode.w;
            _currentScreenResolution.height = displayMode.h;

            SDL_GetWindowPosition( _window, &_activeWindowROI.x, &_activeWindowROI.y );
            SDL_GetWindowSize( _window, &_activeWindowROI.width, &_activeWindowROI.height );
        }
    };
#else
    class RenderEngine : public fheroes2::BaseRenderEngine, public BaseSDLRenderer
    {
    public:
        RenderEngine( const RenderEngine & ) = delete;

        ~RenderEngine() override
        {
            clear();
        }

        RenderEngine & operator=( const RenderEngine & ) = delete;

        void toggleFullScreen() override
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

        bool isFullScreen() const override
        {
            if ( _surface == NULL )
                return BaseRenderEngine::isFullScreen();

            return ( ( _surface->flags & SDL_FULLSCREEN ) != 0 );
        }

        std::vector<fheroes2::Size> getAvailableResolutions() const override
        {
            static std::vector<fheroes2::Size> filteredResolutions;

            if ( filteredResolutions.empty() ) {
                std::set<fheroes2::Size> resolutionSet;
                SDL_Rect ** modes = SDL_ListModes( NULL, SDL_FULLSCREEN | SDL_HWSURFACE );
                if ( modes != NULL && modes != reinterpret_cast<SDL_Rect **>( -1 ) ) {
                    for ( int i = 0; modes[i]; ++i ) {
                        resolutionSet.emplace( modes[i]->w, modes[i]->h );
                    }
                }

                filteredResolutions = FilterResolutions( resolutionSet );
            }

            return filteredResolutions;
        }

        void setTitle( const std::string & title ) override
        {
            SDL_WM_SetCaption( title.c_str(), NULL );
        }

        void setIcon( const fheroes2::Image & icon ) override
        {
            SDL_Surface * surface = generateIconSurface( icon );
            if ( surface == NULL )
                return;

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

        void render( const fheroes2::Display & display, const fheroes2::Rect & roi ) override
        {
            if ( _surface == NULL ) // nothing to render on
                return;

            copyImageToSurface( display, _surface, roi );

            // Logically we should call SDL_UpdateRect but some systems have flickering effect with this function.
            SDL_Flip( _surface );
        }

        void clear() override
        {
            linkRenderSurface( NULL );

            if ( _surface != NULL ) {
                SDL_FreeSurface( _surface );
                _surface = NULL;
            }

            _palette32Bit.clear();
            _palette8Bit.clear();
        }

        bool allocate( int32_t & width_, int32_t & height_, bool isFullScreen ) override
        {
            clear();

            const std::vector<fheroes2::Size> resolutions = getAvailableResolutions();
            if ( !resolutions.empty() ) {
                const fheroes2::Size correctResolution = GetNearestResolution( width_, height_, resolutions );
                width_ = correctResolution.width;
                height_ = correctResolution.height;
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

        void updatePalette( const std::vector<uint8_t> & colorIds ) override
        {
            if ( _surface == NULL || colorIds.size() != 256 )
                return;

            generatePalette( colorIds, _surface );
            if ( _surface->format->BitsPerPixel == 8 ) {
                SDL_SetPalette( _surface, SDL_LOGPAL | SDL_PHYSPAL, _palette8Bit.data(), 0, 256 );
            }
        }

        bool isMouseCursorActive() const override
        {
            return ( SDL_GetAppState() & SDL_APPMOUSEFOCUS ) == SDL_APPMOUSEFOCUS;
        }

    private:
        SDL_Surface * _surface;
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
                    // Copy the image from display buffer to SDL surface in case of fullscreen toggling
                    fheroes2::Display & display = fheroes2::Display::instance();
                    if ( _surface->w == display.width() && _surface->h == display.height() ) {
                        memcpy( _surface->pixels, display.image(), static_cast<size_t>( display.width() * display.height() ) );
                    }

                    // Display class doesn't have support for image pitch so we mustn't link display to surface if width is not divisible by 4.
                    if ( _surface->w % 4 == 0 ) {
                        linkRenderSurface( static_cast<uint8_t *>( _surface->pixels ) );
                    }
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
    {
        _disableTransformLayer();
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
        render( Rect( 0, 0, width(), height() ) );
    }

    void Display::render( const Rect & roi )
    {
        Rect temp( roi );
        if ( !getActiveArea( temp, width(), height() ) )
            return;

        getActiveArea( _prevRoi, width(), height() );

        if ( _cursor->isVisible() && _cursor->isSoftwareEmulation() && !_cursor->_image.empty() ) {
            const Sprite & cursorImage = _cursor->_image;
            const Sprite backup = Crop( *this, cursorImage.x(), cursorImage.y(), cursorImage.width(), cursorImage.height() );
            Blit( cursorImage, *this, cursorImage.x(), cursorImage.y() );

            if ( !backup.empty() ) {
                // ROI must include cursor's area as well, otherwise cursor won't be rendered.
                Rect cursorROI( cursorImage.x(), cursorImage.y(), cursorImage.width(), cursorImage.height() );
                if ( getActiveArea( cursorROI, width(), height() ) ) {
                    temp = getCommonRoi( temp, cursorROI );
                }
            }

            // Previous position of cursor must be updated as well to avoid ghost effect.
            _renderFrame( getCommonRoi( temp, _prevRoi ) );

            if ( _postprocessing != nullptr ) {
                _postprocessing();
            }

            Copy( backup, 0, 0, *this, backup.x(), backup.y(), backup.width(), backup.height() );
        }
        else {
            _renderFrame( getCommonRoi( temp, _prevRoi ) );

            if ( _postprocessing != nullptr ) {
                _postprocessing();
            }
        }

        _prevRoi = temp;
    }

    void Display::_renderFrame( const Rect & roi ) const
    {
        bool updateImage = true;
        if ( _preprocessing != NULL ) {
            std::vector<uint8_t> palette;
            if ( _preprocessing( palette ) ) {
                _engine->updatePalette( palette );
                // when we change a palette for 8-bit image we unwillingly call render so we don't need to re-render the same frame again
                updateImage = ( _renderSurface == NULL );
                if ( updateImage ) {
                    // Pre-processing step is applied to the whole image so we forcefully render the full frame.
                    _engine->render( *this, Rect( 0, 0, width(), height() ) );
                    return;
                }
            }
        }

        if ( updateImage ) {
            _engine->render( *this, roi );
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

    void Display::changePalette( const uint8_t * palette ) const
    {
        if ( currentPalette == palette || ( palette == NULL && currentPalette == PALPAlette() ) )
            return;

        currentPalette = ( palette == NULL ) ? PALPAlette() : palette;

        _engine->updatePalette( StandardPaletteIndexes() );
    }

    void Display::setEngine( std::unique_ptr<BaseRenderEngine> & engine )
    {
        assert( engine.get() != nullptr );
        if ( engine.get() == nullptr ) {
            return;
        }
        std::swap( engine, _engine );
    }

    void Display::setCursor( std::unique_ptr<Cursor> & cursor )
    {
        assert( cursor.get() != nullptr );
        if ( cursor.get() == nullptr ) {
            return;
        }
        std::swap( cursor, _cursor );
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
