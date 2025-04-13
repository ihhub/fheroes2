/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include "math_base.h"

class Castle;
struct Funds;

namespace fheroes2
{
    class Image;

    uint32_t getCastleIcnIndex( const int race, const bool isCastle );

    uint32_t getCastleLeftFlagIcnIndex( const int color );

    void drawCastleIcon( const Castle & castle, Image & output, const Point & offset );

    Rect drawResourcePanel( const Funds & kingdomTreasures, Image & output, const Point & offset );

    void drawCastleName( const Castle & castle, Image & output, const Point & offset );

    void drawCastleDialogBuilding( const int32_t icnId, const uint32_t icnIndex, const Castle & castle, const Point & offset, const Rect & renderArea,
                                   const uint8_t alpha = 255 );
}
