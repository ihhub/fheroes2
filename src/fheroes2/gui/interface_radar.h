/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#ifndef H2INTERFACE_RADAR_H
#define H2INTERFACE_RADAR_H

#include <cstdint>

#include "image.h"
#include "interface_border.h"
#include "math_base.h"
#include "ui_tool.h"
#include "view_world.h"

namespace fheroes2
{
    class Display;
}

namespace Interface
{
    class BaseInterface;

    class Radar final : public BorderWindow
    {
    public:
        explicit Radar( BaseInterface & interface );
        // Creates a radar with a fixed position at the top right of the screen,
        // based on an existing radar and suitable for the View World window
        Radar( const Radar & radar, const fheroes2::Display & display );
        Radar( const Radar & ) = delete;

        ~Radar() override = default;

        Radar & operator=( const Radar & ) = delete;

        void SetPos( int32_t x, int32_t y ) override;

        // Set the render redraw flag from Interface::Redraw enumeration:
        // - 'REDRAW_RADAR' - to redraw the radar map image fully or in ROI and render the cursor over it.
        // - 'REDRAW_RADAR_CURSOR' - to render the previously generated radar map image and the cursor over it.
        void SetRedraw( const uint32_t redrawMode ) const;

        // Set the "need" of render the radar map only in the given 'roi' on next radar Redraw call.
        void SetRenderArea( const fheroes2::Rect & roi );
        void Build();
        void RedrawForViewWorld( const ViewWorld::ZoomROIs & roi, ViewWorldMode mode, const bool renderMapObjects );
        void redrawForEditor( const bool renderMapObjects );

        void SetHide( bool f )
        {
            _hide = f;
        }

        void QueueEventProcessing();
        bool QueueEventProcessingForWorldView( ViewWorld::ZoomROIs & roi ) const;

        // Do not call this method directly, use Interface::AdventureMap::redraw() instead to avoid issues in the "no interface" mode.
        // The name of this method starts from _ on purpose to do not mix with other public methods.
        void _redraw( const bool redrawMapObjects );

    private:
        enum class RadarType : char
        {
            WorldMap,
            ViewWorld
        };

        void SavePosition() override;
        void SetZoom();

        void RedrawObjects( const int32_t playerColor, const ViewWorldMode flags );
        void RedrawCursor( const fheroes2::Rect * roiRectangle = nullptr );

        RadarType _radarType;
        BaseInterface & _interface;

        fheroes2::Image _map;
        fheroes2::MovableSprite _cursorArea;
        fheroes2::Rect _roi;
        double _zoom{ 1.0 };
        bool _hide{ true };
    };
}

#endif
