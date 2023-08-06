/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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

#ifndef H2VIEWWORLD_H
#define H2VIEWWORLD_H

#include <array>
#include <cstdint>

#include "math_base.h"
#include "settings.h"

namespace Interface
{
    class BaseInterface;
}

enum class ViewWorldMode : int32_t
{
    OnlyVisible = 0, // Only show what is currently not under fog of war

    ViewArtifacts = 1,
    ViewMines = 2,
    ViewResources = 3,
    ViewHeroes = 4,
    ViewTowns = 5,

    ViewAll = 6,
};

class ViewWorld
{
public:
    static void ViewWorldWindow( const int32_t color, const ViewWorldMode mode, Interface::BaseInterface & interface );

    struct ZoomROIs
    {
        ZoomROIs( const ZoomLevel zoomLevel, const fheroes2::Point & centerInPixels );

        bool zoomIn( const bool cycle );
        bool zoomOut( const bool cycle );
        bool ChangeCenter( const fheroes2::Point & centerInPixels );

        const fheroes2::Rect & GetROIinPixels() const;
        fheroes2::Rect GetROIinTiles() const;

        ZoomLevel _zoomLevel{ ZoomLevel::ZoomLevel1 };
        fheroes2::Point _center;
        std::array<fheroes2::Rect, 4> _roiForZoomLevels;

    private:
        void _updateZoomLevels();
        bool _updateCenter();
        bool _changeZoom( const ZoomLevel newLevel );
    };
};

#endif
