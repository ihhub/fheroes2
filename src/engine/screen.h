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

namespace fheroes2
{
    class Cursor;
    class Display;

    class BaseRenderEngine
    {
    public:
        friend class Cursor;
        friend class Display;
        virtual ~BaseRenderEngine() {}

        virtual void toggleFullScreen()
        {
            _isFullScreen = !_isFullScreen;
        }

        virtual bool isFullScreen() const
        {
            return _isFullScreen;
        }

        virtual std::vector<std::pair<int, int> > getAvailableResolutions() const
        {
            return std::vector<std::pair<int, int> >();
        }

        virtual void setTitle( const std::string & ) {}

        virtual void setIcon( const Image & ) {}

    protected:
        BaseRenderEngine()
            : _isFullScreen( false )
        {}

        virtual void clear() {}
        virtual void render( const Display & ) {}
        virtual bool allocate( uint32_t, uint32_t, bool )
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
        enum
        {
            DEFAULT_WIDTH = 640,
            DEFAULT_HEIGHT = 480
        };

        static Display & instance();

        virtual ~Display();

        void render(); // render the image on screen

        virtual void resize( int32_t width_, int32_t height_ ) override;
        bool isDefaultSize() const;

        // this function must return true if new palette has been generated
        typedef bool ( *PreRenderProcessing )( std::vector<uint8_t> & palette );
        typedef void ( *PostRenderProcessing )();
        void subscribe( PreRenderProcessing preprocessing, PostRenderProcessing postprocessing );

        // For 8-bit mode we return a pointer to direct surface which we draw on screen
        virtual uint8_t * image() override;
        virtual const uint8_t * image() const override;

        BaseRenderEngine * engine();

    private:
        BaseRenderEngine * _engine;
        PreRenderProcessing _preprocessing;
        PostRenderProcessing _postprocessing;

        void linkRenderSurface( uint8_t * surface ); // only for cases of direct drawing on rendered 8-bit image

        uint8_t * _renderSurface;

        Display();

        void _renderFrame(); // prepare and render a frame
    };

    class Cursor : public Sprite
    {
    public:
        virtual ~Cursor();

        static Cursor & instance();

        void show( bool enable );
        bool isVisible() const;

        bool isFocusActive() const;

    private:
        Cursor();

        bool _show;
    };

    BaseRenderEngine & engine();
}
