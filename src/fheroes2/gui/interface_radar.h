/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
    class Basic;

    class Radar final : public BorderWindow
    {
    public:
        explicit Radar( Basic & );
        // Creates a radar with a fixed position at the top right of the screen,
        // based on an existing radar and suitable for the View World window
        Radar( const Radar & radar, const fheroes2::Display & display );
        Radar( const Radar & ) = delete;

        ~Radar() override = default;

        Radar & operator=( const Radar & ) = delete;

        void SetPos( int32_t ox, int32_t oy ) override;
        void SetRedraw() const;
        void Build();
        void RedrawForViewWorld( const ViewWorld::ZoomROIs & roi, ViewWorldMode mode );

        void SetHide( bool f )
        {
            hide = f;
        }

        void QueueEventProcessing();
        bool QueueEventProcessingForWorldView( ViewWorld::ZoomROIs & roi ) const;

    private:
        friend Basic;

        enum class RadarType : char
        {
            WorldMap,
            ViewWorld
        };

        void SavePosition() override;
        void Generate();

        // Do not call this method directly, use Interface::Basic::Redraw() instead
        // to avoid issues in the "no interface" mode
        void Redraw();
        void RedrawObjects( int color, ViewWorldMode flags ) const;
        void RedrawCursor( const fheroes2::Rect * roiRectangle = nullptr );

        RadarType radarType;
        Basic & interface;

        fheroes2::Image spriteArea;
        fheroes2::MovableSprite cursorArea;
        fheroes2::Point offset;
        bool hide;
    };
}

#endif
