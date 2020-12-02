/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include <sstream>

#include "direction.h"
#include "maps.h"

std::string Direction::String( int direct )
{
    const char * str_direct[] = {"unknown", "center", "top", "top right", "right", "bottom right", "bottom", "bottom left", "left", "top left"};
    std::ostringstream os;

    if ( direct & CENTER )
        os << str_direct[1] << ",";
    if ( direct & TOP )
        os << str_direct[2] << ",";
    if ( direct & TOP_RIGHT )
        os << str_direct[3] << ",";
    if ( direct & RIGHT )
        os << str_direct[4] << ",";
    if ( direct & BOTTOM_RIGHT )
        os << str_direct[5] << ",";
    if ( direct & BOTTOM )
        os << str_direct[6] << ",";
    if ( direct & BOTTOM_LEFT )
        os << str_direct[7] << ",";
    if ( direct & LEFT )
        os << str_direct[8] << ",";
    if ( direct & TOP_LEFT )
        os << str_direct[9] << ",";

    const std::string & res = os.str();

    return res.empty() ? str_direct[0] : res;
}

bool Direction::isDiagonal( int direction )
{
    return ( direction & DIRECTION_ALL_CORNERS ) != 0;
}

bool Direction::ShortDistanceClockWise( int from, int to )
{
    switch ( from ) {
    case TOP:
        switch ( to ) {
        case TOP_RIGHT:
        case RIGHT:
        case BOTTOM_RIGHT:
        case BOTTOM:
            return true;

        default:
            break;
        }
        break;

    case TOP_RIGHT:
        switch ( to ) {
        case RIGHT:
        case BOTTOM_RIGHT:
        case BOTTOM:
        case BOTTOM_LEFT:
            return true;

        default:
            break;
        }
        break;

    case RIGHT:
        switch ( to ) {
        case BOTTOM_RIGHT:
        case BOTTOM:
        case BOTTOM_LEFT:
        case LEFT:
            return true;

        default:
            break;
        }
        break;

    case BOTTOM_RIGHT:
        switch ( to ) {
        case BOTTOM:
        case BOTTOM_LEFT:
        case LEFT:
        case TOP_LEFT:
            return true;

        default:
            break;
        }
        break;

    case BOTTOM:
        switch ( to ) {
        case BOTTOM_LEFT:
        case LEFT:
        case TOP_LEFT:
            return true;

        default:
            break;
        }
        break;

    case BOTTOM_LEFT:
        switch ( to ) {
        case TOP:
        case TOP_RIGHT:
        case LEFT:
        case TOP_LEFT:
            return true;

        default:
            break;
        }
        break;

    case LEFT:
        switch ( to ) {
        case TOP:
        case TOP_RIGHT:
        case RIGHT:
        case TOP_LEFT:
            return true;

        default:
            break;
        }
        break;

    case TOP_LEFT:
        switch ( to ) {
        case TOP:
        case TOP_RIGHT:
        case RIGHT:
        case BOTTOM_RIGHT:
            return true;

        default:
            break;
        }
        break;

    default:
        break;
    }

    return false;
}

int Direction::Reflect( int direct )
{
    switch ( direct ) {
    case TOP_LEFT:
        return BOTTOM_RIGHT;
    case TOP:
        return BOTTOM;
    case TOP_RIGHT:
        return BOTTOM_LEFT;
    case RIGHT:
        return LEFT;
    case BOTTOM_RIGHT:
        return TOP_LEFT;
    case BOTTOM:
        return TOP;
    case BOTTOM_LEFT:
        return TOP_RIGHT;
    case LEFT:
        return RIGHT;
    case CENTER:
        return CENTER;
    default:
        break;
    }

    return UNKNOWN;
}

const Directions & Direction::All( void )
{
    static const Directions allDirections = Directions( {TOP_LEFT, TOP, TOP_RIGHT, RIGHT, BOTTOM_RIGHT, BOTTOM, BOTTOM_LEFT, LEFT} );
    return allDirections;
}
