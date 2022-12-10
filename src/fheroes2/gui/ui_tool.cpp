/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2022                                             *
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

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <string>
#include <utility>

#include "localevent.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "translations.h"

namespace
{
    // Renderer of current time and FPS on screen
    class SystemInfoRenderer
    {
    public:
        SystemInfoRenderer()
            : _startTime( std::chrono::steady_clock::now() )
        {}

        void preRender()
        {
            if ( !Settings::Get().isSystemInfoEnabled() )
                return;

            const int32_t offsetX = 26;
            const int32_t offsetY = fheroes2::Display::instance().height() - 30;

            const tm tmi = System::GetTM( std::time( nullptr ) );

            char mbstr[10] = { 0 };
            std::strftime( mbstr, sizeof( mbstr ), "%H:%M:%S", &tmi );

            std::string info( mbstr );

            std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();
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

            info += _( ", FPS: " );
            info += std::to_string( currentFps );
            if ( averageFps < 10 ) {
                info += '.';
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
        std::chrono::time_point<std::chrono::steady_clock> _startTime;
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

    MovableSprite::~MovableSprite()
    {
        if ( _isHidden ) {
            _restorer.reset();
        }
    }

    MovableSprite & MovableSprite::operator=( const Sprite & sprite )
    {
        Sprite::operator=( sprite );
        _restorer.update( x(), y(), width(), height() );
        return *this;
    }

    void MovableSprite::setPosition( int32_t x_, int32_t y_ )
    {
        if ( _isHidden ) {
            Sprite::setPosition( x_, y_ );
            return;
        }

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

    TimedEventValidator::TimedEventValidator( std::function<bool()> verification, const uint64_t delayBeforeFirstUpdateMs, const uint64_t delayBetweenUpdateMs )
        : _verification( std::move( verification ) )
        , _delayBetweenUpdateMs( delayBetweenUpdateMs )
        , _delayBeforeFirstUpdateMs( delayBeforeFirstUpdateMs )
    {}

    bool TimedEventValidator::isDelayPassed()
    {
        if ( _delayBeforeFirstUpdateMs.isPassed() && _delayBetweenUpdateMs.isPassed() && _verification() ) {
            _delayBetweenUpdateMs.reset();
            return true;
        }
        return false;
    }

    void TimedEventValidator::senderUpdate( const ActionObject * sender )
    {
        if ( sender == nullptr )
            return;
        _delayBeforeFirstUpdateMs.reset();
        _delayBetweenUpdateMs.reset();
    }

    ScreenPaletteRestorer::ScreenPaletteRestorer()
    {
        LocalEvent::PauseCycling();
    }

    ScreenPaletteRestorer::~ScreenPaletteRestorer()
    {
        Display::instance().changePalette( nullptr );
        LocalEvent::ResumeCycling();
    }

    void ScreenPaletteRestorer::changePalette( const uint8_t * palette ) const
    {
        Display::instance().changePalette( palette );
    }

    GameInterfaceTypeRestorer::GameInterfaceTypeRestorer( const bool isEvilInterface_ )
        : isEvilInterface( isEvilInterface_ )
        , isOriginalEvilInterface( Settings::Get().isEvilInterfaceEnabled() )
    {
        if ( isEvilInterface != isOriginalEvilInterface ) {
            Settings::Get().setEvilInterface( isEvilInterface );
        }
    }

    GameInterfaceTypeRestorer::~GameInterfaceTypeRestorer()
    {
        if ( isEvilInterface != isOriginalEvilInterface ) {
            Settings::Get().setEvilInterface( isOriginalEvilInterface );
        }
    }

    Image CreateDeathWaveEffect( const Image & in, const int32_t x, const int32_t waveWidth, const std::vector<int32_t> & deathWaveCurve )
    {
        if ( in.empty() )
            return Image();

        Image out = in;

        const int32_t width = in.width();
        // If the death wave curve is outside of the battlefield - return the original image.
        if ( x < 0 || ( x - waveWidth ) >= width )
            return out;

        const int32_t height = in.height();

        // Set the image horizontal offset from where to draw the wave.
        const int32_t offsetX = x < waveWidth ? 0 : x - waveWidth;
        uint8_t * outImageX = out.image() + offsetX;
        const uint8_t * inImageX = in.image() + offsetX;

        // Set pointers to the start and the end of the death wave curve.
        std::vector<int32_t>::const_iterator pntX = deathWaveCurve.begin() + ( x < waveWidth ? waveWidth - x : 0 );
        const std::vector<int32_t>::const_iterator endX = deathWaveCurve.end() - ( x > width ? x - width : 0 );

        for ( ; pntX != endX; ++pntX, ++outImageX, ++inImageX ) {
            const uint8_t * outImageYEnd = outImageX + ( height + *pntX ) * width;
            const uint8_t * inImageY = inImageX - ( *pntX + 1 ) * width;

            // A loop to shift all horizontal pixels vertically.
            uint8_t * outImageY = outImageX;
            for ( ; outImageY != outImageYEnd; outImageY += width ) {
                inImageY += width;
                *outImageY = *inImageY;
            }

            // Flip the image under the death wave to create a distortion effect.
            for ( int32_t i = 0; i > *pntX; --i ) {
                *outImageY = *inImageY;
                outImageY += width;
                inImageY -= width;
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

    void FadeDisplay( const Image & top, const Point & pos, uint8_t endAlpha, int delayMs )
    {
        Display & display = Display::instance();

        Image shadow = top;
        uint8_t alpha = 255;
        const uint8_t step = 10;
        const uint8_t min = step + 5;
        const int stepDelay = ( delayMs * step ) / ( alpha - min );

        while ( alpha > min + endAlpha ) {
            ApplyAlpha( top, shadow, alpha );
            Copy( shadow, 0, 0, display, pos.x, pos.y, shadow.width(), shadow.height() );

            display.render();

            alpha -= step;
            delayforMs( stepDelay );
        }
    }

    void FadeDisplayWithPalette( const Image & top, const Point & pos, uint8_t paletteId, int delayMs, int frameCount )
    {
        Display & display = Display::instance();
        const int stepDelay = delayMs / frameCount;

        Image shadow = top;

        for ( int i = 0; i < frameCount; ++i ) {
            ApplyPalette( shadow, paletteId );
            Copy( shadow, 0, 0, display, pos.x, pos.y, shadow.width(), shadow.height() );

            display.render();

            delayforMs( stepDelay );
        }
    }

    void FadeDisplay( int delayMs )
    {
        Display & display = Display::instance();
        Image temp;
        Copy( display, temp );

        FadeDisplay( temp, { 0, 0 }, 5, delayMs );

        Copy( temp, display ); // restore the original image
    }

    void InvertedFadeWithPalette( Image & image, const Rect & roi, const Rect & excludedRoi, uint8_t paletteId, int delayMs, int frameCount )
    {
        Display & display = Display::instance();
        const int stepDelay = delayMs / frameCount;

        for ( int i = 0; i < frameCount; ++i ) {
            InvertedShadow( image, roi, excludedRoi, paletteId, 1 );

            display.render();

            delayforMs( stepDelay );
        }
    }

    void InvertedShadow( Image & image, const Rect & roi, const Rect & excludedRoi, const uint8_t paletteId, const int paletteCount )
    {
        // Identify 4 areas around excluded ROI to be used for palette application.
        const Rect topRoi( roi.x, roi.y, roi.width, excludedRoi.y - roi.y );
        const Rect bottomRoi( roi.x, excludedRoi.y + excludedRoi.height, roi.width, roi.y + roi.height - excludedRoi.y - excludedRoi.height );
        const Rect leftRoi( roi.x, excludedRoi.y, excludedRoi.x - roi.x, excludedRoi.height );
        const Rect rightRoi( excludedRoi.x + excludedRoi.width, excludedRoi.y, roi.x + roi.width - excludedRoi.x - excludedRoi.width, excludedRoi.height );

        for ( int i = 0; i < paletteCount; ++i ) {
            ApplyPalette( image, topRoi.x, topRoi.y, image, topRoi.x, topRoi.y, topRoi.width, topRoi.height, paletteId );
            ApplyPalette( image, bottomRoi.x, bottomRoi.y, image, bottomRoi.x, bottomRoi.y, bottomRoi.width, bottomRoi.height, paletteId );
            ApplyPalette( image, leftRoi.x, leftRoi.y, image, leftRoi.x, leftRoi.y, leftRoi.width, leftRoi.height, paletteId );
            ApplyPalette( image, rightRoi.x, rightRoi.y, image, rightRoi.x, rightRoi.y, rightRoi.width, rightRoi.height, paletteId );
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
