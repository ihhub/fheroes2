/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "image.h"
#include "math_base.h"
#include "timing.h"
#include "ui_base.h"
#include "ui_text.h"

namespace fheroes2
{
    class MovableSprite : public Sprite
    {
    public:
        MovableSprite();
        MovableSprite( int32_t width_, int32_t height_, int32_t x_, int32_t y_ );
        explicit MovableSprite( const Sprite & sprite );

        MovableSprite( const MovableSprite & ) = delete;

        ~MovableSprite() override;

        MovableSprite & operator=( const MovableSprite & ) = delete;

        MovableSprite & operator=( const Sprite & sprite );

        void show();
        void hide();

        // In case if Display has changed.
        void redraw()
        {
            hide();
            show();
        }

        bool isHidden() const
        {
            return _isHidden;
        }

        Rect getArea() const
        {
            return { x(), y(), width(), height() };
        }

        void setPosition( int32_t x_, int32_t y_ ) override;

    protected:
        void _resetRestorer()
        {
            _restorer.reset();
        }

    private:
        ImageRestorer _restorer;
        bool _isHidden;
    };

    class MovableText
    {
    public:
        explicit MovableText( Image & output );

        MovableText( const MovableText & ) = delete;

        ~MovableText() = default;

        MovableText & operator=( const MovableText & ) = delete;

        void update( std::unique_ptr<TextBase> text );

        void draw( const int32_t x, const int32_t y );

        void hide();

    private:
        Image & _output;
        ImageRestorer _restorer;
        std::unique_ptr<TextBase> _text;
        bool _isHidden;
    };

    // Renderer of current time and FPS on screen
    class SystemInfoRenderer
    {
    public:
        SystemInfoRenderer();

        SystemInfoRenderer( const SystemInfoRenderer & ) = delete;

        ~SystemInfoRenderer() = default;

        SystemInfoRenderer & operator=( const SystemInfoRenderer & ) = delete;

        void preRender();

        void postRender()
        {
            _text.hide();
        }

    private:
        std::chrono::time_point<std::chrono::steady_clock> _startTime;
        fheroes2::MovableText _text;
        std::deque<double> _fps;
    };

    class TimedEventValidator : public ActionObject
    {
    public:
        explicit TimedEventValidator( std::function<bool()> verification, const uint64_t delayBeforeFirstUpdateMs = 500, const uint64_t delayBetweenUpdateMs = 100 );

        TimedEventValidator( const TimedEventValidator & ) = delete;

        ~TimedEventValidator() override = default;

        TimedEventValidator & operator=( const TimedEventValidator & ) = delete;

        bool isDelayPassed();

    protected:
        void senderUpdate( const ActionObject * sender ) override;

    private:
        std::function<bool()> _verification;
        fheroes2::TimeDelay _delayBetweenUpdateMs;
        fheroes2::TimeDelay _delayBeforeFirstUpdateMs;
    };

    // This class is useful for cases of playing videos only
    class ScreenPaletteRestorer
    {
    public:
        ScreenPaletteRestorer();

        ScreenPaletteRestorer( const ScreenPaletteRestorer & ) = delete;

        ~ScreenPaletteRestorer();

        ScreenPaletteRestorer & operator=( const ScreenPaletteRestorer & ) = delete;

        void changePalette( const uint8_t * palette ) const;
    };

    struct GameInterfaceTypeRestorer
    {
        GameInterfaceTypeRestorer() = delete;
        explicit GameInterfaceTypeRestorer( const bool isEvilInterface_ );

        GameInterfaceTypeRestorer( const GameInterfaceTypeRestorer & ) = delete;

        ~GameInterfaceTypeRestorer();

        GameInterfaceTypeRestorer & operator=( const GameInterfaceTypeRestorer & ) = delete;

        const bool isEvilInterface;
        const bool isOriginalEvilInterface;
    };

    // Fade display image colors to grayscale part of default game palette.
    void colorFade( const std::vector<uint8_t> & palette, const fheroes2::Rect & frameRoi, const uint32_t durationMs, const double fps );

    void CreateDeathWaveEffect( Image & out, const Image & in, const int32_t x, const std::vector<int32_t> & deathWaveCurve );

    Image CreateHolyShoutEffect( const Image & in, const int32_t blurRadius, const uint8_t darkredStrength );

    Image CreateRippleEffect( const Image & in, const int32_t frameId, const double scaleX = 0.05, const double waveFrequency = 20.0 );

    // Fade-out the whole screen.
    void fadeOutDisplay();

    // Fade-out the display image in ROI. The 'halfFade' parameter sets to do only half of fade-out: till half-darkened image.
    void fadeOutDisplay( const Rect & roi, const bool halfFade );

    // Fade-in the prepared image in display instance on the whole screen. The last frame is fully bright so it is a copy of original image.
    void fadeInDisplay();

    // Fade-in the prepared display image in ROI. The 'halfFade' parameter sets to do only half of fade-in: from the half-darkened image.
    // The last frame is fully bright so it is a copy of original image.
    void fadeInDisplay( const Rect & roi, const bool halfFade );

    void FadeDisplayWithPalette( const Image & top, const Point & pos, const uint8_t paletteId, const int32_t fadeTimeMs, const int32_t frameCount );

    void InvertedFadeWithPalette( Image & image, const Rect & roi, const Rect & excludedRoi, const uint8_t paletteId, const int32_t fadeTimeMs,
                                  const int32_t frameCount );

    // Returns the character position number in the 'text' string.
    size_t getTextInputCursorPosition( const std::string & text, const FontType fontType, const size_t currentTextCursorPosition, const int32_t pointerCursorXOffset,
                                       const int32_t textStartXOffset );

    // Returns the character position number in the text.
    size_t getTextInputCursorPosition( const Text & text, const size_t currentTextCursorPosition, const Point & pointerCursorOffset, const Rect & textRoi );

    void InvertedShadow( Image & image, const Rect & roi, const Rect & excludedRoi, const uint8_t paletteId, const int32_t paletteCount );

    bool processIntegerValueTyping( const int32_t minimum, const int32_t maximum, int32_t & value );

    // Render "hero on a horse" portrait dependent from hero race. Used in Editor.
    void renderHeroRacePortrait( const int race, const fheroes2::Rect & portPos, fheroes2::Image & output );
}
