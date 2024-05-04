/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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
#include <vector>

#include "math_base.h"
#include "mp2.h"

class Heroes;

namespace fheroes2
{
    class Image;
    struct ObjectRenderingInfo;
}

namespace Interface
{
    class GameArea;
}

namespace Maps
{
    class Tiles;
    struct TilesAddon;

    void redrawEmptyTile( fheroes2::Image & dst, const fheroes2::Point & mp, const Interface::GameArea & area );

    void redrawTopLayerExtraObjects( const Tiles & tile, fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area );
    void redrawTopLayerObject( const Tiles & tile, fheroes2::Image & dst, const bool isPuzzleDraw, const Interface::GameArea & area, const TilesAddon & addon );

    void drawFog( const Tiles & tile, fheroes2::Image & dst, const Interface::GameArea & area );

    void redrawPassable( const Tiles & tile, fheroes2::Image & dst, const int friendColors, const Interface::GameArea & area, const bool isEditor );

    void redrawBottomLayerObjects( const Tiles & tile, fheroes2::Image & dst, bool isPuzzleDraw, const Interface::GameArea & area, const uint8_t level );

    void drawByObjectIcnType( const Tiles & tile, fheroes2::Image & output, const Interface::GameArea & area, const MP2::ObjectIcnType objectIcnType );

    std::vector<fheroes2::ObjectRenderingInfo> getMonsterSpritesPerTile( const Tiles & tile, const bool isEditorMode );
    std::vector<fheroes2::ObjectRenderingInfo> getMonsterShadowSpritesPerTile( const Tiles & tile, const bool isEditorMode );
    std::vector<fheroes2::ObjectRenderingInfo> getBoatSpritesPerTile( const Tiles & tile );
    std::vector<fheroes2::ObjectRenderingInfo> getBoatShadowSpritesPerTile( const Tiles & tile );
    std::vector<fheroes2::ObjectRenderingInfo> getMineGuardianSpritesPerTile( const Tiles & tile );
    std::vector<fheroes2::ObjectRenderingInfo> getHeroSpritesPerTile( const Heroes & hero );
    std::vector<fheroes2::ObjectRenderingInfo> getHeroShadowSpritesPerTile( const Heroes & hero );
    std::vector<fheroes2::ObjectRenderingInfo> getEditorHeroSpritesPerTile( const Tiles & tile );

    const fheroes2::Image & getTileSurface( const Tiles & tile );
}
