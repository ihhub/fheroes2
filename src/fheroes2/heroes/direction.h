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
#ifndef H2DIRECTION_H
#define H2DIRECTION_H

#include <string>
#include <vector>

typedef std::vector<int> Directions;

namespace Direction
{
    enum
    {
        UNKNOWN = 0x0000,
        TOP_LEFT = 0x0001,
        TOP = 0x0002,
        TOP_RIGHT = 0x0004,
        RIGHT = 0x0008,
        BOTTOM_RIGHT = 0x0010,
        BOTTOM = 0x0020,
        BOTTOM_LEFT = 0x0040,
        LEFT = 0x0080,
        CENTER = 0x0100
    };

    std::string String( int );
    bool isDiagonal( int direction );

    int Reflect( int direct );

    bool ShortDistanceClockWise( int direct1, int direct2 );
    const Directions & All( void );
}

#define DIRECTION_TOP_ROW ( Direction::TOP_LEFT | Direction::TOP | Direction::TOP_RIGHT )
#define DIRECTION_BOTTOM_ROW ( Direction::BOTTOM_LEFT | Direction::BOTTOM | Direction::BOTTOM_RIGHT )
#define DIRECTION_CENTER_ROW ( Direction::LEFT | Direction::CENTER | Direction::RIGHT )
#define DIRECTION_LEFT_COL ( Direction::TOP_LEFT | Direction::LEFT | Direction::BOTTOM_LEFT )
#define DIRECTION_CENTER_COL ( Direction::TOP | Direction::CENTER | Direction::BOTTOM )
#define DIRECTION_RIGHT_COL ( Direction::TOP_RIGHT | Direction::RIGHT | Direction::BOTTOM_RIGHT )
#define DIRECTION_ALL ( DIRECTION_TOP_ROW | DIRECTION_BOTTOM_ROW | DIRECTION_CENTER_ROW )
#define DIRECTION_AROUND ( DIRECTION_TOP_ROW | DIRECTION_BOTTOM_ROW | Direction::LEFT | Direction::RIGHT )

#define DIRECTION_TOP_RIGHT_CORNER ( Direction::TOP | Direction::TOP_RIGHT | Direction::RIGHT )
#define DIRECTION_TOP_LEFT_CORNER ( Direction::TOP | Direction::TOP_LEFT | Direction::LEFT )
#define DIRECTION_BOTTOM_RIGHT_CORNER ( Direction::BOTTOM | Direction::BOTTOM_RIGHT | Direction::RIGHT )
#define DIRECTION_BOTTOM_LEFT_CORNER ( Direction::BOTTOM | Direction::BOTTOM_LEFT | Direction::LEFT )
#define DIRECTION_ALL_CORNERS ( Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM_LEFT | Direction::TOP_LEFT )

#endif
