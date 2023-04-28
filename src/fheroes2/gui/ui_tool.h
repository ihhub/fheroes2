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
#include <functional>
#include <vector>

#include "image.h"
#include "math_base.h"
#include "timing.h"
#include "ui_base.h"

namespace fheroes2
{
    class MovableSprite : public Sprite
    {
    public:
        MovableSprite();
        MovableSprite( int32_t width_, int32_t height_, int32_t x_, int32_t y_ );
        explicit MovableSprite( const Sprite & sprite );
        ~MovableSprite() override;

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

        void setPosition( int32_t x_, int32_t y_ ) override;

    private:
        ImageRestorer _restorer;
        bool _isHidden;
    };

    class TimedEventValidator : public ActionObject
    {
    public:
        explicit TimedEventValidator( std::function<bool()> verification, const uint64_t delayBeforeFirstUpdateMs = 500, const uint64_t delayBetweenUpdateMs = 100 );
        ~TimedEventValidator() override = default;

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

        ~GameInterfaceTypeRestorer();

        GameInterfaceTypeRestorer( const GameInterfaceTypeRestorer & ) = delete;
        GameInterfaceTypeRestorer & operator=( const GameInterfaceTypeRestorer & ) = delete;

        const bool isEvilInterface;

        const bool isOriginalEvilInterface;
    };

    // Fade display image colors to grayscale part of default game palette.
    void colorFade( const std::vector<uint8_t> & palette, const fheroes2::Rect & frameRoi, const uint32_t durationMs, const double fps );

    void CreateDeathWaveEffect( Image & out, const Image & in, const int32_t x, const std::vector<int32_t> & deathWaveCurve );

    Image CreateHolyShoutEffect( const Image & in, const int32_t blurRadius, const uint8_t darkredStrength );

    Image CreateRippleEffect( const Image & in, const int32_t frameId, const double scaleX = 0.05, const double waveFrequency = 20.0 );

    // Fade out the whole screen.
    void fadeOutDisplay( const int32_t fadeTimeMs = 150, const uint32_t frameCount = 6 );

    void fadeOutDisplay( const Rect & roi, const bool halfFade = false, const int32_t fadeTimeMs = 150, const uint32_t frameCount = 6 );

    // Fade in the prepared image in display instance on the whole screen.
    void fadeInDisplay( const int32_t fadeTimeMs = 150, const uint32_t frameCount = 6 );

    void fadeInDisplay( const Rect & roi, const bool halfFade = false, const int32_t fadeTimeMs = 150, const uint32_t frameCount = 6 );

    void FadeDisplayWithPalette( const Image & top, const Point & pos, const uint8_t paletteId, const int32_t fadeTimeMs, const int32_t frameCount );

    void InvertedFadeWithPalette( Image & image, const Rect & roi, const Rect & excludedRoi, const uint8_t paletteId, const int32_t fadeTimeMs,
                                  const int32_t frameCount );

    void InvertedShadow( Image & image, const Rect & roi, const Rect & excludedRoi, const uint8_t paletteId, const int32_t paletteCount );

    // Display pre-render function to show screen system info
    void PreRenderSystemInfo();

    // Display post-render function to hide screen system info
    void PostRenderSystemInfo();
}
