/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "image.h"
#include "math_base.h"
#include "mp2.h"
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
        LEVEL_OBJECTS = 0x00, // we always render objects for each type of view.
        LEVEL_HEROES = 0x01,
        LEVEL_FOG = 0x02,
        LEVEL_ROUTES = 0x04,
        LEVEL_TOWNS = 0x08, // this level is used only for View All / View Towns spells.

        LEVEL_ALL = LEVEL_OBJECTS | LEVEL_HEROES | LEVEL_FOG | LEVEL_ROUTES
    };

    struct BaseObjectAnimationInfo
    {
        BaseObjectAnimationInfo() = default;

        BaseObjectAnimationInfo( const uint32_t uid_, const int32_t tileId_, const MP2::MapObjectType type_ )
            : uid( uid_ )
            , tileId( tileId_ )
            , type( type_ )
        {
            // Do nothing.
        }

        BaseObjectAnimationInfo( const BaseObjectAnimationInfo & ) = delete;
        BaseObjectAnimationInfo & operator=( const BaseObjectAnimationInfo & ) = delete;

        virtual ~BaseObjectAnimationInfo() = default;

        uint32_t uid{ 0 };

        int32_t tileId{ -1 };

        MP2::MapObjectType type{ MP2::OBJ_NONE };

        uint8_t alphaValue{ 255 };

        virtual bool update() = 0;

        virtual bool isAnimationCompleted() const = 0;
    };

    struct ObjectFadingOutInfo : public BaseObjectAnimationInfo
    {
        using BaseObjectAnimationInfo::BaseObjectAnimationInfo;

        ~ObjectFadingOutInfo() override;

        bool update() override
        {
            if ( alphaValue >= alphaStep ) {
                alphaValue -= alphaStep;
            }
            else {
                alphaValue = 0;
            }

            return ( alphaValue == 0 );
        }

        bool isAnimationCompleted() const override
        {
            return ( alphaValue == 0 );
        }

        static const uint8_t alphaStep{ 20 };
    };

    struct ObjectFadingInInfo : public BaseObjectAnimationInfo
    {
        ObjectFadingInInfo() = delete;

        ObjectFadingInInfo( const uint32_t uid_, const int32_t tileId_, const MP2::MapObjectType type_ )
            : BaseObjectAnimationInfo( uid_, tileId_, type_ )
        {
            alphaValue = 0;
        }

        ~ObjectFadingInInfo() override = default;

        bool update() override
        {
            if ( 255 - alphaValue >= alphaStep ) {
                alphaValue += alphaStep;
            }
            else {
                alphaValue = 255;
            }

            return ( alphaValue == 255 );
        }

        bool isAnimationCompleted() const override
        {
            return ( alphaValue == 255 );
        }

        static const uint8_t alphaStep{ 20 };
    };

    class GameArea
    {
    public:
        explicit GameArea( Basic & basic );
        GameArea( const GameArea & ) = default;
        GameArea( GameArea && ) = delete;

        ~GameArea() = default;

        GameArea & operator=( const GameArea & ) = delete;
        GameArea & operator=( GameArea && ) = delete;

        void generate( const fheroes2::Size & screenSize, const bool withoutBorders );

        const fheroes2::Rect & GetROI() const // returns visible Game Area ROI in pixels
        {
            return _windowROI;
        }

        // Do NOT use this method directly in heavy computation loops
        fheroes2::Rect GetVisibleTileROI() const
        {
            return { _getStartTileId(), _visibleTileCount };
        }

        // Shift center in pixels.
        void ShiftCenter( const fheroes2::Point & offset )
        {
            SetCenterInPixels( _topLeftTileOffset + _middlePoint() + offset );
        }

        int GetScrollCursor() const;

        bool NeedScroll() const
        {
            return scrollDirection != 0;
        }

        void Scroll();
        void SetScroll( int );

        void SetCenter( const fheroes2::Point & point )
        {
            _setCenterToTile( point );

            scrollDirection = 0;
        }

        // Do not call this method unless it's needed for manual setup of the position
        void SetCenterInPixels( const fheroes2::Point & point );

        void SetRedraw() const;

        // Do not call this method directly if the rendering takes place on the screen, use
        // Interface::Basic::Redraw() instead to avoid issues in the "no interface" mode
        void Redraw( fheroes2::Image & dst, int flag, bool isPuzzleDraw = false ) const;

        void BlitOnTile( fheroes2::Image & dst, const fheroes2::Image & src, int32_t ox, int32_t oy, const fheroes2::Point & mp, bool flip, uint8_t alpha ) const;

        void BlitOnTile( fheroes2::Image & dst, const fheroes2::Image & src, const fheroes2::Rect & srcRoi, int32_t ox, int32_t oy, const fheroes2::Point & mp, bool flip,
                         uint8_t alpha ) const;

        // Use this method to draw TIL images
        void DrawTile( fheroes2::Image & src, const fheroes2::Image & dst, const fheroes2::Point & mp ) const;

        void SetUpdateCursor()
        {
            updateCursor = true;
        }

        // Update fog directions data for entire map tiles by checking fog data for current player and its allies.
        static void updateMapFogDirections();

        void QueueEventProcessing( bool isCursorOverGamearea );

        static fheroes2::Image GenerateUltimateArtifactAreaSurface( const int32_t index, const fheroes2::Point & offset );

        int32_t GetValidTileIdFromPoint( const fheroes2::Point & point ) const; // returns -1 in case of invalid index (out of World Map)
        fheroes2::Point GetRelativeTilePosition( const fheroes2::Point & tileId ) const; // in relation to screen

        void ResetCursorPosition()
        {
            _prevIndexPos = -1;
        }

        void SetAreaPosition( int32_t, int32_t, int32_t, int32_t );

        fheroes2::Point getCurrentCenterInPixels() const
        {
            return _topLeftTileOffset + _middlePoint();
        }

        void addObjectAnimationInfo( std::shared_ptr<BaseObjectAnimationInfo> info )
        {
            _animationInfo.emplace_back( std::move( info ) );
        }

        uint8_t getObjectAlphaValue( const int32_t tileId, const MP2::MapObjectType type ) const;

        uint8_t getObjectAlphaValue( const uint32_t uid ) const;

        // Make sure you do not have a copy of this object after the execution of the method to avoid incorrect object removal in some cases.
        void runSingleObjectAnimation( const std::shared_ptr<BaseObjectAnimationInfo> & info );

        bool isDragScroll() const
        {
            return _mouseDraggingMovement;
        }

        bool needDragScrollRedraw() const
        {
            return _needRedrawByMouseDragging;
        }

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

        // This member needs to be mutable because it is modified during rendering.
        mutable std::vector<std::shared_ptr<BaseObjectAnimationInfo>> _animationInfo;

        fheroes2::Point _lastMouseDragPosition;
        bool _mouseDraggingInitiated;
        bool _mouseDraggingMovement;
        bool _needRedrawByMouseDragging;

        // Returns middle point of window ROI.
        fheroes2::Point _middlePoint() const
        {
            return { _windowROI.width / 2, _windowROI.height / 2 };
        }

        fheroes2::Point _getStartTileId() const;

        void _setCenterToTile( const fheroes2::Point & tile ); // set center to the middle of tile (input is tile ID)

        void updateObjectAnimationInfo() const;
    };
}

#endif
