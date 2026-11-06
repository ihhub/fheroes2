/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include <memory>
#include <string>

#include "math_base.h"

class CursorRestorer;

namespace Multi
{
    // Shows a short text at the top of the adventure map for a few seconds without blocking the game.
    void showMapNote( const std::string & message );

    // Shows a text at the top of the map and the hourglass cursor until destroyed. It does nothign in self-tests.
    class MapWait
    {
    public:
        explicit MapWait( const std::string & message );
        MapWait( const MapWait & ) = delete;

        ~MapWait();

        MapWait & operator=( const MapWait & ) = delete;

    private:
        std::unique_ptr<CursorRestorer> _cursorRestorer;
    };

    // Called each time the adventure map is drawn. While waiting, it also shows the battle of an ally which can be watched.
    void drawMapNote( const fheroes2::Rect & gameArea );

    // Returns the area of the ally's battle banner, or an empty area if there is none.
    fheroes2::Rect getMapBannerArea();

    // Renders a text on a shaded strip at the top of the map. Returns the area of the strip.
    fheroes2::Rect drawMapText( const std::string & message, const fheroes2::Rect & gameArea, const int32_t offset );

    // Shows the waiting map with the given note until Enter is pressed. Used by self-tests only.
    void showMapNoteForTest( const std::string & message );
}
