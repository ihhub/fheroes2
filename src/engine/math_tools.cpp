/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

#include "math_tools.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

double fheroes2::GetAngle( const Point & start, const Point & target )
{
    const int dx = target.x - start.x;
    const int dy = target.y - start.y;
    double angle = atan2( -dy, dx ) * 180.0 / M_PI;
    // we only care about two quadrants, normalize
    if ( dx < 0 ) {
        angle = ( dy <= 0 ) ? 180 - angle : -angle - 180;
    }
    return angle;
}

std::vector<fheroes2::Point> fheroes2::GetEuclideanLine( const Point & pt1, const Point & pt2, const uint32_t step )
{
    const int dx = pt2.x - pt1.x;
    const int dy = pt2.y - pt1.y;
    const uint32_t dist = static_cast<uint32_t>( std::hypot( dx, dy ) );
    // Round up the integer division and avoid the division by zero in calculation of total line points.
    const uint32_t length = ( step > 0 ) ? ( dist + step / 2 ) / step : 0;

    std::vector<Point> line;

    if ( length < 2 ) {
        // If the length is equal to 0 than 'pt2' could be closer to 'pt1' than 'step'.
        // In this case we put 'pt1' as the start of the line.
        line.emplace_back( pt1 );
        // And put 'pt2' as the end of the line only if 'pt1' is not equal to 'pt2'.
        if ( pt1 != pt2 ) {
            line.emplace_back( pt2 );
        }
    }
    else {
        // Otherwise we calculate the euclidean line, using the determined parameters.
        const double moveX = dx / static_cast<double>( length );
        const double moveY = dy / static_cast<double>( length );

        line.reserve( length + 1 );

        for ( uint32_t i = 0; i <= length; ++i ) {
            line.emplace_back( static_cast<int>( pt1.x + i * moveX ), static_cast<int>( pt1.y + i * moveY ) );
        }
    }
    return line;
}

std::vector<fheroes2::Point> fheroes2::GetLinePoints( const Point & pt1, const Point & pt2, const int32_t step )
{
    std::vector<Point> res;

    const int32_t dx = std::abs( pt2.x - pt1.x );
    const int32_t dy = std::abs( pt2.y - pt1.y );

    int32_t ns = ( dx > dy ? dx : dy ) / 2;

    Point pt( pt1 );

    for ( int32_t i = 0; i <= ( dx > dy ? dx : dy ); ++i ) {
        if ( dx > dy ) {
            pt.x < pt2.x ? ++pt.x : --pt.x;
            ns -= dy;
        }
        else {
            pt.y < pt2.y ? ++pt.y : --pt.y;
            ns -= dx;
        }

        if ( ns < 0 ) {
            if ( dx > dy ) {
                pt.y < pt2.y ? ++pt.y : --pt.y;
                ns += dx;
            }
            else {
                pt.x < pt2.x ? ++pt.x : --pt.x;
                ns += dy;
            }
        }

        if ( 0 == ( i % step ) )
            res.push_back( pt );
    }

    return res;
}

std::vector<fheroes2::Point> fheroes2::GetArcPoints( const Point & from, const Point & to, const int32_t arcHeight, const int32_t step )
{
    std::vector<Point> res;
    Point pt( from );
    // The first projectile point is "from"
    res.push_back( pt );

    // Calculate the number of projectile trajectory points
    const int32_t steps = ( to.x - from.x ) / step;

    // Trajectory start point coordinates
    const double x1 = from.x;
    const double y1 = from.y;

    // Distance to the destination point along the axes
    const double dx = to.x - x1;
    const double dy = to.y - y1;

    // The movement of the projectile is determined according to the parabolic
    // throwing approximation. The first two parabola points are "from" and
    // "to" with an exception that the second ("to") point is at the same
    // height as the start point. The parabola third point "y" coordinate is
    // set using the "arcHeight" parameter, which determines the height of the
    // parabola arc. And its "x" coordinate is taken equal to half the path
    // from the start point to the end point. Using this three point
    // coordinates, a system of three linear equations (y=a*x*x+b*x+c) in
    // three variables is solved by substituting these points "x" and "y".
    // Considering that on an isometric battlefield, the target location above
    // or below corresponds to a simple turn of the shooter to the left or
    // right, a linear movement from point "from" to point "to" is added to the
    // parabola ('dy/dx' in 'b' constant and '-x1*dy/dx' in 'c' constant).

    // Calculation of the parabola equation coefficients
    const double a = 4 * arcHeight / dx / dx;
    const double b = dy / dx - a * ( dx + 2 * x1 );
    const double c = y1 + a * x1 * ( dx + x1 ) - x1 * dy / dx;

    for ( int32_t i = 1; i <= steps; ++i ) {
        pt.x += step;
        pt.y = static_cast<int32_t>( std::lround( a * pt.x * pt.x + b * pt.x + c ) );
        res.push_back( pt );
    }

    return res;
}

int32_t fheroes2::GetRectIndex( const std::vector<Rect> & rects, const Point & pt )
{
    for ( size_t i = 0; i < rects.size(); ++i ) {
        if ( rects[i] & pt )
            return static_cast<int32_t>( i );
    }

    return -1;
}

fheroes2::Rect fheroes2::getBoundaryRect( const Rect & rt1, const Rect & rt2 )
{
    if ( rt2.width == 0 && rt2.height == 0 ) {
        return rt1;
    }

    if ( rt1.width == 0 && rt1.height == 0 ) {
        return rt2;
    }

    const int32_t x = std::min( rt1.x, rt2.x );
    const int32_t y = std::min( rt1.y, rt2.y );
    const int32_t width = std::max( rt1.x + rt1.width, rt2.x + rt2.width ) - x;
    const int32_t height = std::max( rt1.y + rt1.height, rt2.y + rt2.height ) - y;

    return { x, y, width, height };
}
