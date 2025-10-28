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

#include "ui_tool.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "agg_image.h"
#include "cursor.h"
#include "game_delays.h"
#include "icn.h"
#include "image_palette.h"
#include "localevent.h"
#include "pal.h"
#include "race.h"
#include "render_processor.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_text.h"

namespace
{
    // The parameters of display fade effect. Full dark and full bright alpha values.
    const uint8_t fullDarkAlpha = 0;
    const uint8_t fullBrightAlpha = 255;
    // Fade-in and fade-out effects are made in separate functions. Here we set the duration of a single function.
    const uint32_t screenFadeTimeMs = 100;
    // For 100 ms duration 6 frames will result in 60 FPS.
    const uint32_t screenFadeFrameCount = 6;
    const uint8_t screenFadeStep = ( fullBrightAlpha - fullDarkAlpha ) / screenFadeFrameCount;

    void fadeDisplay( const uint8_t startAlpha, const uint8_t endAlpha, const fheroes2::Rect & roi, const uint32_t fadeTimeMs, const uint32_t frameCount )
    {
        if ( frameCount < 2 || roi.height <= 0 || roi.width <= 0 ) {
            return;
        }

        Cursor::Get().SetThemes( Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::Rect fadeRoi( roi ^ fheroes2::Rect( 0, 0, display.width(), display.height() ) );

        fheroes2::Image temp{ fadeRoi.width, fadeRoi.height };
        Copy( display, fadeRoi.x, fadeRoi.y, temp, 0, 0, fadeRoi.width, fadeRoi.height );

        double alpha = startAlpha;
        const uint32_t delay = fadeTimeMs / frameCount;
        const double alphaStep = ( alpha - endAlpha ) / static_cast<double>( frameCount - 1 );

        uint32_t frameNumber = 0;

        LocalEvent & le = LocalEvent::Get();

        Game::passCustomAnimationDelay( delay );
        while ( le.HandleEvents( Game::isCustomDelayNeeded( delay ) ) ) {
            if ( Game::validateCustomAnimationDelay( delay ) ) {
                if ( frameNumber == frameCount ) {
                    break;
                }

                assert( alpha >= 0 && alpha <= 255 );

                const uint8_t fadeAlpha = static_cast<uint8_t>( std::round( alpha ) );

                if ( fadeAlpha == 255 ) {
                    // This alpha is for fully bright image so there is no need to apply alpha.
                    Copy( temp, 0, 0, display, fadeRoi.x, fadeRoi.y, fadeRoi.width, fadeRoi.height );
                }
                else if ( fadeAlpha == 0 ) {
                    // This alpha is for fully dark image so fill it with the black color.
                    // Color index '0' in all game palettes (including videos) corresponds to the black color.
                    Fill( display, fadeRoi.x, fadeRoi.y, fadeRoi.width, fadeRoi.height, 0 );
                }
                else {
                    ApplyAlpha( temp, 0, 0, display, fadeRoi.x, fadeRoi.y, fadeRoi.width, fadeRoi.height, fadeAlpha );
                }

                display.render( fadeRoi );

                alpha -= alphaStep;
                ++frameNumber;
            }
        }
    }
}

namespace fheroes2
{
    MovableSprite::MovableSprite()
        : _restorer( Display::instance(), 0, 0, 0, 0 )
    {
        // Do nothing.
    }

    MovableSprite::MovableSprite( const int32_t width, const int32_t height, const int32_t x, const int32_t y )
        : Sprite( width, height, x, y )
        , _restorer( Display::instance(), x, y, width, height )
        , _isHidden( height == 0 && width == 0 )
    {
        // Do nothing.
    }

    MovableSprite::MovableSprite( const Sprite & sprite )
        : Sprite( sprite )
        , _restorer( Display::instance(), 0, 0, 0, 0 )
        , _isHidden( false )
    {
        // Do nothing.
    }

    MovableSprite::~MovableSprite()
    {
        if ( _isHidden ) {
            _restorer.reset();
        }
    }

    MovableSprite & MovableSprite::operator=( const Sprite & sprite )
    {
        if ( this == &sprite ) {
            return *this;
        }

        Sprite::operator=( sprite );

        _restorer.update( x(), y(), width(), height() );

        return *this;
    }

    void MovableSprite::setPosition( const int32_t x, const int32_t y )
    {
        if ( _isHidden ) {
            Sprite::setPosition( x, y );
            return;
        }

        hide();
        Sprite::setPosition( x, y );
        show();
    }

    void MovableSprite::show()
    {
        if ( _isHidden ) {
            _restorer.update( x(), y(), width(), height() );
            Blit( *this, Display::instance(), x(), y() );
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

    void MovableText::drawInRoi( const int32_t x, const int32_t y, const Rect & roi )
    {
        hide();

        assert( _text != nullptr );

        Rect textArea = _text->area();
        textArea.x += x;
        textArea.y += y;

        // Not to cut off the top of diacritic signs in capital letters we shift the text down.
        const int32_t extraShiftY = textArea.y < roi.y ? roi.y - textArea.y : 0;
        textArea.height += extraShiftY;

        const Rect overlappedRoi = textArea ^ roi;

        _restorer.update( overlappedRoi.x, overlappedRoi.y, overlappedRoi.width, overlappedRoi.height );
        _text->drawInRoi( x, y + extraShiftY, _output, overlappedRoi );

        _isHidden = false;
    }

    TextInputField::TextInputField( const Rect & textArea, const bool isMultiLine, const bool isCenterAligned, Image & output,
                                    const std::optional<SupportedLanguage> language )
        : _output( output )
        , _text( FontType::normalWhite(), textArea.width, isMultiLine, language )
        , _cursor( getCursorSprite( FontType::normalWhite() ) )
        // We enlarge background to have space for cursor at text edges and space for diacritics.
        , _background( output, textArea.x - 1, textArea.y - 2, textArea.width + 2, textArea.height + 2 )
        , _textInputArea( textArea )
        , _isSingleLineTextCenterAligned( !isMultiLine && isCenterAligned )
    {
        // Do nothing.
    }

    bool TextInputField::eventProcessing()
    {
        if ( !Game::validateAnimationDelay( Game::DelayType::CURSOR_BLINK_DELAY ) ) {
            return false;
        }

        if ( _cursor.isHidden() ) {
            _cursor.show();
        }
        else {
            _cursor.hide();
        }

        return true;
    }

    void TextInputField::draw( const std::string & newText, const int32_t cursorPositionInText )
    {
        _cursor.hide();
        _background.restore();

        _text.set( newText, cursorPositionInText );

        // Multi-line text is currently always automatically center-aligned.
        const int32_t offsetX = _isSingleLineTextCenterAligned ? _textInputArea.x + ( _textInputArea.width - _text.width() ) / 2 : _textInputArea.x;
        const int32_t offsetY = _textInputArea.y + 2;

        _text.drawInRoi( offsetX, offsetY, _output, _background.rect() );

        _cursor.setPosition( _text.cursorArea().x + offsetX, _text.cursorArea().y + offsetY );
        _cursor.show();

        Game::AnimateResetDelay( Game::DelayType::CURSOR_BLINK_DELAY );
    }

    SystemInfoRenderer::SystemInfoRenderer()
        : _startTime( std::chrono::steady_clock::now() )
        , _text( fheroes2::Display::instance() )
    {}

    void SystemInfoRenderer::preRender()
    {
        const int32_t offsetX = 26;
        const int32_t offsetY = fheroes2::Display::instance().height() - 30;

        const tm tmi = System::GetTM( std::time( nullptr ) );

        std::array<char, 9> mbstr{ 0 };
        std::strftime( mbstr.data(), mbstr.size(), "%H:%M:%S", &tmi );

        std::string info( mbstr.data() );

        const std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();
        const std::chrono::duration<double> time = endTime - _startTime;
        _startTime = endTime;

        const double totalTime = time.count() * 1000.0;
        const double fps = totalTime < 1 ? 0 : 1000 / totalTime;

        _fps.push_front( fps );
        while ( _fps.size() > 10 ) {
            _fps.pop_back();
        }

        double averageFps = 0;
        for ( const double value : _fps ) {
            averageFps += value;
        }

        averageFps /= static_cast<double>( _fps.size() );
        const int32_t currentFps = static_cast<int32_t>( averageFps );

        info += _( ", FPS: " );
        info += std::to_string( currentFps );
        if ( averageFps < 10 ) {
            info += '.';
            info += std::to_string( static_cast<int32_t>( ( averageFps - currentFps ) * 10 ) );
        }

        _text.update( std::make_unique<fheroes2::Text>( std::move( info ), fheroes2::FontType::normalWhite() ) );
        _text.draw( offsetX, offsetY );
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
        RenderProcessor::instance().stopColorCycling();
    }

    ScreenPaletteRestorer::~ScreenPaletteRestorer()
    {
        Display::instance().changePalette( nullptr );
        RenderProcessor::instance().startColorCycling();
    }

    void ScreenPaletteRestorer::changePalette( const uint8_t * palette ) const
    {
        Display::instance().changePalette( palette );
    }

    GameInterfaceTypeRestorer::GameInterfaceTypeRestorer( const InterfaceType interfaceType_ )
        : interfaceType( interfaceType_ )
        , originalInterfaceType( Settings::Get().getInterfaceType() )
    {
        if ( interfaceType != originalInterfaceType ) {
            Settings::Get().setInterfaceType( interfaceType_ );
        }
    }

    GameInterfaceTypeRestorer::~GameInterfaceTypeRestorer()
    {
        if ( interfaceType != originalInterfaceType ) {
            Settings::Get().setInterfaceType( originalInterfaceType );
        }
    }

    void colorFade( const std::vector<uint8_t> & palette, const fheroes2::Rect & frameRoi, const uint32_t durationMs, const double fps )
    {
        assert( fps > 0 );

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

        const uint32_t delay = static_cast<uint32_t>( std::round( 1000.0 / fps ) );

        // Gradually fade the palette.
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

        // If the out image is small for the Death Wave spell effect, resize it.
        if ( out.width() < outWaveWidth || out.height() < inHeight ) {
            out.resize( outWaveWidth, inHeight );
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

        Image out;
        out._disableTransformLayer();
        out.resize( width, height );

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

    Sprite createRippleEffect( const Sprite & in, const int32_t amplitudeInPixels, const double phaseAtImageTop, const int32_t periodInPixels )
    {
        if ( in.empty() || in.singleLayer() || amplitudeInPixels == 0 ) {
            return in;
        }

        const int32_t widthIn = in.width();
        const int32_t height = in.height();

        Sprite out( widthIn + amplitudeInPixels * 2, height, in.x() - amplitudeInPixels, in.y() );
        out.reset();

        const int32_t widthOut = out.width();

        uint8_t * outImageY = out.image();
        uint8_t * outTransformY = out.transform();

        const uint8_t * inImageY = in.image();
        const uint8_t * inTransformY = in.transform();

        for ( int32_t y = 0; y < height; ++y, inImageY += widthIn, inTransformY += widthIn, outImageY += widthOut, outTransformY += widthOut ) {
            // Calculate sin starting at `phaseAtImageTop` with period set by `periodInPixels`, result is in interval [-1.0, 1.0].
            const double sinResult = std::sin( ( 2.0 * M_PI ) * y / periodInPixels + phaseAtImageTop );
            const int32_t offset = static_cast<int32_t>( std::round( amplitudeInPixels * ( sinResult + 1.0 ) ) );

            memcpy( outImageY + offset, inImageY, widthIn );
            memcpy( outTransformY + offset, inTransformY, widthIn );
        }

        return out;
    }

    void fadeOutDisplay()
    {
        const Display & display = Display::instance();

        fadeOutDisplay( { 0, 0, display.width(), display.height() }, false );
    }

    void fadeOutDisplay( const Rect & roi, const bool halfFade )
    {
        static_assert( screenFadeFrameCount != 0 );

        // As we are doing fade-out we already have the full bright picture so we skip it and calculate the startAlpha.
        const uint8_t startAlpha = fullBrightAlpha - screenFadeStep;

        if ( halfFade ) {
            fadeDisplay( startAlpha, ( fullBrightAlpha - fullDarkAlpha ) / 2, roi, screenFadeTimeMs / 2, screenFadeFrameCount / 2 );
        }
        else {
            fadeDisplay( startAlpha, fullDarkAlpha, roi, screenFadeTimeMs, screenFadeFrameCount );
        }
    }

    void fadeInDisplay()
    {
        const Display & display = Display::instance();

        fadeInDisplay( { 0, 0, display.width(), display.height() }, false );
    }

    void fadeInDisplay( const Rect & roi, const bool halfFade )
    {
        static_assert( screenFadeFrameCount != 0 );

        // As we are doing fade-in we already have the full dark picture from the previous fade-out so we skip it and calculate the startAlpha.
        const uint8_t startAlpha = fullDarkAlpha + screenFadeStep;

        if ( halfFade ) {
            // We add an extra frame to have a smoother fade effect taking into account that the last frame is fully bright to render the image.
            fadeDisplay( ( fullBrightAlpha - startAlpha ) / 2, fullBrightAlpha, roi, screenFadeTimeMs * 3 / 4, screenFadeFrameCount / 2 + 1 );
        }
        else {
            fadeDisplay( startAlpha, fullBrightAlpha, roi, screenFadeTimeMs, screenFadeFrameCount );
        }
    }

    void FadeDisplayWithPalette( const Image & top, const Point & pos, const uint8_t paletteId, const int32_t fadeTimeMs, const int32_t frameCount )
    {
        Display & display = Display::instance();
        const int32_t stepDelay = fadeTimeMs / frameCount;

        Image shadow = top;
        const fheroes2::Rect roi{ pos.x, pos.y, shadow.width(), shadow.height() };

        LocalEvent & le = LocalEvent::Get();
        int32_t frameNumber = 0;

        Game::passCustomAnimationDelay( stepDelay );
        while ( le.HandleEvents( Game::isCustomDelayNeeded( stepDelay ) ) ) {
            if ( Game::validateCustomAnimationDelay( stepDelay ) ) {
                if ( frameNumber == frameCount ) {
                    break;
                }

                ApplyPalette( shadow, paletteId );
                Copy( shadow, 0, 0, display, roi.x, roi.y, roi.width, roi.height );

                display.render( roi );

                ++frameNumber;
            }
        }
    }

    void InvertedFadeWithPalette( Image & image, const Rect & roi, const Rect & excludedRoi, const uint8_t paletteId, const int32_t fadeTimeMs, const int32_t frameCount )
    {
        Display & display = Display::instance();
        const int32_t stepDelay = fadeTimeMs / frameCount;

        LocalEvent & le = LocalEvent::Get();
        int32_t frameNumber = 0;

        Game::passCustomAnimationDelay( stepDelay );
        while ( le.HandleEvents( Game::isCustomDelayNeeded( stepDelay ) ) ) {
            if ( Game::validateCustomAnimationDelay( stepDelay ) ) {
                if ( frameNumber == frameCount ) {
                    break;
                }

                InvertedShadow( image, roi, excludedRoi, paletteId, 1 );

                display.render( roi );

                ++frameNumber;
            }
        }
    }

    void InvertedShadow( Image & image, const Rect & roi, const Rect & excludedRoi, const uint8_t paletteId, const int32_t paletteCount )
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

    std::optional<int32_t> processIntegerValueTyping( const int32_t min, const int32_t max, std::string & valueBuf )
    {
        assert( min <= max );

        const LocalEvent & le = LocalEvent::Get();

        if ( !le.isAnyKeyPressed() ) {
            return {};
        }

        const int32_t zeroBufValue = std::clamp( 0, min, max );

        if ( le.isKeyPressed( fheroes2::Key::KEY_BACKSPACE ) || le.isKeyPressed( fheroes2::Key::KEY_DELETE ) ) {
            valueBuf.clear();

            return zeroBufValue;
        }

        if ( le.isKeyPressed( fheroes2::Key::KEY_MINUS ) || le.isKeyPressed( fheroes2::Key::KEY_KP_MINUS ) ) {
            if ( min >= 0 ) {
                return {};
            }

            if ( !std::all_of( valueBuf.begin(), valueBuf.end(), []( const char ch ) { return ( ch == '0' ); } ) ) {
                return {};
            }

            valueBuf = "-";

            return zeroBufValue;
        }

        if ( const std::optional<char> newDigit = [&le]() -> std::optional<char> {
                 using KeyUnderlyingType = std::underlying_type_t<fheroes2::Key>;

                 const fheroes2::Key keyValue = le.getPressedKeyValue();

                 if ( keyValue >= fheroes2::Key::KEY_0 && keyValue <= fheroes2::Key::KEY_9 ) {
                     return fheroes2::checkedCast<char>( static_cast<KeyUnderlyingType>( keyValue ) - static_cast<KeyUnderlyingType>( fheroes2::Key::KEY_0 ) + '0' );
                 }

                 if ( keyValue >= fheroes2::Key::KEY_KP_0 && keyValue <= fheroes2::Key::KEY_KP_9 ) {
                     return fheroes2::checkedCast<char>( static_cast<KeyUnderlyingType>( keyValue ) - static_cast<KeyUnderlyingType>( fheroes2::Key::KEY_KP_0 ) + '0' );
                 }

                 return {};
             }();
             newDigit ) {
            valueBuf.push_back( *newDigit );

            const std::optional<int32_t> value = [&valueBuf = std::as_const( valueBuf )]() -> std::optional<int32_t> {
                try {
                    return std::stoi( valueBuf );
                }
                catch ( std::out_of_range & ) {
                    return {};
                }
            }();

            if ( !value || ( min <= 0 && value < min ) || ( max >= 0 && value > max ) ) {
                valueBuf.pop_back();

                return {};
            }

            if ( value == std::clamp( *value, min, max ) ) {
                return value;
            }

            return {};
        }

        return {};
    }

    void renderHeroRacePortrait( const int race, const fheroes2::Rect & portPos, fheroes2::Image & output )
    {
        fheroes2::Image racePortrait( portPos.width, portPos.height );

        auto preparePortrait = [&racePortrait, &portPos]( const int icnId, const int bkgIndex, const bool applyRandomPalette ) {
            fheroes2::SubpixelResize( fheroes2::AGG::GetICN( ICN::STRIP, bkgIndex ), racePortrait );
            const fheroes2::Sprite & heroSprite = fheroes2::AGG::GetICN( icnId, 1 );
            if ( applyRandomPalette ) {
                fheroes2::Sprite tmp = heroSprite;
                fheroes2::ApplyPalette( tmp, PAL::GetPalette( PAL::PaletteType::PURPLE ) );
                fheroes2::Blit( tmp, 0, std::max( 0, tmp.height() - portPos.height ), racePortrait, ( portPos.width - tmp.width() ) / 2,
                                std::max( 0, portPos.height - tmp.height() ), tmp.width(), portPos.height );
            }
            else {
                fheroes2::Blit( heroSprite, 0, std::max( 0, heroSprite.height() - portPos.height ), racePortrait, ( portPos.width - heroSprite.width() ) / 2,
                                std::max( 0, portPos.height - heroSprite.height() ), heroSprite.width(), portPos.height );
            }
        };

        switch ( race ) {
        case Race::KNGT:
            preparePortrait( ICN::CMBTHROK, 4, false );
            break;
        case Race::BARB:
            preparePortrait( ICN::CMBTHROB, 5, false );
            break;
        case Race::SORC:
            preparePortrait( ICN::CMBTHROS, 6, false );
            break;
        case Race::WRLK:
            preparePortrait( ICN::CMBTHROW, 7, false );
            break;
        case Race::WZRD:
            preparePortrait( ICN::CMBTHROZ, 8, false );
            break;
        case Race::NECR:
            preparePortrait( ICN::CMBTHRON, 9, false );
            break;
        case Race::RAND:
            preparePortrait( ICN::CMBTHROW, 10, true );
            break;
        default:
            // Have you added a new race? Correct the logic above!
            assert( 0 );
            break;
        }
        fheroes2::Copy( racePortrait, 0, 0, output, portPos );
    }

    std::vector<LocalizedString> getLocalizedStrings( std::string text, const SupportedLanguage currentLanguage, const std::string_view toReplace,
                                                      std::string_view replacement, const SupportedLanguage replacementLanguage )
    {
        if ( currentLanguage == replacementLanguage ) {
            StringReplace( text, toReplace.data(), replacement );
            return { { std::move( text ), currentLanguage } };
        }

        // Check whether the replacement text even exists.
        const std::string::size_type pos = text.find( toReplace );
        if ( pos == std::string::npos ) {
            return { { std::move( text ), currentLanguage } };
        }

        std::vector<LocalizedString> strings;

        strings.emplace_back( text.substr( 0, pos ), currentLanguage );
        strings.emplace_back( std::string( replacement ), replacementLanguage );
        strings.emplace_back( text.substr( pos + toReplace.size() ), currentLanguage );

        return strings;
    }

    std::unique_ptr<TextBase> getLocalizedText( std::vector<LocalizedString> texts, const FontType font )
    {
        if ( texts.empty() ) {
            return {};
        }

        if ( texts.size() == 1 ) {
            return std::make_unique<Text>( std::move( texts.front().text ), font, texts.front().language );
        }

        auto multiFontText = std::make_unique<MultiFontText>();
        for ( auto & text : texts ) {
            multiFontText->add( Text( std::move( text.text ), font, text.language ) );
        }

        return multiFontText;
    }

    std::unique_ptr<TextBase> getLocalizedText( std::vector<std::pair<LocalizedString, FontType>> texts )
    {
        if ( texts.empty() ) {
            return {};
        }

        if ( texts.size() == 1 ) {
            auto & [text, font] = texts.front();
            return std::make_unique<Text>( std::move( text.text ), font, text.language );
        }

        auto multiFontText = std::make_unique<MultiFontText>();
        for ( auto & [text, font] : texts ) {
            multiFontText->add( Text( std::move( text.text ), font, text.language ) );
        }

        return multiFontText;
    }
}
