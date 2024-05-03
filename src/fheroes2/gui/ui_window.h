/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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
#include <string>

#include "image.h"
#include "math_base.h"
#include "screen.h"

namespace fheroes2
{
    class Button;
    class ButtonSprite;

    // Standard window with shadow.
    class StandardWindow
    {
    public:
        enum class Padding : uint8_t
        {
            TOP_LEFT,
            TOP_CENTER,
            TOP_RIGHT,
            CENTER_LEFT,
            CENTER_CENTER,
            CENTER_RIGHT,
            BOTTOM_LEFT,
            BOTTOM_CENTER,
            BOTTOM_RIGHT
        };

        StandardWindow() = delete;
        StandardWindow( const StandardWindow & ) = delete;
        StandardWindow & operator=( const StandardWindow & ) = delete;
        StandardWindow( const int32_t width, const int32_t height, const bool renderBackground, Image & output = Display::instance() );
        StandardWindow( const int32_t x, const int32_t y, const int32_t width, const int32_t height, const bool renderBackground, Image & output = Display::instance() );
        ~StandardWindow()
        {
            Display & display = Display::instance();
            if ( &_output == &display ) {
                // The screen area of the closed window should be updated during the next '.render()' call.
                display.updateNextRenderRoi( _totalArea );
            }
        }

        // Returns the window background ROI.
        const Rect & activeArea() const
        {
            return _activeArea;
        }

        // Returns ROI that includes window background and window borders.
        const Rect & windowArea() const
        {
            return _windowArea;
        }

        // Returns ROI that includes window background, borders and window shadow.
        const Rect & totalArea() const
        {
            return _totalArea;
        }

        void render();

        void renderScrollbarBackground( const Rect & roi, const bool isEvilInterface );

        void renderButtonSprite( ButtonSprite & button, const std::string & buttonText, const int32_t buttonWidth, const Point & offset, const bool isEvilInterface,
                                 const Padding padding );
        void renderButton( Button & button, const int icnId, const uint32_t releasedIndex, const uint32_t pressedIndex, const Point & offset, const Padding padding );
        void renderOkayCancelButtons( Button & buttonOk, Button & buttonCancel, const bool isEvilInterface );

        void applyTextBackgroundShading( const Rect & roi );
        static void applyTextBackgroundShading( Image & output, const Rect & roi );

        static void renderBackgroundImage( fheroes2::Image & output, const Rect & roi, const int32_t borderOffset, const bool isEvilInterface );

    private:
        Image & _output;
        const Rect _activeArea;
        const Rect _windowArea;
        const Rect _totalArea;
        ImageRestorer _restorer;
        const bool _hasBackground{ true };

        Point _getRenderPos( const Point & offset, const Size & itemSize, const Padding padding ) const;
    };
}
