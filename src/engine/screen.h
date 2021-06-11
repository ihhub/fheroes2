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
#pragma once

#include "image.h"

#include <memory>

namespace fheroes2
{
    class Cursor;
    class Display;

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

        virtual std::vector<Size> getAvailableResolutions() const
        {
            return std::vector<Size>();
        }

        virtual void setTitle( const std::string & ) {}

        virtual void setIcon( const Image & ) {}

        virtual fheroes2::Rect getActiveWindowROI() const
        {
            return fheroes2::Rect();
        }

        virtual fheroes2::Size getCurrentScreenResolution() const
        {
            return fheroes2::Size();
        }

    protected:
        BaseRenderEngine()
            : _isFullScreen( false )
        {}

        virtual void clear() {}
        virtual void render( const Display &, const Rect & ) {}
        virtual bool allocate( int32_t &, int32_t &, bool )
        {
            return false;
        }

        virtual bool isMouseCursorActive() const
        {
            return false;
        }

        // to support color cycling we need to update palette
        virtual void updatePalette( const std::vector<uint8_t> & ) {}

        void linkRenderSurface( uint8_t * surface ) const; // declaration of this method is in source file

    private:
        bool _isFullScreen;
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

        void render(); // render full image on screen
        void render( const Rect & roi ); // render a part of image on screen. Prefer this method over full image if you don't draw full screen.

        void resize( int32_t width_, int32_t height_ ) override;
        bool isDefaultSize() const;

        // this function must return true if new palette has been generated
        using PreRenderProcessing = bool ( * )( std::vector<uint8_t> & palette );
        using PostRenderProcessing = void ( * )();
        void subscribe( PreRenderProcessing preprocessing, PostRenderProcessing postprocessing );

        // For 8-bit mode we return a pointer to direct surface which we draw on screen
        uint8_t * image() override;
        const uint8_t * image() const override;

        void release(); // to release all allocated resources. Should be used at the end of the application

        // Change whole color representation on the screen. Make sure that palette exists all the time!!!
        // NULL input parameters means to set to default value
        void changePalette( const uint8_t * palette = nullptr ) const;

        friend BaseRenderEngine & engine();
        friend Cursor & cursor();

        void setEngine( std::unique_ptr<BaseRenderEngine> & engine );
        void setCursor( std::unique_ptr<Cursor> & cursor );

    private:
        std::unique_ptr<BaseRenderEngine> _engine;
        std::unique_ptr<Cursor> _cursor;
        PreRenderProcessing _preprocessing;
        PostRenderProcessing _postprocessing;

        uint8_t * _renderSurface;

        // Previous area drawn on the screen.
        Rect _prevRoi;

        void linkRenderSurface( uint8_t * surface ); // only for cases of direct drawing on rendered 8-bit image

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

        virtual void update( const fheroes2::Image & image, int32_t offsetX, int32_t offsetY )
        {
            _image = fheroes2::Sprite( image, offsetX, offsetY );
        }

        void setPosition( int32_t x, int32_t y )
        {
            _image.setPosition( x, y );
        }

        // Default implementation of Cursor uses software emulation.
        virtual void enableSoftwareEmulation( const bool ) {}

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
