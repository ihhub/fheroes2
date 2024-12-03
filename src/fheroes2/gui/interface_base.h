/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

#include "game_mode.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "math_base.h"
#include "screen.h"
#include "ui_constants.h"

namespace Interface
{
    enum Redraw : uint32_t
    {
        // The next values are base for game/editor.

        // To render the cursor over the previously generated radar map image.
        REDRAW_RADAR_CURSOR = 0x01,
        // To render radar map fully or in ROI and then the cursor over it.
        REDRAW_RADAR = 0x02,
        REDRAW_BORDER = 0x04,
        REDRAW_GAMEAREA = 0x08,
        REDRAW_STATUS = 0x10,

        // The next values are base for game (Adventure Map) interface.
        REDRAW_HEROES = 0x20,
        REDRAW_CASTLES = 0x40,
        REDRAW_BUTTONS = 0x80,

        REDRAW_ICONS = REDRAW_HEROES | REDRAW_CASTLES,

        // The next value is base for Map Editor interface.
        REDRAW_PANEL = 0x100,

        REDRAW_ALL = 0x1FF,

        // This option is only for the Editor.
        REDRAW_PASSABILITIES = 0x200
    };

    class BaseInterface
    {
    public:
        explicit BaseInterface( const bool isEditor_ )
            : _gameArea( *this )
            , _radar( *this )
            , _isEditor( isEditor_ )
        {
            // Do nothing
        }

        virtual ~BaseInterface() = default;

        virtual void redraw( const uint32_t force ) = 0;

        bool needRedraw() const
        {
            return _redraw != 0;
        }

        void setRedraw( const uint32_t r )
        {
            _redraw |= r;
        }

        uint32_t getRedrawMask() const
        {
            return _redraw;
        }

        static bool isScrollLeft( const fheroes2::Point & cursorPos )
        {
            return cursorPos.x < fheroes2::borderWidthPx;
        }

        static bool isScrollRight( const fheroes2::Point & cursorPos )
        {
            const fheroes2::Display & display = fheroes2::Display::instance();

            return cursorPos.x >= display.width() - fheroes2::borderWidthPx;
        }

        static bool isScrollTop( const fheroes2::Point & cursorPos )
        {
            return cursorPos.y < fheroes2::borderWidthPx;
        }

        static bool isScrollBottom( const fheroes2::Point & cursorPos )
        {
            const fheroes2::Display & display = fheroes2::Display::instance();

            return cursorPos.y >= display.height() - fheroes2::borderWidthPx;
        }

        GameArea & getGameArea()
        {
            return _gameArea;
        }

        Radar & getRadar()
        {
            return _radar;
        }

        static fheroes2::GameMode EventExit();

        virtual bool useMouseDragMovement()
        {
            return true;
        }

        virtual void mouseCursorAreaClickLeft( const int32_t tileIndex ) = 0;
        virtual void mouseCursorAreaPressRight( const int32_t tileIndex ) const = 0;
        virtual void mouseCursorAreaLongPressLeft( const int32_t tileIndex ) = 0;

        // Regenerates the game area and updates the panel positions depending on the UI settings
        virtual void reset() = 0;

        virtual void updateCursor( const int32_t tileIndex ) = 0;

        bool isEditor() const
        {
            return _isEditor;
        }

        // Call this function to plan (set) redraw if fade in is not set
        // or redraw the given interface items and do fade in with them rendered.
        void renderWithFadeInOrPlanRender( const uint32_t redrawMask );

    protected:
        // If display fade-in state is set reset it to false and fade-in the full display image. Otherwise render full display image without fade-in.
        void validateFadeInAndRender();

        GameArea _gameArea;
        Radar _radar;

        uint32_t _redraw{ 0 };

        const bool _isEditor{ false };
    };
}
