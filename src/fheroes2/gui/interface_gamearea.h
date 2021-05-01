/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#ifndef H2INTERFACE_GAMEAREA_H
#define H2INTERFACE_GAMEAREA_H

#include "gamedefs.h"
#include "image.h"
#include "timing.h"

namespace Interface
{
    class Basic;

    enum ScollingType
    {
        SCROLL_NONE = 0x00,
        SCROLL_LEFT = 0x01,
        SCROLL_RIGHT = 0x02,
        SCROLL_TOP = 0x04,
        SCROLL_BOTTOM = 0x08
    };

    enum RedrawLevelType
    {
        LEVEL_BOTTOM = 0x01,
        LEVEL_TOP = 0x02,
        LEVEL_HEROES = 0x04,
        LEVEL_OBJECTS = 0x08,
        LEVEL_FOG = 0x20,
        LEVEL_ROUTES = 0x40,

        LEVEL_ALL = 0xFF
    };

    class GameArea
    {
    public:
        explicit GameArea( Basic & );
        void Build( void );

        const fheroes2::Rect & GetROI() const // returns visible Game Area ROI in pixels
        {
            return _windowROI;
        }

        // Do NOT use this method directly in heavy computation loops
        fheroes2::Rect GetVisibleTileROI( void ) const;

        void ShiftCenter( const fheroes2::Point & offset ); // in pixels

        int GetScrollCursor( void ) const;

        bool NeedScroll() const
        {
            return scrollDirection != 0;
        }

        void Scroll( void );
        void SetScroll( int );

        void SetCenter( const fheroes2::Point & point );

        // Do not call this method unless it's needed for manual setup of the position
        void SetCenterInPixels( const fheroes2::Point & point );

        void SetRedraw( void ) const;

        void Redraw( fheroes2::Image & dst, int flag, bool isPuzzleDraw = false ) const;

        void BlitOnTile( fheroes2::Image & dst, const fheroes2::Image & src, int32_t ox, int32_t oy, const fheroes2::Point & mp, bool flip = false,
                         uint8_t alpha = 255 ) const;
        void BlitOnTile( fheroes2::Image & dst, const fheroes2::Sprite & src, const fheroes2::Point & mp ) const;

        // Use this method to draw TIL images
        void DrawTile( fheroes2::Image & src, const fheroes2::Image & dst, const fheroes2::Point & mp ) const;

        void SetUpdateCursor( void )
        {
            updateCursor = true;
        }

        void QueueEventProcessing( void );

        fheroes2::Rect RectFixed( fheroes2::Point & dst, int rw, int rh ) const;

        static fheroes2::Image GenerateUltimateArtifactAreaSurface( int32_t index );

        int32_t GetValidTileIdFromPoint( const fheroes2::Point & point ) const; // returns -1 in case of invalid index (out of World Map)
        fheroes2::Point GetRelativeTilePosition( const fheroes2::Point & tileId ) const; // in relation to screen

        void ResetCursorPosition()
        {
            _prevIndexPos = -1;
        }

        void SetAreaPosition( int32_t, int32_t, int32_t, int32_t );

    private:
        Basic & interface;

        fheroes2::Rect _windowROI; // visible to draw area of World Map in pixels
        fheroes2::Point _topLeftTileOffset; // offset of tiles to be drawn (from here we can find any tile ID)

        // boundaries for World Map
        int32_t _minLeftOffset;
        int32_t _maxLeftOffset;
        int32_t _minTopOffset;
        int32_t _maxTopOffset;

        fheroes2::Size _visibleTileCount; // number of tiles to be drawn on screen

        int32_t _prevIndexPos;
        int scrollDirection;
        bool updateCursor;

        fheroes2::Time scrollTime;

        fheroes2::Point _middlePoint() const; // returns middle point of window ROI
        fheroes2::Point _getStartTileId() const;
        void _setCenterToTile( const fheroes2::Point & tile ); // set center to the middle of tile (input is tile ID)
    };
}

#endif
