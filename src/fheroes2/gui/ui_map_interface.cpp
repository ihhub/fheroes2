/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#include "ui_map_interface.h"

#include <algorithm>
#include <cassert>

#include "gamedefs.h"
#include "maps.h"

namespace Interface
{
    fheroes2::Rect getPopupWindowPosition( const fheroes2::Point & mousePos, const fheroes2::Rect & interfaceArea, const fheroes2::Size & windowSize )
    {
        // If this assertion blows up then you are trying to render a window bigger than the interface area.
        assert( interfaceArea.width >= windowSize.width && interfaceArea.height >= windowSize.height );

        fheroes2::Point windowsPos{ ( ( mousePos.x - BORDERWIDTH ) / TILEWIDTH ) * TILEWIDTH + TILEWIDTH - ( windowSize.width / 2 ),
                                    ( ( mousePos.y - BORDERWIDTH ) / TILEWIDTH ) * TILEWIDTH + TILEWIDTH - ( windowSize.height / 2 ) };

        // Clamp area to the edges of the area.
        windowsPos.x = std::clamp( windowsPos.x, BORDERWIDTH, ( interfaceArea.width - windowSize.width ) + BORDERWIDTH );
        windowsPos.y = std::clamp( windowsPos.y, BORDERWIDTH, ( interfaceArea.height - windowSize.height ) + BORDERWIDTH );

        return { windowsPos.x, windowsPos.y, windowSize.width, windowSize.height };
    }
}
