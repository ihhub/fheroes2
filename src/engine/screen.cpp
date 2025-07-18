/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2025                                             *
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
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <ostream>
#include <set>
#include <utility>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_mouse.h>
#include <SDL_pixels.h>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>
#include <SDL_version.h>
#include <SDL_video.h>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif

#if defined( TARGET_PS_VITA )
#include <vita2d.h>
#endif

#include "image_palette.h"
#include "logging.h"
#include "math_tools.h"
#include "screen.h"
#include "system.h"

namespace
{
    // Returns nearest screen supported resolution
    fheroes2::ResolutionInfo GetNearestResolution( fheroes2::ResolutionInfo resolutionInfo, const std::vector<fheroes2::ResolutionInfo> & resolutions )
    {
        if ( resolutions.empty() ) {
            return resolutionInfo;
        }

        if ( resolutionInfo.gameWidth < 1 ) {
            resolutionInfo.gameWidth = 1;
        }

        if ( resolutionInfo.gameHeight < 1 ) {
            resolutionInfo.gameHeight = 1;
        }

        if ( resolutionInfo.screenWidth < resolutionInfo.gameWidth ) {
            resolutionInfo.screenWidth = resolutionInfo.gameWidth;
        }

        if ( resolutionInfo.screenHeight < resolutionInfo.gameHeight ) {
            resolutionInfo.screenHeight = resolutionInfo.gameHeight;
        }

        const double gameX = resolutionInfo.gameWidth;
        const double gameY = resolutionInfo.gameHeight;
        const double screenX = resolutionInfo.screenWidth;
        const double screenY = resolutionInfo.screenHeight;

        std::vector<double> similarity( resolutions.size(), 0 );
        for ( size_t i = 0; i < resolutions.size(); ++i ) {
            similarity[i] = std::fabs( resolutions[i].gameWidth - gameX ) / gameX + std::fabs( resolutions[i].gameHeight - gameY ) / gameY
                            + std::fabs( resolutions[i].screenWidth - screenX ) / screenX + std::fabs( resolutions[i].screenHeight - screenY ) / screenY;
        }

        const std::vector<double>::difference_type id = std::distance( similarity.begin(), std::min_element( similarity.begin(), similarity.end() ) );

        return resolutions[id];
    }

#if !defined( TARGET_PS_VITA )
    bool IsLowerThanDefaultRes( const fheroes2::ResolutionInfo & value )
    {
        return value.gameWidth < fheroes2::Display::DEFAULT_WIDTH || value.gameHeight < fheroes2::Display::DEFAULT_HEIGHT;
    }

    std::set<fheroes2::ResolutionInfo> FilterResolutions( const std::set<fheroes2::ResolutionInfo> & resolutionSet )
    {
        static_assert( fheroes2::Display::DEFAULT_WIDTH == 640 && fheroes2::Display::DEFAULT_HEIGHT == 480, "Default resolution must be 640 x 480" );

        if ( resolutionSet.empty() ) {
            return { { fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT } };
        }

        std::set<fheroes2::ResolutionInfo> resolutions;

        for ( const fheroes2::ResolutionInfo & resolution : resolutionSet ) {
            if ( IsLowerThanDefaultRes( resolution ) ) {
                continue;
            }

            resolutions.emplace( resolution );
        }

        if ( resolutions.empty() ) {
            return { { fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT } };
        }

        {
            // TODO: add resolutions which are close to the current screen aspect ratio.
            // Some operating systems do not work well with SDL so they return very limited number of high resolutions.
            // Populate missing resolutions into the list.
            const std::set<fheroes2::ResolutionInfo> possibleResolutions
                = { { 640, 480 },   { 800, 600 },  { 1024, 768 },  { 1152, 864 }, { 1280, 600 }, { 1280, 720 },  { 1280, 768 }, { 1280, 960 },
                    { 1280, 1024 }, { 1360, 768 }, { 1400, 1050 }, { 1440, 900 }, { 1600, 900 }, { 1680, 1050 }, { 1920, 1080 } };

            assert( !resolutions.empty() );
            const fheroes2::ResolutionInfo lowestResolution = *( resolutions.begin() );

            for ( const fheroes2::ResolutionInfo & resolution : possibleResolutions ) {
                if ( lowestResolution.gameWidth < resolution.gameWidth || lowestResolution.gameHeight < resolution.gameHeight || lowestResolution == resolution ) {
                    continue;
                }

                resolutions.emplace( resolution );
            }
        }

        std::set<fheroes2::ResolutionInfo> scaledResolutions;

        {
            // Widescreen devices support much higher resolutions but items on such resolutions are too tiny. In order to improve user
            // experience on these devices we are adding a special non-standard resolution with (potentially) non-integer scale.
            assert( !resolutions.empty() );
            const fheroes2::ResolutionInfo biggestResolution = *( std::prev( resolutions.end() ) );
            assert( biggestResolution.gameWidth == biggestResolution.screenWidth && biggestResolution.gameHeight == biggestResolution.screenHeight );

            // This resolution should be really "widescreen" (or at least square), i.e. the width should be not less than the height, otherwise
            // the scaled resolution's in-game width will become less than DEFAULT_WIDTH
            if ( biggestResolution.gameWidth > fheroes2::Display::DEFAULT_WIDTH && biggestResolution.gameWidth >= biggestResolution.gameHeight ) {
                assert( biggestResolution.gameHeight >= fheroes2::Display::DEFAULT_HEIGHT );

                scaledResolutions.emplace( biggestResolution.gameWidth * fheroes2::Display::DEFAULT_HEIGHT / biggestResolution.gameHeight,
                                           fheroes2::Display::DEFAULT_HEIGHT, biggestResolution.gameWidth, biggestResolution.gameHeight );
            }
        }

        // Try to find and add scaled resolutions with integer scale (x2, x3, x4, etc) which correspond to existing "hardware" resolutions
        for ( const fheroes2::ResolutionInfo & resolution : resolutions ) {
            assert( resolution.gameWidth == resolution.screenWidth && resolution.gameHeight == resolution.screenHeight );

            const int32_t maxScale = std::min( resolution.gameWidth / fheroes2::Display::DEFAULT_WIDTH, resolution.gameHeight / fheroes2::Display::DEFAULT_HEIGHT );

            for ( int32_t scale = 2; scale <= maxScale; ++scale ) {
                if ( resolution.gameWidth % scale != 0 || resolution.gameHeight % scale != 0 ) {
                    continue;
                }

                scaledResolutions.emplace( resolution.gameWidth / scale, resolution.gameHeight / scale, resolution.gameWidth, resolution.gameHeight );
            }
        }

        resolutions.merge( scaledResolutions );

        return resolutions;
    }
#endif

    std::vector<uint8_t> StandardPaletteIndexes()
    {
        std::vector<uint8_t> indexes( 256 );
        for ( uint32_t i = 0; i < 256; ++i ) {
            indexes[i] = static_cast<uint8_t>( i );
        }
        return indexes;
    }

    const uint8_t * PALPalette( const bool forceDefaultPaletteUpdate = false )
    {
        static std::vector<uint8_t> palette;
        if ( palette.empty() || forceDefaultPaletteUpdate ) {
            const uint8_t * gamePalette = fheroes2::getGamePalette();

            palette.resize( 256 * 3 );
            for ( size_t i = 0; i < palette.size(); ++i ) {
                palette[i] = gamePalette[i] << 2;
            }
        }

        return palette.data();
    }

    // Updates `roi` to be only inside {0, 0, width, height} rectangle.
    // Returns false if `roi` is out of such rectangle.
    bool getActiveArea( fheroes2::Rect & roi, const int32_t width, const int32_t height )
    {
        if ( roi.width <= 0 || roi.height <= 0 || roi.x >= width || roi.y >= height ) {
            return false;
        }

        if ( roi.x < 0 ) {
            if ( -roi.x >= roi.width ) {
                return false;
            }

            roi.width += roi.x;
            roi.x = 0;
        }

        if ( roi.y < 0 ) {
            if ( -roi.y >= roi.height ) {
                return false;
            }

            roi.height += roi.y;
            roi.y = 0;
        }

        if ( const int32_t offsetX = roi.x + roi.width - width; offsetX > 0 ) {
            if ( offsetX >= roi.width ) {
                return false;
            }

            roi.width -= offsetX;
        }

        if ( const int32_t offsetY = roi.y + roi.height - height; offsetY > 0 ) {
            if ( offsetY >= roi.height ) {
                return false;
            }

            roi.height -= offsetY;
        }

        return true;
    }

    const uint8_t * currentPalette = PALPalette();

// If SDL library is used
#if !defined( TARGET_PS_VITA )
    class BaseSDLRenderer
    {
    protected:
        std::vector<uint32_t> _palette32Bit;
        std::vector<SDL_Color> _palette8Bit;

        void copyImageToSurface( const fheroes2::Image & image, SDL_Surface * surface, const fheroes2::Rect & roi )
        {
            assert( surface != nullptr && !image.empty() );

            if ( SDL_MUSTLOCK( surface ) ) {
                const int returnCode = SDL_LockSurface( surface );
                if ( returnCode < 0 ) {
                    ERROR_LOG( "Failed to lock surface. The error value: " << returnCode << ", description: " << SDL_GetError() )
                }
            }

            const int32_t imageWidth = image.width();
            const int32_t imageHeight = image.height();

            const bool fullFrame = ( roi.width == imageWidth ) && ( roi.height == imageHeight );

            const uint8_t * imageIn = image.image();

            if ( fullFrame ) {
                if ( surface->format->BitsPerPixel == 32 ) {
                    uint32_t * out = static_cast<uint32_t *>( surface->pixels );
                    const uint32_t * outEnd = out + imageWidth * imageHeight;
                    const uint8_t * in = imageIn;
                    const uint32_t * transform = _palette32Bit.data();

                    for ( ; out != outEnd; ++out, ++in )
                        *out = *( transform + *in );
                }
                else if ( ( surface->format->BitsPerPixel == 8 ) && ( surface->pixels != imageIn ) ) {
                    if ( imageWidth % 4 != 0 ) {
                        const int32_t screenWidth = ( imageWidth / 4 ) * 4 + 4;
                        for ( int32_t i = 0; i < imageHeight; ++i ) {
                            memcpy( static_cast<uint8_t *>( surface->pixels ) + screenWidth * i, imageIn + imageWidth * i, static_cast<size_t>( imageWidth ) );
                        }
                    }
                    else {
                        memcpy( surface->pixels, imageIn, static_cast<size_t>( imageWidth ) * imageHeight );
                    }
                }
            }
            else {
                if ( surface->format->BitsPerPixel == 32 ) {
                    uint32_t * outY = static_cast<uint32_t *>( surface->pixels );
                    const uint32_t * outYEnd = outY + imageWidth * roi.height;
                    const uint8_t * inY = imageIn + roi.x + roi.y * imageWidth;
                    const uint32_t * transform = _palette32Bit.data();

                    for ( ; outY != outYEnd; outY += imageWidth, inY += imageWidth ) {
                        uint32_t * outX = outY;
                        const uint32_t * outXEnd = outX + roi.width;
                        const uint8_t * inX = inY;

                        for ( ; outX != outXEnd; ++outX, ++inX )
                            *outX = *( transform + *inX );
                    }
                }
                else if ( ( surface->format->BitsPerPixel == 8 ) && ( surface->pixels != imageIn ) ) {
                    const int32_t screenWidth = ( imageWidth / 4 ) * 4 + 4;
                    const int32_t screenOffset = roi.x + roi.y * screenWidth;
                    const int32_t imageOffset = roi.x + roi.y * imageWidth;
                    for ( int32_t i = 0; i < roi.height; ++i ) {
                        memcpy( static_cast<uint8_t *>( surface->pixels ) + screenWidth * i + screenOffset, imageIn + imageOffset + imageWidth * i,
                                static_cast<size_t>( roi.width ) );
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
            if ( icon.empty() || icon.singleLayer() ) {
                // What are you trying to do? Icon should have not empty both image and transform layers.
                assert( 0 );
                return nullptr;
            }

            SDL_Surface * surface = SDL_CreateRGBSurface( 0, icon.width(), icon.height(), 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000 );
            if ( surface == nullptr ) {
                ERROR_LOG( "Failed to create a surface of " << icon.width() << " x " << icon.height() << " size for cursor. The error: " << SDL_GetError() )
                return nullptr;
            }

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

    class RenderCursor final : public fheroes2::Cursor
    {
    public:
        RenderCursor( const RenderCursor & ) = delete;

        ~RenderCursor() override
        {
            clear();
        }

        RenderCursor & operator=( const RenderCursor & ) = delete;

        void show( const bool enable ) override
        {
            fheroes2::Cursor::show( enable );

            if ( !_emulation ) {
                const int returnCode = SDL_ShowCursor( _show ? SDL_ENABLE : SDL_DISABLE );
                if ( returnCode < 0 ) {
                    ERROR_LOG( "Failed to set cursor. The error value: " << returnCode << ", description: " << SDL_GetError() )
                }
            }
        }

        bool isVisible() const override
        {
            if ( _emulation ) {
                return fheroes2::Cursor::isVisible();
            }

            return fheroes2::Cursor::isVisible() && ( SDL_ShowCursor( SDL_QUERY ) == SDL_ENABLE );
        }

        void update( const fheroes2::Image & image, int32_t offsetX, int32_t offsetY ) override
        {
            if ( image.empty() || image.singleLayer() ) {
                // What are you trying to do? Set an invisible cursor? Use hide() method!
                assert( 0 );
                return;
            }

            if ( _emulation ) {
                fheroes2::Cursor::update( image, offsetX, offsetY );
                return;
            }

            SDL_Surface * surface = SDL_CreateRGBSurface( 0, image.width(), image.height(), 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000 );
            if ( surface == nullptr ) {
                ERROR_LOG( "Failed to create a surface of " << image.width() << " x " << image.height() << " size for cursor. The error: " << SDL_GetError() )
                return;
            }

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
            if ( tempCursor == nullptr ) {
                ERROR_LOG( "Failed to create a cursor. The error description: " << SDL_GetError() )
            }
            else {
                SDL_SetCursor( tempCursor );
            }

            const int returnCode = SDL_ShowCursor( _show ? SDL_ENABLE : SDL_DISABLE );
            if ( returnCode < 0 ) {
                ERROR_LOG( "Failed to set cursor state. The error value: " << returnCode << ", description: " << SDL_GetError() )
            }
            SDL_FreeSurface( surface );

            if ( tempCursor != nullptr ) {
                clear();
                std::swap( _cursor, tempCursor );
            }
        }

        void enableSoftwareEmulation( const bool enable ) override
        {
            if ( enable == _emulation )
                return;

            if ( enable ) {
                clear();

                const int returnCode = SDL_ShowCursor( SDL_DISABLE );
                if ( returnCode < 0 ) {
                    ERROR_LOG( "Failed to disable cursor. The error value: " << returnCode << ", description: " << SDL_GetError() )
                }

                _emulation = true;
            }
            else {
                const int returnCode = SDL_ShowCursor( _show ? SDL_ENABLE : SDL_DISABLE );
                if ( returnCode < 0 ) {
                    ERROR_LOG( "Failed to set cursor state. The error value: " << returnCode << ", description: " << SDL_GetError() )
                }

                _emulation = false;
            }

            if ( _cursorUpdater != nullptr )
                _cursorUpdater();
        }

        static RenderCursor * create()
        {
            return new RenderCursor;
        }

    private:
        SDL_Cursor * _cursor{ nullptr };

        RenderCursor()
        {
            _emulation = false;

            const int returnCode = SDL_ShowCursor( _show ? SDL_ENABLE : SDL_DISABLE );
            if ( returnCode < 0 ) {
                ERROR_LOG( "Failed to set cursor state. The error value: " << returnCode << ", description: " << SDL_GetError() )
            }
        }

        void clear()
        {
            if ( _cursor != nullptr ) {
                SDL_FreeCursor( _cursor );
                _cursor = nullptr;
            }
        }
    };

#if defined( TARGET_PS_VITA )
    class RenderEngine final : public fheroes2::BaseRenderEngine
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
            BaseRenderEngine::toggleFullScreen();

            const fheroes2::Display & display = fheroes2::Display::instance();
            _calculateScreenScaling( display.width(), display.height(), isFullScreen() );
        }

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
            return { VITA_FULLSCREEN_WIDTH, VITA_FULLSCREEN_HEIGHT };
        }

        std::vector<fheroes2::ResolutionInfo> getAvailableResolutions() const override
        {
            static const std::vector<fheroes2::ResolutionInfo> filteredResolutions = []() {
                std::set<fheroes2::ResolutionInfo> resolutionSet;
                resolutionSet.emplace( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );
                resolutionSet.emplace( VITA_ASPECT_CORRECTED_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );
                resolutionSet.emplace( VITA_FULLSCREEN_WIDTH, VITA_FULLSCREEN_HEIGHT );

                return std::vector<fheroes2::ResolutionInfo>{ resolutionSet.rbegin(), resolutionSet.rend() };
            }();

            return filteredResolutions;
        }

    private:
        SDL_Window * _window{ nullptr };
        SDL_Surface * _surface{ nullptr };
        vita2d_texture * _texBuffer{ nullptr };
        uint8_t * _palettedTexturePointer{ nullptr };
        fheroes2::Rect _destRect;

        RenderEngine() = default;

        enum : int32_t
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

        bool allocate( fheroes2::ResolutionInfo & resolutionInfo, bool isFullScreen ) override
        {
            clear();

            const std::vector<fheroes2::ResolutionInfo> resolutions = getAvailableResolutions();
            assert( !resolutions.empty() );
            if ( !resolutions.empty() ) {
                resolutionInfo = GetNearestResolution( resolutionInfo, resolutions );
            }

            vita2d_init();

            _window = SDL_CreateWindow( "", 0, 0, resolutionInfo.gameWidth, resolutionInfo.gameHeight, 0 );
            if ( _window == nullptr ) {
                ERROR_LOG( "Failed to create an application window of " << resolutionInfo.gameWidth << " x " << resolutionInfo.gameHeight
                                                                        << " size. The error: " << SDL_GetError() )

                clear();
                return false;
            }

            _surface = SDL_CreateRGBSurface( 0, 1, 1, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 );

            if ( _surface == nullptr || _surface->w <= 0 || _surface->h <= 0 ) {
                ERROR_LOG( "Failed to create a surface of " << resolutionInfo.gameWidth << " x " << resolutionInfo.gameHeight << " size. The error: " << SDL_GetError() )

                clear();
                return false;
            }

            vita2d_texture_set_alloc_memblock_type( SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW );
            _texBuffer = vita2d_create_empty_texture_format( resolutionInfo.gameWidth, resolutionInfo.gameHeight, SCE_GXM_TEXTURE_FORMAT_P8_ABGR );
            _palettedTexturePointer = static_cast<uint8_t *>( vita2d_texture_get_datap( _texBuffer ) );
            memset( _palettedTexturePointer, 0, resolutionInfo.gameWidth * resolutionInfo.gameHeight * sizeof( uint8_t ) );
            _createPalette();

            _calculateScreenScaling( resolutionInfo.gameWidth, resolutionInfo.gameHeight, isFullScreen );

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
            vita2d_draw_rectangle( 0, 0, VITA_FULLSCREEN_WIDTH, VITA_FULLSCREEN_HEIGHT, 0xff000000 );
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

        void _createPalette()
        {
            updatePalette( StandardPaletteIndexes() );
        }

        void _calculateScreenScaling( const int32_t width_, const int32_t height_, const bool isFullScreen )
        {
            _destRect.x = 0;
            _destRect.y = 0;
            _destRect.width = width_;
            _destRect.height = height_;

            if ( width_ == VITA_FULLSCREEN_WIDTH && height_ == VITA_FULLSCREEN_HEIGHT ) {
                // Nothing to do more.
                return;
            }

            if ( !isFullScreen ) {
                // Center game area.
                _destRect.x = ( VITA_FULLSCREEN_WIDTH - width_ ) / 2;
                _destRect.y = ( VITA_FULLSCREEN_HEIGHT - height_ ) / 2;
                return;
            }

            const SceGxmTextureFilter textureFilter = isNearestScaling() ? SCE_GXM_TEXTURE_FILTER_POINT : SCE_GXM_TEXTURE_FILTER_LINEAR;

            vita2d_texture_set_filters( _texBuffer, textureFilter, textureFilter );
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
    };
#else
    std::optional<bool> isWindowInAnyDisplay( const fheroes2::ResolutionInfo & resolutionInfo, const fheroes2::Point & windowPos )
    {
        const int numDisplays = SDL_GetNumVideoDisplays();
        if ( numDisplays < 1 ) {
            ERROR_LOG( "Failed to get the number of video displays. The error value: " << numDisplays << ", description: " << SDL_GetError() )
            return std::nullopt;
        }

        for ( int displayIndex = 0; displayIndex < numDisplays; ++displayIndex ) {
            SDL_Rect displayBounds;
            if ( SDL_GetDisplayBounds( displayIndex, &displayBounds ) != 0 ) {
                ERROR_LOG( "Failed to get display bounds for display #" << displayIndex << ". The error: " << SDL_GetError() )
                continue;
            }
            const int displayMaxX = displayBounds.x + displayBounds.w;
            const int displayMaxY = displayBounds.y + displayBounds.h;
            const int32_t windowMaxX = windowPos.x + resolutionInfo.screenWidth;
            const int32_t windowMaxY = windowPos.y + resolutionInfo.screenHeight;
            if ( windowPos.x >= displayBounds.x && windowMaxX < displayMaxX && windowPos.y >= displayBounds.y && windowMaxY < displayMaxY ) {
                return true;
            }
        }
        return false;
    }

    class RenderEngine final : public fheroes2::BaseRenderEngine, public BaseSDLRenderer
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
            if ( _window == nullptr ) {
                BaseRenderEngine::toggleFullScreen();
                return;
            }

            uint32_t flags = SDL_GetWindowFlags( _window );
            if ( flags & ( SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP ) ) {
                flags &= ~( SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP );
            }
            else {
#if defined( _WIN32 )
                if ( fheroes2::cursor().isSoftwareEmulation() ) {
                    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
                }
                else {
                    flags |= SDL_WINDOW_FULLSCREEN;
                }
#else
                flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif

                // Update the window position, in case we get queried for it
                SDL_GetWindowPosition( _window, &_prevWindowPos.x, &_prevWindowPos.y );

                // If the window size has been manually changed to one that does not match any of the resolutions supported by the display, then after switching
                // to full-screen mode, the in-game display area may not occupy the entire screen, and black bars will remain on the sides of the screen. In this
                // case, even if it is specified to use the SDL_WINDOW_FULLSCREEN, the SDL_WINDOW_FULLSCREEN_DESKTOP will still be used under the hood. To avoid
                // this, we need to remember the window size (to restore it later when turning off full-screen mode) and then set the window size to the size of
                // the in-game display area before switching to the full-screen mode.
                SDL_GetWindowSize( _window, &_windowedSize.width, &_windowedSize.height );

                if ( const fheroes2::Display & display = fheroes2::Display::instance(); display.width() != 0 && display.height() != 0 ) {
                    assert( display.screenSize().width >= display.width() && display.screenSize().height >= display.height() );

                    SDL_SetWindowSize( _window, display.screenSize().width, display.screenSize().height );
                }
            }

            if ( const int returnCode = SDL_SetWindowFullscreen( _window, flags ); returnCode < 0 ) {
                ERROR_LOG( "Failed to set fullscreen mode flags. The error value: " << returnCode << ", description: " << SDL_GetError() )
            }

            _syncFullScreen();

            // If the full-screen mode has been turned off, then restore the remembered window size.
            if ( !isFullScreen() && _windowedSize.width != 0 && _windowedSize.height != 0 ) {
                SDL_SetWindowSize( _window, _windowedSize.width, _windowedSize.height );
            }

            _retrieveWindowInfo();
            _toggleMouseCaptureMode();
        }

        bool isFullScreen() const override
        {
            if ( _window == nullptr ) {
                return BaseRenderEngine::isFullScreen();
            }

            const uint32_t flags = SDL_GetWindowFlags( _window );
            return ( flags & ( SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP ) );
        }

        std::vector<fheroes2::ResolutionInfo> getAvailableResolutions() const override
        {
            static const std::vector<fheroes2::ResolutionInfo> filteredResolutions = []() {
                std::set<fheroes2::ResolutionInfo> resolutionSet;

                const int displayCount = SDL_GetNumVideoDisplays();
                if ( displayCount >= 1 ) {
                    const int displayModeCount = SDL_GetNumDisplayModes( 0 );
                    if ( displayModeCount >= 1 ) {
                        for ( int i = 0; i < displayModeCount; ++i ) {
                            SDL_DisplayMode videoMode;
                            const int returnCode = SDL_GetDisplayMode( 0, i, &videoMode );
                            if ( returnCode != 0 ) {
                                ERROR_LOG( "Failed to get display mode. The error value: " << returnCode << ", description: " << SDL_GetError() )
                            }
                            else {
                                resolutionSet.emplace( videoMode.w, videoMode.h );
                            }
                        }
                    }
                    else {
                        ERROR_LOG( "Failed to get the number of display modes. The error value: " << displayModeCount << ", description: " << SDL_GetError() )
                    }
                }
                else {
                    ERROR_LOG( "Failed to get the number of displays. The error value: " << displayCount << ", description: " << SDL_GetError() )
                }

#if defined( TARGET_NINTENDO_SWITCH )
                // Nintendo Switch supports arbitrary resolutions via the HW scaler
                // 848x480 is the smallest resolution supported by fheroes2
                resolutionSet.emplace( 848, 480 );
#endif
                resolutionSet = FilterResolutions( resolutionSet );

                return std::vector<fheroes2::ResolutionInfo>{ resolutionSet.rbegin(), resolutionSet.rend() };
            }();

            return filteredResolutions;
        }

        void setTitle( const std::string & title ) override
        {
            if ( _window == nullptr ) {
                return;
            }

            SDL_SetWindowTitle( _window, System::encLocalToUTF8( title ).c_str() );
        }

        void setIcon( const fheroes2::Image & icon ) override
        {
            if ( _window == nullptr )
                return;

            SDL_Surface * surface = generateIconSurface( icon );
            if ( surface == nullptr )
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

        void setVSync( const bool enable ) override
        {
            _isVSyncEnabled = enable;

            if ( _renderer == nullptr ) {
                return;
            }

            _toggleVSync();
        }

        fheroes2::Point getWindowPos() const override
        {
            if ( isFullScreen() ) {
                return _prevWindowPos;
            }
            if ( _window == nullptr ) {
                return { -1, -1 };
            }
            fheroes2::Point result;
            SDL_GetWindowPosition( _window, &result.x, &result.y );
            return result;
        }

        void setWindowPos( const fheroes2::Point pos ) override
        {
            if ( pos == fheroes2::Point{ -1, -1 } ) {
                _prevWindowPos = { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED };
            }
            else {
                _prevWindowPos = pos;
            }
        }

    private:
        SDL_Window * _window{ nullptr };
        SDL_Surface * _surface{ nullptr };
        SDL_Renderer * _renderer{ nullptr };
        SDL_Texture * _texture{ nullptr };
        int _driverIndex{ -1 };

        std::string _previousWindowTitle;
        fheroes2::Point _prevWindowPos{ SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED };
        fheroes2::Size _currentScreenResolution;
        fheroes2::Rect _activeWindowROI;

        fheroes2::Size _windowedSize;

        bool _isVSyncEnabled{ false };

        RenderEngine() = default;

        void clear() override
        {
            if ( _texture != nullptr ) {
                SDL_DestroyTexture( _texture );
                _texture = nullptr;
            }

            if ( _renderer != nullptr ) {
                SDL_DestroyRenderer( _renderer );
                _renderer = nullptr;
            }

            if ( _window != nullptr ) {
                // Let's collect needed info about previous setup
                if ( !isFullScreen() ) {
                    SDL_GetWindowPosition( _window, &_prevWindowPos.x, &_prevWindowPos.y );
                }
                _previousWindowTitle = System::encUTF8ToLocal( SDL_GetWindowTitle( _window ) );

                SDL_DestroyWindow( _window );
                _window = nullptr;
            }

            if ( _surface != nullptr ) {
                SDL_FreeSurface( _surface );
                _surface = nullptr;
            }

            _driverIndex = -1;
            _windowedSize = {};
        }

        void render( const fheroes2::Display & display, const fheroes2::Rect & roi ) override
        {
            if ( _surface == nullptr ) {
                return;
            }

            assert( _renderer != nullptr && _texture != nullptr );

            copyImageToSurface( display, _surface, roi );

            const bool fullFrame = ( roi.width == display.width() ) && ( roi.height == display.height() );
            if ( fullFrame ) {
                const int returnCode = SDL_UpdateTexture( _texture, nullptr, _surface->pixels, _surface->pitch );
                if ( returnCode < 0 ) {
                    ERROR_LOG( "Failed to update texture. The error value: " << returnCode << ", description: " << SDL_GetError() )
                }
            }
            else {
                SDL_Rect area;
                area.x = roi.x;
                area.y = roi.y;
                area.w = roi.width;
                area.h = roi.height;

                const int returnCode = SDL_UpdateTexture( _texture, &area, _surface->pixels, _surface->pitch );
                if ( returnCode < 0 ) {
                    ERROR_LOG( "Failed to update texture. The error value: " << returnCode << ", description: " << SDL_GetError() )
                }
            }

            int returnCode = SDL_RenderClear( _renderer );
            if ( returnCode < 0 ) {
                ERROR_LOG( "Failed to clear renderer. The error value: " << returnCode << ", description: " << SDL_GetError() )
                return;
            }

            returnCode = SDL_RenderCopy( _renderer, _texture, nullptr, nullptr );
            if ( returnCode < 0 ) {
                ERROR_LOG( "Failed to copy render.The error value: " << returnCode << ", description: " << SDL_GetError() )
                return;
            }

            SDL_RenderPresent( _renderer );
        }

        bool allocate( fheroes2::ResolutionInfo & resolutionInfo, bool isFullScreen ) override
        {
            clear();

            const std::vector<fheroes2::ResolutionInfo> resolutions = getAvailableResolutions();
            assert( !resolutions.empty() );
            if ( !resolutions.empty() ) {
                resolutionInfo = GetNearestResolution( resolutionInfo, resolutions );
            }

#if defined( ANDROID )
            // Same as ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE
            if ( SDL_SetHint( SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight" ) == SDL_FALSE ) {
                ERROR_LOG( "Failed to set the " SDL_HINT_ORIENTATIONS " hint." )
            }
#endif

            uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
            if ( isFullScreen ) {
#if defined( _WIN32 )
                if ( fheroes2::cursor().isSoftwareEmulation() ) {
                    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
                }
                else {
                    flags |= SDL_WINDOW_FULLSCREEN;
                }
#else
                flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
            }

            std::optional<bool> isInDisplay = isWindowInAnyDisplay( resolutionInfo, _prevWindowPos );

            // check if the previous window position is within display or error occured
            if ( !isInDisplay || !*isInDisplay ) {
                // reset position if it is not within the bounds of any display
                _prevWindowPos = { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED };
            }

            _window = SDL_CreateWindow( System::encLocalToUTF8( _previousWindowTitle ).c_str(), _prevWindowPos.x, _prevWindowPos.y, resolutionInfo.screenWidth,
                                        resolutionInfo.screenHeight, flags );
            if ( _window == nullptr ) {
                ERROR_LOG( "Failed to create an application window of " << resolutionInfo.screenWidth << " x " << resolutionInfo.screenHeight
                                                                        << " size. The error: " << SDL_GetError() )
                clear();
                return false;
            }

            _syncFullScreen();

            bool isPaletteModeSupported = false;

            SDL_RendererInfo rendererInfo;
            _driverIndex = -1;

            if ( const int driverCount = SDL_GetNumRenderDrivers(); driverCount >= 0 ) {
                const uint32_t requiredFlags = SDL_RENDERER_ACCELERATED;

                for ( int driverId = 0; driverId < driverCount; ++driverId ) {
                    int returnCode = SDL_GetRenderDriverInfo( driverId, &rendererInfo );
                    if ( returnCode < 0 ) {
                        ERROR_LOG( "Failed to get renderer driver info. The error value: " << returnCode << ", description: " << SDL_GetError() )
                        continue;
                    }

                    if ( ( rendererInfo.flags & requiredFlags ) != requiredFlags ) {
                        continue;
                    }

                    for ( uint32_t i = 0; i < rendererInfo.num_texture_formats; ++i ) {
                        if ( rendererInfo.texture_formats[i] == SDL_PIXELFORMAT_INDEX8 ) {
                            // Bingo! This is the best driver and format.
                            isPaletteModeSupported = true;
                            _driverIndex = driverId;
                            break;
                        }
                    }

                    if ( isPaletteModeSupported ) {
                        break;
                    }

                    if ( _driverIndex < 0 ) {
                        _driverIndex = driverId;
                    }
                }
            }
            else {
                ERROR_LOG( "Failed to get the number of render drivers. The error value: " << driverCount << ", description: " << SDL_GetError() )
            }

            _surface = SDL_CreateRGBSurface( 0, resolutionInfo.gameWidth, resolutionInfo.gameHeight, isPaletteModeSupported ? 8 : 32, 0, 0, 0, 0 );
            if ( _surface == nullptr ) {
                ERROR_LOG( "Failed to create a surface of " << resolutionInfo.gameWidth << " x " << resolutionInfo.gameHeight << " size. The error: " << SDL_GetError() )
                clear();
                return false;
            }

            if ( _surface->w <= 0 || _surface->h <= 0 || _surface->w != resolutionInfo.gameWidth || _surface->h != resolutionInfo.gameHeight ) {
                clear();
                return false;
            }

            _createPalette();

            _renderer = SDL_CreateRenderer( _window, _driverIndex, SDL_RENDERER_ACCELERATED );
            if ( _renderer == nullptr ) {
                ERROR_LOG( "Failed to create a window renderer of " << resolutionInfo.gameWidth << " x " << resolutionInfo.gameHeight
                                                                    << " size. The error: " << SDL_GetError() )
                clear();
                return false;
            }

            if ( const int returnCode = SDL_SetRenderDrawColor( _renderer, 0, 0, 0, SDL_ALPHA_OPAQUE ); returnCode < 0 ) {
                ERROR_LOG( "Failed to set default color for renderer. The error value: " << returnCode << ", description: " << SDL_GetError() )
            }

            if ( const int returnCode = SDL_SetRenderTarget( _renderer, nullptr ); returnCode < 0 ) {
                ERROR_LOG( "Failed to set render target to window. The error value: " << returnCode << ", description: " << SDL_GetError() )
            }

            if ( SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, ( isNearestScaling() ? "nearest" : "linear" ) ) == SDL_FALSE ) {
                ERROR_LOG( "Failed to set the " SDL_HINT_RENDER_SCALE_QUALITY " hint." )
            }

            // Setting this hint prevents the window to regain focus after losing it in fullscreen mode.
            // It also fixes issues when SDL_UpdateTexture() calls fail because of refocusing.
            if ( SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0" ) == SDL_FALSE ) {
                ERROR_LOG( "Failed to set the " SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS " hint." )
            }

            if ( const int returnCode = SDL_RenderSetLogicalSize( _renderer, resolutionInfo.gameWidth, resolutionInfo.gameHeight ); returnCode < 0 ) {
                ERROR_LOG( "Failed to create logical size of " << resolutionInfo.gameWidth << " x " << resolutionInfo.gameHeight
                                                               << " size. The error value: " << returnCode << ", description: " << SDL_GetError() )
                clear();
                return false;
            }

            _texture = SDL_CreateTextureFromSurface( _renderer, _surface );
            if ( _texture == nullptr ) {
                ERROR_LOG( "Failed to create a texture from a surface of " << resolutionInfo.gameWidth << " x " << resolutionInfo.gameHeight
                                                                           << " size. The error: " << SDL_GetError() )
                clear();
                return false;
            }

            if ( !_retrieveWindowInfo() ) {
                clear();
                return false;
            }

            _toggleMouseCaptureMode();
            _toggleVSync();

            return true;
        }

        void updatePalette( const std::vector<uint8_t> & colorIds ) override
        {
            if ( _surface == nullptr || colorIds.size() != 256 )
                return;

            generatePalette( colorIds, _surface );
            if ( _surface->format->BitsPerPixel == 8 ) {
                const int returnCode = SDL_SetPaletteColors( _surface->format->palette, _palette8Bit.data(), 0, 256 );
                if ( returnCode < 0 ) {
                    ERROR_LOG( "Failed to set palette color. The error value: " << returnCode << ", description: " << SDL_GetError() )
                }
            }
        }

        bool isMouseCursorActive() const override
        {
            return ( _window != nullptr ) && ( ( SDL_GetWindowFlags( _window ) & SDL_WINDOW_MOUSE_FOCUS ) == SDL_WINDOW_MOUSE_FOCUS );
        }

        void _createPalette()
        {
            if ( _surface == nullptr )
                return;

            updatePalette( StandardPaletteIndexes() );

            if ( _surface->format->BitsPerPixel == 8 ) {
                if ( !SDL_MUSTLOCK( _surface ) ) {
                    // copy the image from display buffer to SDL surface
                    const fheroes2::Display & display = fheroes2::Display::instance();
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

        bool _retrieveWindowInfo()
        {
            assert( _window != nullptr );

            const int displayIndex = SDL_GetWindowDisplayIndex( _window );
            if ( displayIndex < 0 ) {
                ERROR_LOG( "Failed to get window display index. The error value: " << displayIndex << ", description: " << SDL_GetError() )
                return false;
            }

            SDL_DisplayMode displayMode;

            const int returnCode = SDL_GetCurrentDisplayMode( displayIndex, &displayMode );
            if ( returnCode < 0 ) {
                ERROR_LOG( "Failed to retrieve current display mode. The error value: " << returnCode << ", description: " << SDL_GetError() )
                return false;
            }

            _currentScreenResolution.width = displayMode.w;
            _currentScreenResolution.height = displayMode.h;

#if defined( TARGET_NINTENDO_SWITCH )
            // On a Nintendo Switch the game is always fullscreen
            _activeWindowROI = { 0, 0, _currentScreenResolution.width, _currentScreenResolution.height };
#else
            SDL_GetWindowPosition( _window, &_activeWindowROI.x, &_activeWindowROI.y );
            SDL_GetWindowSize( _window, &_activeWindowROI.width, &_activeWindowROI.height );
#endif

            return true;
        }

        void _toggleVSync()
        {
            assert( _renderer != nullptr );

#if !SDL_VERSION_ATLEAST( 2, 0, 18 )
#error SDL_RenderSetVSync() is only supported since SDL 2.0.18
#endif

            if ( const int returnCode = SDL_RenderSetVSync( _renderer, _isVSyncEnabled ? SDL_ENABLE : SDL_DISABLE ); returnCode != 0 ) {
                ERROR_LOG( "Failed to " << ( _isVSyncEnabled ? "enable" : "disable" ) << " vsync mode for renderer. The error value: " << returnCode
                                        << ", description: " << SDL_GetError() )
            }
        }

        void _toggleMouseCaptureMode()
        {
            assert( _window != nullptr );

            // To properly support fullscreen mode on devices with multiple displays or devices with notch,
            // it is important to lock the mouse in the application window area.
            SDL_SetWindowGrab( _window, isFullScreen() ? SDL_TRUE : SDL_FALSE );
        }

        void _syncFullScreen()
        {
            if ( isFullScreen() == BaseRenderEngine::isFullScreen() ) {
                return;
            }

            BaseRenderEngine::toggleFullScreen();

            assert( isFullScreen() == BaseRenderEngine::isFullScreen() );
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
    {
        _disableTransformLayer();
    }

    void Display::resize( int32_t width_, int32_t height_ )
    {
        assert( width_ == width() && height_ == height() );

#ifdef NDEBUG
        (void)width_;
        (void)height_;
#endif
    }

    void Display::setResolution( ResolutionInfo info )
    {
        if ( width() > 0 && height() > 0 && info.gameWidth == width() && info.gameHeight == height() && info.screenWidth == _screenSize.width
             && info.screenHeight == _screenSize.height ) // nothing to resize
            return;

        const bool isFullScreen = _engine->isFullScreen();

        // deallocate engine resources
        _engine->clear();

        _prevRoi = {};

        // allocate engine resources
        if ( !_engine->allocate( info, isFullScreen ) ) {
            clear();
        }

        Image::resize( info.gameWidth, info.gameHeight );
        Image::reset();

        _screenSize = { info.screenWidth, info.screenHeight };
    }

    void Display::setWindowPos( const Point point )
    {
        _engine->setWindowPos( point );
    }

    Display & Display::instance()
    {
        static Display display;
        return display;
    }

    void Display::render( const Rect & roi )
    {
        Rect temp( roi );
        if ( !getActiveArea( temp, width(), height() ) ) {
            return;
        }

        if ( _cursor->isVisible() && _cursor->isSoftwareEmulation() && !_cursor->_image.empty() ) {
            const Sprite & cursorImage = _cursor->_image;
            Rect cursorROI( cursorImage.x(), cursorImage.y(), cursorImage.width(), cursorImage.height() );

            if ( _cursor->_keepInScreenArea ) {
                cursorROI.x = std::clamp( cursorROI.x, 0, width() - cursorROI.width );
                cursorROI.y = std::clamp( cursorROI.y, 0, height() - cursorROI.height );
            }

            const Sprite backup = Crop( *this, cursorROI.x, cursorROI.y, cursorROI.width, cursorROI.height );
            Blit( cursorImage, 0, 0, *this, cursorROI.x, cursorROI.y, cursorROI.width, cursorROI.height );

            // ROI must include cursor's area as well, otherwise cursor won't be rendered.
            if ( !backup.empty() && getActiveArea( cursorROI, width(), height() ) ) {
                temp = getBoundaryRect( temp, cursorROI );
            }

            // Previous position of cursor must be updated as well to avoid ghost effect.
            _renderFrame( getBoundaryRect( temp, _prevRoi ) );

            if ( _postprocessing ) {
                _postprocessing();
            }

            Copy( backup, 0, 0, *this, backup.x(), backup.y(), backup.width(), backup.height() );
        }
        else {
            _renderFrame( getBoundaryRect( temp, _prevRoi ) );

            if ( _postprocessing ) {
                _postprocessing();
            }
        }

        _prevRoi = temp;
    }

    void Display::updateNextRenderRoi( const Rect & roi )
    {
        _prevRoi = getBoundaryRect( _prevRoi, roi );
        getActiveArea( _prevRoi, width(), height() );
    }

    void Display::_renderFrame( const Rect & roi ) const
    {
        bool updateImage = true;
        if ( _preprocessing ) {
            std::vector<uint8_t> palette;
            if ( _preprocessing( palette ) ) {
                _engine->updatePalette( palette );
                // when we change a palette for 8-bit image we unwillingly call render so we don't need to re-render the same frame again
                updateImage = ( _renderSurface == nullptr );
                if ( updateImage ) {
                    // Pre-processing step is applied to the whole image so we forcefully render the full frame.
                    _engine->render( *this, { 0, 0, width(), height() } );
                    return;
                }
            }
        }

        if ( updateImage ) {
            _engine->render( *this, roi );
        }
    }

    uint8_t * Display::image()
    {
        return _renderSurface != nullptr ? _renderSurface : Image::image();
    }

    const uint8_t * Display::image() const
    {
        return _renderSurface != nullptr ? _renderSurface : Image::image();
    }

    void Display::release()
    {
        _engine->clear();
        _cursor.reset();
        clear();

        _prevRoi = {};
    }

    void Display::changePalette( const uint8_t * palette, const bool forceDefaultPaletteUpdate ) const
    {
        if ( currentPalette == palette || ( palette == nullptr && currentPalette == PALPalette() && !forceDefaultPaletteUpdate ) )
            return;

        currentPalette = ( palette == nullptr ) ? PALPalette( forceDefaultPaletteUpdate ) : palette;

        _engine->updatePalette( StandardPaletteIndexes() );
    }

    bool Cursor::isFocusActive() const
    {
        return engine().isMouseCursorActive();
    }

    BaseRenderEngine & engine()
    {
        const fheroes2::Display & display = Display::instance();

        assert( display._engine );

        return *( display._engine );
    }

    Cursor & cursor()
    {
        const fheroes2::Display & display = Display::instance();

        assert( display._cursor );

        return *( display._cursor );
    }
}
