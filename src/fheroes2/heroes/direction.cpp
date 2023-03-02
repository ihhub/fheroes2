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

#include "direction.h"

std::string Direction::String( int direct )
{
    std::string temp;

    if ( direct & CENTER ) {
        temp += "center, ";
    }
    if ( direct & TOP ) {
        temp += "top, ";
    }
    if ( direct & TOP_RIGHT ) {
        temp += "top-right, ";
    }
    if ( direct & RIGHT ) {
        temp += "right, ";
    }
    if ( direct & BOTTOM_RIGHT ) {
        temp += "bottom-right, ";
    }
    if ( direct & BOTTOM ) {
        temp += "bottom, ";
    }
    if ( direct & BOTTOM_LEFT ) {
        temp += "bottom-left, ";
    }
    if ( direct & LEFT ) {
        temp += "left, ";
    }
    if ( direct & TOP_LEFT ) {
        temp += "top-left, ";
    }

    if ( !temp.empty() && temp.back() == ' ' ) {
        temp.pop_back();
    }

    if ( !temp.empty() && temp.back() == ',' ) {
        temp.pop_back();
    }

    return temp.empty() ? "unknown" : temp;
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

const Directions & Direction::All()
{
    static const Directions allDirections = Directions( { TOP_LEFT, TOP, TOP_RIGHT, RIGHT, BOTTOM_RIGHT, BOTTOM, BOTTOM_LEFT, LEFT } );
    return allDirections;
}
