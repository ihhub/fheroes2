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

#include "ui_tool.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <string>
#include <utility>

#include "game_delays.h"
#include "image_palette.h"
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

    void colorFade( const std::vector<uint8_t> & palette, const fheroes2::Rect & frameRoi, const uint32_t durationMs, const uint32_t fps )
    {
        // Game palette has 256 values for red, green and blue, so its size is: 256 * 3 = 768.
        const int32_t paletteSize = 768;
        // Do a color fade only for valid palette.
        if ( palette.size() != paletteSize ) {
            return;
        }

        // The biggest problem here is that the palette could be different from the one in the game.
        // Since we want to do color fading to grayscale colors we take the original palette and
        // find the nearest colors in video's palette to grayscale colors of the original palette.
        // Then we gradually change the current palette to be only grayscale and after reaching the
        // last frame change all colors of the frame to be only grayscale colors of the original palette.

        const uint8_t * originalPalette = fheroes2::getGamePalette();

        // Yes, these values are hardcoded. There are ways to do it programmatically.
        const int32_t startGrayScaleColorId = 10;
        const int32_t endGrayScaleColorId = 36;

        // The game's palette has 256 color indexes.
        const int32_t paletteIndexes = 256;

        std::vector<uint8_t> assignedValue( paletteIndexes );

        for ( size_t id = 0; id < paletteIndexes; ++id ) {
            int32_t nearestDistance = INT32_MAX;

            for ( uint8_t colorId = startGrayScaleColorId; colorId <= endGrayScaleColorId; ++colorId ) {
                const int32_t redDiff = static_cast<int32_t>( palette[id * 3] ) - static_cast<int32_t>( originalPalette[static_cast<size_t>( colorId ) * 3] ) * 4;
                const int32_t greenDiff
                    = static_cast<int32_t>( palette[id * 3 + 1] ) - static_cast<int32_t>( originalPalette[static_cast<size_t>( colorId ) * 3 + 1] ) * 4;
                const int32_t blueDiff
                    = static_cast<int32_t>( palette[id * 3 + 2] ) - static_cast<int32_t>( originalPalette[static_cast<size_t>( colorId ) * 3 + 2] ) * 4;

                const int32_t distance = redDiff * redDiff + greenDiff * greenDiff + blueDiff * blueDiff;
                if ( nearestDistance > distance ) {
                    nearestDistance = distance;
                    assignedValue[id] = colorId;
                }
            }
        }

        std::array<uint8_t, paletteSize> endPalette{ 0 };
        for ( size_t i = 0; i < paletteIndexes; ++i ) {
            const uint8_t valuePosition = assignedValue[i] * 3;
            // Red color.
            endPalette[i * 3] = originalPalette[valuePosition] * 4;
            // Green color.
            endPalette[i * 3 + 1] = originalPalette[valuePosition + 1] * 4;
            // Blue color.
            endPalette[i * 3 + 2] = originalPalette[valuePosition + 2] * 4;
        }

        // Gradually fade the palette.
        const uint32_t delay = 1000 / fps;
        const uint32_t gradingSteps = durationMs / delay;
        uint32_t gradingId = 1;
        std::vector<uint8_t> gradingPalette( paletteSize );
        std::vector<uint8_t> prevPalette( paletteSize );

        const fheroes2::ScreenPaletteRestorer screenRestorer;
        fheroes2::Display & display = fheroes2::Display::instance();
        LocalEvent & le = LocalEvent::Get();

        Game::passCustomAnimationDelay( delay );

        while ( le.HandleEvents( Game::isCustomDelayNeeded( delay ) ) ) {
            if ( Game::validateCustomAnimationDelay( delay ) ) {
                if ( gradingId == gradingSteps ) {
                    break;
                }

                for ( int32_t i = 0; i < paletteSize; ++i ) {
                    gradingPalette[i] = static_cast<uint8_t>( ( palette[i] * ( gradingSteps - gradingId ) + endPalette[i] * gradingId ) / gradingSteps );
                }

                screenRestorer.changePalette( gradingPalette.data() );

                // We need to swap the palettes so the next call of 'changePalette()' will think that the palette is new.
                std::swap( prevPalette, gradingPalette );

                display.render( frameRoi );

                ++gradingId;
            }
        }

        // Convert all video frame colors to original game palette colors.
        fheroes2::ApplyPalette( display, frameRoi.x, frameRoi.y, display, frameRoi.x, frameRoi.y, frameRoi.width, frameRoi.height, assignedValue );
        screenRestorer.changePalette( originalPalette );
    }

    void CreateDeathWaveEffect( Image & out, const Image & in, const int32_t x, const std::vector<int32_t> & deathWaveCurve )
    {
        if ( in.empty() ) {
            return;
        }

        const int32_t inWidth = in.width();
        const int32_t waveLength = static_cast<int32_t>( deathWaveCurve.size() );

        // If the death wave curve is outside of the battlefield - return.
        if ( x < 0 || ( x - waveLength ) >= inWidth || deathWaveCurve.empty() ) {
            return;
        }

        const int32_t inHeight = in.height();
        const int32_t outWaveWidth = x > waveLength ? ( x > inWidth ? ( waveLength - x + inWidth ) : waveLength ) : x;

        // If the out image is small for the Death Wave spell effect, resize it and fill the transform layer with "0".
        if ( out.width() < outWaveWidth || out.height() < inHeight ) {
            out.resize( outWaveWidth, inHeight );
            std::fill( out.transform(), out.transform() + static_cast<size_t>( outWaveWidth * inHeight ), static_cast<uint8_t>( 0 ) );
        }

        const int32_t outWidth = out.width();

        // Set the input image horizontal offset from where to draw the wave.
        const int32_t offsetX = x < waveLength ? 0 : x - waveLength;
        const uint8_t * inImageX = in.image() + offsetX;

        uint8_t * outImageX = out.image();

        // Set pointers to the start and the end of the death wave curve.
        std::vector<int32_t>::const_iterator pntX = deathWaveCurve.begin() + ( x < waveLength ? waveLength - x : 0 );
        const std::vector<int32_t>::const_iterator endX = deathWaveCurve.end() - ( x > inWidth ? x - inWidth : 0 );

        for ( ; pntX != endX; ++pntX, ++outImageX, ++inImageX ) {
            // The death curve should have only negative values and should not be higher, than the height of 'in' image.
            if ( ( *pntX >= 0 ) || ( *pntX <= -inHeight ) ) {
                assert( 0 );
                continue;
            }

            const uint8_t * outImageYEnd = outImageX + static_cast<ptrdiff_t>( inHeight + *pntX ) * outWidth;
            const uint8_t * inImageY = inImageX - static_cast<ptrdiff_t>( *pntX + 1 ) * inWidth;

            // A loop to shift all horizontal pixels vertically.
            uint8_t * outImageY = outImageX;
            for ( ; outImageY != outImageYEnd; outImageY += outWidth ) {
                inImageY += inWidth;
                *outImageY = *inImageY;
            }

            // Flip the image under the death wave to create a distortion effect.
            for ( int32_t i = 0; i > *pntX; --i ) {
                *outImageY = *inImageY;
                outImageY += outWidth;
                inImageY -= inWidth;
            }
        }
    }

    Image CreateHolyShoutEffect( const Image & in, const int32_t blurRadius, const uint8_t darkredStrength )
    {
        if ( in.empty() ) {
            return {};
        }

        if ( blurRadius < 1 ) {
            return in;
        }

        const int32_t width = in.width();
        const int32_t height = in.height();

        // To properly call 'GetColorId' we need to multiply the RGB color values by a coefficient equal to 4. Also the Holy Word/Shout spell effect
        // should have some dark + red effect. So we reduce all these coefficients for all colors, for green and blue we make a stronger reduction.
        const double redCoeff = 4.0 - darkredStrength / 280.0;
        const double greenBlueCoeff = 4.0 - darkredStrength / 80.0;

        Image out( width, height );
        std::fill( out.transform(), out.transform() + static_cast<size_t>( width * height ), static_cast<uint8_t>( 0 ) );

        uint8_t * imageOutX = out.image();
        const uint8_t * imageIn = in.image();

        const uint8_t * gamePalette = getGamePalette();

        // The spell effect represents as a blurry image. The blur algorithm should blur only horizontally and vertically from current pixel.
        // So the color data is averaged in a "cross" around the current pixel (not a square or circle like other blur algorithms).
        for ( int32_t y = 0; y < height; ++y ) {
            const int32_t startY = std::max( y - blurRadius, 0 );
            const int32_t rangeY = std::min( y + blurRadius + 1, height ) - startY;
            const uint8_t * imageInXStart = imageIn + static_cast<ptrdiff_t>( y ) * width;
            const uint8_t * imageInYStart = imageIn + static_cast<ptrdiff_t>( startY ) * width;

            for ( int32_t x = 0; x < width; ++x, ++imageOutX ) {
                const int32_t startX = std::max( x - blurRadius, 0 );
                const int32_t rangeX = std::min( x + blurRadius + 1, width ) - startX;

                uint32_t sumRed = 0;
                uint32_t sumGreen = 0;
                uint32_t sumBlue = 0;

                const uint8_t * imageInX = imageInXStart + startX;
                const uint8_t * imageInXEnd = imageInX + rangeX;

                for ( ; imageInX != imageInXEnd; ++imageInX ) {
                    const uint8_t * palette = gamePalette + static_cast<ptrdiff_t>( *imageInX ) * 3;

                    sumRed += *palette;
                    sumGreen += *( palette + 1 );
                    sumBlue += *( palette + 2 );
                }

                const uint8_t * imageInY = imageInYStart + x;
                const uint8_t * imageInYEnd = imageInY + static_cast<ptrdiff_t>( rangeY ) * width;
                const uint8_t * currentPixel = imageInXStart + x;

                for ( ; imageInY != imageInYEnd; imageInY += width ) {
                    if ( imageInY != currentPixel ) {
                        const uint8_t * palette = gamePalette + static_cast<ptrdiff_t>( *imageInY ) * 3;

                        sumRed += *palette;
                        sumGreen += *( palette + 1 );
                        sumBlue += *( palette + 2 );
                    }
                }

                const uint32_t roiSize = static_cast<uint32_t>( rangeX + rangeY - 1 );
                *imageOutX = GetColorId( static_cast<uint8_t>( redCoeff * sumRed / roiSize ), static_cast<uint8_t>( greenBlueCoeff * sumGreen / roiSize ),
                                         static_cast<uint8_t>( greenBlueCoeff * sumBlue / roiSize ) );
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

        const fheroes2::Rect roi{ pos.x, pos.y, shadow.width(), shadow.height() };

        while ( alpha > min + endAlpha ) {
            ApplyAlpha( top, shadow, alpha );
            Copy( shadow, 0, 0, display, roi.x, roi.y, roi.width, roi.height );

            display.render( roi );

            alpha -= step;
            // TODO: we should deduct from sleeping delay the time we spent for preparing and rendering the frame.
            delayforMs( stepDelay );
        }
    }

    void FadeDisplayWithPalette( const Image & top, const Point & pos, uint8_t paletteId, int delayMs, int frameCount )
    {
        Display & display = Display::instance();
        const int stepDelay = delayMs / frameCount;

        Image shadow = top;
        const fheroes2::Rect roi{ pos.x, pos.y, shadow.width(), shadow.height() };

        for ( int i = 0; i < frameCount; ++i ) {
            ApplyPalette( shadow, paletteId );
            Copy( shadow, 0, 0, display, roi.x, roi.y, roi.width, roi.height );

            display.render( roi );
            // TODO: we should deduct from sleeping delay the time we spent for preparing and rendering the frame.
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

            display.render( roi );
            // TODO: we should deduct from sleeping delay the time we spent for preparing and rendering the frame.
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
