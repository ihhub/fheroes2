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

class Sprite;

enum scroll_t
{
    SCROLL_NONE = 0x00,
    SCROLL_LEFT = 0x01,
    SCROLL_RIGHT = 0x02,
    SCROLL_TOP = 0x04,
    SCROLL_BOTTOM = 0x08
};

enum level_t
{
    LEVEL_BOTTOM = 0x01,
    LEVEL_TOP = 0x02,
    LEVEL_HEROES = 0x04,
    LEVEL_OBJECTS = 0x08,
    LEVEL_FOG = 0x20,

    LEVEL_ALL = 0xFF
};

typedef std::map<std::pair<int, uint32_t>, Sprite> MapObjectSprite;

namespace Interface
{
    class Basic;

    class GameArea
    {
    public:
        GameArea( Basic & );
        void Build( void );

        const Rect & GetROI( void ) const; // returns visible Game Area ROI in pixels
        Rect GetVisibleTileROI( void ) const;
        void ShiftCenter( const Point & offset ); // in pixels

        int GetScrollCursor( void ) const;
        bool NeedScroll( void ) const;
        void Scroll( void );
        void SetScroll( int );

        void SetCenter( const Point & );
        void SetRedraw( void ) const;

        void Redraw( fheroes2::Image & dst, int ) const;

        void BlitOnTile( fheroes2::Image & src, const fheroes2::Image & dst, int32_t ox, int32_t oy, const Point & mp, bool flip = false, uint8_t alpha = 255 ) const;
        void BlitOnTile( fheroes2::Image & src, const fheroes2::Sprite & dst, const Point & mp ) const;

        void SetUpdateCursor( void );
        void QueueEventProcessing( void );

        Rect RectFixed( Point & dst, int rw, int rh ) const;

        static fheroes2::Image GenerateUltimateArtifactAreaSurface( s32 );

        int32_t GetValidTileIdFromPoint( const Point & point ) const; // returns -1 in case of invalid index (out of World Map)
        Point GetRelativeTilePosition( const Point & tileId ) const; // in relation to screen

        void ResetCursorPosition();

    private:
        void SetAreaPosition( s32, s32, u32, u32 );

        Basic & interface;

        Rect _windowROI; // visible to draw area of World Map in pixels
        Point _topLeftTileOffset; // offset of tiles to be drawn (from here we can find any tile ID)

        // boundaries for World Map
        int16_t _minLeftOffset;
        int16_t _maxLeftOffset;
        int16_t _minTopOffset;
        int16_t _maxTopOffset;

        Size _visibleTileCount; // number of tiles to be drawn on screen

        int32_t _prevIndexPos;
        int scrollDirection;
        bool updateCursor;

        SDL::Time scrollTime;

        Point _middlePoint() const; // returns middle point of window ROI
        Point _getStartTileId() const;
        void _setCenterToTile( const Point & tile ); // set center to the middle of tile (input is tile ID)
        void _setCenter( const Point & point ); // in pixels
        Point _getRelativePosition( const Point & point ) const; // returns relative to screen position
    };
}

#endif
