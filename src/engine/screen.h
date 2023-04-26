/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "image.h"
#include "math_base.h"

namespace fheroes2
{
    class Cursor;
    class Display;

    struct ResolutionInfo
    {
        ResolutionInfo() = default;

        ResolutionInfo( const int32_t gameWidth_, const int32_t gameHeight_ )
            : gameWidth( gameWidth_ )
            , gameHeight( gameHeight_ )
            , screenWidth( gameWidth_ )
            , screenHeight( gameHeight_ )
        {
            // Do nothing.
        }

        ResolutionInfo( const int32_t gameWidth_, const int32_t gameHeight_, const int32_t screenWidth_, const int32_t screenHeight_ )
            : gameWidth( gameWidth_ )
            , gameHeight( gameHeight_ )
            , screenWidth( screenWidth_ )
            , screenHeight( screenHeight_ )
        {
            // Do nothing.
        }

        bool operator==( const ResolutionInfo & info ) const
        {
            return gameWidth == info.gameWidth && gameHeight == info.gameHeight && screenWidth == info.screenWidth && screenHeight == info.screenHeight;
        }

        bool operator!=( const ResolutionInfo & info ) const
        {
            return !operator==( info );
        }

        bool operator<( const ResolutionInfo & info ) const
        {
            return std::tie( gameWidth, gameHeight, screenWidth, screenHeight ) < std::tie( info.gameWidth, info.gameHeight, info.screenWidth, info.screenHeight );
        }

        int32_t gameWidth{ 0 };

        int32_t gameHeight{ 0 };

        int32_t screenWidth{ 0 };

        int32_t screenHeight{ 0 };
    };

    class BaseRenderEngine
    {
    public:
        friend class Cursor;
        friend class Display;

        virtual ~BaseRenderEngine() = default;

        virtual void toggleFullScreen()
        {
            _isFullScreen = !_isFullScreen;
        }

        virtual bool isFullScreen() const
        {
            return _isFullScreen;
        }

        virtual std::vector<ResolutionInfo> getAvailableResolutions() const
        {
            return {};
        }

        virtual void setTitle( const std::string & )
        {
            // Do nothing.
        }

        virtual void setIcon( const Image & )
        {
            // Do nothing.
        }

        virtual Rect getActiveWindowROI() const
        {
            return {};
        }

        virtual Size getCurrentScreenResolution() const
        {
            return {};
        }

        virtual void setVSync( const bool )
        {
            // Do nothing.
        }

        void setNearestScaling( const bool enable )
        {
            _nearestScaling = enable;
        }

        bool isNearestScaling() const
        {
            return _nearestScaling;
        }

    protected:
        BaseRenderEngine()
            : _isFullScreen( false )
            , _nearestScaling( false )
        {
            // Do nothing.
        }

        virtual void clear()
        {
            // Do nothing.
        }

        virtual void render( const Display &, const Rect & )
        {
            // Do nothing.
        }

        virtual bool allocate( ResolutionInfo & /*unused*/, bool /*unused*/ )
        {
            return false;
        }

        virtual bool isMouseCursorActive() const
        {
            return false;
        }

        // To support color cycling we need to update palette.
        virtual void updatePalette( const std::vector<uint8_t> & )
        {
            // Do nothing.
        }

        void linkRenderSurface( uint8_t * surface ) const; // declaration of this method is in source file

    private:
        bool _isFullScreen;

        bool _nearestScaling;
    };

    class Display : public Image
    {
    public:
        friend class BaseRenderEngine;

        enum : int32_t
        {
            DEFAULT_WIDTH = 640,
            DEFAULT_HEIGHT = 480
        };

        static Display & instance();

        ~Display() override = default;

        // Render a full frame on screen.
        void render()
        {
            render( { 0, 0, width(), height() } );
        }

        void render( const Rect & roi ); // render a part of image on screen. Prefer this method over full image if you don't draw full screen.

        // Update the area which will be rendered on the next render() call.
        void updateNextRenderRoi( const Rect & roi );

        // Do not call this method. It serves as a patch over the basic class.
        void resize( int32_t width_, int32_t height_ ) override;

        void setResolution( ResolutionInfo info );

        bool isDefaultSize() const
        {
            return width() == DEFAULT_WIDTH && height() == DEFAULT_HEIGHT;
        }

        // this function must return true if new palette has been generated
        using PreRenderProcessing = bool ( * )( std::vector<uint8_t> & palette );
        using PostRenderProcessing = void ( * )();

        void subscribe( PreRenderProcessing preprocessing, PostRenderProcessing postprocessing )
        {
            _preprocessing = preprocessing;
            _postprocessing = postprocessing;
        }

        // For 8-bit mode we return a pointer to direct surface which we draw on screen
        uint8_t * image() override;
        const uint8_t * image() const override;

        void release(); // to release all allocated resources. Should be used at the end of the application

        // Change the whole color representation on the screen. Make sure that palette exists all the time!!!
        // nullptr input parameter is used to reset palette to default one.
        void changePalette( const uint8_t * palette = nullptr, const bool forceDefaultPaletteUpdate = false ) const;

        Size screenSize() const
        {
            return _screenSize;
        }

        friend BaseRenderEngine & engine();
        friend Cursor & cursor();

    private:
        std::unique_ptr<BaseRenderEngine> _engine;
        std::unique_ptr<Cursor> _cursor;
        PreRenderProcessing _preprocessing;
        PostRenderProcessing _postprocessing;

        uint8_t * _renderSurface;

        // Previous area drawn on the screen.
        Rect _prevRoi;

        Size _screenSize;

        // Only for cases of direct drawing on rendered 8-bit image.
        void linkRenderSurface( uint8_t * surface )
        {
            _renderSurface = surface;
        }

        Display();

        void _renderFrame( const Rect & roi ) const; // prepare and render a frame
    };

    class Cursor
    {
    public:
        friend Display;
        virtual ~Cursor() = default;

        virtual void show( const bool enable )
        {
            _show = enable;
        }

        virtual bool isVisible() const
        {
            return _show;
        }

        bool isFocusActive() const;

        virtual void update( const Image & image, int32_t offsetX, int32_t offsetY )
        {
            _image = Sprite( image, offsetX, offsetY );
        }

        void setPosition( int32_t x, int32_t y )
        {
            _image.setPosition( x, y );
        }

        // Default implementation of Cursor uses software emulation.
        virtual void enableSoftwareEmulation( const bool )
        {
            // Do nothing.
        }

        bool isSoftwareEmulation() const
        {
            return _emulation;
        }

        void registerUpdater( void ( *cursorUpdater )() )
        {
            _cursorUpdater = cursorUpdater;
        }

    protected:
        Sprite _image;
        bool _emulation;
        bool _show;
        void ( *_cursorUpdater )();

        Cursor()
            : _emulation( true )
            , _show( false )
            , _cursorUpdater( nullptr )
        {}
    };

    BaseRenderEngine & engine();
    Cursor & cursor();
}
