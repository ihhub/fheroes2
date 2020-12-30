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

#include "ui_tool.h"
#include "localevent.h"
#include "screen.h"
#include "settings.h"
#include "text.h"
#include "types.h"

#include <chrono>
#include <cstring>
#include <ctime>
#include <deque>

namespace
{
    // Renderer of current time and FPS on screen
    class SystemInfoRenderer
    {
    public:
        SystemInfoRenderer()
            : _startTime( std::chrono::high_resolution_clock::now() )
        {}

        void preRender()
        {
            if ( !Settings::Get().ExtGameShowSystemInfo() )
                return;

            const int32_t offsetX = 26;
            const int32_t offsetY = fheroes2::Display::instance().height() - 30;

            time_t rawtime;
            std::time( &rawtime );
            // strtime format: Www Mmm dd hh:mm:ss yyyy
            const char * timeFormat = std::ctime( &rawtime );

            std::string info( &timeFormat[11], 8 );

            std::chrono::time_point<std::chrono::high_resolution_clock> endTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double> time = endTime - _startTime;
            _startTime = endTime;

            const double totalTime = time.count() * 1000.0;
            const double fps = totalTime < 1 ? 0 : 1000 / totalTime;

            _fps.push_front( fps );
            while ( _fps.size() > 10 )
                _fps.pop_back();

            double averageFps = 0;
            for ( const double value : _fps )
                averageFps += value;

            averageFps /= static_cast<double>( _fps.size() );
            const int currentFps = static_cast<int>( averageFps );

            info += ", FPS: ";
            info += std::to_string( currentFps );
            if ( averageFps < 10 ) {
                info += ".";
                info += std::to_string( static_cast<int>( ( averageFps - currentFps ) * 10 ) );
            }

            _text.SetPos( offsetX, offsetY );
            _text.SetText( info );
            _text.Show();
        }

        void postRender()
        {
            if ( _text.isShow() )
                _text.Hide();
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;
        TextSprite _text;
        std::deque<double> _fps;
    };

    SystemInfoRenderer systemInfoRenderer;
}

namespace fheroes2
{
    MovableSprite::MovableSprite()
        : _restorer( Display::instance(), 0, 0, 0, 0 )
        , _isHidden( true )
    {}

    MovableSprite::MovableSprite( int32_t width_, int32_t height_, int32_t x_, int32_t y_ )
        : Sprite( width_, height_, x_, y_ )
        , _restorer( Display::instance(), x_, y_, width_, height_ )
        , _isHidden( height_ == 0 && width_ == 0 )
    {}

    MovableSprite::MovableSprite( const Sprite & sprite )
        : Sprite( sprite )
        , _restorer( Display::instance(), 0, 0, 0, 0 )
        , _isHidden( false )
    {}

    MovableSprite::~MovableSprite() {}

    MovableSprite & MovableSprite::operator=( const Sprite & sprite )
    {
        Sprite::operator=( sprite );
        _restorer.update( x(), y(), width(), height() );
        return *this;
    }

    void MovableSprite::setPosition( int32_t x_, int32_t y_ )
    {
        hide();
        Sprite::setPosition( x_, y_ );
        show();
    }

    void MovableSprite::show()
    {
        if ( _isHidden ) {
            _restorer.update( x(), y(), width(), height() );
            fheroes2::Blit( *this, Display::instance(), x(), y() );
            _isHidden = false;
        }
    }

    void MovableSprite::hide()
    {
        if ( !_isHidden ) {
            _restorer.restore();
            _isHidden = true;
        }
    }

    void MovableSprite::redraw()
    {
        hide();
        show();
    }

    bool MovableSprite::isHidden() const
    {
        return _isHidden;
    }

    ScreenPaletteRestorer::ScreenPaletteRestorer()
    {
        LocalEvent::Get().PauseCycling();
    }

    ScreenPaletteRestorer::~ScreenPaletteRestorer()
    {
        Display::instance().changePalette( NULL );
        LocalEvent::Get().ResumeCycling();
    }

    void ScreenPaletteRestorer::changePalette( const uint8_t * palette )
    {
        Display::instance().changePalette( palette );
    }

    Image CreateDeathWaveEffect( const Image & in, int32_t x, int32_t waveWidth, int32_t waveHeight )
    {
        if ( in.empty() )
            return Image();

        Image out = in;

        const int32_t width = in.width();
        const int32_t height = in.height();

        if ( x + waveWidth < 0 || x - waveWidth >= width )
            return out;

        const int32_t startX = ( x > waveWidth ) ? x - waveWidth : 0;
        const int32_t endX = ( x + waveWidth < width ) ? x + waveWidth : width;

        const double pi = std::acos( -1 );
        const double waveLimit = waveWidth / pi;

        uint8_t * outImageX = out.image() + startX;
        uint8_t * outTransformX = out.transform() + startX;

        const uint8_t * inImageX = in.image() + startX;
        const uint8_t * inTransformX = in.transform() + startX;

        for ( int32_t posX = startX; posX < endX; ++posX, ++outImageX, ++outTransformX, ++inImageX, ++inTransformX ) {
            const int32_t waveX = posX - x;

            const int32_t offsetY = static_cast<int32_t>( waveHeight * ( ( waveX < waveLimit ) ? tan( waveX / waveLimit ) / 2 : sin( waveX / waveLimit ) ) );

            const int32_t offsetOut = offsetY >= 0 ? offsetY * width : 0;
            const int32_t offsetOutEnd = offsetY >= 0 ? ( height - 1 - offsetY ) * width : ( height - 1 + offsetY ) * width;

            uint8_t * outImageY = outImageX + offsetOut;
            uint8_t * outTransformY = outTransformX + offsetOut;
            uint8_t * outImageYEnd = outImageX + offsetOutEnd;

            const int32_t offsetIn = offsetY >= 0 ? 0 : -offsetY * width;

            const uint8_t * inImageY = inImageX + offsetIn;
            const uint8_t * inTransformY = inTransformX + offsetIn;

            for ( ; outImageY != outImageYEnd; outImageY += width, outTransformY += width, inImageY += width, inTransformY += width ) {
                *outImageY = *inImageY;
                *outTransformY = *inTransformY;
            }
        }

        return out;
    }

    Image CreateRippleEffect( const Image & in, int32_t frameId, double scaleX, double waveFrequency )
    {
        if ( in.empty() )
            return Image();

        const int32_t widthIn = in.width();
        const int32_t height = in.height();

        // convert frames to -10...10 range with a period of 40
        const int32_t linearWave = std::abs( 20 - ( frameId + 10 ) % 40 ) - 10;
        const int32_t progress = 7 - frameId / 10;

        const double rippleXModifier = ( progress * scaleX + 0.3 ) * linearWave;
        const int32_t offsetX = static_cast<int32_t>( std::abs( rippleXModifier ) );
        const double pi = std::acos( -1 );
        const int32_t limitY = static_cast<int32_t>( waveFrequency * pi );

        Image out( widthIn + offsetX * 2, height );
        out.reset();

        const int32_t widthOut = out.width();

        uint8_t * outImageY = out.image();
        uint8_t * outTransformY = out.transform();

        const uint8_t * inImageY = in.image();
        const uint8_t * inTransformY = in.transform();

        for ( int32_t y = 0; y < height; ++y, inImageY += widthIn, inTransformY += widthIn, outImageY += widthOut, outTransformY += widthOut ) {
            // Take top half the sin wave starting at 0 with period set by waveFrequency, result is -1...1
            const double sinYEffect = sin( ( y % limitY ) / waveFrequency ) * 2.0 - 1;
            const int32_t offset = static_cast<int32_t>( rippleXModifier * sinYEffect ) + offsetX;

            memcpy( outImageY + offset, inImageY, widthIn );
            memcpy( outTransformY + offset, inTransformY, widthIn );
        }

        return out;
    }

    void FadeDisplay( const Image & top, const Point & pos, uint8_t endAlpha, int delay )
    {
        Display & display = Display::instance();

        Image shadow = top;
        uint8_t alpha = 255;
        const uint8_t step = 10;
        const uint8_t min = step + 5;
        const int stepDelay = ( delay * step ) / ( alpha - min );

        while ( alpha > min + endAlpha ) {
            ApplyAlpha( top, shadow, alpha );
            Copy( shadow, 0, 0, display, pos.x, pos.y, shadow.width(), shadow.height() );

            display.render();

            alpha -= step;
            DELAY( stepDelay );
        }
    }

    void FadeDisplayWithPalette( const Image & top, const Point & pos, uint8_t paletteId, int delay, int frameCount )
    {
        Display & display = Display::instance();
        const int stepDelay = delay / frameCount;

        Image shadow = top;

        for ( int i = 0; i < frameCount; ++i ) {
            ApplyPalette( shadow, paletteId );
            Copy( shadow, 0, 0, display, pos.x, pos.y, shadow.width(), shadow.height() );

            display.render();

            DELAY( stepDelay );
        }
    }

    void FadeDisplay( int delay )
    {
        Display & display = Display::instance();
        const Image temp = display;

        FadeDisplay( temp, Point( 0, 0 ), 5, delay );

        Copy( temp, display ); // restore the original image
    }

    void InvertedFade( const Image & top, const Point & offset, const Image & middle, const Point & middleOffset, uint8_t endAlpha, int delay )
    {
        Display & display = Display::instance();
        Image shadow = top;
        uint8_t alpha = 255;
        const uint8_t step = 10;
        const uint8_t min = step + 5;
        const int stepDelay = ( delay * step ) / ( alpha - min );

        while ( alpha > min + endAlpha ) {
            ApplyAlpha( top, shadow, alpha );
            Copy( shadow, 0, 0, display, offset.x, offset.y, shadow.width(), shadow.height() );
            Copy( middle, 0, 0, display, middleOffset.x, middleOffset.y, middle.width(), middle.height() );

            display.render();

            alpha -= step;
            DELAY( stepDelay );
        }
    }

    void InvertedFadeWithPalette( const Image & top, const Point & offset, const Image & middle, const Point & middleOffset, uint8_t paletteId, int delay,
                                  int frameCount )
    {
        Display & display = Display::instance();
        Image shadow = top;
        const int stepDelay = delay / frameCount;

        for ( int i = 0; i < frameCount; ++i ) {
            ApplyPalette( shadow, paletteId );
            Copy( shadow, 0, 0, display, offset.x, offset.y, shadow.width(), shadow.height() );
            Copy( middle, 0, 0, display, middleOffset.x, middleOffset.y, middle.width(), middle.height() );

            display.render();

            DELAY( stepDelay );
        }
    }

    void PreRenderSystemInfo()
    {
        systemInfoRenderer.preRender();
    }

    void PostRenderSystemInfo()
    {
        systemInfoRenderer.postRender();
    }
}
