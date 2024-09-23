/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

namespace fheroes2
{
    double GetAngle( const Point & start, const Point & target );
    std::vector<Point> GetEuclideanLine( const Point & pt1, const Point & pt2, const uint32_t step );
    std::vector<Point> GetLinePoints( const Point & pt1, const Point & pt2, const int32_t step );
    std::vector<Point> GetArcPoints( const Point & from, const Point & to, const int32_t arcHeight, const int32_t step );
    int32_t GetRectIndex( const std::vector<Rect> & rects, const Point & pt );
    Rect getBoundaryRect( const Rect & rt1, const Rect & rt2 );
}
